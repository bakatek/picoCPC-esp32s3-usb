#pragma once
#include <Arduino.h>

// Evenement d'entree venant d'un clavier/souris Bluetooth (traite dans loop())
struct InputEvt {
  uint8_t type;      // 1 = clavier (d[0..7] = rapport boot), 2 = souris (d[0]=boutons, d[1]=dx, d[2]=dy, d[3]=molette)
  uint8_t d[8];
};

void bleBegin();
void bleScan();
void bleConnect(const String& addr);
void bleForget(const String& addr);
String bleJson();                       // etat pour la page web
bool blePop(InputEvt& e);               // vrai si un evenement etait en attente
