# 🌿 Garden Scene — OpenGL 2D Animation

A fully interactive, animated 2D garden scene built with **OpenGL (GLUT)** in C++, developed as a Computer Graphics course project. The scene demonstrates core CG algorithms, 2D transformations, and a rich set of animated elements with dynamic weather and day/night transitions.

---

## 🖼️ Scene Overview

The scene renders a 1200×700 window depicting a lush garden environment featuring:

- A dynamic sky with gradient transitions between day and night
- A glowing sun (daytime) and a cratered moon with halo (nighttime)
- Animated clouds, birds, butterflies, a dragonfly, and an airplane
- A pond with swimming fish and water shimmer effects
- Flowers with gentle swaying animation
- Lamp posts with dual glow lights (active at night)
- A fenced garden with trees, a bench, and a cat house
- A **grey-white cat** with a full AI-style state machine

---

## ✨ Features

### 🎨 Core CG Algorithms
| Algorithm | Usage in Scene |
|---|---|
| **DDA Line** | Sun rays, rain streaks |
| **Bresenham Line** | Structural outlines (fence, lamp posts) |
| **Midpoint Circle** | Sun corona and decorative outlines |

### 🔄 2D Transformations
All transformations are implemented manually via OpenGL matrix operations:
- **Translation** — positioning scene objects
- **Rotation** — wing flapping, bird animation
- **Scaling** — resizing elements and reflections
- **Reflection (X/Y axis)** — mirroring the airplane on turnaround
- **Shear** — stylized structural elements

### 🌦️ Weather System
- **Rain Mode** — 200 animated raindrops falling across the scene
- **Storm Mode** — 400 heavy raindrops, strong wind boost to clouds, darkened sky with a purple-grey tint, and storm clouds
- **Lightning** — randomized jagged bolts with glowing halos and branch forks
- All weather effects influence the overall scene lighting and color

### 🌙 Day / Night Cycle
- Smooth `dayFactor` interpolation transitions the sky, ground, and all object colors
- Stars twinkle at night with pulsing glow
- Fireflies appear and float around at night
- Lamp posts activate their dual warm-glow lights after dark

### 🐱 Cat State Machine
The cat has four behavioural states that respond to weather:

```
CAT_SITTING ──(rain/storm)──► CAT_RUNNING_TO_HOUSE
                                        │
                               CAT_IN_HOUSE
                                        │
                          (weather clears)
                                        │
                              CAT_RUNNING_TO_BENCH
                                        │
                               CAT_SITTING
```

- Sits on the garden bench during calm weather
- Runs to its house (outside the fence) when rain or storm begins
- Returns to the bench once the weather clears
- Animated legs, tail wag, and direction-aware facing

### 🐾 Animated Elements
| Element | Animation |
|---|---|
| Clouds | Drift across sky; speed increases during storm |
| Butterfly × 2 | Sinusoidal flight path, wing flapping |
| Dragonfly | Figure-eight hover pattern |
| Birds × 2 | Fly across screen with wing beat |
| Airplane | Bobs gently across the sky; hides during heavy storm |
| Fish × 3 | Swim back and forth in the pond |
| Flowers | Sway with wind; sway amplitude increases in storm |
| Fireflies × 20 | Drift in organic patterns at night |
| Rain / Storm drops | Gravity + wind drift, wrap vertically |
| Lightning | Randomised flash, glow, and branch |

---

## 🎮 Controls

| Key | Action |
|---|---|
| `N` | Toggle **Day / Night** |
| `R` | Toggle **Rain** |
| `T` | Toggle **Storm** (includes rain) |
| `W` | Increase wind speed (faster clouds) |
| `S` | Decrease wind speed |
| `+` / `=` | Increase animation speed |
| `-` / `_` | Decrease animation speed |
| `ESC` | Exit |

---

## 🛠️ Requirements

- **C++ compiler** (GCC / MSVC / Clang)
- **OpenGL**
- **GLUT** (or FreeGLUT)
- Standard C libraries: `math.h`, `stdio.h`, `stdlib.h`, `string.h`, `time.h`

---

## 🚀 Building & Running

### Linux / macOS
```bash
g++ CG_PROJECT_FINAL.cpp -o garden_scene -lGL -lGLU -lglut -lm
./garden_scene
```

### Windows (MinGW)
```bash
g++ CG_PROJECT_FINAL.cpp -o garden_scene.exe -lfreeglut -lopengl32 -lglu32 -lm
garden_scene.exe
```

### Windows (MSVC)
Link against `opengl32.lib`, `glu32.lib`, and `freeglut.lib` in your project settings, then build normally.

---

## 📐 Technical Details

- **Window size:** 1200 × 700 px
- **Coordinate system:** Orthographic 2D — `(-600, 600)` × `(-350, 350)`
- **Rendering:** Double-buffered RGBA with alpha blending enabled
- **Frame rate:** ~60 FPS via `glutTimerFunc` at 16 ms intervals
- **Blend mode:** `GL_SRC_ALPHA / GL_ONE_MINUS_SRC_ALPHA` for all transparency

---

## 📁 File Structure

```
CG_PROJECT_FINAL.cpp   ← Single-file source (all drawing, logic, and animation)
README.md
```

---

## 👨‍💻 Project Context

This project was built as a final submission for a **Computer Graphics** course. It demonstrates practical implementation of:
- Rasterisation algorithms (DDA, Bresenham, Midpoint Circle)
- Affine 2D transformations using OpenGL matrix stack
- Real-time animation via timer callbacks
- Scene composition with layered rendering
- State-machine driven character behaviour