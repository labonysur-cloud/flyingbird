<p align="center">
  <img src="https://img.shields.io/badge/Language-C%2B%2B17-blue?style=for-the-badge&logo=cplusplus" />
  <img src="https://img.shields.io/badge/Graphics-OpenGL%20%2B%20FreeGLUT-green?style=for-the-badge" />
  <img src="https://img.shields.io/badge/Platform-Windows-lightgrey?style=for-the-badge&logo=windows" />
  <img src="https://img.shields.io/badge/IDE-Code%3A%3ABlocks-orange?style=for-the-badge" />
  <img src="https://img.shields.io/badge/Algorithms-DDA%20%7C%20Midpoint%20%7C%20Bresenham%20%7C%20B%C3%A9zier%20%7C%20Cohen--Sutherland-purple?style=for-the-badge" />
  <img src="https://img.shields.io/badge/License-MIT-yellow?style=for-the-badge" />
</p>

<h1 align="center">🐦 Flying Bird — OpenGL Computer Graphics Project</h1>

<p align="center">
  <b>A fully playable, visually rich Flying Bird game built entirely in C++ using OpenGL and FreeGLUT.</b><br/>
  Submitted as a <b>Computer Graphics Lab Project</b> demonstrating mastery of graphics algorithms,<br/>
  2D &amp; 3D transformations, primitives, animation, and real-time rendering — no game engines, no image assets, pure OpenGL code.
</p>

<p align="center">
  <a href="#-project-overview">Overview</a> &nbsp;|&nbsp;
  <a href="#-graphics-algorithms-implemented">Algorithms</a> &nbsp;|&nbsp;
  <a href="#-2d-transformations">Transformations</a> &nbsp;|&nbsp;
  <a href="#-3d-rendering">3D Rendering</a> &nbsp;|&nbsp;
  <a href="#-primitives-used">Primitives</a> &nbsp;|&nbsp;
  <a href="#-ep-mapping">EP Mapping</a> &nbsp;|&nbsp;
  <a href="#-features">Features</a> &nbsp;|&nbsp;
  <a href="#-setup--build">Build</a> &nbsp;|&nbsp;
  <a href="#-controls">Controls</a>
</p>

---

## 📌 Project Overview

This project is a complete **Flying Bird game** written from scratch in **C++17** using **OpenGL** and **FreeGLUT**. Every single visual element — the bird, pipes, clouds, sky, city skyline, rain, lightning, stars, sun, moon, 3D chick, 3D spinning coins, and all UI — is drawn programmatically using OpenGL drawing primitives. There are **zero image files** and **zero external assets** of any kind.

The project was developed specifically to satisfy the **Computer Graphics Lab Project rubric (Week 6 Mini Project)**, and goes far beyond the minimum requirements by implementing **five** graphics algorithms, all **four** major 2D transformation types, full **3D perspective rendering** with `GL_LIGHTING`, an advanced **particle system**, a **reflection** effect, and **five dynamic weather environments**.

### What makes this project stand out

| Requirement | Minimum | This Project |
|-------------|---------|-------------|
| Language | C or C++ | **C++17** |
| Graphics Algorithms | 2 | **5** (DDA + Midpoint Circle + Bresenham + Bézier + Cohen-Sutherland) |
| 2D Transformations | Translation + one more | **All 4** (Translation, Rotation, Scaling, Reflection) |
| 3D Transformation | Basic | **Full 3D** — gluPerspective + GL_LIGHTING chick bird + 3D spinning coin |
| Animated Objects | 1 | **10+** (bird, pipes, clouds, rain, snow, sun rays, particles, lightning, shooting stars, coins) |
| Primitives | Points, Lines, Polygons, Circles | **All** — used meaningfully in every scene layer |
| Sound | Optional | **7 programmatic sounds** (no .wav files, pure PCM synthesis) |
| Weather Modes | — | **5 modes** (Day, Sunny, Rain, Night, Snow) |
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

This is the most important technical section. I manually implemented **five rasterisation and clipping algorithms** from scratch — no OpenGL line-drawing shortcuts were used for these.

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

