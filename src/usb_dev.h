#pragma once
#include <stdint.h>

// Peripherique USB composite : cle USB (MSC) + clavier + souris (HID, 1 seule interface)
void usbSetup();
bool usbMounted();                 // vrai si la PicoCPC a enumere l'ESP32
void usbRemount();                 // detache puis rattache le bus (la PicoCPC relit la cle)

// rapport clavier boot : [modificateurs, reserve, k1..k6]
void usbKeyboardReport(const uint8_t r[8]);
// souris : etat des boutons (b0 gauche, b1 droit, b2 milieu) + deplacements relatifs
void usbMouse(uint8_t buttons, int dx, int dy, int wheel);
void usbReleaseAll();

// mode HID (une seule interface boot possible) : 1 = clavier, 2 = souris ; change apres redemarrage
int usbHidMode();
void usbSetHidMode(int m);
