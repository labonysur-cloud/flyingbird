/* Windows audio */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

/* OpenGL */
#include <GL/glut.h>

/* Standard headers (C-style to avoid MinGW/GLUT conflicts) */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// --- CONSTANTS ---

#define WIN_W            800
#define WIN_H            600
#define WORLD_W          800.0f
#define WORLD_H          600.0f

/* Bird */
#define BIRD_X           160.0f
#define BIRD_RADIUS      18.0f
#define GRAVITY         -0.52f   /* slightly stronger pull = tighter arc   */
#define FLAP_VEL         7.5f    /* less aggressive jump = smoother control */
#define MAX_FALL_VEL    -11.0f   /* proportional to new gravity             */
#define TILT_UP_DEG      25.0f
#define TILT_DOWN_DEG   -55.0f

/* Pipes */
#define PIPE_W           70.0f

#define PIPE_SPACING     290.0f
#define PIPE_COUNT       3
#define PIPE_MIN_H       80.0f
#define PIPE_CAP_H       20.0f
#define PIPE_BASE_SPEED  2.7f
#define PIPE_SPEED_INC   0.002f
/* Coins */
#define COIN_RADIUS        14.0f
#define COIN_COLLECT_BONUS 5

/* Ground */
#define GROUND_Y         80.0f
#define GROUND_H         80.0f
#define GRASS_H_RATIO    0.30f

/* Weather */
#define WEATHER_COUNT        5
#define WEATHER_CYCLE_FRAMES 1800

/* Clouds */
#define CLOUD_COUNT      7
#define CLOUD_SPEED_FAR  0.35f
#define CLOUD_SPEED_NEAR 0.80f

/* City */
#define BUILDING_COUNT   14

/* Rain */
#define RAIN_COUNT       220

/* Stars */
#define STAR_COUNT       110

/* Wing animation */
#define WING_FRAMES      3
#define WING_ANIM_RATE   8

/* FPS */
#define TARGET_FPS       60
#define FRAME_MS         (1000 / TARGET_FPS)

/* Game over delay before Play Again button appears */
#define GAMEOVER_DELAY   90   /* 1.5 seconds at 60 fps */

/* Weather selector button layout (title screen) */
#define WBTN_W           118.0f
#define WBTN_H            70.0f
#define WBTN_GAP          8.0f
/* Total width = 5*118 + 4*8 = 622; start X = (800-622)/2 = 89 */
#define WBTN_STARTX      89.0f
#define WBTN_Y           120.0f   /* bottom edge of buttons */

/* Play Again button (game over screen) */
#define PLAY_BTN_W       200.0f
#define PLAY_BTN_H        46.0f

/* Close (quit) button — top-right corner */
#define CLOSE_BTN_SIZE    30.0f
#define CLOSE_BTN_X      (WORLD_W - CLOSE_BTN_SIZE - 8.0f)
#define CLOSE_BTN_Y      (WORLD_H - CLOSE_BTN_SIZE - 8.0f)

/* Menu button — top-left corner */
#define MENU_BTN_SIZE     30.0f
#define MENU_BTN_X        10.0f
#define MENU_BTN_Y       (WORLD_H - MENU_BTN_SIZE - 65.0f)

/* Sound sample rate */
#define SFX_RATE         22050
#define SFX_BUF          3145728   /* 64 KB per sound, enough for ~1.5 sec */

/* Sound IDs */
#define SFX_FLAP    0
#define SFX_DIE     1
#define SFX_COIN    2
#define SFX_START   3
#define SFX_THUNDER1  4
#define SFX_THUNDER2  5
#define SFX_COUNT         6

// --- ENUMS ---

typedef enum { STATE_TITLE, STATE_PLAYING, STATE_PAUSED, STATE_GAMEOVER } GameState;

typedef enum {
    WEATHER_DAY   = 0,
    WEATHER_SUNNY = 1,
    WEATHER_RAIN  = 2,
    WEATHER_NIGHT = 3,
    WEATHER_SNOW  = 4
} WeatherMode;

typedef enum {
    DIFF_EASY = 0,
    DIFF_NORMAL = 1,
    DIFF_HARD = 2
} Difficulty;

// --- STRUCTS ---

typedef struct { float x, y, scale, speed; int layer; } Cloud;
typedef struct { float x, gapCenterY; int scored; } Pipe;
typedef struct { float x, w, h, r, g, b; } Building;
typedef struct { float y, vy, angle; int wingFrame, wingTimer, alive; } Bird;
typedef struct { float x, y, speed, alpha, len; } RainDrop;
typedef struct { float x, y, size, phase; } Star;
typedef struct { float x, y, speed, size, drift; } SnowFlake;
typedef struct { float x, y, vx, vy, life, len; int active; } ShootingStar;
typedef struct { float x, y, spinAngle; int collected; } Coin;

/* Particle — one GL_POINTS sparkle emitted on every score */
#define PARTICLE_COUNT 60
typedef struct {
    float x, y;       /* world-space position       */
    float vx, vy;     /* velocity (pixels / frame)  */
    float r, g, b;    /* colour                     */
    float life;       /* 1.0 = fresh, 0.0 = dead    */
    int   active;
} Particle;

/* Per-weather visual theme */
typedef struct {
    float topR, topG, topB;     /* Sky zenith colour   */
    float botR, botG, botB;     /* Sky horizon colour  */
    float cldR, cldG, cldB, cldA; /* Cloud colour      */
    float grassR, grassG, grassB; /* Grass colour       */
    float darkness;             /* Building dim factor */
    float btnR, btnG, btnB;     /* Selector button bg  */
    const char *name;
    const char *desc;
} WeatherTheme;

static const WeatherTheme g_themes[5] = {
    /* DAY */
    { 0.22f,0.60f,0.90f,  0.45f,0.83f,0.97f,
      1.00f,1.00f,1.00f,0.92f,  0.49f,0.76f,0.26f,  0.0f,
      0.25f,0.65f,0.90f,  "Day",   "Clear skies, bright sun"  },
    /* SUNNY */
    { 0.12f,0.45f,0.92f,  0.98f,0.74f,0.22f,
      1.00f,0.97f,0.82f,0.88f,  0.55f,0.82f,0.22f,  0.0f,
      0.90f,0.70f,0.20f,  "Sunny", "Golden hour, warm glow"   },
    /* RAIN */
    { 0.22f,0.26f,0.36f,  0.34f,0.38f,0.50f,
      0.72f,0.78f,0.84f,0.72f,  0.38f,0.52f,0.30f,  0.3f,
      0.30f,0.40f,0.55f,  "Rain",  "Heavy rain and lightning" },
    /* NIGHT */
    { 0.02f,0.03f,0.18f,  0.05f,0.10f,0.34f,
      0.80f,0.86f,0.96f,0.65f,  0.12f,0.32f,0.14f,  0.7f,
      0.12f,0.15f,0.32f,  "Night", "Stars, moon, city lights" },
    /* SNOW */
    { 0.85f,0.92f,1.0f,   0.95f,0.97f,1.0f,
      1.0f,1.0f,1.0f,0.85f,   0.45f,0.65f,0.8f,  0.1f,
      0.80f,0.88f,0.98f,  "Snow", "Freezing cold, snowflakes" }
};

// --- GLOBALS ---

/* Game state */
static GameState  g_state     = STATE_TITLE;
static Bird       g_bird;
static Pipe       g_pipes[PIPE_COUNT];
static Cloud      g_clouds[CLOUD_COUNT];
static Building   g_buildings[BUILDING_COUNT];
static RainDrop   g_rain[RAIN_COUNT];
static Star       g_stars[STAR_COUNT];
static SnowFlake  g_snow[180];
static ShootingStar g_shootingStars[3];
static Particle   g_particles[PARTICLE_COUNT];

static int        g_score     = 0;
static int        g_highScore = 0;
static float      g_pipeSpeed = PIPE_BASE_SPEED;
static float      g_pipeGap   = 190.0f;
static Difficulty g_difficulty = DIFF_NORMAL;
static int        g_hoveredDiff = -1;
static Coin       g_coins[PIPE_COUNT];
static int        g_coinScore      = 0;
static int        g_coinPopupTimer = 0;

/* Screen shake */
static float      g_shakeX = 0, g_shakeY = 0;
static int        g_shakeTicks = 0;

/* Flash effects */
static int        g_flashTicks  = 0;
static int        g_wFlashTicks = 0;

/* Title animation */
static float      g_titleBobY = 0, g_titleBobT = 0;
static float      g_groundScroll = 0;
static int        g_frame = 0;

/* Weather */
static WeatherMode g_weather      = WEATHER_DAY;
static int         g_weatherTimer = 0;
static int         g_weatherNameTimer = 0;

/* Lightning */
static int         g_lightning = 0;
static float       g_boltX = 400.f;
static float       g_boltSegs[8];

/* Full screen */
static int         g_fullscreen = 1;

/* Game over delay */
static int         g_gameOverDelay = 0;
static float       g_shearX = 0.f;
static int         g_hintTimer = 0;    /* controls-box fade timer (frames remaining) */

/* Mouse tracking (world space) */
static float       g_mouseX = 0, g_mouseY = 0;

/* Hover states */
static int         g_hoveredWeather   = -1;  /* -1 = none */
static int         g_hoveredPlayAgain = 0;
static int         g_hoveredClose     = 0;
static int         g_hoveredMenu      = 0;

/* Menu state */
static int         g_menuExpanded     = 0;
static float       g_menuAnimPhase    = 0.0f;

/* Viewport (for mouse coordinate conversion) */
static int         g_vpX = 0, g_vpY = 0, g_vpW = WIN_W, g_vpH = WIN_H;
static int         g_winH = WIN_H;

/* Sound system uses mciSendString directly for all BGM and SFX */

// --- UTILITY ---

static float lerpf(float a, float b, float t) { return a + (b - a) * t; }

static float clampf(float v, float lo, float hi) {
    return v < lo ? lo : v > hi ? hi : v;
}

static float randf(float lo, float hi) {
    return lo + ((float)rand() / (float)RAND_MAX) * (hi - lo);
}

/* Rectangle hit test in world space */
static int isInRect(float mx, float my, float rx, float ry, float rw, float rh) {
    return mx >= rx && mx <= rx + rw && my >= ry && my <= ry + rh;
}

/* Convert GLUT screen pixel to world coordinate */
static void screenToWorld(int sx, int sy, float *wx, float *wy) {
    /* GLUT Y is from top; OpenGL is from bottom */
    int sy_gl = g_winH - sy;
    float rx = (float)(sx - g_vpX) / (float)g_vpW;
    float ry = (float)(sy_gl - g_vpY) / (float)g_vpH;
    *wx = rx * WORLD_W;
    *wy = ry * WORLD_H;
}

// --- DRAWING PRIMITIVES ---

static void col(int r, int g, int b)
    { glColor3f(r / 255.f, g / 255.f, b / 255.f); }
static void col4(int r, int g, int b, int a)
    { glColor4f(r / 255.f, g / 255.f, b / 255.f, a / 255.f); }
static void colF(float r, float g, float b)
    { glColor3f(r, g, b); }
static void colF4(float r, float g, float b, float a)
    { glColor4f(r, g, b, a); }

static void fillRect(float x, float y, float w, float h) {
    glBegin(GL_QUADS);
        glVertex2f(x, y);     glVertex2f(x + w, y);
        glVertex2f(x + w, y + h); glVertex2f(x, y + h);
    glEnd();
}

static void outlineRect(float x, float y, float w, float h, float t) {
    fillRect(x, y, w, t);
    fillRect(x, y + h - t, w, t);
    fillRect(x, y, t, h);
    fillRect(x + w - t, y, t, h);
}

/* Rounded rectangle (corner radius r, approximated as rect + circles) */
static void fillRoundRect(float x, float y, float w, float h, float rc) {
    fillRect(x + rc, y,    w - rc * 2, h);        /* centre strip  */
    fillRect(x,    y + rc, w,       h - rc * 2);  /* wide strip    */
    /* Four corner circles */
    int seg = 10;
    float corners[4][2] = {
        {x + rc,     y + rc},
        {x + w - rc, y + rc},
        {x + w - rc, y + h - rc},
        {x + rc,     y + h - rc}
    };
    float startA[4] = {180.f, 270.f, 0.f, 90.f};
    for (int c = 0; c < 4; c++) {
        glBegin(GL_TRIANGLE_FAN);
            glVertex2f(corners[c][0], corners[c][1]);
            for (int i = 0; i <= seg; i++) {
                float a = (startA[c] + 90.f * i / seg) * 3.14159f / 180.f;
                glVertex2f(corners[c][0] + cosf(a) * rc,
                           corners[c][1] + sinf(a) * rc);
            }
        glEnd();
    }
}

static void fillCircle(float cx, float cy, float r, int segs) {
    glBegin(GL_TRIANGLE_FAN);
        glVertex2f(cx, cy);
        for (int i = 0; i <= segs; i++) {
            float a = 2.f * 3.14159265f * i / segs;
            glVertex2f(cx + cosf(a) * r, cy + sinf(a) * r);
        }
    glEnd();
}

static void fillEllipse(float cx, float cy, float rx, float ry, int segs) {
    glPushMatrix();
        glTranslatef(cx, cy, 0.f);
        glScalef(rx, ry, 1.f);
        glBegin(GL_TRIANGLE_FAN);
            glVertex2f(0, 0);
            for (int i = 0; i <= segs; i++) {
                float a = 2.f * 3.14159265f * i / segs;
                glVertex2f(cosf(a), sinf(a));
            }
        glEnd();
    glPopMatrix();
}

// --- ALGORITHM 1 : DDA LINE  (Digital Differential Analyser) ---
// *  Principle: Compute the number of steps = max(|dx|,|dy|),
// then increment x and y by dx/steps and dy/steps each step.
// This guarantees exactly one pixel plotted per step.
// *  Usage in this project:
// - Draws the four outline edges of every pipe body and cap.
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

/* Draw four edges of a rectangle using DDA lines. */
static void ddaOutlineRect(float x, float y, float w, float h) {
    ddaLine(x,     y,     x + w, y    );   /* bottom */
    ddaLine(x + w, y,     x + w, y + h);   /* right  */
    ddaLine(x + w, y + h, x,     y + h);   /* top    */
    ddaLine(x,     y + h, x,     y    );   /* left   */
}

// --- ALGORITHM 2 : MIDPOINT CIRCLE  (Bresenham's Circle Algorithm) ---
// *  Principle: Start at (0, r). Use an integer decision variable
// d = 1 - r to choose between East (x++) and South-East
// (x++, y--) at each step. Exploits 8-fold symmetry so only
// one octant needs to be computed.
// *  Usage in this project:
// - Draws the bird's circular body outline every frame.
// - Also used for the semi-transparent reflection outline.
static void midpointCircle(float cx, float cy, int r) {
    int x = 0, y = r;
    int d = 1 - r;          /* Initial decision variable */
    glBegin(GL_POINTS);
    while (x <= y) {
        /* 8-fold symmetry: plot one point in each octant */
        glVertex2f(cx + x, cy + y);  glVertex2f(cx - x, cy + y);
        glVertex2f(cx + x, cy - y);  glVertex2f(cx - x, cy - y);
        glVertex2f(cx + y, cy + x);  glVertex2f(cx - y, cy + x);
        glVertex2f(cx + y, cy - x);  glVertex2f(cx - y, cy - x);
        if (d < 0)
            d += 2 * x + 3;          /* Midpoint inside  -> move East     */
        else {
            d += 2 * (x - y) + 5;   /* Midpoint outside -> move SE       */
            y--;
        }
        x++;
    }
    glEnd();
}

