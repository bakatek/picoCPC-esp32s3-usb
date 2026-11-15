#pragma once
// ============================================================
//  PicoCPC USB Bridge - configuration
// ============================================================

// --- Wi-Fi ----------------------------------------------------
// Si WIFI_SSID est vide, ou si la connexion echoue apres 15 s,
// l'ESP32 cree son propre reseau (point d'acces) :
//   SSID AP_SSID, mot de passe AP_PASS, page sur http://192.168.4.1/
#define WIFI_SSID   ""
#define WIFI_PASS   ""
#define AP_SSID     "PicoCPC-ESP32"
#define AP_PASS     "picocpc1234"      // 8 caracteres minimum
#define HOSTNAME    "picocpc"          // http://picocpc.local/ (mDNS)
#define TZ_INFO     "CET-1CEST,M3.5.0,M10.5.0/3"   // pour l'horodatage des fichiers

// --- Disque virtuel ---------------------------------------------
#define DISK_PART_LABEL   "disk"       // nom de la partition dans partitions.csv
#define VOLUME_LABEL      "PICOCPC"    // etiquette du volume FAT16
#define CACHE_BLOCKS      16           // blocs de 4 Ko en cache d'ecriture (en PSRAM)
#define FLUSH_IDLE_MS     400          // ecriture en flash apres 400 ms sans ecriture

// --- "Live" : remontage de la cle vers la PicoCPC ---------------
// Apres une modification faite depuis la page web, l'ESP32 se detache
// brievement du bus USB puis se rattache : la PicoCPC voit la cle
// retiree puis remise, et relit son contenu.
#define AUTO_REMOUNT      1            // 1 = automatique, 0 = bouton seulement
#define REMOUNT_DELAY_MS  2500         // delai apres la derniere modification
#define DETACH_MS         700          // duree du detachement USB

// --- Bluetooth ----------------------------------------------------
#define BLE_PASSKEY       123456       // code a taper sur un clavier BT lors de l'appairage
