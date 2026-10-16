// SquachWatch-CYD — LORA, the T-Deck's landing page. See include/ui_lorahome.h.
//
// Laid out for 320x240 exactly, the lab's pixels: the dish in a 104-wide
// column on the left, the tiles, strip and waterfall in the 210 to its right,
// the two chats from 134 down, the compose line at 213, the bar at 226.
#if SQUACH_LORA && defined(TDECK_PLUS)
#include "ui_lorahome.h"
#include "ui_lorachat.h"
#include "lora_sniffer.h"
#include "lora_profiles.h"
#include "lora_radio.h"
#include "lora_meshtastic.h"
#include "lora_meshcore.h"
#include "settings.h"
#include "theme.h"
#include "privacy.h"
#include "gnss.h"
#include "squachy.h"
#include <Arduino.h>
#include <math.h>
#include <string.h>
#include <esp_heap_caps.h>

namespace {

// ---- the layout ------------------------------------------------------------
const int DISH_CX = 58, DISH_CY = 66, DISH_R = 40;
const int COL_X = 108, COL_W = 210;            // the right column
const int TILE_Y = 26, TILE_H = 30, TILE_W = 50, TILE_GAP = 3;
const int STRIP_Y = 59, STRIP_H = 46;
const int WF_Y = 107, WF_H = 20;
const int CHAT_Y = 134, CHAT_BOTTOM = 212, CHAT_ROW_H = 12;
const int COMPOSE_Y = 213, COMPOSE_H = 12;
const int BAR_Y = 226;

// ---- state -----------------------------------------------------------------
uint8_t  s_net = 0;                      // the switch: 0 Meshtastic, 1 MeshCore
char     s_compose[161] = { 0 };
uint8_t  s_composeN = 0;
char     s_status[40] = { 0 };
uint32_t s_statusAt = 0;
// Where the chat columns' tap zones are, for uiLoraHomeHit.
int      s_switchX = 240, s_switchW = 76;

uint16_t dim(uint16_t col, uint16_t t) { return Theme::blend(Theme::BG, col, t); }
uint16_t netCol(Lora::Proto p) { return p == Lora::Proto::MESHCORE ? Theme::PINK : Theme::CYAN; }

void age(uint32_t now, uint32_t ms, char* out, size_t cap) {
    const uint32_t s = (now - ms) / 1000;
    if (s < 60)         snprintf(out, cap, "%lus", (unsigned long)s);
    else if (s < 3600)  snprintf(out, cap, "%lum", (unsigned long)(s / 60));
    else if (s < 86400) snprintf(out, cap, "%luh", (unsigned long)(s / 3600));
    else                snprintf(out, cap, "%lud", (unsigned long)(s / 86400));
}

// Text in the 6x8 font, size 1: the whole page is drawn in it, like the lab.
void small(TFT_eSPI& t, int x, int y, const char* s, uint16_t col, uint16_t bg = Theme::BG) {
    t.setTextFont(1); t.setTextSize(1);
    t.setTextColor(col, bg);
    t.setCursor(x, y); t.print(s);
}
void smallClip(TFT_eSPI& t, int x, int y, const char* s, uint16_t col, int maxW) {
    char buf[64];
    snprintf(buf, sizeof buf, "%s", s);
    t.setTextFont(1); t.setTextSize(1);
    size_t n = strlen(buf);
    while (n && t.textWidth(buf) > maxW) buf[--n] = '\0';
    t.setTextColor(col, Theme::BG);
    t.setCursor(x, y); t.print(buf);
}

// Every third row a shade darker over a region: the lab's scanlines, kept
// to the instruments so the text stays crisp. Read-modify-write on the
// sprite, about seven thousand pixels a frame.
void scanlines(TFT_eSPI& t, int x, int y, int w, int h) {
    for (int yy = y + (3 - (y % 3)) % 3; yy < y + h; yy += 3)
        for (int xx = x; xx < x + w; xx++) {
            const uint16_t c = t.readPixel(xx, yy);
            if (c != Theme::BG) t.drawPixel(xx, yy, Theme::blend(c, Theme::BLACK, 72));
        }
}

// ---- the header --------------------------------------------------------------
void drawHeader(TFT_eSPI& t, uint32_t now) {
    char line[40];
    const uint8_t nodes = Lora::nodeCount();
    snprintf(line, sizeof line, "LORA  %s  %u NODES",
             Settings::loraListen() == 3 ? "BOTH" : Settings::loraListen() == 2 ? "MESHCORE" : "MESHTASTIC", (unsigned)nodes);
    t.setTextFont(1); t.setTextSize(1);
    small(t, (t.width() - t.textWidth(line)) / 2, 3, line, Theme::WHITE);
    if (Lora::mode() != Lora::Mode::OFF && (now / 600) % 2) small(t, 60, 3, "LIVE", Theme::GREEN);
    // BACK in the top-right corner, away from the compose line's switch: at
    // the bottom it was a thumb's slip from MESHCORE (asked for 2026-10-15).
    small(t, t.width() - 6 - 54, 3, "[ BACK ]", Theme::PURPLE);
    // Each network's preset, under the title, in its colour.
    char mt[32] = "MT off", mc[32] = "MC off";
    for (uint8_t i = 0; i < Lora::profileCount(); i++) {
        const Lora::Profile& p = Lora::profile(i);
        if (p.group != Lora::PG_MESH_US && p.group != Lora::PG_MESH_EU) continue;
        char mhz[12], bw[8];
        Lora::formatMHz(p.freqHz, mhz, sizeof mhz);
        Lora::formatBw(p.bwKhz10, bw, sizeof bw);
        if (p.hint == Lora::Proto::MESHTASTIC) snprintf(mt, sizeof mt, "MT %s SF%u BW%s", mhz, (unsigned)p.sf, bw);
        if (p.hint == Lora::Proto::MESHCORE)   snprintf(mc, sizeof mc, "MC %s SF%u BW%s", mhz, (unsigned)p.sf, bw);
    }
    small(t, 14, 14, mt, Settings::loraListen() & 1 ? Theme::CYAN : dim(Theme::CYAN, 110));
    small(t, 176, 14, mc, Settings::loraListen() & 2 ? Theme::PINK : dim(Theme::PINK, 110));
    // The radio-busy dot: lit while the radio is on.
    if (Lora::mode() != Lora::Mode::OFF) t.fillCircle(296, 17, 2, Theme::GREEN);
}

// ---- one snapshot a frame -------------------------------------------------------
// Everything the page shows from the radio lives behind the sniffer's lock,
// and each read gives up after 10-20 ms when the radio task is holding it --
// mid-frame, on a busy band. The page used to read straight through and draw
// whatever came back, so a timed-out read drew a frame with no nodes, no
// spikes or no chats, and the radar flashed (2026-10-16). Now the page takes
// its own copies, the hits every frame and the slower things on their own
// clocks, and keeps the last good copy whenever a read comes back empty
// because it timed out rather than because there is nothing.
Lora::Hit  s_pHits[Lora::HIT_CAP];
uint8_t    s_pHitN = 0;
const uint8_t SNAP_NODES = 32;                    // plenty for a dish this size
// The big copies, about 13 KB, in PSRAM: the T-Deck has 7.8 MB of it free
// and its internal RAM is what the radios and the frame push live on.
struct PageSnap {
    Lora::Nodes::Node nodes[SNAP_NODES], nodesTmp[SNAP_NODES];
    Lora::Msgs::Msg   keep[2][6], keepTmp[2][6];
    Lora::Packet      pkt;
};
PageSnap*  s_ps = nullptr;
uint8_t    s_pNodeN = 0;
uint32_t   s_pNodesAt = 0;
bool       s_pHavePkt = false;
uint32_t   s_pPktAt = 0;
uint8_t    s_pKept[2] = { 0, 0 };
uint16_t   s_pCount[2] = { 0, 0 };
uint32_t   s_pMsgAt = 0;

void refreshSnapshot(uint32_t now, bool force) {
    if (!s_ps) s_ps = (PageSnap*)heap_caps_calloc(1, sizeof(PageSnap), MALLOC_CAP_SPIRAM);
    if (!s_ps) return;
    {   // The hits: the ring only ever grows, so an empty answer after a full
        // one is a timeout.
        Lora::Hit tmp[Lora::HIT_CAP];
        const uint8_t n = Lora::hitTrail(tmp, Lora::HIT_CAP);
        if (n || !s_pHitN) { memcpy(s_pHits, tmp, sizeof(Lora::Hit) * n); s_pHitN = n; }
    }
    if (force || now - s_pNodesAt >= 400) {
        const uint8_t n = Lora::nodeSnapshot(s_ps->nodesTmp, SNAP_NODES);
        if (n || Lora::nodeCount() == 0) {
            memcpy(s_ps->nodes, s_ps->nodesTmp, sizeof(Lora::Nodes::Node) * n);
            s_pNodeN = n; s_pNodesAt = now;
        }
    }
    if (force || now - s_pPktAt >= 200) {
        if (Lora::packetAt(0, s_ps->pkt)) { s_pHavePkt = true; s_pPktAt = now; }
    }
    if (force || now - s_pMsgAt >= 500) {
        // A walk that hits a timeout part-way is thrown away whole.
        auto& keep = s_ps->keepTmp;
        uint8_t kept[2] = { 0, 0 };
        uint16_t count[2] = { 0, 0 };
        const int lines = (CHAT_BOTTOM - (CHAT_Y + 15)) / CHAT_ROW_H;
        const uint16_t n = Lora::msgCount();
        Lora::Msgs::Msg m;
        bool ok = true;
        for (uint16_t i = 0; i < n; i++) {
            if (!Lora::msgAt(i, m)) { ok = false; break; }
            const int c = m.proto == Lora::Proto::MESHTASTIC ? 0 : m.proto == Lora::Proto::MESHCORE ? 1 : -1;
            if (c < 0) continue;
            count[c]++;
            if (kept[c] < lines && kept[c] < 6) keep[c][kept[c]++] = m;
        }
        if (ok) {
            memcpy(s_ps->keep, keep, sizeof keep);
            s_pKept[0] = kept[0]; s_pKept[1] = kept[1];
            s_pCount[0] = count[0]; s_pCount[1] = count[1];
            s_pMsgAt = now;
        }
    }
}

// ---- the numbers -------------------------------------------------------------
struct Numbers { int16_t now, floor, peak; int16_t margin; };

Numbers numbers(uint32_t nowMs) {
    Numbers n = {};
    int8_t trail[Lora::TRAIL_CAP];
    const uint16_t tn = Lora::rssiTrail(trail, Lora::TRAIL_CAP);
    n.now = tn ? trail[tn - 1] : -128;
    n.floor = Lora::stats().noiseDbm ? Lora::stats().noiseDbm : -128;
    n.peak = -128;
    const Lora::Hit* hits = s_pHits;
    const uint8_t hn = s_pHitN;
    for (uint8_t i = 0; i < hn; i++) if (nowMs - hits[i].ms < 80000 && hits[i].rssi > n.peak) n.peak = hits[i].rssi;
    // How far the loudest packet of the last 80 s stood above the floor: a
    // link quality. NOW over the floor sits near zero on a quiet band, and a
    // quiet band is not a bad one (the first walk, 2026-10-15).
    n.margin = (n.peak > -128 && n.floor > -128) ? (int16_t)(n.peak - n.floor) : -128;
    return n;
}

void drawTiles(TFT_eSPI& t, const Numbers& n) {
    const char* keys[4] = { "NOW", "FLOOR", "PEAK", "MARGIN" };
    const int16_t vals[4] = { n.now, n.floor, n.peak, n.margin };
    const uint16_t cols[4] = { Theme::WHITE, dim(Theme::WHITE, 120), Theme::AMBER,
                               n.margin <= -128 ? dim(Theme::WHITE, 120) : n.margin >= 15 ? Theme::GREEN : n.margin >= 6 ? Theme::AMBER : Theme::RED };
    for (int i = 0; i < 4; i++) {
        const int x = COL_X + i * (TILE_W + TILE_GAP);
        t.fillRect(x, TILE_Y, TILE_W, TILE_H, 0x0862);
        t.drawRect(x, TILE_Y, TILE_W, TILE_H, 0x2946);
        t.setTextFont(1); t.setTextSize(1);
        small(t, x + (TILE_W - t.textWidth(keys[i])) / 2, TILE_Y + 3, keys[i], dim(Theme::WHITE, 110), 0x0862);
        char v[8];
        if (vals[i] <= -128) snprintf(v, sizeof v, "--");
        else if (i == 3)     snprintf(v, sizeof v, "%+d", (int)vals[i]);
        else                 snprintf(v, sizeof v, "%d", (int)vals[i]);
        t.setTextSize(2);
        t.setTextColor(cols[i], 0x0862);
        t.setCursor(x + (TILE_W - t.textWidth(v)) / 2, TILE_Y + 13); t.print(v);
        t.setTextSize(1);
    }
}

// ---- the strip and the waterfall ------------------------------------------------
// Both read the same two rings: the noise samples (one a second, 80 of them)
// and the packet hits (when, how loud, which network). The strip is the
// noise and the hits as spikes; the waterfall is the same seconds as colour.
//
// And three things to keep them alive on a quiet hill, from the Quiet Air
// Lab (2026-10-15) and the owner's notes on the first go:
//   PHOSPHOR  the noise is a solid line edge to edge, and a meteor sweeps it
//             from the left: a white-hot head, a tail fading into an
//             afterglow layer, sparks thrown off behind it. The waterfall's
//             cells hold heat that cools instead of snapping off. Real data,
//             slow glass, a little showing off.
//   SPARKLE   the waterfall is always a twinkling field in the owner's own
//             theme colours, drifting leftward, with the packets' heat drawn
//             over it; a few sparks behind the strip when it is quiet.
//             Decoration, kept dim enough that a packet always wins.
//   GHOST     the newest packet keeps echoing: its spike repeats fainter for
//             two minutes, and its own bytes rain down the waterfall. Real.
const uint32_t QUIET_MS = 8000, GHOST_MS = 120000;
const int GLOW_W = 208, GLOW_H = 44;              // inside the strip's border
const int WF_COLS = 52, WF_ROWS = 10, WF_CW = 4, WF_CH = 2;
const uint32_t WF_COL_MS = 1540;                  // 52 columns, 80 seconds

uint8_t* s_glow = nullptr;                        // PHOSPHOR afterglow, PSRAM
float    s_heat[WF_COLS][WF_ROWS];                // waterfall heat, 0..~1.5
uint8_t  s_kind[WF_COLS][WF_ROWS];                // 0 noise, 1 Meshtastic, 2 MeshCore, 3 other
uint32_t s_lastFrame = 0, s_colAt = 0, s_hitSeen = 0, s_openedAt = 0;
uint16_t s_beamIdx = 0;

int yForDbm(int16_t dbm, int top, int h) {
    // -120 at the bottom, -40 at the top.
    int v = 120 + dbm; if (v < 0) v = 0; if (v > 80) v = 80;
    return top + h - 3 - (v * (h - 6)) / 80;
}
int wfRow(int16_t dbm) {
    int v = 120 + dbm; if (v < 0) v = 0; if (v > 79) v = 79;
    return WF_ROWS - 1 - (v * WF_ROWS) / 80;
}
uint8_t kindOf(Lora::Proto p) { return p == Lora::Proto::MESHTASTIC ? 1 : p == Lora::Proto::MESHCORE ? 2 : 3; }
uint16_t kindCol(uint8_t k) { return k == 1 ? Theme::CYAN : k == 2 ? Theme::PINK : Theme::PURPLE; }
uint16_t scale565(uint16_t c, float a) {           // c at brightness a (0..1) over black
    if (a <= 0) return 0; if (a > 1) a = 1;
    return Theme::blend(0x0000, c, (uint16_t)(a * 256));
}

// When the last packet landed, or when the page opened if none has.
uint32_t lastPacketMs(const Lora::Hit* hits, uint8_t hn) {
    uint32_t last = 0;
    for (uint8_t i = 0; i < hn; i++) if (!last || (int32_t)(hits[i].ms - last) > 0) last = hits[i].ms;
    return last ? last : s_openedAt;
}

// A deposit of heat in the newest column, for a packet: hot at its row, and
// a cooler tail below it, the way the old waterfall drew its glow.
void heatPacket(int col, int16_t rssi, Lora::Proto p) {
    const int row = wfRow(rssi);
    const uint8_t k = kindOf(p);
    s_heat[col][row] = 1.5f; s_kind[col][row] = k;
    for (int r = row + 1; r < WF_ROWS; r++) {
        const float h = 0.55f / (float)(r - row);
        if (h > s_heat[col][r]) { s_heat[col][r] = h; s_kind[col][r] = k; }
    }
}

// The page opening: heat laid down from what the rings already hold, aged,
// so the waterfall opens with the last 80 seconds in it rather than empty.
void heatPrefill(uint32_t now) {
    memset(s_heat, 0, sizeof s_heat); memset(s_kind, 0, sizeof s_kind);
    int8_t trail[Lora::TRAIL_CAP];
    const uint16_t tn = Lora::rssiTrail(trail, Lora::TRAIL_CAP);
    for (uint16_t i = 0; i < tn; i++) {
        const uint32_t ago = (uint32_t)(tn - 1 - i) * 1000;
        const int col = WF_COLS - 1 - (int)(ago / WF_COL_MS);
        if (col < 0) continue;
        s_heat[col][wfRow(trail[i])] = 0.6f * expf(-(float)ago / 3000.0f) + 0.08f;
    }
    const Lora::Hit* hits = s_pHits;
    const uint8_t hn = s_pHitN;
    for (uint8_t i = 0; i < hn; i++) {
        const uint32_t ago = now - hits[i].ms;
        if (ago > 80000) continue;
        heatPacket(WF_COLS - 1 - (int)(ago / WF_COL_MS), hits[i].rssi, hits[i].proto);
        if ((int32_t)(hits[i].ms - s_hitSeen) > 0) s_hitSeen = hits[i].ms;
    }
    s_colAt = now;
}

// A ramp through the theme's own accent colours, so the field follows the
// owner's palette (Settings > APPEARANCE): u 0..1 round the loop, at
// brightness a.
uint16_t themeRamp(float u, float a) {
    const uint16_t stops[6] = { Theme::CYAN, Theme::VAPOR_BLUE, Theme::PURPLE, Theme::VAPOR_PURPLE, Theme::PINK, Theme::VAPOR_PINK };
    u -= floorf(u);
    const float x = u * 6.0f; const int i = (int)x; const float f = x - i;
    const uint16_t c = Theme::blend(stops[i % 6], stops[(i + 1) % 6], (uint16_t)(f * 256));
    return scale565(c, a);
}
// The meteor's sparks: thrown off the head, drifting back and down, fading.
struct Spark { float x, y, vx, vy; int16_t life, full; };
Spark    s_sparks[40];
uint32_t s_sparkSeed = 0x5EED;
float    sparkRand() { s_sparkSeed = s_sparkSeed * 1664525u + 1013904223u; return (s_sparkSeed >> 8) / 16777216.0f; }

// A cheap, steady hash: the same cell twinkles the same way every pass.
uint32_t mix32(uint32_t x) { x ^= x >> 16; x *= 0x7feb352dU; x ^= x >> 15; x *= 0x846ca68bU; x ^= x >> 16; return x; }
// How lit the sparkle field is: nothing while packets are arriving, then a
// two-second fade in once it has been quiet for QUIET_MS.
float sparkleLevel(uint32_t quiet) { return quiet > QUIET_MS ? fminf(1.0f, (quiet - QUIET_MS) / 2000.0f) : 0.0f; }

// ---- the rain (the Rain Lab's picks, 2026-10-15) -----------------------------------
// The DIGITAL background's rain, brought in quietly, the same way it draws:
// ASCII from the 6x8 font, a white-hot head, a trail that fades.
//   CODE FALL  faint glyph columns in the theme's colours behind the strip's
//              phosphor line; the meteor and the spikes stay in front. (It
//              fell through the waterfall too until 2026-10-16, when the
//              owner took it out of there.)
//   CHAT RAIN  a dim rain inside each message box in that network's colour,
//              cyan behind Meshtastic and pink behind MeshCore, under the
//              lines. (A backdrop through the page's background came out the
//              same day: the owner wanted it in the boxes, not around them.)
const char RAIN_GLYPHS[] = "01ABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*<>{}[]/\\|+=~SASQUACH";
const int  RAIN_GN = sizeof(RAIN_GLYPHS) - 1;
const float CODE_STRIP_A = 0.25f, CHAT_RAIN_A = 0.3f;
struct RainCol { float y, v; uint8_t len, depth; bool active; char g[16]; };
struct Rain { RainCol c[54]; uint8_t n; int16_t x0, spacing, top, h; float density; uint32_t seed; bool ready; };
Rain s_code = {}, s_chatMt = {}, s_chatMc = {};

float rainRand(Rain& R) { R.seed = R.seed * 1664525u + 1013904223u; return (R.seed >> 8) / 16777216.0f; }
char  rainGlyph(Rain& R) { return RAIN_GLYPHS[(int)(rainRand(R) * RAIN_GN) % RAIN_GN]; }

void rainInit(Rain& R, int x0, int x1, int spacing, int top, int h, float density, uint32_t seed) {
    R.x0 = x0; R.spacing = spacing; R.top = top; R.h = h; R.density = density; R.seed = seed;
    R.n = (uint8_t)((x1 - x0) / spacing); if (R.n > 54) R.n = 54;
    for (uint8_t i = 0; i < R.n; i++) {
        RainCol& c = R.c[i];
        c.len = (uint8_t)(6 + rainRand(R) * 10);
        c.y = top + rainRand(R) * (h + c.len * 8);        // already falling when the page opens
        c.v = 0.025f + rainRand(R) * 0.04f;               // 25-65 px a second
        c.depth = (uint8_t)(rainRand(R) * 4);
        c.active = rainRand(R) < density;
        for (int k = 0; k < 16; k++) c.g[k] = rainGlyph(R);
    }
    R.ready = true;
}

void rainStep(Rain& R, uint32_t dt, float speed) {
    for (uint8_t i = 0; i < R.n; i++) {
        RainCol& c = R.c[i];
        if (!c.active) {
            if (rainRand(R) < 0.0015f * dt * R.density) { c.active = true; c.y = R.top - 8; }
            continue;
        }
        c.y += c.v * speed * dt;
        if (rainRand(R) < 0.02f) c.g[(int)(rainRand(R) * 16) & 15] = rainGlyph(R);
        if (c.y - c.len * 8 > R.top + R.h) {
            c.y = R.top - 8 - rainRand(R) * 30;
            c.len = (uint8_t)(6 + rainRand(R) * 10);
            c.depth = (uint8_t)(rainRand(R) * 4);
            c.active = rainRand(R) < R.density;
        }
    }
}

// Draw the columns. `ok` says whether a glyph's 6x8 box may be drawn there
// (every caller clips with a viewport instead, today); `col` is the trail's
// colour for a column. The
// glyph is drawn with a transparent background, so only its pixels land.
template <typename OK, typename COL>
void rainDraw(TFT_eSPI& t, Rain& R, float alpha, OK ok, COL col) {
    for (uint8_t i = 0; i < R.n; i++) {
        const RainCol& c = R.c[i];
        if (!c.active) continue;
        const int x = R.x0 + i * R.spacing;
        const uint16_t trail = col(c, x);
        for (int k = 0; k < c.len; k++) {
            const int y = (int)c.y - k * 8;
            if (y < R.top - 8 || y > R.top + R.h) continue;
            if (!ok(x, y)) continue;
            const float a = alpha * (k == 0 ? 1.0f : (1.0f - (float)k / c.len) * 0.75f) * (0.55f + c.depth * 0.15f);
            if (a < 0.03f) continue;
            const uint16_t fg = scale565(k == 0 ? Theme::WHITE : trail, a);
            t.drawChar(x, y, (uint8_t)c.g[k & 15], fg, fg, 1);
        }
    }
}


void drawStrip(TFT_eSPI& t, uint32_t now, const Numbers& n, uint32_t dt) {
    const int x0 = COL_X + 1, w = COL_W - 2;
    t.fillRect(COL_X, STRIP_Y, COL_W, STRIP_H, 0x0000);
    for (int g = 1; g < 4; g++) t.drawFastHLine(x0, STRIP_Y + g * STRIP_H / 4, w, 0x0841);
    // CODE FALL, faint, behind the phosphor line: clipped to the strip.
    t.setViewport(x0, STRIP_Y + 1, w, STRIP_H - 2, false);
    rainDraw(t, s_code, CODE_STRIP_A, [](int, int) { return true; },
             [&](const RainCol&, int x) { return themeRamp((float)(x - COL_X) / COL_W + now / 9000.0f, 1.0f); });
    t.resetViewport();
    const Lora::Hit* hits = s_pHits;
    const uint8_t hn = s_pHitN;
    const uint32_t quiet = now - lastPacketMs(hits, hn);

    // SPARKLE, behind everything: a few dim twinkles drifting left, in the
    // spectrum's colours, while it is quiet.
    const float spk = sparkleLevel(quiet);
    if (spk > 0) {
        const uint32_t drift = now / 220;                 // one pixel every 0.22 s
        for (int i = 0; i < 24; i++) {
            const uint32_t h = mix32(i * 2654435761u);
            const int x = x0 + (int)((h % w + w - drift % w) % w);
            const int y = STRIP_Y + 2 + (int)((h >> 8) % (STRIP_H - 6));
            const float tw = sinf(now * (0.002f + (h % 7) * 0.0006f) + (h >> 16)) * 0.5f + 0.5f;
            const float a = tw * tw * tw * 0.45f * spk;
            if (a < 0.05f) continue;
            const float u = (float)(x - x0) / w + now / 9000.0f;
            t.drawPixel(x, y, themeRamp(u, a));
            if (a > 0.3f) { t.drawPixel(x - 1, y, themeRamp(u, a * 0.4f)); t.drawPixel(x + 1, y, themeRamp(u, a * 0.4f)); }
        }
    }

    // PHOSPHOR: the afterglow fades, then the beam draws the newest part of
    // the noise into it. Scaled to the ten dB either side of the floor, so
    // the radio's own hiss fills the strip rather than lying flat on the
    // bottom; the packets keep the full -120..-40 scale below.
    if (!s_glow) s_glow = (uint8_t*)heap_caps_calloc(GLOW_W * GLOW_H, 1, MALLOC_CAP_SPIRAM);
    int8_t trail[Lora::TRAIL_CAP];
    const uint16_t tn = Lora::rssiTrail(trail, Lora::TRAIL_CAP);
    if (s_glow) {
        const float keep = expf(-(float)dt / 350.0f);
        const uint16_t k8 = (uint16_t)(keep * 256);
        for (int i = 0; i < GLOW_W * GLOW_H; i++) if (s_glow[i]) s_glow[i] = (uint8_t)((s_glow[i] * k8) >> 8);
        const float fl = n.floor > -128 ? (float)n.floor : -110.0f;
        auto gy = [&](int8_t v) { float f = ((float)v - (fl - 10)) / 20.0f; if (f < 0) f = 0; if (f > 1) f = 1; return (int)((1 - f) * (GLOW_H - 3)) + 1; };
        // Edge to edge whatever the trail holds: a page opened a minute after
        // boot has fewer than 80 samples, and they used to start partway in.
        auto gx = [&](uint16_t k) { return tn > 1 ? (int)((uint32_t)k * (GLOW_W - 1) / (tn - 1)) : GLOW_W - 1; };
        auto plot = [&](int x, int y, uint8_t v) { if (x >= 0 && x < GLOW_W && y >= 0 && y < GLOW_H && s_glow[y * GLOW_W + x] < v) s_glow[y * GLOW_W + x] = v; };
        int headX = -1, headY = 0;
        if (tn > 1) {
            const uint16_t idx = (uint16_t)(((now % 1800) * (uint32_t)(tn - 1)) / 1800);
            uint16_t from = s_beamIdx <= idx ? s_beamIdx : 0;
            if (idx - from > 6) from = idx - 6;
            for (uint16_t k = from ? from - 1 : 0; k < idx; k++) {        // Bresenham, segment by segment
                int xa = gx(k), ya = gy(trail[k]); const int xb = gx(k + 1), yb = gy(trail[k + 1]);
                const int dx = abs(xb - xa), dy = -abs(yb - ya), sx = xa < xb ? 1 : -1, sy = ya < yb ? 1 : -1;
                int err = dx + dy;
                for (;;) { plot(xa, ya, 170); if (xa == xb && ya == yb) break; const int e2 = 2 * err; if (e2 >= dy) { err += dy; xa += sx; } if (e2 <= dx) { err += dx; ya += sy; } }
            }
            s_beamIdx = idx;
            // The head, between this sample and the next, so it glides.
            const float p = ((now % 1800) * (float)(tn - 1)) / 1800.0f, fr = p - idx;
            const uint16_t nx = idx + 1 < tn ? idx + 1 : idx;
            headX = (int)(gx(idx) + (gx(nx) - gx(idx)) * fr);
            headY = (int)(gy(trail[idx]) + (gy(trail[nx]) - gy(trail[idx])) * fr);
            for (int dy = -1; dy <= 1; dy++) for (int dx = -2; dx <= 1; dx++) plot(headX + dx, headY + dy, (dx == 0 && dy == 0) ? 255 : 215);
            // Sparks: one or two a frame, thrown back and out from the head.
            int spawn = 1 + (int)(dt / 14);
            for (Spark& sp : s_sparks) {
                if (!spawn) break;
                if (sp.life > 0) continue;
                sp.x = (float)headX; sp.y = (float)headY;
                sp.vx = -(0.015f + sparkRand() * 0.05f);
                sp.vy = (sparkRand() - 0.5f) * 0.06f;
                sp.full = sp.life = (int16_t)(350 + sparkRand() * 550);
                spawn--;
            }
        }
        // The solid line: every sample joined, always there, a dim accent.
        const uint16_t lineCol = scale565(Theme::CYAN, 0.30f);
        for (uint16_t k = 0; k + 1 < tn; k++)
            t.drawLine(x0 + gx(k), STRIP_Y + 1 + gy(trail[k]), x0 + gx(k + 1), STRIP_Y + 1 + gy(trail[k + 1]), lineCol);
        // The tail: the afterglow, white-hot nearest the head, the theme's
        // accent behind it, fading out.
        const uint16_t hot = Theme::blend(Theme::CYAN, Theme::WHITE, 170);
        for (int y = 0; y < GLOW_H; y++) for (int x = 0; x < GLOW_W; x++) {
            const uint8_t v = s_glow[y * GLOW_W + x];
            if (v <= 8) continue;
            const uint16_t c = v > 200 ? Theme::blend(Theme::CYAN, hot, (uint16_t)((v - 200) * 4.6f))
                                       : scale565(Theme::CYAN, v / 200.0f);
            t.drawPixel(x0 + x, STRIP_Y + 1 + y, c);
        }
        // The sparks, falling back off the head and cooling from white
        // through the accent to nothing.
        for (Spark& sp : s_sparks) {
            if (sp.life <= 0) continue;
            sp.life -= (int16_t)dt;
            sp.x += sp.vx * dt; sp.y += sp.vy * dt; sp.vy += 0.00012f * dt;
            if (sp.life <= 0 || sp.x < 0 || sp.x >= GLOW_W || sp.y < 0 || sp.y >= GLOW_H) { sp.life = 0; continue; }
            const float lf = (float)sp.life / sp.full;
            const uint16_t c = lf > 0.7f ? Theme::blend(Theme::CYAN, Theme::WHITE, (uint16_t)((lf - 0.7f) * 850))
                                         : themeRamp(0.35f * (1 - lf), lf * 1.3f);
            t.drawPixel(x0 + (int)sp.x, STRIP_Y + 1 + (int)sp.y, c);
            if (lf > 0.75f) t.drawPixel(x0 + (int)sp.x + 1, STRIP_Y + 1 + (int)sp.y, scale565(Theme::CYAN, lf * 0.5f));
        }
        // And the head itself, on top: a small halo and a white core.
        if (headX >= 0) {
            t.fillCircle(x0 + headX, STRIP_Y + 1 + headY, 3, scale565(Theme::CYAN, 0.35f));
            t.fillRect(x0 + headX - 1, STRIP_Y + headY, 2, 2, Theme::WHITE);
        }
    }

    // The packets: a spike each, in its network's colour, on the full scale.
    int16_t peakHold[8] = { -128, -128, -128, -128, -128, -128, -128, -128 };
    for (uint8_t i = 0; i < hn; i++) {
        const uint32_t ago = now - hits[i].ms;
        if (ago > 80000) continue;
        const int x = x0 + w - 1 - (int)(ago * (uint32_t)w / 80000);
        const int y = yForDbm(hits[i].rssi, STRIP_Y, STRIP_H);
        t.fillRect(x - 1, y, 3, STRIP_Y + STRIP_H - 3 - y, netCol(hits[i].proto));
        const int bucket = (int)(ago / 10000); if (bucket < 8 && hits[i].rssi > peakHold[bucket]) peakHold[bucket] = hits[i].rssi;
    }
    // Peak hold: the loudest packet of each ten seconds, in amber.
    for (int b = 7; b >= 0; b--) {
        if (peakHold[b] <= -128) continue;
        const int xa = x0 + w - 1 - (int)((b + 1) * w / 8), xb = x0 + w - 1 - (int)(b * w / 8);
        t.drawFastHLine(xa, yForDbm(peakHold[b], STRIP_Y, STRIP_H), xb - xa, dim(Theme::AMBER, 150));
    }

    // GHOST: the newest packet's spike, echoing leftward every 3.5 s and
    // dimmer each time, for two minutes.
    if (s_pHavePkt) {
        const Lora::Packet& pk = s_ps->pkt;
        const uint32_t ago = now - pk.ms;
        if (ago > 1500 && ago < GHOST_MS) {
            const float fade = fmaxf(0.08f, 1.0f - (float)ago / GHOST_MS);
            const float e = (ago % 3500) / 3500.0f;
            const int x = x0 + w - 1 - (int)(e * 60);
            const int y = yForDbm(pk.rssi, STRIP_Y, STRIP_H);
            t.fillRect(x - 1, y, 3, STRIP_Y + STRIP_H - 3 - y, scale565(kindCol(kindOf(pk.proto)), 0.35f * fade * (1 - e)));
        }
    }

    t.drawRect(COL_X, STRIP_Y, COL_W, STRIP_H, 0x2946);
    // No labels on the instruments (asked for 2026-10-15): the picture is the
    // reading, and the numbers that matter are in the tiles above.
}

void drawWaterfall(TFT_eSPI& t, uint32_t now, uint32_t dt) {
    const int x0 = COL_X + 1;
    t.fillRect(COL_X, WF_Y, COL_W, WF_H, 0x0000);
    const Lora::Hit* hits = s_pHits;
    const uint8_t hn = s_pHitN;
    const uint32_t quiet = now - lastPacketMs(hits, hn);

    // PHOSPHOR heat: a new column every 1.54 s takes the newest noise sample;
    // a new packet heats the newest column; everything cools.
    while (now - s_colAt >= WF_COL_MS) {
        s_colAt += WF_COL_MS;
        memmove(&s_heat[0][0], &s_heat[1][0], sizeof(float) * WF_ROWS * (WF_COLS - 1));
        memmove(&s_kind[0][0], &s_kind[1][0], WF_ROWS * (WF_COLS - 1));
        for (int r = 0; r < WF_ROWS; r++) { s_heat[WF_COLS - 1][r] = 0; s_kind[WF_COLS - 1][r] = 0; }
        int8_t trail[Lora::TRAIL_CAP];
        const uint16_t tn = Lora::rssiTrail(trail, Lora::TRAIL_CAP);
        if (tn) s_heat[WF_COLS - 1][wfRow(trail[tn - 1])] = 0.6f;
    }
    for (uint8_t i = 0; i < hn; i++) if ((int32_t)(hits[i].ms - s_hitSeen) > 0) {
        s_hitSeen = hits[i].ms;
        heatPacket(WF_COLS - 1, hits[i].rssi, hits[i].proto);
    }
    // Noise cools in about three seconds, packets in about twelve.
    const float coolN = expf(-(float)dt / 3000.0f), coolP = expf(-(float)dt / 12000.0f);
    for (int c = 0; c < WF_COLS; c++) for (int r = 0; r < WF_ROWS; r++) s_heat[c][r] *= s_kind[c][r] ? coolP : coolN;

    // SPARKLE, behind the heat, all the time (asked for 2026-10-15): a
    // twinkling field in the theme's own colours, drifting leftward at the
    // waterfall's pace. Each cell twinkles on its own steady rhythm; about
    // one in four is lit at any moment. A little brighter when it is quiet.
    const float spk = 0.6f + 0.4f * sparkleLevel(quiet);
    {
        const uint32_t shift = now / 400;                // a column every 0.4 s
        for (int c = 0; c < WF_COLS; c++) for (int r = 0; r < WF_ROWS; r++) {
            const uint32_t h = mix32((uint32_t)(c + shift) * 73856093u ^ (uint32_t)r * 19349663u);
            if ((h & 3) != 0) continue;
            const float tw = sinf(now * (0.0015f + (h >> 4 & 7) * 0.0005f) + (float)(h >> 8 & 255) * 0.0246f) * 0.5f + 0.5f;
            const float a = (0.12f + 0.88f * tw * tw * tw) * spk * 0.85f;
            const float u = (float)c / WF_COLS + r * 0.035f + now / 9000.0f;
            t.fillRect(x0 + c * WF_CW, WF_Y + r * WF_CH, WF_CW - 1, WF_CH, themeRamp(u, a));
        }
    }


    // The heat itself: warm amber for the noise, the network's colour for a
    // packet, both fading as they cool.
    for (int c = 0; c < WF_COLS; c++) for (int r = 0; r < WF_ROWS; r++) {
        const float h = s_heat[c][r];
        if (h < 0.03f) continue;
        const uint16_t col = s_kind[c][r] ? scale565(kindCol(s_kind[c][r]), fminf(1.0f, h))
                                          : scale565(0xFC80, fminf(1.0f, h * 0.8f));
        t.fillRect(x0 + c * WF_CW, WF_Y + r * WF_CH, WF_CW - 1, WF_CH, col);
    }

    // GHOST: the newest packet's own bytes, raining slowly, brighter for
    // bigger values, fading over two minutes.
    if (s_pHavePkt) {
        const Lora::Packet& pk = s_ps->pkt;
        const uint32_t ago = now - pk.ms;
        if (ago < GHOST_MS) {
            const float fade = fmaxf(0.08f, 1.0f - (float)ago / GHOST_MS);
            const uint16_t c = kindCol(kindOf(pk.proto));
            const uint8_t n = pk.len < 60 ? pk.len : 60;
            for (uint8_t i = 0; i < n; i++) {
                const int x = x0 + ((i * 37) % WF_COLS) * WF_CW;
                const int yy = (int)fmodf(ago * 0.004f + i * 7, (float)(WF_H + 8)) - 4;
                if (yy < 0 || yy >= WF_H - 1) continue;
                t.fillRect(x, WF_Y + yy, 3, 2, scale565(c, (0.15f + 0.6f * pk.data[i] / 255.0f) * fade));
            }
        }
    }
    t.drawRect(COL_X, WF_Y, COL_W, WF_H, 0x2946);
}

// ---- the dish ------------------------------------------------------------------
struct Blip { float x, y; bool known, direct; Lora::Proto proto; float miles; const char* tag; float bearing; };

const char* compass(float deg) {
    static const char* N[8] = { "N", "NE", "E", "SE", "S", "SW", "W", "NW" };
    int i = (int)((deg + 22.5f) / 45.0f) % 8; if (i < 0) i += 8;
    return N[i];
}

// The ping (asked for 2026-10-16): rings that open out from the middle of
// the dish. A soft sonar pulse once a sweep, in the theme's accent, and a
// bright double ripple in the network's colour whenever a packet lands.
struct Ping { uint32_t at; uint16_t col; bool loud; };
Ping     s_pings[6];
uint8_t  s_pingNext = 0;
uint32_t s_pingSeen = 0, s_pingSweep = 0;
const uint32_t PING_MS = 1400;

void pingStart(uint32_t now, uint16_t col, bool loud) {
    s_pings[s_pingNext] = Ping{ now ? now : 1, col, loud };
    s_pingNext = (uint8_t)((s_pingNext + 1) % 6);
}

void drawPings(TFT_eSPI& t, uint32_t now, int cx, int cy, int R) {
    // New packets, and the start of each sweep.
    const Lora::Hit* hits = s_pHits;
    const uint8_t hn = s_pHitN;
    for (uint8_t i = 0; i < hn; i++) if ((int32_t)(hits[i].ms - s_pingSeen) > 0) {
        s_pingSeen = hits[i].ms;
        if (now - hits[i].ms < 2000) pingStart(now, netCol(hits[i].proto), true);
    }
    if (now / 4000 != s_pingSweep) { s_pingSweep = now / 4000; pingStart(now, Theme::CYAN, false); }
    for (const Ping& p : s_pings) {
        if (!p.at) continue;
        const uint32_t age = now - p.at;
        if (age >= PING_MS) continue;
        const float u = (float)age / PING_MS;
        const float ease = 1.0f - (1.0f - u) * (1.0f - u);          // fast out, slowing at the rim
        const int r = (int)(2 + ease * (R - 3));
        const float a = (1.0f - u) * (p.loud ? 1.0f : 0.35f);
        t.drawCircle(cx, cy, r, scale565(p.col, a));
        if (r > 2) t.drawCircle(cx, cy, r - 1, scale565(p.col, a * 0.45f));      // a soft inner edge
        if (r > 4) t.drawCircle(cx, cy, r - 3, scale565(p.col, a * 0.15f));
        // A loud ping rings twice: a second, thinner ring a beat behind.
        if (p.loud && age > 220) {
            const float u2 = (float)(age - 220) / PING_MS, e2 = 1.0f - (1.0f - u2) * (1.0f - u2);
            const int r2 = (int)(2 + e2 * (R - 3));
            if (r2 < R) t.drawCircle(cx, cy, r2, scale565(p.col, (1.0f - u2) * 0.5f));
        }
    }
}

void drawDish(TFT_eSPI& t, uint32_t now) {
    const int cx = DISH_CX, cy = DISH_CY, R = DISH_R;
    t.fillCircle(cx, cy, R, 0x0041);
    t.drawCircle(cx, cy, R, 0x0C67);
    t.drawCircle(cx, cy, R * 2 / 3, 0x0A45);
    t.drawCircle(cx, cy, R / 3, 0x0A45);
    for (int a = 0; a < 360; a += 6) {   // the dotted rim: where a node with no bearing sits
        const float r = a * 0.0174533f;
        t.drawPixel(cx + (int)lroundf((R - 4) * cosf(r)), cy + (int)lroundf((R - 4) * sinf(r)), 0x4208);
    }
    t.drawFastHLine(cx - R, cy, 2 * R, 0x0A45);
    t.drawFastVLine(cx, cy - R, 2 * R, 0x0A45);
    drawPings(t, now, cx, cy, R);
    // The sweep: a line going round every four seconds, with a fading trail.
    const float sweep = ((now % 4000) / 4000.0f) * 6.2831853f - 1.5707963f;
    for (int k = 3; k >= 0; k--) {
        const float a = sweep - k * 0.12f;
        t.drawLine(cx, cy, cx + (int)lroundf(R * cosf(a)), cy + (int)lroundf(R * sinf(a)), dim(Theme::CYAN, (uint16_t)(k ? 60 - k * 14 : 180)));
    }
    // The nodes.
    // The page's own copy (see refreshSnapshot); the table holds 96.
    const uint8_t SNAP = SNAP_NODES;
    const Lora::Nodes::Node* nodes = s_ps ? s_ps->nodes : nullptr;
    const uint8_t nn = s_ps ? s_pNodeN : 0;
    const Gnss::Fix& fix = Gnss::fix();
    const bool haveFix = fix.valid && Gnss::fresh(now);
    Blip blips[SNAP];
    uint8_t nb = 0, unknownN = 0;
    for (uint8_t i = 0; i < nn; i++) {
        const Lora::Nodes::Node& n = nodes[i];
        if (n.proto != Lora::Proto::MESHTASTIC && n.proto != Lora::Proto::MESHCORE) continue;
        if (now - n.lastMs > 3600000u) continue;       // an hour: the dish is who is around now
        Blip& b = blips[nb];
        b.proto = n.proto; b.direct = n.directPackets > 0; b.tag = n.name[0] ? n.name : n.tag; b.known = false; b.miles = 0; b.bearing = 0;
        if (haveFix && n.hasPos) {
            const float lat0 = fix.lat7 / 1e7f, lat1 = n.latE7 / 1e7f;
            const float dy = (lat1 - lat0) * 69.0f;                                  // miles per degree
            const float dx = (n.lonE7 - fix.lon7) / 1e7f * 69.0f * cosf(lat0 * 0.0174533f);
            b.miles = sqrtf(dx * dx + dy * dy);
            float deg = atan2f(dx, dy) * 57.29578f; if (deg < 0) deg += 360;
            b.bearing = deg;
            const float d = (R - 8) * fminf(1.0f, log10f(1.0f + b.miles) / log10f(11.0f));   // 10 miles at the rim
            const float a = (deg - 90) * 0.0174533f;
            b.x = cx + d * cosf(a); b.y = cy + d * sinf(a); b.known = true;
        } else unknownN++;
        nb++;
    }
    // The unknown ones round the rim, spaced.
    uint8_t k = 0;
    for (uint8_t i = 0; i < nb; i++) if (!blips[i].known) {
        const float a = 3.5f + (unknownN ? (6.2831853f * k) / unknownN : 0); k++;
        blips[i].x = cx + (R - 4) * cosf(a); blips[i].y = cy + (R - 4) * sinf(a);
    }
    // Nearest first, so its name wins the label.
    uint8_t order[SNAP];
    for (uint8_t i = 0; i < nb; i++) order[i] = i;
    for (uint8_t i = 1; i < nb; i++) for (uint8_t j = i; j > 0; j--) {
        const Blip& a = blips[order[j - 1]]; const Blip& b = blips[order[j]];
        const bool swap = (!a.known && b.known) || (a.known && b.known && b.miles < a.miles);
        if (swap) { const uint8_t tmp = order[j]; order[j] = order[j - 1]; order[j - 1] = tmp; } else break;
    }
    uint8_t labels = 0;
    int placedY[3] = { -99, -99, -99 };
    for (uint8_t oi = 0; oi < nb; oi++) {
        const Blip& b = blips[order[oi]];
        const uint16_t c = netCol(b.proto);
        const int x = (int)lroundf(b.x), y = (int)lroundf(b.y);
        if (b.known) {
            t.fillCircle(x, y, 4, dim(c, 60));
            if (b.direct) t.fillCircle(x, y, 2, c); else t.drawCircle(x, y, 2, c);
            if (labels < 3) {
                char pv[24];
                const char* nm = Privacy::name(b.tag, pv, sizeof pv);
                t.setTextFont(1); t.setTextSize(1);
                const int tw = t.textWidth(nm);
                int lx = x > cx ? x + 5 : x - 5 - tw;
                if (lx + tw > cx + R + 6) lx = x - 5 - tw;
                if (lx < 2) lx = 2;
                int ly = y - 4;
                for (int q = 0; q < 3; q++) if (placedY[q] > -99 && abs(placedY[q] - ly) < 9) ly = placedY[q] + 9;
                placedY[labels++] = ly;
                smallClip(t, lx, ly, nm, c, 60);
            }
        } else {
            if (b.direct) t.fillCircle(x, y, 1, dim(c, 190)); else t.drawCircle(x, y, 2, dim(c, 150));
        }
    }
    t.fillCircle(cx, cy, 2, Theme::WHITE);
    small(t, cx - 3, cy - R - 9, "N", dim(Theme::WHITE, 110));
    small(t, cx - R + 2, cy + R - 8, "10mi", dim(Theme::WHITE, 110), 0x0041);
    // Under the dish, three lines. The GPS's own state first, in words: the
    // legend used to end "NO POS", which meant "a rim dot sent no position"
    // and read as "your GPS has no position" (a tester, 2026-10-15).
    {
        char g[24];
        uint16_t gc;
        if (haveFix)                 { snprintf(g, sizeof g, "GPS FIX %u SATS", (unsigned)fix.used); gc = Theme::GREEN; }
        else if (Gnss::good() > 0)   { snprintf(g, sizeof g, "GPS SEARCHING"); gc = Theme::AMBER; }
        else                         { snprintf(g, sizeof g, "GPS NOT HEARD"); gc = dim(Theme::WHITE, 150); }
        smallClip(t, 4, cy + R + 3, g, gc, 100);
    }
    // Then the nearest node that has said where it is, or why there is none.
    if (nb && blips[order[0]].known) {
        const Blip& b = blips[order[0]];
        char pv[24], line[28];
        const char* nm = Privacy::name(b.tag, pv, sizeof pv);
        if (b.miles < 10) snprintf(line, sizeof line, "%s %u.%umi %s", nm, (unsigned)b.miles, (unsigned)(b.miles * 10) % 10, compass(b.bearing));
        else              snprintf(line, sizeof line, "%s %umi %s", nm, (unsigned)b.miles, compass(b.bearing));
        smallClip(t, 4, cy + R + 12, line, netCol(b.proto), 100);
    } else if (nb) {
        // Heard, but none has sent a position yet (Meshtastic's default is one
        // every 15-30 min, and many owners turn it off), or no fix to place them.
        smallClip(t, 4, cy + R + 12, haveFix ? "NO NODE POS YET" : "NEEDS A GPS FIX", dim(Theme::WHITE, 150), 100);
    } else {
        small(t, 4, cy + R + 12, "NOBODY HEARD YET", dim(Theme::WHITE, 150));
    }
    // And the key: filled heard direct, hollow through relays. A rim dot is a
    // node with no position; the line above says when that is all of them.
    t.fillCircle(7, cy + R + 25, 1, Theme::WHITE);   small(t, 11, cy + R + 21, "DIRECT", dim(Theme::WHITE, 150));
    t.drawCircle(56, cy + R + 25, 2, Theme::WHITE);  small(t, 61, cy + R + 21, "RELAY", dim(Theme::WHITE, 150));
}

// ---- the chats -------------------------------------------------------------------
// CHAT RAIN: inside each message box, under its lines, clipped to the box.
void drawChatRain(TFT_eSPI& t) {
    t.setTextFont(1); t.setTextSize(1);
    const int top = CHAT_Y + 14, h = CHAT_BOTTOM - CHAT_Y - 15;
    t.setViewport(5, top, 152, h, false);
    rainDraw(t, s_chatMt, CHAT_RAIN_A, [](int, int) { return true; }, [](const RainCol&, int) { return Theme::CYAN; });
    t.resetViewport();
    t.setViewport(163, top, 152, h, false);
    rainDraw(t, s_chatMc, CHAT_RAIN_A, [](int, int) { return true; }, [](const RainCol&, int) { return Theme::PINK; });
    t.resetViewport();
}

void drawChats(TFT_eSPI& t, uint32_t now) {
    const int colW = 154;
    const int xs[2] = { 4, 162 };
    const uint16_t cols[2] = { Theme::CYAN, Theme::PINK };
    const char* names[2] = { "MESHTASTIC", "MESHCORE" };
    // The page's own copy, refreshed twice a second (see refreshSnapshot).
    const uint16_t* count = s_pCount;
    if (!s_ps) return;
    auto& keep = s_ps->keep;
    const uint8_t* kept = s_pKept;
    const uint16_t unread = uiLoraChatUnread();
    for (int c = 0; c < 2; c++) {
        const int x = xs[c];
        t.fillRect(x + 1, CHAT_Y + 1, colW - 2, 12, dim(cols[c], 36));
        t.drawRect(x, CHAT_Y, colW, CHAT_BOTTOM - CHAT_Y, dim(cols[c], 110));
        small(t, x + 4, CHAT_Y + 3, names[c], cols[c], dim(cols[c], 36));
        char badge[12];
        if (c == 0 && unread) {
            snprintf(badge, sizeof badge, "%u NEW", (unsigned)unread);
            t.setTextFont(1); t.setTextSize(1);
            const int bw = t.textWidth(badge) + 6;
            t.fillRect(x + colW - 3 - bw, CHAT_Y + 2, bw, 10, cols[c]);
            small(t, x + colW - bw, CHAT_Y + 3, badge, Theme::BLACK, cols[c]);
        } else {
            snprintf(badge, sizeof badge, "%u", (unsigned)count[c]);
            t.setTextFont(1); t.setTextSize(1);
            small(t, x + colW - 4 - t.textWidth(badge), CHAT_Y + 3, badge, cols[c], dim(cols[c], 36));
        }
        if (!kept[c]) {
            small(t, x + 4, CHAT_Y + 18, "nothing yet", dim(Theme::WHITE, 120));
            continue;
        }
        for (uint8_t i = 0; i < kept[c]; i++) {
            const Lora::Msgs::Msg& r = keep[c][i];
            const int y = CHAT_Y + 15 + i * CHAT_ROW_H;
            if (i == 0) t.fillRect(x + 1, y - 1, colW - 2, CHAT_ROW_H, dim(cols[c], 22));
            char pv[24], ago[8];
            const char* who = Privacy::name(r.sender[0] ? r.sender : "?", pv, sizeof pv);
            age(now, r.ms, ago, sizeof ago);
            t.setTextFont(1); t.setTextSize(1);
            const uint16_t bg = i == 0 ? dim(cols[c], 22) : Theme::BG;
            t.setTextColor(cols[c], bg); t.setCursor(x + 4, y + 1); t.print(who);
            const int tx = t.getCursorX() + 4;
            const int agoW = t.textWidth(ago);
            char text[48]; snprintf(text, sizeof text, "%s", r.text);
            size_t tn = strlen(text);
            while (tn && tx + t.textWidth(text) > x + colW - 8 - agoW) text[--tn] = '\0';
            t.setTextColor(Theme::WHITE, bg); t.setCursor(tx, y + 1); t.print(text);
            t.setTextColor(dim(Theme::WHITE, 110), bg); t.setCursor(x + colW - 4 - agoW, y + 1); t.print(ago);
        }
    }
}

// ---- the compose line and the bar -------------------------------------------------
void drawCompose(TFT_eSPI& t, uint32_t now) {
    const uint16_t accent = s_net ? Theme::PINK : Theme::CYAN;
    const int w = 232;
    t.fillRoundRect(4, COMPOSE_Y, w, COMPOSE_H, 2, dim(Theme::WHITE, 18));
    t.drawRoundRect(4, COMPOSE_Y, w, COMPOSE_H, 2, dim(accent, 150));
    t.setTextFont(1); t.setTextSize(1);
    if (s_status[0] && now - s_statusAt < 2500) {
        small(t, 8, COMPOSE_Y + 2, s_status, Theme::VAPOR_YELLOW, dim(Theme::WHITE, 18));
    } else if (!Settings::loraTxDbm() && !s_composeN) {
        small(t, 8, COMPOSE_Y + 2, "TYPE TO SEND (LORA TX IS OFF: SETTINGS)", dim(Theme::WHITE, 120), dim(Theme::WHITE, 18));
    } else if (!s_composeN) {
        small(t, 8, COMPOSE_Y + 2, "TYPE TO SEND, ENTER SENDS", dim(accent, 170), dim(Theme::WHITE, 18));
    } else {
        char line[168]; snprintf(line, sizeof line, "%s_", s_compose);
        const char* p = line;
        while (*p && t.textWidth(p) > w - 10) p++;
        small(t, 8, COMPOSE_Y + 2, p, Theme::WHITE, dim(Theme::WHITE, 18));
    }
}

void drawBar(TFT_eSPI& t) {
    t.fillRect(0, BAR_Y, t.width(), t.height() - BAR_Y, 0x0862);
    small(t, 6, BAR_Y + 3, "[ CHATS ]", Theme::CYAN, 0x0862);
    small(t, 96, BAR_Y + 3, "ENTER SENDS", dim(Theme::WHITE, 110), 0x0862);
}

// The switch: which network Enter sends to. Twice the compose line's height
// (asked for 2026-10-15), so it reaches down over the bar's empty right end
// rather than taking a line from the chats; drawn after the bar for that.
void drawSwitch(TFT_eSPI& t) {
    const uint16_t accent = s_net ? Theme::PINK : Theme::CYAN;
    s_switchX = 240; s_switchW = 76;
    const int h = COMPOSE_H * 2;
    t.fillRoundRect(s_switchX, COMPOSE_Y, s_switchW, h, 3, dim(accent, 40));
    t.drawRoundRect(s_switchX, COMPOSE_Y, s_switchW, h, 3, accent);
    const char* lab = s_net ? "> MESHCORE" : "> MESHTASTIC";
    t.setTextFont(1); t.setTextSize(1);
    small(t, s_switchX + (s_switchW - t.textWidth(lab)) / 2, COMPOSE_Y + (h - 8) / 2, lab, accent, dim(accent, 40));
}

}  // namespace

