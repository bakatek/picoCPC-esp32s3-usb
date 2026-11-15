#include "fat16.h"
#include "flashdisk.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <algorithm>

Fat16 g_fat;

struct DiskLock { DiskLock() { g_disk.lock(); } ~DiskLock() { g_disk.unlock(); } };

static inline uint16_t rd16(const uint8_t* p) { return p[0] | (p[1] << 8); }
static inline uint32_t rd32(const uint8_t* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24); }
static inline void wr16(uint8_t* p, uint16_t v) { p[0] = v; p[1] = v >> 8; }
static inline void wr32(uint8_t* p, uint32_t v) { p[0] = v; p[1] = v >> 8; p[2] = v >> 16; p[3] = v >> 24; }

// ---------------------------------------------------------------- utilitaires noms
static bool ieq(const std::string& a, const std::string& b) {
  if (a.size() != b.size()) return false;
  for (size_t i = 0; i < a.size(); i++)
    if (tolower((unsigned char)a[i]) != tolower((unsigned char)b[i])) return false;
  return true;
}

static void utf8to16(const std::string& s, std::vector<uint16_t>& o) {
  for (size_t i = 0; i < s.size();) {
    uint8_t c = s[i]; uint32_t cp = '_'; size_t n = 1;
    if (c < 0x80) { cp = c; }
    else if ((c & 0xE0) == 0xC0 && i + 1 < s.size()) { cp = ((c & 0x1F) << 6) | (s[i + 1] & 0x3F); n = 2; }
    else if ((c & 0xF0) == 0xE0 && i + 2 < s.size()) { cp = ((c & 0x0F) << 12) | ((s[i + 1] & 0x3F) << 6) | (s[i + 2] & 0x3F); n = 3; }
    else if ((c & 0xF8) == 0xF0 && i + 3 < s.size()) { n = 4; }
    o.push_back(cp > 0xFFFF ? '_' : (uint16_t)cp);
    i += n;
  }
}

static void utf16to8(const uint16_t* u, int n, std::string& o) {
  for (int i = 0; i < n; i++) {
    uint16_t c = u[i];
    if (c == 0 || c == 0xFFFF) break;
    if (c < 0x80) o += (char)c;
    else if (c < 0x800) { o += (char)(0xC0 | (c >> 6)); o += (char)(0x80 | (c & 0x3F)); }
    else { o += (char)(0xE0 | (c >> 12)); o += (char)(0x80 | ((c >> 6) & 0x3F)); o += (char)(0x80 | (c & 0x3F)); }
  }
}

static bool validLongName(const std::string& n) {
  if (n.empty() || n.size() > 120) return false;
  for (unsigned char c : n) if (c < 0x20 || strchr("\\/:*?\"<>|", c)) return false;
  if (n == "." || n == "..") return false;
  if (n.back() == '.' || n.back() == ' ' || n[0] == ' ') return false;
  return true;
}

static bool sfnChar(unsigned char c) {
  return c && c < 128 && (isalnum(c) || strchr("!#$%&'()-@^_`{}~", c));
}

// nom 8.3 strict (insensible a la casse) -> remplit sfn[11]
static bool fit83(const std::string& n, uint8_t sfn[11]) {
  size_t dot = n.rfind('.');
  std::string b = (dot == std::string::npos) ? n : n.substr(0, dot);
  std::string e = (dot == std::string::npos) ? "" : n.substr(dot + 1);
  if (b.empty() || b.size() > 8 || e.size() > 3) return false;
  if (b.find('.') != std::string::npos) return false;
  for (unsigned char c : b) if (!sfnChar(c)) return false;
  for (unsigned char c : e) if (!sfnChar(c)) return false;
  memset(sfn, ' ', 11);
  for (size_t i = 0; i < b.size(); i++) sfn[i] = toupper((unsigned char)b[i]);
  for (size_t i = 0; i < e.size(); i++) sfn[8 + i] = toupper((unsigned char)e[i]);
  return true;
}

