# 🪐 Hierarchical Solar System Simulator

> **Computer Graphics Capstone Project**
> SITE — 3rd Year | OpenGL + FreeGLUT | C++17

A real-time, interactive solar system simulation built entirely from OpenGL
primitives. Every shape, orbit, glow, and ring is generated programmatically
— no external textures, images, or 3D models are used anywhere in the project.

---

## 📋 Table of Contents

- [Project Overview](#project-overview)
- [Features](#features)
- [Screenshots](#screenshots)
- [Project Structure](#project-structure)
- [Build Instructions](#build-instructions)
- [Controls](#controls)
- [Solar System Data](#solar-system-data)
- [Graphics Concepts Demonstrated](#graphics-concepts-demonstrated)
- [Hierarchical Transformation Explanation](#hierarchical-transformation-explanation)
- [glPushMatrix and glPopMatrix](#glpushmatrix-and-glpopmatrix)
- [Elliptical Orbit Calculation](#elliptical-orbit-calculation)
- [File Architecture](#file-architecture)
- [Known Limitations](#known-limitations)

---

## Project Overview

This project implements a hierarchical solar system simulator using legacy
OpenGL (immediate mode) and FreeGLUT. The simulation demonstrates the core
principles of hierarchical modeling in computer graphics — specifically, how
nested coordinate frames allow objects to move relative to their parents
rather than relative to a fixed world origin.

The sun sits at the world origin. Planets orbit the sun along elliptical
paths. Moons orbit their parent planets. Each level of the hierarchy is
expressed entirely through the OpenGL matrix stack — no manual world-space
position calculations are needed for child objects.

---

## Features

### Solar System
- ☀️  1 Sun with animated glow corona effect
- 🪐  8 Planets — Mercury through Neptune
- 🌙  7 Moons across 6 planets (Earth, Mars ×2, Jupiter, Saturn, Uranus, Neptune)
- 💍  Saturn's ring system (filled annulus with alpha transparency)
- 🔴  Scientifically inspired details:
  - Venus and Uranus have retrograde axial rotation
  - Triton (Neptune's moon) has a retrograde orbit
  - Phobos orbits Mars faster than Mars rotates
  - Mercury has a slow axial spin matching its real 59-day rotation

### Rendering
- Elliptical orbit paths for all planets and moons
- Pseudo-3D sphere rendering using stacked latitude ellipses
- Starfield background (300 procedurally placed stars, fixed seed)
- Sun corona glow (layered concentric circles with decreasing alpha)
- Planet name labels (bitmap text, always upright)
- Selection highlight ring with pulsing animation

### Interactivity
- Pause / Resume simulation
- Speed up, slow down, and reverse time
- Reset speed to default
- Zoom in / out (keyboard + mouse wheel + arrow keys)
- Select any planet by keyboard (1–8)
- Scale selected planet size up or down
- On-screen HUD with live status display

---

## Screenshots

```
[ Replace this section with actual screenshots after running the project ]

Suggested screenshots:
  1. Full solar system view at default zoom
  2. Zoomed in on Saturn showing ring system
  3. Earth-Moon system with selection highlight active
  4. Simulation running in reverse (REVERSED label visible in HUD)
```

---

## Project Structure

```
SolarSystem/
│
├── main.cpp          Application entry point, GLUT init, solar system
│                     construction, callback registration, main loop
│
├── planet.h          Planet class declaration — all orbital parameters,
│                     visual properties, moon hierarchy, ring config
│
├── planet.cpp        Planet class implementation — update(), addMoon(),
│                     setupRings(), scaleSize(), getOrbitX/Y()
│
├── renderer.h        SimState struct declaration, renderer function
│                     declarations
│
├── renderer.cpp      All OpenGL draw calls — sun, planets, moons, rings,
│                     orbit paths, starfield, HUD overlay
│
├── controls.h        Input handler function declarations
│
├── controls.cpp      Keyboard and mouse input handling — all SimState
│                     mutations in response to user input
│
├── math_utils.h      Vec2 struct, constants, and all utility function
│                     declarations
│
├── math_utils.cpp    Vec2 methods, angle conversion, ellipse math,
│                     interpolation, OpenGL primitive draw helpers,
│                     starfield generation
│
└── README.md         This file
```

---

## Build Instructions

### Prerequisites

| Platform | Requirement |
|----------|-------------|
| Linux    | `g++`, `freeglut3-dev`, `libgl1-mesa-dev` |
| macOS    | Xcode Command Line Tools (OpenGL + GLUT frameworks included) |
| Windows  | MinGW or MSVC, FreeGLUT binaries, OpenGL headers |

---

### Linux (Ubuntu / Debian)

**Install dependencies:**
```bash
sudo apt update
sudo apt install build-essential freeglut3-dev libgl1-mesa-dev libglu1-mesa-dev
```

**Compile:**
```bash
g++ -std=c++17 \
    main.cpp planet.cpp renderer.cpp controls.cpp math_utils.cpp \
    -lGL -lGLU -lglut \
    -o SolarSystem
```

**Run:**
```bash
./SolarSystem
```

---

### macOS

**Compile:**
```bash
g++ -std=c++17 \
    main.cpp planet.cpp renderer.cpp controls.cpp math_utils.cpp \
    -framework OpenGL -framework GLUT \
    -o SolarSystem
```

> **Note:** On macOS 10.14+ you may see a deprecation warning about OpenGL.
> This is expected — the legacy API still works correctly. Suppress with:
> `-Wno-deprecated-declarations`

**Run:**
```bash
./SolarSystem
```

---

### Windows (MinGW + FreeGLUT)

1. Download FreeGLUT binaries from https://www.transmissionzero.co.uk/software/freeglut-devel/
2. Place `freeglut.h`, `libfreeglut.a`, and `freeglut.dll` in your project directory
3. Compile:

```cmd
g++ -std=c++17 ^
    main.cpp planet.cpp renderer.cpp controls.cpp math_utils.cpp ^
    -I. -L. -lfreeglut -lopengl32 -lglu32 ^
    -o SolarSystem.exe
```

4. Place `freeglut.dll` in the same directory as `SolarSystem.exe`
5. Run: `SolarSystem.exe`

---

### Makefile (Linux / macOS)

Save as `Makefile` in the project directory:

```makefile
CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
SRCS     = main.cpp planet.cpp renderer.cpp controls.cpp math_utils.cpp
TARGET   = SolarSystem

# Linux flags
LDFLAGS_LINUX  = -lGL -lGLU -lglut

# macOS flags
LDFLAGS_MACOS  = -framework OpenGL -framework GLUT

UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
    LDFLAGS = $(LDFLAGS_MACOS) -Wno-deprecated-declarations
else
    LDFLAGS = $(LDFLAGS_LINUX)
endif

all:
	$(CXX) $(CXXFLAGS) $(SRCS) $(LDFLAGS) -o $(TARGET)

clean:
	rm -f $(TARGET)

run: all
	./$(TARGET)
```

**Usage:**
```bash
make        # build
make run    # build and run
make clean  # remove binary
```

---

## Controls

### Simulation Controls

| Key | Action |
|-----|--------|
| `SPACE` | Pause / Resume simulation |
| `+` or `=` | Increase simulation speed (×1.5 per press) |
| `-` | Decrease simulation speed (÷1.5 per press) |
| `R` | Reverse time direction (preserves speed magnitude) |
| `T` | Reset speed to 1.0× forward |

### Camera Controls

| Key / Input | Action |
|-------------|--------|
| `Z` | Zoom in |
| `X` | Zoom out |
| `↑` Arrow Up | Zoom in |
| `↓` Arrow Down | Zoom out |
| `→` Arrow Right | Speed up |
| `←` Arrow Left | Slow down |
| Mouse Wheel Up | Zoom in |
| Mouse Wheel Down | Zoom out |

### Planet Selection & Scaling

| Key | Action |
|-----|--------|
| `1` | Select / deselect Mercury |
| `2` | Select / deselect Venus |
| `3` | Select / deselect Earth |
| `4` | Select / deselect Mars |
| `5` | Select / deselect Jupiter |
| `6` | Select / deselect Saturn |
| `7` | Select / deselect Uranus |
| `8` | Select / deselect Neptune |
| `[` | Decrease selected planet size (min 0.2× original) |
| `]` | Increase selected planet size (max 5.0× original) |

### Application

| Key | Action |
|-----|--------|
| `ESC` | Exit application |

---

## Solar System Data

All orbital parameters are scaled for visual clarity rather than physical
accuracy. Speed ratios are loosely inspired by real orbital periods.

| # | Planet | Orbit Radii (X/Y) | Orbit Speed | Self-Rotation | Moons |
|---|--------|-------------------|-------------|---------------|-------|
| 1 | Mercury | 22 / 17 | 47°/s | 15°/s | — |
| 2 | Venus | 32 / 30 | 35°/s | −8°/s ⟳ | — |
| 3 | Earth | 44 / 42 | 29°/s | 25°/s | Moon |
| 4 | Mars | 58 / 54 | 24°/s | 24.5°/s | Phobos, Deimos |
| 5 | Jupiter | 78 / 75 | 13°/s | 60°/s | Ganymede |
| 6 | Saturn | 100 / 97 | 9.5°/s | 55°/s | Titan |
| 7 | Uranus | 122 / 120 | 6.8°/s | −42°/s ⟳ | Miranda |
| 8 | Neptune | 145 / 143 | 5.4°/s | 50°/s | Triton ⟳ |

> ⟳ = retrograde (real astronomical property preserved in the simulation)

---

## Graphics Concepts Demonstrated

### 1. Hierarchical Modeling
The solar system is a tree: Sun → Planets → Moons. Each node's transform is
expressed relative to its parent. The renderer recurses through this tree
while inside the parent's matrix push/pop block, so child positions are
computed automatically by matrix multiplication.

### 2. Matrix Stack Operations
`glPushMatrix()` and `glPopMatrix()` save and restore coordinate frames.
Each planet pushes a frame, applies its orbit and rotation, draws itself,
processes its moons (still inside the pushed frame), then pops back to world
space. This is the defining technique of the project.

### 3. Affine Transformations
Every object position is computed by composing:
- `glRotatef` — orbital revolution around the parent
- `glTranslatef` — radial offset from the parent centre
- `glRotatef` — axial self-rotation

### 4. Elliptical Orbits (Parametric Equations)
Orbit paths use the parametric ellipse:
```
x(θ) = radiusX × cos(θ)
y(θ) = radiusY × sin(θ)
```
Setting radiusX ≠ radiusY produces a true ellipse. The angle θ advances
each frame by `orbitSpeed × dt`.

### 5. Frame-Rate Independent Animation
All angle updates use:
```
angle += speed × dt
```
where `dt = frameTime × timeScale`. The simulation runs at the same apparent
speed regardless of frame rate or time scale value.

### 6. Alpha Blending
Used for: sun corona glow, Saturn's ring transparency, faint orbit paths.
Blending equation: `result = src.rgb × src.a + dst.rgb × (1 − src.a)`

### 7. Orthographic Projection
`glOrtho` defines a rectangular world-space viewing volume. The horizontal
extent is fixed; the vertical extent scales with the window aspect ratio
so the scene is never stretched or distorted on resize.

### 8. HUD Projection Switch
The HUD switches to pixel-space orthographic projection for text rendering,
then restores the world-space projection — all via `glPushMatrix` /
`glPopMatrix` on the `GL_PROJECTION` matrix stack.

### 9. Pseudo-3D Sphere Rendering
Spheres are approximated by stacked horizontal ellipses (latitude bands) plus
a vertical meridian circle — giving a globe appearance using only 2D primitives
and the immediate-mode GL_LINE_LOOP primitive.

### 10. Programmatic Primitive Construction
Every visible shape — circles, ellipses, filled disks, annuli, spheres,
stars — is generated at runtime from trigonometric loops. No external assets
of any kind are loaded or referenced.

---

## Hierarchical Transformation Explanation

The core of this project is how a moon knows where to draw itself without
ever being told its world-space position. The answer is the matrix stack.

Consider Earth and its Moon. At render time:

```
World origin (0, 0) — the Sun

Step 1: glPushMatrix()
        → Save the world origin on the stack

Step 2: glTranslatef(earth.getOrbitX(), earth.getOrbitY(), 0)
        → Move the origin to where Earth currently is on its ellipse
        → Stack now has: [world origin] [earth position]

Step 3: drawCelestialBody(earth)
        → Earth is drawn at (0,0) in the current frame = Earth's position ✓

Step 4: (still inside Earth's push/pop — origin IS at Earth's position)
        glPushMatrix()
        → Save Earth's position

Step 5: glRotatef(moon.orbitAngle, 0, 0, 1)
        → Rotate the frame around Earth (not the Sun!)

Step 6: glTranslatef(moon.orbitRadiusX, 0, 0)
        → Move out from Earth by the moon's orbital radius

Step 7: drawCelestialBody(moon)
        → Moon is drawn at its correct world position
        → OpenGL multiplied ALL the matrices together automatically ✓

Step 8: glPopMatrix()  → back to Earth's position
Step 9: glPopMatrix()  → back to world origin
```

The Moon never "knows" where the Sun is. It only knows it is
`orbitRadiusX` units away from Earth at angle `orbitAngle`. The matrix
stack handles the rest. This is hierarchical modeling.

---

## glPushMatrix and glPopMatrix

OpenGL maintains a stack of 4×4 transformation matrices. Think of it as a
stack of coordinate frames, where each frame describes "where is my origin
and which way am I pointing?"

```
glPushMatrix()
```
Duplicates the top matrix and pushes the copy onto the stack. Any
transforms applied after this call affect only the copy — the original
is safely preserved underneath.

```
glPopMatrix()
```
Discards the top matrix and restores the one below it. All transforms
applied since the matching `glPushMatrix` are undone.

**Why this matters for the solar system:**

Without push/pop, translating to Earth's position would permanently move
the origin there. Mars would then be drawn relative to Earth instead of
the Sun. Every planet would need to know the absolute world positions of
all previously drawn planets — defeating the purpose of hierarchical design.

With push/pop, each planet's translation is isolated. After drawing Earth
and its moon, we pop back to the world origin, and Mars starts fresh from
the Sun's position. The code is clean, modular, and each planet is
completely independent of the others.

**Stack depth in this project:**

```
[World origin]                    ← base (set by setupCamera)
  [Planet position]               ← pushed in drawPlanetHierarchy
    [Moon position]               ← pushed in moon loop
```

Maximum stack depth: 3 levels. OpenGL guarantees at least 32 levels,
so we are well within limits.

---

## Elliptical Orbit Calculation

A circle is a special case of an ellipse where both radii are equal.
By using two independent radii, we get orbits that look more natural
and demonstrate a richer mathematical model.

**Parametric ellipse equations:**
```
x(θ) = radiusX × cos(θ)
y(θ) = radiusY × sin(θ)
```

At `θ = 0°`:   planet is at the rightmost point  (radiusX, 0)
At `θ = 90°`:  planet is at the top              (0, radiusY)
At `θ = 180°`: planet is at the leftmost point   (−radiusX, 0)
At `θ = 270°`: planet is at the bottom           (0, −radiusY)

**Advancing the angle each frame:**
```cpp
orbitAngle += orbitSpeed * dt;
```

**Computing position from angle (planet.cpp):**
```cpp
float Planet::getOrbitX() const {
    return orbitRadiusX * std::cos(degreesToRadians(orbitAngle));
}
float Planet::getOrbitY() const {
    return orbitRadiusY * std::sin(degreesToRadians(orbitAngle));
}
```

**Drawing the orbit path ellipse (math_utils.cpp):**
```cpp
void drawEllipse(float radiusX, float radiusY, int segments) {
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < segments; ++i) {
        float angle = (i / (float)segments) * 2.0f * PI;
        glVertex2f(radiusX * cos(angle), radiusY * sin(angle));
    }
    glEnd();
}
```

The orbit path and the planet's position use exactly the same formula —
the planet always sits precisely on its visible orbit path.

---

## File Architecture

```
Dependency graph (→ means "depends on"):

main.cpp
  → planet.h / planet.cpp
  → renderer.h / renderer.cpp
  → controls.h / controls.cpp
  → math_utils.h / math_utils.cpp

renderer.cpp
  → planet.h
  → math_utils.h

controls.cpp
  → renderer.h  (for SimState)
  → planet.h
  → math_utils.h  (for clampf)

planet.cpp
  → planet.h
  → math_utils.h  (for degreesToRadians)

math_utils.cpp
  → math_utils.h
  → OpenGL / GLUT headers only
```

`math_utils` has no project dependencies — it sits at the bottom of the
dependency tree. Every other file flows upward toward `main.cpp`.

---

## Known Limitations

- **Legacy OpenGL only.** The project uses the immediate-mode API
  (`glBegin`/`glEnd`). This is intentional for a graphics fundamentals
  course but would be replaced with VAOs/VBOs in a modern OpenGL project.

- **No lighting model.** Shading is flat colour only. A Phong lighting
  model would require surface normals and a true 3D sphere mesh.

- **2D simulation.** All orbits lie in the XY plane. Real orbital
  inclinations are not modelled.

- **Approximate ellipses.** Orbital eccentricities are chosen for visual
  clarity, not physical accuracy. Kepler's laws (variable orbital speed
  near perihelion) are not implemented — angular speed is constant.

- **GLUT scroll wheel.** Mouse wheel zoom uses FreeGLUT's button 3/4
  encoding. On some platforms or GLUT implementations the wheel may not
  work; use `Z`/`X` keyboard keys instead.

---

*Hierarchical Solar System Simulator — Computer Graphics Capstone*
*SITE 3rd Year | Built with OpenGL + FreeGLUT | C++17*
