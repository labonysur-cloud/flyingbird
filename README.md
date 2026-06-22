<p align="center">
  <img src="https://img.shields.io/badge/Language-C99-blue?style=for-the-badge&logo=c" />
  <img src="https://img.shields.io/badge/Graphics-OpenGL%20%2B%20FreeGLUT-green?style=for-the-badge" />
  <img src="https://img.shields.io/badge/Platform-Windows-lightgrey?style=for-the-badge&logo=windows" />
  <img src="https://img.shields.io/badge/IDE-Code%3A%3ABlocks-orange?style=for-the-badge" />
  <img src="https://img.shields.io/badge/Algorithms-DDA%20%7C%20Midpoint%20%7C%20Bresenham-purple?style=for-the-badge" />
  <img src="https://img.shields.io/badge/License-MIT-yellow?style=for-the-badge" />
</p>

<h1 align="center">🐦 Flappy Bird — OpenGL Computer Graphics Project</h1>

<p align="center">
  <b>A fully playable, visually rich Flappy Bird clone built entirely in C using OpenGL and FreeGLUT.</b><br/>
  Submitted as a <b>Computer Graphics Lab Project</b> demonstrating mastery of graphics algorithms,<br/>
  2D transformations, primitives, animation, and real-time rendering — no game engines, no image assets, pure OpenGL code.
</p>

<p align="center">
  <a href="#-project-overview">Overview</a> &nbsp;|&nbsp;
  <a href="#-graphics-algorithms-implemented">Algorithms</a> &nbsp;|&nbsp;
  <a href="#-2d-transformations">Transformations</a> &nbsp;|&nbsp;
  <a href="#-primitives-used">Primitives</a> &nbsp;|&nbsp;
  <a href="#-ep-mapping">EP Mapping</a> &nbsp;|&nbsp;
  <a href="#-features">Features</a> &nbsp;|&nbsp;
  <a href="#-setup--build">Build</a> &nbsp;|&nbsp;
  <a href="#-controls">Controls</a>
</p>

---

## 📌 Project Overview

This project is a complete **Flappy Bird clone** written from scratch in **C99** using **OpenGL** and **FreeGLUT**. Every single visual element — the bird, pipes, clouds, sky, city skyline, rain, lightning, stars, sun, moon, and all UI — is drawn programmatically using OpenGL drawing primitives. There are zero image files and zero external assets of any kind.

The project was developed specifically to satisfy the **Computer Graphics Lab Project rubric** (40 marks), and goes beyond the minimum requirements by implementing **three** graphics line/circle algorithms, all **four** major 2D transformation types, an advanced **particle system**, a **reflection** effect, and **four dynamic weather environments**.

### What makes this project stand out

| Requirement | Minimum | This Project |
|-------------|---------|-------------|
| Graphics Algorithms | 2 | **3** (DDA + Midpoint Circle + Bresenham) |
| Transformations | Translation + one more | **All 4** (Translation, Rotation, Scaling, Reflection) |
| Animated Objects | 1 | **7+** (bird, pipes, clouds, rain, sun rays, particles, lightning) |
| Primitives | Points, Lines, Polygons, Circles | **All** — used meaningfully in every scene layer |
| Theme | Any real-world theme | **Interactive game** — the most complex category |

---

## 🖼️ Screenshots

<p align="center">
  <img src="asset/image/day.png" width="48%" title="Day Mode" />
  <img src="asset/image/dawn.png" width="48%" title="Sunny / Golden Hour Mode" />
</p>
<p align="center">
  <img src="asset/image/rainy.png" width="48%" title="Rain Mode with Lightning" />
  <img src="asset/image/night.png" width="48%" title="Night Mode with Stars and Moon" />
</p>

---

## 🔬 Graphics Algorithms Implemented

This is the most important technical section. I manually implemented **three rasterisation algorithms** from scratch — no OpenGL line-drawing shortcuts were used for these.

---

### Algorithm 1 — DDA Line (Digital Differential Analyser)

**What it is:**
The DDA algorithm rasterises a straight line between two points by computing increments along both axes. It determines the number of steps as `max(|dx|, |dy|)`, then increments `x` and `y` by `dx/steps` and `dy/steps` each step, guaranteeing exactly one pixel plotted per step.

