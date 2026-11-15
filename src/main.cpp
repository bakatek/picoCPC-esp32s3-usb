// PicoCPC USB Bridge - ESP32-S3
//   * cle USB virtuelle (FAT16 en flash) geree depuis une page web
//   * clavier / souris USB pour la PicoCPC, alimentes par Bluetooth ou par la page web
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <WebSocketsServer.h>
#include <time.h>
#include <Preferences.h>
#include "config.h"
#include "flashdisk.h"
#include "fat16.h"
#include "usb_dev.h"
#include "ble_hid.h"
#include "web_ui.h"

#ifdef SET_LOOP_TASK_STACK_SIZE
SET_LOOP_TASK_STACK_SIZE(16 * 1024);
#endif

static WebServer server(80);
static WebSocketsServer ws(81);

static bool g_pending = false, g_remountNow = false;
static uint32_t g_changedAt = 0;
static String g_ip = "?";
static String cfgSsid, cfgPass;            // Wi-Fi memorise en flash (prioritaire sur config.h)
static bool g_sta = false;
static bool g_reboot = false;
static uint32_t g_rebootAt = 0;

static void markChanged() { g_pending = true; g_changedAt = millis(); }

// ---------------------------------------------------------------- utilitaires
static String jesc(const std::string& s) {
  String o;
  for (unsigned char c : s) {
    if (c == '"' || c == '\\') { o += '\\'; o += (char)c; }
    else if (c < 0x20) o += ' ';
    else o += (char)c;
  }
  return o;
}

static void sendJson(int code, const String& j) { server.send(code, "application/json", j); }
static void sendErr(int code, const std::string& m) { sendJson(code, "{\"ok\":false,\"error\":\"" + jesc(m) + "\"}"); }
static void sendOk() { sendJson(200, "{\"ok\":true}"); }

// ---------------------------------------------------------------- HTTP
static void hStatus() {
  uint32_t total = g_fat.totalBytes(), fr = g_fat.freeBytes();
  String j = "{\"total\":" + String(total) + ",\"free\":" + String(fr) + ",\"usb\":" + (usbMounted() ? "true" : "false") +
             ",\"pending\":" + (g_pending ? "true" : "false") + ",\"ip\":\"" + g_ip + "\"}";
  sendJson(200, j);
}

static void hFiles() {
  std::vector<FatFileInfo> l;
  if (!g_fat.list(l)) {
    // volume illisible (ex. reformate en FAT32 par l'hote) : on l'indique
    sendErr(500, g_fat.lastError + " - utilisez \"Formater\"");
    return;
  }
  String j = "{\"files\":[";
  for (size_t i = 0; i < l.size(); i++) {
    if (i) j += ',';
    j += "{\"name\":\"" + jesc(l[i].name) + "\",\"short\":\"" + jesc(l[i].shortName) + "\",\"size\":" + String(l[i].size) +
         ",\"dir\":" + (l[i].dir ? "true" : "false") + "}";
  }
  j += "]}";
  sendJson(200, j);
}

static void hDelete() {
  if (!g_fat.remove(server.arg("name").c_str())) { sendErr(400, g_fat.lastError); return; }
  markChanged(); sendOk();
}

static void hRename() {
  if (!g_fat.rename(server.arg("from").c_str(), server.arg("to").c_str())) { sendErr(400, g_fat.lastError); return; }
  markChanged(); sendOk();
}

static void hFormat() {
  if (server.arg("confirm") != "1") { sendErr(400, "confirmation manquante"); return; }
  if (!g_fat.format(VOLUME_LABEL)) { sendErr(500, g_fat.lastError); return; }
  markChanged(); sendOk();
}

static void hRemount() { g_remountNow = true; sendOk(); }

static bool upOk = true;
static std::string upErr;

