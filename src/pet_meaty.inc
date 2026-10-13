// SquachWatch-CYD — MEATY. Included by pet.cpp inside namespace Pet, after
// T0@$TY, whose balloon, grip and throw he borrows.
//
// A flat rust-brown ball of ground meat with a lumpy edge, a black line round
// him, pale specks and two stray hairs; a small face low and right of centre,
// heavy-lidded eyes looking up from under the lids, and one tooth in a frown.
// He rolls, and his face stays still while the meat turns under it. He eats a
// catch, turns igloo when it is a serious one, shows you a hot dog now and
// then, splits into three when he lands hard and rolls back together, and
// does "Shake it" when Squachy dances. Earned by knocking three times on the
// igloo in the SNOWFALL snow.
//
// His body is worked out from a shape every frame (meaty_shape.inc, made by
// tools/gen_meaty_shape.py) in whole numbers, so the ESP32-C5 boards with
// no floating-point unit draw him at the same cost as everyone else.
//
// His lines fit three balloon lines of 24 characters at most.
#include "meaty_shape.inc"

static const char* const MW_TIPS[] = {
    "I'm a meatball. I'm doing a real good job.",
    "What we doing? Are we still doing it?",
    "I rolled here all by myself.",
    "You wanna see a hot dog? I can do a hot dog.",
    "I got a hair in me. It's mine now.",
    "I know what that thing is. It's a thing.",
    "Nobody tells me nothing.",
    "I'm helping. You just can't tell yet.",
    "I counted to three. Then I lost it.",
    "My face don't move when I roll. Watch.",
    "Is it lunch? It feels like lunch.",
    "I'm keepin' a eye out. The big one.",
};
static const uint8_t MW_TIPS_N = sizeof(MW_TIPS) / sizeof(MW_TIPS[0]);
static const char* const MW_NIGHT[] = {
    "It's dark. I'm a little scared of the dark.",
    "I'm gonna be a igloo for bed.",
};
static const char* const MW_SQUAD[] = {
    "Company! You want some of my hair?",
    "Hey new friend. I'm made of meat.",
};
static const char* const MW_POKE[] = {
    "Hey! That's my face.",
    "You poked my meat.",
    "Ow. Do it again.",
    "I'm ticklish in the middle.",
};
static const char* const MW_THROWN[] = {
    "Can I do that again?",
    "I'm okay! Most of me is okay!",
    "I got grass in me again.",
    "Whee! I'm a meteor!",
    "My face stayed on. Good.",
    "Throwing ain't nice. Do it again.",
    "I bounced off my own face.",
    "I'm flatter now. Is that bad?",
    "That's how I go places.",
    "I saw the top of the screen!",
    "Somebody catch me next time.",
    "I'm gonna need a minute. I'm meat.",
};
static const uint8_t MW_THROWN_N = sizeof(MW_THROWN) / sizeof(MW_THROWN[0]);
static const char* const MW_SPLIT[] = {
    "Whoa! There's three of me!",
    "We're all gonna get back together.",
    "Which one's me? Oh. All of 'em.",
    "Hold on. Gotta find my other parts.",
};
static const char* const MW_STREAK[] = {
    "That's %d. I'm countin' 'em.",
    "%d throws! I'm a ball, I can take it.",
    "Number %d. My hair fell out.",
    "%d times. Am I a sport now?",
};
static const uint8_t MW_STREAK_N = sizeof(MW_STREAK) / sizeof(MW_STREAK[0]);
static const char* const MW_SQTHROWN[] = {
    "Whoa! You're flyin' like me!",
    "Roll when you land! Roll!",
    "Can I do that next?",
};
static const char* const MW_SCARED[] = {
    "I'm a igloo now. Nobody's home.",
    "Go away. I'm a igloo.",
    "You can't see me. I'm ice.",
};
static const char* const MW_DOG[] = {
    "Look! A hot dog!",
    "Hot dog! Ta-da!",
    "It's a hot dog. You can tell.",
};
static const char* const MW_SHAKE[] = {
    "Shake it! Shake it!",
    "I'm dancing! Don't look!",
    "This is how you do it.",
};
// What Squachy says back, in his own bubble: after he eats a catch, and
// after anything else he says.
static const char* const MW_BACK_EAT[] = {
    "please stop eating the evidence", "chew, buddy. chew.", "that's not food",
    "was that the camera?",
};
static const char* const MW_BACK_TIP[] = {
    "good meatball", "he means well", "you're doing great, buddy", "sure you did",
};

// He eats every catch. These are what he says with his mouth full.
static const char* mwCatchLine(uint8_t type) {
    switch ((DetectionType)type) {
        case DetectionType::FLOCK:       return "I ate the camera. It was lookin' at me funny.";
        case DetectionType::AXON:        return "A body camera. Tasted official.";
        case DetectionType::META:        return "Camera glasses. Crunchy.";
        case DetectionType::SKIMMER:     return "It wanted my card. I ain't got a card.";
        case DetectionType::RAVEN:       return "It listens for bangs. I'm quiet. I'm meat.";
        case DetectionType::AIRTAG:
        case DetectionType::SAMSUNG_TAG:
        case DetectionType::GOOGLE_TAG:
        case DetectionType::TILE:        return "That one beeped all the way down.";
        case DetectionType::DRONE:       return "Come down here. I just wanna talk to you.";
        case DetectionType::ALPR:        return "It reads plates? I been on a plate.";
        case DetectionType::CAMERA:      return "Tasted like batteries. Good batteries.";
        case DetectionType::RING:        return "A doorbell. Ding dong. I ate it.";
        case DetectionType::DEAUTH:      return "Somebody's kickin' the WiFi. Not me.";
        case DetectionType::EVILTWIN:    return "A evil twin. I ain't got no twin. Wait.";
        case DetectionType::HACKER:      return "I don't know what that was. I ate it to be safe.";
        case DetectionType::IBEACON:     return "A shop beacon. I wanna go to the shop.";
        default:                         return nullptr;
    }
}
// Serious enough that, having eaten it, he hides.
static bool mwSerious(uint8_t type) {
    switch ((DetectionType)type) {
        case DetectionType::FLOCK: case DetectionType::AXON: case DetectionType::SKIMMER:
        case DetectionType::RAVEN: case DetectionType::ALPR: case DetectionType::DEAUTH:
        case DetectionType::EVILTWIN: case DetectionType::HACKER:
            return true;
        default:
            return false;
    }
}

