#pragma once
#include <Arduino.h>

// Petit journal en memoire (80 dernieres lignes), copie sur Serial, visible sur la page web.
void dbg(const char* fmt, ...) __attribute__((format(printf, 1, 2)));    // journal + Serial
void dbgq(const char* fmt, ...) __attribute__((format(printf, 1, 2)));   // journal seulement (rapide)
String dbgDump();
void dbgClear();