// --- ALGORITHM 3 : BRESENHAM LINE  (integer error-accumulation) ---
// *  Principle: Maintain an error term err = dx - dy. Each step,
// advance in the major axis and conditionally step in the minor
// axis when the accumulated error exceeds 0. No floating-point
// arithmetic needed.
// *  Usage in this project:
// - Draws the lightning bolt zigzag segments in RAIN mode.
static void bresenhamLine(int x1, int y1, int x2, int y2) {
    int dx  = abs(x2 - x1);
    int dy  = abs(y2 - y1);
    int sx  = (x1 < x2) ? 1 : -1;
    int sy  = (y1 < y2) ? 1 : -1;
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

// --- ALGORITHM 4 : CUBIC BEZIER CURVE ---
// *  Principle: Interpolate between 4 control points P0, P1, P2, P3.
// Formula: B(t) = (1-t)^3*P0 + 3(1-t)^2*t*P1 + 3(1-t)*t^2*P2 + t^3*P3
// Usage: Animated water wave on the ground.
static void bezierPoint(float t, float *px, float *py,
                        float p0x, float p0y,
                        float p1x, float p1y,
                        float p2x, float p2y,
                        float p3x, float p3y)
{
    float u   = 1.0f - t;
    float tt  = t * t;
    float uu  = u * u;
    float uuu = uu * u;
    float ttt = tt * t;

    *px = uuu * p0x + 3 * uu * t * p1x + 3 * u * tt * p2x + ttt * p3x;
    *py = uuu * p0y + 3 * uu * t * p1y + 3 * u * tt * p2y + ttt * p3y;
}

static void drawBezierWave(void) {
    float yBase = GROUND_Y + GROUND_H * 0.7f;
    float anim  = sinf(g_frame * 0.03f) * 8.0f;
    float p0x = 0,              p0y = yBase + anim;
    float p1x = WORLD_W * 0.33f, p1y = yBase - anim * 2.0f;
    float p2x = WORLD_W * 0.66f, p2y = yBase + anim * 2.0f;
    float p3x = WORLD_W,         p3y = yBase - anim;

    col4(100, 200, 255, 120);
    glLineWidth(2.5f);
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= 40; i++) {
        float t = i / 40.0f;
        float bx, by;
        bezierPoint(t, &bx, &by, p0x, p0y, p1x, p1y, p2x, p2y, p3x, p3y);
        glVertex2f(bx, by);
    }
    glEnd();
    glLineWidth(2.0f);
}

// --- ALGORITHM 5 : COHEN-SUTHERLAND LINE CLIPPING ---
// *  Principle: Clips a line segment against a rectangular window using
// outcodes (LEFT=1, RIGHT=2, BOTTOM=4, TOP=8).
// Usage: Clip rain drops against the ground boundary.
#define CS_INSIDE 0
#define CS_LEFT   1
#define CS_RIGHT  2
#define CS_BOTTOM 4
#define CS_TOP    8

static int csOutcode(float x, float y,
                     float xmin, float xmax,
                     float ymin, float ymax)
{
    int code = CS_INSIDE;
    if      (x < xmin) code |= CS_LEFT;
    else if (x > xmax) code |= CS_RIGHT;
    if      (y < ymin) code |= CS_BOTTOM;
    else if (y > ymax) code |= CS_TOP;
    return code;
}

static int cohenSutherland(float *x1, float *y1, float *x2, float *y2,
                           float xmin, float xmax,
                           float ymin, float ymax)
{
    int outcode0 = csOutcode(*x1, *y1, xmin, xmax, ymin, ymax);
    int outcode1 = csOutcode(*x2, *y2, xmin, xmax, ymin, ymax);
    int accept   = 0;

    /* Save original coordinates to prevent float precision drift and infinite loops */
    float ox1 = *x1, oy1 = *y1;
    float ox2 = *x2, oy2 = *y2;
    float dx = ox2 - ox1;
    float dy = oy2 - oy1;
    if (dx == 0.f) dx = 0.0001f;
    if (dy == 0.f) dy = 0.0001f;

    for (int iter = 0; iter < 8; iter++) {
        if (!(outcode0 | outcode1)) {
            accept = 1; break;
        } else if (outcode0 & outcode1) {
            break;
        } else {
            float x = 0.f, y = 0.f;
            int outcodeOut = outcode0 ? outcode0 : outcode1;

            /* Use dx and dy from original points so the slope never drifts! */
            if (outcodeOut & CS_TOP) {
                x = ox1 + dx * (ymax - oy1) / dy;
                y = ymax;
            } else if (outcodeOut & CS_BOTTOM) {
                x = ox1 + dx * (ymin - oy1) / dy;
                y = ymin;
            } else if (outcodeOut & CS_RIGHT) {
                y = oy1 + dy * (xmax - ox1) / dx;
                x = xmax;
            } else if (outcodeOut & CS_LEFT) {
                y = oy1 + dy * (xmin - ox1) / dx;
                x = xmin;
            }
            if (outcodeOut == outcode0) {
                *x1 = x; *y1 = y;
                outcode0 = csOutcode(*x1, *y1, xmin, xmax, ymin, ymax);
            } else {
                *x2 = x; *y2 = y;
                outcode1 = csOutcode(*x2, *y2, xmin, xmax, ymin, ymax);
            }
        }
    }
    return accept;
}

// --- TEXT ---

static void bitmapText(float x, float y, void *font, const char *s) {
    glRasterPos2f(x, y);
    while (*s) glutBitmapCharacter(font, *s++);
}

static void strokeText(float x, float y, float scale, const char *s) {
    glPushMatrix();
        glTranslatef(x, y, 0.f);
        glScalef(scale, scale, 1.f);
        while (*s) glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, *s++);
    glPopMatrix();
}

static float strokeWidth(const char *s, float scale) {
    int w = 0;
    while (*s) w += glutStrokeWidth(GLUT_STROKE_MONO_ROMAN, *s++);
    return w * scale;
}

// --- SOUND SYSTEM ---
// *  Generates PCM WAV data in memory and plays via PlaySound.
// No external WAV files required.

#define PI2  6.28318530f

// --- ASYNC AUDIO SYSTEM (SINGLE PERSISTENT WORKER THREAD) ---
// *  MCI playback is tied to the calling thread. If a thread exits,
// its playback is aborted. Therefore, we use a single persistent
// worker thread and a command queue to handle all audio without
// lagging the main game loop and without sounds cutting off.

#define MAX_AUDIO_CMDS 256
static char g_audioQueue[MAX_AUDIO_CMDS][128];
static int  g_audioQueueHead = 0;
static int  g_audioQueueTail = 0;
static CRITICAL_SECTION g_audioCS;
static HANDLE g_audioEvent;

static DWORD WINAPI audioWorkerThread(LPVOID lpParam) {
    (void)lpParam;
    while (1) {
        char cmd[128] = {0};
        
        EnterCriticalSection(&g_audioCS);
        int empty = (g_audioQueueHead == g_audioQueueTail);
        LeaveCriticalSection(&g_audioCS);
        
        if (empty) {
            /* Wait up to 1500ms. If timeout, we do BGM maintenance. */
            WaitForSingleObject(g_audioEvent, 1500);
        }
        
        EnterCriticalSection(&g_audioCS);
        if (g_audioQueueHead != g_audioQueueTail) {
            strcpy(cmd, g_audioQueue[g_audioQueueHead]);
            g_audioQueueHead = (g_audioQueueHead + 1) % MAX_AUDIO_CMDS;
        }
        LeaveCriticalSection(&g_audioCS);
        
        if (cmd[0] != '\0') {
            mciSendStringA(cmd, NULL, 0, NULL);
        } else {
            /* Idle timeout (1.5s passed without commands): BGM maintenance */
            char buf[32];
            int state = g_state; 
            if (state == STATE_TITLE) {
                buf[0] = '\0';
                mciSendStringA("status home mode", buf, sizeof(buf), NULL);
                if (strcmp(buf, "playing") != 0) {
                    mciSendStringA("seek home to start", NULL, 0, NULL);
                    mciSendStringA("play home",   NULL, 0, NULL);
                }
            } else if (state == STATE_PLAYING || state == STATE_PAUSED) {
                buf[0] = '\0';
                mciSendStringA("status ambient mode", buf, sizeof(buf), NULL);
                if (strcmp(buf, "playing") != 0 && strcmp(buf, "paused") != 0) {
                    mciSendStringA("seek ambient to start", NULL, 0, NULL);
                    mciSendStringA("play ambient",   NULL, 0, NULL);
                }
            }
        }
    }
    return 0;
}

static void sendAudioCmd(const char* cmd) {
    if (!cmd) return;
    EnterCriticalSection(&g_audioCS);
    int nextTail = (g_audioQueueTail + 1) % MAX_AUDIO_CMDS;
    if (nextTail != g_audioQueueHead) {
        strncpy(g_audioQueue[g_audioQueueTail], cmd, 127);
        g_audioQueue[g_audioQueueTail][127] = '\0';
        g_audioQueueTail = nextTail;
    }
    LeaveCriticalSection(&g_audioCS);
    SetEvent(g_audioEvent);
}

static void playSound(int id) {
    char cmd[128];
    switch (id) {
        case SFX_FLAP: {
            static int flapIdx = 0;
            snprintf(cmd, sizeof(cmd), "play flap%d from 0", flapIdx);
            sendAudioCmd(cmd);
            flapIdx = (flapIdx + 1) % 3;
            break;
        }
        case SFX_COIN: {
            static int coinIdx = 0;
            snprintf(cmd, sizeof(cmd), "play coin%d from 0", coinIdx);
            sendAudioCmd(cmd);
            coinIdx = (coinIdx + 1) % 2;
            break;
        }
        case SFX_DIE:
            sendAudioCmd("play sfx_die from 0");
            break;
        case SFX_START:
            sendAudioCmd("play sfx_start from 0");
            break;
        case SFX_THUNDER1:
            sendAudioCmd("play sfx_thunder1 from 0");
            break;
        case SFX_THUNDER2:
            sendAudioCmd("play sfx_thunder2 from 0");
            break;
    }
}

static void initSounds(void) {
    /* Initialize command queue and start the persistent worker thread first */
    InitializeCriticalSection(&g_audioCS);
    g_audioEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    CloseHandle(CreateThread(NULL, 0, audioWorkerThread, NULL, 0, NULL));

    /* Open all aliases IN THE WORKER THREAD so they are owned by it!
       We append 'type mpegvideo' to all files to route them through DirectShow.
       This ensures that downloaded sounds (which might be 24-bit, 32-bit, or disguised MP3s)
       play flawlessly, avoiding the strict limitations of the legacy waveaudio codec. */
    sendAudioCmd("open \"D:\\bird_game\\asset\\sound\\jump.wav\"         alias flap0");
    sendAudioCmd("open \"D:\\bird_game\\asset\\sound\\jump.wav\"         alias flap1");
    sendAudioCmd("open \"D:\\bird_game\\asset\\sound\\jump.wav\"         alias flap2");
    
    sendAudioCmd("open \"D:\\bird_game\\asset\\sound\\collect-coin.wav\" alias coin0");
    sendAudioCmd("open \"D:\\bird_game\\asset\\sound\\collect-coin.wav\" alias coin1");

    sendAudioCmd("open \"D:\\bird_game\\asset\\sound\\game-over.wav\"    alias sfx_die");
    sendAudioCmd("open \"D:\\bird_game\\asset\\sound\\game-start.wav\"   alias sfx_start");
    sendAudioCmd("open \"D:\\bird_game\\asset\\sound\\thunder-1.wav\"    alias sfx_thunder1");
    sendAudioCmd("open \"D:\\bird_game\\asset\\sound\\thunder-2.wav\"    alias sfx_thunder2");

    sendAudioCmd("open \"D:\\bird_game\\asset\\sound\\home-page.wav\"    alias home");
    
    /* Start the home page BGM through the queue */
    sendAudioCmd("play home");
}

// --- INIT: SUB-SYSTEMS ---

static void initClouds(void) {
    for (int i = 0; i < CLOUD_COUNT; i++) {
        g_clouds[i].layer = i % 2;
        g_clouds[i].x     = randf(0.f, WORLD_W);
        g_clouds[i].y     = randf(GROUND_Y + GROUND_H + 70.f, WORLD_H - 50.f);
        g_clouds[i].scale = randf(0.9f, 1.7f);
        g_clouds[i].speed = (g_clouds[i].layer == 0) ? CLOUD_SPEED_FAR : CLOUD_SPEED_NEAR;
    }
}

static void initBuildings(void) {
    float pal[4][3] = {{0.25f,0.35f,0.50f},{0.20f,0.30f,0.45f},
                       {0.30f,0.40f,0.55f},{0.18f,0.28f,0.42f}};
    float x = 0.f;
    for (int i = 0; i < BUILDING_COUNT; i++) {
        float w = randf(40.f, 80.f); float h = randf(40.f, 140.f);
        int   p = rand() % 4;
        g_buildings[i].x = x; g_buildings[i].w = w + 4.f; g_buildings[i].h = h;
        g_buildings[i].r = pal[p][0]; g_buildings[i].g = pal[p][1]; g_buildings[i].b = pal[p][2];
        x += w - randf(0.f, 20.f);
    }
}

static void initRain(void) {
    for (int i = 0; i < RAIN_COUNT; i++) {
        g_rain[i].x     = randf(0.f, WORLD_W);
        g_rain[i].y     = randf(0.f, WORLD_H);
        g_rain[i].speed = randf(9.f, 16.f);
        g_rain[i].alpha = randf(0.35f, 0.75f);
        g_rain[i].len   = randf(10.f, 20.f);
    }
}

static void initStars(void) {
    for (int i = 0; i < STAR_COUNT; i++) {
        g_stars[i].x     = randf(0.f, WORLD_W);
        g_stars[i].y     = randf(WORLD_H * 0.4f, WORLD_H);
        g_stars[i].size  = randf(1.f, 2.5f);
        g_stars[i].phase = randf(0.f, PI2);
    }
}

static void initSnow(void) {
    for (int i = 0; i < 180; i++) {
        g_snow[i].x     = randf(0.f, WORLD_W);
        g_snow[i].y     = randf(0.f, WORLD_H);
        g_snow[i].speed = randf(1.5f, 4.0f);
        g_snow[i].size  = randf(2.f, 5.f);
        g_snow[i].drift = randf(-1.f, 1.f);
    }
}

static void initShootingStars(void) {
    for (int i = 0; i < 3; i++) { g_shootingStars[i].active = 0; }
}

static void initBird(void) {
    g_bird.y = WORLD_H / 2.f; g_bird.vy = 0.f; g_bird.angle = 0.f;
    g_bird.wingFrame = 0; g_bird.wingTimer = 0; g_bird.alive = 1;
}

static void initPipes(void) {
    float minC = GROUND_Y + GROUND_H * GRASS_H_RATIO + PIPE_MIN_H + g_pipeGap / 2.f;
    float maxC = WORLD_H - PIPE_MIN_H - g_pipeGap / 2.f;
    for (int i = 0; i < PIPE_COUNT; i++) {
        g_pipes[i].x          = 900.f + i * PIPE_SPACING;
        g_pipes[i].gapCenterY = randf(minC, maxC);
        g_pipes[i].scored     = 0;
        g_coins[i].x          = g_pipes[i].x + PIPE_W / 2.f;
        g_coins[i].y          = g_pipes[i].gapCenterY;
        g_coins[i].spinAngle  = 0.f;
        g_coins[i].collected  = 0;
    }
}

// --- GAME LIFECYCLE ---

static void updateWeatherAmbient(void);

static void resetGame(void) {
    g_score         = 0;
    g_coinScore     = 0;
    g_coinPopupTimer = 0;
    if (g_difficulty == DIFF_EASY) {
        g_pipeSpeed = 1.8f;
        g_pipeGap   = 230.0f;
    } else if (g_difficulty == DIFF_HARD) {
        g_pipeSpeed = 4.0f;
        g_pipeGap   = 140.0f;
    } else { /* Normal */
        g_pipeSpeed = 2.7f;
        g_pipeGap   = 190.0f;
    }
    g_shakeTicks = 0; g_flashTicks = 0; g_groundScroll = 0.f;
    g_gameOverDelay = 0;
    g_shearX    = 0.f;
    g_hintTimer = 360;  /* show controls box for ~6 s at 60 fps */
    g_menuExpanded = 0; /* Auto-close menu when starting */
    initBird(); initPipes();
    g_state = STATE_PLAYING;
    sendAudioCmd("stop home"); /* Stop home page BGM */
    playSound(SFX_START);
    updateWeatherAmbient(); /* Start ambient BGM */
}

