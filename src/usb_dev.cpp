#include "usb_dev.h"
#include "config.h"
#include "flashdisk.h"
#include <Arduino.h>
#include <USB.h>
#include <USBMSC.h>
#include <USBHID.h>
#include <USBHIDKeyboard.h>
#include <USBHIDMouse.h>

#if __has_include(<tusb.h>)
  #include <tusb.h>
  #define HAVE_TUSB 1
#endif

static USBMSC msc;
static USBHIDKeyboard keyboard;
static USBHIDMouse mouse;
static uint8_t g_btn = 0;

// ---------------------------------------------------------------- MSC
static int32_t mscRead(uint32_t lba, uint32_t offset, void* buf, uint32_t size) {
  return g_disk.read(lba * 512u + offset, buf, size) ? (int32_t)size : -1;
}

static int32_t mscWrite(uint32_t lba, uint32_t offset, uint8_t* buf, uint32_t size) {
  return g_disk.write(lba * 512u + offset, buf, size) ? (int32_t)size : -1;
}

static bool mscStartStop(uint8_t power_condition, bool start, bool load_eject) {
  if (load_eject && !start) g_disk.flush();     // ejection demandee par l'hote
  return true;
}

// ---------------------------------------------------------------- setup
void usbSetup() {
  msc.vendorID("PicoCPC");
  msc.productID("USB Bridge");
  msc.productRevision("1.0");
  msc.onRead(mscRead);
  msc.onWrite(mscWrite);
  msc.onStartStop(mscStartStop);
  msc.mediaPresent(true);
  msc.begin(g_disk.sectorCount(), 512);

  keyboard.begin();
  mouse.begin();

  USB.manufacturerName("ESP32-S3");
  USB.productName("PicoCPC USB Bridge");
  USB.begin();
}

bool usbMounted() {
#ifdef HAVE_TUSB
  return tud_mounted();
#else
  return true;
#endif
}

void usbRemount() {
  g_disk.flush();
#ifdef HAVE_TUSB
  tud_disconnect();
  delay(DETACH_MS);
  tud_connect();
#else
  msc.mediaPresent(false);
  delay(DETACH_MS);
  msc.mediaPresent(true);
#endif
  g_btn = 0;
}

// ---------------------------------------------------------------- HID
void usbKeyboardReport(const uint8_t r[8]) {
  if (!usbMounted()) return;
  KeyReport kr;
  kr.modifiers = r[0];
  kr.reserved = 0;
  memcpy(kr.keys, r + 2, 6);
  keyboard.sendReport(&kr);
}

void usbMouse(uint8_t buttons, int dx, int dy, int wheel) {
  if (!usbMounted()) return;
  static const uint8_t bits[3] = {MOUSE_LEFT, MOUSE_RIGHT, MOUSE_MIDDLE};
  for (int i = 0; i < 3; i++) {
    uint8_t m = 1 << i;
    if ((buttons & m) && !(g_btn & m)) mouse.press(bits[i]);
    if (!(buttons & m) && (g_btn & m)) mouse.release(bits[i]);
  }
  g_btn = buttons & 7;
  while (dx || dy || wheel) {
    int sx = constrain(dx, -127, 127), sy = constrain(dy, -127, 127), sw = constrain(wheel, -127, 127);
    mouse.move(sx, sy, sw);
    dx -= sx; dy -= sy; wheel -= sw;
  }
}

void usbReleaseAll() {
  uint8_t z[8] = {0};
  usbKeyboardReport(z);
  usbMouse(0, 0, 0, 0);
}
