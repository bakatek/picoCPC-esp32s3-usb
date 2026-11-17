#include "dbglog.h"
#include <stdarg.h>

static const int N = 80;
static char lines[N][100];
static uint32_t head = 0, first = 0;
static SemaphoreHandle_t m = nullptr;

static void ensure() { if (!m) m = xSemaphoreCreateMutex(); }

static void put(const char* b, bool echo) {
  ensure();
  xSemaphoreTake(m, portMAX_DELAY);
  snprintf(lines[head % N], sizeof lines[0], "%8lu %s", (unsigned long)millis(), b);
  head++;
  if (head - first > (uint32_t)N) first = head - N;
  xSemaphoreGive(m);
  if (echo) Serial.println(b);
}

void dbg(const char* fmt, ...) {
  char b[88];
  va_list a; va_start(a, fmt); vsnprintf(b, sizeof b, fmt, a); va_end(a);
  put(b, true);
}

void dbgq(const char* fmt, ...) {
  char b[88];
  va_list a; va_start(a, fmt); vsnprintf(b, sizeof b, fmt, a); va_end(a);
  put(b, false);
}

String dbgDump() {
  ensure();
  String o;
  xSemaphoreTake(m, portMAX_DELAY);
  for (uint32_t i = first; i < head; i++) { o += lines[i % N]; o += '\n'; }
  xSemaphoreGive(m);
  return o;
}

void dbgClear() {
  ensure();
  xSemaphoreTake(m, portMAX_DELAY);
  first = head;
  xSemaphoreGive(m);
}
