# PicoCPC USB Bridge (ESP32-S3)

Cle USB virtuelle (FAT16 en flash, ~12,9 Mo) + clavier/souris USB pour la PicoCPC,
pilotes par une page web et par Bluetooth.

1. Editer `src/config.h` (Wi-Fi, sinon l'ESP32 cree le reseau `PicoCPC-ESP32`, mdp `picocpc1234`).
2. Brancher la carte par le port **UART** au PC, `pio run -t upload`, `pio device monitor`.
3. Debrancher du PC, relier le port **USB** (natif) de la carte au port USB-A de la PicoCPC
   (cable USB-A -> USB-C donnees).
4. Ouvrir `http://192.168.4.1/` (mode AP) ou `http://picocpc.local/`.

Tests hote (PC) du disque + FAT16 : `tools/hosttest` (g++ + python3), voir `test.cpp` et `verify_fat16.py`.

TODO :
- tester BLE
- ajouter monter/demonter a coté des fichiers sur la clé virtuelle. (cela fait un dload/deject correspondant), ajouter un bouton pour le groupe DGP
- ajouter la possibilité de coller du code via clavier.
- clean la partie saisie wifi (scan ssid fonctionne ?)
- essayer d'optimiser la saisie clavier qui est peu réactive.
- ajouter un update FW sur la page web
- ajouter dans le projet un binaire (windows et linux) pour flasher le fw simplement (python ?).