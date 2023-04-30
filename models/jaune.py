# Ouvrir le fichier d'entrée
with open("Rubber Duck.obj", "r") as f:
    lines = f.readlines()

# Ouvrir le fichier de sortie
with open("Rubber Duck jaune.obj", "w") as f:
    # Parcourir chaque ligne du fichier d'entrée
    for line in lines:
        # Si la ligne commence par "v ", ajouter la chaîne de caractères "0.973 0.929 0.0" à la fin
        if line.startswith("v "):
            line = line.rstrip() + " 0.973 0.929 0.0\n"
        # Écrire la ligne modifiée dans le fichier de sortie
        f.write(line)