**My implementation (C++):**
```cpp
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

**Where used:** Draws the **4-edge outline of every pipe body and cap** via `ddaOutlineRect()`. Visible every single frame during gameplay.

---

### Algorithm 2 — Midpoint Circle (Bresenham's Circle Algorithm)

**What it is:**
The Midpoint Circle algorithm plots a circle of radius `r` by starting at `(0, r)` and using an integer decision variable `d = 1 − r` to choose between moving East `(x++)` or South-East `(x++, y--)` at each step. It exploits **8-fold symmetry** so only one octant needs computation.

**Mathematical foundation:**
```
d = 1 − r
while x ≤ y:
    plot 8 symmetric points
    if d < 0:
        d += 2x + 3        // midpoint inside circle → move East
    else:
        d += 2(x−y) + 5   // midpoint outside circle → move South-East
        y−−
    x++
```

**My implementation (C++):**
```cpp
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

**Where used:** Draws the **bird's circular body outline** every frame, and the **bird reflection outline** in the puddle.

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

**My implementation (C++):**
```cpp
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

**Where used:** Draws the **lightning bolt** in Rain weather mode. Two passes (glow + core) create the classic jagged lightning effect.

---

### Algorithm 4 — Cubic Bézier Curve

**What it is:**
Interpolates smoothly between 4 control points P₀, P₁, P₂, P₃ using the parametric cubic formula. As `t` goes from 0 to 1, the curve sweeps continuously from P₀ to P₃.

**Mathematical foundation:**
```
B(t) = (1−t)³P₀ + 3(1−t)²t·P₁ + 3(1−t)t²·P₂ + t³P₃
```

**My implementation (C++):**
```cpp
static void bezierPoint(float t, float *px, float *py,
    float p0x, float p0y, float p1x, float p1y,
    float p2x, float p2y, float p3x, float p3y)
{
    float u = 1.0f - t;
    *px = u*u*u*p0x + 3*u*u*t*p1x + 3*u*t*t*p2x + t*t*t*p3x;
    *py = u*u*u*p0y + 3*u*u*t*p1y + 3*u*t*t*p2y + t*t*t*p3y;
}
```

**Where used:** Draws the **animated water wave** on the ground. The control points oscillate with `sin(frame)` to create a flowing river-like animation.

---

### Algorithm 5 — Cohen-Sutherland Line Clipping

**What it is:**
Clips a line segment against a rectangular clipping window using outcodes (bit flags: LEFT=1, RIGHT=2, BOTTOM=4, TOP=8). Repeatedly tests endpoint pairs and clips to the window boundary until the segment is fully inside or fully rejected.

**Mathematical foundation:**
```
Assign outcode to each endpoint based on region
while not (trivially accept or trivially reject):
    clip outside endpoint to window boundary
    recompute outcode
```

**My implementation (C++):**
```cpp
static int cohenSutherland(float *x1, float *y1, float *x2, float *y2,
                           float xmin, float xmax, float ymin, float ymax);
```

**Where used:** **Clips every rain drop** against the ground boundary before drawing. Rain drops that would extend into the ground are cleanly clipped — demonstrates real-time line clipping every frame.

---

### Algorithm Comparison Summary

| Property | DDA | Midpoint Circle | Bresenham | Bézier | Cohen-Sutherland |
|----------|-----|-----------------|-----------|--------|-----------------|
| Arithmetic | Float | Integer | Integer | Float | Float |
| Draws | Lines | Circles | Lines | Curves | Clipped lines |
| Applied to | Pipe outlines | Bird outline | Lightning | Water wave | Rain clipping |
| Visible every frame? | Yes | Yes | Rain mode | Yes | Rain mode |
| GL Primitive | `GL_POINTS` | `GL_POINTS` | `GL_POINTS` | `GL_LINE_STRIP` | `GL_LINES` |

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
- **Pipe scrolling** — each pipe's X position decreases by `pipeSpeed` per frame
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
- **Bird tilt** — `glRotatef(g_bird.angle, 0, 0, 1)` rotates the bird up to +25° on flap and down to −55° in freefall. Angle is smoothly interpolated using `lerp`
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
- **Title bird pulse** — `glScalef(titleScale, titleScale, 1)` where `titleScale = 1 + 0.12·sin(frame·0.07)` makes the bird breathe in and out
- **Ellipse drawing** — `glScalef(rx, ry, 1)` inside `fillEllipse()` draws all clouds, wing shapes, belly highlights
- **Reflection Y-scale** — `glScalef(1, -1, 1)` is used as part of the reflection transform

```cpp
/* Title bird scaling — uniform scale transform */
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
I implemented a **bird puddle reflection** that appears in the ground strip when the bird flies close to the ground — like a mirror image in a rain puddle.