// ---- his colours and size ---------------------------------------------------
static const uint16_t MW_BODY  = 0xA203;   // #a2401b
static const uint16_t MW_LINE  = 0x1041;   // #120a08
static const uint16_t MW_SPECK = 0xF718;   // #f4e3c3
static const uint16_t MW_BRICK = 0x7962;   // #7a2e10, the igloo's mortar
static const uint16_t MW_DOOR  = 0x38A1;   // #3a160a
static const uint16_t MW_GULLET = 0x58C2;  // #5a1a10, inside the mouth
static const int MW_W = 30, MW_H = 27;     // his box, feet on its bottom line
static const uint32_t MW_TALK_MS = 5500, MW_MOUTH_MS = 2400;   // the balloon; the moving mouth
static const uint32_t MW_ROLL_MS = 900, MW_LAND_MS = 420;
static const uint32_t MW_FORM_MS = 3600, MW_POOF_MS = 300;
static const uint32_t MW_SPLIT_MS = 2400;
static const float    MW_G = 0.0028f;

enum class MFace : uint8_t { IDLE, TALK, EAT, HAPPY, SCARED, DIZZY };
enum class MPh   : uint8_t { NONE, HELD, AIR, SPLAT, SPLIT };
enum class MForm : uint8_t { BALL, IGLOO, DOG, SHAKE };

static const char* s_mLine    = nullptr;
static uint32_t    s_mSaidAt  = 0, s_mShownMs = 0, s_mLastNow = 0;
static uint32_t    s_mNextTip = 0, s_mNextRoll = 0, s_mPokeAt = 0, s_mScaredAt = 0;
static PetHop      s_mHop;
static bool        s_mCaught = false, s_mAnswered = true, s_mSpotB = false;
static bool        s_mBackEat = false;      // the line waiting for Squachy's reply was a catch
static uint8_t     s_mTip = 0, s_mPoke = 0, s_mPending = 0;
static int16_t     s_mX = -100, s_mY = 0;
static int16_t     s_mLastX = -100;
static int16_t     s_mRot = 0;              // his turn, 0..255 a whole turn, x16 for smoothness
static int8_t      s_mLook = 0;
static uint32_t    s_mLookAt = 0;
// A catch: the morsel falling into him, then the swallow.
static uint32_t    s_mEatAt = 0, s_mHideAt = 0;
static uint8_t     s_mEatType = 0;
static uint16_t    s_mEatCol = 0;
static bool        s_mEatSaid = false;
// The shapes he takes: when, which, for how long.
static MForm       s_mForm = MForm::BALL;
static uint32_t    s_mFormAt = 0, s_mFormMs = 0;
static int8_t      s_mAct = -1;             // asked for from the console or the emulator
// Thrown.
static MPh         s_mPh = MPh::NONE;
static float       s_mfX = 0, s_mfY = 0, s_mvX = 0, s_mvY = 0;
static Grip        s_mG;
static uint32_t    s_mThLast = 0, s_mThAt = 0;
static bool        s_mFlung = false;
static uint8_t     s_mStreak = 0;
static uint32_t    s_mStreakAt = 0;
static char        s_mStreakLine[40];
static bool        s_maValid = false, s_mSqWasThrown = false, s_mWasDancing = false;
static int         s_maCx = 0, s_maHalfW = 0, s_maBot = 0;
static bool        s_mOut = false;           // drawn as the pet last frame (see meatyAway)

static bool mwTalking() { return s_mLine && s_mShownMs < MW_TALK_MS; }
static void mwSay(const char* line, uint32_t now, bool caught) {
    s_mShownMs = 0;
    s_mLine = line; s_mSaidAt = now ? now : 1; s_mCaught = caught; s_mAnswered = caught;
}
static const char* mwStreakLine(uint32_t now) {
    if (now - s_mStreakAt > 60000) s_mStreak = 0;
    s_mStreakAt = now;
    if (s_mStreak < 255) s_mStreak++;
    if (s_mStreak < 3 || random(0, 2)) return nullptr;
    snprintf(s_mStreakLine, sizeof(s_mStreakLine), MW_STREAK[random(0, MW_STREAK_N)], (int)s_mStreak);
    return s_mStreakLine;
}
static void mwShape(MForm f, uint32_t now, uint32_t ms) { s_mForm = f; s_mFormAt = now ? now : 1; s_mFormMs = ms; }
static bool mwInForm(uint32_t now) { return s_mForm != MForm::BALL && now - s_mFormAt < s_mFormMs; }

// ---- drawing at his size -----------------------------------------------------
// Everything about him is drawn in art pixels and scaled by MW_K about an
// anchor (his feet), so the look worked out in the lab is the same at any
// size: doubled on the screens with room (asked for 2026-10-13); as drawn on
// the 135-pixel boards, where double would be a third of the screen; and as
// drawn on the 3.5in boards too, whose pet layer is already doubled (pet.cpp's
// zoom) -- doubling again made him as wide as Squachy. Set each frame.
#if defined(SQW_SMALL)
static const int MW_K_ROOMY = 1;
#else
static const int MW_K_ROOMY = 2;
#endif
static int MW_K = MW_K_ROOMY;
#define MW_SW (MW_W * MW_K)      // his size on the screen (the pets' layer)
#define MW_SH (MW_H * MW_K)
// A roll is a roll: a small bounce, however far. petHopStart grows a hop with
// distance (sized for C1iPPY), which had him bouncing three times, 56 pixels high.
static void mwRollFix() {
    if (s_mHop.height > 3.0f * MW_K) s_mHop.height = 3.0f * MW_K;
    if (s_mHop.ms > MW_ROLL_MS + 300) s_mHop.ms = MW_ROLL_MS + 300;
}
static void mwRollStart(int from, int to, uint32_t now) {
    petHopStart(s_mHop, from, 0.0f, to, now, MW_ROLL_MS, 3.0f * MW_K);
    mwRollFix();
}
static int s_mOx = 0, s_mOy = 0;
static inline void mwAnchor(int x, int y) { s_mOx = x; s_mOy = y; }
static inline int mwSx(int x) { return s_mOx + (x - s_mOx) * MW_K; }
static inline int mwSy(int y) { return s_mOy + (y - s_mOy) * MW_K; }
static inline void mR(TFT_eSPI& t, int x, int y, int w, int h, uint16_t c) { if (w > 0 && h > 0) t.fillRect(mwSx(x), mwSy(y), w * MW_K, h * MW_K, c); }
static inline void mP(TFT_eSPI& t, int x, int y, uint16_t c) { mR(t, x, y, 1, 1, c); }
static inline void mH(TFT_eSPI& t, int x, int y, int w, uint16_t c) { mR(t, x, y, w, 1, c); }
static inline void mV(TFT_eSPI& t, int x, int y, int h, uint16_t c) { mR(t, x, y, 1, h, c); }
static inline void mRR(TFT_eSPI& t, int x, int y, int w, int h, int r, uint16_t c) { if (w > 0 && h > 0) t.fillRoundRect(mwSx(x), mwSy(y), w * MW_K, h * MW_K, r * MW_K, c); }
static inline void mDim(TFT_eSPI& t, int x, int y, int w, int h, uint8_t a) { Theme::dimRegion(t, mwSx(x), mwSy(y), w * MW_K, h * MW_K, a); }

