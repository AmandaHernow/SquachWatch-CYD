// SquachWatch-CYD — LORA, the T-Deck's landing page for its radio.
//
// One screen with everything the radio knows right now, no tabs: a radar of
// the nodes on the left, the four numbers (NOW, FLOOR, PEAK, MARGIN) and the
// last 80 seconds of signal on the right, as a strip and as a waterfall
// under it, both chats side by side below, and a compose line that sends to
// whichever network the switch says. Picked from the LoRa landing lab
// (2026-10-15, "Under the Dish, revised"): a nod to Cairn's Radio page, in
// Squachy's colours.
//
// The radar is honest. A node that has sent its position sits at its real
// bearing and distance (log scale, rings in miles) when the board has a fix;
// a node without one sits on the dotted ring at the rim, spaced out, because
// one antenna gives no direction. A filled dot was heard directly, a hollow
// one only through relays, and signal strength is never drawn as distance.
//
// Opened by Enter on the home screen and by the LORA pill. Built only with
// SQUACH_LORA on the T-Deck.
#pragma once
#include <TFT_eSPI.h>
#include <stdint.h>

enum class LoraHomeHit : uint8_t { NONE = 0, CHAT_MESHTASTIC, CHAT_MESHCORE, SWITCH, CHATS, BACK };

void        uiLoraHomeInit(TFT_eSPI& t);
void        uiLoraHomeTick(TFT_eSPI& t, uint32_t now);
LoraHomeHit uiLoraHomeHit(TFT_eSPI& t, int x, int y);
// The switch: which network the compose line sends to. 0 Meshtastic, 1 MeshCore.
void        uiLoraHomeToggleNet();
uint8_t     uiLoraHomeNet();
// The keyboard while the page is up: printable keys type, '\b' deletes,
// '\n' sends what is typed on the switch's network.
void        uiLoraHomeKey(char c, uint32_t now);