**Mathematical foundation:**
```
steps = max(|x2−x1|, |y2−y1|)
xInc  = (x2−x1) / steps
yInc  = (y2−y1) / steps

for i in 0..steps:
    plot(x, y)
    x += xInc
    y += yInc
```

**My implementation:**
```c
static void ddaLine(float x1, float y1, float x2, float y2) {
    float dx    = x2 - x1;
    float dy    = y2 - y1;
    float steps = fabsf(dx) > fabsf(dy) ? fabsf(dx) : fabsf(dy);
    if (steps < 1.0f) steps = 1.0f;
    float xInc  = dx / steps;
    float yInc  = dy / steps;
    float x = x1, y = y1;
    glBegin(GL_POINTS);
    for (int i = 0; i <= (int)steps; i++) {
        glVertex2f(x, y);
        x += xInc;
        y += yInc;
    }
    glEnd();
}
```

**Where and why I used it:**
I used DDA to draw the **outline border of every pipe** (both body and cap). Each pipe has four edges drawn as four DDA line calls via `ddaOutlineRect()`. This is visible every single frame during gameplay — every green pipe border you see is rasterised by my DDA implementation using `GL_POINTS`.

I chose DDA for pipe outlines because:
1. Pipes have edges at all angles (the cap extends wider than the body, creating non-trivial corners)
2. DDA handles arbitrary slopes cleanly with floating-point increments
3. It runs every frame, making it a genuinely active part of the rendering pipeline

---

### Algorithm 2 — Midpoint Circle (Bresenham's Circle Algorithm)

**What it is:**
The Midpoint Circle algorithm plots a circle of radius `r` by starting at `(0, r)` and using an integer decision variable `d = 1 − r` to choose between moving East `(x++)` or South-East `(x++, y--)` at each step. It exploits 8-fold symmetry so only one octant needs computation — all 8 symmetric points are plotted simultaneously.

**Mathematical foundation:**
```
d = 1 − r
while x ≤ y:
    plot 8 symmetric points
    if d < 0:
        d += 2x + 3        // midpoint is inside circle → move East
    else:
        d += 2(x−y) + 5   // midpoint is outside circle → move South-East
        y−−
    x++
```

**My implementation:**
```c
static void midpointCircle(float cx, float cy, int r) {
    int x = 0, y = r;
    int d = 1 - r;
    glBegin(GL_POINTS);
    while (x <= y) {
        glVertex2f(cx+x, cy+y);  glVertex2f(cx-x, cy+y);
        glVertex2f(cx+x, cy-y);  glVertex2f(cx-x, cy-y);
        glVertex2f(cx+y, cy+x);  glVertex2f(cx-y, cy+x);
        glVertex2f(cx+y, cy-x);  glVertex2f(cx-y, cy-x);
        if (d < 0)
            d += 2 * x + 3;
        else { d += 2 * (x - y) + 5; y--; }
        x++;
    }
    glEnd();
}
```

**Where and why I used it:**
I used Midpoint Circle to draw the **bird's circular body outline** — the dark orange ring that frames the bird every frame. I also reuse the same function inside `drawBirdReflection()` to draw the reflection's outline.

I chose Midpoint Circle for the bird because:
1. The bird's body is a circle — this is the most natural application
2. The algorithm uses only integer arithmetic inside the decision logic, making it efficient
3. It runs every frame in a transformed coordinate space (after `glRotatef` for tilt), demonstrating that algorithm output integrates correctly with OpenGL's matrix pipeline

---

### Algorithm 3 — Bresenham Line (Integer Error-Accumulation)

**What it is:**
Bresenham's line algorithm avoids floating-point arithmetic entirely by maintaining an integer error term `err = dx − dy`. At each step it advances in the major axis and conditionally steps in the minor axis when the accumulated error exceeds zero.

**Mathematical foundation:**
```
err = dx − dy
loop:
    plot(x1, y1)
    e2 = 2 × err
    if e2 > −dy: err −= dy;  x1 += sx
    if e2 <  dx: err += dx;  y1 += sy
```