// ---- his body ----------------------------------------------------------------
// Rasterised into a bitmap each frame: which pixels he covers. The bitmap is
// big enough for him squashed flat and swollen at once.
static const int MW_MAP_W = 64, MW_MAP_H = 40;
static uint8_t s_mwBits[(MW_MAP_W * MW_MAP_H + 7) / 8];
static int16_t s_mwX0 = 0, s_mwY0 = 0;
static uint8_t s_mwW = 0, s_mwH = 0;
static inline bool mwIn(int x, int y) {
    x -= s_mwX0; y -= s_mwY0;
    if (x < 0 || y < 0 || x >= s_mwW || y >= s_mwH) return false;
    const int i = y * s_mwW + x;
    return (s_mwBits[i >> 3] >> (i & 7)) & 1;
}
static inline int mwSin(int a8) { return Theme::isin256((uint16_t)((a8 & 255) << 8)); }
static inline int mwCos(int a8) { return mwSin(a8 + 64); }

// Fills the bitmap for him at (cx, fy) -- fy the floor under him -- W x H at
// scale, sw and sh (256 is 1.0) for squash and swell, rot8 his turn, lean the
// pixels his top leans over by (the dance).
static void mwRaster(int cx, int fy, int W, int H, int sw, int sh, int rot8, int lean) {
    const int hw256 = W * sw / 2, hh256 = H * sh / 2;     // half width, half height, x256
    const int hwPx = hw256 >> 8, hhPx = hh256 >> 8;
    const int al = lean < 0 ? -lean : lean;
    s_mwX0 = (int16_t)(cx - hwPx - 3 - al);
    const int wantW = 2 * hwPx + 7 + 2 * al, wantH = fy - (fy - 2 * hhPx - 2);   // no min(): the emulator has none
    s_mwW  = (uint8_t)(wantW < MW_MAP_W ? wantW : MW_MAP_W);
    s_mwY0 = (int16_t)(fy - 2 * hhPx - 2);
    s_mwH  = (uint8_t)(wantH < MW_MAP_H ? wantH : MW_MAP_H);
    memset(s_mwBits, 0, sizeof s_mwBits);
    if (hw256 <= 0 || hh256 <= 0) return;
    const int32_t kU = (int32_t)(20L * 65536L * 256L / hw256);   // pixels x256 to table columns, x65536
    for (int y = 0; y < s_mwH; y++) {
        // v of this row's centre: +1 at his top, -1 at the floor (x256).
        const int32_t v256 = (int32_t)((int32_t)(fy - (s_mwY0 + y)) * 256 - 128) * 256 / hh256 - 256;
        const int vi = (int)((320 - v256) * 20 >> 8);
        if (vi < 0 || vi >= MW_ROWS) continue;
        const int shift = lean * (int)(v256 + 256) / 512;          // the lean: none at the floor
        for (int x = 0; x < s_mwW; x++) {
            int32_t dx = (int32_t)(s_mwX0 + x - shift - cx) * 256 + 128;
            const bool left = dx < 0;
            if (left) dx = -dx;
            const int ui = (int)((dx * kU) >> 24);
            if (ui >= MW_COLS) continue;
            int a = MW_A[vi][ui];
            if (left) a = (128 - a) & 255;
            if (MW_R[vi][ui] < MW_BUMP[(a - rot8) & 255]) {
                const int i = y * s_mwW + x;
                s_mwBits[i >> 3] |= (uint8_t)(1 << (i & 7));
            }
        }
    }
}
// Lays the bitmap down: the meat a run at a time, then a line round it.
static void mwPaint(TFT_eSPI& t, uint16_t fill) {
    for (int y = 0; y < s_mwH; y++) {
        int run = -1;
        for (int x = 0; x <= s_mwW; x++) {
            const bool on = x < s_mwW && mwIn(s_mwX0 + x, s_mwY0 + y);
            if (on && run < 0) run = x;
            if (!on && run >= 0) { mH(t, s_mwX0 + run, s_mwY0 + y, x - run, fill); run = -1; }
        }
    }
    for (int y = -1; y <= s_mwH; y++)
        for (int x = -1; x <= s_mwW; x++) {
            const int X = s_mwX0 + x, Y = s_mwY0 + y;
            if (mwIn(X, Y)) continue;
            if (mwIn(X - 1, Y) || mwIn(X + 1, Y) || mwIn(X, Y - 1) || mwIn(X, Y + 1)) mP(t, X, Y, MW_LINE);
        }
}