```cpp
static void drawBirdReflection(void) {
    float grassY = GROUND_Y + GROUND_H * GRASS_H_RATIO;
    float reflY  = 2.f * grassY - g_bird.y;
    glPushMatrix();
        glTranslatef(BIRD_X, reflY, 0.f);   // Step 1: move to reflected position
        glRotatef(-g_bird.angle, 0,0,1);     // Step 2: flip rotation sign
        glScalef(1.0f, -1.0f, 1.0f);        // Step 3: mirror about Y axis
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

## 🌐 3D Rendering

The project uses **full 3D perspective rendering** via `gluPerspective` + `gluLookAt` + `GL_LIGHTING` for two key elements:

### 3D White Fluffy Chick Bird

The bird is rendered as a **fully lit 3D chick** using:
- `gluPerspective(60°)` — perspective projection
- `gluLookAt` — camera positioned at z=5, looking at origin
- `GL_LIGHT0` — directional light with diffuse, ambient, and specular components
- `glMaterialfv` — per-part material properties (white body, golden beak, dark eyes)
- `glutSolidSphere` — body, cheek puffs, eyes, irises, specular highlights
- `glutSolidCone` — golden beak
- `glutSolidCube` — orange legs and feet
- `glRotatef` — Z-axis tilt based on velocity (matched to 2D angle)

The viewport is temporarily switched to a small 3D sub-viewport using `begin3D()`/`end3D()` helpers, then restored to the 2D ortho projection for the rest of the scene.

### 3D Spinning Coin

Each pipe has a **3D gold coin** rendered with:
- Same `gluPerspective` + `GL_LIGHTING` setup
- `glutSolidSphere` flattened by `glScalef(1, 1, 0.22f)` — creates the flat coin disc
- Y-axis rotation `glRotatef(spinAngle, 0, 1, 0)` — creates the spinning illusion
- Gold material (`GL_DIFFUSE`, `GL_SPECULAR`, `GL_SHININESS`) for realistic metallic look
- Inner darker ring rendered as a second sphere at smaller scale

---

## 🔷 Primitives Used

All four required OpenGL primitive types are used meaningfully throughout the scene.

| Primitive | OpenGL Constant | Where Used |
|-----------|-----------------|------------|
| **Points** | `GL_POINTS` | DDA pipe outlines, Midpoint Circle bird outline, Bresenham lightning, score particle sparkles, night stars, snowflakes |
| **Lines** | `GL_LINES`, `GL_LINE_STRIP`, `GL_LINE_LOOP` | Sun rays, close-button X cross, rain drop streaks, star cross-arms, Bézier water wave, rainbow arcs, medal ring |
| **Polygons / Quads** | `GL_QUADS`, `GL_TRIANGLE_FAN` | Sky background gradient, pipe bodies and caps, ground layers, buildings, clouds, all filled circles/ellipses |
| **3D Solids** | `glutSolidSphere`, `glutSolidCone`, `glutSolidCube` | 3D chick body, eyes, beak, legs, 3D spinning coin |

> Every primitive listed above is active in at least one visible game state and contributes meaningfully to the visual output — not just placed as a token demonstration.

---

## 🎮 Features

### Core Gameplay
- **Gravity physics** — constant downward acceleration with terminal velocity cap
- **Flap mechanic** — upward velocity impulse on Space / left click
- **Smooth bird tilt** — angle linearly interpolates toward target (`lerp`) every frame
- **3-frame wing animation** — up / mid / down cycling at configurable rate
- **Progressive difficulty** — pipe speed increases `+0.002 px/frame` indefinitely
- **3 Difficulty Levels** — Easy / Normal / Hard (affects pipe gap and speed)

### Pipe & Coin System
- 3 pipe pairs simultaneously on screen with random gap heights
- Pipe cap extends wider than body (capped end detail, DDA-outlined)
- Each pipe has a **3D spinning gold coin** in the gap — collect for bonus points
- Coin collection popup `+5` animation on screen

### Collision & Scoring
- Circle vs. AABB collision test (slightly shrunk hitbox for fairness)
- Ground and ceiling boundary checks
- Score increments on passing each pipe pair
- **Score sparkle particles** — 20 golden `GL_POINTS` burst outward on every score
- Coin score tracked separately, shown on HUD in gold
- **Medal system** on Game Over — Bronze / Silver / Gold / Platinum based on score
- High score persisted to `highscore.txt`

### 5 Dynamic Weather Modes

| Mode | Sky Gradient | Special Elements | Pipe Tint |
|------|-------------|------------------|-----------|
| **Day** | Blue gradient | Animated sun + 14 rotating rays | Normal green |
| **Sunny** | Warm gold gradient | Larger sun, rainbow, warm cloud tones | Slightly warmer |
| **Rain** | Dark grey overcast | 220 rain drops, Bresenham lightning, fog layers | Darker |
| **Night** | Deep navy | 110 twinkling stars, crescent moon, shooting stars | Dimmed |
| **Snow** | Pale white-blue | 180 falling snowflakes, snow-capped pipes, icy trees | Cold tint |

All scene elements adapt — pipe colour, grass shade, building brightness, window glow colour, cloud transparency — based on the active `WeatherTheme` struct.

### Weather Selector
- Interactive clickable buttons on **Title Screen** and **Game Over Screen**
- Hover effects + glow border on selected button
- Animated weather icons inside each button (sun rays spin, rain drops move, snowflake crystals)
- Weather description text shown below selector

### Interactive UI
- **Title screen** — animated logo, bobbing + scaling 3D chick, pulsing start prompt
- **Game Over screen** — score panel, coin score, medal, weather selector for next round
- **Pause screen** — dim overlay with resume instruction
- **Close button** — red ✕ in top-right corner, works in all game states
- **Weather name toast** — fading announcement on weather change
- **Loading arc** — circular progress indicator before Play Again button appears

### Audio (Programmatic — No .wav Files)
All sounds are **generated in memory** as 16-bit PCM WAV data using frequency sweeps with attack/release envelopes:

| Sound | Effect | Trigger |
|-------|--------|---------|
| Flap | Quick rising chirp | Every flap |
| Score | Happy C–E–G arpeggio | Passing a pipe |
| Die | Sad descending melody | Collision |
| Click | Sharp button press | UI buttons |
| Hover | Soft tick | Button hover |
| Weather | Rising arpeggio | Weather change |
| Coin | Bright ascending ting | Coin collected |

### Visual Polish
| Effect | Description |
|--------|-------------|
| Screen shake | 15-frame camera offset on death |
| Death flash | White overlay fade on collision |
| Weather flash | Brief white flash on theme change |
| Shear transform | Bird shears sideways on game over |
| Parallax clouds | 2-layer cloud scroll (0.35× and 0.80×) |
| Scrolling ground | Grass tile pattern synced to pipe speed |
| Parallax mountains | 3-layer mountain silhouettes |
| Trees | Scrolling animated trees (snow-capped in Snow mode) |
| Rainbow | Appears in Sunny mode after rain |
| Rain puddles | Ellipse puddles in ground strip (Rain mode) |
| Star twinkle | Alpha modulated by `sin(frame + phase)` |
| Shooting stars | Random streaks across Night sky |
| City silhouette | 14 procedural buildings, lit windows per weather |
| Aspect ratio lock | Pillarbox / letterbox viewport for any window size |

---

## 📐 Code Architecture

The entire game lives in a single file `flying_bird.cpp` (~2800 lines) structured into clearly labelled sections.

```
flying_bird.cpp
│
├── Constants & Defines        Window size, physics, pipe, weather, sound IDs
├── Enums & Structs             GameState, WeatherMode, Difficulty, Bird, Pipe, Cloud, Particle...
├── Globals                    All game state variables
├── Utility Functions          lerpf, clampf, randf, isInRect, screenToWorld
│
├── ── ALGORITHMS ─────────────────────────────────────────────────────────
│   ├── ddaLine()              DDA Line rasteriser (Algorithm 1)
│   ├── ddaOutlineRect()       4-edge DDA rectangle outline
│   ├── midpointCircle()       8-octant Midpoint Circle rasteriser (Algorithm 2)
│   ├── bresenhamLine()        Integer Bresenham line rasteriser (Algorithm 3)
│   ├── bezierPoint()          Cubic Bézier point evaluator (Algorithm 4)
│   ├── drawBezierWave()       Animated ground water wave
│   └── cohenSutherland()      Line clipping against rectangle (Algorithm 5)
│
├── Drawing Primitives         fillRect, outlineRect, fillRoundRect,
│                              fillCircle, fillEllipse
├── Text Helpers               bitmapText, strokeText, strokeWidth
├── Sound System               buildWav (PCM generator), playSound, initSounds
│
├── ── 3D RENDERING ──────────────────────────────────────────────────────
│   ├── begin3D() / end3D()    Perspective viewport switcher
│   ├── setupChickLight()      GL_LIGHT0 setup for 3D bird
│   ├── setMat()               Per-part material properties
│   ├── drawChickGeometry()    Full 3D chick (sphere+cone+cube parts)
│   ├── drawBird()             3D bird + 2D Midpoint Circle overlay
│   ├── drawSingleCoin()       3D spinning gold coin per pipe
│   └── drawCoins()            Renders all active coins
│
├── ── DRAW FUNCTIONS ────────────────────────────────────────────────────
│   ├── drawBackground()       Weather-aware sky gradient quad
│   ├── drawSun()              Circle body + GL_LINES rotating rays
│   ├── drawMoon()             Circle + crescent cutout + craters
│   ├── drawStars()            GL_POINTS + GL_LINES cross-arms (Night)
│   ├── drawShootingStars()    Fading streaks across Night sky
│   ├── drawRain()             GL_LINES angled streaks (Rain, Cohen-Sutherland clipped)
│   ├── drawLightning()        Bresenham zigzag segments (Rain)
│   ├── drawFog()              Translucent overlay bands (Rain/Snow)
│   ├── drawSnow()             GL_POINTS snowflakes (Snow)
│   ├── drawCloud()            8-ellipse blob with shadow/highlight
│   ├── drawMountains()        3-layer parallax mountain silhouettes
│   ├── drawTrees()            Scrolling trees with snow-cap variant
│   ├── drawRainbow()          GL_LINE_STRIP rainbow (Sunny)
│   ├── drawCitySilhouette()   14 procedural buildings + windows
│   ├── drawGround()           Dirt/grass layers, Bézier wave, puddles
│   ├── drawBirdReflection()   Y-reflection transform + faded bird
│   ├── drawParticles()        GL_POINTS score sparkle burst
│   ├── drawSinglePipe()       Filled body + highlights + DDA outline
│   ├── drawPipes()            Renders all 3 pipe pairs
│   ├── drawHUD()              Score, coins, high score, difficulty label
│   ├── drawWeatherSelector()  5 interactive weather theme buttons
│   ├── drawWeatherIcon()      Animated mini-icon per weather button
│   ├── drawDifficultySelector() Easy / Normal / Hard buttons
│   ├── drawTitleScreen()      Logo + 3D bird + selector + start prompt
│   ├── drawPlayAgainButton()  Loading arc → clickable green button
│   ├── drawMedal()            Bronze / Silver / Gold / Platinum medals
│   ├── drawGameOverScreen()   Score panel + 3D "GAME OVER" text
│   ├── drawPauseScreen()      Dim overlay
│   ├── drawWeatherName()      Fading weather toast
│   └── drawCloseButton()      Red ✕ quit button (top-right)
│
├── ── PARTICLE SYSTEM ───────────────────────────────────────────────────
│   ├── triggerParticles()     Spawn 20 radial sparkles on score
│   └── updateParticles()      Physics step (gravity + drag + fade)
│
├── ── UPDATE FUNCTIONS ──────────────────────────────────────────────────
│   ├── updateBird()           Gravity, velocity clamp, tilt lerp, wing
│   ├── updatePipes()          Scroll, recycle, score detection
│   ├── updateParticles()      Advance particle physics each frame
│   ├── updateClouds()         Scroll and wrap cloud positions
│   ├── updateRainDrops()      Move and recycle rain particles
│   ├── updateSnow()           Move and recycle snowflakes
│   ├── updateShootingStars()  Random spawn and movement of shooting stars
│   ├── updateWeather()        Auto-cycle timer, lightning trigger
│   └── updateShake()          Decay screen-shake offset
│
├── checkCollision()           Circle-AABB vs pipes + boundary check
├── display()                  Master render call (all draw functions)
├── timerCallback()            60 FPS game loop via glutTimerFunc
├── doFlap()                   Unified flap/start/restart handler
├── keyboardInput()            Space, W, P, R, ESC
├── specialKeys()              F11 fullscreen toggle
├── mouseInput()               Click → close, weather, difficulty, flap, play again
├── passiveMotion()            Hover detection (weather, difficulty, play again, close)
├── reshape()                  Pillarbox/letterbox viewport calculation
└── main()                     GLUT init + callbacks + game loop start
```

### Physics Constants

```cpp
#define GRAVITY           -0.45f   // downward acceleration per frame
#define FLAP_VEL          10.0f    // upward velocity on flap
#define MAX_FALL_VEL     -13.0f    // terminal velocity cap
#define PIPE_GAP         190.0f    // vertical gap (Normal difficulty)
#define PIPE_SPACING     290.0f    // horizontal gap between pairs
#define PIPE_BASE_SPEED    2.7f    // initial scroll speed (px/frame)
#define PIPE_SPEED_INC     0.002f  // speed increase per frame
#define WEATHER_CYCLE_FRAMES 1800  // frames between auto weather change (~30s)
#define GAMEOVER_DELAY      90     // frames before Play Again appears (1.5s)
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

