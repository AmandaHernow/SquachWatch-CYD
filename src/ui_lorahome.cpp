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

// ---- the numbers -------------------------------------------------------------
struct Numbers { int16_t now, floor, peak; int16_t margin; uint16_t pktMin; uint8_t airPct10, busyPct; };

Numbers numbers(uint32_t nowMs) {
    Numbers n = {};
    int8_t trail[Lora::TRAIL_CAP];
    const uint16_t tn = Lora::rssiTrail(trail, Lora::TRAIL_CAP);
    n.now = tn ? trail[tn - 1] : -128;
    n.floor = Lora::stats().noiseDbm ? Lora::stats().noiseDbm : -128;
    n.peak = -128;
    Lora::Hit hits[Lora::HIT_CAP];
    const uint8_t hn = Lora::hitTrail(hits, Lora::HIT_CAP);
    for (uint8_t i = 0; i < hn; i++) if (nowMs - hits[i].ms < 80000 && hits[i].rssi > n.peak) n.peak = hits[i].rssi;
    // How far the loudest packet of the last 80 s stood above the floor: a
    // link quality. NOW over the floor sits near zero on a quiet band, and a
    // quiet band is not a bad one (the first walk, 2026-10-15).
    n.margin = (n.peak > -128 && n.floor > -128) ? (int16_t)(n.peak - n.floor) : -128;
    // The last minute: packets and their airtime, off the ring.
    uint32_t air = 0; uint16_t cnt = 0;
    Lora::Packet pk;
    for (uint16_t i = 0; i < 64; i++) {
        if (!Lora::packetAt(i, pk)) break;
        if (nowMs - pk.ms > 60000) break;
        cnt++; air += pk.toaUs / 1000;
    }
    n.pktMin = cnt;
    n.airPct10 = (uint8_t)((air * 1000 / 60000 > 999) ? 999 : air * 1000 / 60000);   // tenths of a percent
    const Lora::Stats& st = Lora::stats();
    n.busyPct = st.cadRounds ? (uint8_t)((uint64_t)st.cadHits * 100 / st.cadRounds) : 0;
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
// noise as a line with the hits as spikes; the waterfall is the same 80
// seconds as colour, each column a second and a half, each row ten dB.
int yForDbm(int16_t dbm, int top, int h) {
    // -120 at the bottom, -40 at the top.
    int v = 120 + dbm; if (v < 0) v = 0; if (v > 80) v = 80;
    return top + h - 3 - (v * (h - 6)) / 80;
}

void drawStrip(TFT_eSPI& t, uint32_t now, const Numbers& n) {
    const int x0 = COL_X + 1, w = COL_W - 2;
    t.fillRect(COL_X, STRIP_Y, COL_W, STRIP_H, 0x0000);
    for (int g = 1; g < 4; g++) t.drawFastHLine(x0, STRIP_Y + g * STRIP_H / 4, w, 0x0841);
    // The floor, dotted.
    if (n.floor > -128) { const int fy = yForDbm(n.floor, STRIP_Y, STRIP_H); for (int x = x0; x < x0 + w; x += 3) t.drawPixel(x, fy, 0x39C7); }
    // Noise: one sample a second, right edge is now.
    int8_t trail[Lora::TRAIL_CAP];
    const uint16_t tn = Lora::rssiTrail(trail, Lora::TRAIL_CAP);
    int lastX = -1, lastY = 0;
    for (uint16_t i = 0; i < tn; i++) {
        const int x = x0 + w - 1 - (int)((tn - 1 - i) * w / 80);
        if (x < x0) continue;
        const int y = yForDbm(trail[i], STRIP_Y, STRIP_H);
        if (lastX >= 0) t.drawLine(lastX, lastY, x, y, 0x2D6A);
        lastX = x; lastY = y;
    }
    // Hits: a spike per packet, in its network's colour, three pixels wide.
    Lora::Hit hits[Lora::HIT_CAP];
    const uint8_t hn = Lora::hitTrail(hits, Lora::HIT_CAP);
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
        const int y = yForDbm(peakHold[b], STRIP_Y, STRIP_H);
        t.drawFastHLine(xa, y, xb - xa, dim(Theme::AMBER, 150));
    }
    t.drawRect(COL_X, STRIP_Y, COL_W, STRIP_H, 0x2946);
    small(t, COL_X + 4, STRIP_Y + 2, "80 s", dim(Theme::WHITE, 110), 0x0000);
    char line[24];
    snprintf(line, sizeof line, "PKT/MIN %u", (unsigned)n.pktMin);
    small(t, COL_X + 130, STRIP_Y + 2, line, dim(Theme::WHITE, 110), 0x0000);
    snprintf(line, sizeof line, "AIR %u.%u%%  CAD %u%%", (unsigned)(n.airPct10 / 10), (unsigned)(n.airPct10 % 10), (unsigned)n.busyPct);
    small(t, COL_X + 130, STRIP_Y + 12, line, dim(Theme::WHITE, 110), 0x0000);
    small(t, COL_X + 4, STRIP_Y + STRIP_H - 9, "- PEAK HOLD", dim(Theme::AMBER, 170), 0x0000);
}