// ---- the API ------------------------------------------------------------------------
void uiLoraHomeInit(TFT_eSPI& t) {
    // The switch follows what the radio listens to; BOTH keeps the last choice.
    if (Settings::loraListen() == 1) s_net = 0;
    else if (Settings::loraListen() == 2) s_net = 1;
    t.fillRect(0, 0, t.width(), t.height(), Theme::BG);
    s_openedAt = millis();
    s_lastFrame = 0;
    refreshSnapshot(s_openedAt, true);
    s_beamIdx = 0;
    heatPrefill(s_openedAt);
}

void uiLoraHomeToggleNet() { s_net = (uint8_t)(s_net ? 0 : 1); }
uint8_t uiLoraHomeNet() { return s_net; }

void uiLoraHomeTick(TFT_eSPI& t, uint32_t now) {
    refreshSnapshot(now, false);
    t.fillRect(0, 0, t.width(), t.height(), Theme::BG);
    const uint32_t dt = s_lastFrame ? (now - s_lastFrame > 200 ? 200 : now - s_lastFrame) : 16;
    s_lastFrame = now;
    // The rain: CODE FALL down the instruments, and a CHAT RAIN in each
    // message box; each is drawn inside its own box, clipped.
    if (!s_code.ready)   rainInit(s_code, COL_X + 2, COL_X + COL_W - 4, 6, STRIP_Y, STRIP_H, 0.7f, 0xC0DE);
    if (!s_chatMt.ready) rainInit(s_chatMt, 7, 155, 6, CHAT_Y + 14, CHAT_BOTTOM - CHAT_Y - 14, 0.5f, 0xCA75);
    if (!s_chatMc.ready) rainInit(s_chatMc, 165, 313, 6, CHAT_Y + 14, CHAT_BOTTOM - CHAT_Y - 14, 0.5f, 0x7C0E);
    rainStep(s_code, dt, 0.45f);
    rainStep(s_chatMt, dt, 0.4f);
    rainStep(s_chatMc, dt, 0.4f);
    const Numbers n = numbers(now);
    drawHeader(t, now);
    drawTiles(t, n);
    drawStrip(t, now, n, dt);
    drawWaterfall(t, now, dt);
    drawDish(t, now);
    scanlines(t, COL_X, STRIP_Y, COL_W, WF_Y + WF_H - STRIP_Y);
    scanlines(t, DISH_CX - DISH_R, DISH_CY - DISH_R, 2 * DISH_R + 1, 2 * DISH_R + 1);
    drawChatRain(t);
    drawChats(t, now);
    drawCompose(t, now);
    drawBar(t);
    drawSwitch(t);
}