static void loadHighScore(void) {
    FILE *f = fopen("highscore.txt", "r");
    if (f) {
        fscanf(f, "%d", &g_highScore);
        fclose(f);
    }
}

static void saveHighScore(void) {
    FILE *f = fopen("highscore.txt", "w");
    if (f) {
        fprintf(f, "%d\n", g_highScore);
        fclose(f);
    }
}

static void init(void) {
    srand((unsigned int)time(NULL));
    glClearColor(0.05f, 0.10f, 0.25f, 1.f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(2.f);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluOrtho2D(0.0, WORLD_W, 0.0, WORLD_H);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();

    initClouds(); initBuildings(); initBird(); initPipes();
    initRain(); initStars(); initSnow(); initShootingStars();
    loadHighScore();

    for (int i = 0; i < 8; i++) g_boltSegs[i] = randf(-25.f, 25.f);

    initSounds();

    g_state           = STATE_TITLE;
    g_weather         = WEATHER_DAY;
    g_weatherTimer    = 0;
    g_hoveredWeather  = -1;
    g_hoveredPlayAgain = 0;
}

// --- SCREEN SHAKE ---

static void triggerShake(int t) { g_shakeTicks = t; }

static void updateShake(void) {
    if (g_shakeTicks > 0) {
        float m = 6.f * ((float)g_shakeTicks / 15.f);
        g_shakeX = randf(-m, m); g_shakeY = randf(-m, m);
        g_shakeTicks--;
    } else { g_shakeX = g_shakeY = 0.f; }
}

// --- WEATHER SYSTEM ---

static void updateWeatherAmbient(void) {
    sendAudioCmd("close ambient");
    if      (g_weather == WEATHER_RAIN)  sendAudioCmd("open \"D:\\bird_game\\asset\\sound\\rain.wav\" alias ambient");
    else if (g_weather == WEATHER_NIGHT) sendAudioCmd("open \"D:\\bird_game\\asset\\sound\\night-crickets.wav\" alias ambient");
    else if (g_weather == WEATHER_DAY)   sendAudioCmd("open \"D:\\bird_game\\asset\\sound\\day-birds.wav\" alias ambient");
    else if (g_weather == WEATHER_SUNNY) sendAudioCmd("open \"D:\\bird_game\\asset\\sound\\morning-birds.wav\" alias ambient");
    else if (g_weather == WEATHER_SNOW)  sendAudioCmd("open \"D:\\bird_game\\asset\\sound\\snow.mp3\" alias ambient");
    
    sendAudioCmd("play ambient");
}


static void nextWeather(void) {
    g_weather         = static_cast<WeatherMode>((g_weather + 1) % WEATHER_COUNT);
    g_weatherTimer    = 0;
    g_weatherNameTimer = 150;
    g_wFlashTicks     = 12;
    initRain();
    updateWeatherAmbient();
}

static void setWeather(WeatherMode w) {
    g_weather          = w;
    g_weatherTimer     = 0;
    g_weatherNameTimer = 120;
    g_wFlashTicks      = 10;
    initRain();
    updateWeatherAmbient();
}

static void updateWeather(void) {
    g_weatherTimer++;
    /* Weather no longer auto-cycles — only changes on explicit user input */
    if (g_weatherNameTimer > 0) g_weatherNameTimer--;
    if (g_wFlashTicks > 0) g_wFlashTicks--;

    /* Lightning in rain mode */
    if (g_weather == WEATHER_RAIN) {
        if (g_lightning > 0) {
            g_lightning--;
        } else if (rand() % 240 == 0) {
            g_lightning = 10;
            g_boltX = randf(80.f, WORLD_W - 80.f);
            for (int i = 0; i < 8; i++) g_boltSegs[i] = randf(-25.f, 25.f);
            playSound(rand() % 2 == 0 ? SFX_THUNDER1 : SFX_THUNDER2);
        }
    } else { g_lightning = 0; }
}

// --- DRAW: SKY BACKGROUND ---

static void drawBackground(void) {
    const WeatherTheme *t = &g_themes[g_weather];

    glBegin(GL_QUADS);
        colF(t->botR, t->botG, t->botB);
        glVertex2f(0.f, 0.f);
        glVertex2f(WORLD_W, 0.f);
        colF(t->topR, t->topG, t->topB);
        glVertex2f(WORLD_W, WORLD_H);
        glVertex2f(0.f, WORLD_H);
    glEnd();
}

// --- DRAW: SUN  (Day + Sunny) ---

static void drawSun(void) {
    float cx = WORLD_W - 90.f, cy = WORLD_H - 80.f, r = 32.f;

    /* Outer glow rings (expanded and enhanced for beauty) */
    col4(255, 230, 80, 20);  fillCircle(cx, cy, r * 3.5f, 24);
    col4(255, 235, 100, 30); fillCircle(cx, cy, r * 2.5f, 22);
    col4(255, 240, 120, 50); fillCircle(cx, cy, r * 1.6f, 20);

    /* Rays */
    int rayN = 16;
    for (int i = 0; i < rayN; i++) {
        float a     = PI2 * i / rayN + g_frame * 0.005f;
        float pulse = 1.f + 0.25f * sinf(g_frame * 0.04f + i * 0.5f);
        float r1 = r + 8.f, r2 = r + 28.f * pulse;
        glLineWidth(3.5f);
        col4(255, 220, 40, 180);
        glBegin(GL_LINES);
            glVertex2f(cx + cosf(a) * r1, cy + sinf(a) * r1);
            glVertex2f(cx + cosf(a) * r2, cy + sinf(a) * r2);
        glEnd();
        glLineWidth(1.f);
    }

    /* Body */
    col(255, 240, 60); fillCircle(cx, cy, r, 24);
    /* Soft inner gradient / highlight */
    col(255, 250, 150); fillCircle(cx - r * 0.15f, cy + r * 0.15f, r * 0.7f, 20);
    col(255, 255, 220); fillCircle(cx - r * 0.25f, cy + r * 0.25f, r * 0.4f, 16);
}

// --- DRAW: MOON  (Night) ---

static void drawMoon(void) {
    float cx = WORLD_W - 95.f, cy = WORLD_H - 75.f, r = 26.f;
    /* Glow */
    col4(200, 210, 255, 18); fillCircle(cx, cy, r * 2.8f, 20);
    col4(210, 220, 255, 30); fillCircle(cx, cy, r * 1.8f, 20);
    /* Moon */
    col(250, 248, 210); fillCircle(cx, cy, r, 22);
    /* Craters */
    col(225, 218, 185);
    fillCircle(cx + r * 0.30f, cy + r * 0.22f, r * 0.22f, 12);
    fillCircle(cx - r * 0.25f, cy - r * 0.28f, r * 0.14f, 10);
    fillCircle(cx + r * 0.05f, cy - r * 0.10f, r * 0.10f, 10);
    /* Crescent cutout using sky colour */
    const WeatherTheme *th = &g_themes[WEATHER_NIGHT];
    glColor3f(th->topR, th->topG, th->topB);
    fillCircle(cx + r * 0.40f, cy, r * 0.82f, 18);
}

// --- DRAW: STARS  (Night) ---

static void drawStars(void) {
    for (int i = 0; i < STAR_COUNT; i++) {
        float tw = 0.55f + 0.45f * sinf(g_frame * 0.04f + g_stars[i].phase);
        col4(255, 255, 225, (int)(tw * 245));
        float sz = g_stars[i].size;
        if (sz > 2.0f) {
            glBegin(GL_LINES);
                glVertex2f(g_stars[i].x - sz * 1.8f, g_stars[i].y);
                glVertex2f(g_stars[i].x + sz * 1.8f, g_stars[i].y);
                glVertex2f(g_stars[i].x, g_stars[i].y - sz * 1.8f);
                glVertex2f(g_stars[i].x, g_stars[i].y + sz * 1.8f);
            glEnd();
        }
        fillCircle(g_stars[i].x, g_stars[i].y, sz, 6);
    }
}

// --- DRAW: RAIN ---

static void drawRain(void) {
    glLineWidth(1.5f);
    for (int i = 0; i < RAIN_COUNT; i++) {
        col4(180, 205, 230, (int)(g_rain[i].alpha * 210));
        float x1 = g_rain[i].x, y1 = g_rain[i].y, len = g_rain[i].len;
        float x2 = x1 - len * 0.28f, y2 = y1 - len;
        /* ALGORITHM 5: Cohen-Sutherland Line Clipping for rain drops */
        if (cohenSutherland(&x1, &y1, &x2, &y2,
                            0.f, WORLD_W,
                            GROUND_Y + GROUND_H, WORLD_H - 20.f)) {
            glBegin(GL_LINES);
                glVertex2f(x1, y1);
                glVertex2f(x2, y2);
            glEnd();
        }
    }
    glLineWidth(2.f);
}

// --- DRAW: LIGHTNING ---

static void drawLightning(void) {
    if (g_lightning <= 0) return;
    float a = (float)g_lightning / 10.f;

    col4(210, 225, 255, (int)(a * 65)); fillRect(0, 0, WORLD_W, WORLD_H);

    float bx   = g_boltX, by = WORLD_H;
    float segH = (WORLD_H - GROUND_Y - GROUND_H) * 0.13f;

    /* ALGORITHM 3 — Bresenham Line: outer glow pass */
    col4(180, 210, 255, (int)(a * 80));
    glPointSize(6.5f);
    for (int i = 0; i < 8; i++) {
        float nx = bx + g_boltSegs[i], ny = by - segH;
        bresenhamLine((int)bx, (int)by, (int)nx, (int)ny);
        bx = nx; by = ny;
    }

    /* ALGORITHM 3 — Bresenham Line: inner bright core pass */
    bx = g_boltX; by = WORLD_H;
    col4(240, 248, 255, (int)(a * 240));
    glPointSize(2.5f);
    for (int i = 0; i < 8; i++) {
        float nx = bx + g_boltSegs[i], ny = by - segH;
        bresenhamLine((int)bx, (int)by, (int)nx, (int)ny);
        bx = nx; by = ny;
    }
    glPointSize(1.0f);
}

// --- DRAW: FOG  (Rain) ---

static void drawFog(void) {
    for (int i = 0; i < 3; i++) {
        float fy = GROUND_Y + GROUND_H + (i * 60.f);
        col4(160, 170, 185, 26 - i * 4);
        fillRect(0, fy, WORLD_W, 80.f);
    }
    col4(140, 155, 170, 16); fillRect(0, 0, WORLD_W, WORLD_H);
}

// --- DRAW: BEAUTIFUL CLOUDS  (8-blob per cloud) ---

static void drawCloud(float cx, float cy, float sc) {
    static const float body[7][4] = {
        {  0.f,  0.f, 34.f, 22.f },
        { 30.f,  4.f, 26.f, 18.f },
        {-30.f,  2.f, 24.f, 16.f },
        { 15.f, 16.f, 22.f, 16.f },
        {-15.f, 14.f, 20.f, 15.f },
        { 48.f, -2.f, 16.f, 12.f },
        {-46.f, -1.f, 15.f, 11.f },
    };
    const WeatherTheme *t = &g_themes[g_weather];

    /* Shadow */
    colF4(t->cldR * 0.70f, t->cldG * 0.70f, t->cldB * 0.76f, t->cldA * 0.50f);
    fillEllipse(cx, cy - 6.f * sc, 36.f * sc, 8.f * sc, 14);

    /* Body blobs */
    colF4(t->cldR, t->cldG, t->cldB, t->cldA);
    for (int i = 0; i < 7; i++)
        fillEllipse(cx + body[i][0] * sc, cy + body[i][1] * sc,
                    body[i][2] * sc, body[i][3] * sc, 16);

    /* Depth tint */
    colF4(t->cldR * 0.88f, t->cldG * 0.88f, t->cldB * 0.90f, t->cldA * 0.48f);
    fillEllipse(cx, cy, 30.f * sc, 18.f * sc, 14);

    /* Top-left highlight */
    colF4(1.f, 1.f, 1.f, t->cldA * 0.70f);
    fillEllipse(cx - 8.f * sc, cy + 12.f * sc, 16.f * sc, 9.f * sc, 12);

    /* Inner glow */
    colF4(1.f, 1.f, 1.f, t->cldA * 0.24f);
    fillEllipse(cx, cy + 4.f * sc, 22.f * sc, 14.f * sc, 12);
}

static void drawClouds(void) {
    for (int i = 0; i < CLOUD_COUNT; i++)
        drawCloud(g_clouds[i].x, g_clouds[i].y, g_clouds[i].scale);
}

// --- SNOW UPDATE & DRAW ---

static void updateSnow(void) {
    for (int i = 0; i < 180; i++) {
        g_snow[i].y -= g_snow[i].speed;
        g_snow[i].x += g_snow[i].drift + sinf(g_frame * 0.05f + i) * 0.5f;
        if (g_snow[i].y < GROUND_Y) {
            g_snow[i].y = WORLD_H + 10.f;
            g_snow[i].x = randf(0.f, WORLD_W);
        }
    }
}

static void updateShootingStars(void) {
    for (int i = 0; i < 3; i++) {
        if (g_shootingStars[i].active) {
            g_shootingStars[i].x += g_shootingStars[i].vx;
            g_shootingStars[i].y += g_shootingStars[i].vy;
            g_shootingStars[i].life--;
            if (g_shootingStars[i].life <= 0) g_shootingStars[i].active = 0;
        } else if (rand() % 300 == 0) {
            g_shootingStars[i].active = 1;
            g_shootingStars[i].x     = randf(WORLD_W * 0.5f, WORLD_W + 50.f);
            g_shootingStars[i].y     = randf(WORLD_H * 0.6f, WORLD_H + 50.f);
            g_shootingStars[i].vx   = randf(-15.f, -8.f);
            g_shootingStars[i].vy   = randf(-10.f, -5.f);
            g_shootingStars[i].len  = randf(40.f, 100.f);
            g_shootingStars[i].life = 60;
        }
    }
}

static void drawSnow(void) {
    for (int i = 0; i < 180; i++) {
        col4(255, 255, 255, 220);
        glPointSize(g_snow[i].size);
        glBegin(GL_POINTS);
            glVertex2f(g_snow[i].x, g_snow[i].y);
        glEnd();
    }
    glPointSize(1.f);
}

static void drawShootingStars(void) {
    glLineWidth(2.5f);
    for (int i = 0; i < 3; i++) {
        if (g_shootingStars[i].active) {
            float alpha = (float)g_shootingStars[i].life / 60.f;
            col4(255, 255, 255, (int)(alpha * 255));
            glBegin(GL_LINES);
            glVertex2f(g_shootingStars[i].x, g_shootingStars[i].y);
            col4(255, 255, 255, 0);
            float dx  = g_shootingStars[i].vx;
            float dy  = g_shootingStars[i].vy;
            float mag = sqrtf(dx * dx + dy * dy);
            glVertex2f(g_shootingStars[i].x - (dx / mag) * g_shootingStars[i].len,
                       g_shootingStars[i].y - (dy / mag) * g_shootingStars[i].len);
            glEnd();
        }
    }
    glLineWidth(2.0f);
}

static void drawMountains(void) {
    const WeatherTheme *t = &g_themes[g_weather];
    /* Layer 1 - back */
    colF(t->botR * 0.5f, t->botG * 0.5f, t->botB * 0.5f);
    float scroll1 = g_groundScroll * 0.05f;
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(WORLD_W, GROUND_Y);
    glVertex2f(0, GROUND_Y);
    for (int i = 0; i <= 6; i++) {
        float x = (WORLD_W / 5.f) * i - fmodf(scroll1, WORLD_W / 5.f);
        float y = GROUND_Y + GROUND_H + 150.f + sinf(i * 123.45f) * 100.f;
        glVertex2f(x, y);
    }
    glEnd();

    /* Layer 2 - mid */
    colF(t->botR * 0.65f, t->botG * 0.65f, t->botB * 0.65f);
    float scroll2 = g_groundScroll * 0.1f;
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(WORLD_W, GROUND_Y);
    glVertex2f(0, GROUND_Y);
    for (int i = 0; i <= 8; i++) {
        float x = (WORLD_W / 7.f) * i - fmodf(scroll2, WORLD_W / 7.f);
        float y = GROUND_Y + GROUND_H + 80.f + sinf(i * 321.12f) * 60.f;
        glVertex2f(x, y);
    }
    glEnd();

    /* Layer 3 - front */
    colF(t->botR * 0.8f, t->botG * 0.8f, t->botB * 0.8f);
    float scroll3 = g_groundScroll * 0.2f;
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(WORLD_W, GROUND_Y);
    glVertex2f(0, GROUND_Y);
    for (int i = 0; i <= 10; i++) {
        float x = (WORLD_W / 9.f) * i - fmodf(scroll3, WORLD_W / 9.f);
        float y = GROUND_Y + GROUND_H + 30.f + sinf(i * 555.55f) * 40.f;
        glVertex2f(x, y);
    }
    glEnd();
}

static void drawTrees(void) {
    float baseY  = GROUND_Y + GROUND_H;
    float scroll = fmodf(g_groundScroll, 100.f);
    for (int i = -1; i <= 8; i++) {
        float x = i * 100.f - scroll + 50.f;
        /* Trunk */
        col(100, 70, 40);
        fillRect(x - 4.f, baseY, 8.f, 30.f);
        /* Canopy */
        if      (g_weather == WEATHER_SNOW)  col(200, 220, 240);
        else if (g_weather == WEATHER_NIGHT) col(30, 60, 30);
        else                                 col(40, 140, 50);
        float anim = sinf(g_frame * 0.05f + i) * 2.f;
        fillCircle(x + anim, baseY + 30.f, 20.f, 12);

        if (g_weather == WEATHER_SNOW) {
            col(255, 255, 255);
            fillCircle(x + anim, baseY + 42.f, 12.f, 8);
        }
    }
}

static void drawRainbow(void) {
    float cx = WORLD_W * 0.65f; // Place rainbow behind buildings
    float cy = GROUND_Y + GROUND_H;
    float colors[6][3] = {
        {255.f,  0.f,   0.f},
        {255.f, 127.f,  0.f},
        {255.f, 255.f,  0.f},
        {  0.f, 255.f,  0.f},
        {  0.f,   0.f, 255.f},
        {139.f,   0.f, 255.f}
    };
    glLineWidth(12.f);
    for (int i = 0; i < 6; i++) {
        col4((int)colors[i][0], (int)colors[i][1], (int)colors[i][2], 100);
        float r = 130.f - i * 10.f;
        glBegin(GL_LINE_STRIP);
        for (int a = 0; a <= 20; a++) {
            float ang = 3.1415926535f * a / 20.f;
            glVertex2f(cx + cosf(ang) * r, cy + sinf(ang) * r);
        }
        glEnd();
    }
    glLineWidth(2.f);
}

static void drawCitySilhouette(void) {
    float gs   = GROUND_Y + GROUND_H * GRASS_H_RATIO;
    float dark = g_themes[g_weather].darkness;
    for (int i = 0; i < BUILDING_COUNT; i++) {
        float bx = g_buildings[i].x, bw = g_buildings[i].w;
        float bh = g_buildings[i].h, by = gs - 2.f;
        glColor3f(g_buildings[i].r * (1.f - dark),
                  g_buildings[i].g * (1.f - dark),
                  g_buildings[i].b * (1.f - dark * 0.7f));
        fillRect(bx, by, bw, bh);

        float wr, wg, wb, wa;
        if (g_weather == WEATHER_NIGHT) { wr=1.0f; wg=0.85f; wb=0.45f; wa=0.90f; }
        else                            { wr=0.84f;wg=0.93f; wb=1.0f;  wa=0.70f; }
        float wSz = 5.f, wGX = 14.f, wGY = 18.f;
        for (float wy = by + 10.f; wy + wSz < by + bh - 10.f; wy += wGY)
            for (float wx = bx + 8.f; wx + wSz < bx + bw - 8.f; wx += wGX)
                if ((int)(wx * 7 + wy * 13) % 3 != 0) {
                    if (g_weather == WEATHER_NIGHT) {
                        col4(255, 200, 80, 32);
                        fillRect(wx - 2.f, wy - 2.f, wSz + 4.f, wSz + 4.f);
                    }
                    colF4(wr, wg, wb, wa);
                    fillRect(wx, wy, wSz, wSz);
                }
    }
}

// --- DRAW: GROUND ---

static void drawGround(void) {
    float grassH = GROUND_H * GRASS_H_RATIO;
    const WeatherTheme *t = &g_themes[g_weather];

    if      (g_weather == WEATHER_NIGHT) col(100, 80, 55);
    else if (g_weather == WEATHER_RAIN)  col(150, 120, 90);
    else                                 col(222, 184, 135);
    fillRect(0, 0, WORLD_W, GROUND_Y);

    colF(t->grassR, t->grassG, t->grassB);
    fillRect(0, GROUND_Y, WORLD_W, grassH);

    colF(t->grassR * 0.82f, t->grassG * 0.82f, t->grassB * 0.82f);
    float tileW  = 40.f;
    float scroll = fmodf(g_groundScroll, tileW * 2.f);
    for (float tx = -tileW * 2.f + scroll; tx < WORLD_W + tileW; tx += tileW * 2.f)
        fillRect(tx, GROUND_Y + 4.f, tileW - 4.f, 8.f);

    colF(t->grassR * 0.65f, t->grassG * 0.65f, t->grassB * 0.65f);
    fillRect(0, GROUND_Y + grassH - 2.f, WORLD_W, 3.f);

    /* ALGORITHM 4: Cubic Bezier Curve (water wave) */
    drawBezierWave();

    /* Rain puddles */
    if (g_weather == WEATHER_RAIN) {
        col4(120, 145, 175, 90);
        for (int i = 0; i < 8; i++) {
            float px = fmodf(i * 137.f + 22.f, WORLD_W - 40.f);
            float pw = 20.f + fmodf(i * 53.f, 30.f);
            fillEllipse(px + pw / 2.f, GROUND_Y - 4.f, pw / 2.f, 5.f, 12);
        }
    }

    /* Pebbles */
    col4(170, 140, 100, 175);
    for (int i = 0; i < 20; i++) {
        float px = fmodf(i * 123.7f + 37.f, WORLD_W);
        float py = 12.f + fmodf(i * 71.3f, GROUND_Y - 20.f);
        fillRect(px, py, 5.f, 3.f);
    }
}

// --- 3D PROJECTION HELPERS ---
// *  begin3D / end3D switch from global glOrtho to gluPerspective
// (same approach as the lab code) for 3D element rendering.

static void begin3D(float worldX, float worldY, int vpSize) {
    float sx = g_vpX + (worldX / WORLD_W) * (float)g_vpW;
    float sy = g_vpY + (worldY / WORLD_H) * (float)g_vpH;
    glViewport((int)(sx - vpSize), (int)(sy - vpSize),
               vpSize * 2, vpSize * 2);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluPerspective(60.0, 1.0, 0.1, 100.0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    gluLookAt(0.0, 0.0, 5.0,  0.0, 0.0, 0.0,  0.0, 1.0, 0.0);
}

static void end3D(void) {
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glViewport(g_vpX, g_vpY, g_vpW, g_vpH);
}

// --- DRAW: 3D WHITE FLUFFY CHICK ---
// *  Matches the reference image: round white body, big black eyes
// with specular highlights, small golden beak, short orange legs.
// *  Technique: gluPerspective + gluLookAt + GL_LIGHTING (lab style)
// glRotatef  -- Z-axis tilt based on velocity
// glTranslatef / glScalef -- position all sub-parts

static void setupChickLight(void) {
    GLfloat pos[]  = { -2.0f,  3.0f,  5.0f,  1.0f };
    GLfloat diff[] = {  1.0f,  1.0f,  1.0f,  1.0f };
    GLfloat amb[]  = {  0.45f, 0.45f, 0.45f, 1.0f };
    GLfloat spec[] = {  0.6f,  0.6f,  0.6f,  1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, pos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  diff);
    glLightfv(GL_LIGHT0, GL_AMBIENT,  amb);
    glLightfv(GL_LIGHT0, GL_SPECULAR, spec);
}

static void setMat(float dr, float dg, float db,
                   float ar, float ag, float ab,
                   float sr, float sg, float sb,
                   float shine)
{
    GLfloat d[]  = { dr, dg, db, 1.0f };
    GLfloat a[]  = { ar, ag, ab, 1.0f };
    GLfloat s[]  = { sr, sg, sb, 1.0f };
    GLfloat sh[] = { shine };
    glMaterialfv(GL_FRONT, GL_DIFFUSE,   d);
    glMaterialfv(GL_FRONT, GL_AMBIENT,   a);
    glMaterialfv(GL_FRONT, GL_SPECULAR,  s);
    glMaterialfv(GL_FRONT, GL_SHININESS, sh);
}

static void drawChickGeometry(float tiltAngle, float shearFactor, int wingFrame) {
    glRotatef(-tiltAngle * 0.55f, 0.0f, 0.0f, 1.0f);

    if (shearFactor > 0.001f) {
        float m[16] = {
            1.0f,        0.0f, 0.0f, 0.0f,
            shearFactor, 1.0f, 0.0f, 0.0f,
            0.0f,        0.0f, 1.0f, 0.0f,
            0.0f,        0.0f, 0.0f, 1.0f
        };
        glMultMatrixf(m);
    }

    /* Force material colors to show up properly */
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

    /* 1. Main Body (Yellow, slightly squished) */
    glColor3f(1.0f, 0.9f, 0.1f);
    setMat(1.0f, 0.9f, 0.1f,   0.8f, 0.7f, 0.1f,   0.3f, 0.3f, 0.1f, 20.f);
    glPushMatrix();
        glScalef(1.1f, 0.9f, 1.0f);
        glutSolidSphere(1.0, 30, 30);
    glPopMatrix();

    /* 2. Eye (White sphere on the side facing the camera: +Z) */
    glPushMatrix();
        glTranslatef(0.40f, 0.25f, 0.95f); /* Pushed out more on Z */
        glColor3f(1.0f, 1.0f, 1.0f);
        setMat(1.0f, 1.0f, 1.0f,   0.9f, 0.9f, 0.9f,   1.0f, 1.0f, 1.0f, 80.f);
        glutSolidSphere(0.35, 20, 20); /* Bigger white eyeball */

        /* Pupil (Black) */
        glTranslatef(0.15f, 0.05f, 0.28f);
        glColor3f(0.1f, 0.1f, 0.1f);
        setMat(0.1f, 0.1f, 0.1f,   0.0f, 0.0f, 0.0f,   0.5f, 0.5f, 0.5f, 100.f);
        glutSolidSphere(0.14, 12, 12);

        /* Specular highlight (Tiny white dot) */
        glTranslatef(0.08f, 0.08f, 0.10f);
        glColor3f(1.0f, 1.0f, 1.0f);
        setMat(1.0f, 1.0f, 1.0f,   1.0f, 1.0f, 1.0f,   1.0f, 1.0f, 1.0f, 128.f);
        glutSolidSphere(0.05, 8, 8);
    glPopMatrix();

    /* 3. Beak (Red protruding to the right: +X) */
    glPushMatrix();
        glTranslatef(1.05f, -0.1f, 0.45f); /* Moved further right to stick out of body */

        /* Top lip */
        glColor3f(1.0f, 0.1f, 0.1f);
        setMat(1.0f, 0.1f, 0.1f,   0.8f, 0.05f, 0.05f,   0.4f, 0.1f, 0.1f, 30.f); /* Bright Red */
        glPushMatrix();
            glTranslatef(0.0f, 0.1f, 0.0f);
            glScalef(0.6f, 0.25f, 0.4f); /* Made beak larger */
            glutSolidSphere(1.0, 20, 20);
        glPopMatrix();

        /* Bottom lip */
        glColor3f(0.8f, 0.05f, 0.05f);
        setMat(0.8f, 0.05f, 0.05f,  0.7f, 0.05f, 0.05f,  0.3f, 0.05f, 0.05f, 30.f); /* Slightly darker Red */
        glPushMatrix();
            glTranslatef(-0.1f, -0.15f, 0.0f);
            glScalef(0.4f, 0.18f, 0.3f);
            glutSolidSphere(1.0, 20, 20);
        glPopMatrix();
    glPopMatrix();

    /* 4. Wing (Orange on the side facing camera: +Z) */
    glPushMatrix();
        glTranslatef(-0.25f, -0.05f, 1.10f); /* Pushed out to Z=1.1 so it's clearly visible */
        /* Flap animation based on wingFrame (0=mid, 1=up, 2=down) */
        float wingAngle = 0.0f;
        if (wingFrame == 1) wingAngle = 45.0f;
        else if (wingFrame == 2) wingAngle = -45.0f;

        glRotatef(wingAngle + tiltAngle * 0.5f, 0.0f, 0.0f, 1.0f);
        glColor3f(1.0f, 0.5f, 0.0f);
        setMat(1.0f, 0.5f, 0.0f,  0.8f, 0.4f, 0.0f,  0.5f, 0.2f, 0.0f, 20.f); /* Orange wing */
        glScalef(0.6f, 0.35f, 0.15f); /* Made wing much larger */
        glutSolidSphere(1.0, 20, 20);
    glPopMatrix();

    glDisable(GL_COLOR_MATERIAL);
}

static void drawBird(void) {
    begin3D(BIRD_X, g_bird.y, 54);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    setupChickLight();
    drawChickGeometry(g_bird.angle, g_shearX, g_bird.wingFrame);
    glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);
    glDisable(GL_DEPTH_TEST);
    end3D();

    /* ALGORITHM 2 - Midpoint Circle outline in 2D ortho overlay */
    glPushMatrix();
        glTranslatef(BIRD_X, g_bird.y, 0.f);
        glRotatef(g_bird.angle, 0.f, 0.f, 1.f);
        col4(200, 200, 200, 55);
        glPointSize(1.5f);
        midpointCircle(0, 0, (int)BIRD_RADIUS);
        glPointSize(1.0f);
    glPopMatrix();
}

// --- DRAW: BIRD REFLECTION  (2D Reflection Transformation) ---
// *  Reflects the bird about the grass surface line (y = grassY).
// Formula:  y' = 2 * grassY - y
// Implemented via glScalef(1, -1, 1) after translating the
// coordinate origin to the reflection axis (grassY).
// Demonstrates the REFLECTION transformation from the syllabus.
static void drawBirdReflection(void) {
    float grassY = GROUND_Y + GROUND_H * GRASS_H_RATIO;
    float reflY  = 2.f * grassY - g_bird.y;

    /* Only visible when bird is close enough for a ground puddle */
    if (reflY > grassY - 1.f || reflY < GROUND_Y - BIRD_RADIUS * 2.f) return;

    glPushMatrix();
        /* Step 1 — Translate to the mirrored world position */
        glTranslatef(BIRD_X, reflY, 0.f);
        /* Step 2 — Flip rotation sign to match the reflection */
        glRotatef(-g_bird.angle, 0.f, 0.f, 1.f);
        /* Step 3 — Apply Y-reflection: scale Y axis by -1 */
        glScalef(1.0f, -1.0f, 1.0f);

        /* Draw a faded silhouette so it looks like a puddle */
        col4(255, 195, 50,  45); fillCircle(0, 0, BIRD_RADIUS, 20);
        col4(200, 120, 20,  45);
        glPointSize(2.5f);
        midpointCircle(0, 0, (int)BIRD_RADIUS);   /* Midpoint Circle outline */
        glPointSize(1.0f);
        col4(255, 255, 255, 30); fillCircle(6.f,   5.f,  6.5f, 14);
        col4( 30,  30,  30, 30); fillCircle(7.5f,  4.5f, 3.5f, 12);
    glPopMatrix();
}

// --- DRAW: SCORE SPARKLE PARTICLES  (GL_POINTS primitive demo) ---
// *  Each live particle is rendered as a single GL_POINT with an
// alpha value tied to its remaining life — clearly demonstrating
// the GL_POINTS primitive type as required by the rubric.
static void drawParticles(void) {
    glPointSize(5.5f);
    glBegin(GL_POINTS);
    for (int i = 0; i < PARTICLE_COUNT; i++) {
        if (!g_particles[i].active) continue;
        glColor4f(g_particles[i].r,
                  g_particles[i].g,
                  g_particles[i].b,
                  g_particles[i].life);
        glVertex2f(g_particles[i].x, g_particles[i].y);
    }
    glEnd();
    glPointSize(1.0f);
}

// --- DRAW: PIPES ---

static void drawSinglePipe(float x, float y1, float y2, int flipped) {
    float dark  = g_themes[g_weather].darkness;
    float dr = 1.f - dark * 0.4f, dg = 1.f - dark * 0.3f, db = 1.f - dark * 0.2f;
    float capOff = 7.f, capW = PIPE_W + 14.f;

    colF(80  / 255.f * dr, 200 / 255.f * dg, 80  / 255.f * db);
    fillRect(x, y1, PIPE_W, y2 - y1);
    colF(120 / 255.f * dr, 230 / 255.f * dg, 100 / 255.f * db);
    fillRect(x + 8.f, y1, 12.f, y2 - y1);
    colF(40  / 255.f * dr, 140 / 255.f * dg, 40  / 255.f * db);
    fillRect(x + PIPE_W - 10.f, y1, 10.f, y2 - y1);
    /* ALGORITHM 1 — DDA Line: draws the 4-edge outline of the pipe body */
    colF(30  / 255.f * dr, 110 / 255.f * dg, 30  / 255.f * db);
    glPointSize(2.0f);
    ddaOutlineRect(x, y1, PIPE_W, y2 - y1);
    glPointSize(1.0f);

    float capY = flipped ? y1 - 4.f : y2 - PIPE_CAP_H, capH = PIPE_CAP_H + 4.f;
    colF(80  / 255.f * dr, 200 / 255.f * dg, 80  / 255.f * db);
    fillRect(x - capOff, capY, capW, capH);
    colF(120 / 255.f * dr, 230 / 255.f * dg, 100 / 255.f * db);
    fillRect(x - capOff + 8.f, capY, 14.f, capH);
    colF(40  / 255.f * dr, 140 / 255.f * dg, 40  / 255.f * db);
    fillRect(x - capOff + capW - 12.f, capY, 12.f, capH);
    /* ALGORITHM 1 — DDA Line: draws the 4-edge outline of the pipe cap */
    colF(30  / 255.f * dr, 110 / 255.f * dg, 30  / 255.f * db);
    glPointSize(2.0f);
    ddaOutlineRect(x - capOff, capY, capW, capH);
    glPointSize(1.0f);

    /* Snow-capped pipes */
    if (g_weather == WEATHER_SNOW && !flipped) {
        col(240, 248, 255);
        fillRect(x - capOff + 2.f, capY + capH - 2.f, capW - 4.f, 6.f);
        fillCircle(x - capOff + 4.f, capY + capH, 4.f, 8);
        fillCircle(x - capOff + capW - 4.f, capY + capH, 4.f, 8);
    }
}

// --- DRAW: 3D SPINNING COIN ---
// *  Uses begin3D/end3D (gluPerspective + gluLookAt) with GL_LIGHTING.
// Y-axis rotation (glRotatef) creates the spinning disc illusion.
// The coin is a glutSolidSphere flattened by glScalef on Z axis.
static void drawSingleCoin(float worldX, float worldY, float spinAngle) {
    begin3D(worldX, worldY, 26);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    GLfloat lpos[]  = { 2.0f, 3.0f, 5.0f, 1.0f };
    GLfloat ldiff[] = { 1.0f, 1.0f, 0.8f, 1.0f };
    GLfloat lamb[]  = { 0.4f, 0.3f, 0.0f, 1.0f };
    GLfloat lspec[] = { 1.0f, 0.9f, 0.5f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lpos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  ldiff);
    glLightfv(GL_LIGHT0, GL_AMBIENT,  lamb);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lspec);

    glRotatef(spinAngle, 0.0f, 1.0f, 0.0f);
    glScalef(1.0f, 1.0f, 0.22f);

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(1.0f, 0.843f, 0.0f); /* Pure Gold (FFD700) */

    GLfloat goldS[]  = { 1.0f, 0.95f, 0.7f, 1.0f };
    GLfloat goldSh[] = { 100.0f };
    glMaterialfv(GL_FRONT, GL_SPECULAR,  goldS);
    glMaterialfv(GL_FRONT, GL_SHININESS, goldSh);
    glutSolidSphere(1.0, 24, 24);
    glDisable(GL_COLOR_MATERIAL);

    glScalef(0.65f, 0.65f, 1.0f);
    glEnable(GL_COLOR_MATERIAL);
    glColor3f(0.85f, 0.7f, 0.0f); /* Slightly darker pure gold for depth */
    glutSolidTorus(0.2, 1.0, 16, 16);
    glDisable(GL_COLOR_MATERIAL);

    glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);
    glDisable(GL_DEPTH_TEST);
    end3D();
}