**My implementation:**
```c
static void bresenhamLine(int x1, int y1, int x2, int y2) {
    int dx = abs(x2-x1), dy = abs(y2-y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    glBegin(GL_POINTS);
    for (;;) {
        glVertex2i(x1, y1);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 <  dx) { err += dx; y1 += sy; }
    }
    glEnd();
}
```

**Where and why I used it:**
I used Bresenham to draw the **lightning bolt** in Rain weather mode. Each bolt is made of 8 diagonal segments connecting points at different angles — exactly the kind of arbitrary-slope lines where Bresenham excels. Two passes are drawn: a thick outer glow pass (`glPointSize(6.5)`) and a thin bright inner core pass (`glPointSize(2.5)`), creating the classic jagged lightning appearance.

I chose Bresenham for lightning because:
1. Lightning segments have diagonal, non-trivial slopes — a perfect showcase for a line algorithm
2. The integer-only arithmetic makes it the fastest of the three algorithms
3. Drawing it twice (glow + core) using `GL_POINTS` naturally produces a thick glowing effect

---

### Algorithm Comparison Summary

| Property | DDA | Midpoint Circle | Bresenham |
|----------|-----|-----------------|-----------|
| Arithmetic | Floating-point | Integer | Integer |
| Draws | Straight lines | Circles | Straight lines |
| Applied to | Pipe outlines | Bird body outline | Lightning bolt |
| Visible every frame? | Yes (all states) | Yes (all states) | Rain mode only |
| GL Primitive used | `GL_POINTS` | `GL_POINTS` | `GL_POINTS` |

---

## 🔄 2D Transformations

I applied all four major 2D transformation types using OpenGL's matrix stack (`glPushMatrix` / `glPopMatrix`).

---

### 1. Translation

**Definition:** Moves an object from one position to another by adding offset values to its coordinates.
```
x' = x + tx
y' = y + ty
```

**Where I used it:**
- **Bird vertical movement** — `glTranslatef(BIRD_X, g_bird.y, 0)` positions the bird at its current Y position every frame
- **Pipe scrolling** — each pipe's X position decreases by `pipeSpeed` per frame, translating the entire set leftward
- **Cloud parallax** — each cloud translates at a different speed depending on its layer (0.35 or 0.80 px/frame)
- **Reflection positioning** — `glTranslatef(BIRD_X, reflY, 0)` places the mirrored bird at the correct reflected Y

---

### 2. Rotation

**Definition:** Rotates an object about a pivot point by angle θ.
```
x' = x·cos(θ) − y·sin(θ)
y' = x·sin(θ) + y·cos(θ)
```

**Where I used it:**
- **Bird tilt** — `glRotatef(g_bird.angle, 0, 0, 1)` rotates the bird up to +25° on flap and down to −55° in freefall. The tilt angle is smoothly interpolated toward its target each frame using `lerp`
- **Sun ray spin** — each of the 14 sun rays rotates continuously: `a = PI2*i/14 + g_frame*0.008f`
- **Title bird sway** — `g_bird.angle = 5 * sin(g_frame * 0.05)` makes the title bird rock gently

---

### 3. Scaling

**Definition:** Enlarges or shrinks an object by multiplying its coordinates by scale factors.
```
x' = x · sx
y' = y · sy
```

**Where I used it:**
- **Title bird pulse** — `glScalef(titleScale, titleScale, 1)` where `titleScale = 1 + 0.12·sin(frame·0.07)` makes the bird breathe in and out on the title screen
- **Ellipse drawing** — `glScalef(rx, ry, 1)` inside `fillEllipse()` draws all clouds, wing shapes, belly highlights, and reflection distortions
- **Reflection Y-scale** — `glScalef(1, -1, 1)` is used as part of the reflection transform (see below)

```c
/* Title bird scaling — explicit scaling transform */
float titleScale = 1.0f + 0.12f * sinf(g_frame * 0.07f);
glPushMatrix();
    glTranslatef(BIRD_X, tY - 22.f, 0.f);
    glScalef(titleScale, titleScale, 1.0f);   // ← uniform scale
    glTranslatef(-BIRD_X, -(tY - 22.f), 0.f);
    drawBird();
glPopMatrix();
```