static uint8_t lfnChecksum(const uint8_t* s) {
  uint8_t sum = 0;
  for (int i = 0; i < 11; i++) sum = ((sum & 1) ? 0x80 : 0) + (sum >> 1) + s[i];
  return sum;
}

static void nowFat(uint16_t& date, uint16_t& tim) {
  time_t t = time(nullptr);
  struct tm tm;
  if (t > 1704067200) localtime_r(&t, &tm);       // apres le 01/01/2024 : l'heure est valide
  else { memset(&tm, 0, sizeof tm); tm.tm_year = 126; tm.tm_mon = 0; tm.tm_mday = 1; }
  date = ((tm.tm_year + 1900 - 1980) << 9) | ((tm.tm_mon + 1) << 5) | tm.tm_mday;
  tim = (tm.tm_hour << 11) | (tm.tm_min << 5) | (tm.tm_sec / 2);
}

// ---------------------------------------------------------------- montage / format
bool Fat16::mount() {
  _ok = false;
  uint8_t s[512];
  if (!g_disk.read(0, s, 512)) return fail("lecture du secteur 0 impossible");
  if (s[510] != 0x55 || s[511] != 0xAA) return fail("pas de signature FAT (disque non formate ?)");
  uint32_t bps = rd16(s + 11), spc = s[13], rsvd = rd16(s + 14), nf = s[16], rootEnt = rd16(s + 17);
  uint32_t tot = rd16(s + 19); if (!tot) tot = rd32(s + 32);
  uint32_t spf = rd16(s + 22);
  if (bps != 512 || !spc || (spc & (spc - 1)) || !nf || nf > 2 || !rootEnt || !spf || !rsvd)
    return fail("volume non FAT16 (reformatez-le)");
  uint32_t rootSecs = (rootEnt * 32 + 511) / 512;
  uint32_t dataStart = rsvd + nf * spf + rootSecs;
  if (tot > g_disk.sectorCount() || tot <= dataStart) return fail("geometrie FAT incoherente");
  uint32_t clusters = (tot - dataStart) / spc;
  if (clusters < 4085 || clusters >= 65525) return fail("volume non FAT16 (reformatez-le)");
  _spc = spc; _rsvd = rsvd; _nfats = nf; _rootEnt = rootEnt; _spf = spf; _total = tot;
  _fatStart = rsvd; _rootStart = rsvd + nf * spf; _dataStart = dataStart; _clusters = clusters;
  if (_hint < 2 || _hint >= clusters + 2) _hint = 2;
  _ok = true;
  return true;
}

bool Fat16::format(const char* label) {
  DiskLock lk;
  if (_w.active) abortWrite();
  _r.active = false;
  uint32_t total = g_disk.sectorCount();
  if (total > 65535) total = 65535;                // champ 16 bits suffisant ici (<32 Mo)
  const uint32_t rsvd = 1, nfats = 2, rootEnt = 512, rootSecs = 32;
  uint32_t spc = 4, spf = 1, clusters = 0;
  for (; spc >= 1; spc >>= 1) {
    spf = 1;
    for (;;) {
      uint32_t dataSecs = total - rsvd - rootSecs - nfats * spf;
      clusters = dataSecs / spc;
      uint32_t need = ((clusters + 2) * 2 + 511) / 512;
      if (need <= spf) break;
      spf = need;
    }
    if (clusters >= 4100) break;
  }
  if (clusters < 4085) return fail("partition trop petite pour FAT16");

  uint8_t z[512]; memset(z, 0, sizeof z);
  uint8_t s[512]; memset(s, 0, sizeof s);
  s[0] = 0xEB; s[1] = 0x3C; s[2] = 0x90;
  memcpy(s + 3, "MSDOS5.0", 8);
  wr16(s + 11, 512); s[13] = spc; wr16(s + 14, rsvd); s[16] = nfats; wr16(s + 17, rootEnt);
  wr16(s + 19, total); s[21] = 0xF8; wr16(s + 22, spf); wr16(s + 24, 63); wr16(s + 26, 255);
  wr32(s + 28, 0); wr32(s + 32, 0);
  s[36] = 0x80; s[38] = 0x29; wr32(s + 39, (uint32_t)rand() ^ 0x50434350);
  char lab[11]; memset(lab, ' ', 11);
  for (size_t i = 0; i < 11 && label[i]; i++) lab[i] = toupper((unsigned char)label[i]);
  memcpy(s + 43, lab, 11);
  memcpy(s + 54, "FAT16   ", 8);
  s[510] = 0x55; s[511] = 0xAA;

  bool ok = g_disk.write(0, s, 512);
  for (uint32_t f = 0; f < nfats && ok; f++)
    for (uint32_t i = 0; i < spf && ok; i++) {
      uint8_t sec[512]; memset(sec, 0, 512);
      if (i == 0) { wr16(sec, 0xFFF8); wr16(sec + 2, 0xFFFF); }
      ok = g_disk.write((rsvd + f * spf + i) * 512u, sec, 512);
    }
  for (uint32_t i = 0; i < rootSecs && ok; i++) ok = g_disk.write((rsvd + nfats * spf + i) * 512u, z, 512);
  if (ok) {  // entree "volume label" dans la racine
    uint8_t e[32]; memset(e, 0, 32); memcpy(e, lab, 11); e[11] = 0x08;
    uint16_t d, t; nowFat(d, t); wr16(e + 22, t); wr16(e + 24, d);
    ok = g_disk.write((rsvd + nfats * spf) * 512u, e, 32);
  }
  if (!ok) return fail("ecriture du formatage impossible");
  g_disk.flush();
  _hint = 2;
  return mount();
}

