#pragma once
// Gestionnaire FAT16 minimal (repertoire racine uniquement, noms longs VFAT geres)
// qui travaille directement sur le meme disque que celui expose en USB.
#include <stdint.h>
#include <stddef.h>
#include <string>
#include <vector>

struct FatFileInfo {
  std::string name;        // nom long (ou 8.3 si pas de nom long)
  std::string shortName;   // alias 8.3
  uint32_t size;
  bool dir;
};

class Fat16 {
public:
  bool mount();                         // verifie le BPB (appele avant chaque operation)
  bool format(const char* label);
  bool list(std::vector<FatFileInfo>& out);
  bool remove(const std::string& name);
  bool rename(const std::string& from, const std::string& to);
  uint32_t totalBytes();
  uint32_t freeBytes();

  // ecriture en flux (un seul fichier a la fois)
  bool beginWrite(const std::string& name);
  bool writeData(const uint8_t* d, size_t n);
  bool endWrite();
  void abortWrite();
  bool writing() const { return _w.active; }

  // lecture en flux
  bool openRead(const std::string& name, uint32_t& size);
  int  readData(uint8_t* buf, size_t n);   // -1 erreur, 0 fin

  std::string lastError;

private:
  struct Item {
    uint16_t first, sfn;      // index de la 1re entree (LFN) et de l'entree 8.3
    std::string name, shortName;
    uint8_t raw[32];
  };
  struct Ent { uint8_t b[32]; };

  bool fail(const char* m) { lastError = m; return false; }
  bool scan(std::vector<Item>& out);
  const Item* find(const std::vector<Item>& v, const std::string& n);
  bool buildEntries(const std::string& name, const std::vector<Item>& items, std::vector<Ent>& out);
  int  findSlots(uint32_t n);
  bool rdEnt(uint32_t i, uint8_t* e);
  bool wrEnt(uint32_t i, const uint8_t* e);
  void killItem(const Item& it);

  uint16_t fatGet(uint32_t c);
  void fatSet(uint32_t c, uint16_t v);
  uint32_t allocCluster();
  void freeChain(uint32_t c);
  uint32_t lba(uint32_t c) const { return _dataStart + (c - 2) * _spc; }
  uint32_t clusterBytes() const { return _spc * 512u; }
  bool flushCluster();

  // geometrie
  bool _ok = false;
  uint32_t _spc = 0, _rsvd = 0, _nfats = 0, _rootEnt = 0, _spf = 0, _total = 0;
  uint32_t _fatStart = 0, _rootStart = 0, _dataStart = 0, _clusters = 0, _hint = 2;

  struct W { bool active = false; std::string name; std::vector<uint8_t> buf; uint32_t fill = 0, first = 0, prev = 0, size = 0; } _w;
  struct R { bool active = false; uint32_t cl = 0, remain = 0, off = 0; } _r;
};

extern Fat16 g_fat;