---

### 4. Reflection (Y-axis Mirror)

**Definition:** Reflects an object about a horizontal axis at height `grassY`.
```
y' = 2 · grassY − y     (reflection about the line y = grassY)
```

**Where I used it:**
I implemented a **bird puddle reflection** that appears in the ground strip when the bird flies close to the ground. The reflection is drawn as a faded, semi-transparent copy of the bird, flipped upside-down — like a mirror image in a puddle.

**Implementation:**
```c
static void drawBirdReflection(void) {
    float grassY = GROUND_Y + GROUND_H * GRASS_H_RATIO;  // reflection axis
    float reflY  = 2.f * grassY - g_bird.y;              // reflected Y coord

    glPushMatrix();
        glTranslatef(BIRD_X, reflY, 0.f);    // Step 1: move to reflected position
        glRotatef(-g_bird.angle, 0,0,1);     // Step 2: flip rotation sign
        glScalef(1.0f, -1.0f, 1.0f);        // Step 3: mirror about Y axis
        col4(255,195,50, 45);
        fillCircle(0, 0, BIRD_RADIUS, 20);   // faded body
        midpointCircle(0, 0, BIRD_RADIUS);   // faded outline
    glPopMatrix();
}
```

### Transformation Summary

| Transform | OpenGL Call | Applied To | Effect |
|-----------|-------------|------------|--------|
| Translation | `glTranslatef(tx, ty, 0)` | Bird, pipes, clouds | Movement and positioning |
| Rotation | `glRotatef(angle, 0, 0, 1)` | Bird tilt, sun rays | Dynamic tilt and spin |
| Scaling | `glScalef(sx, sy, 1)` | Title bird, ellipses | Pulse animation and shape |
| Reflection | `glScalef(1, -1, 1)` + translate | Bird ground reflection | Mirror/puddle illusion |

---

## 🔷 Primitives Used

All four required OpenGL primitive types are used meaningfully throughout the scene.

| Primitive | OpenGL Constant | Where Used |
|-----------|-----------------|------------|
| **Points** | `GL_POINTS` | DDA pipe outlines, Midpoint Circle bird outline, Bresenham lightning, score particle sparkles, night stars |
| **Lines** | `GL_LINES` | Sun rays, close-button X cross, rain drop streaks (`GL_LINES`), star cross-arms |
| **Polygons / Quads** | `GL_QUADS`, `GL_TRIANGLES` | Sky background, pipe bodies and caps, ground layers, buildings, bird beak |
| **Circles** | `GL_TRIANGLE_FAN` | Bird body fill, sun body, moon body, cloud ellipse blobs, pipe cap corners via `fillRoundRect` |

> Every primitive listed above is active in at least one visible game state and contributes meaningfully to the visual output — not just placed as a token demonstration.

---

## 🎮 Features

### Core Gameplay
- **Gravity physics** — constant downward acceleration with terminal velocity cap
- **Flap mechanic** — upward velocity impulse on Space / left click
- **Smooth bird tilt** — angle linearly interpolates toward target (`lerp`) every frame
- **3-frame wing animation** — up / mid / down cycling at configurable rate
- **Progressive difficulty** — pipe speed increases `+0.002 px/frame` indefinitely

### Pipe System
- 3 pipe pairs simultaneously on screen
- Random gap centre height on each recycle
- Pipe cap extends wider than body (capped end detail)
- DDA-drawn dark outline on body and cap edges

### Collision & Scoring
- Circle vs. AABB collision test (slightly shrunk hitbox for fairness)
- Ground and ceiling boundary checks
- Score increments on passing each pipe pair
- **Score sparkle particles** — 20 golden `GL_POINTS` burst outward on every score
- Session high score persisted in memory

### 4 Dynamic Weather Modes

| Mode | Sky Gradient | Special Elements | Pipe Tint |
|------|-------------|------------------|-----------|
| **Day** | Blue gradient | Animated sun + 14 rotating rays | Normal green |
| **Sunny** | Warm gold gradient | Larger sun, warm cloud tones | Slightly warmer |
| **Rain** | Dark grey overcast | 220 rain drops, Bresenham lightning, fog layers | Darker |
| **Night** | Deep navy | 110 twinkling stars, crescent moon with craters | Dimmed |