uint32_t Fat16::totalBytes() { DiskLock lk; return mount() ? _clusters * clusterBytes() : 0; }

uint32_t Fat16::freeBytes() {
  DiskLock lk;
  if (!mount()) return 0;
  uint32_t n = 0;
  for (uint32_t c = 2; c < _clusters + 2; c++) if (fatGet(c) == 0) n++;
  return n * clusterBytes();
}

// ---------------------------------------------------------------- FAT
uint16_t Fat16::fatGet(uint32_t c) {
  uint8_t b[2] = {0xFF, 0xFF};
  g_disk.read(_fatStart * 512u + c * 2, b, 2);
  return rd16(b);
}

void Fat16::fatSet(uint32_t c, uint16_t v) {
  uint8_t b[2]; wr16(b, v);
  for (uint32_t i = 0; i < _nfats; i++) g_disk.write((_fatStart + i * _spf) * 512u + c * 2, b, 2);
}

uint32_t Fat16::allocCluster() {
  for (uint32_t n = 0; n < _clusters; n++) {
    uint32_t c = 2 + ((_hint - 2 + n) % _clusters);
    if (fatGet(c) == 0) {
      fatSet(c, 0xFFFF);
      _hint = c + 1; if (_hint >= _clusters + 2) _hint = 2;
      return c;
    }
  }
  return 0;
}

void Fat16::freeChain(uint32_t c) {
  for (uint32_t guard = 0; c >= 2 && c < _clusters + 2 && guard <= _clusters; guard++) {
    uint16_t nx = fatGet(c);
    fatSet(c, 0);
    if (nx < 2 || nx >= 0xFFF8) break;
    c = nx;
  }
}

// ---------------------------------------------------------------- repertoire racine
bool Fat16::rdEnt(uint32_t i, uint8_t* e) { return g_disk.read(_rootStart * 512u + i * 32, e, 32); }
bool Fat16::wrEnt(uint32_t i, const uint8_t* e) { return g_disk.write(_rootStart * 512u + i * 32, e, 32); }