LoraHomeHit uiLoraHomeHit(TFT_eSPI& t, int x, int y) {
    // BACK is the top-right corner, with a finger's worth of slack; the bar's
    // right half does nothing, so a slip below the switch goes nowhere.
    if (y < 14 && x >= t.width() - 80) return LoraHomeHit::BACK;
    // The switch, all of its double height and a little round it.
    if (y >= COMPOSE_Y - 2 && x >= s_switchX - 6) return LoraHomeHit::SWITCH;
    if (y >= BAR_Y - 4) return x < 80 ? LoraHomeHit::CHATS : LoraHomeHit::NONE;
    if (y >= COMPOSE_Y - 2) return LoraHomeHit::NONE;
    if (y >= CHAT_Y - 2 && y < COMPOSE_Y - 2) return x < t.width() / 2 ? LoraHomeHit::CHAT_MESHTASTIC : LoraHomeHit::CHAT_MESHCORE;
    return LoraHomeHit::NONE;
}

void uiLoraHomeKey(char c, uint32_t now) {
    if (c == '\b') { if (s_composeN) s_compose[--s_composeN] = '\0'; return; }
    if (c == '\n') {
        if (!s_composeN) return;
        if (!Settings::loraTxDbm()) { snprintf(s_status, sizeof s_status, "LORA TX IS OFF: SETTINGS > LORA"); s_statusAt = now; return; }
        const char* nick = Squachy::nickname();
        if (!nick || !*nick) nick = "SQUACHY";
        const bool ok = Lora::sendText(s_net ? Lora::Proto::MESHCORE : Lora::Proto::MESHTASTIC, nick, s_compose, (int8_t)Settings::loraTxDbm());
        snprintf(s_status, sizeof s_status, ok ? "SENT" : "SEND FAILED");
        s_statusAt = now;
        if (ok) { s_composeN = 0; s_compose[0] = '\0'; }
        return;
    }
    if (c < 32 || c > 126) return;
    const uint8_t cap = s_net ? 120 : 160;
    if (s_composeN < cap) { s_compose[s_composeN++] = c; s_compose[s_composeN] = '\0'; }
}

#endif  // SQUACH_LORA && TDECK_PLUS
