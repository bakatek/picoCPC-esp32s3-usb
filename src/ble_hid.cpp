// Client Bluetooth LE "HID over GATT" : se connecte a un clavier / une souris BLE
// et transmet leurs rapports vers l'USB (via une file lue par loop()).
// Utilise NimBLE-Arduino 1.4.x. Mode "boot protocol" privilegie (rapports fixes,
// pas besoin d'analyser le descripteur HID).
#include "ble_hid.h"
#include "config.h"
#include <NimBLEDevice.h>
#include <vector>

struct Found {
  NimBLEAddress a;
  String s, name;
  int rssi = 0;
  bool hid = false, bonded = false;
  Found(const NimBLEAddress& x) : a(x) {}
};
struct Conn { NimBLEClient* cl; String s, name, kind; bool alive; };
struct Cmd { uint8_t t; char a[24]; };

static SemaphoreHandle_t mtx;
static QueueHandle_t inQ, cmdQ;
static std::vector<Found> found;
static std::vector<Conn> conns;
static String status = "Prêt";
static bool scanning = false;

struct Lock { Lock() { xSemaphoreTake(mtx, portMAX_DELAY); } ~Lock() { xSemaphoreGive(mtx); } };

static String esc(const String& s) {
  String o;
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '"' || c == '\\') { o += '\\'; o += c; }
    else if ((uint8_t)c < 0x20) o += ' ';
    else o += c;
  }
  return o;
}
static void setStatus(const String& s) { Lock l; status = s; }

// ---------------------------------------------------------------- rapports -> file
static void onBootKbd(NimBLERemoteCharacteristic*, uint8_t* d, size_t n, bool) {
  InputEvt e{1, {0}};
  memcpy(e.d, d, n < 8 ? n : 8);
  xQueueSend(inQ, &e, 0);
}
static void onBootMouse(NimBLERemoteCharacteristic*, uint8_t* d, size_t n, bool) {
  if (n < 3) return;
  InputEvt e{2, {0}};
  e.d[0] = d[0]; e.d[1] = d[1]; e.d[2] = d[2]; e.d[3] = n > 3 ? d[3] : 0;
  xQueueSend(inQ, &e, 0);
}
// Repli (peripherique sans boot protocol) : on devine d'apres la taille du rapport.
static void onReport(NimBLERemoteCharacteristic* c, uint8_t* d, size_t n, bool b) {
  if (n == 8) onBootKbd(c, d, n, b);
  else if (n >= 3 && n <= 5) onBootMouse(c, d, n, b);
}

static void pushRelease() {
  InputEvt k{1, {0}}, m{2, {0}};
  xQueueSend(inQ, &k, 0);
  xQueueSend(inQ, &m, 0);
}

// ---------------------------------------------------------------- callbacks NimBLE
class CCb : public NimBLEClientCallbacks {
public:
  void onConnect(NimBLEClient*) {}
  void onDisconnect(NimBLEClient* c) {
    {
      Lock l;
      for (auto& x : conns) if (x.cl == c) { x.alive = false; status = "Déconnecté : " + x.name; }
    }
    pushRelease();
  }
};
static CCb ccb;

class AdvCb : public NimBLEAdvertisedDeviceCallbacks {
public:
  void onResult(NimBLEAdvertisedDevice* d) {
    String s = d->getAddress().toString().c_str();
    bool hid = d->isAdvertisingService(NimBLEUUID((uint16_t)0x1812));
    String name = d->getName().c_str();
    if (!hid && name.length() == 0) return;
    Lock l;
    for (auto& f : found) if (f.s == s) { f.rssi = d->getRSSI(); if (name.length()) f.name = name; f.hid |= hid; return; }
    if (found.size() >= 30) return;
    Found f(d->getAddress());
    f.s = s; f.name = name; f.rssi = d->getRSSI(); f.hid = hid;
    found.push_back(f);
  }
};
static AdvCb advCb;