bool Fat16::scan(std::vector<Item>& out) {
  out.clear();
  uint16_t lbuf[20 * 13];
  bool lv = false; uint8_t lsum = 0; int lfirst = -1;
  for (uint32_t i = 0; i < _rootEnt; i++) {
    uint8_t e[32];
    if (!rdEnt(i, e)) return fail("lecture du repertoire impossible");
    if (e[0] == 0x00) break;
    if (e[0] == 0xE5) { lv = false; continue; }
    uint8_t attr = e[11];
    if ((attr & 0x3F) == 0x0F) {                       // entree de nom long
      uint8_t ord = e[0];
      if (ord & 0x40) { lv = true; lsum = e[13]; lfirst = i; for (auto& c : lbuf) c = 0xFFFF; }
      if (!lv) continue;
      int idx = (ord & 0x3F) - 1;
      if (idx < 0 || idx >= 20 || e[13] != lsum) { lv = false; continue; }
      static const uint8_t pos[13] = {1, 3, 5, 7, 9, 14, 16, 18, 20, 22, 24, 28, 30};
      for (int k = 0; k < 13; k++) lbuf[idx * 13 + k] = rd16(e + pos[k]);
      continue;
    }
    if (attr & 0x08) { lv = false; continue; }         // etiquette de volume
    Item it; memcpy(it.raw, e, 32); it.sfn = i;
    std::string b((char*)e, 8), x((char*)e + 8, 3);
    while (!b.empty() && b.back() == ' ') b.pop_back();
    while (!x.empty() && x.back() == ' ') x.pop_back();
    if (!b.empty() && (uint8_t)b[0] == 0x05) b[0] = (char)0xE5;
    if (e[12] & 0x08) for (auto& c : b) c = tolower((unsigned char)c);
    if (e[12] & 0x10) for (auto& c : x) c = tolower((unsigned char)c);
    it.shortName = x.empty() ? b : b + "." + x;
    if (lv && lsum == lfnChecksum(e)) { it.first = lfirst; utf16to8(lbuf, 20 * 13, it.name); }
    else { it.first = i; it.name = it.shortName; }
    if (it.name.empty()) it.name = it.shortName;
    lv = false;
    out.push_back(it);
  }
  return true;
}

const Fat16::Item* Fat16::find(const std::vector<Item>& v, const std::string& n) {
  for (auto& it : v) if (ieq(it.name, n) || ieq(it.shortName, n)) return &it;
  return nullptr;
}

bool Fat16::list(std::vector<FatFileInfo>& out) {
  DiskLock lk;
  out.clear();
  if (!mount()) return false;
  std::vector<Item> v;
  if (!scan(v)) return false;
  for (auto& it : v) out.push_back({it.name, it.shortName, rd32(it.raw + 28), (bool)(it.raw[11] & 0x10)});
  return true;
}

// Construit les entrees (LFN puis 8.3) ; l'entree 8.3 n'a que son nom rempli.
bool Fat16::buildEntries(const std::string& name, const std::vector<Item>& items, std::vector<Ent>& out) {
  out.clear();
  uint8_t sfn[11];
  Ent se; memset(se.b, 0, 32);
  if (fit83(name, sfn)) {
    memcpy(se.b, sfn, 11);
    out.push_back(se);
    return true;
  }
  // nom long : fabriquer un alias 8.3 unique de la forme BASE~N.EXT
  size_t dot = name.rfind('.');
  std::string b = (dot == std::string::npos) ? name : name.substr(0, dot);
  std::string x = (dot == std::string::npos) ? "" : name.substr(dot + 1);
  std::string B, X;
  for (unsigned char c : b) { if (c == ' ' || c == '.') continue; B += sfnChar(toupper(c)) ? (char)toupper(c) : '_'; if (B.size() == 6) break; }
  for (unsigned char c : x) { if (c == ' ' || c == '.') continue; X += sfnChar(toupper(c)) ? (char)toupper(c) : '_'; if (X.size() == 3) break; }
  if (B.empty()) B = "_";
  bool found = false;
  for (int n = 1; n < 100000 && !found; n++) {
    std::string tail = "~" + std::to_string(n);
    std::string base = B.substr(0, std::min(B.size(), 8 - tail.size())) + tail;
    memset(sfn, ' ', 11);
    memcpy(sfn, base.data(), base.size());
    memcpy(sfn + 8, X.data(), X.size());
    found = true;
    for (auto& it : items) if (!memcmp(it.raw, sfn, 11)) { found = false; break; }
  }
  if (!found) return fail("impossible de generer un nom court");
  memcpy(se.b, sfn, 11);

  std::vector<uint16_t> u; utf8to16(name, u);
  uint32_t nl = (u.size() + 12) / 13;
  uint8_t sum = lfnChecksum(sfn);
  static const uint8_t pos[13] = {1, 3, 5, 7, 9, 14, 16, 18, 20, 22, 24, 28, 30};
  for (uint32_t k = nl; k >= 1; k--) {
    Ent le; memset(le.b, 0, 32);
    le.b[0] = k | (k == nl ? 0x40 : 0);
    le.b[11] = 0x0F; le.b[13] = sum;
    for (int j = 0; j < 13; j++) {
      uint32_t idx = (k - 1) * 13 + j;
      uint16_t c = idx < u.size() ? u[idx] : (idx == u.size() ? 0x0000 : 0xFFFF);
      wr16(le.b + pos[j], c);
    }
    out.push_back(le);
  }
  out.push_back(se);
  return true;
}

