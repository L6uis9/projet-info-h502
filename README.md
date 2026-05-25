# SpaceGame - INFO-H-502

A 3D space shooter in OpenGL where the player must avoid asteroids in space.  
Project made for the course **INFO-H-502 - 3D Graphics** (ULB, 2025-2026).

## Author

DUMAN Louis-David

## Build

### Requirements

- CMake ≥ 3.16
- C++17 compiler (GCC, Clang or MSVC)
- OpenGL, GLFW, GLAD (bundled in the repository)

### Steps

```bash
git clone https://github.com/L6uis9/projet-info-h502.git
cd projet-info-h502
mkdir build && cd build
cmake ..
cmake --build .
```

## Running

## Running

**Linux / macOS:**
```bash
./build/SpaceGame
```

**Windows:**
```powershell
.\build\Debug\SpaceGame.exe
```

## Controls

| Key | Action |
|---|---|
| `Z / W` | Move forward |
| `S` | Look backward |
| `X` | Toggle spaceship refraction |
| `Mouse` | Aim the spaceship |
| `Escape` | Quit |

---

## Implemented Features

### Basic

- Lighting (ambient, diffuse, specular) - applied to all models (spaceship, Earth, asteroids) using star light sources
- Textures - stone texture on asteroids with triplanar mapping, diffuse texture on the spaceship and Earth
- Loading multiple models - spaceship, Earth (planet in the scene)
- Cubemap - space environment skybox surrounding the entire scene
- Game logic - player navigates and avoids asteroids, collisions trigger explosions, invicibility management when the spaceship get hit
- Objects moving in the scene - asteroids continuously spawning and moving through the scene
- Free navigation (camera control) - third-person camera locked behind the spaceship, steered by mouse
- Reflection / Refraction - reflection applied to the spaceship when `X` is pressed, refraction applied to the spaceship's windows.

### Intermediate

- Frame Buffer Object - motion blur applied to the whole scene during fast movement
- Particles - engine fire trail while thrusting, burst explosions on asteroid and ship collisions
- Billboarding - stars rendered as screen-aligned quads that always face the camera

### Advanced

- Collision Detection - sphere-based collision detection between the spaceship and asteroids, handled manually without external physics library
- 3D Mesh Loading from File - OBJ + MTL format parsed by hand, supporting vertices, normals, UV coordinates, and material textures

---

## Link with Theory

Chosen chapter: **Chapter 16** - *Procedural Fractal Terrain* - "Texturing and Modeling: A Procedural Approach" by D. Ebert et al.

The project implements procedural fractal terrain generation for asteroids directly related to the techniques described in this chapter.

---

## Project Structure

```
projet-info-h502/
├── src/              # C++ source code
│   ├── main.cpp
│   ├── camera.{cpp,h}
│   ├── asteroid.{cpp,h}
│   ├── model.{cpp,h}
│   ├── particle.{cpp,h}
│   ├── postprocess.{cpp,h}
│   ├── skybox.{cpp,h}
│   ├── star.{cpp,h}
│   └── shader.{cpp,h}
├── shaders/          # GLSL shaders
│   ├── model.{vert,frag}
│   ├── motionblur.{vert,frag}
│   ├── particle.{vert,frag}
│   ├── skybox.{vert,frag}
│   └── star.{vert,frag}
├── docs/
│   └── Guidelines_project.pdf
│   └── Texturing And Modeling. A Procedural Approach (David S. Ebert, F. Kenton Musgrave etc.)
│   └── video.mp4
│   └── report.pdf
└── CMakeLists.txt
```

## Demo Video

See in docs/video.mp4

---

## Sources & References

- [Cubemap image](https://www.eso.org/public/images/eso0932a/)
- [Image to cubemap converter](https://jaxry.github.io/panorama-to-cubemap/)
- [Asteroid texture](https://www.magnific.com/free-photo/photo-stone-texture-pattern_414764463.htm#fromView=keyword&page=1&position=0&uuid=52f9083a-e0f6-40a0-8ed2-776861b7fdff&query=Asteroid+texture)
- [Earth object and texture](https://free3d.com/3d-model/earth-photorealistic-2k-927613.html)
- [Spaceship object and texture](https://free3d.com/3d-model/e-45-aircraft-71823.html)
- Texturing And Modeling. A Procedural Approach (David S. Ebert, F. Kenton Musgrave etc.) (see docs/Texturing And Modeling. A Procedural Approach (David S. Ebert, F. Kenton Musgrave etc.))
- [Triplanar Mapping](https://bgolus.medium.com/normal-mapping-for-a-triplanar-shader-10bf39dca05a)
- [Motion Blur](https://developer.nvidia.com/gpugems/gpugems3/part-iv-image-effects/chapter-27-motion-blur-post-processing-effect)
- [Generate a random uniform direction](https://gwern.net/doc/statistics/probability/1972-marsaglia.pdf)