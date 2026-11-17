#!/usr/bin/env python3
"""Flashe le firmware PicoCPC USB Bridge sur l'ESP32-S3 (Windows, Linux, macOS).

Utilisation (ESP32 branche par le port "UART" de la carte, pas par le port "USB") :
    python flash.py                  # flashe l'image complete (bootloader + partitions + application)
    python flash.py --app            # flashe seulement l'application (garde tout le reste)
    python flash.py --port COM5      # force le port serie (Linux : /dev/ttyUSB0)
    python flash.py --erase          # efface TOUTE la flash avant (supprime aussi la cle virtuelle et le Wi-Fi)

Le script a besoin de "esptool" ; s'il manque, il l'installe tout seul (acces internet requis la 1re fois).
Ne touche jamais a la partition de la cle USB, sauf avec --erase.
"""
import argparse
import glob
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
APP_OFFSET = "0x10000"
VENV = os.path.join(HERE, ".flash-venv")


def run(cmd, quiet=False):
    return subprocess.run(cmd, stdout=subprocess.DEVNULL if quiet else None,
                          stderr=subprocess.DEVNULL if quiet else None).returncode


def has_esptool(py):
    return run([py, "-c", "import esptool"], quiet=True) == 0


def venv_python():
    sub = "Scripts" if os.name == "nt" else "bin"
    return os.path.join(VENV, sub, "python.exe" if os.name == "nt" else "python")


def find_python():
    """Retourne un interpreteur Python qui dispose d'esptool, en l'installant si besoin."""
    if has_esptool(sys.executable):
        return sys.executable
    vp = venv_python()
    if os.path.exists(vp) and has_esptool(vp):
        return vp
    print("esptool n'est pas installe : installation (internet requis)...")
    if run([sys.executable, "-m", "pip", "install", "--user", "--quiet", "esptool"]) == 0 and has_esptool(sys.executable):
        return sys.executable
    print("pip direct refuse (systeme gere par le gestionnaire de paquets) : creation d'un environnement prive...")
    if run([sys.executable, "-m", "venv", VENV]) != 0:
        sys.exit("Impossible de creer l'environnement Python.\n"
                 "Linux (Debian/Ubuntu) : sudo apt install python3-venv   puis relancez.\n"
                 "Ou installez esptool vous-meme : pip install esptool")
    if run([vp, "-m", "pip", "install", "--quiet", "esptool"]) != 0 or not has_esptool(vp):
        sys.exit("Installation d'esptool impossible (pas d'internet ?). Installez-le : pip install esptool")
    return vp


def pick(pattern):
    found = sorted(glob.glob(os.path.join(HERE, pattern)))
    return found[-1] if found else None


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--app", action="store_true", help="flasher seulement l'application (firmware-app)")
    ap.add_argument("--file", help="fichier .bin a flasher (a la place de celui du dossier)")
    ap.add_argument("--port", help="port serie (COM5, /dev/ttyUSB0...) ; sinon detection automatique")
    ap.add_argument("--baud", default="460800", help="vitesse (defaut 460800 ; 115200 si ca echoue)")
    ap.add_argument("--erase", action="store_true", help="effacer toute la flash avant (perd la cle virtuelle et le Wi-Fi)")
    ap.add_argument("--yes", action="store_true", help="ne pas demander de confirmation")
    a = ap.parse_args()

    if a.app and a.erase:
        sys.exit("--erase efface aussi le bootloader : utilisez-le sans --app.")
    if a.app:
        fw, off = a.file or pick("*-app.bin"), APP_OFFSET
    else:
        fw, off = a.file or pick("*-full.bin"), "0x0"
    if not fw or not os.path.exists(fw):
        sys.exit("Fichier a flasher introuvable (cherche *-%s.bin a cote de flash.py)." % ("app" if a.app else "full"))

    print("Fichier : %s  (%d Ko) -> adresse %s" % (os.path.basename(fw), os.path.getsize(fw) // 1024, off))
    if a.erase:
        print("ATTENTION : --erase supprime aussi tous les fichiers de la cle virtuelle et le reglage Wi-Fi.")
    if not a.yes:
        if input("Continuer ? [O/n] ").strip().lower() in ("n", "non", "no"):
            sys.exit("Annule.")

    py = find_python()
    base = [py, "-m", "esptool", "--chip", "esp32s3", "--baud", a.baud]
    if a.port:
        base += ["--port", a.port]
    if a.erase and run(base + ["erase_flash"]) != 0:
        sys.exit(fail_help())
    if run(base + ["write_flash", off, fw]) != 0:
        sys.exit(fail_help())
    print("\nTermine. L'ESP32 redemarre. Debranchez le cable UART si vous voulez, il n'est pas necessaire ensuite.")


def fail_help():
    return ("\nEchec. Verifiez :\n"
            " - le cable est dans le port « UART » (pas « USB ») de la carte ;\n"
            " - maintenez BOOT, appuyez sur RESET, relachez RESET puis BOOT, et relancez ;\n"
            " - essayez --baud 115200 ;\n"
            " - Linux : sudo usermod -aG dialout $USER (puis reconnexion), ou lancez avec sudo ;\n"
            " - Windows : pilote CP210x / CH340 installe, port visible dans le gestionnaire de peripheriques.")


if __name__ == "__main__":
    main()