int Fat16::findSlots(uint32_t n) {
  uint32_t run = 0, start = 0;
  for (uint32_t i = 0; i < _rootEnt; i++) {
    uint8_t e[32];
    if (!rdEnt(i, e)) return -1;
    bool freeE = (e[0] == 0x00 || e[0] == 0xE5);
    if (!freeE) { run = 0; continue; }
    if (run == 0) start = i;
    run++;
    if (run >= n) return start;
    if (e[0] == 0x00) return (_rootEnt - start >= n) ? (int)start : -1;   // tout le reste est libre
  }
  return -1;
}

void Fat16::killItem(const Item& it) {
  for (uint32_t i = it.first; i <= it.sfn; i++) {
    uint8_t e[32];
    if (rdEnt(i, e)) { e[0] = 0xE5; wrEnt(i, e); }
  }
}

bool Fat16::remove(const std::string& name) {
  DiskLock lk;
  if (!mount()) return false;
  std::vector<Item> v; if (!scan(v)) return false;
  const Item* it = find(v, name);
  if (!it) return fail("fichier introuvable");
  if (it->raw[11] & 0x10) return fail("suppression de dossier non geree");
  uint32_t cl = rd16(it->raw + 26);
  killItem(*it);
  if (cl >= 2) freeChain(cl);
  return true;
}

bool Fat16::rename(const std::string& from, const std::string& to) {
  DiskLock lk;
  if (!mount()) return false;
  if (!validLongName(to)) return fail("nom invalide");
  std::vector<Item> v; if (!scan(v)) return false;
  const Item* it = find(v, from);
  if (!it) return fail("fichier introuvable");
  if (it->raw[11] & 0x10) return fail("renommage de dossier non gere");
  const Item* other = find(v, to);
  if (other && other->sfn != it->sfn) return fail("ce nom existe deja");
  std::vector<Ent> ents;
  if (!buildEntries(to, v, ents)) return false;
  int slot = findSlots(ents.size());
  if (slot < 0) return fail("repertoire racine plein");
  Item old = *it;
  uint8_t sfnRaw[32]; memcpy(sfnRaw, old.raw, 32);
  memcpy(sfnRaw, ents.back().b, 11);
  sfnRaw[12] = 0;                                    // pas de drapeaux de casse
  memcpy(ents.back().b, sfnRaw, 32);
  // checksum LFN recalcule sur le nouvel alias deja fait par buildEntries
  for (size_t i = 0; i < ents.size(); i++) if (!wrEnt(slot + i, ents[i].b)) return fail("ecriture repertoire");
  killItem(old);
  return true;
}

// ---------------------------------------------------------------- ecriture en flux
bool Fat16::beginWrite(const std::string& name) {
  DiskLock lk;
  if (_w.active) abortWrite();
  if (!mount()) return false;
  if (!validLongName(name)) return fail("nom de fichier invalide");
  std::vector<Item> v; if (!scan(v)) return false;
  const Item* it = find(v, name);
  if (it && (it->raw[11] & 0x10)) return fail("un dossier porte deja ce nom");
  _w = W();
  _w.active = true; _w.name = name;
  _w.buf.assign(clusterBytes(), 0);
  return true;
}