static void drawCoins(void) {
    for (int i = 0; i < PIPE_COUNT; i++) {
        if (g_coins[i].collected) continue;
        if (g_coins[i].x < -30.f || g_coins[i].x > WORLD_W + 30.f) continue;
        drawSingleCoin(g_coins[i].x, g_coins[i].y, g_coins[i].spinAngle);
    }
}

static void drawPipes(void) {
    float gs = GROUND_Y + GROUND_H * GRASS_H_RATIO;
    for (int i = 0; i < PIPE_COUNT; i++) {
        float gT = g_pipes[i].gapCenterY + g_pipeGap / 2.f;
        float gB = g_pipes[i].gapCenterY - g_pipeGap / 2.f;
        drawSinglePipe(g_pipes[i].x, gs, gB, 0);
        drawSinglePipe(g_pipes[i].x, gT, WORLD_H + 10.f, 1);
    }
}

// --- DRAW: CONTROLS HINT BOX ---
// *  Appears when a new game starts. Stays fully opaque for the first
// 280 frames then smoothly fades out over the last 80 frames.
// Shows all key bindings inside a premium dark-glass panel with
// golden key badges, a blue accent header and a pulsing glow.
static void drawControlsBox(void) {
    float alpha = 1.0f;   /* always fully visible on the title screen */

    float boxW = 190.f, boxH = 128.f;
    float bx   = 10.f;    /* left edge — clear of centred difficulty/weather UI */
    float by   = 248.f;   /* sits beside the difficulty row, above weather buttons */

    /* --- Outer pulsing glow --- */
    float pulse = 0.72f + 0.28f * sinf((float)g_frame * 0.07f);
    col4(50, 110, 235, (int)(alpha * 38.f * pulse));
    fillRoundRect(bx - 6.f, by - 6.f, boxW + 12.f, boxH + 12.f, 14.f);

    /* --- Main dark-glass panel --- */
    col4(7, 11, 22, (int)(alpha * 215.f));
    fillRoundRect(bx, by, boxW, boxH, 9.f);

    /* --- Top accent bar (blue) --- */
    col4(65, 140, 255, (int)(alpha * 210.f));
    fillRect(bx + 9.f, by + boxH - 6.f, boxW - 18.f, 4.f);

    /* --- Panel border --- */
    col4(60, 100, 180, (int)(alpha * 145.f));
    outlineRect(bx, by, boxW, boxH, 1.5f);

    /* --- Title (centred, stroke text) --- */
    col4(135, 195, 255, (int)(alpha * 255.f));
    float tw = strokeWidth("CONTROLS", 0.055f);
    strokeText(bx + boxW / 2.f - tw / 2.f, by + boxH - 20.f, 0.055f, "CONTROLS");

    /* --- Divider line --- */
    col4(45, 80, 145, (int)(alpha * 130.f));
    fillRect(bx + 9.f, by + boxH - 26.f, boxW - 18.f, 1.f);

    /* --- Key-action table --- */
    const char *keys[5]    = { "SPACE/Click", "P",     "W",       "F11",        "ESC"  };
    const char *actions[5] = { "Flap",        "Pause", "Weather", "Fullscreen", "Quit" };

    float lineH = 19.f;
    float topY  = by + boxH - 28.f;

    for (int i = 0; i < 5; i++) {
        float ly = topY - (float)(i + 1) * lineH;

        /* Key badge background */
        col4(32, 46, 74, (int)(alpha * 235.f));
        fillRoundRect(bx + 9.f, ly - 2.f, 78.f, 14.f, 3.f);
        /* Key badge border */
        col4(80, 118, 195, (int)(alpha * 165.f));
        outlineRect(bx + 9.f, ly - 2.f, 78.f, 14.f, 1.f);

        /* Key label — golden yellow */
        col4(255, 225, 75, (int)(alpha * 255.f));
        bitmapText(bx + 13.f, ly + 1.f, GLUT_BITMAP_HELVETICA_10, keys[i]);

        /* Arrow separator */
        col4(95, 112, 148, (int)(alpha * 205.f));
        bitmapText(bx + 93.f, ly + 1.f, GLUT_BITMAP_HELVETICA_10, "->");

        /* Action label — soft blue-white */
        col4(205, 222, 255, (int)(alpha * 245.f));
        bitmapText(bx + 108.f, ly + 1.f, GLUT_BITMAP_HELVETICA_10, actions[i]);
    }
}

