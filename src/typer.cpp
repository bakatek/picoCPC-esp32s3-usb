#include "typer.h"
#include "usb_dev.h"
#include <Arduino.h>

static const int CAP = 6000;
static uint16_t buf[CAP];
static int head = 0, tail = 0;
static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
static int tHold = 30, tGap = 30, tEol = 150;
static volatile bool abortReq = false;
static int phase = 0;              // 0 pret, 1 modif. seul envoye, 2 touche envoyee, 3 touche relachee, 4 tout relache
static uint32_t nextAt = 0;
static uint8_t cm = 0, ck = 0;

static int count() { return (tail - head + CAP) % CAP; }

void typerSetTiming(int h, int g, int e) {
  tHold = constrain(h, 10, 500); tGap = constrain(g, 5, 500); tEol = constrain(e, 20, 5000);
}

size_t typerPush(const uint8_t* d, size_t n) {
  size_t ok = 0;
  for (size_t i = 0; i + 1 < n; i += 2) {
    portENTER_CRITICAL(&mux);
    bool full = count() >= CAP - 1;
    if (!full) { buf[tail] = (uint16_t)(d[i] << 8 | d[i + 1]); tail = (tail + 1) % CAP; }
    portEXIT_CRITICAL(&mux);
    if (full) break;
    ok++;
  }
  return ok;
}

static void rep(uint8_t m, uint8_t k) {
  uint8_t r[8] = {m, 0, k, 0, 0, 0, 0, 0};
  usbKeyboardReport(r);
}

void typerAbort() { abortReq = true; }
bool typerActive() { return count() > 0 || phase != 0; }
int typerRemaining() { return count(); }

void typerRun() {
  if (abortReq) {
    abortReq = false;
    portENTER_CRITICAL(&mux); head = tail = 0; portEXIT_CRITICAL(&mux);
    phase = 0; rep(0, 0);
    return;
  }
  uint32_t now = millis();
  if ((int32_t)(now - nextAt) < 0) return;
  switch (phase) {
    case 0: {
      if (head == tail) return;
      uint16_t s;
      portENTER_CRITICAL(&mux); s = buf[head]; head = (head + 1) % CAP; portEXIT_CRITICAL(&mux);
      cm = s >> 8; ck = s & 0xFF;
      if (cm == 0xFF) { nextAt = now + (uint32_t)ck * 50; return; }       // pause
      if (cm) { rep(cm, 0); phase = 1; nextAt = now + tHold / 2; }          // modificateur d'abord (Maj, AltGr)
      else { rep(0, ck); phase = 2; nextAt = now + tHold; }
      break;
    }
    case 1: rep(cm, ck); phase = 2; nextAt = now + tHold; break;
    case 2:
      if (cm) { rep(cm, 0); phase = 3; nextAt = now + tHold / 2; }
      else { rep(0, 0); phase = 4; nextAt = now + (ck == 0x28 ? tEol : tGap); }
      break;
    case 3: rep(0, 0); phase = 4; nextAt = now + (ck == 0x28 ? tEol : tGap); break;
    case 4: phase = 0; break;
  }
}
