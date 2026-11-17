# Script PlatformIO (post-build) : apres chaque compilation, prepare le dossier "release/" :
#   picocpc-usb-bridge-vX.Y-full.bin  image complete (bootloader + partitions + otadata + application)
#   picocpc-usb-bridge-vX.Y-app.bin   application seule (= celle a envoyer depuis la page web)
#   flash.py / flash.bat / flash.sh   flasheur autonome (Windows / Linux), voir LISEZMOI.txt
# L'image complete ne touche pas a la partition de la cle USB : on peut la flasher sans perdre les fichiers.
import os
import re
import shutil

OFFSETS = (("bootloader", 0x0), ("partitions", 0x8000), ("otadata", 0xE000), ("app", 0x10000))

README = """PicoCPC USB Bridge v%(ver)s - flasher le firmware
================================================

1. Branchez l'ESP32-S3 a l'ordinateur par le port "UART" de la carte (pas le port "USB").
2. Windows : double-clic sur flash.bat      Linux/macOS : ./flash.sh
   (Python 3 requis ; esptool s'installe tout seul a la premiere utilisation.)

   flash.bat --app        ne flashe que l'application (plus rapide)
   flash.bat --erase      efface TOUT d'abord (perd la cle virtuelle et le Wi-Fi)
   flash.bat --port COM5  force le port

Ensuite, les mises a jour peuvent se faire depuis la page web (carte "Mise a jour du firmware")
avec le fichier *-app.bin. Le flash par cable n'est utile que la premiere fois ou en cas de panne.
"""


def build_full(parts, out):
    """parts : dict nom -> chemin ; ecrit l'image fusionnee (octets libres = 0xFF)."""
    size = 0
    data = {}
    for name, off in OFFSETS:
        with open(parts[name], "rb") as f:
            data[name] = f.read()
        size = max(size, off + len(data[name]))
    img = bytearray(b"\xff" * size)
    for name, off in OFFSETS:
        d = data[name]
        end = off + len(d)
        if any(b != 0xFF for b in img[off:end]):
            raise RuntimeError("recouvrement de %s dans l'image fusionnee" % name)
        img[off:end] = d
    with open(out, "wb") as f:
        f.write(img)
    return len(img)


def make_release(project_dir, build_dir, boot_app0, tools_dir):
    with open(os.path.join(project_dir, "src", "config.h"), encoding="utf-8") as f:
        m = re.search(r'#define\s+FW_VERSION\s+"([^"]+)"', f.read())
    ver = m.group(1) if m else "x"
    rel = os.path.join(project_dir, "release")
    os.makedirs(rel, exist_ok=True)
    for old in os.listdir(rel):
        if old.endswith(("-full.bin", "-app.bin")):
            os.remove(os.path.join(rel, old))
    stem = os.path.join(rel, "picocpc-usb-bridge-v" + ver)
    app = os.path.join(build_dir, "firmware.bin")
    n = build_full({"bootloader": os.path.join(build_dir, "bootloader.bin"),
                    "partitions": os.path.join(build_dir, "partitions.bin"),
                    "otadata": boot_app0, "app": app}, stem + "-full.bin")
    shutil.copyfile(app, stem + "-app.bin")
    for fn in ("flash.py", "flash.bat", "flash.sh"):
        shutil.copyfile(os.path.join(tools_dir, "flash", fn), os.path.join(rel, fn))
    os.chmod(os.path.join(rel, "flash.sh"), 0o755)
    with open(os.path.join(rel, "LISEZMOI.txt"), "w", encoding="utf-8", newline="\n") as f:
        f.write(README % {"ver": ver})
    return rel, ver, n, os.path.getsize(app)


try:
    Import("env")  # noqa: F821  (fourni par PlatformIO/SCons)
except NameError:
    env = None

if env is not None:
    def _post(source, target, env):
        try:
            fw = env.PioPlatform().get_package_dir("framework-arduinoespressif32")
            rel, ver, n, a = make_release(env.subst("$PROJECT_DIR"), env.subst("$BUILD_DIR"),
                                          os.path.join(fw, "tools", "partitions", "boot_app0.bin"),
                                          os.path.join(env.subst("$PROJECT_DIR"), "tools"))
            print("RELEASE v%s : %s (application %d Ko sur 1536 Ko, image complete %d Ko)" % (ver, rel, a // 1024, n // 1024))
        except Exception as e:  # ne jamais faire echouer la compilation pour ca
            print("RELEASE : non genere (%s)" % e)
    env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", _post)
