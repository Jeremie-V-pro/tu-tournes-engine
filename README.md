# Tu-Tournes engine

par GRANGER Aloïs VERNAY Jérémie

Ce projet consiste en la modélisation et l'animation d'un océan et des nuage sur un moteur de rendu C++ / Vulkan.

## Image du rendu final

![alt text](http://www.image-heberg.fr/files/1701529577642878628.png )


# Mise en place du projet

Le projet à été consue et tester uniquement sur Linux. Il peut normalement fonctionner sur windows mais il n'as pas été testé.

## Dépendances vulkan

[Lien tuto installation vulkan](https://vulkan-tutorial.com/Development_environment)

Le site du tutoriel de vulkan indique quel paquet installer selon l'os et la distribution.
Il indique les paquets pour vulkan ainsi que les paquets pour la compilation des shaders, la gestion de la fenêtre...

## Dépendances pour la compilation

- gcc
- g++
- cmake
- make

## Consigne pour compiler

### Sur UNIX

Vous avez juste à éxecuter la commande : `./unixBuild.sh`

Le programme s'execute automatiquement

### Pour les autres OS

Pour compiler :
```
mkdir build && cd build
cmake ..
cmake --build ./ --target Shaders
cmake --build ./ --target all
```
Pour lancer :
```
./LveEngine
```
# Explication projet

## Sources

Pour le moteur nous nous somme basé sur le tutoriel de [Brendan Galea](https://www.youtube.com/@BrendanGalea) qui suit le tutoriel du site [vulkan-tutorial](https://vulkan-tutorial.com/).
Cela nous à permis d'avoir une base qui permetait de charger et d'afficher des fichier 3D (obj) avec un modèle de lumière Blinn–Phong.

Pour l'implémentation de l'eau nous nous somme basé sur l'implémenation [Jump Trajectory](https://www.youtube.com/watch?v=kGEqaX4Y4bQ).


## Structure du projet

Tout les fichier du moteur se trouve dans src. Les shaders et les différentes ressource se trouve dans leur dossiers respectif.

### Explication du moteur de rendu

#### - firs_app -
C'est ce fichier qui s'occupe de l'initialisation et de la boucle du moteur. Il charge les différents shader, les objets de la scène, les objets vulkan.

#### - keyboard_movement_component -
Ce fichier s'occupe de la gestion des entrées clavier. Il permet de déplacer la caméra dans la scène.

#### - lve_buffer -
Ce fichier s'occupe de la gestion des buffers. Il permet de créer des buffers, de les remplir et de les détruire. Les buffers sont utilisé pour stoquer les donnée de transfert entre le gpu et le cpu, comme les vertex, les uniform buffer...

#### - lve_c_pipeline -
Ce fichier s'occupe de la gestion des pipelines de calcul. Il permet de créer des pipelines de calcul, de les détruire et de les utiliser.

#### - lve_camera -
Ce fichier s'occupe de la gestion de la caméra. Créer les matrices de projection et de vue. Il permet aussi de déplacer la caméra.

#### - lve_descriptor_set -
Ce fichier s'occupe de la gestion des descriptor set et layout. Les descriptors servent à passer beaucoup de donnée au shaders (images, liste d'objet....).

#### - lve_device -
Ce fichier s'occupe de la gestion du GPU. Cela permet d'obtenir tout les informations sur le GPU (queue, extension supporté, taille de descriptor set max, taille de push constant max...). C'est lui qui permet de faire le lien entre le CPU et le GPU avec à travers les fonction de vulkan.

#### - lve_g_pipeline -
Ce fichier s'occupe de la gestion des pipelines graphique. Les pipeline graphic indique quel étape du rendu seront effectuer (vertex, fragment, tesselation). Cela permet de modifier les paramètre de rendu général (transparence, multi sampling).

#### - lve_game_object -
Ce fichier s'occupe de la gestion des objets de la scène.

#### - lve_model -
Ce fichier s'occupe de la gestion des modèles 3D. Il permet de charger un fichier obj et de le stocker dans un buffer.

#### - lve_post_processing_manager -
Ce fichier s'occupe de la gestion du post processing. Il permet d'exécuter plusieurs système de post processing en leur fournissant une image d'entré et de sortie.

#### - lve_pre_processing_manager -
Ce fichier s'occupe de la gestion du pre processing. Il permet d'exécuter plusieurs système de pre processing qui servent à préparer les donnée pour le rendu (bouger la position de particule, culling...).

#### - lve_renderer -
Ce fichier s'occupe de la gestion du rendu. Il permet de créer les différents objets de rendu (command buffer, swapchain, render pass, frame buffer...). Il s'occupe de lancer les différents étape du rendu.

#### - lve_swap_chain -
Ce fichier s'occupe de la gestion de la swapchain. La swapchain est un ensemble d'image qui seront afficher à l'écran.

#### - lve_texture -
Ce fichier s'occupe de la gestion des textures. Il permet de charger une texture et de la stocker dans un buffer à partir de plusieurs source (fichier, donnée généré par le processeur) pour différente utilisation (texture pour du calcul, pour être sampler sur un modèle...).

#### - lve_window -
Ce fichier s'occupe de la gestion de la fenêtre. Il permet de récupérer les entrées clavier et souris.


### Explication des systèmes

Les différent système sont dans le dossier src/systems. Et sont divisé en 2 catégories : ceux de rendu et ceux de calcul.

#### - WaveGenerationSystem -

Ce système s'occupe de générer les vagues. Ce system implémente l'algorithme de [Jump Trajectory](https://www.youtube.com/watch?v=kGEqaX4Y4bQ) pour la génération des texture de displacement, de normal et de turbulence en se basant sur les fonction JONSWAP et de FFT.


#### - water_system -

Ce système s'occupe du rendu de l'eau. Il utilise les textures génére par le système WaveGenerationSystem pour calculer la position des vertex de l'eau et les afficher à l'écran. Il utilise aussi une texture de normal pour simuler les reflet de la lumière sur l'eau.