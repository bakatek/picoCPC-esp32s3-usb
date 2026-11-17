# PicoCPC USB Bridge (ESP32-S3)

**English** · [Français](README_FR.md) · [Español](README_ES.md)

An ESP32-S3 plugged into the USB-A host port of a **PicoCPC** (Amstrad CPC) that turns it into:

* a **virtual USB stick** stored in the ESP32's flash (FAT16, about 12.9 MiB) — upload, rename and delete files from a web page, with the free space displayed;
* a **USB keyboard and mouse** for the PicoCPC, fed by a **Bluetooth** keyboard/mouse paired with the ESP32, or by a capture zone on the web page;
* a **remote typist**: paste a BASIC listing or commands in the page and the ESP32 types them key by key on the CPC;
* **virtual floppy shortcuts**: « Mount A / B » next to each `.dsk` (types `|DLOAD,n`), eject, floppy groups (`|DGP`, `|DRESET`).

The page is available in **English, French and Spanish** (selector at the top right).

## Screenshots

![Screenshot 1](imgs/1.png)

![Screenshot 2](imgs/2.png)

![Screenshot 3](imgs/3.png)

## Hardware

* ESP32-S3 **DevKitC-1 N16R8** (16 MB flash, 8 MB octal PSRAM).
* The board has two USB-C ports:
  * **UART** port: flashing and serial monitor (connect it to your computer);
  * **USB** port (native): connect it to the PicoCPC USB-A port with a **data** cable (USB-A → USB-C). The PicoCPC provides the 5 V.
* No microSD needed: everything lives in flash.

## Build and first flash (PlatformIO / VSCodium)

1. Open the folder in VSCodium/VS Code with the PlatformIO extension.
2. Optional: edit `src/config.h` (Wi-Fi SSID/password, options). Without Wi-Fi settings the ESP32 creates the network **`PicoCPC-ESP32`** (password `picocpc1234`).
3. Plug the **UART** port into your computer, then *Upload* (`pio run -t upload`). The first time this must be done with PlatformIO because it installs the partition table (2 firmware slots + the stick).
4. Unplug it from the computer and connect the **USB** port to the PicoCPC.
5. Open `http://picocpc.local/` (or `http://192.168.4.1/` on the ESP32's own network). Wi-Fi can be set in the page (« Wi-Fi » card).

After every build the **`release/`** folder is generated (see below).

## Updating the firmware

* **From the web page**: « Firmware update » card → choose `release/picocpc-usb-bridge-vX.Y-app.bin` (or `.pio/build/picocpc_bridge/firmware.bin`) → *Update*. The image is checked (ESP32-S3), written to the free slot and the ESP32 restarts. Stick files and Wi-Fi settings are kept.
* **With a cable (Windows / Linux / macOS)**: copy the `release/` folder anywhere, plug the **UART** port and run `flash.bat` (Windows) or `./flash.sh` (Linux/macOS). Python 3 is required; `esptool` installs itself on first use. `flash.py --app` flashes only the application; `flash.py --erase` wipes **everything** (including the stick and Wi-Fi settings).

## Using the web page

| Card | What it does |
| --- | --- |
| **Virtual USB stick** | Drag & drop files, download / rename / delete, free space, *Remount now* (the PicoCPC sees the stick unplugged then replugged and re-reads it), *Format*. Files are listed alphabetically, like `|DCAT`. |
| **Virtual CPC floppies** | `|DCAT`, eject A:/B:, group A:/B: (`|DGP`), reset groups (`|DRESET`). « Mount A / B » next to a `.dsk` types `|DLOAD,n[,"B"]`. « No. of the 1st disk in `|DCAT` » is `0` by default. |
| **Keyboard / mouse** | Click the zone to capture your keyboard (tick *mouse* to also capture the pointer; Esc releases it). *Type / paste text* sends text key by key (Ctrl+V works in the zone too). Latency of the WebSocket is shown. |
| **Bluetooth** | *Scan*, connect, forget. Pairing code for keyboards: **123456** (`BLE_PASSKEY`). Paired devices reconnect on their own. |
| **Wi-Fi** | Scan networks, set SSID/password, save and restart. Empty SSID = back to the ESP32 access point. |
| **Firmware update** | See above. |
| **USB log** | The last 80 requests the PicoCPC made to the stick. |

### Settings on the PicoCPC side

* The PicoCPC takes **about 10 s** to recognise the ESP32 after power-up or after a *remount*. Do not type `|DCAT` before that.
* « PicoCPC layout » in the page must match `KeyboardLang` in `PicoCPC.cfg` (`[USB]`): **FR** (French; `|` via AltGr+6), **FR_PIPE** (French, `ù` key gives `|`) or **EN**. If you see `Q` instead of `A`, choose **FR**.
* The hardware allows **only one HID interface**: the ESP32 is either a **keyboard** or a **mouse** (buttons *Keyboard / Mouse*, the ESP32 restarts). Keyboard is the default.
* Keyboard and mouse use standard *boot protocol* (keyboard = protocol 1, mouse = protocol 2) because the PicoCPC firmware sorts devices by protocol.

## Troubleshooting

| Symptom | Cause / fix |
| --- | --- |
| `|DCAT` hangs or does nothing right after connecting or remounting | Wait ~10 s (the PicoCPC is still enumerating). |
| Letters are wrong (`A` → `Q`), `|` missing or `ù` shown | Wrong « PicoCPC layout »: choose **FR** (or **FR_PIPE**) as in `PicoCPC.cfg`. |
| Typed text does nothing | The ESP32 must be in **keyboard** mode and the CPC at the BASIC prompt. |
| ESP32 reboots when Bluetooth starts | Never call `WiFi.setSleep(false)` (incompatible with Bluetooth). It is not in this project. |
| *Program size greater than maximum allowed* at build | The firmware must fit in a 1.5 MB slot (`partitions.csv`). |
| The stick is unreadable on a PC or the CPC | *Format* in the page, or hold the **BOOT** button for 3 s on the ESP32 (re-formats the volume). |
| Keyboard feels slow | Look at the *latency* chip. Wi-Fi power-saving (forced by Bluetooth) adds delay: connect to the ESP32's own access point for the lowest latency. |
| Flashing fails | Use the **UART** port (not **USB**); hold BOOT, press RESET, release; try `--baud 115200`; Linux: `sudo usermod -aG dialout $USER`. |

## Project layout

```
platformio.ini        PlatformIO environment (espressif32 6.9.0, Arduino 2.0.17)
partitions.csv        nvs, otadata, app0, app1 (1.5 MB each), disk (virtual stick)
src/config.h          Wi-Fi, options, firmware version
src/main.cpp          web server, WebSocket, Wi-Fi, OTA, input task
src/usb_dev.cpp       USB: mass storage + custom boot HID (keyboard or mouse)
src/typer.cpp         automatic typing engine (regular timing)
src/ble_hid.cpp       Bluetooth HID client (keyboard/mouse)
src/flashdisk.cpp     flash-backed disk with write cache
src/fat16.cpp         FAT16 with long file names
src/web_ui.h          the web page (fr/en/es)
tools/flash/          standalone flasher (flash.py / .bat / .sh)
tools/release.py      builds release/ after each compilation
tools/hosttest/       PC tests of the disk + FAT16 code
```