// ---- his face ------------------------------------------------------------------
// Each eye is 6 wide by 5 tall: a ring, a lid line across the top, white below
// it, and a 2 x 2 pupil tucked up under the lid; a lash flicks off the outer
// corner. The two share the column between them.
static void mwLash(TFT_eSPI& t, int x, int y, bool right) {
    if (right) { mP(t, x + 5, y, MW_LINE); mP(t, x + 6, y - 1, MW_LINE); }
    else       { mP(t, x, y, MW_LINE);     mP(t, x - 1, y - 1, MW_LINE); }
}
static void mwEye(TFT_eSPI& t, int x, int y, MFace f, bool blink, int look, uint32_t now, bool right) {
    if (f == MFace::HAPPY) {
        mP(t, x + 1, y + 2, MW_LINE); mH(t, x + 2, y + 1, 2, MW_LINE); mP(t, x + 4, y + 2, MW_LINE);
        return;
    }
    mV(t, x, y + 1, 3, MW_LINE); mV(t, x + 5, y + 1, 3, MW_LINE);
    mH(t, x + 1, y + 4, 4, MW_LINE);
    if (blink) { mH(t, x + 1, y + 2, 4, MW_LINE); mwLash(t, x, y + 1, right); return; }
    mR(t, x + 1, y + 1, 4, 3, Theme::WHITE);
    mH(t, x + 1, y, 4, MW_LINE);
    if (f == MFace::SCARED) { mP(t, x + 2 + (right ? 1 : 0), y + 2, MW_LINE); return; }
    if (f == MFace::DIZZY) {
        static const int8_t O[4][2] = { { 1, 1 }, { 2, 1 }, { 2, 2 }, { 1, 2 } };
        const int k = (int)((now / 160) & 3);
        mP(t, x + 1 + O[k][0], y + O[k][1], MW_LINE);
        return;
    }
    mwLash(t, x, y, right);
    int px = 3 + look;                       // up under the lid, looking up and to his left
    if (px < 1) px = 1;
    if (px > 3) px = 3;
    mR(t, x + px, y + 1, 2, 2, MW_LINE);
}
// The mouth sits under his left eye, a frown, one tooth hanging from its top.
// While he talks it flips between three shapes, the way his does.
enum : uint8_t { MW_CLOSED, MW_SMALL, MW_WIDE, MW_EAT, MW_SMILE, MW_OH };
static void mwMouth(TFT_eSPI& t, int mx, int my, uint8_t shape) {
    const uint16_t L = MW_LINE, T = Theme::WHITE;
    switch (shape) {
        case MW_SMALL:
            mH(t, mx - 3, my - 1, 7, L); mR(t, mx - 4, my, 9, 2, L);
            mP(t, mx - 5, my + 2, L); mH(t, mx - 4, my + 2, 2, L); mH(t, mx - 1, my + 2, 2, L);
            mH(t, mx + 3, my + 2, 2, L); mP(t, mx + 5, my + 2, L);
            mR(t, mx, my, 2, 2, T);
            break;
        case MW_WIDE:
            mH(t, mx - 3, my - 2, 7, L); mR(t, mx - 5, my - 1, 11, 3, L); mR(t, mx - 6, my + 1, 13, 2, L);
            mP(t, mx - 6, my + 3, L); mH(t, mx - 4, my + 3, 2, L); mH(t, mx - 1, my + 3, 3, L);
            mH(t, mx + 3, my + 3, 2, L); mP(t, mx + 6, my + 3, L);
            mR(t, mx, my - 1, 2, 2, T);
            break;
        case MW_EAT:
            mH(t, mx - 3, my - 3, 7, L); mR(t, mx - 4, my - 2, 9, 5, L); mH(t, mx - 3, my + 3, 7, L);
            mR(t, mx - 1, my - 2, 2, 2, T); mH(t, mx - 2, my + 1, 5, MW_GULLET);
            break;
        case MW_SMILE:
            mP(t, mx - 3, my - 1, L); mP(t, mx - 2, my, L); mH(t, mx - 1, my + 1, 3, L);
            mP(t, mx + 2, my, L); mP(t, mx + 3, my - 1, L);
            mH(t, mx - 1, my, 2, T);
            break;
        case MW_OH:
            mR(t, mx - 1, my - 1, 3, 3, L); mP(t, mx, my - 1, T);
            break;
        default:   // MW_CLOSED: the frown, the tooth under its top
            mP(t, mx - 5, my + 2, L); mP(t, mx - 4, my + 1, L); mP(t, mx - 3, my, L);
            mH(t, mx - 2, my - 1, 5, L);
            mP(t, mx + 3, my, L); mP(t, mx + 4, my + 1, L); mP(t, mx + 5, my + 2, L);
            mR(t, mx, my, 2, 2, T); mP(t, mx - 1, my, L); mP(t, mx + 2, my, L); mH(t, mx, my + 2, 2, L);
            break;
    }
}
static const uint8_t MW_TALK[8] = { MW_CLOSED, MW_WIDE, MW_SMALL, MW_WIDE, MW_CLOSED, MW_SMALL, MW_WIDE, MW_SMALL };
static void mwFace(TFT_eSPI& t, int ex, int ey, int mx, int my, MFace f, uint32_t now, bool mouthMoving) {
    int dx = 0, dy = 0;
    if (f == MFace::TALK && mouthMoving) {
        static const int8_t DX[6] = { 0, 1, 1, 0, -1, 0 }, DY[6] = { 0, 0, 1, 1, 0, -1 };
        const uint32_t k = (now / 220) % 6;
        dx = DX[k]; dy = DY[k];
    }
    const bool blink = (f == MFace::IDLE || f == MFace::TALK) && (now % 4300u) < 140;
    mwEye(t, ex - 5 + dx, ey + dy, f, blink, s_mLook, now, false);
    mwEye(t, ex + dx, ey + dy, f, blink, s_mLook, now, true);
    uint8_t m = MW_CLOSED;
    if (f == MFace::TALK) m = mouthMoving ? MW_TALK[(now / 90) % 8] : MW_CLOSED;
    else if (f == MFace::EAT) m = MW_EAT;
    else if (f == MFace::HAPPY) m = MW_SMILE;
    else if (f == MFace::SCARED || f == MFace::DIZZY) m = MW_OH;
    mwMouth(t, mx + dx, my + dy, m);
}

// Specks: (angle, distance out x100, length), turning with him.
static const uint8_t MW_SPECKS[12][3] = {
    { 16, 82, 2 }, { 45, 86, 1 }, { 69, 80, 2 }, { 94, 84, 1 }, { 116, 78, 2 }, { 139, 86, 1 },
    { 163, 70, 1 }, { 183, 83, 2 }, { 208, 80, 1 }, { 236, 84, 2 }, { 106, 45, 1 }, { 159, 40, 2 } };
static const uint8_t MW_HAIRS[2] = { 72, 59 };   // out of the top

// The whole of him as a ball: body, specks, hairs, face. sw/sh/swell x256.
static void mwBall(TFT_eSPI& t, int cx, int fy, int W, int H, int sw, int sh, int rot8, int lean,
                   MFace f, uint32_t now, bool mouthMoving, bool air) {
    if (!air) mDim(t, cx - W * sw / 600, fy, W * sw / 300, 2, 128);
    mwRaster(cx, fy, W, H, sw, sh, rot8, lean);
    mwPaint(t, MW_BODY);
    const int hw = W * sw / 512, hh = H * sh / 512;
    // the face's box, kept clear of specks
    const int fx0 = cx - W * 30 / 100, fx1 = cx + W * 42 / 100, fy0 = fy - hh * 2 * 68 / 100;
    for (uint8_t i = 0; i < 12; i++) {
        const int a = MW_SPECKS[i][0] + rot8;
        const int x = cx + mwCos(a) * MW_SPECKS[i][1] * hw / 25600;
        const int y = fy - hh - mwSin(a) * MW_SPECKS[i][1] * hh / 25600;
        const int len = MW_SPECKS[i][2];
        if (!mwIn(x, y) || !mwIn(x + len - 1, y)) continue;
        if (x > fx0 && x < fx1 && y > fy0 && y < fy - 2) continue;
        mH(t, x, y, len, MW_SPECK);
    }
    if (sh < 100) return;                                  // all but flat: the poof into a shape
    for (uint8_t i = 0; i < 2; i++) {
        const int a = MW_HAIRS[i] + rot8, c = mwCos(a), s = mwSin(a);
        if (s < -50) continue;                            // under him
        int bx = cx, by = fy - hh;
        for (int f100 = 50; f100 < 140; f100 += 3) {
            bx = cx + c * f100 * hw / 25600; by = fy - hh - s * f100 * hh / 25600;
            if (!mwIn(bx, by)) break;
        }
        for (int j = 1; j <= 3; j++) {
            const int curl = j == 3 ? (i ? 1 : -1) : 0;
            mP(t, bx + (c * j - s * curl) / 256, by - (s * j + c * curl) / 256, MW_LINE);
        }
    }
    // The face stays put while the meat turns; it squashes with him.
    const int ex = cx + W * 17 / 100 + lean / 2, ey = fy - hh * 2 * 55 / 100;
    const int mx = cx - W * 4 / 100 + lean / 4, my = fy - hh * 2 * 28 / 100;
    mwFace(t, ex, ey, mx, my, f, now, mouthMoving);
}

