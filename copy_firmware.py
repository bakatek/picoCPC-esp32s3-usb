Import("env")
import os
import shutil

def copy_firmware_to_build(source, target, env):
    # Chemin du fichier binaire généré par PlatformIO
    target_file = str(target[0])

    # Création du dossier 'build' à la racine s'il n'existe pas
    build_dir = os.path.join(env.get("PROJECT_DIR"), "build")
    os.makedirs(build_dir, exist_ok=True)

    # Nom du fichier de destination (inclut le nom de l'environnement)
    env_name = env["PIOENV"]
    ext = os.path.splitext(target_file)[1]
    dest_file = os.path.join(build_dir, f"firmware_{env_name}{ext}")

    # Copie effective du fichier
    shutil.copyfile(target_file, dest_file)
    print(f"Firmware exporté avec succès vers : {dest_file}")

# Enregistre la fonction pour qu'elle s'exécute après la compilation du programme
env.AddPostAction("buildprog", copy_firmware_to_build)