// ---------------------------------------------------------------- operations (tache BLE)
static void doScan(uint32_t secs) {
  { Lock l; scanning = true; found.clear(); status = "Recherche en cours…"; }
  NimBLEScan* sc = NimBLEDevice::getScan();
  sc->setAdvertisedDeviceCallbacks(&advCb);
  sc->setActiveScan(true);
  sc->setInterval(97);
  sc->setWindow(37);
  sc->clearResults();
  sc->start(secs);
  sc->clearResults();
  Lock l;
  for (auto& f : found) f.bonded = NimBLEDevice::isBonded(f.a);
  scanning = false;
  status = "Recherche terminée (" + String((int)found.size()) + " appareil(s))";
}

static bool isConnected(const String& s) {
  Lock l;
  for (auto& c : conns) if (c.alive && c.s == s) return true;
  return false;
}

static void connectDev(const String& s) {
  if (isConnected(s)) return;
  NimBLEAddress* ap = nullptr;
  String name;
  {
    Lock l;
    for (auto& f : found) if (f.s == s) { ap = new NimBLEAddress(f.a); name = f.name; }
  }
  if (!ap) { setStatus("Appareil inconnu : relancez une recherche"); return; }
  if (!name.length()) name = s;
  setStatus("Connexion à " + name + "…");

  NimBLEClient* c = NimBLEDevice::createClient();
  c->setClientCallbacks(&ccb, false);
  c->setConnectionParams(12, 12, 0, 51);
  c->setConnectTimeout(10);
  bool ok = c->connect(*ap);
  delete ap;
  if (!ok) { NimBLEDevice::deleteClient(c); setStatus("Échec de connexion à " + name); return; }

  setStatus("Appairage de " + name + " (code " + String(BLE_PASSKEY) + " si demandé)…");
  c->secureConnection();                       // sans effet si deja chiffre ; erreur ignoree

  NimBLERemoteService* hid = c->getService(NimBLEUUID((uint16_t)0x1812));
  if (!hid) { c->disconnect(); NimBLEDevice::deleteClient(c); setStatus(name + " n'est pas un périphérique HID"); return; }

  NimBLERemoteCharacteristic *bootK = nullptr, *bootM = nullptr, *proto = nullptr;
  std::vector<NimBLERemoteCharacteristic*> reports;
  auto* chars = hid->getCharacteristics(true);
  if (chars) {
    for (auto* ch : *chars) {
      NimBLEUUID u = ch->getUUID();
      if (u == NimBLEUUID((uint16_t)0x2A22)) bootK = ch;
      else if (u == NimBLEUUID((uint16_t)0x2A33)) bootM = ch;
      else if (u == NimBLEUUID((uint16_t)0x2A4E)) proto = ch;
      else if (u == NimBLEUUID((uint16_t)0x2A4D) && ch->canNotify()) reports.push_back(ch);
    }
  }

  String kind;
  if (bootK || bootM) {
    if (proto) { uint8_t z = 0; proto->writeValue(&z, 1, false); }     // Protocol Mode = Boot
    if (bootK && bootK->canNotify() && bootK->subscribe(true, onBootKbd)) kind += "clavier";
    if (bootM && bootM->canNotify() && bootM->subscribe(true, onBootMouse)) kind += (kind.length() ? " + souris" : "souris");
  }
  if (!kind.length()) {                                   // repli : caracteristiques "Report"
    int ok2 = 0;
    for (auto* r : reports) if (r->subscribe(true, onReport)) ok2++;
    if (ok2) kind = "HID (rapports, best effort)";
  }
  if (!kind.length()) { c->disconnect(); NimBLEDevice::deleteClient(c); setStatus("Aucun rapport HID utilisable sur " + name); return; }

  Lock l;
  conns.push_back({c, s, name, kind, true});
  status = "Connecté : " + name + " (" + kind + ")";
}

static void forgetDev(const String& s) {
  // deconnexion eventuelle
  NimBLEClient* cl = nullptr;
  { Lock l; for (auto& c : conns) if (c.s == s && c.alive) cl = c.cl; }
  if (cl) cl->disconnect();
  bool done = false;
  { Lock l; for (auto& f : found) if (f.s == s) { NimBLEDevice::deleteBond(f.a); f.bonded = false; done = true; } }
  if (!done) {
    int n = NimBLEDevice::getNumBonds();
    for (int i = 0; i < n; i++) {
      NimBLEAddress b = NimBLEDevice::getBondedAddress(i);
      if (String(b.toString().c_str()) == s) { NimBLEDevice::deleteBond(b); done = true; break; }
    }
  }
  setStatus(done ? "Appairage supprimé" : "Appareil inconnu");
}

