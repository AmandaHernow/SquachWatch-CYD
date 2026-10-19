// SquachWatch-CYD — the radio half of the firmware, stood in for on the Tab5.
//
// The ESP32-P4 has no radio of its own; the Tab5's ESP32-C6 hangs off SDIO
// and is not driven this round. So the four files that include NimBLE, WiFi
// or SD (detection.cpp, sd_log.cpp, ota_ble.cpp, ota_wifi.cpp) are left out
// of [env:tab5] (its build_src_filter) and this file supplies what they
// would have: the emulator's own stand-in for the detection engine and the
// SD log, unchanged, plus an OtaBle/OtaWifi that say "not available".
//
// When the C6 is driven, this file goes and those four come back.
#if defined(TAB5)

// The emulator's engine: same class from the same header, in-memory data, no
// radio. Included rather than copied so the two can never drift.
#include "../sim/detection_sim.cpp"

#include "ota_ble.h"
#include "ota_wifi.h"
#include <Arduino.h>
#include <string.h>

// ---- What the emulator gets from main_live.cpp / meshsim.cpp -------------
// The radio is never asleep because there is none; the mesh never
// advertises; the RADIO console command says so.
bool DetectionEngine::restRadios(bool) { return false; }
void DetectionEngine::wakeRadios() {}
void radioReport(bool) { Serial.println("[radio] none on this board: the Tab5's ESP32-C6 is not driven yet"); }
#if SQUACH_MESH
namespace Mesh {
TxStatus txStatus() { return TxStatus{ false, false, 0, 0, 0 }; }
void radioTick(uint32_t) {}
}
#endif

// ---- OtaBle: no Bluetooth on this board ------------------------------------
namespace OtaBle {
bool        available()     { return false; }
bool        begin()         { return false; }
void        end()           {}
void        tick(uint32_t)  {}
State       state()         { return State::OFF; }
bool        codeAccepted()  { return false; }
uint8_t     percent()       { return 0; }
uint32_t    bytesReceived() { return 0; }
uint32_t    bytesExpected() { return 0; }
const char* failureText()   { return "No Bluetooth on this board."; }
const char* deviceName()    { return "SquachWatch"; }
uint32_t    pairingCode()   { return 0; }
}  // namespace OtaBle

// ---- OtaWifi: no WiFi on this board (yet) ----------------------------------
namespace OtaWifi {
static State s_state = State::OFF;
bool        begin()               { s_state = State::FAILED; return false; }
bool        end()                 { s_state = State::OFF; return false; }
void        tick(uint32_t)        {}
void        rescan()              {}
State       state()               { return s_state; }
uint8_t     netCount()            { return 0; }
const Net*  net(uint8_t)          { return nullptr; }
bool        hasSaved()            { return false; }
bool        savedPassAt(uint8_t, char* out, size_t cap) { if (out && cap) out[0] = 0; return false; }
void        forget()              {}
uint8_t     savedCount()          { return 0; }
const char* savedSsidAt(uint8_t)  { return ""; }
uint8_t     savedUse()            { return 0; }
int8_t      savedIndexOf(const char*) { return -1; }
SavedResult savedResult(uint8_t)  { return SavedResult::UNTRIED; }
bool        saveNetwork(const char*, const char*) { return false; }
void        removeSaved(uint8_t)  {}
void        useSaved(uint8_t)     {}
void        connectSavedAt(uint8_t) {}
void        printSaved()          { Serial.println("[wifi] no radio on this board"); }
void        connect(const char*, const char*, bool) { s_state = State::FAILED; }
void        connectSaved()        { s_state = State::FAILED; }
bool        bootCheck(uint32_t)   { return false; }
const char* network()             { return ""; }
const char* latestVersion()       { return ""; }
bool        upToDate()            { return true; }
void        install()             {}
bool        canTryAgain()         { return false; }
void        tryAgain()            {}
uint8_t     percent()             { return 0; }
uint32_t    bytesReceived()       { return 0; }
uint32_t    bytesExpected()       { return 0; }
const char* failureText()         { return "No WiFi on this board yet: the Tab5's radio is not driven."; }
}  // namespace OtaWifi

#endif  // TAB5