// A small whole-number square root, for the igloo's dome.
static int mwIsqrt(int v) { int r = 0; while ((r + 1) * (r + 1) <= v) r++; return r; }

// The igloo, made of him: a dome of meat with mortar lines, a door low on the
// left, and his face on the right of it. k (0..256) grows it in.
static void mwIgloo(TFT_eSPI& t, int cx, int fy, int k, MFace f, uint32_t now, bool mouthMoving) {
    const int R = 15 * k / 256, Hh = 17 * k / 256;
    if (R < 2 || Hh < 2) return;
    mDim(t, cx - R, fy, 2 * R, 2, 128);
    for (int i = 0; i < Hh; i++) {
        const int d = Hh - i;
        const int hw = mwIsqrt(R * R * (Hh * Hh - d * d + d) / (Hh * Hh));
        const int y = fy - Hh + i;
        mH(t, cx - hw - 1, y, 2 * hw + 2, MW_LINE);
        if (hw > 0) mH(t, cx - hw, y, 2 * hw, MW_BODY);
    }
    mH(t, cx - 3, fy - Hh - 1, 6, MW_LINE);
    if (k < 200) return;
    for (int y = fy - 4, row = 0; y > fy - Hh + 1; y -= 4, row++) {
        const int d = fy - y;
        const int hw = mwIsqrt(R * R * (Hh * Hh - d * d) / (Hh * Hh)) - 1;
        if (hw > 1) mH(t, cx - hw, y, 2 * hw, MW_BRICK);
        for (int x = cx - hw + 2 + (row & 1) * 3; x < cx + hw - 1; x += 6) mV(t, x, y - 3, 3, MW_BRICK);
    }
    const int dx = cx - 8;
    mR(t, dx - 4, fy - 5, 8, 5, MW_DOOR); mH(t, dx - 3, fy - 6, 6, MW_DOOR); mH(t, dx - 2, fy - 7, 4, MW_DOOR);
    mwFace(t, cx + 7, fy - 11, cx + 3, fy - 5, f, now, mouthMoving);
}
// The hot dog: him stretched long, rounded at both ends, his face at the
// front. k (0..256) stretches him out.
static void mwDog(TFT_eSPI& t, int cx, int fy, int k, MFace f, uint32_t now, bool mouthMoving) {
    const int L = 26 + 22 * k / 256, h = 26 - 13 * k / 256;
    const int x = cx - L / 2, y = fy - h;
    mDim(t, x, fy, L, 2, 128);
    mRR(t, x - 1, y - 1, L + 2, h + 2, (h + 2) / 2, MW_LINE);
    mRR(t, x, y, L, h, h / 2, MW_BODY);
    if (k < 200) return;
    mH(t, x + 5, y + 3, 2, MW_SPECK); mP(t, x + 12, y + 2, MW_SPECK); mH(t, x + 9, y + h - 3, 2, MW_SPECK);
    mwFace(t, x + L - 9, y + 3, x + L - 14, y + h - 3, f, now, mouthMoving);
}
// One of the three small ones he splits into.
static void mwMini(TFT_eSPI& t, int cx, int fy, int rot8) {
    mwRaster(cx, fy, 12, 11, 256, 256, rot8, 0);
    mwPaint(t, MW_BODY);
    mR(t, cx, fy - 7, 2, 2, Theme::WHITE); mR(t, cx + 3, fy - 7, 2, 2, Theme::WHITE);
    mH(t, cx - 1, fy - 7, 7, MW_LINE); mP(t, cx + 1, fy - 6, MW_LINE); mP(t, cx + 4, fy - 6, MW_LINE);
    mH(t, cx - 1, fy - 4, 3, MW_LINE); mP(t, cx, fy - 3, Theme::WHITE);
}
// The morsel of a catch, in its colour.
static void mwMorsel(TFT_eSPI& t, int x, int y, uint16_t col) {
    mR(t, x - 2, y - 2, 5, 4, MW_LINE); mR(t, x - 1, y - 1, 3, 2, col); mP(t, x, y - 1, Theme::WHITE);
}

// ---- the plumbing pet.cpp calls ------------------------------------------------
static void meatyNoteCatch(uint8_t type) { s_mPending = (uint8_t)(type + 1); }
static bool meatyHit(int x, int y) {
    if (s_mX < -50) return false;
    return x >= s_mX - MW_SW / 2 - 6 && x <= s_mX + MW_SW / 2 + 6 && y >= s_mY - MW_SH - 6 && y <= s_mY + 2;
}
static bool meatyCenter(int& x, int& y) {
    if (s_mX < -50) return false;
    x = s_mX; y = s_mY - MW_SH / 2;
    return true;
}
static void meatyPoke(uint32_t now) {
    if (now - s_mPokeAt > 8000) s_mPoke = 0;
    s_mPokeAt = now;
    mwSay(MW_POKE[s_mPoke % 4], now, false);
    s_mAnswered = true;
    s_mPoke++;
}
static void meatyGrab(int x, int y, uint32_t now) {
    if (s_mX < -50) return;
    s_mPh = MPh::HELD;
    s_mfX = (float)s_mX; s_mfY = (float)s_mY;
    gripStart(s_mG, x, y, s_mX, s_mY, now);
    s_mHop.at = 0;
    s_mForm = MForm::BALL;
    s_mHideAt = 0;
    if (s_mEatAt && !s_mEatSaid) s_mEatAt = 0;      // he never got to swallow it
}
static void meatyDrag(int x, int y, uint32_t now) {
    if (s_mPh != MPh::HELD) return;
    gripMove(s_mG, x, y, now);
}
static void mwLaunch(float vx, float vy, uint32_t now) {
    if (vx > CLIP_MAX_V) vx = CLIP_MAX_V;
    if (vx < -CLIP_MAX_V) vx = -CLIP_MAX_V;
    if (vy > CLIP_MAX_V) vy = CLIP_MAX_V;
    if (vy < -CLIP_MAX_V) vy = -CLIP_MAX_V;
    s_mvX = vx; s_mvY = vy;
    s_mPh = MPh::AIR;
    s_mThAt = s_mThLast = now;
}
static void meatyRelease(uint32_t now) {
    if (s_mPh != MPh::HELD) return;
    float vx, vy;
    switch (gripLetGo(s_mG, now, vx, vy)) {
        case 0:  s_mPh = MPh::NONE; meatyPoke(now); break;
        case 1:  mwLaunch(vx, vy, now); s_mFlung = true; noteThrow(now); Sound::play(Sound::Id::THROW); break;
        case 2:  mwLaunch(0, 0, now); s_mFlung = false; break;
        default: s_mPh = MPh::NONE; break;
    }
}
// The console's and the emulator's: 0 igloo, 1 hot dog, 2 Shake it, 3 three of him.
void meatyAct(int8_t which) { if (meatyOn()) s_mAct = which; }   // only when he is the pet

