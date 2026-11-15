#pragma once
// Disque virtuel de 512 octets/secteur au-dessus d'une partition flash brute,
// avec cache d'ecriture (blocs de 4 Ko = taille d'effacement de la flash).
#include <stdint.h>
#include <stddef.h>
#include "esp_partition.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

class FlashDisk {
public:
  bool begin(const char* label, int cacheBlocks);
  uint32_t sectorCount() const { return _sectors; }

  // Acces par adresse en octets (thread-safe)
  bool read(uint32_t addr, void* buf, uint32_t len)        { return rw(addr, (uint8_t*)buf, len, false); }
  bool write(uint32_t addr, const void* buf, uint32_t len) { return rw(addr, (uint8_t*)buf, len, true); }

  bool flush();                       // ecrit tous les blocs modifies en flash
  void flushIfIdle(uint32_t idleMs);  // idem, apres idleMs sans ecriture
  bool dirty() const;

  void lock()   { if (_mtx) xSemaphoreTakeRecursive(_mtx, portMAX_DELAY); }
  void unlock() { if (_mtx) xSemaphoreGiveRecursive(_mtx); }

private:
  struct Block { int32_t blk; bool dirty; uint32_t stamp; uint8_t* data; };
  bool rw(uint32_t addr, uint8_t* buf, uint32_t len, bool wr);
  Block* get(uint32_t blk, bool load);
  bool flushBlock(Block* b);

  const esp_partition_t* _part = nullptr;
  uint32_t _sectors = 0;
  Block* _c = nullptr;
  int _n = 0;
  uint8_t* _tmp = nullptr;
  uint32_t _clock = 0;
  uint32_t _lastWrite = 0;
  SemaphoreHandle_t _mtx = nullptr;
};

extern FlashDisk g_disk;