1. **Five manual algorithms** implemented from first principles:
   - DDA Line, Midpoint Circle, Bresenham Line, Cubic Bézier, Cohen-Sutherland Clipping

2. **All four 2D transformations** applied correctly:
   - Translation, Rotation, Scaling, Reflection

3. **Full 3D rendering** with perspective projection and lighting:
   - `gluPerspective` + `gluLookAt` + `GL_LIGHTING` + `glMaterialfv`

4. **Advanced rendering concepts** beyond basic drawing:
   - Alpha blending, double buffering, parametric circle generation, 8-fold symmetry exploitation

5. **Physics simulation** — gravity accumulation, velocity clamping, `lerp`-based angle smoothing

---

### EP3 — Depth of Analysis Required (8 Marks)

> *"Must analyze transformations, shape modeling, object movement, and algorithmic efficiency."*

**How I addressed EP3:**

1. **Algorithmic analysis**: Each of the 5 algorithms is documented with its decision variable, termination condition, and comparison of arithmetic types (float vs integer)

2. **Transformation sequencing**: The reflection transform uses a precise three-step matrix sequence (translate → rotate → scale) that cannot be reordered

3. **3D scene decomposition**: The 3D chick is broken into 9+ sub-parts (body, cheeks, left eye white, left iris, right eye white, right iris, beak, 2 legs, 2 feet) each with correct material properties

