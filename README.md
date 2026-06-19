<p align="center">
  <img src="https://img.shields.io/badge/Language-C-blue?style=for-the-badge&logo=c" />
  <img src="https://img.shields.io/badge/Graphics-OpenGL%20%2B%20GLUT-green?style=for-the-badge&logo=opengl" />
  <img src="https://img.shields.io/badge/IDE-Code%3A%3ABlocks-orange?style=for-the-badge" />
  <img src="https://img.shields.io/badge/Platform-Windows-lightgrey?style=for-the-badge&logo=windows" />
  <img src="https://img.shields.io/badge/License-MIT-yellow?style=for-the-badge" />
</p>

<h1 align="center">🐦 Flappy Bird — OpenGL Edition</h1>

<p align="center">
  <b>A fully playable, visually polished Flappy Bird clone built entirely in C using OpenGL + GLUT.</b><br/>
  Developed as a Computer Graphics Lab Project — no external game engines, no assets, pure code.
</p>

<p align="center">
  <a href="#-features">Features</a> •
  <a href="#-prerequisites">Prerequisites</a> •
  <a href="#-setup--installation">Setup</a> •
  <a href="#-building--running">Build & Run</a> •
  <a href="#-controls">Controls</a> •
  <a href="#-project-structure">Structure</a> •
  <a href="#-code-architecture">Architecture</a> •
  <a href="#-license">License</a>
</p>

---

## 🎮 Demo / Preview

```
┌────────────────────────────────────────────────┐
│  🌤  Sky gradient  ·  Parallax clouds          │
│                                                │
│  🏙  City silhouette with glowing windows      │
│                                                │
│  🐦──────────  [Score: 7]  ──────────          │
│              ┃         ┃                       │
│        ══════╋═════════╋══════  ← Pipes       │
│              ┃  GAP    ┃                       │
│              ┃         ┃                       │
│▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓  ← Ground     │
└────────────────────────────────────────────────┘
```

> **All graphics are drawn in real-time using OpenGL primitives** — no image files, no sprite sheets.

---

## ✨ Features

### 🐦 Bird Mechanics
- Realistic gravity and physics simulation
- Flap impulse on **Space** or **Left Click**
- Smooth tilt interpolation: nose-up on flap, nose-dive on fall
- 3-frame wing animation (up / mid / down) synced to movement

### 🌿 Pipe System
- 3 simultaneous pipe pairs scrolling continuously left
- Randomly generated gap heights every recycle
- Adjustable gap size and spacing constants
- **Progressive difficulty** — pipe speed increases with score

### 💥 Collision Detection
- Circle vs. AABB (Axis-Aligned Bounding Box) collision per pipe
- Ground and ceiling boundary detection
- Slightly forgiving hitbox for fair gameplay

### 🏆 Scoring System
- Score increments when bird successfully passes a pipe pair
- **Session high score** retained in memory
- Large centered score display + top-left best score

### 🖼 Visual Polish
| Effect | Description |
|--------|-------------|
| Sky gradient | Smooth lerp from warm cyan at horizon to deep blue at zenith |
| Parallax clouds | 6 clouds in 2 speed layers, wrapping seamlessly |
| City silhouette | 14 procedural buildings with pseudo-random lit windows |
| Scrolling ground | Animated grass tiles synced to pipe speed |
| Screen shake | 15-frame camera shake on collision |
| Death flash | White flash overlay on bird death |
| Pulsing UI | `sin()`-driven alpha animation on prompts |
| Aspect ratio lock | Pillarbox / letterbox for any window size |

### 🎮 Game States
```
[Title Screen] ──SPACE──► [Playing] ──P──► [Paused] ──P──► [Playing]
                               │
                          (Collision)
                               │
                               ▼
                        [Game Over] ──SPACE/R──► [Playing]
```

---

## 📋 Prerequisites

