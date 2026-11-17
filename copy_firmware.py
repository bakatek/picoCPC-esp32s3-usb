Import("env")
import os
import shutil

def copy_firmware_to_build(source, target, env):
    # Récupère le chemin absolu du fichier .bin généré via les variables de l'environnement PlatformIO
    firmware_path = env.subst("$BUILD_DIR/${PROGNAME}.bin")
    
    # Si le fichier .bin n'existe pas, essaie avec le format .hex par sécurité
    if not os.path.exists(firmware_path):
        firmware_path = env.subst("$BUILD_DIR/${PROGNAME}.hex")

    if not os.path.exists(firmware_path):
        print(f"Erreur : Aucun fichier firmware binaire trouvé dans {env.subst('$BUILD_DIR')}")
        return

    # Crée le dossier 'build' à la racine du projet s'il n'existe pas
    project_dir = env.get("PROJECT_DIR")
    build_dir = os.path.join(project_dir, "build")
    os.makedirs(build_dir, exist_ok=True)
    
    # Détermine le nom de destination avec le nom de l'environnement (ex: firmware_picocpc_bridge.bin)
    env_name = env["PIOENV"]
    ext = os.path.splitext(firmware_path)[1]
    dest_file = os.path.join(build_dir, f"firmware_{env_name}{ext}")
    
    # Copie effective du fichier
    shutil.copyfile(firmware_path, dest_file)
    print(f"Succès : Firmware copié vers -> {dest_file}")

# Enregistre l'action après la construction du programme
env.AddPostAction("buildprog", copy_firmware_to_build)