bool Fat16::flushCluster() {
  uint32_t c = allocCluster();
  if (!c) return fail("disque plein");
  if (_w.fill < _w.buf.size()) memset(_w.buf.data() + _w.fill, 0, _w.buf.size() - _w.fill);
  if (!g_disk.write(lba(c) * 512u, _w.buf.data(), _w.buf.size())) return fail("ecriture flash");
  if (_w.prev) fatSet(_w.prev, c); else _w.first = c;
  _w.prev = c; _w.fill = 0;
  return true;
}

bool Fat16::writeData(const uint8_t* d, size_t n) {
  DiskLock lk;
  if (!_w.active) return fail("pas d'ecriture en cours");
  while (n) {
    size_t room = _w.buf.size() - _w.fill;
    size_t k = n < room ? n : room;
    memcpy(_w.buf.data() + _w.fill, d, k);
    _w.fill += k; _w.size += k; d += k; n -= k;
    if (_w.fill == _w.buf.size() && !flushCluster()) return false;
  }
  return true;
}

void Fat16::abortWrite() {
  DiskLock lk;
  if (!_w.active) return;
  if (_w.first) freeChain(_w.first);
  _w = W();
}

bool Fat16::endWrite() {
  DiskLock lk;
  if (!_w.active) return fail("pas d'ecriture en cours");
  if (_w.fill > 0 && !flushCluster()) { abortWrite(); return false; }
  std::vector<Item> v;
  if (!scan(v)) { abortWrite(); return false; }
  // remplacement d'un fichier existant de meme nom
  if (const Item* old = find(v, _w.name)) {
    uint32_t cl = rd16(old->raw + 26);
    Item copy = *old;
    killItem(copy);
    if (cl >= 2) freeChain(cl);
    if (!scan(v)) { abortWrite(); return false; }
  }
  std::vector<Ent> ents;
  if (!buildEntries(_w.name, v, ents)) { abortWrite(); return false; }
  int slot = findSlots(ents.size());
  if (slot < 0) { abortWrite(); return fail("repertoire racine plein"); }
  uint8_t* e = ents.back().b;
  uint16_t d, t; nowFat(d, t);
  e[11] = 0x20;                                      // archive
  wr16(e + 14, t); wr16(e + 16, d); wr16(e + 18, d); wr16(e + 22, t); wr16(e + 24, d);
  wr16(e + 26, _w.first); wr32(e + 28, _w.size);
  for (size_t i = 0; i < ents.size(); i++)
    if (!wrEnt(slot + i, ents[i].b)) { abortWrite(); return fail("ecriture repertoire"); }
  _w = W();
  return true;
}

// ---------------------------------------------------------------- lecture en flux
bool Fat16::openRead(const std::string& name, uint32_t& size) {
  DiskLock lk;
  _r = R();
  if (!mount()) return false;
  std::vector<Item> v; if (!scan(v)) return false;
  const Item* it = find(v, name);
  if (!it || (it->raw[11] & 0x10)) return fail("fichier introuvable");
  size = rd32(it->raw + 28);
  _r.active = true; _r.cl = rd16(it->raw + 26); _r.remain = size; _r.off = 0;
  if (_r.cl < 2) _r.remain = 0;
  return true;
}

int Fat16::readData(uint8_t* buf, size_t n) {
  DiskLock lk;
  if (!_r.active) return 0;
  size_t tot = 0, cb = clusterBytes();
  while (tot < n && _r.remain > 0) {
    if (_r.off == cb) {
      uint16_t nx = fatGet(_r.cl);
      if (nx < 2 || nx >= 0xFFF8) { _r.remain = 0; break; }
      _r.cl = nx; _r.off = 0;
    }
    size_t chunk = std::min<size_t>(std::min<size_t>(n - tot, cb - _r.off), _r.remain);
    if (!g_disk.read(lba(_r.cl) * 512u + _r.off, buf + tot, chunk)) return -1;
    _r.off += chunk; _r.remain -= chunk; tot += chunk;
  }
  return (int)tot;
}