// --- DRAW: HUD ---

static void drawHUD(void) {
    char buf[48];
    float cx = WORLD_W / 2.f;

    /* ── SCORE box  (top-centre, tallest) ─────────────────────── */
    {
        float bw = 88.f, bh = 56.f;
        float bx = cx - bw / 2.f;
        float by = WORLD_H - bh - 8.f;

        /* deep-purple body (solid) */
        col(32, 14, 76);
        fillRoundRect(bx, by, bw, bh, 8.f);
        /* violet top accent bar */
        col4(155, 90, 255, 235);
        fillRect(bx + 7.f, by + bh - 5.f, bw - 14.f, 5.f);
        /* border */
        col4(140, 80, 255, 185);
        outlineRect(bx, by, bw, bh, 1.5f);
        /* "SCORE" label */
        col4(205, 170, 255, 225);
        bitmapText(cx - 16.f, by + bh - 16.f, GLUT_BITMAP_HELVETICA_10, "SCORE");
        /* large number */
        sprintf(buf, "%d", g_score);
        float sw = strokeWidth(buf, 0.15f);
        col4(40, 30, 60, 160);                                  /* soft shadow */
        strokeText(cx - sw / 2.f + 1.5f, by + 10.f, 0.15f, buf);
        col(255, 255, 255);
        strokeText(cx - sw / 2.f, by + 11.5f, 0.15f, buf);
    }

    /* ── BEST box  (top-left) ──────────────────────────────────── */
    {
        float bw = 86.f, bh = 42.f;
        float bx = 10.f;
        float by = WORLD_H - bh - 10.f;

        col(6, 44, 108);                  /* ocean blue (solid) */
        fillRoundRect(bx, by, bw, bh, 7.f);
        col4(55, 148, 255, 235);                /* sky-blue accent */
        fillRect(bx + 7.f, by + bh - 5.f, bw - 14.f, 5.f);
        col4(50, 135, 240, 180);
        outlineRect(bx, by, bw, bh, 1.5f);
        col4(130, 200, 255, 225);
        bitmapText(bx + 8.f, by + bh - 15.f, GLUT_BITMAP_HELVETICA_10, "BEST");
        sprintf(buf, "%d", g_highScore);
        col(200, 232, 255);
        bitmapText(bx + 8.f, by + 7.f, GLUT_BITMAP_HELVETICA_18, buf);
    }

    /* ── Difficulty pill  (small tag below BEST box) ───────────── */
    {
        const char *dlbl = (g_difficulty == DIFF_EASY) ? "EASY" :
                           (g_difficulty == DIFF_HARD) ? "HARD" : "NORMAL";
        int pr, pg, pb;
        if      (g_difficulty == DIFF_EASY) { pr = 22;  pg = 115; pb = 45;  }
        else if (g_difficulty == DIFF_HARD) { pr = 150; pg = 28;  pb = 28;  }
        else                                { pr = 155; pg = 100; pb = 8;   }

        float bw = 56.f, bh = 18.f;
        float bx = 10.f;
        float by = WORLD_H - 42.f - 10.f - bh - 5.f;  /* 5 px below BEST box */

        col(pr, pg, pb);
        fillRoundRect(bx, by, bw, bh, 9.f); /* fully rounded pill shape */
        col4(pr + 70, pg + 70, pb + 70, 200);
        outlineRect(bx, by, bw, bh, 1.f);
        col(255, 255, 255);
        
        /* Center text horizontally */
        int textW = 0;
        for (const char *p = dlbl; *p; p++) {
            textW += glutBitmapWidth(GLUT_BITMAP_HELVETICA_10, *p);
        }
        bitmapText(bx + bw / 2.f - (float)textW / 2.f, by + 5.f, GLUT_BITMAP_HELVETICA_10, dlbl);
    }

    /* ── COINS box  (top-right, clear of close button) ─────────── */
    {
        float bw = 88.f, bh = 42.f;
        float bx = WORLD_W - (float)CLOSE_BTN_SIZE - 10.f - bw - 4.f;
        float by = WORLD_H - bh - 10.f;

        col(78, 50, 4);                   /* dark amber (solid) */
        fillRoundRect(bx, by, bw, bh, 7.f);
        col4(218, 168, 0, 235);                 /* gold accent */
        fillRect(bx + 7.f, by + bh - 5.f, bw - 14.f, 5.f);
        col4(200, 155, 0, 180);
        outlineRect(bx, by, bw, bh, 1.5f);
        col4(255, 215, 55, 225);
        bitmapText(bx + 8.f, by + bh - 15.f, GLUT_BITMAP_HELVETICA_10, "COINS");
        sprintf(buf, "%d", g_coinScore);
        col(255, 222, 40);
        bitmapText(bx + 8.f, by + 7.f, GLUT_BITMAP_HELVETICA_18, buf);
    }

    /* ── Coin collect popup ─────────────────────────────────────── */
    if (g_coinPopupTimer > 0) {
        float alpha = clampf((float)g_coinPopupTimer / 40.f, 0.f, 1.f);
        float rise  = (40 - g_coinPopupTimer) * 0.8f;
        col4(255, 230, 0, (int)(alpha * 240));
        strokeText(BIRD_X + 25.f, g_bird.y + 20.f + rise, 0.12f, "+5");
    }

    /* controls hint box is shown on the title screen only */
}

// --- DRAW: WEATHER SELECTOR (title screen interactive buttons) ---

