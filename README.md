# Space Shooter 3D — INFO-H-502

Jeu de vaisseau spatial en OpenGL où le joueur doit éviter des astéroïdes dans l'espace.  
Projet réalisé dans le cadre du cours **INFO-H-502 – 3D Graphics** (ULB, 2025-2026).

---

## Auteur

DUMAN Louis-David

## Compilation

### Prérequis

- CMake ≥ 3.16
- Compilateur C++17 (GCC, Clang ou MSVC)

### Build

```bash
git clone https://github.com/L6uis9/projet-info-h502.git
cd projet-info-h502
mkdir build && cd build
cmake --build .
```

## Lancement

```bash
cd build
./SpaceGame
```

---

## Contrôles

| Touche | Action |
|---|---|
| `Z / W` | Avancer |
| `S` | Reculer |
| `Q / A` | Déplacer à gauche |
| `D` | Déplacer à droite |
| `Espace` | Monter |
| `Shift` | Descendre |
| `Souris` | Orienter la caméra |
| `Échap` | Quitter |
---

## Features implémentées

### Basiques (obligatoires)

- **Lumières** — modèle de Phong (ambiante, diffuse, spéculaire) dans `basic.frag`
- **Textures** — textures diffuse et spéculaire sur le vaisseau et les astéroïdes (`stb_image`)
- **Plusieurs modèles** — vaisseau (`spaceship.obj`) et astéroïde (`asteroid.obj`) chargés via `tinyobjloader`
- **Cubemap** — skybox spatiale avec `GL_TEXTURE_CUBE_MAP` (`Skybox.cpp`)
- **Game logic** — score, vies, états de jeu (menu / jeu / game over)
- **Déplacement** — vaisseau déplaçable dans les 6 directions avec accélération
- **Caméra libre** — navigation free-fly à la souris (`Camera.cpp`)
- **Réflexion / réfraction** — effet de bouclier sur le vaisseau via `reflect.frag` + env map

### Intermédiaires

### Avancées

## Lien avec la théorie

Chapitre choisi : **Chapitre 16**

## Structure du projet


## Vidéo de démonstration

[Lien à compléter]
