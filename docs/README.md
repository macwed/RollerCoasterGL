**Status:** work in progress (WIP), first prototype in progress.

# 🎢 RollerCoasterGL

> **OpenGL 4.6  •  C++20  •  GLFW + GLAD + GLM  •  Linux / CLion**  
> Student roller‑coaster ride – computer‑graphics course project.

![WIP screenshot](../docs/screenshot_wip.gif)

## Table of Contents
- [Requirements](#requirements)
- [Building](#building)
- [Run & Controls](#run--controls)
- [Directory Layout](#directory-layout)
- [Roadmap / TODO](#roadmap--todo)
- [License](#license)

---

## Requirements

| Tool / Library  | Tested Version    | Notes                                                          |
|-----------------|-------------------|----------------------------------------------------------------|
| **CMake**       | ≥ 3.27            | CLion bundles a recent CMake; otherwise sudo apt install cmake |
| **GCC / Clang** | GCC 13 / Clang 17 | Must support **C++20**                                         |
| **vcpkg**       | commit HEAD       | For dependency management                                      |
| **GLFW 3**      | 3.3.x             | From vcpkg                                                     |
| **GLAD**        | 0.1.x             | Loader generated for OpenGL 4.6 core                           |
| **GLM**         | 0.9.9             | Header‑only math library                                       |
| **stb**         | latest            | stb_image.h for texture loading                                |
| **Assimp**      | 5.x               | Import external 3D models                                      |

---

## Building

### Quick start (Linux + CLion 2023)

1. **Clone with submodules / manifest dependencies**

   
bash
   git clone --recurse-submodules git@github.com:macwed/RollerCoasterGL.git
   cd RollerCoasterGL


2. **Install third-party libraries via vcpkg**
   
bash
   git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
   ~/vcpkg/bootstrap-vcpkg.sh
   ~/vcpkg/vcpkg install

3. **Configure CLion**
   
text
   File → Settings → Build, Execution, Deployment → CMake options


4. **Build & Run**
   
(Shift + F10) – you should see a black GLFW window.

---

## Directory Layout
<details> <summary>click to expand</summary>

   
text

    RollerCoasterGL/
    ├─ assets/          # textures, shaders, imported models
    │   ├─ shaders/
    │   └─ textures/
    ├─ docs/            # design docs, GIFs, architecture diagrams
    ├─ src/
    │   ├─ camera/      # camera
    │   ├─ core/        # window setup, timing, input
    │   ├─ gfx/         # renderer, resources, shaders
    │   │   ├─ geometry # meshes
    │   │   ├─ gl
    │   │   └─ render
    │   ├─ math/        # Catmull‑Rom spline, array helper
    │   ├─ physics/     # kinematics
    │   ├─ terrain/     # terrain, heightmaps, simplex noise
    │   └─ track/
    ├─ tests/           # unit tests (Catch2 / GoogleTest)
    ├─ thirdparty
    │   ├─ glad/
    │   └─ imgui/
    ├─ CMakeLists.txt
    └─ vcpkg.json


</details>

---

## Run & Controls

   <!-- TO DO -->

---

## Roadmap / TODO

<details> <summary>progress tracker</summary> 

- [x] ✅ Hello Triangle

- [ ] 🚂 SplineTrack

- [ ] 🌄 Texturing – at least two textures

- [ ] 💡 Lighting – directional + point light

- [ ] 🐦 Instanced birds

- [ ] 📊 HUD (ImGui)

- [ ] 🎥 Demo video

</details>

---

## License

MIT