static void hUploadData() {
  HTTPUpload& up = server.upload();
  if (up.status == UPLOAD_FILE_START) {
    upOk = true; upErr.clear();
    std::string n = up.filename.c_str();
    size_t p = n.find_last_of("/\\");
    if (p != std::string::npos) n = n.substr(p + 1);
    if (!g_fat.beginWrite(n)) { upOk = false; upErr = g_fat.lastError; }
  } else if (up.status == UPLOAD_FILE_WRITE) {
    if (upOk && !g_fat.writeData(up.buf, up.currentSize)) { upOk = false; upErr = g_fat.lastError; g_fat.abortWrite(); }
  } else if (up.status == UPLOAD_FILE_END) {
    if (upOk) { if (g_fat.endWrite()) markChanged(); else { upOk = false; upErr = g_fat.lastError; } }
  } else if (up.status == UPLOAD_FILE_ABORTED) {
    g_fat.abortWrite(); upOk = false; upErr = "envoi annule";
  }
}

static void hUploadDone() {
  if (upOk) sendOk(); else sendErr(500, upErr);
}

static void hDownload() {
  std::string name = server.arg("name").c_str();
  uint32_t size = 0;
  if (!g_fat.openRead(name, size)) { server.send(404, "text/plain", "introuvable"); return; }
  String safe;
  for (char c : name) safe += (c == '"' || (uint8_t)c < 0x20) ? '_' : c;
  server.sendHeader("Content-Disposition", "attachment; filename=\"" + safe + "\"");
  server.setContentLength(size);
  server.send(200, "application/octet-stream", "");
  WiFiClient cl = server.client();
  static uint8_t buf[2048];
  int n;
  while ((n = g_fat.readData(buf, sizeof buf)) > 0) {
    if (cl.write(buf, n) != (size_t)n) break;
  }
}

// ---- Wi-Fi : configuration depuis la page ----
static void hWifi() {
  String j = "{\"ssid\":\"" + jesc(cfgSsid.c_str()) + "\",\"mode\":\"" + (g_sta ? "STA" : "AP") + "\",\"ap\":\"" AP_SSID "\",\"ip\":\"" + g_ip + "\"}";
  sendJson(200, j);
}

static void hWifiScan() {
  if (WiFi.getMode() == WIFI_AP) WiFi.mode(WIFI_AP_STA);   // le scan exige le mode station
  int n = WiFi.scanNetworks();
  String j = "{\"nets\":[";
  bool first = true;
  for (int i = 0; i < n; i++) {
    String s = WiFi.SSID(i);
    if (!s.length()) continue;
    if (!first) j += ',';
    first = false;
    j += "{\"ssid\":\"" + jesc(s.c_str()) + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
  }
  j += "]}";
  WiFi.scanDelete();
  sendJson(200, j);
}

static void hWifiSave() {
  String ssid = server.arg("ssid"), pass = server.arg("pass");
  if (ssid.length() > 32 || pass.length() > 63 || (pass.length() && pass.length() < 8)) {
    sendErr(400, "SSID 32 caracteres max ; mot de passe de 8 a 63 caracteres");
    return;
  }
  Preferences p;
  p.begin("wifi", false);
  p.putString("ssid", ssid);                 // SSID vide = retour au point d'acces
  p.putString("pass", pass);
  p.end();
  sendOk();
  g_reboot = true; g_rebootAt = millis() + 800;
}

static void hBle()        { sendJson(200, bleJson()); }
static void hBleScan()    { bleScan(); sendOk(); }
static void hBleConnect() { bleConnect(server.arg("addr")); sendOk(); }
static void hBleForget()  { bleForget(server.arg("addr")); sendOk(); }

// ---------------------------------------------------------------- WebSocket (clavier / souris)
static void wsEvent(uint8_t, WStype_t type, uint8_t* p, size_t len) {
  if (type == WStype_DISCONNECTED) { usbReleaseAll(); return; }
  if (type != WStype_TEXT || len < 1) return;
  const char* s = (const char*)p;
  if (s[0] == 'k') {
    int m, k[6];
    if (sscanf(s, "k %d %d %d %d %d %d %d", &m, &k[0], &k[1], &k[2], &k[3], &k[4], &k[5]) == 7) {
      uint8_t r[8] = {(uint8_t)m, 0, (uint8_t)k[0], (uint8_t)k[1], (uint8_t)k[2], (uint8_t)k[3], (uint8_t)k[4], (uint8_t)k[5]};
      usbKeyboardReport(r);
    }
  } else if (s[0] == 'm') {
    int b, dx, dy, w;
    if (sscanf(s, "m %d %d %d %d", &b, &dx, &dy, &w) == 4) usbMouse(b & 7, dx, dy, w);
  } else if (s[0] == 'r') {
    usbReleaseAll();
  }
}