All scene elements adapt — pipe colour, grass shade, building brightness, window glow colour, cloud transparency — based on the active `WeatherTheme` struct.

### Weather Selector
- Interactive clickable buttons on **Title Screen**
- Same selector also appears on **Game Over Screen** so players can choose their next theme before restarting
- Hover effects + glow border on selected button
- Animated weather icons inside each button (sun rays spin, rain drops move)

### Interactive UI
- **Title screen** — animated logo, bobbing + scaling bird, pulsing start prompt
- **Game Over screen** — score panel + weather selector for next round
- **Pause screen** — dim overlay with resume instruction
- **Close button** — red ✕ in top-right corner, works in all game states
- **Weather name toast** — fading announcement on weather change

### Audio (Programmatic)
All sounds are **generated in memory** as 16-bit PCM WAV data — no `.wav` files needed. Multi-segment frequency sweeps with attack/release envelopes create:

| Sound | Effect | Trigger |
|-------|--------|---------|
| Flap | Quick rising chirp | Every flap |
| Score | Happy C–E–G arpeggio | Passing a pipe |
| Die | Sad descending melody | Collision |
| Click | Sharp button press | UI buttons |
| Hover | Soft tick | Button hover |
| Weather | Rising arpeggio | Weather change |

### Visual Polish
| Effect | Description |
|--------|-------------|
| Screen shake | 15-frame camera offset on death |
| Death flash | White overlay fade on collision |
| Weather flash | Brief white flash on theme change |
| Parallax clouds | 2-layer cloud scroll (0.35× and 0.80×) |
| Scrolling ground | Grass tile pattern synced to pipe speed |
| Rain puddles | Ellipse puddles in ground strip (Rain mode) |
| Star twinkle | Alpha modulated by `sin(frame + phase)` |
| City silhouette | 14 procedural buildings, lit windows per weather |
| Aspect ratio lock | Pillarbox / letterbox viewport for any window size |

---

## 📐 Code Architecture

The entire game lives in a single file `flappy_bird.c` (~1970 lines) structured into clearly labelled sections.