4. **Scene layering order**: Objects must be drawn back-to-front (painter's algorithm):
   ```
   Sky → Stars/Moon/Sun → Mountains → Clouds → Rainbow → City → Trees →
   Pipes → Coins → Ground → Reflection → 3D Bird → HUD → Rain/Fog/Lightning/Snow →
   Particles → UI → Close Button
   ```

5. **Particle physics**: Score sparkle particles use gravity (`vy -= 0.28`) and drag (`vx *= 0.97`) to produce realistic arc trajectories

6. **Collision analysis**: A circle–AABB test runs in O(n) with n = number of pipes, providing fair and consistent gameplay

---

### EP4 — Familiarity of Issues (8 Marks)

> *"Problems are within the familiar domain of computer graphics but require integration of multiple techniques."*

**How I addressed EP4:**

1. **Integration of multiple techniques** — the scene combines 5 rasterisation algorithms, 2D+3D transformation matrices, physics simulation, particle systems, procedural audio, and state-machine logic all working together in a single render loop

2. **Real-time rendering constraints** — achieving 60 FPS requires every draw call to complete within ~16ms. Heavy elements (220 rain particles, DDA outlines, 3D lighting calculations) were implemented efficiently

3. **Coordinate system management** — GLUT mouse Y is top-down while OpenGL Y is bottom-up. `screenToWorld()` correctly converts between them for accurate button hit testing. The 3D sub-viewport requires careful coordinate remapping from world space to screen space

4. **Weather theme system** — a `WeatherTheme` struct stores all colour parameters per mode. Every visual element queries this struct for correct per-weather colouring

5. **Procedural sound** — generating WAV data in memory (44-byte header + 16-bit PCM samples) with frequency-sweep segments and attack/release envelopes

---

### EA1 & EA3 — Range of Resources and Complexity of Activities (16 Marks)

**EA1 (Resources):** Used OpenGL 1.x API, FreeGLUT, Windows Multimedia API (WinMM), C++17 standard library (math.h, stdlib.h, time.h, string.h), GLU library (gluPerspective, gluLookAt, gluOrtho2D), and Code::Blocks IDE with MinGW GCC 8.1.

**EA3 (Complexity):** The project creates a dynamic, multi-object interactive scene with:
- 10+ independently animated objects
- A 4-state game state machine
- 5 weather environments with ~18 visual parameters each
- Real-time collision detection + response
- 3D perspective rendering with GL_LIGHTING
- An interactive UI with hover/click detection in all states
- Programmatic PCM audio synthesis (7 sounds)
- All within a single 2800-line C++ file

---

## ⚙️ Setup & Build

### Prerequisites

| Item | Details |
|------|---------|
| OS | Windows 7 / 10 / 11 |
| IDE | Code::Blocks with MinGW bundled (`codeblocks-XX.XX-mingw-setup.exe`) |
| Graphics Library | FreeGLUT for MinGW |
| Language Standard | **C++17** |

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
Open FlyingBird.cbp → Press F9 (Build and Run)
```

**Option B — Command Line**
```bash
# Using Code::Blocks MinGW g++
"C:\Program Files\CodeBlocks\MinGW\bin\g++.exe" -std=c++17 flying_bird.cpp ^
    -o flying_bird.exe -lopengl32 -lglu32 -lfreeglut -lwinmm
flying_bird.exe
```

> Make sure `freeglut.dll` is in the same folder as `flying_bird.exe`.

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
| Weather buttons | Click Day / Sunny / Rain / Night / Snow on Title or Game Over |
| Difficulty buttons | Click Easy / Normal / Hard on Title screen |

---

## 🗂️ Project Files

```
flappybird/
├── flying_bird.cpp     Complete game source (~2800 lines, C++17, single file)
├── flying_bird.c       Original C99 source (kept for reference)
├── FlyingBird.cbp      Code::Blocks project file (g++ / c++17, linker flags pre-set)
├── freeglut.dll        FreeGLUT runtime DLL (required next to .exe)
├── highscore.txt       Persistent high score storage
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
| C++ compile errors with system g++ | Use **Code::Blocks MinGW g++** at `C:\Program Files\CodeBlocks\MinGW\bin\g++.exe` |
| `at_quick_exit` / `quick_exit` errors | Use C-style headers (`math.h` etc.) — already fixed in `flying_bird.cpp` |

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
  Built with ❤️ using <b>C++17</b>, OpenGL, and FreeGLUT &nbsp;|&nbsp; Computer Graphics Laboratory 2026
  <br/>
  <i>Every pixel drawn by hand. Every algorithm implemented from scratch. Zero image assets.</i>
</p>