/* Draw a small icon for each weather mode inside a button */
static void drawWeatherIcon(WeatherMode w, float cx, float cy) {
    switch (w) {
        case WEATHER_DAY: {
            /* Simple sun: circle + short rays */
            col(255, 230, 60); fillCircle(cx, cy, 10.f, 16);
            col4(255, 220, 40, 200);
            for (int i = 0; i < 8; i++) {
                float a = PI2 * i / 8 + g_frame * 0.01f;
                glLineWidth(2.f);
                glBegin(GL_LINES);
                    glVertex2f(cx + cosf(a) * 12.f, cy + sinf(a) * 12.f);
                    glVertex2f(cx + cosf(a) * 17.f, cy + sinf(a) * 17.f);
                glEnd();
            }
        } break;
        case WEATHER_SUNNY: {
            /* Larger warm sun */
            col(255, 200, 30); fillCircle(cx, cy, 13.f, 16);
            col(255, 255, 180); fillCircle(cx - 4.f, cy + 4.f, 5.f, 10);
            col4(255, 180, 20, 180);
            for (int i = 0; i < 10; i++) {
                float a = PI2 * i / 10 + g_frame * 0.012f;
                glLineWidth(2.5f);
                glBegin(GL_LINES);
                    glVertex2f(cx + cosf(a) * 15.f, cy + sinf(a) * 15.f);
                    glVertex2f(cx + cosf(a) * 21.f, cy + sinf(a) * 21.f);
                glEnd();
            }
            glLineWidth(2.f);
        } break;
        case WEATHER_RAIN: {
            /* Small cloud + three rain drops */
            col4(180, 195, 210, 230);
            fillEllipse(cx, cy + 6.f, 18.f, 10.f, 12);
            fillEllipse(cx - 10.f, cy + 2.f, 10.f, 7.f, 10);
            fillEllipse(cx + 10.f, cy + 3.f, 10.f, 7.f, 10);
            /* Rain drops */
            col4(100, 150, 210, 220);
            glLineWidth(2.f);
            for (int i = 0; i < 3; i++) {
                float rx = cx - 10.f + i * 10.f;
                glBegin(GL_LINES);
                    glVertex2f(rx, cy - 5.f);
                    glVertex2f(rx - 3.f, cy - 14.f);
                glEnd();
            }
        } break;
        case WEATHER_NIGHT: {
            /* Stars + crescent moon */
            col(250, 245, 200); fillCircle(cx + 4.f, cy + 2.f, 11.f, 16);
            /* Cutout circle for crescent */
            col4((int)(0.12f * 255), (int)(0.15f * 255), (int)(0.32f * 255), 255);
            fillCircle(cx + 9.f, cy + 2.f, 8.5f, 14);
            /* Stars */
            col4(255, 255, 200, 210);
            fillCircle(cx - 10.f, cy + 8.f, 2.f, 6);
            fillCircle(cx - 14.f, cy, 1.5f, 6);
            fillCircle(cx - 8.f, cy - 8.f, 1.8f, 6);
        } break;
        case WEATHER_SNOW: {
            /* Snowflake icon */
            col(255, 255, 255);
            glLineWidth(2.f);
            for (int i = 0; i < 3; i++) {
                float a = 3.1415926535f * i / 3.f;
                glBegin(GL_LINES);
                glVertex2f(cx + cosf(a) * 12.f, cy + sinf(a) * 12.f);
                glVertex2f(cx - cosf(a) * 12.f, cy - sinf(a) * 12.f);
                glEnd();
            }
            glPointSize(3.f);
            glBegin(GL_POINTS);
            for (int i = 0; i < 6; i++) {
                float a = PI2 * i / 6.f;
                glVertex2f(cx + cosf(a) * 8.f, cy + sinf(a) * 8.f);
            }
            glEnd();
            glPointSize(1.f);
        } break;
    }
    glLineWidth(2.f);
}

static void drawDifficultySelector(void) {
    float startX = WORLD_W / 2.f - 170.f;
    float y = 260.f;
    float w = 110.f;
    float h = 40.f;
    const char *labels[] = {"EASY", "NORMAL", "HARD"};

    for (int i = 0; i < 3; i++) {
        float bx = startX + i * (w + 10.f);
        if (i == (int)g_difficulty) {
            col(255, 255, 0);
            outlineRect(bx - 2.f, y - 2.f, w + 4.f, h + 4.f, 3.f);
        }
        if      (i == DIFF_EASY)   col(100, 200, 100);
        else if (i == DIFF_NORMAL) col(200, 200, 100);
        else                       col(200, 100, 100);

        if (i == g_hoveredDiff) col4(255, 255, 255, 180);

        fillRect(bx, y, w, h);
        col(0, 0, 0);
        outlineRect(bx, y, w, h, 2.f);

        float lw = strokeWidth(labels[i], 0.08f);
        col(255, 255, 255);
        strokeText(bx + w / 2.f - lw / 2.f, y + 10.f, 0.08f, labels[i]);
    }
}

static void drawWeatherSelector(void) {
    /* ── "Choose your weather" label box ──────────────────────── */
    {
        const char *label = "CHOOSE  YOUR  WEATHER";
        float lw   = strokeWidth(label, 0.068f);
        float boxW = lw + 44.f;
        float boxH = 30.f;
        float bx   = WORLD_W / 2.f - boxW / 2.f;
        float by   = WBTN_Y + WBTN_H + 8.f;   /* sits just above the buttons */

        /* Shadow */
        col4(0, 0, 0, 50);
        fillRoundRect(bx + 3.f, by - 3.f, boxW, boxH, 8.f);

        /* Dark fill */
        col4(10, 14, 28, 205);
        fillRoundRect(bx, by, boxW, boxH, 8.f);

        /* Left accent bar (sky blue) */
        col4(80, 160, 255, 220);
        fillRect(bx, by + 4.f, 4.f, boxH - 8.f);

        /* Top inner highlight */
        col4(255, 255, 255, 18);
        fillRect(bx + 6.f, by + boxH - 6.f, boxW - 12.f, 4.f);

        /* Border */
        col4(180, 210, 255, 160);
        outlineRect(bx, by, boxW, boxH, 1.5f);

        /* Text — bright white, centred */
        col(255, 255, 255);
        strokeText(WORLD_W / 2.f - lw / 2.f,
                   by + boxH / 2.f - 5.f,
                   0.068f, label);
    }

    for (int i = 0; i < WEATHER_COUNT; i++) {
        float bx       = WBTN_STARTX + i * (WBTN_W + WBTN_GAP);
        float by       = WBTN_Y;
        int   selected = (g_weather == static_cast<WeatherMode>(i));
        int   hovered  = (g_hoveredWeather == i);
        const WeatherTheme *t = &g_themes[i];

        /* Button background */
        float alpha = selected ? 0.95f : hovered ? 0.80f : 0.65f;
        colF4(t->btnR, t->btnG, t->btnB, alpha);
        fillRoundRect(bx, by, WBTN_W, WBTN_H, 8.f);

        /* Selected glow */
        if (selected) {
            float pulse = 0.7f + 0.3f * sinf(g_frame * 0.10f);
            col4(255, 240, 100, (int)(pulse * 160));
            outlineRect(bx - 3.f, by - 3.f, WBTN_W + 6.f, WBTN_H + 6.f, 3.f);
            col4(255, 255, 200, (int)(pulse * 80));
            outlineRect(bx - 5.f, by - 5.f, WBTN_W + 10.f, WBTN_H + 10.f, 2.f);
        } else if (hovered) {
            col4(255, 255, 255, 70);
            fillRoundRect(bx, by, WBTN_W, WBTN_H, 8.f);
        }

        /* Border */
        if (selected) col(255, 240, 100);
        else          col(200, 210, 220);
        outlineRect(bx, by, WBTN_W, WBTN_H, 2.f);

        /* Weather icon (centred, upper half of button) */
        drawWeatherIcon(static_cast<WeatherMode>(i), bx + WBTN_W / 2.f, by + WBTN_H * 0.60f);

        /* Weather name text (lower portion) */
        if (selected) col(255, 240, 100);
        else          col(240, 245, 255);
        const char *nm = g_themes[i].name;
        float nw = strokeWidth(nm, 0.060f);
        strokeText(bx + WBTN_W / 2.f - nw / 2.f, by + 8.f, 0.060f, nm);
    }

    /* Description of selected weather */
    const char *desc = g_themes[g_weather].desc;
    col4(220, 230, 255, 200);
    float dw = strokeWidth(desc, 0.055f);
    strokeText(WORLD_W / 2.f - dw / 2.f, WBTN_Y - 22.f, 0.055f, desc);
}

// --- DRAW: TITLE SCREEN ---

static void drawTitleScreen(void) {
    /* Beautiful Solid 2.5D Title Text */
    float tY = WORLD_H * 0.72f; /* Kept perfectly static */
    float sc = 0.38f; /* Slightly larger and bolder scale */

    // We use GLUT_STROKE_ROMAN which is proportionally spaced and elegant
    // Calculate widths manually for centering
    float w1 = 0; for(const char *c = "FLYING"; *c; c++) w1 += glutStrokeWidth(GLUT_STROKE_ROMAN, *c); w1 *= sc;
    float w2 = 0; for(const char *c = "BIRD"; *c; c++) w2 += glutStrokeWidth(GLUT_STROKE_ROMAN, *c); w2 *= sc;

    float x1 = WORLD_W / 2.f - w1 / 2.f;
    float x2 = WORLD_W / 2.f - w2 / 2.f;
    float y1 = tY;
    float y2 = tY - 65.f;

    // Enable blending for smooth antialiased lines
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);

    // 1. Drop Shadow (Soft)
    glLineWidth(12.0f);
    col4(0, 0, 0, 100);
    glPushMatrix(); glTranslatef(x1 + 15.f, y1 - 15.f, 0.f); glScalef(sc, sc, 1.f);
    for(const char *c = "FLYING"; *c; c++) glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
    glPopMatrix();
    glPushMatrix(); glTranslatef(x2 + 15.f, y2 - 15.f, 0.f); glScalef(sc, sc, 1.f);
    for(const char *c = "BIRD"; *c; c++) glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
    glPopMatrix();

    // 2. Dense Solid Extrusion (3D Body)
    // Draw from back to front
    glLineWidth(14.0f); /* Thick bold extrusion */
    int layers = 15;
    for (int i = layers; i >= 1; i--) {
        float offset = i * 1.0f;
        float depth = (float)i / layers; // 1.0 (back) to 0.0 (front)

        // Golden/Orange side for FLYING
        col(180 - depth * 100, 100 - depth * 60, 0);
        glPushMatrix(); glTranslatef(x1 + offset, y1 - offset, 0.f); glScalef(sc, sc, 1.f);
        for(const char *c = "FLYING"; *c; c++) glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
        glPopMatrix();

        // Green/Teal side for BIRD
        col(0, 140 - depth * 80, 100 - depth * 60);
        glPushMatrix(); glTranslatef(x2 + offset, y2 - offset, 0.f); glScalef(sc, sc, 1.f);
        for(const char *c = "BIRD"; *c; c++) glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
        glPopMatrix();
    }

    // 3. Front Face (Vibrant)
    glLineWidth(8.0f); /* Bold face */
    // FLYING - Vibrant Yellow/Gold
    col(255, 220, 40);
    glPushMatrix(); glTranslatef(x1, y1, 0.f); glScalef(sc, sc, 1.f);
    for(const char *c = "FLYING"; *c; c++) glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
    glPopMatrix();

    // BIRD - Vibrant Lime Green
    col(80, 255, 100);
    glPushMatrix(); glTranslatef(x2, y2, 0.f); glScalef(sc, sc, 1.f);
    for(const char *c = "BIRD"; *c; c++) glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
    glPopMatrix();

    // 4. Specular Inner Highlight
    glLineWidth(3.0f);
    col(255, 255, 230);
    glPushMatrix(); glTranslatef(x1, y1, 0.f); glScalef(sc, sc, 1.f);
    for(const char *c = "FLYING"; *c; c++) glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
    glPopMatrix();
    col(220, 255, 230);
    glPushMatrix(); glTranslatef(x2, y2, 0.f); glScalef(sc, sc, 1.f);
    for(const char *c = "BIRD"; *c; c++) glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
    glPopMatrix();

    glDisable(GL_LINE_SMOOTH);
    glLineWidth(1.0f);


    /* Showcase bird near title */
    /* SCALING TRANSFORM — title bird pulses in and out (uniform scale) */
    float sy = g_bird.y, sa = g_bird.angle;
    g_bird.y = tY - 22.f; g_bird.angle = 5.f * sinf(g_frame * 0.05f);
    float titleScale = 1.0f + 0.12f * sinf(g_frame * 0.07f);
    glPushMatrix();
        glTranslatef(BIRD_X, tY - 22.f, 0.f);
        glScalef(titleScale, titleScale, 1.0f);
        glTranslatef(-BIRD_X, -(tY - 22.f), 0.f);
        drawBird();
    glPopMatrix();
    g_bird.y = sy; g_bird.angle = sa;

    /* Weather selector */
    drawWeatherSelector();
    drawDifficultySelector();

    /* ── Start prompt box ─────────────────────────────────────── */
    {
        const char *prompt = "PRESS  SPACE  OR  CLICK  TO  START";
        float pw   = strokeWidth(prompt, 0.072f);
        float boxW = pw + 48.f;
        float boxH = 36.f;
        float bx   = WORLD_W / 2.f - boxW / 2.f;
        /* sit just below the weather-description line (WBTN_Y - 22 = 98),
           leaving a small gap so nothing overlaps                          */
        float by   = WBTN_Y - boxH - 10.f;   /* = 120 - 36 - 10 = 74      */

        /* Outer shadow */
        col4(0, 0, 0, 55);
        fillRoundRect(bx + 3.f, by - 3.f, boxW, boxH, 10.f);

        /* Dark semi-transparent fill */
        col4(10, 14, 28, 210);
        fillRoundRect(bx, by, boxW, boxH, 10.f);

        /* Inner top highlight strip */
        col4(255, 255, 255, 22);
        fillRect(bx + 10.f, by + boxH - 8.f, boxW - 20.f, 5.f);

        /* White border */
        col4(255, 255, 255, 180);
        outlineRect(bx, by, boxW, boxH, 2.f);

        /* Centred label — bright white, large and clear */
        col(255, 255, 255);
        strokeText(WORLD_W / 2.f - pw / 2.f,
                   by + boxH / 2.f - 6.f,
                   0.072f, prompt);
    }

}

// --- DRAW: PLAY AGAIN BUTTON ---
// Shown after g_gameOverDelay reaches 0.
// Shows a progress indicator while waiting.

static void drawPlayAgainButton(float px, float py) {
    float btnW = PLAY_BTN_W, btnH = PLAY_BTN_H;
    float btnX = WORLD_W / 2.f - btnW / 2.f;
    float btnY = py + 12.f;

    if (g_gameOverDelay > 0) {
        /* Waiting: show a small loading arc */
        float progress = 1.f - (float)g_gameOverDelay / GAMEOVER_DELAY;
        float arcAngle = progress * 360.f;
        col4(150, 150, 150, 120);
        float r = 14.f, ccx = WORLD_W / 2.f, ccy = btnY + btnH / 2.f;
        fillCircle(ccx, ccy, r + 4.f, 20);
        col4(60, 60, 60, 180); fillCircle(ccx, ccy, r, 20);
        col4(100, 200, 100, 200);
        int arcSegs = (int)(arcAngle / 360.f * 20.f);
        glBegin(GL_TRIANGLE_FAN);
            glVertex2f(ccx, ccy);
            for (int i = 0; i <= arcSegs; i++) {
                float a = -PI2 / 4.f + PI2 * i / 20;
                glVertex2f(ccx + cosf(a) * r, ccy + sinf(a) * r);
            }
        glEnd();
    } else {
        /* Button ready */
        int hov = g_hoveredPlayAgain;

        /* Shadow */
        col4(0, 0, 0, 60);
        fillRoundRect(btnX + 3.f, btnY - 3.f, btnW, btnH, 10.f);

        /* Button body */
        if (hov) col(80, 210, 90);
        else     col(55, 185, 65);
        fillRoundRect(btnX, btnY, btnW, btnH, 10.f);

        /* Hover shimmer */
        if (hov) {
            col4(255, 255, 255, 50);
            fillRoundRect(btnX, btnY + btnH / 2.f, btnW, btnH / 2.f, 10.f);
        }

        /* Border */
        col(30, 120, 35); outlineRect(btnX, btnY, btnW, btnH, 2.5f);
        /* Top highlight */
        col4(255, 255, 255, hov ? 100 : 70);
        fillRect(btnX + 12.f, btnY + btnH - 8.f, btnW - 24.f, 4.f);

        /* Text */
        const char *txt = "PLAY AGAIN";
        float tw = strokeWidth(txt, 0.085f);
        /* Shadow */
        col4(0, 80, 0, 150);
        strokeText(WORLD_W / 2.f - tw / 2.f + 1.5f, btnY + btnH / 2.f - 5.f, 0.085f, txt);
        /* Text */
        col(240, 255, 240);
        strokeText(WORLD_W / 2.f - tw / 2.f, btnY + btnH / 2.f - 4.f, 0.085f, txt);

        /* Keyboard hint below button */
        col4(120, 120, 120, 180);
        bitmapText(WORLD_W / 2.f - 50.f, btnY - 16.f,
                   GLUT_BITMAP_HELVETICA_12, "or press SPACE / R");
    }
}

