# Flying Bird 2D - OpenGL Project Documentation

This repository contains the full source code for the **Flying Bird** game, built purely using C++ and the legacy OpenGL pipeline (FreeGLUT).

Below is the detailed documentation explaining how and why specific algorithms (especially in computer graphics, 2D/3D paradigms, animation, and sound) were designed and implemented.

---

## 1. Graphics Algorithms & Rendering (2D and Pseudo-3D)

### 1.1. Orthographic Projection (2D space)
- **How**: The entire game environment is mapped to screen space using `gluOrtho2D(0, WIN_W, 0, WIN_H)`.
- **Why**: This maps standard coordinate units directly to screen pixels, bypassing complex 3D perspective projection. This is ideal for 2D side-scrolling games because it allows precise, pixel-perfect placement of UI elements, sprites, and background textures on a rigid 2D plane.

### 1.2. Primitive Generation & Procedural Shapes
- **How**: Rather than relying heavily on loaded external textures (like PNGs), almost all assets (birds, pipes, buildings, trees, mountains) are generated procedurally using algorithmic geometric construction (Primitives like `GL_QUADS`, `GL_TRIANGLE_FAN`, `GL_LINE_STRIP`, `GL_QUAD_STRIP`, `GL_POLYGON`).
  - **Circle Algorithm (Trigonometric Midpoint)**: Drawn using `GL_TRIANGLE_FAN`. A loop calculates points along the circumference from 0 to 2π radians utilizing `cos()` and `sin()` to plot the radius.
  - **Rainbow Algorithm**: Uses `GL_QUAD_STRIP` across 60 segments, mathematically interpolating between an inner and outer radius. This dynamically draws 7 completely solid, smooth, concentric colored bands (Red, Orange, Yellow, Green, Blue, Indigo, Violet) perfectly aligned behind the background layers.
- **Why**: Procedural generation keeps the memory footprint extremely low, removes external image loading dependencies, and demonstrates a core understanding of how primitive vectors (Vertices) form complex objects directly within the graphics engine.

### 1.3. Parallax Scrolling (Pseudo-3D Illusion)
- **How**: The background environment is divided into distinct depth layers (Sky, Clouds, Mountains, Buildings, Trees, Grass). Each layer scrolls horizontally to the left, but their speeds are scaled based on depth. For example, mountains move at `bgSpeed * 0.1f`, buildings at `0.5f`, and grass at `1.0f`.
- **Why**: In a flat 2D environment, mimicking 3D depth is crucial for immersion. Objects further away from the camera must visually move slower than objects closer to the camera. This algorithm successfully simulates volumetric depth without needing a Z-buffer or actual 3D camera translation.

### 1.4. Matrix Transformations (Foundational 3D Concepts)
- **How**: The bird character utilizes the OpenGL matrix stack (`glPushMatrix()`, `glTranslatef()`, `glRotatef()`, and `glPopMatrix()`). As the bird's vertical velocity (`vy`) changes, a target angle is calculated (pointing up during a flap, nose-diving during a fall). `glRotatef()` is then applied to pitch the model correctly.
- **Why**: By pushing and popping the matrix stack, the rotation algorithm ensures that the local tilt only affects the bird model, completely preventing the rotation matrix from accidentally twisting the entire global game world.

---

## 2. Animation System

The game relies on three distinct animation paradigms to keep the visual feed lively and fluid:

1. **Keyframe (Sprite) Animation**: The bird's wing is toggled between multiple states (up, middle, down). A time-based algorithm tracks the frames since the last flap to decide which exact model state to render.
2. **Newtonian Physics Engine Integration**: Instead of moving the bird linearly, a basic Newtonian integration algorithm is used:
   - `Velocity (vy) = vy - (Gravity * DeltaTime)`
   - `Position (y) = y + (vy * DeltaTime)`
   - This creates a completely smooth, realistic parabolic arc during jumps and falls.
3. **Procedural Weather Animation**: Snow, Rain, and the rotating Sun are procedurally animated. The sun utilizes a continuous rotation matrix scalar (`sunRot += 0.5f`), while the rain and snow rely on particle system logic (resetting their Y coordinate to the top of the screen when they fall off the bottom).

---

## 3. Sound & Concurrency (Multi-threading)

- **How**: Sound is handled natively via the Windows MCI (Media Control Interface). However, blocking the main OpenGL render thread to load or seek audio causes severe frame stuttering. To solve this, a **Multi-threaded Consumer-Producer Algorithm** is implemented.
  - The main game thread acts as the **Producer**, pushing audio commands (e.g., `play flap from 0`) into a circular queue protected by a Critical Section (Mutex).
  - A persistent background **Worker Thread** runs continuously, waiting for an Event signal. When signaled, it acts as the **Consumer**, popping commands from the queue and executing them asynchronously on the Windows API.
- **Why**: This completely eliminates UI freezing and audio latency. It also allows complex background tasks, like checking if BGM (Background Music) has ended and auto-restarting it, to happen without interrupting the crucial 60 FPS graphics rendering loop.

---

## 4. Collision Detection & Interaction

- **How**: The core interaction mechanism relies on **Axis-Aligned Bounding Box (AABB)** intersection algorithms.
  - The bird has an invisible bounding box calculated dynamically on every frame.
  - The pipes have explicit top, bottom, left, and right boundaries.
  - If the overlapping constraints (`Bird.Right > Pipe.Left && Bird.Left < Pipe.Right`, etc.) are met, a collision is registered and the Game Over state is triggered.
- **Why**: AABB is the most mathematically efficient form of collision detection for 2D grid-aligned objects, requiring only 4 fast floating-point comparisons per object per frame.

---

*Note: A formatted `.docx` version of this document (`Project_Documentation.docx`) is also included in this repository for offline viewing or academic submission.*