static void autoReconnect() {
  if (NimBLEDevice::getNumBonds() == 0) return;
  { Lock l; int alive = 0; for (auto& c : conns) if (c.alive) alive++; if (alive >= 2 || alive >= (int)NimBLEDevice::getNumBonds()) return; }   // tout le monde est la : pas de scan (il gene le Wi-Fi)
  doScan(4);
  std::vector<String> todo;
  { Lock l; for (auto& f : found) if (f.bonded) todo.push_back(f.s); }
  for (auto& s : todo) connectDev(s);
}

static void purgeDead() {
  std::vector<NimBLEClient*> dead;
  {
    Lock l;
    for (size_t i = 0; i < conns.size();) {
      if (!conns[i].alive) { dead.push_back(conns[i].cl); conns.erase(conns.begin() + i); }
      else i++;
    }
  }
  for (auto* c : dead) NimBLEDevice::deleteClient(c);
}

static void bleTask(void*) {
  NimBLEDevice::init("PicoCPC-ESP32");
  NimBLEDevice::setSecurityAuth(true, false, true);        // bonding, sans MITM, Secure Connections
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY);  // le code s'affiche sur la page web
  NimBLEDevice::setSecurityPasskey(BLE_PASSKEY);
  NimBLEDevice::setPower(ESP_PWR_LVL_P6);
  setStatus("Prêt");

  uint32_t lastAuto = millis();
  for (;;) {
    Cmd c;
    if (xQueueReceive(cmdQ, &c, pdMS_TO_TICKS(1000)) == pdTRUE) {
      String a = c.a;
      switch (c.t) {
        case 1: doScan(5); break;
        case 2: connectDev(a); break;
        case 3: forgetDev(a); break;
      }
      lastAuto = millis();
    } else {
      purgeDead();
      if (millis() - lastAuto > 20000) { lastAuto = millis(); autoReconnect(); }
    }
  }
}

// ---------------------------------------------------------------- API
void bleBegin() {
  mtx = xSemaphoreCreateMutex();
  inQ = xQueueCreate(64, sizeof(InputEvt));
  cmdQ = xQueueCreate(8, sizeof(Cmd));
  xTaskCreatePinnedToCore(bleTask, "ble", 10240, nullptr, 1, nullptr, 0);
}

static void post(uint8_t t, const String& a) {
  Cmd c{t, {0}};
  strncpy(c.a, a.c_str(), sizeof(c.a) - 1);
  xQueueSend(cmdQ, &c, 0);
}
void bleScan() { post(1, ""); }
void bleConnect(const String& addr) { post(2, addr); }
void bleForget(const String& addr) { post(3, addr); }

bool blePop(InputEvt& e) { return inQ && xQueueReceive(inQ, &e, 0) == pdTRUE; }

String bleJson() {
  Lock l;
  String j = "{\"status\":\"" + esc(status) + "\",\"scanning\":" + (scanning ? "true" : "false") +
             ",\"pin\":" + String(BLE_PASSKEY) + ",\"devices\":[";
  bool first = true;
  for (auto& c : conns) {
    if (!c.alive) continue;
    if (!first) j += ',';
    first = false;
    j += "{\"addr\":\"" + c.s + "\",\"name\":\"" + esc(c.name) + "\",\"connected\":true,\"bonded\":true,\"hid\":true,\"kind\":\"" + esc(c.kind) + "\"}";
  }
  for (auto& f : found) {
    bool conn = false;
    for (auto& c : conns) if (c.alive && c.s == f.s) conn = true;
    if (conn) continue;
    if (!first) j += ',';
    first = false;
    j += "{\"addr\":\"" + f.s + "\",\"name\":\"" + esc(f.name) + "\",\"rssi\":" + String(f.rssi) +
         ",\"connected\":false,\"bonded\":" + (f.bonded ? "true" : "false") + ",\"hid\":" + (f.hid ? "true" : "false") + "}";
  }
  j += "]}";
  return j;
}
