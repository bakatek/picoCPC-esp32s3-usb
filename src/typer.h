#pragma once
#include <stdint.h>
#include <stddef.h>

// Saisie automatique de texte : le navigateur envoie des "pas" (modificateurs, code HID) ;
// l'ESP32 les rejoue avec un minutage regulier (independant des aleas du Wi-Fi).
//   pas = 2 octets : [modificateurs, code]   ;   modificateurs 0xFF = pause de code x 50 ms
void typerSetTiming(int holdMs, int gapMs, int eolMs);
size_t typerPush(const uint8_t* steps, size_t nBytes);   // retourne le nombre de pas acceptes
void typerRun();                                         // a appeler souvent
void typerAbort();
bool typerActive();
int  typerRemaining();
