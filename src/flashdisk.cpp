#include "flashdisk.h"
#include <Arduino.h>
#include <string.h>
#include "esp_heap_caps.h"

FlashDisk g_disk;

static const uint32_t BLK = 4096;

bool FlashDisk::begin(const char* label, int cacheBlocks) {
  _part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_FAT, label);
  if (!_part) return false;
  _sectors = _part->size / 512;
  _mtx = xSemaphoreCreateRecursiveMutex();
  _n = cacheBlocks < 4 ? 4 : cacheBlocks;
  _c = new Block[_n];
  for (int i = 0; i < _n; i++) {
    uint8_t* p = (uint8_t*)heap_caps_malloc(BLK, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!p) p = (uint8_t*)heap_caps_malloc(BLK, MALLOC_CAP_8BIT);
    if (!p) { _n = i; break; }
    _c[i] = {-1, false, 0, p};
  }
  if (_n < 4) return false;
  _tmp = (uint8_t*)heap_caps_malloc(BLK, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (!_tmp) _tmp = (uint8_t*)heap_caps_malloc(BLK, MALLOC_CAP_8BIT);
  return _tmp != nullptr;
}

FlashDisk::Block* FlashDisk::get(uint32_t blk, bool load) {
  for (int i = 0; i < _n; i++)
    if (_c[i].blk == (int32_t)blk) { _c[i].stamp = ++_clock; return &_c[i]; }

  Block* v = nullptr;
  for (int i = 0; i < _n; i++) if (_c[i].blk < 0) { v = &_c[i]; break; }
  if (!v) {
    v = &_c[0];
    for (int i = 1; i < _n; i++) if (_c[i].stamp < v->stamp) v = &_c[i];
    if (v->dirty && !flushBlock(v)) return nullptr;
  }
  if (load) {
    if (esp_partition_read(_part, blk * BLK, v->data, BLK) != ESP_OK) { v->blk = -1; return nullptr; }
  }
  v->blk = blk; v->dirty = false; v->stamp = ++_clock;
  return v;
}

bool FlashDisk::flushBlock(Block* b) {
  if (!b->dirty) return true;
  uint32_t off = (uint32_t)b->blk * BLK;
  if (esp_partition_read(_part, off, _tmp, BLK) != ESP_OK) return false;
  bool same = true, progOnly = true;       // progOnly : on peut ecrire sans effacer (que des 1->0)
  for (uint32_t i = 0; i < BLK; i++) {
    if (_tmp[i] != b->data[i]) same = false;
    if ((_tmp[i] & b->data[i]) != b->data[i]) progOnly = false;
  }
  if (!same) {
    if (!progOnly && esp_partition_erase_range(_part, off, BLK) != ESP_OK) return false;
    if (esp_partition_write(_part, off, b->data, BLK) != ESP_OK) return false;
  }
  b->dirty = false;
  return true;
}

bool FlashDisk::rw(uint32_t addr, uint8_t* buf, uint32_t len, bool wr) {
  if (!_part) return false;
  if ((uint64_t)addr + len > (uint64_t)_sectors * 512) return false;
  lock();
  bool ok = true;
  while (len && ok) {
    uint32_t blk = addr / BLK, off = addr % BLK;
    uint32_t n = BLK - off; if (n > len) n = len;
    Block* b = get(blk, !(wr && off == 0 && n == BLK));
    if (!b) { ok = false; break; }
    if (wr) { memcpy(b->data + off, buf, n); b->dirty = true; _lastWrite = millis(); }
    else    { memcpy(buf, b->data + off, n); }
    addr += n; buf += n; len -= n;
  }
  unlock();
  return ok;
}

bool FlashDisk::dirty() const {
  for (int i = 0; i < _n; i++) if (_c[i].blk >= 0 && _c[i].dirty) return true;
  return false;
}

bool FlashDisk::flush() {
  lock();
  bool ok = true;
  for (int i = 0; i < _n; i++)
    if (_c[i].blk >= 0 && _c[i].dirty && !flushBlock(&_c[i])) ok = false;
  unlock();
  return ok;
}

void FlashDisk::flushIfIdle(uint32_t idleMs) {
  if (!dirty()) return;
  if (millis() - _lastWrite < idleMs) return;
  flush();
}