```
flappy_bird.c
│
├── Constants & Defines        Window size, physics, pipe, weather, sound IDs
├── Enums & Structs            GameState, WeatherMode, Bird, Pipe, Cloud, Particle...
├── Globals                    All game state variables
├── Utility Functions          lerpf, clampf, randf, isInRect, screenToWorld
│
├── ── ALGORITHMS ─────────────────────────────────────────────────────
│   ├── ddaLine()              DDA Line rasteriser
│   ├── ddaOutlineRect()       4-edge DDA rectangle outline
│   ├── midpointCircle()       8-octant Midpoint Circle rasteriser
│   └── bresenhamLine()        Integer Bresenham line rasteriser
│
├── Drawing Primitives         fillRect, outlineRect, fillRoundRect,
│                              fillCircle, fillEllipse
├── Text Helpers               bitmapText, strokeText, strokeWidth
├── Sound System               buildWav (PCM generator), playSound, initSounds
│
├── ── DRAW FUNCTIONS ─────────────────────────────────────────────────
│   ├── drawBackground()       Weather-aware sky gradient quad
│   ├── drawSun()              Circle body + GL_LINES rotating rays
│   ├── drawMoon()             Circle + crescent cutout + craters
│   ├── drawStars()            GL_POINTS + GL_LINES cross-arms (Night)
│   ├── drawRain()             GL_LINES angled streaks (Rain)
│   ├── drawLightning()        Bresenham zigzag segments (Rain)
│   ├── drawFog()              Translucent overlay bands (Rain)
│   ├── drawCloud()            8-ellipse blob with shadow/highlight
│   ├── drawClouds()           Renders all 7 cloud instances
│   ├── drawCitySilhouette()   14 procedural buildings + windows
│   ├── drawGround()           Dirt/grass layers, puddles, pebbles
│   ├── drawBird()             Body (fillCircle) + midpointCircle outline
│   │                          + wing (ellipse) + eye + beak (triangle)
│   ├── drawBirdReflection()   Y-reflection transform + faded bird
│   ├── drawParticles()        GL_POINTS score sparkle burst
│   ├── drawSinglePipe()       Filled body + highlights + DDA outline
│   ├── drawPipes()            Renders all 3 pipe pairs
│   ├── drawHUD()              Score display + high score + hint
│   ├── drawWeatherSelector()  4 interactive weather theme buttons
│   ├── drawWeatherIcon()      Animated mini-icon per weather button
│   ├── drawTitleScreen()      Logo + bird + selector + start prompt
│   ├── drawPlayAgainButton()  Loading arc → clickable green button
│   ├── drawGameOverScreen()   Score panel + weather selector
│   ├── drawPauseScreen()      Dim overlay
│   ├── drawWeatherName()      Fading weather toast
│   └── drawCloseButton()      Red ✕ quit button (top-right)
│
├── ── PARTICLE SYSTEM ────────────────────────────────────────────────
│   ├── triggerParticles()     Spawn 20 radial sparkles on score
│   └── updateParticles()      Physics step (gravity + drag + fade)
│
├── ── UPDATE FUNCTIONS ───────────────────────────────────────────────
│   ├── updateBird()           Gravity, velocity clamp, tilt lerp, wing
│   ├── updatePipes()          Scroll, recycle, score detection
│   ├── updateParticles()      Advance particle physics each frame
│   ├── updateClouds()         Scroll and wrap cloud positions
│   ├── updateRainDrops()      Move and recycle rain particles
│   ├── updateWeather()        Auto-cycle timer, lightning trigger
│   └── updateShake()          Decay screen-shake offset
│
├── checkCollision()           Circle-AABB vs pipes + boundary check
├── display()                  Master render call (all draw functions)
├── timerCallback()            60 FPS game loop via glutTimerFunc
├── doFlap()                   Unified flap/start/restart handler
├── keyboardInput()            Space, W, P, R, ESC
├── specialKeys()              F11 fullscreen toggle
├── mouseInput()               Click → close, weather, flap, play again
├── passiveMotion()            Hover detection (weather, play again, close)
├── reshape()                  Pillarbox/letterbox viewport calculation
└── main()                     GLUT init + callbacks + game loop start
```

### Physics Constants

```c
#define GRAVITY           -0.45f   // downward acceleration per frame
#define FLAP_VEL          10.0f    // upward velocity on flap
#define MAX_FALL_VEL     -13.0f    // terminal velocity cap
#define PIPE_GAP         190.0f    // vertical gap (larger = easier)
#define PIPE_SPACING     290.0f    // horizontal gap between pairs
#define PIPE_BASE_SPEED    2.7f    // initial scroll speed (px/frame)
#define PIPE_SPEED_INC     0.002f  // speed increase per frame
#define WEATHER_CYCLE_FRAMES 1800  // frames between auto weather change (~30s)
```

### Game State Machine

```
┌──────────────┐   SPACE / Click     ┌─────────────┐
│  TITLE       │ ──────────────────► │  PLAYING    │
│  (selector)  │                     │             │
└──────────────┘                     └──────┬──────┘
                                            │ P key
                                     ┌──────▼──────┐
                                     │   PAUSED    │
                                     │             │
                                     └──────┬──────┘
                                            │ collision
                                     ┌──────▼──────┐   SPACE/R/Click
                                     │  GAME OVER  │ ──────────────► PLAYING
                                     │ (selector)  │
                                     └─────────────┘
```

---

## 🎯 EP Mapping — How This Project Satisfies Every Criterion

### EP1 — Depth of Knowledge Required (8 Marks)

> *"Requires advanced understanding of computer graphics algorithms, transformations, and rendering concepts."*

**How I addressed EP1:**

1. **Three manual algorithms** implemented from first principles — no shortcuts:
   - DDA Line: incremental floating-point rasteriser for pipe outlines
   - Midpoint Circle: integer decision-variable circle scanner for bird outline
   - Bresenham Line: error-accumulation integer line drawer for lightning