// --- DRAW: GAME OVER SCREEN ---

static void drawMedal(int score, float cx, float cy) {
    if (score < 5) return;

    col(200, 40, 40); /* red ribbons */
    fillRect(cx - 15.f, cy - 35.f, 10.f, 35.f);
    fillRect(cx + 5.f,  cy - 35.f, 10.f, 35.f);

    if (score >= 30) {
        col4(255, 255, 200, 150);
        fillCircle(cx, cy, 32.f, 16);
    }

    if      (score >= 50) col(200, 150, 255); /* Platinum */
    else if (score >= 30) col(255, 215, 0);   /* Gold     */
    else if (score >= 15) col(192, 192, 192); /* Silver   */
    else                  col(205, 127, 50);  /* Bronze   */

    fillCircle(cx, cy, 22.f, 16);
    col(255, 255, 255);
    glLineWidth(3.f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 16; i++) {
        float a = PI2 * i / 16.f;
        glVertex2f(cx + cosf(a) * 22.f, cy + sinf(a) * 22.f);
    }
    glEnd();
    glLineWidth(2.f);
}

static void drawGameOverScreen(void) {
    col4(0, 0, 0, 110); fillRect(0, 0, WORLD_W, WORLD_H);

    float px = WORLD_W / 2.f - 180.f, py = 480.f;
    col4(250, 242, 205, 245); fillRect(px, py, 360.f, 180.f);
    col(90, 60, 20);   outlineRect(px, py, 360.f, 180.f, 4.f);
    col(180, 130, 50); outlineRect(px + 4.f, py + 4.f, 352.f, 172.f, 3.f);

    char buf[48];
    col(60, 40, 10);
    sprintf(buf, "Score:  %d", g_score);
    bitmapText(WORLD_W / 2.f - 60.f, py + 130.f, GLUT_BITMAP_HELVETICA_18, buf);
    col(160, 110, 0);
    sprintf(buf, "Coins:  %d", g_coinScore);
    bitmapText(WORLD_W / 2.f - 60.f, py + 108.f, GLUT_BITMAP_HELVETICA_18, buf);
    col(60, 40, 10);
    sprintf(buf, "Best:   %d", g_highScore);
    bitmapText(WORLD_W / 2.f - 60.f, py + 86.f,  GLUT_BITMAP_HELVETICA_18, buf);

    drawMedal(g_score, WORLD_W / 2.f - 110.f, py + 110.f);
    drawPlayAgainButton(px, py);
    drawWeatherSelector();

    /* Beautiful Solid 2.5D GAME OVER Text */
    {
        float sc = 0.38f; /* Bold scale */
        const char *gtxt = "GAME OVER";
        float w1 = 0; for(const char *c = gtxt; *c; c++) w1 += glutStrokeWidth(GLUT_STROKE_ROMAN, *c); w1 *= sc;

        float x1 = WORLD_W / 2.f - w1 / 2.f;
        float y1 = 330.f; /* Centered nicely */

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_LINE_SMOOTH);

        // 1. Drop Shadow (Soft)
        glLineWidth(12.0f);
        col4(0, 0, 0, 100);
        glPushMatrix(); glTranslatef(x1 + 15.f, y1 - 15.f, 0.f); glScalef(sc, sc, 1.f);
        for (const char *c = gtxt; *c; c++) glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
        glPopMatrix();

        // 2. Dense Solid Extrusion (3D Body)
        glLineWidth(14.0f); /* Thick bold extrusion */
        int layers = 15;
        for (int i = layers; i >= 1; i--) {
            float offset = i * 1.0f;
            float depth = (float)i / layers; // 1.0 (back) to 0.0 (front)

            // Dark red to bright red
            col(150 - depth * 80, 0, 0);
            glPushMatrix(); glTranslatef(x1 + offset, y1 - offset, 0.f); glScalef(sc, sc, 1.f);
            for (const char *c = gtxt; *c; c++) glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
            glPopMatrix();
        }

        // 3. Front Face (Vibrant Red)
        glLineWidth(8.0f); /* Bold face */
        col(255, 30, 30);
        glPushMatrix(); glTranslatef(x1, y1, 0.f); glScalef(sc, sc, 1.f);
        for (const char *c = gtxt; *c; c++) glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
        glPopMatrix();

        // 4. Specular Inner Highlight
        glLineWidth(3.0f);
        col(255, 200, 200);
        glPushMatrix(); glTranslatef(x1, y1, 0.f); glScalef(sc, sc, 1.f);
        for (const char *c = gtxt; *c; c++) glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
        glPopMatrix();

        glDisable(GL_LINE_SMOOTH);
        glLineWidth(1.0f);
    }
}

// --- DRAW: PAUSE ---

static void drawPauseScreen(void) {
    col4(0, 0, 0, 80); fillRect(0, 0, WORLD_W, WORLD_H);
    const char *pt = "PAUSED"; float psc = 0.25f;
    col(255, 255, 255);
    strokeText(WORLD_W / 2.f - strokeWidth(pt, psc) / 2.f, WORLD_H / 2.f - 10.f, psc, pt);
    col(200, 200, 200);
    bitmapText(WORLD_W / 2.f - 75.f, WORLD_H / 2.f - 50.f,
               GLUT_BITMAP_HELVETICA_18, "Press P to Resume");
}

// --- DRAW: WEATHER NAME ANNOUNCEMENT ---

static void drawWeatherName(void) {
    if (g_weatherNameTimer <= 0) return;
    float alpha     = clampf((float)g_weatherNameTimer / 60.f, 0.f, 1.f);
    const char *name = g_themes[g_weather].name;
    float pw = 220.f, ph = 52.f;
    float ppx = WORLD_W / 2.f - pw / 2.f, ppy = WORLD_H * 0.72f;
    col4(0, 0, 0, (int)(alpha * 120)); fillRect(ppx, ppy, pw, ph);
    col4(255, 255, 255, (int)(alpha * 55)); outlineRect(ppx, ppy, pw, ph, 2.f);
    col4(255, 255, 255, (int)(alpha * 255));
    float tw = strokeWidth(name, 0.16f);
    strokeText(WORLD_W / 2.f - tw / 2.f, ppy + 16.f, 0.16f, name);
}

// --- DRAW: CLOSE BUTTON  (top-right corner, always visible) ---

static void drawCloseButton(void) {
    float bx  = CLOSE_BTN_X;
    float by  = CLOSE_BTN_Y;
    float sz  = CLOSE_BTN_SIZE;
    float cx  = bx + sz * 0.5f;
    float cy  = by + sz * 0.5f;
    float arm = sz * 0.22f;   /* half-length of each X arm */

    /* Shadow */
    col4(0, 0, 0, 60);
    fillRoundRect(bx + 2.f, by - 2.f, sz, sz, 6.f);

    /* Button body — brighter red on hover */
    if (g_hoveredClose) col4(235, 60,  60, 245);
    else                col4(195, 45,  45, 215);
    fillRoundRect(bx, by, sz, sz, 6.f);

    /* Top highlight strip (gives depth) */
    col4(255, 130, 130, g_hoveredClose ? 90 : 55);
    fillRoundRect(bx + 2.f, by + sz * 0.55f, sz - 4.f, sz * 0.4f, 5.f);

    /* Border */
    col4(255, 110, 110, 180);
    outlineRect(bx, by, sz, sz, 1.5f);

    /* White X — two crossing lines */
    col(255, 255, 255);
    glLineWidth(g_hoveredClose ? 2.8f : 2.2f);
    glBegin(GL_LINES);
        glVertex2f(cx - arm, cy - arm);   /* top-left  -> bot-right */
        glVertex2f(cx + arm, cy + arm);
        glVertex2f(cx + arm, cy - arm);   /* top-right -> bot-left  */
        glVertex2f(cx - arm, cy + arm);
    glEnd();
    glLineWidth(2.f);
}

// --- DRAW: MENU BUTTON  (top-left corner, expandable) ---

static void drawMenuButton(void) {
    float bx  = MENU_BTN_X;
    float by  = MENU_BTN_Y;
    float sz  = MENU_BTN_SIZE;

    /* Dropdown Panel */
    if (g_menuAnimPhase > 0.001f) {
        float panelW = 160.f;
        float maxH   = 160.f;
        float curH   = maxH * g_menuAnimPhase;
        float py     = by - curH + 4.f; /* overlap slightly with button */
        
        /* Box Background (solid colour) */
        col(20, 25, 40); 
        if (curH > 16.0f) fillRoundRect(bx, py, panelW, curH, 8.f);
        else              fillRect(bx, py, panelW, curH);
        
        /* Border */
        col4(60, 100, 180, (int)(255 * g_menuAnimPhase));
        if (curH > 4.0f) outlineRect(bx, py, panelW, curH, 1.5f);

        /* Instructions Text (Fade in) */
        if (g_menuAnimPhase > 0.8f) {
            float textAlpha = (g_menuAnimPhase - 0.8f) * 5.0f;
            col4(255, 255, 255, (int)(255 * textAlpha));
            float tx = bx + 15.f;
            float ty = by - 22.f; /* relative to top of panel */
            float lh = 22.f;
            bitmapText(tx, ty, GLUT_BITMAP_HELVETICA_12, "SPACE / Click : Flap");
            bitmapText(tx, ty - lh*1, GLUT_BITMAP_HELVETICA_12, "W : Weather");
            bitmapText(tx, ty - lh*2, GLUT_BITMAP_HELVETICA_12, "P : Pause");
            bitmapText(tx, ty - lh*3, GLUT_BITMAP_HELVETICA_12, "R : Restart");
            bitmapText(tx, ty - lh*4, GLUT_BITMAP_HELVETICA_12, "F11 : Fullscreen");
            bitmapText(tx, ty - lh*5, GLUT_BITMAP_HELVETICA_12, "ESC : Quit");
        }
    }

    /* Button Shadow */
    col4(0, 0, 0, 60);
    fillRoundRect(bx + 2.f, by - 2.f, sz, sz, 6.f);

    /* Button body */
    if (g_hoveredMenu || g_menuExpanded) col(80, 140, 240);
    else                                 col(60, 110, 200);
    fillRoundRect(bx, by, sz, sz, 6.f);

    /* Highlight */
    col4(160, 200, 255, (g_hoveredMenu || g_menuExpanded) ? 90 : 55);
    fillRoundRect(bx + 2.f, by + sz * 0.55f, sz - 4.f, sz * 0.4f, 5.f);

    /* Border */
    col(120, 170, 255);
    outlineRect(bx, by, sz, sz, 1.5f);

    /* Hamburger Icon ≡ */
    col(255, 255, 255);
    float cx = bx + sz/2.f;
    float cy = by + sz/2.f;
    float w  = sz * 0.45f;
    float hGap = 5.f;
    glLineWidth(2.5f);
    glBegin(GL_LINES);
        glVertex2f(cx - w/2.f, cy + hGap);
        glVertex2f(cx + w/2.f, cy + hGap);
        
        glVertex2f(cx - w/2.f, cy);
        glVertex2f(cx + w/2.f, cy);
        
        glVertex2f(cx - w/2.f, cy - hGap);
        glVertex2f(cx + w/2.f, cy - hGap);
    glEnd();
    glLineWidth(2.f);
}

// --- DISPLAY CALLBACK ---

static void display(void) {
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();
    glTranslatef(g_shakeX, g_shakeY, 0.f);

    drawBackground();
    if (g_weather == WEATHER_SUNNY || g_weather == WEATHER_DAY) drawSun();
    if (g_weather == WEATHER_NIGHT) { drawShootingStars(); drawStars(); drawMoon(); }
    drawMountains();
    drawClouds();
    if (g_weather == WEATHER_SUNNY) drawRainbow();
    drawCitySilhouette();
    // drawTrees();

    switch (g_state) {
        case STATE_TITLE:
            drawGround();
            drawTitleScreen();
            break;
        case STATE_PLAYING:
            /* drawBirdReflection() — Reflection Transform — must come AFTER
               drawGround() so it renders inside the ground strip, then
               drawBird() renders the real bird on top. */
            drawPipes(); drawCoins(); drawGround(); drawBirdReflection(); drawBird(); drawHUD();
            break;
        case STATE_PAUSED:
            drawPipes(); drawCoins(); drawGround(); drawBirdReflection(); drawBird(); drawHUD();
            drawPauseScreen();
            break;
        case STATE_GAMEOVER:
            drawPipes(); drawCoins(); drawGround(); drawBirdReflection(); drawBird(); drawHUD();
            drawGameOverScreen();
            break;
    }

    if (g_weather == WEATHER_RAIN) { drawFog(); drawRain(); drawLightning(); }
    if (g_weather == WEATHER_SNOW) { drawFog(); drawSnow(); }
    drawParticles();      /* GL_POINTS — score sparkle burst */
    drawWeatherName();

    /* Always visible buttons on top */
    drawCloseButton();
    drawMenuButton();

    /* Weather change flash */
    if (g_wFlashTicks > 0) {
        float a = (float)g_wFlashTicks / 12.f;
        col4(255, 255, 255, (int)(a * 170)); fillRect(0, 0, WORLD_W, WORLD_H);
    }
    /* Death flash */
    if (g_flashTicks > 0) {
        float a = (float)g_flashTicks / 10.f;
        col4(255, 255, 255, (int)(a * 200)); fillRect(0, 0, WORLD_W, WORLD_H);
    }

    glutSwapBuffers();
}

// --- COLLISION DETECTION ---

static int checkCollision(void) {
    float bx = BIRD_X, by = g_bird.y, br = BIRD_RADIUS - 4.f;
    float gs = GROUND_Y + GROUND_H * GRASS_H_RATIO;
    if (by - br <= gs)     return 1;
    if (by + br >= WORLD_H) return 1;
    for (int i = 0; i < PIPE_COUNT; i++) {
        float px = g_pipes[i].x;
        float gT = g_pipes[i].gapCenterY + g_pipeGap / 2.f;
        float gB = g_pipes[i].gapCenterY - g_pipeGap / 2.f;
        if (bx + br > px - 7.f && bx - br < px + PIPE_W + 7.f) {
            if (by - br < gB) return 1;
            if (by + br > gT) return 1;
        }
    }
    return 0;
}

// --- PARTICLE SYSTEM — trigger, update, (draw is in draw section) ---

/* Spawn a radial burst of 20 sparkle particles at world pos (x,y). */
static void triggerParticles(float x, float y) {
    int spawned = 0;
    for (int i = 0; i < PARTICLE_COUNT && spawned < 20; i++) {
        if (!g_particles[i].active) {
            float angle          = randf(0.f, PI2);
            float speed          = randf(2.5f, 9.f);
            g_particles[i].x     = x;
            g_particles[i].y     = y;
            g_particles[i].vx   = cosf(angle) * speed;
            g_particles[i].vy   = sinf(angle) * speed;
            g_particles[i].r    = randf(0.85f, 1.0f);
            g_particles[i].g    = randf(0.70f, 1.0f);
            g_particles[i].b    = randf(0.0f,  0.35f);
            g_particles[i].life = 1.0f;
            g_particles[i].active = 1;
            spawned++;
        }
    }
}