| Requirement | Details |
|-------------|---------|
| **Operating System** | Windows 7 / 8 / 10 / 11 |
| **IDE** | [Code::Blocks](https://www.codeblocks.org/) with MinGW bundled |
| **Graphics Library** | [FreeGLUT for MinGW](https://www.transmissionzero.co.uk/software/freeglut-devel/) |
| **Compiler** | GCC (MinGW) — ships with Code::Blocks |
| **C Standard** | C99 or later |

---

## 🔧 Setup & Installation

### Step 1 — Install Code::Blocks with MinGW

Download the **"codeblocks-XX.XX-setup.exe with MinGW"** installer from:
> https://www.codeblocks.org/downloads/binaries/

Run the installer with default settings. MinGW (GCC) is bundled automatically.

---

### Step 2 — Download FreeGLUT

Download the **MinGW package** of FreeGLUT from:
> https://www.transmissionzero.co.uk/software/freeglut-devel/

Extract the ZIP. You will see these folders:
```
freeglut/
├── bin/           ← freeglut.dll
├── include/GL/    ← glut.h, freeglut.h, ...
└── lib/           ← libfreeglut.a, libfreeglut_static.a
```

---

### Step 3 — Copy FreeGLUT Files

Replace `C:\Program Files\CodeBlocks\MinGW\` with your actual MinGW path.

| FreeGLUT Source | Copy To |
|-----------------|---------|
| `include\GL\*.h` | `<MinGW>\include\GL\` |
| `lib\libfreeglut.a` | `<MinGW>\lib\` |
| `lib\libfreeglut_static.a` | `<MinGW>\lib\` |
| `bin\freeglut.dll` | Next to your compiled `.exe` |

> **Quick method for the DLL:** Copy `freeglut.dll` into `bin\Debug\` and `bin\Release\` inside the project folder.

---

### Step 4 — Clone This Repository

```bash
git clone https://github.com/labonysur-cloud/flappybird.git
cd flappybird
```

---

## 🚀 Building & Running

### Option A — Code::Blocks (Recommended)

1. Open **`FlappyBird.cbp`** in Code::Blocks
2. Press **F9** → Build and Run

The project file already has the correct linker flags pre-configured:
```
-lopengl32  -lglu32  -lfreeglut  -lm
```

---

### Option B — Manual MinGW Command Line

Open a Command Prompt or PowerShell in the project directory:

```bash
gcc flappy_bird.c -o flappy_bird.exe -lopengl32 -lglu32 -lfreeglut -lm -std=c99
flappy_bird.exe
```

Make sure `freeglut.dll` is in the **same directory** as `flappy_bird.exe`.

---

### Option C — If You Configure Your Own Code::Blocks Project

Go to **Project → Build options → Linker settings → Other linker options** and add:
```
-lopengl32 -lglu32 -lfreeglut -lm
```

---

## 🕹 Controls

| Input | Action |
|-------|--------|
| `SPACE` | Flap / Start game / Restart after death |
| `Left Mouse Button` | Same as SPACE |
| `P` | Pause / Unpause |
| `R` | Restart from Game Over screen |
| `ESC` | Quit |

---

## 📁 Project Structure

```
flappybird/
│
├── flappy_bird.c       # Complete game source (~1100 lines, single file)
├── FlappyBird.cbp      # Code::Blocks project file
├── README.md           # This file
└── LICENSE             # MIT License
```

> **Single-file design** — the entire game fits in one `.c` file for easy submission, review, and presentation.

---

## 🏗 Code Architecture

The code is organized into clearly separated, well-commented logical sections:

### Initialization
| Function | Responsibility |
|----------|---------------|
| `init()` | OpenGL state, projection matrix, seeds RNG, calls sub-inits |
| `initBird()` | Reset bird position, velocity, animation frame |
| `initPipes()` | Spawn 3 pipe pairs at staggered X offsets with random gaps |
| `initClouds()` | Distribute 6 clouds across the sky at random positions |
| `initBuildings()` | Generate 14 deterministic city silhouette buildings |
| `resetGame()` | Full game reset — calls all inits, resets score and speed |

### Rendering Pipeline
```
display()
  ├── drawBackground()        Sky gradient quad
  ├── drawClouds()            Parallax cloud blobs (2 speed layers)
  ├── drawCitySilhouette()    Dark building shapes + window dots
  ├── drawPipes()             3 pipe pairs with cap + highlight/shadow
  ├── drawGround()            Grass + dirt + scrolling tile details
  ├── drawBird()              Body, wing (animated), belly, eye, beak
  ├── drawHUD()               Score (stroked) + high score (bitmap)
  ├── drawTitleScreen()       Logo, bobbing bird, pulsing prompt
  ├── drawGameOverScreen()    Panel, scores, pulsing restart text
  └── drawPauseScreen()       Overlay + resume text
```

### Game Logic
| Function | Responsibility |
|----------|---------------|
| `updateGame()` | Master update dispatcher (calls below per state) |
| `updateBird()` | Apply gravity, clamp velocity, compute tilt, step wing anim |
| `updatePipes()` | Scroll, recycle off-screen pipes, detect score events |
| `updateClouds()` | Scroll + wrap each cloud to the right edge |
| `updateShake()` | Decay screen shake magnitude each frame |
| `checkCollision()` | Circle–AABB test vs. pipes, plus boundary checks |

### Input & Timing
| Function | Responsibility |
|----------|---------------|
| `timerCallback()` | GLUT timer fires every 16ms (~60 FPS), calls update + redisplay |
| `doFlap()` | Unified flap action — triggers start, flap, or restart |
| `keyboardInput()` | GLUT keyboard callback: Space, R, P, ESC |
| `mouseInput()` | GLUT mouse callback: left click → doFlap |
| `reshape()` | Recalculates viewport with pillar/letterbox for any window size |

### Physics & Constants (tunable)

```c
#define GRAVITY         -0.55f   // Downward acceleration per frame
#define FLAP_VEL         10.5f   // Upward burst on flap
#define MAX_FALL_VEL    -14.0f   // Terminal velocity
#define PIPE_GAP        155.0f   // Vertical gap size (increase = easier)
#define PIPE_SPACING    270.0f   // Horizontal gap between pipe pairs
#define PIPE_BASE_SPEED   3.0f   // Starting scroll speed
#define PIPE_SPEED_INC    0.004f // Speed increase per frame
```

---

## 🧩 Key Technical Concepts Used

| Concept | Where Used |
|---------|-----------|
| **Orthographic 2D projection** | `gluOrtho2D(0, 800, 0, 600)` — world coordinate system |
| **Double buffering** | `GLUT_DOUBLE` — eliminates flickering |
| **Timer-based game loop** | `glutTimerFunc(16, callback, 0)` — 60 FPS fixed timestep |
| **Alpha blending** | `glEnable(GL_BLEND)` — cloud opacity, overlays, flash |
| **Matrix transforms** | `glRotatef / glScalef` — bird tilt, cloud ellipses, beak |
| **Parametric circle** | `cos(θ), sin(θ)` — bird body, wing, eyes |
| **Lerp / Clamp** | Smooth tilt transition from current to target angle |
| **Parallax scrolling** | Two cloud layers at different speeds simulate depth |
| **AABB collision** | Fast rectangular hit detection for pipe shafts and caps |

---

## 🔊 Sound Hooks (Extension)

Sound stubs are included as comments at the bottom of `flappy_bird.c`. To add audio using the Windows API:

```c
#include <windows.h>
#include <mmsystem.h>   // add -lwinmm to linker flags

void playSound(int id) {
    const char *files[] = {"flap.wav", "score.wav", "die.wav"};
    PlaySound(files[id], NULL, SND_FILENAME | SND_ASYNC);
}
```

Then uncomment the three `/* playSound(...) */` lines in the source.

---

## 🐛 Troubleshooting

| Problem | Solution |
|---------|----------|
| `fatal error: GL/glut.h` | Copy FreeGLUT headers to `<MinGW>\include\GL\` |
| `undefined reference to glutInit` | Add `-lfreeglut` to linker settings |
| `freeglut.dll is missing` | Copy `freeglut.dll` next to your `.exe` |
| Game opens then immediately closes | Run from terminal to see the error message |
| Very slow / choppy animation | Build in **Release** mode, not Debug |
| Window appears black | Ensure OpenGL drivers are up to date |

---

## 📈 Possible Improvements

- [ ] Add `.wav` sound effects (flap, score, die)
- [ ] Add medal system (bronze / silver / gold) based on score
- [ ] Add scrolling parallax ground tiles at multiple depths
- [ ] Implement a leaderboard saved to a `.txt` file
- [ ] Add pipe color variety every N pipes
- [ ] Add a day/night cycle background effect
- [ ] Port to Linux/macOS with `freeglut` or `glut` package

---

## 👩‍💻 Author

**Labony Sur**  
Computer Science & Engineering  
Computer Graphics Laboratory Project

- GitHub: [@labonysur-cloud](https://github.com/labonysur-cloud)
- Email: labonysur473@gmail.com

---

## 📄 License

This project is licensed under the **MIT License** — see the [LICENSE](LICENSE) file for details.

```
MIT License

Copyright (c) 2026 Labony Sur

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
```

---

<p align="center">
  Made with ❤️ using C, OpenGL, and GLUT &nbsp;·&nbsp; Computer Graphics Lab 2026
</p>