static void meatyTick(TFT_eSPI& t, uint32_t now, int screenW, int floorY) {
    s_mOut = true;
    MW_K = s_k > 1 ? 1 : MW_K_ROOMY;
    int cx, halfW, top, bot;
    if (!zFootprint(cx, halfW, top, bot)) { s_mX = -100; return; }
    (void)top;
    bot = floorY;
    const bool sqAway = Squachy::isHeld() || Squachy::thrown();
    if (sqAway) {
        if (!s_maValid) { s_mX = -100; return; }
        cx = s_maCx; halfW = s_maHalfW; bot = s_maBot;
    } else {
        s_maCx = cx; s_maHalfW = halfW; s_maBot = bot; s_maValid = true;
    }
    const bool sqThrown = Squachy::thrown();
    if (sqThrown && !s_mSqWasThrown && s_mPh == MPh::NONE) {
        s_mScaredAt = now ? now : 1;
        if (!mwTalking()) { mwSay(MW_SQTHROWN[random(0, 3)], now, false); s_mAnswered = true; }
    }
    s_mSqWasThrown = sqThrown;
    uint32_t seenMs = (s_mLastNow && now - s_mLastNow < 100000u) ? now - s_mLastNow : 0;
    if (seenMs > 100) seenMs = 100;
    s_mLastNow = now;
    const int screenH = t.height();
    // Two spots beside Squachy, the right-hand side when it has room. The
    // spot is his CENTRE: he is drawn from the middle out.
    int a = cx + halfW + 4 + MW_SW / 2, b = a + MW_SW + 4;
    if (b + MW_SW / 2 + 4 > screenW - 2) b = a;
    if (a + MW_SW / 2 + 4 > screenW - 2) { a = cx - halfW - 4 - MW_SW / 2; b = a - MW_SW - 4; if (b - MW_SW / 2 < 4) b = a; }

    if (const char* intro = Squachy::takeMeatyIntro()) {
        mwSay(intro, now, false);
        s_mAnswered = true;
        Theme::showToast("MEATY", "your new pet. Settings > Pet", Theme::AMBER, 3500);
        s_mNextTip = now + 60000;
    }
    if (!s_mNextTip) s_mNextTip = now + 8000;
    if (!s_mNextRoll) s_mNextRoll = now + 30000 + (uint32_t)random(0, 20000);
    // His eyes wander now and then.
    if (now - s_mLookAt > 1700) { s_mLookAt = now; static const int8_t L[5] = { 0, -1, 1, 0, 0 }; s_mLook = L[random(0, 5)]; }
    const bool rolling = petHopping(s_mHop, now);
    const bool eating = s_mEatAt && now - s_mEatAt < 2600;
    const bool busy = s_mPh != MPh::NONE || rolling || eating || mwInForm(now);
    // Asked for from the console or the emulator.
    if (s_mAct >= 0 && s_mPh == MPh::NONE) {
        const int8_t act = s_mAct;
        s_mAct = -1;
        if (act == 0) { mwShape(MForm::IGLOO, now, MW_FORM_MS); mwSay(MW_SCARED[random(0, 3)], now, false); }
        else if (act == 1) { mwShape(MForm::DOG, now, MW_FORM_MS); mwSay(MW_DOG[random(0, 3)], now, false); }
        else if (act == 2) { mwShape(MForm::SHAKE, now, 4000); mwSay(MW_SHAKE[random(0, 3)], now, false); }
        else if (act == 3) { s_mPh = MPh::SPLIT; s_mThAt = now; s_mfX = (float)(s_mX > -50 ? s_mX : (s_mSpotB ? b : a)); s_mfY = (float)bot; mwSay(MW_SPLIT[random(0, 4)], now, false); }
        s_mAnswered = true;
    }
    // Squachy dancing: so is he.
    const bool dancing = Squachy::dancing();
    if (dancing && !s_mWasDancing && s_mPh == MPh::NONE && !eating) {
        mwShape(MForm::SHAKE, now, 4000);
        if (!mwTalking()) mwSay(MW_SHAKE[random(0, 3)], now, false);
    }
    s_mWasDancing = dancing;
    // A catch: a morsel in that device's colour drops into him.
    if (s_mPending && !busy) {
        s_mEatType = (uint8_t)(s_mPending - 1);
        s_mPending = 0;
        s_mEatAt = now ? now : 1;
        s_mEatSaid = false;
        s_mEatCol = Theme::colorFor((DetectionType)s_mEatType);
        s_mForm = MForm::BALL;
    }
    if ((int32_t)(now - s_mNextTip) >= 0 && !mwTalking() && !busy) {
        // One tip in three he shows you a hot dog instead.
        if (random(0, 3) == 0) {
            mwShape(MForm::DOG, now, MW_FORM_MS);
            mwSay(MW_DOG[random(0, 3)], now, false);
            s_mBackEat = false;
        } else {
            const char* l = MW_TIPS[s_mTip++ % MW_TIPS_N];
            if (Squachy::visiting() && random(0, 2) == 0) l = MW_SQUAD[random(0, 2)];
            else if (Clock::night() && random(0, 3) == 0) l = MW_NIGHT[random(0, 2)];
            mwSay(l, now, false);
            s_mBackEat = false;
        }
        s_mNextTip = now + 60000 + (uint32_t)random(0, 30000);
    }
    const bool talking = mwTalking();
    const bool mouthMoving = talking && now - s_mSaidAt < MW_MOUTH_MS;
    if (s_mLine && !talking && !s_mAnswered) {
        s_mAnswered = true;
        if (random(0, 2) == 0) Squachy::visitSay(s_mBackEat ? MW_BACK_EAT[random(0, 4)] : MW_BACK_TIP[random(0, 4)]);
    }
    if (!talking && !busy && (int32_t)(now - s_mNextRoll) >= 0 && a != b) {
        const int from = s_mSpotB ? b : a;
        s_mSpotB = !s_mSpotB;
        mwRollStart(from, s_mSpotB ? b : a, now);
        s_mNextRoll = now + 30000 + (uint32_t)random(0, 20000);
    }
    const int to = s_mSpotB ? b : a;
    // He rolls: the meat turns by how far he went (rot in 16ths of a step).
    auto turnBy = [&](int newX) {
        const int d = newX - s_mLastX;
        if (s_mLastX > -100 && d < MW_SW && d > -MW_SW) s_mRot = (int16_t)(s_mRot - d * 16 * 256 / (int)(3.14159f * MW_SW));
        s_mLastX = (int16_t)newX;
    };

    // ---- picked up, thrown, splatted, or in three pieces ----
    if (s_mPh != MPh::NONE) {
        const float floorAt = (float)bot;
        const float leftX = (float)(MW_SW / 2 + 2), rightX = (float)(screenW - MW_SW / 2 - 2), ceilY = (float)(16 + MW_SH);
        MFace f = MFace::SCARED;
        int sw = 256, sh = 256;
        bool air = false;
        if (s_mPh == MPh::HELD) {
            s_mfX = (float)(s_mG.fingerX - s_mG.offX);
            s_mfY = (float)(s_mG.fingerY - s_mG.offY);
            if (s_mfX < leftX) s_mfX = leftX;
            if (s_mfX > rightX) s_mfX = rightX;
            if (s_mfY < ceilY) s_mfY = ceilY;
            if (s_mfY > floorAt) s_mfY = floorAt;
            if (!s_mG.moved) f = MFace::IDLE;
            else { sw = 236; sh = 280; }                 // dangling: meat stretches
            air = true;
            if (now - s_mG.gT > 600 && s_mG.moved) { mwLaunch(0, 0, now); s_mFlung = false; }
        } else if (s_mPh == MPh::AIR) {
            air = true;
            uint32_t dt = now - s_mThLast;
            if (dt > 60) dt = 60;
            s_mThLast = now;
            while (dt && s_mPh == MPh::AIR) {
                const float st = (float)(dt > 8 ? 8 : dt);
                dt -= (dt > 8 ? 8 : dt);
                s_mvY += MW_G * st;
                s_mfX += s_mvX * st;
                s_mfY += s_mvY * st;
                s_mRot = (int16_t)(s_mRot - (int)(s_mvX * st * 16.0f * 256.0f / (3.14159f * MW_SW)) - (s_mvX >= 0 ? 8 : -8) * (int)st);
                if (s_mfX < leftX && s_mvX < 0)  { s_mfX = leftX;  s_mvX = -s_mvX * 0.5f; }
                if (s_mfX > rightX && s_mvX > 0) { s_mfX = rightX; s_mvX = -s_mvX * 0.5f; }
                if (s_mfY < ceilY && s_mvY < 0)  { s_mfY = ceilY;  s_mvY = -s_mvY * 0.3f; }
                if (s_mfY >= floorAt && s_mvY > 0) {
                    // Meat does not bounce. It splats, or it comes apart.
                    s_mfY = floorAt;
                    s_mThAt = now;
                    const bool hard = s_mvY > 0.75f;
                    if (s_mFlung) { zLandedAt((int)s_mfX); Sound::play(Sound::Id::LAND); }
                    const char* streak = s_mFlung ? mwStreakLine(now) : nullptr;
                    if (hard && s_mFlung) {
                        s_mPh = MPh::SPLIT;
                        s_mvX = 0;
                        mwSay(streak ? streak : MW_SPLIT[random(0, 4)], now, false);
                    } else {
                        s_mPh = MPh::SPLAT;
                        if (s_mFlung) mwSay(streak ? streak : MW_THROWN[random(0, MW_THROWN_N)], now, false);
                    }
                    s_mvY = 0;
                    s_mAnswered = true;
                    s_mFlung = false;
                }
            }
            if (s_mPh == MPh::AIR && now - s_mThAt > 5000) { s_mfY = floorAt; s_mPh = MPh::SPLAT; s_mThAt = now; s_mFlung = false; }
        } else if (s_mPh == MPh::SPLAT) {
            f = MFace::DIZZY;
            const uint32_t e = now - s_mThAt;
            // flat on landing, rounding back out, rolling to a stop
            if (e < MW_LAND_MS) { sw = 256 + 115 * (int)(MW_LAND_MS - e) / (int)MW_LAND_MS; sh = 256 - 141 * (int)(MW_LAND_MS - e) / (int)MW_LAND_MS; }
            s_mfX += s_mvX * (float)seenMs; s_mvX *= 0.97f;
            if (s_mfX < leftX) s_mfX = leftX;
            if (s_mfX > rightX) s_mfX = rightX;
            if (e > 2200) { s_mPh = MPh::NONE; mwRollStart((int)lroundf(s_mfX), to, now); }
        } else if (s_mPh == MPh::SPLIT) {
            const uint32_t e = now - s_mThAt;
            if (e > MW_SPLIT_MS) { s_mPh = MPh::NONE; s_mLastX = (int16_t)lroundf(s_mfX); mwRollStart((int)lroundf(s_mfX), to, now); }
        }
        if (s_mPh != MPh::NONE) {
            s_mX = (int16_t)lroundf(s_mfX);
            s_mY = (int16_t)lroundf(s_mfY);
            mwAnchor(s_mX, s_mY);
            if (s_mPh == MPh::SPLIT) {
                // Three of him: apart, a moment, and rolling back together.
                const uint32_t e = now - s_mThAt;
                int k = e < 360 ? (int)(e * 256 / 360) : e < 1800 ? 256 : 256 - (int)((e - 1800) * 256 / (MW_SPLIT_MS - 1800));
                if (k < 0) k = 0;
                const int sp = 16 * k / 256;
                const int r8 = (s_mRot >> 4) + (int)(e / 6);
                mwMini(t, s_mX - sp - 4, s_mY, r8);
                mwMini(t, s_mX - 4, s_mY, -r8);
                mwMini(t, s_mX + sp - 4, s_mY, r8 + 40);
            } else {
                if (s_mPh == MPh::SPLAT) turnBy(s_mX);          // rolling to a stop
                else s_mLastX = s_mX;                           // held, or flying (the flight turns him)
                mwBall(t, s_mX, s_mY, MW_W, MW_H, sw, sh, s_mRot >> 4, 0, f, now, false, air);
            }
            if (mwTalking() && (s_mPh == MPh::SPLAT || s_mPh == MPh::SPLIT)) {
                petBalloon(t, s_mLine, s_mX - MW_SW / 2, s_mY - MW_SH, MW_SW, screenW, screenH);
                s_mShownMs += seenMs;
            }
            return;
        }
    }

    // ---- on his spot ----
    petHopGuard(s_mHop, to, cx, halfW, s_mX, now, MW_ROLL_MS, 3.0f * MW_K);
    mwRollFix();                            // a restart from the guard grows with distance too
    float x = (float)to, hop = 0.0f;
    MFace face = talking ? MFace::TALK : MFace::IDLE;
    const uint32_t since = now - s_mHop.at;
    if (rolling) {
        // A roll, not a hop: eased along the ground with a little bounce.
        const float k = (float)since / (float)s_mHop.ms;
        const float e = 0.5f - 0.5f * cosf(k * 3.14159265f);
        x = (float)s_mHop.fromX + (float)(to - s_mHop.fromX) * e;
        hop = s_mHop.fromLift * (1.0f - k) - fabsf(sinf(k * 3.14159265f * 3.0f)) * s_mHop.height;
    }
    s_mHop.lift = hop;
    s_mX = (int16_t)lroundf(x);
    s_mY = (int16_t)(bot + (int)lroundf(hop));
    mwAnchor(s_mX, s_mY);
    turnBy(s_mX);
    int swell = 256;
    int morselY = -1000;
    if (s_mEatAt) {
        const uint32_t age = now - s_mEatAt;
        if (age < 700) {
            face = MFace::EAT;
            morselY = (s_mY - MW_H - 30) + (int)((MW_H + 30) * age / 700);
            if (morselY > s_mY - MW_H * 28 / 100) morselY = s_mY - MW_H * 28 / 100;
        } else {
            if (!s_mEatSaid) {
                s_mEatSaid = true;
                if (const char* l = mwCatchLine(s_mEatType)) { mwSay(l, now, false); s_mBackEat = true; s_mNextTip = now + 60000; }
                if (mwSerious(s_mEatType)) s_mHideAt = now + 1600;
            }
            const uint32_t e = age - 700;
            swell = e < 250 ? 292 : e < 1300 ? 292 - 36 * (int)(e - 250) / 1050 : 256;
            if (e < 1100) face = MFace::HAPPY;
            if (age > 60000) s_mEatAt = 0;
        }
    }
    if (s_mHideAt && (int32_t)(now - s_mHideAt) >= 0) {
        s_mHideAt = 0;
        mwShape(MForm::IGLOO, now, MW_FORM_MS);
        mwSay(MW_SCARED[random(0, 3)], now, false);
        s_mAnswered = true;
    }
    if (s_mScaredAt && now - s_mScaredAt < 2500) face = MFace::SCARED;
    else s_mScaredAt = 0;

    // ---- his shapes: a poof down, the shape, a poof back ----
    if (mwInForm(now) && s_mForm != MForm::SHAKE) {
        const uint32_t e = now - s_mFormAt;
        int k = 256;
        if (e < MW_POOF_MS) k = (int)(e * 256 / MW_POOF_MS);
        else if (e > s_mFormMs - MW_POOF_MS) k = (int)((s_mFormMs - e) * 256 / MW_POOF_MS);
        if (k < 128) {
            // the ball sinking into the change
            mwBall(t, s_mX, s_mY, MW_W, MW_H, 256 + (128 - k) / 2, k * 2, s_mRot >> 4, 0, MFace::SCARED, now, false, false);
        } else {
            const int kk = (k - 128) * 2;
            const MFace sf = talking ? MFace::TALK : (s_mForm == MForm::DOG ? MFace::HAPPY : MFace::IDLE);
            if (s_mForm == MForm::IGLOO) mwIgloo(t, s_mX, s_mY, 128 + kk / 2, sf, now, mouthMoving);
            else mwDog(t, s_mX, s_mY, 128 + kk / 2, sf, now, mouthMoving);
        }
    } else {
        int lean = 0, sw = swell, sh = swell, lift = 0;
        if (mwInForm(now) && s_mForm == MForm::SHAKE) {
            // Shake it: a sway that is mostly hips, and no rhythm at all.
            const uint32_t e = now - s_mFormAt;
            const int s1 = Theme::isin256((uint16_t)(e * 65536u / 690u));
            lean = s1 * 9 / 256;
            const int as = s1 < 0 ? -s1 : s1;
            sh = swell - as * 26 / 256; sw = swell + as * 15 / 256;
            const int s2 = Theme::isin256((uint16_t)(e * 65536u / 1380u));
            lift = (s2 < 0 ? -s2 : s2) * 3 / 256;
            if (!talking) face = MFace::HAPPY;
        } else if (s_mHop.at && since >= s_mHop.ms && since < s_mHop.ms + 160) {
            sh = swell - 30 * (int)(s_mHop.ms + 160 - since) / 160;      // settling after the roll
        }
        mwBall(t, s_mX, s_mY - lift, MW_W, MW_H, sw, sh, s_mRot >> 4, lean, face, now, mouthMoving, hop < -0.5f);
    }
    if (morselY > -1000) mwMorsel(t, s_mX - MW_W * 4 / 100, morselY, s_mEatCol);
    if (talking && !rolling) {
        petBalloon(t, s_mLine, s_mX - MW_SW / 2, s_mY - MW_SH, MW_SW, screenW, screenH);
        s_mShownMs += seenMs;
    }
}

static void meatyReset() {
    s_mX = -100;
    s_mLastX = -100;
    s_mHop = PetHop();
    s_mPh = MPh::NONE;
    s_mForm = MForm::BALL;
    s_mEatAt = 0;
    s_mHideAt = 0;
    s_mAct = -1;
    s_mPending = 0;
    s_mLine = nullptr;
    s_maValid = false;
}
// Called every frame he is not the pet on screen (another pet chosen, or the
// ball out): the first such frame puts everything he was in the middle of
// away, so nothing from before -- a hide on its way, a hold, a banked catch --
// fires the moment he is chosen again.
static void meatyAway() { if (s_mOut) { meatyReset(); s_mOut = false; } }