// ---------------------------------------------------------------- Wi-Fi
static void wifiSetup() {
  WiFi.setHostname(HOSTNAME);
  Preferences p;
  p.begin("wifi", false);
  cfgSsid = p.getString("ssid", WIFI_SSID);  // la flash l'emporte sur config.h
  cfgPass = p.getString("pass", WIFI_PASS);
  p.end();
  bool sta = false;
  if (cfgSsid.length()) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(cfgSsid.c_str(), cfgPass.c_str());
    for (int i = 0; i < 30 && WiFi.status() != WL_CONNECTED; i++) delay(500);
    sta = WiFi.status() == WL_CONNECTED;
  }
  g_sta = sta;
  if (sta) {
    g_ip = WiFi.localIP().toString();
    configTzTime(TZ_INFO, "pool.ntp.org", "time.google.com");
  } else {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);
    g_ip = WiFi.softAPIP().toString();
  }
  if (MDNS.begin(HOSTNAME)) MDNS.addService("http", "tcp", 80);
  Serial.printf("Wi-Fi %s - http://%s/ (ou http://%s.local/)\n", sta ? "STA" : "AP", g_ip.c_str(), HOSTNAME);
}

// ---------------------------------------------------------------- setup / loop
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\nPicoCPC USB Bridge");

  if (!g_disk.begin(DISK_PART_LABEL, CACHE_BLOCKS)) {
    Serial.println("ERREUR : partition 'disk' introuvable ou memoire insuffisante (verifiez partitions.csv / PSRAM)");
    for (;;) delay(1000);
  }
  if (!g_fat.mount()) {
    Serial.printf("Volume absent ou illisible (%s) : formatage FAT16\n", g_fat.lastError.c_str());
    if (!g_fat.format(VOLUME_LABEL)) Serial.printf("Formatage impossible : %s\n", g_fat.lastError.c_str());
  }
  Serial.printf("Disque : %u Ko, libre %u Ko\n", g_fat.totalBytes() / 1024, g_fat.freeBytes() / 1024);

  usbSetup();
  wifiSetup();
  bleBegin();

  server.on("/", HTTP_GET, [] { server.send_P(200, "text/html; charset=utf-8", INDEX_HTML); });
  server.on("/api/status", HTTP_GET, hStatus);
  server.on("/api/files", HTTP_GET, hFiles);
  server.on("/api/delete", HTTP_POST, hDelete);
  server.on("/api/rename", HTTP_POST, hRename);
  server.on("/api/format", HTTP_POST, hFormat);
  server.on("/api/remount", HTTP_POST, hRemount);
  server.on("/api/upload", HTTP_POST, hUploadDone, hUploadData);
  server.on("/download", HTTP_GET, hDownload);
  server.on("/api/wifi", HTTP_GET, hWifi);
  server.on("/api/wifi/scan", HTTP_GET, hWifiScan);
  server.on("/api/wifi/save", HTTP_POST, hWifiSave);
  server.on("/api/ble", HTTP_GET, hBle);
  server.on("/api/ble/scan", HTTP_POST, hBleScan);
  server.on("/api/ble/connect", HTTP_POST, hBleConnect);
  server.on("/api/ble/forget", HTTP_POST, hBleForget);
  server.onNotFound([] { server.send(404, "text/plain", "404"); });
  server.begin();

  ws.begin();
  ws.onEvent(wsEvent);
}

void loop() {
  server.handleClient();
  ws.loop();

  InputEvt e;
  for (int n = 0; n < 16 && blePop(e); n++) {
    if (e.type == 1) usbKeyboardReport(e.d);
    else usbMouse(e.d[0] & 7, (int8_t)e.d[1], (int8_t)e.d[2], (int8_t)e.d[3]);
  }

  g_disk.flushIfIdle(FLUSH_IDLE_MS);

  if (g_reboot && millis() > g_rebootAt) { g_disk.flush(); ESP.restart(); }

  bool due = AUTO_REMOUNT && g_pending && (millis() - g_changedAt > REMOUNT_DELAY_MS);
  if ((g_remountNow || due) && !g_fat.writing()) {
    g_remountNow = false; g_pending = false;
    usbRemount();
  }
  delay(1);
}