2. **All four 2D transformations** applied correctly:
   - Translation for movement, Rotation for tilt and spin, Scaling for pulse animation, Reflection for puddle mirror

3. **Advanced rendering concepts** beyond basic drawing:
   - Alpha blending (`glEnable(GL_BLEND)`) for transparent overlays, rain, puddles, and star twinkle
   - Double buffering (`GLUT_DOUBLE`) for tear-free 60 FPS animation
   - Parametric circle generation using `cos/sin` for smooth fills
   - 8-fold symmetry exploitation in the circle algorithm

4. **Physics simulation** — gravity accumulation, velocity clamping, `lerp`-based angle smoothing

---

### EP3 — Depth of Analysis Required (8 Marks)

> *"Must analyze transformations, shape modeling, object movement, and algorithmic efficiency."*

**How I addressed EP3:**

1. **Algorithmic analysis**: Each algorithm is documented with its decision variable, termination condition, and octant-symmetry analysis. The three algorithms are compared for arithmetic type (float vs integer), applicable shapes, and performance.

2. **Transformation sequencing**: The reflection transform uses a precise three-step matrix sequence (translate → rotate → scale) that cannot be reordered. This was analysed carefully to produce a correct puddle mirror.

3. **Object decomposition**: The bird is broken into 7 sub-components (body circle, Midpoint outline, wing ellipse, belly ellipse, eye white, iris, beak triangle) each drawn with appropriate primitives.

