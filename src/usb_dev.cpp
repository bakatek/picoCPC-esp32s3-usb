#include "usb_dev.h"
#include "config.h"
#include "flashdisk.h"
#include "dbglog.h"
#include <Arduino.h>
#include <USB.h>
#include <USBMSC.h>
#include <Preferences.h>
#include "esp32-hal-tinyusb.h"

#include <tusb.h>
#define HAVE_TUSB 1

static USBMSC msc;
static uint8_t g_btn = 0;

// ---------------------------------------------------------------- HID sur mesure
// Le HID du coeur Arduino (USBHID) declare une interface de protocole "aucun", avec un point
// de terminaison OUT et des paquets de 64 octets. Le firmware de la PicoCPC classe les
// peripheriques par protocole boot (1 = clavier, 2 = souris) : on declare donc nous-memes une
// interface boot standard, endpoint IN seul, paquets de 8 octets, SANS report ID.
// (Le coeur n'offre qu'UNE interface HID : mode clavier OU mode souris, au choix, voir /api/hidmode.)
static const uint8_t kbdDesc[] = {
  0x05,0x01,0x09,0x06,0xA1,0x01,0x05,0x07,0x19,0xE0,0x29,0xE7,0x15,0x00,0x25,0x01,0x75,0x01,0x95,0x08,0x81,0x02,
  0x95,0x01,0x75,0x08,0x81,0x01,0x95,0x05,0x75,0x01,0x05,0x08,0x19,0x01,0x29,0x05,0x91,0x02,0x95,0x01,0x75,0x03,0x91,0x01,
  0x95,0x06,0x75,0x08,0x15,0x00,0x25,0x65,0x05,0x07,0x19,0x00,0x29,0x65,0x81,0x00,0xC0};
static const uint8_t mouseDesc[] = {
  0x05,0x01,0x09,0x02,0xA1,0x01,0x09,0x01,0xA1,0x00,0x05,0x09,0x19,0x01,0x29,0x03,0x15,0x00,0x25,0x01,0x95,0x03,0x75,0x01,0x81,0x02,
  0x95,0x01,0x75,0x05,0x81,0x03,0x05,0x01,0x09,0x30,0x09,0x31,0x09,0x38,0x15,0x81,0x25,0x7F,0x75,0x08,0x95,0x03,0x81,0x06,0xC0,0xC0};

static uint8_t g_hidMode = 1;                 // 1 = clavier boot, 2 = souris boot

int usbHidMode() { return g_hidMode; }
void usbSetHidMode(int m) {
  Preferences p; p.begin("usb", false); p.putUChar("hid", m == 2 ? 2 : 1); p.end();
}

static uint16_t hidLoadDescriptor(uint8_t* dst, uint8_t* itf) {
  uint8_t str = tinyusb_add_string_descriptor("PicoCPC HID");
  uint8_t epIn = tinyusb_get_free_in_endpoint();
  TU_VERIFY(epIn != 0, 0);
  uint8_t d[TUD_HID_DESC_LEN] = {
    TUD_HID_DESCRIPTOR(*itf, str, g_hidMode == 2 ? HID_ITF_PROTOCOL_MOUSE : HID_ITF_PROTOCOL_KEYBOARD,
                       g_hidMode == 2 ? sizeof(mouseDesc) : sizeof(kbdDesc), (uint8_t)(0x80 | epIn), 8, 10)};
  *itf += 1;
  memcpy(dst, d, TUD_HID_DESC_LEN);
  return TUD_HID_DESC_LEN;
}

extern "C" {
uint8_t const* tud_hid_descriptor_report_cb(uint8_t instance) {
  (void)instance;
  return g_hidMode == 2 ? mouseDesc : kbdDesc;
}
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
  (void)instance; (void)report_id; (void)report_type; (void)buffer; (void)reqlen;
  return 0;
}
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
  (void)instance; (void)report_id; (void)report_type; (void)buffer; (void)bufsize;   // LED clavier : ignorees
}
}

// ---------------------------------------------------------------- MSC
// Journal limite : les 40 premieres operations, les erreurs, puis 1 sur 256 (pas d'echo Serial).
static uint32_t nRead = 0, nWrite = 0;

static int32_t mscRead(uint32_t lba, uint32_t offset, void* buf, uint32_t size) {
  bool ok = g_disk.read(lba * 512u + offset, buf, size);
  nRead++;
  if (!ok || nRead <= 40 || (nRead & 255) == 0)
    dbgq("MSC lecture #%u lba=%u off=%u len=%u %s", (unsigned)nRead, (unsigned)lba, (unsigned)offset, (unsigned)size, ok ? "ok" : "ERREUR");
  return ok ? (int32_t)size : -1;
}

static int32_t mscWrite(uint32_t lba, uint32_t offset, uint8_t* buf, uint32_t size) {
  bool ok = g_disk.write(lba * 512u + offset, buf, size);
  nWrite++;
  if (!ok || nWrite <= 40 || (nWrite & 255) == 0)
    dbgq("MSC ecriture #%u lba=%u off=%u len=%u %s", (unsigned)nWrite, (unsigned)lba, (unsigned)offset, (unsigned)size, ok ? "ok" : "ERREUR");
  return ok ? (int32_t)size : -1;
}

static bool mscStartStop(uint8_t power_condition, bool start, bool load_eject) {
  dbgq("MSC start/stop pc=%u start=%d eject=%d", power_condition, start, load_eject);
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

#if ENABLE_HID
  { Preferences p; p.begin("usb", true); g_hidMode = p.getUChar("hid", 1) == 2 ? 2 : 1; p.end(); }
  tinyusb_enable_interface(USB_INTERFACE_HID, TUD_HID_DESC_LEN, hidLoadDescriptor);
#endif

  USB.manufacturerName("ESP32-S3");
  USB.productName("PicoCPC USB Bridge");
  USB.begin();
  dbg("USB initialise (HID %s, %u secteurs)", ENABLE_HID ? (g_hidMode == 2 ? "souris boot" : "clavier boot") : "inactif", (unsigned)g_disk.sectorCount());
}

bool usbMounted() {
#ifdef HAVE_TUSB
  return tud_mounted();
#else
  return true;
#endif
}

void usbRemount() {
  dbg("Remontage : detachement USB");
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

// ---------------------------------------------------------------- HID : envoi
static bool hidSend(const uint8_t* d, uint8_t n) {
  for (int i = 0; i < 20 && !tud_hid_n_ready(0); i++) delay(1);
  if (!tud_hid_n_ready(0)) return false;
  return tud_hid_n_report(0, 0, d, n);        // report ID 0 : format boot brut
}

void usbKeyboardReport(const uint8_t r[8]) {
#if ENABLE_HID
  if (g_hidMode != 1 || !usbMounted()) return;
  hidSend(r, 8);
#endif
}

void usbMouse(uint8_t buttons, int dx, int dy, int wheel) {
#if ENABLE_HID
  if (g_hidMode != 2 || !usbMounted()) return;
  g_btn = buttons & 7;
  bool first = true;
  while (first || dx || dy || wheel) {
    first = false;
    int sx = constrain(dx, -127, 127), sy = constrain(dy, -127, 127), sw = constrain(wheel, -127, 127);
    uint8_t rep[4] = {g_btn, (uint8_t)(int8_t)sx, (uint8_t)(int8_t)sy, (uint8_t)(int8_t)sw};
    hidSend(rep, 4);
    dx -= sx; dy -= sy; wheel -= sw;
  }
#endif
}

void usbReleaseAll() {
  uint8_t z[8] = {0};
  usbKeyboardReport(z);
  usbMouse(0, 0, 0, 0);
}