/* Advance all live particles one simulation step. */
static void updateParticles(void) {
    for (int i = 0; i < PARTICLE_COUNT; i++) {
        if (!g_particles[i].active) continue;
        g_particles[i].x    += g_particles[i].vx;
        g_particles[i].y    += g_particles[i].vy;
        g_particles[i].vy   -= 0.28f;   /* gravity */
        g_particles[i].vx   *= 0.97f;   /* drag    */
        g_particles[i].life -= 0.030f;
        if (g_particles[i].life <= 0.f)
            g_particles[i].active = 0;
    }
}

// --- UPDATE FUNCTIONS ---

static void updateBird(void) {
    g_bird.vy += GRAVITY;
    if (g_bird.vy < MAX_FALL_VEL) g_bird.vy = MAX_FALL_VEL;
    g_bird.y  += g_bird.vy;
    float tgt;
    if (g_bird.vy > 2.f) {
        tgt = TILT_UP_DEG;
    } else {
        float t = clampf((g_bird.vy - MAX_FALL_VEL) / (2.f - MAX_FALL_VEL), 0.f, 1.f);
        tgt = lerpf(TILT_DOWN_DEG, 0.f, t);
    }
    g_bird.angle += (tgt - g_bird.angle) * 0.25f;
    if (++g_bird.wingTimer >= WING_ANIM_RATE) {
        g_bird.wingTimer = 0;
        g_bird.wingFrame = (g_bird.wingFrame + 1) % WING_FRAMES;
    }
}

static void updatePipes(void) {
    float minC = GROUND_Y + GROUND_H * GRASS_H_RATIO + PIPE_MIN_H + g_pipeGap / 2.f;
    float maxC = WORLD_H - PIPE_MIN_H - g_pipeGap / 2.f;
    for (int i = 0; i < PIPE_COUNT; i++) {
        g_pipes[i].x       -= g_pipeSpeed;
        g_coins[i].x       -= g_pipeSpeed;
        g_coins[i].spinAngle += 3.0f;
        if (g_pipes[i].x + PIPE_W < -20.f) {
            float mx = g_pipes[i].x;
            for (int j = 0; j < PIPE_COUNT; j++)
                if (j != i && g_pipes[j].x > mx) mx = g_pipes[j].x;
            g_pipes[i].x          = mx + PIPE_SPACING;
            g_pipes[i].gapCenterY = randf(minC, maxC);
            g_pipes[i].scored     = 0;
            g_coins[i].x          = g_pipes[i].x + PIPE_W / 2.f;
            g_coins[i].y          = g_pipes[i].gapCenterY;
            g_coins[i].spinAngle  = 0.f;
            g_coins[i].collected  = 0;
        }
        if (!g_pipes[i].scored && g_pipes[i].x + PIPE_W / 2.f < BIRD_X) {
            g_pipes[i].scored = 1;
            g_score++;
            if (g_score > g_highScore) {
                g_highScore = g_score;
                saveHighScore();
            }
            triggerParticles(BIRD_X, g_bird.y);
        }
    }
    g_pipeSpeed += PIPE_SPEED_INC;
}

static void updateClouds(void) {
    for (int i = 0; i < CLOUD_COUNT; i++) {
        g_clouds[i].x -= g_clouds[i].speed;
        if (g_clouds[i].x + 100.f * g_clouds[i].scale < 0.f) {
            g_clouds[i].x = WORLD_W + 70.f;
            g_clouds[i].y = randf(GROUND_Y + GROUND_H + 60.f, WORLD_H - 60.f);
        }
    }
}

static void updateRainDrops(void) {
    for (int i = 0; i < RAIN_COUNT; i++) {
        g_rain[i].y -= g_rain[i].speed;
        g_rain[i].x -= g_rain[i].speed * 0.28f;
        if (g_rain[i].y < -20.f) {
            g_rain[i].y = WORLD_H + 10.f;
            g_rain[i].x = randf(-50.f, WORLD_W + 50.f);
        }
        if (g_rain[i].x < -20.f) {
            g_rain[i].x = WORLD_W + randf(0.f, 80.f);
            g_rain[i].y = randf(0.f, WORLD_H);
        }
    }
}

// --- MASTER UPDATE ---

static void updateGame(void) {
    g_frame++;
    updateWeather();

    if (g_menuExpanded) {
        if (g_menuAnimPhase < 1.0f) g_menuAnimPhase += 0.15f;
        if (g_menuAnimPhase > 1.0f) g_menuAnimPhase = 1.0f;
    } else {
        if (g_menuAnimPhase > 0.0f) g_menuAnimPhase -= 0.15f;
        if (g_menuAnimPhase < 0.0f) g_menuAnimPhase = 0.0f;
    }

    if (g_state == STATE_TITLE) {
        updateClouds();
        if (g_weather == WEATHER_RAIN)  updateRainDrops();
        if (g_weather == WEATHER_SNOW)  updateSnow();
        if (g_weather == WEATHER_NIGHT) updateShootingStars();
        g_titleBobT += 0.05f;
        g_titleBobY  = 8.f * sinf(g_titleBobT);
        if (++g_bird.wingTimer >= WING_ANIM_RATE) {
            g_bird.wingTimer = 0;
            g_bird.wingFrame = (g_bird.wingFrame + 1) % WING_FRAMES;
        }
        return;
    }
    if (g_state == STATE_PAUSED) return;

    if (g_state == STATE_GAMEOVER) {
        if (g_flashTicks > 0)    g_flashTicks--;
        if (g_gameOverDelay > 0) g_gameOverDelay--;
        if (g_weather == WEATHER_RAIN)  updateRainDrops();
        if (g_weather == WEATHER_SNOW)  updateSnow();
        if (g_weather == WEATHER_NIGHT) updateShootingStars();
        updateClouds();
        updateShake();
        g_shearX += (0.4f - g_shearX) * 0.04f; /* shear animation */
        return;
    }

    /* Playing */
    updateBird();
    if (g_hintTimer > 0) g_hintTimer--;   /* tick down controls-box timer */
    updatePipes();
    updateParticles();     /* advance score-sparkle particles */

    /* Coin collection: check proximity each frame */
    for (int ci = 0; ci < PIPE_COUNT; ci++) {
        if (g_coins[ci].collected) continue;
        float cdx = g_coins[ci].x - BIRD_X;
        float cdy = g_coins[ci].y - g_bird.y;
        if (cdx * cdx + cdy * cdy <
            (COIN_RADIUS + BIRD_RADIUS) * (COIN_RADIUS + BIRD_RADIUS)) {
            g_coins[ci].collected = 1;
            g_coinScore          += COIN_COLLECT_BONUS;
            g_coinPopupTimer      = 40;
            playSound(SFX_COIN);
        }
    }
    if (g_coinPopupTimer > 0) g_coinPopupTimer--;
    updateClouds();
    if (g_weather == WEATHER_RAIN)  updateRainDrops();
    if (g_weather == WEATHER_SNOW)  updateSnow();
    if (g_weather == WEATHER_NIGHT) updateShootingStars();
    g_groundScroll += g_pipeSpeed;

    if (checkCollision()) {
        g_state         = STATE_GAMEOVER;
        g_hintTimer     = 0;             /* hide controls box immediately on death */
        g_bird.alive    = 0;
        g_flashTicks    = 10;
        g_gameOverDelay = GAMEOVER_DELAY;
        triggerShake(15);
        playSound(SFX_DIE);
    }
    updateShake();
    if (g_flashTicks > 0) g_flashTicks--;
}

// --- BGM LOOP MAINTENANCE ---
// *  MCI's 'repeat' flag can silently stop working on some Windows
// builds. This function is called every frame and polls the current
// BGM / ambient status every 90 frames (~1.5 s at 60 fps). If the
// track has stopped playing it restarts it from the beginning.
// This makes looping rock-solid regardless of MCI implementation.
/* Timer maintenance moved to audio worker thread timeout */

// --- TIMER ---

static void timerCallback(int v) {
    (void)v;
    updateGame();
    glutPostRedisplay();
    glutTimerFunc(FRAME_MS, timerCallback, 0);
}

// --- FLAP ---

static void doFlap(void) {
    if (g_state == STATE_TITLE) {
        resetGame();
        playSound(SFX_FLAP);
        return;
    }
    if (g_state == STATE_GAMEOVER) {
        if (g_gameOverDelay <= 0) {
            resetGame();
        }
        return;
    }
    if (g_state == STATE_PLAYING) {
        g_bird.vy        = FLAP_VEL;
        g_bird.angle     = TILT_UP_DEG;
        g_bird.wingFrame = 1;
        playSound(SFX_FLAP);
    }
}

// --- HOVER CHECK HELPER ---
// Returns which weather button (0-3) the mouse is over, or -1.

static int hoveredDiffButton(void) {
    float startX = WORLD_W / 2.f - 170.f;
    float y = 260.f, w = 110.f, h = 40.f;  /* must match drawDifficultySelector */
    for (int i = 0; i < 3; i++) {
        float bx = startX + i * (w + 10.f);
        if (isInRect(g_mouseX, g_mouseY, bx, y, w, h)) return i;
    }
    return -1;
}

static int hoveredWeatherButton(void) {
    for (int i = 0; i < WEATHER_COUNT; i++) {
        float bx = WBTN_STARTX + i * (WBTN_W + WBTN_GAP);
        if (isInRect(g_mouseX, g_mouseY, bx, WBTN_Y, WBTN_W, WBTN_H))
            return i;
    }
    return -1;
}

static int hoveredPlayAgainButton(void) {
    float btnW = PLAY_BTN_W, btnH = PLAY_BTN_H;
    float btnX = WORLD_W / 2.f - btnW / 2.f;
    float py   = WORLD_H / 2.f - 80.f;
    float btnY = py + 12.f;
    return isInRect(g_mouseX, g_mouseY, btnX, btnY, btnW, btnH);
}

// --- KEYBOARD INPUT ---

static void keyboardInput(unsigned char key, int x, int y) {
    (void)x; (void)y;
    switch (key) {
        case ' ':
            doFlap();
            break;
        case 'r': case 'R':
            if (g_state == STATE_GAMEOVER && g_gameOverDelay <= 0) resetGame();
            else if (g_state == STATE_TITLE) resetGame();
            break;
        case 'p': case 'P':
            if      (g_state == STATE_PLAYING) g_state = STATE_PAUSED;
            else if (g_state == STATE_PAUSED)  g_state = STATE_PLAYING;
            break;
        case 'w': case 'W':
            nextWeather();
            break;
        case 27:
            exit(0);
            break;
        default:
            break;
    }
}

static void specialKeys(int key, int x, int y) {
    (void)x; (void)y;
    if (key == GLUT_KEY_F11) {
        g_fullscreen = !g_fullscreen;
        if (g_fullscreen) glutFullScreen();
        else { glutReshapeWindow(WIN_W, WIN_H); glutPositionWindow(100, 80); }
    }
}

// --- MOUSE INPUT  (click) ---

static void mouseInput(int button, int state, int x, int y) {
    if (button != GLUT_LEFT_BUTTON || state != GLUT_DOWN) return;
    screenToWorld(x, y, &g_mouseX, &g_mouseY);

    /* Close button — checked first, works in every game state */
    if (isInRect(g_mouseX, g_mouseY, CLOSE_BTN_X, CLOSE_BTN_Y,
                 CLOSE_BTN_SIZE, CLOSE_BTN_SIZE)) {
        exit(0);
    }

    /* Menu button */
    if (isInRect(g_mouseX, g_mouseY, MENU_BTN_X, MENU_BTN_Y, MENU_BTN_SIZE, MENU_BTN_SIZE)) {
        g_menuExpanded = !g_menuExpanded;
        return; /* Prevent flap */
    }

    if (g_state == STATE_TITLE || g_state == STATE_GAMEOVER) {
        /* Weather buttons work on both title and game-over screens */
        int hw = hoveredWeatherButton();
        if (hw >= 0) {
            setWeather(static_cast<WeatherMode>(hw));
            return;
        }
        if (g_state == STATE_TITLE) {
            int hd = hoveredDiffButton();
            if (hd >= 0) {
                g_difficulty = static_cast<Difficulty>(hd);
                return;
            }
            /* Clicking elsewhere on title starts the game */
            doFlap(); return;
        }
    }

    if (g_state == STATE_GAMEOVER && g_gameOverDelay <= 0) {
        if (hoveredPlayAgainButton()) {
            resetGame();
            return;
        }
    }

    if (g_state == STATE_PLAYING) {
        doFlap();
    }
}

// --- PASSIVE MOTION  (mouse move without button - hover detection) ---

static void passiveMotion(int x, int y) {
    screenToWorld(x, y, &g_mouseX, &g_mouseY);

    /* Weather button hover — active on title AND game-over screens */
    if (g_state == STATE_TITLE || g_state == STATE_GAMEOVER) {
        int prev      = g_hoveredWeather;
        g_hoveredWeather = hoveredWeatherButton();
        if (g_hoveredWeather != prev && g_hoveredWeather >= 0) {}

        if (g_state == STATE_TITLE) {
            int prevD    = g_hoveredDiff;
            g_hoveredDiff = hoveredDiffButton();
            if (g_hoveredDiff != prevD && g_hoveredDiff >= 0) {}
        }
    } else {
        g_hoveredWeather = -1;
        g_hoveredDiff    = -1;
    }

    /* Play Again button hover */
    if (g_state == STATE_GAMEOVER && g_gameOverDelay <= 0) {
        int prev         = g_hoveredPlayAgain;
        g_hoveredPlayAgain = hoveredPlayAgainButton();
        if (g_hoveredPlayAgain && !prev) {
        }
    } else {
        g_hoveredPlayAgain = 0;
    }

    /* Close button hover — always active */
    {
        int prev       = g_hoveredClose;
        g_hoveredClose = isInRect(g_mouseX, g_mouseY,
                                  CLOSE_BTN_X, CLOSE_BTN_Y,
                                  CLOSE_BTN_SIZE, CLOSE_BTN_SIZE);
        if (g_hoveredClose && !prev) {}
    }

    /* Menu button hover — always active */
    g_hoveredMenu = isInRect(g_mouseX, g_mouseY,
                             MENU_BTN_X, MENU_BTN_Y,
                             MENU_BTN_SIZE, MENU_BTN_SIZE);
}

// --- RESHAPE  (store viewport for mouse coordinate conversion) ---

static void reshape(int w, int h) {
    if (h == 0) h = 1;
    g_winH = h;
    float wa  = WORLD_W / WORLD_H, ww = (float)w / (float)h;
    int vpX, vpY, vpW, vpH;
    if (ww > wa) { vpH = h; vpW = (int)(h * wa); vpX = (w - vpW) / 2; vpY = 0; }
    else         { vpW = w; vpH = (int)(w / wa);  vpX = 0; vpY = (h - vpH) / 2; }
    g_vpX = vpX; g_vpY = vpY; g_vpW = vpW; g_vpH = vpH;
    glViewport(vpX, vpY, vpW, vpH);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluOrtho2D(0.0, WORLD_W, 0.0, WORLD_H);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
}

// --- MAIN ---

int main(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WIN_W, WIN_H);
    glutInitWindowPosition(50, 30);
    glutCreateWindow("Flying Bird  -  OpenGL Edition");

    /* Start in full screen */
    glutFullScreen();

    /* Register callbacks */
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboardInput);
    glutSpecialFunc(specialKeys);
    glutMouseFunc(mouseInput);
    glutPassiveMotionFunc(passiveMotion);   /* hover tracking */

    /* Start 60 FPS timer */
    glutTimerFunc(FRAME_MS, timerCallback, 0);

    /* Initialise all sub-systems including sounds */
    init();

    glutMainLoop();
    return 0;
}