4. **Scene layering order**: Objects must be drawn back-to-front (painter's algorithm):
   ```
   Sky → Stars/Moon/Sun → Clouds → City → Pipes → Ground →
   Reflection → Bird → HUD → Rain/Fog/Lightning → Particles → UI
   ```
   Any deviation causes incorrect visual occlusion.

5. **Particle physics**: Score sparkle particles use gravity (`vy -= 0.28`) and drag (`vx *= 0.97`) to produce realistic arc trajectories, with `life` fading their alpha from 1.0 to 0.

6. **Collision analysis**: A circle–AABB test was chosen over pixel-perfect collision because it provides consistent, forgiving gameplay and runs in O(n) with n = number of pipes.

---

### EP4 — Familiarity of Issues (8 Marks)

> *"Problems are within the familiar domain of computer graphics but require integration of multiple techniques."*

**How I addressed EP4:**

1. **Integration of multiple techniques** — the scene combines rasterisation algorithms, transformation matrices, physics simulation, particle systems, procedural audio, and state-machine logic all working together in a single render loop.

2. **Real-time rendering constraints** — achieving 60 FPS requires every draw call to complete within ~16ms. Heavy elements (220 rain particles, DDA outlines for 6 pipe sides) were profiled and found to be well within budget.

3. **Coordinate system management** — GLUT mouse Y is top-down while OpenGL Y is bottom-up. `screenToWorld()` correctly converts between them for accurate button hit testing.

4. **Weather theme system** — a `WeatherTheme` struct stores all colour parameters per mode. Interpolating these across drawing functions (sky, clouds, grass, building darkness, window colour) required careful analysis of which components need per-weather variation.

5. **Procedural sound** — generating WAV data in memory (44-byte header + 16-bit PCM samples) with frequency-sweep segments and attack/release envelopes demonstrates integration of signal processing with graphics.

---

### EA1 & EA3 — Range of Resources and Complexity of Activities (16 Marks)

**EA1 (Resources):** Used OpenGL 1.x API, FreeGLUT, Windows Multimedia API (WinMM), C99 standard library (math, stdlib, time, string), and Code::Blocks IDE with MinGW GCC 8.1.

**EA3 (Complexity):** The project creates a dynamic, multi-object interactive scene with:
- 7+ independently animated objects
- A 4-state game state machine
- 4 weather environments with ~15 visual parameters each
- Real-time collision detection + response
- An interactive UI with hover/click detection in all states
- Programmatic PCM audio synthesis
- All within a single 1970-line C file

---

## ⚙️ Setup & Build

### Prerequisites

| Item | Details |
|------|---------|
| OS | Windows 7 / 10 / 11 |
| IDE | Code::Blocks with MinGW bundled (`codeblocks-XX.XX-mingw-setup.exe`) |
| Graphics Library | FreeGLUT for MinGW |
| C Standard | C99 |

---

### Step 1 — Install Code::Blocks with MinGW

Download from [codeblocks.org/downloads](https://www.codeblocks.org/downloads/binaries/). Choose the installer file that includes MinGW (named `codeblocks-XX.XX-mingw-setup.exe`).

---

### Step 2 — Download and Install FreeGLUT

Download the MinGW FreeGLUT package from [transmissionzero.co.uk](https://www.transmissionzero.co.uk/software/freeglut-devel/).

Copy the extracted files into your MinGW installation (`C:\Program Files\CodeBlocks\MinGW\`):

| Copy from FreeGLUT | Paste into MinGW |
|---------------------|------------------|
| `include\GL\` (all `.h` files) | `<MinGW>\include\GL\` |
| `lib\libfreeglut.a` | `<MinGW>\lib\` |
| `lib\libfreeglut_static.a` | `<MinGW>\lib\` |
| `bin\freeglut.dll` | Project folder (next to `.exe`) |

---

### Step 3 — Clone and Build

```bash
git clone https://github.com/labonysur-cloud/flappybird.git
cd flappybird
```

**Option A — Code::Blocks (Recommended)**
```
Open FlappyBird.cbp → Press F9 (Build and Run)
```

**Option B — Command Line**
```bash
gcc flappy_bird.c -o flappy_bird.exe ^
    -lopengl32 -lglu32 -lfreeglut -lwinmm -lm -std=c99
flappy_bird.exe
```

> Make sure `freeglut.dll` is in the same folder as `flappy_bird.exe`.

---

## 🕹️ Controls

| Input | Action |
|-------|--------|
| `Space` | Flap / Start game / Restart after death |
| `Left Click` | Same as Space (also activates UI buttons) |
| `W` | Cycle weather mode |
| `P` | Pause / Unpause |
| `R` | Restart from Game Over |
| `F11` | Toggle fullscreen |
| `ESC` | Quit |
| `✕ Button` | Red close button (top-right) — quit from any state |
| Weather buttons | Click Day / Sunny / Rain / Night on Title or Game Over |

---

## 🗂️ Project Files

```
flappybird/
├── flappy_bird.c       Complete game source (~1970 lines, single file)
├── FlappyBird.cbp      Code::Blocks project file (linker flags pre-set)
├── freeglut.dll        FreeGLUT runtime DLL (required next to .exe)
├── README.md           This file
├── LICENSE             MIT License
├── .gitignore          Excludes build output and binaries
└── asset/image/
    ├── day.png         Day mode screenshot
    ├── dawn.png        Sunny mode screenshot
    ├── rainy.png       Rain mode screenshot
    └── night.png       Night mode screenshot
```

---

## 🐛 Troubleshooting

| Problem | Solution |
|---------|----------|
| `fatal error: GL/glut.h: No such file` | Copy FreeGLUT headers into `<MinGW>\include\GL\` |
| `undefined reference to glutInit` | Add `-lfreeglut` to Code::Blocks linker settings |
| `freeglut.dll not found` | Copy `freeglut.dll` next to your compiled `.exe` |
| Window opens and immediately closes | Run from a Command Prompt to see the error message |
| Choppy animation | Build in **Release** mode (not Debug) |
| Window appears black | Update your graphics card drivers |
| Mouse clicks not registering correctly | Resize window with F11 to reset the viewport mapping |

---

## 👤 Author

**Labony Sur**
Department of Computer Science and Engineering
Computer Graphics Laboratory — 2026

- GitHub: [labonysur-cloud](https://github.com/labonysur-cloud)
- Email: labonysur473@gmail.com

---

## 📄 License

This project is licensed under the **MIT License** — see the [LICENSE](LICENSE) file for details.

---

<p align="center">
  Built with ❤️ using C99, OpenGL, and FreeGLUT &nbsp;|&nbsp; Computer Graphics Laboratory 2026
  <br/>
  <i>Every pixel drawn by hand. Every algorithm implemented from scratch.</i>
</p>