void drawWaterfall(TFT_eSPI& t, uint32_t now) {
    const int x0 = COL_X + 1, w = COL_W - 2;
    t.fillRect(COL_X, WF_Y, COL_W, WF_H, 0x0000);
    const int CW = 4, CH = 2, COLS = w / CW, ROWS = WF_H / CH;   // 52 columns of 1.5 s, 10 rows of 8 dB
    int8_t trail[Lora::TRAIL_CAP];
    const uint16_t tn = Lora::rssiTrail(trail, Lora::TRAIL_CAP);
    // Noise: a warm cell where the sample sat.
    for (uint16_t i = 0; i < tn; i++) {
        const uint32_t ago = (uint32_t)(tn - 1 - i) * 1000;
        const int col = COLS - 1 - (int)(ago / 1540);
        if (col < 0) continue;
        int v = 120 + trail[i]; if (v < 0) v = 0; if (v > 79) v = 79;
        const int row = ROWS - 1 - (v * ROWS) / 80;
        t.fillRect(x0 + col * CW, WF_Y + row * CH, CW - 1, CH, 0x3904);
        if (row + 1 < ROWS) t.fillRect(x0 + col * CW, WF_Y + (row + 1) * CH, CW - 1, CH, 0x1882);
    }
    // Packets: a hot cell in the network's colour, and a glow below it.
    Lora::Hit hits[Lora::HIT_CAP];
    const uint8_t hn = Lora::hitTrail(hits, Lora::HIT_CAP);
    for (uint8_t i = 0; i < hn; i++) {
        const uint32_t ago = now - hits[i].ms;
        if (ago > 80000) continue;
        const int col = COLS - 1 - (int)(ago / 1540);
        int v = 120 + hits[i].rssi; if (v < 0) v = 0; if (v > 79) v = 79;
        const int row = ROWS - 1 - (v * ROWS) / 80;
        const uint16_t c = netCol(hits[i].proto);
        t.fillRect(x0 + col * CW, WF_Y + row * CH, CW - 1, CH, c);
        for (int r = row + 1; r < ROWS; r++) t.fillRect(x0 + col * CW, WF_Y + r * CH, CW - 1, CH, dim(c, (uint16_t)(90 / (r - row))));
    }
    t.drawRect(COL_X, WF_Y, COL_W, WF_H, 0x2946);
    small(t, COL_X + 4, WF_Y + WF_H - 8, "WATERFALL", dim(Theme::WHITE, 110), 0x0000);
    small(t, COL_X + 150, WF_Y + WF_H - 8, "MT", Theme::CYAN, 0x0000);
    small(t, COL_X + 172, WF_Y + WF_H - 8, "MC", Theme::PINK, 0x0000);
}

// ---- the dish ------------------------------------------------------------------
struct Blip { float x, y; bool known, direct; Lora::Proto proto; float miles; const char* tag; float bearing; };

const char* compass(float deg) {
    static const char* N[8] = { "N", "NE", "E", "SE", "S", "SW", "W", "NW" };
    int i = (int)((deg + 22.5f) / 45.0f) % 8; if (i < 0) i += 8;
    return N[i];
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
    // The sweep: a line going round every four seconds, with a fading trail.
    const float sweep = ((now % 4000) / 4000.0f) * 6.2831853f - 1.5707963f;
    for (int k = 3; k >= 0; k--) {
        const float a = sweep - k * 0.12f;
        t.drawLine(cx, cy, cx + (int)lroundf(R * cosf(a)), cy + (int)lroundf(R * sinf(a)), dim(Theme::CYAN, (uint16_t)(k ? 60 - k * 14 : 180)));
    }
    // The nodes.
    // The newest 32 are plenty for a dish this size; the table holds 96.
    const uint8_t SNAP = 32;
    static Lora::Nodes::Node nodes[SNAP];
    const uint8_t nn = Lora::nodeSnapshot(nodes, SNAP);
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
void drawChats(TFT_eSPI& t, uint32_t now) {
    const int colW = 154;
    const int xs[2] = { 4, 162 };
    const uint16_t cols[2] = { Theme::CYAN, Theme::PINK };
    const char* names[2] = { "MESHTASTIC", "MESHCORE" };
    uint16_t count[2] = { 0, 0 };
    const int lines = (CHAT_BOTTOM - (CHAT_Y + 15)) / CHAT_ROW_H;
    // Walk the ring newest first once for both columns.
    static Lora::Msgs::Msg keep[2][6];
    uint8_t kept[2] = { 0, 0 };
    const uint16_t n = Lora::msgCount();
    Lora::Msgs::Msg m;
    for (uint16_t i = 0; i < n; i++) {
        if (!Lora::msgAt(i, m)) break;
        const int c = m.proto == Lora::Proto::MESHTASTIC ? 0 : m.proto == Lora::Proto::MESHCORE ? 1 : -1;
        if (c < 0) continue;
        count[c]++;
        if (kept[c] < lines && kept[c] < 6) keep[c][kept[c]++] = m;
    }
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
    // The switch: which network Enter sends to.
    s_switchX = 240; s_switchW = 76;
    t.fillRoundRect(s_switchX, COMPOSE_Y, s_switchW, COMPOSE_H, 2, dim(accent, 40));
    t.drawRoundRect(s_switchX, COMPOSE_Y, s_switchW, COMPOSE_H, 2, accent);
    small(t, s_switchX + 4, COMPOSE_Y + 2, s_net ? "> MESHCORE" : "> MESHTASTIC", accent, dim(accent, 40));
}

void drawBar(TFT_eSPI& t) {
    t.fillRect(0, BAR_Y, t.width(), t.height() - BAR_Y, 0x0862);
    small(t, 6, BAR_Y + 3, "[ CHATS ]", Theme::CYAN, 0x0862);
    small(t, 96, BAR_Y + 3, "ENTER SENDS   BKSP DELETES", dim(Theme::WHITE, 110), 0x0862);
}

}  // namespace

// ---- the API ------------------------------------------------------------------------
void uiLoraHomeInit(TFT_eSPI& t) {
    // The switch follows what the radio listens to; BOTH keeps the last choice.
    if (Settings::loraListen() == 1) s_net = 0;
    else if (Settings::loraListen() == 2) s_net = 1;
    t.fillRect(0, 0, t.width(), t.height(), Theme::BG);
}

void uiLoraHomeToggleNet() { s_net = (uint8_t)(s_net ? 0 : 1); }
uint8_t uiLoraHomeNet() { return s_net; }

void uiLoraHomeTick(TFT_eSPI& t, uint32_t now) {
    t.fillRect(0, 0, t.width(), t.height(), Theme::BG);
    const Numbers n = numbers(now);
    drawHeader(t, now);
    drawTiles(t, n);
    drawStrip(t, now, n);
    drawWaterfall(t, now);
    drawDish(t, now);
    scanlines(t, COL_X, STRIP_Y, COL_W, WF_Y + WF_H - STRIP_Y);
    scanlines(t, DISH_CX - DISH_R, DISH_CY - DISH_R, 2 * DISH_R + 1, 2 * DISH_R + 1);
    drawChats(t, now);
    drawCompose(t, now);
    drawBar(t);
}

LoraHomeHit uiLoraHomeHit(TFT_eSPI& t, int x, int y) {
    // BACK is the top-right corner, with a finger's worth of slack; the bar's
    // right half does nothing, so a slip below the switch goes nowhere.
    if (y < 14 && x >= t.width() - 80) return LoraHomeHit::BACK;
    if (y >= BAR_Y - 4) return x < 80 ? LoraHomeHit::CHATS : LoraHomeHit::NONE;
    if (y >= COMPOSE_Y - 2 && y < BAR_Y + 4) return x >= s_switchX - 6 ? LoraHomeHit::SWITCH : LoraHomeHit::NONE;
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
