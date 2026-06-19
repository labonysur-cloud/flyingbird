/*
 * =========================================================
 *  FLAPPY BIRD CLONE  -  OpenGL / GLUT  -  C Language
 * =========================================================
 *
 *  Author  : Computer Graphics Lab Project
 *  Build   : Code::Blocks with MinGW + FreeGLUT
 *
 *  === HOW TO SET UP IN Code::Blocks ===
 *  1. Install MinGW (comes with Code::Blocks installer "with MinGW").
 *  2. Download FreeGLUT for MinGW from:
 *       https://www.transmissionzero.co.uk/software/freeglut-devel/
 *  3. Copy freeglut headers to:  <MinGW>\include\GL\
 *  4. Copy freeglut libs     to:  <MinGW>\lib\
 *  5. Copy freeglut.dll      to:  your project folder (next to .exe)
 *  6. In Code::Blocks:
 *       Project -> Build options -> Linker settings -> Other linker options:
 *         -lopengl32 -lglu32 -lfreeglut
 *  7. Build & Run (F9)
 *
 *  === COMPILE MANUALLY WITH MINGW ===
 *    gcc flappy_bird.c -o flappy_bird.exe -lopengl32 -lglu32 -lfreeglut
 *
 *  === CONTROLS ===
 *    SPACE / Left Mouse Click  : Flap / Start / Restart
 *    R                         : Restart (from Game Over)
 *    P                         : Pause / Unpause
 *    ESC                       : Quit
 *
 * =========================================================
 */

/* ---- Standard Headers ---- */
#include <GL/glut.h>    /* Pulls in OpenGL + GLU + GLUT               */
#include <math.h>       /* sinf, cosf, fmodf                          */
#include <stdio.h>      /* sprintf                                     */
#include <stdlib.h>     /* rand, srand, exit                          */
#include <string.h>     /* strlen                                      */
#include <time.h>       /* time (for random seed)                     */

/* =========================================================
 *  CONSTANTS & TUNING PARAMETERS
 * ========================================================= */

/* --- Window / World --- */
#define WIN_W           800
#define WIN_H           600
#define WORLD_W         800.0f
#define WORLD_H         600.0f

/* --- Bird --- */
#define BIRD_X          160.0f    /* Fixed horizontal position          */
#define BIRD_RADIUS     18.0f     /* Visual radius                      */
#define GRAVITY        -0.55f     /* Applied each frame (negative = down) */
#define FLAP_VEL        10.5f     /* Upward velocity on flap            */
#define MAX_FALL_VEL   -14.0f     /* Terminal fall speed                */
#define TILT_UP_DEG     25.0f     /* Degrees tilted up after flap       */
#define TILT_DOWN_DEG  -55.0f     /* Degrees tilted down at max fall    */

/* --- Pipes --- */
#define PIPE_W          70.0f     /* Width of each pipe                 */
#define PIPE_GAP       155.0f     /* Vertical opening between pipes     */
#define PIPE_SPACING   270.0f     /* Horizontal distance between pairs  */
#define PIPE_COUNT      3         /* Simultaneous pipe pairs on screen  */
#define PIPE_MIN_H      80.0f     /* Minimum pipe segment height        */
#define PIPE_CAP_H      20.0f     /* Height of decorative pipe cap      */
#define PIPE_BASE_SPEED  3.0f     /* Starting scroll speed              */
#define PIPE_SPEED_INC   0.004f   /* Speed increase per frame           */

/* --- Ground --- */
#define GROUND_Y        80.0f     /* Y of ground surface                */
#define GROUND_H        80.0f     /* Visual height of ground strip      */
#define GRASS_H_RATIO   0.30f     /* Fraction of GROUND_H that is grass */

/* --- Clouds --- */
#define CLOUD_COUNT      6
#define CLOUD_SPEED_FAR  0.4f
#define CLOUD_SPEED_NEAR 0.9f

/* --- City silhouette --- */
#define BUILDING_COUNT  14

/* --- Wing animation --- */
#define WING_FRAMES      3
#define WING_ANIM_RATE   8        /* Frames between wing-frame changes  */

/* --- Timing --- */
#define TARGET_FPS      60
#define FRAME_MS        (1000 / TARGET_FPS)

/* --- Game states --- */
typedef enum {
    STATE_TITLE,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_GAMEOVER
} GameState;

/* =========================================================
 *  DATA STRUCTURES
 * ========================================================= */

/* Scrolling cloud layer */
typedef struct {
    float x, y;     /* World-space position of cloud centre */
    float scale;    /* Uniform size multiplier              */
    float speed;    /* Horizontal scroll speed (px/frame)   */
    int   layer;    /* 0=far (muted), 1=near (bright)       */
} Cloud;

/* A single pipe pair */
typedef struct {
    float x;             /* Left edge X                     */
    float gapCenterY;    /* Y of the centre of the gap      */
    int   scored;        /* 1 once the bird has passed it   */
} Pipe;

/* Background building (part of city silhouette) */
typedef struct {
    float x, w, h;       /* Position and size               */
    float r, g, b;       /* Silhouette colour               */
} Building;

/* The player bird */
typedef struct {
    float y;             /* Vertical centre position        */
    float vy;            /* Vertical velocity               */
    float angle;         /* Visual tilt in degrees          */
    int   wingFrame;     /* 0, 1, or 2                      */
    int   wingTimer;     /* Counts frames to next anim step */
    int   alive;         /* 0 when dead                     */
} Bird;

/* =========================================================
 *  GLOBAL STATE
 * ========================================================= */

static GameState g_state     = STATE_TITLE;
static Bird      g_bird;
static Pipe      g_pipes[PIPE_COUNT];
static Cloud     g_clouds[CLOUD_COUNT];
static Building  g_buildings[BUILDING_COUNT];

static int       g_score     = 0;
static int       g_highScore = 0;
static float     g_pipeSpeed = PIPE_BASE_SPEED;

/* Screen shake */
static float     g_shakeX    = 0.0f;
static float     g_shakeY    = 0.0f;
static int       g_shakeTicks= 0;

/* Death flash */
static int       g_flashTicks= 0;

/* Title bob animation */
static float     g_titleBobY = 0.0f;
static float     g_titleBobT = 0.0f;

/* Ground scroll offset */
static float     g_groundScroll = 0.0f;

/* Global frame counter (drives animations) */
static int       g_frame     = 0;

/* =========================================================
 *  UTILITY HELPERS
 * ========================================================= */

static float lerpf(float a, float b, float t) { return a + (b - a) * t; }

static float clampf(float v, float lo, float hi) {
    return (v < lo) ? lo : (v > hi) ? hi : v;
}

static float randf(float lo, float hi) {
    return lo + ((float)rand() / (float)RAND_MAX) * (hi - lo);
}

/* =========================================================
 *  DRAWING PRIMITIVES
 * ========================================================= */

/* Convenience colour setters (0-255 range) */
static void col(int r, int g, int b)           { glColor3f(r/255.f,g/255.f,b/255.f);       }
static void col4(int r, int g, int b, int a)   { glColor4f(r/255.f,g/255.f,b/255.f,a/255.f); }

/* Solid filled rectangle */
static void fillRect(float x, float y, float w, float h) {
    glBegin(GL_QUADS);
        glVertex2f(x,     y);
        glVertex2f(x + w, y);
        glVertex2f(x + w, y + h);
        glVertex2f(x,     y + h);
    glEnd();
}

/* Rectangle outline of given border thickness */
static void outlineRect(float x, float y, float w, float h, float t) {
    fillRect(x,         y,         w, t);          /* bottom  */
    fillRect(x,         y + h - t, w, t);          /* top     */
    fillRect(x,         y,         t, h);          /* left    */
    fillRect(x + w - t, y,         t, h);          /* right   */
}

/* Filled circle using a triangle-fan polygon */
static void fillCircle(float cx, float cy, float r, int segs) {
    glBegin(GL_TRIANGLE_FAN);
        glVertex2f(cx, cy);
        for (int i = 0; i <= segs; i++) {
            float a = 2.0f * 3.14159265f * i / segs;
            glVertex2f(cx + cosf(a) * r, cy + sinf(a) * r);
        }
    glEnd();
}

/* =========================================================
 *  TEXT RENDERING
 * ========================================================= */

/* Draw a GLUT bitmap-font string at world position (x,y) */
static void bitmapText(float x, float y, void *font, const char *s) {
    glRasterPos2f(x, y);
    while (*s) glutBitmapCharacter(font, *s++);
}

/*
 * Draw large stroked text scaled to 'scale'.
 * GLUT stroke characters are ~120 units tall before scaling.
 */
static void strokeText(float x, float y, float scale, const char *s) {
    glPushMatrix();
        glTranslatef(x, y, 0.f);
        glScalef(scale, scale, 1.f);
        while (*s) glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, *s++);
    glPopMatrix();
}

/* Compute the pixel width of a stroke string at the given scale */
static float strokeWidth(const char *s, float scale) {
    int w = 0;
    while (*s) w += glutStrokeWidth(GLUT_STROKE_MONO_ROMAN, *s++);
    return w * scale;
}

/* =========================================================
 *  SUBSYSTEM INITIALISATION
 * ========================================================= */

static void initClouds(void) {
    for (int i = 0; i < CLOUD_COUNT; i++) {
        g_clouds[i].layer = i % 2;
        g_clouds[i].x     = randf(0.f, WORLD_W);
        g_clouds[i].y     = randf(GROUND_Y + GROUND_H + 60.f, WORLD_H - 60.f);
        g_clouds[i].scale = randf(0.8f, 1.6f);
        g_clouds[i].speed = (g_clouds[i].layer == 0) ? CLOUD_SPEED_FAR : CLOUD_SPEED_NEAR;
    }
}

static void initBuildings(void) {
    /* Dark blue-grey palette for silhouette */
    float pal[4][3] = {
        {0.25f,0.35f,0.50f}, {0.20f,0.30f,0.45f},
        {0.30f,0.40f,0.55f}, {0.18f,0.28f,0.42f}
    };
    float x = 0.f;
    for (int i = 0; i < BUILDING_COUNT; i++) {
        float w = randf(40.f, 80.f);
        float h = randf(40.f, 140.f);
        int   p = rand() % 4;
        g_buildings[i].x = x;
        g_buildings[i].w = w + 4.f;   /* slight overlap avoids seams */
        g_buildings[i].h = h;
        g_buildings[i].r = pal[p][0];
        g_buildings[i].g = pal[p][1];
        g_buildings[i].b = pal[p][2];
        x += w - randf(0.f, 20.f);
    }
}

static void initBird(void) {
    g_bird.y         = WORLD_H / 2.f;
    g_bird.vy        = 0.f;
    g_bird.angle     = 0.f;
    g_bird.wingFrame = 0;
    g_bird.wingTimer = 0;
    g_bird.alive     = 1;
}

static void initPipes(void) {
    float minC = GROUND_Y + GROUND_H * GRASS_H_RATIO + PIPE_MIN_H + PIPE_GAP / 2.f;
    float maxC = WORLD_H  - PIPE_MIN_H - PIPE_GAP / 2.f;
    for (int i = 0; i < PIPE_COUNT; i++) {
        g_pipes[i].x          = 900.f + i * PIPE_SPACING;
        g_pipes[i].gapCenterY = randf(minC, maxC);
        g_pipes[i].scored     = 0;
    }
}

/* =========================================================
 *  GAME LIFECYCLE
 * ========================================================= */

static void resetGame(void) {
    g_score       = 0;
    g_pipeSpeed   = PIPE_BASE_SPEED;
    g_shakeTicks  = 0;
    g_flashTicks  = 0;
    g_groundScroll= 0.f;
    initBird();
    initPipes();
    g_state = STATE_PLAYING;
}

static void init(void) {
    srand((unsigned int)time(NULL));

    glClearColor(0.31f, 0.75f, 0.94f, 1.f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(2.f);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, WORLD_W, 0.0, WORLD_H);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    initClouds();
    initBuildings();
    initBird();
    initPipes();
    g_state = STATE_TITLE;
}

/* =========================================================
 *  SCREEN SHAKE
 * ========================================================= */

static void triggerShake(int ticks) { g_shakeTicks = ticks; }

static void updateShake(void) {
    if (g_shakeTicks > 0) {
        float m = 6.f * ((float)g_shakeTicks / 15.f);
        g_shakeX = randf(-m, m);
        g_shakeY = randf(-m, m);
        g_shakeTicks--;
    } else {
        g_shakeX = g_shakeY = 0.f;
    }
}

/* =========================================================
 *  DRAW: SKY BACKGROUND  (gradient)
 * ========================================================= */

static void drawBackground(void) {
    glBegin(GL_QUADS);
        /* Horizon (lighter, warmer cyan) */
        glColor3f(0.45f, 0.83f, 0.97f);
        glVertex2f(0,       GROUND_Y + GROUND_H);
        glVertex2f(WORLD_W, GROUND_Y + GROUND_H);
        /* Zenith (deeper blue) */
        glColor3f(0.22f, 0.60f, 0.90f);
        glVertex2f(WORLD_W, WORLD_H);
        glVertex2f(0,       WORLD_H);
    glEnd();
}

/* =========================================================
 *  DRAW: CITY SILHOUETTE
 * ========================================================= */

static void drawCitySilhouette(void) {
    float groundSurface = GROUND_Y + GROUND_H * GRASS_H_RATIO;
    for (int i = 0; i < BUILDING_COUNT; i++) {
        float bx = g_buildings[i].x;
        float bw = g_buildings[i].w;
        float bh = g_buildings[i].h;
        float by = groundSurface - 2.f;

        glColor3f(g_buildings[i].r, g_buildings[i].g, g_buildings[i].b);
        fillRect(bx, by, bw, bh);

        /* Small lit windows (pseudo-random pattern) */
        col(215, 238, 255);
        float wSz = 5.f, wGX = 14.f, wGY = 18.f;
        for (float wy = by + 10.f; wy + wSz < by + bh - 10.f; wy += wGY) {
            for (float wx = bx + 8.f; wx + wSz < bx + bw - 8.f; wx += wGX) {
                if ((int)(wx*7 + wy*13) % 3 != 0)
                    fillRect(wx, wy, wSz, wSz);
            }
        }
    }
}

/* =========================================================
 *  DRAW: CLOUDS
 * ========================================================= */

/*
 * Draw one fluffy cloud at (cx,cy) with given scale.
 * Composed of 5 overlapping ellipses drawn as scaled circles.
 */
static void drawCloud(float cx, float cy, float scale) {
    /* Each entry: {offset-x, offset-y, radius-x, radius-y} */
    static const float parts[5][4] = {
        {  0.f,  0.f, 30.f, 20.f },
        { 28.f,  6.f, 22.f, 16.f },
        {-28.f,  4.f, 20.f, 14.f },
        { 12.f, 14.f, 20.f, 14.f },
        {-12.f, 12.f, 18.f, 13.f },
    };
    for (int i = 0; i < 5; i++) {
        glPushMatrix();
            glTranslatef(cx + parts[i][0]*scale, cy + parts[i][1]*scale, 0.f);
            glScalef(parts[i][2]*scale, parts[i][3]*scale, 1.f);
            glBegin(GL_TRIANGLE_FAN);
                glVertex2f(0,0);
                for (int s = 0; s <= 16; s++) {
                    float a = 2.f*3.14159265f*s/16;
                    glVertex2f(cosf(a), sinf(a));
                }
            glEnd();
        glPopMatrix();
    }
}

static void drawClouds(void) {
    for (int i = 0; i < CLOUD_COUNT; i++) {
        if (g_clouds[i].layer == 0) col4(195,222,245,205);
        else                         col4(255,255,255,235);
        drawCloud(g_clouds[i].x, g_clouds[i].y, g_clouds[i].scale);
    }
}

/* =========================================================
 *  DRAW: GROUND
 * ========================================================= */

static void drawGround(void) {
    float grassH = GROUND_H * GRASS_H_RATIO;

    /* Dirt base */
    col(222,184,135);
    fillRect(0, 0, WORLD_W, GROUND_Y);

    /* Grass layer */
    col(124,194,66);
    fillRect(0, GROUND_Y, WORLD_W, grassH);

    /* Scrolling grass tile details */
    col(100,170,50);
    float tileW = 40.f;
    float scroll = fmodf(g_groundScroll, tileW * 2.f);
    for (float tx = -tileW*2.f + scroll; tx < WORLD_W + tileW; tx += tileW*2.f)
        fillRect(tx, GROUND_Y + 4.f, tileW - 4.f, 8.f);

    /* Grass top edge line */
    col(80,140,35);
    fillRect(0, GROUND_Y + grassH - 2.f, WORLD_W, 3.f);

    /* Static pebbles in the dirt */
    col(190,155,110);
    for (int i = 0; i < 20; i++) {
        float px = fmodf(i*123.7f + 37.f, WORLD_W);
        float py = 12.f + fmodf(i*71.3f, GROUND_Y - 20.f);
        fillRect(px, py, 5.f, 3.f);
    }
}

/* =========================================================
 *  DRAW: BIRD
 *  Round body, coloured wing with 3-frame animation,
 *  eye with shine, orange beak — all in original pixel style.
 * ========================================================= */

static void drawBird(void) {
    float cx = BIRD_X;
    float cy = g_bird.y;

    glPushMatrix();
    glTranslatef(cx, cy, 0.f);
    glRotatef(g_bird.angle, 0.f, 0.f, 1.f);

    /* --- Body --- */
    col(255,195,50);
    fillCircle(0,0, BIRD_RADIUS, 20);

    /* Body outline */
    col(200,120,20);
    glLineWidth(2.5f);
    glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 20; i++) {
            float a = 2.f*3.14159265f*i/20;
            glVertex2f(cosf(a)*BIRD_RADIUS, sinf(a)*BIRD_RADIUS);
        }
    glEnd();
    glLineWidth(2.f);

    /* --- Wing (3-frame animation: mid=0, up=1, down=2) --- */
    float wingYOff[3] = {0.f, 8.f, -8.f};
    float wy = wingYOff[g_bird.wingFrame];
    col(230,155,20);
    glPushMatrix();
        glTranslatef(-6.f, wy, 0.f);
        glScalef(10.f, 6.f, 1.f);
        glBegin(GL_TRIANGLE_FAN);
            glVertex2f(0,0);
            for (int s = 0; s <= 14; s++) {
                float a = 2.f*3.14159265f*s/14;
                glVertex2f(cosf(a),sinf(a));
            }
        glEnd();
    glPopMatrix();
    /* Wing outline */
    col(190,110,10);
    glPushMatrix();
        glTranslatef(-6.f, wy, 0.f);
        glScalef(10.f, 6.f, 1.f);
        glBegin(GL_LINE_LOOP);
            for (int s = 0; s < 14; s++) {
                float a = 2.f*3.14159265f*s/14;
                glVertex2f(cosf(a),sinf(a));
            }
        glEnd();
    glPopMatrix();

    /* --- Belly highlight --- */
    col(255,230,150);
    glPushMatrix();
        glTranslatef(3.f,-3.f,0.f);
        glScalef(10.f,8.f,1.f);
        glBegin(GL_TRIANGLE_FAN);
            glVertex2f(0,0);
            for (int s = 0; s <= 14; s++) {
                float a = 2.f*3.14159265f*s/14;
                glVertex2f(cosf(a),sinf(a));
            }
        glEnd();
    glPopMatrix();

    /* --- Eye white --- */
    col(255,255,255);
    fillCircle(6.f,5.f,6.5f,14);

    /* --- Pupil --- */
    col(30,30,30);
    fillCircle(7.5f,4.5f,3.5f,12);

    /* --- Eye shine --- */
    col(255,255,255);
    fillCircle(8.5f,5.5f,1.2f,8);

    /* --- Beak (orange triangle) --- */
    col(255,120,30);
    glBegin(GL_TRIANGLES);
        glVertex2f(BIRD_RADIUS-2.f,  3.5f);
        glVertex2f(BIRD_RADIUS+12.f, 0.f);
        glVertex2f(BIRD_RADIUS-2.f, -3.5f);
    glEnd();
    col(200,80,10);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
        glVertex2f(BIRD_RADIUS-2.f,  3.5f);
        glVertex2f(BIRD_RADIUS+12.f, 0.f);
        glVertex2f(BIRD_RADIUS-2.f, -3.5f);
    glEnd();
    glLineWidth(2.f);

    glPopMatrix();
}

/* =========================================================
 *  DRAW: PIPES
 *  Classic green pipes with cap and highlight/shadow stripes.
 * ========================================================= */

/*
 * drawSinglePipe -- draws one pipe segment from y1 to y2.
 *   flipped = 0 : cap is at top  (bottom pipe, pointing up)
 *   flipped = 1 : cap is at bottom (top pipe, hanging down)
 */
static void drawSinglePipe(float x, float y1, float y2, int flipped) {
    float capExtraW = 14.f;
    float capOff    = capExtraW / 2.f;
    float capW      = PIPE_W + capExtraW;

    /* Main shaft */
    col(80,200,80);
    fillRect(x, y1, PIPE_W, y2-y1);

    /* Highlight stripe */
    col(120,230,100);
    fillRect(x+8.f, y1, 12.f, y2-y1);

    /* Shadow stripe */
    col(40,140,40);
    fillRect(x+PIPE_W-10.f, y1, 10.f, y2-y1);

    /* Shaft outline */
    col(30,110,30);
    outlineRect(x, y1, PIPE_W, y2-y1, 2.5f);

    /* Cap */
    float capY = flipped ? y1-4.f : y2-PIPE_CAP_H;
    float capH = PIPE_CAP_H + 4.f;

    col(80,200,80);
    fillRect(x-capOff, capY, capW, capH);

    col(120,230,100);
    fillRect(x-capOff+8.f, capY, 14.f, capH);

    col(40,140,40);
    fillRect(x-capOff+capW-12.f, capY, 12.f, capH);

    col(30,110,30);
    outlineRect(x-capOff, capY, capW, capH, 2.5f);
}

static void drawPipes(void) {
    float groundSurface = GROUND_Y + GROUND_H * GRASS_H_RATIO;
    for (int i = 0; i < PIPE_COUNT; i++) {
        float gapTop = g_pipes[i].gapCenterY + PIPE_GAP/2.f;
        float gapBot = g_pipes[i].gapCenterY - PIPE_GAP/2.f;
        float px     = g_pipes[i].x;

        drawSinglePipe(px, groundSurface, gapBot, 0);     /* bottom pipe */
        drawSinglePipe(px, gapTop, WORLD_H+10.f, 1);      /* top pipe    */
    }
}

/* =========================================================
 *  DRAW: HUD
 * ========================================================= */

static void drawHUD(void) {
    char buf[32];

    /* Centred score counter */
    sprintf(buf, "%d", g_score);
    float cx = WORLD_W/2.f;
    float cy = WORLD_H - 55.f;
    float sw = strokeWidth(buf, 0.20f);

    col(40,40,40);
    strokeText(cx-sw/2.f+2.f, cy-2.f, 0.20f, buf);   /* drop shadow */

    col(255,255,255);
    strokeText(cx-sw/2.f, cy, 0.20f, buf);            /* white text  */

    /* Top-left high score */
    sprintf(buf, "BEST: %d", g_highScore);
    col(50,50,50);
    bitmapText(10.f, WORLD_H-22.f, GLUT_BITMAP_HELVETICA_18, buf);
}

/* =========================================================
 *  DRAW: TITLE SCREEN
 * ========================================================= */

static void drawTitleScreen(void) {
    float tY = WORLD_H*0.62f + g_titleBobY;  /* Bobbing anchor point */

    /* Title shadow */
    col(50,80,20);
    float sc = 0.30f;
    float w1 = strokeWidth("FLAPPY", sc);
    float w2 = strokeWidth("BIRD",   sc);
    strokeText(WORLD_W/2.f-w1/2.f+3.f, tY-3.f,      sc, "FLAPPY");
    strokeText(WORLD_W/2.f-w2/2.f+3.f, tY-55.f-3.f, sc, "BIRD");

    /* Title text – yellow FLAPPY */
    col(255,220,40);
    strokeText(WORLD_W/2.f-w1/2.f, tY,      sc, "FLAPPY");

    /* Green BIRD */
    col(120,230,60);
    strokeText(WORLD_W/2.f-w2/2.f, tY-55.f, sc, "BIRD");

    /* Decorative border box */
    float bx = WORLD_W/2.f-160.f, by = tY-70.f;
    col(255,255,255);
    outlineRect(bx, by, 320.f, 110.f, 4.f);
    col(50,120,20);
    outlineRect(bx+4.f, by+4.f, 312.f, 102.f, 3.f);

    /* Pulsing start prompt */
    float pulse = 0.6f + 0.4f*sinf(g_frame*0.08f);
    col4(255,255,255,(int)(pulse*255));
    const char *prompt = "Press SPACE or Click to Start";
    float pw = strokeWidth(prompt, 0.085f);
    strokeText(WORLD_W/2.f-pw/2.f, WORLD_H*0.30f, 0.085f, prompt);

    /* Bird preview positioned near the title */
    float savedY   = g_bird.y;
    float savedAng = g_bird.angle;
    g_bird.y     = tY - 25.f;
    g_bird.angle = 0.f;
    drawBird();
    g_bird.y     = savedY;
    g_bird.angle = savedAng;

    /* Hint line at the bottom */
    col(30,60,10);
    bitmapText(WORLD_W/2.f-80.f, GROUND_Y+GROUND_H+10.f,
               GLUT_BITMAP_HELVETICA_12, "P = Pause   ESC = Quit");
}

/* =========================================================
 *  DRAW: GAME OVER PANEL
 * ========================================================= */

static void drawGameOverScreen(void) {
    /* Darken scene */
    col4(0,0,0,105);
    fillRect(0,0,WORLD_W,WORLD_H);

    /* Info panel */
    float px = WORLD_W/2.f-180.f, py = WORLD_H/2.f-80.f;
    col4(250,240,200,242);
    fillRect(px, py, 360.f, 175.f);
    col(90,60,20);
    outlineRect(px,py,360.f,175.f,4.f);
    col(180,130,50);
    outlineRect(px+4.f,py+4.f,352.f,167.f,3.f);

    /* "GAME OVER" text */
    const char *go = "GAME OVER";
    float gsc = 0.20f;
    float gw  = strokeWidth(go, gsc);
    col(200,40,40);
    strokeText(WORLD_W/2.f-gw/2.f+2.f, py+125.f-2.f, gsc, go);
    col(255,80,80);
    strokeText(WORLD_W/2.f-gw/2.f,     py+125.f,     gsc, go);

    /* Score info */
    char buf[32];
    sprintf(buf,"Score: %d", g_score);
    col(60,40,10);
    bitmapText(WORLD_W/2.f-55.f, py+90.f, GLUT_BITMAP_HELVETICA_18, buf);

    sprintf(buf,"Best:  %d", g_highScore);
    bitmapText(WORLD_W/2.f-55.f, py+65.f, GLUT_BITMAP_HELVETICA_18, buf);

    /* Pulsing restart prompt */
    float pulse = 0.55f + 0.45f*sinf(g_frame*0.10f);
    col4(80,40,0,(int)(pulse*255));
    const char *rp = "Press SPACE or R to Restart";
    float rpw = strokeWidth(rp, 0.065f);
    strokeText(WORLD_W/2.f-rpw/2.f, py+18.f, 0.065f, rp);
}

/* =========================================================
 *  DRAW: PAUSE OVERLAY
 * ========================================================= */

static void drawPauseScreen(void) {
    col4(0,0,0,80);
    fillRect(0,0,WORLD_W,WORLD_H);

    const char *pt = "PAUSED";
    float psc = 0.25f;
    float pw  = strokeWidth(pt, psc);
    col(255,255,255);
    strokeText(WORLD_W/2.f-pw/2.f, WORLD_H/2.f-10.f, psc, pt);

    col(200,200,200);
    bitmapText(WORLD_W/2.f-75.f, WORLD_H/2.f-50.f,
               GLUT_BITMAP_HELVETICA_18, "Press P to Resume");
}

/* =========================================================
 *  DISPLAY CALLBACK  (called by GLUT each frame)
 * ========================================================= */

static void display(void) {
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    /* Apply shake offset to entire scene */
    glTranslatef(g_shakeX, g_shakeY, 0.f);

    /* Always draw sky + clouds + city silhouette */
    drawBackground();
    drawClouds();
    drawCitySilhouette();

    /* State-specific content */
    switch (g_state) {
        case STATE_TITLE:
            drawGround();
            drawTitleScreen();
            break;

        case STATE_PLAYING:
            drawPipes();
            drawGround();
            drawBird();
            drawHUD();
            break;

        case STATE_PAUSED:
            drawPipes();
            drawGround();
            drawBird();
            drawHUD();
            drawPauseScreen();
            break;

        case STATE_GAMEOVER:
            drawPipes();
            drawGround();
            drawBird();
            drawHUD();
            drawGameOverScreen();
            break;
    }

    /* White flash on death */
    if (g_flashTicks > 0) {
        float alpha = (float)g_flashTicks / 10.f;
        col4(255,255,255,(int)(alpha*200));
        fillRect(0,0,WORLD_W,WORLD_H);
    }

    glutSwapBuffers();
}

/* =========================================================
 *  COLLISION DETECTION
 *  Uses a slightly-reduced circle hitbox for fairness.
 * ========================================================= */

static int checkCollision(void) {
    float bx = BIRD_X;
    float by = g_bird.y;
    float br = BIRD_RADIUS - 4.f;   /* Forgiving hitbox radius */

    float groundSurface = GROUND_Y + GROUND_H * GRASS_H_RATIO;

    /* Ground and ceiling */
    if (by - br <= groundSurface) return 1;
    if (by + br >= WORLD_H)       return 1;

    /* Pipe pairs */
    for (int i = 0; i < PIPE_COUNT; i++) {
        float px    = g_pipes[i].x;
        float gapTop = g_pipes[i].gapCenterY + PIPE_GAP/2.f;
        float gapBot = g_pipes[i].gapCenterY - PIPE_GAP/2.f;
        float capOff = 7.f;   /* Extra width from caps */

        /* Horizontal overlap test */
        if (bx + br > px - capOff && bx - br < px + PIPE_W + capOff) {
            if (by - br < gapBot) return 1;   /* Hit bottom pipe */
            if (by + br > gapTop) return 1;   /* Hit top pipe    */
        }
    }
    return 0;
}

/* =========================================================
 *  UPDATE: BIRD  (physics + animation)
 * ========================================================= */

static void updateBird(void) {
    /* Gravity */
    g_bird.vy += GRAVITY;
    if (g_bird.vy < MAX_FALL_VEL) g_bird.vy = MAX_FALL_VEL;
    g_bird.y += g_bird.vy;

    /* Tilt: aim toward angle based on velocity */
    float targetAngle;
    if (g_bird.vy > 2.f) {
        targetAngle = TILT_UP_DEG;
    } else {
        float t = clampf((g_bird.vy - MAX_FALL_VEL) / (2.f - MAX_FALL_VEL), 0.f, 1.f);
        targetAngle = lerpf(TILT_DOWN_DEG, 0.f, t);
    }
    g_bird.angle += (targetAngle - g_bird.angle) * 0.25f;

    /* Wing animation */
    if (++g_bird.wingTimer >= WING_ANIM_RATE) {
        g_bird.wingTimer = 0;
        g_bird.wingFrame = (g_bird.wingFrame + 1) % WING_FRAMES;
    }
}

/* =========================================================
 *  UPDATE: PIPES  (scrolling + recycling + scoring)
 * ========================================================= */

static void updatePipes(void) {
    float minC = GROUND_Y + GROUND_H*GRASS_H_RATIO + PIPE_MIN_H + PIPE_GAP/2.f;
    float maxC = WORLD_H  - PIPE_MIN_H - PIPE_GAP/2.f;

    for (int i = 0; i < PIPE_COUNT; i++) {
        g_pipes[i].x -= g_pipeSpeed;

        /* Recycle: find rightmost pipe and place this one after it */
        if (g_pipes[i].x + PIPE_W < -20.f) {
            float maxX = g_pipes[i].x;
            for (int j = 0; j < PIPE_COUNT; j++)
                if (j != i && g_pipes[j].x > maxX) maxX = g_pipes[j].x;
            g_pipes[i].x          = maxX + PIPE_SPACING;
            g_pipes[i].gapCenterY = randf(minC, maxC);
            g_pipes[i].scored     = 0;
        }

        /* Score when bird centre passes the pipe centre */
        if (!g_pipes[i].scored && g_pipes[i].x + PIPE_W/2.f < BIRD_X) {
            g_pipes[i].scored = 1;
            g_score++;
            if (g_score > g_highScore) g_highScore = g_score;
            /* Sound hook: playSound(SFX_SCORE); */
        }
    }

    /* Difficulty ramp-up */
    g_pipeSpeed += PIPE_SPEED_INC;
}

/* =========================================================
 *  UPDATE: CLOUDS
 * ========================================================= */

static void updateClouds(void) {
    for (int i = 0; i < CLOUD_COUNT; i++) {
        g_clouds[i].x -= g_clouds[i].speed;
        if (g_clouds[i].x + 80.f*g_clouds[i].scale < 0.f) {
            g_clouds[i].x = WORLD_W + 60.f;
            g_clouds[i].y = randf(GROUND_Y+GROUND_H+60.f, WORLD_H-60.f);
        }
    }
}

/* =========================================================
 *  MASTER UPDATE  (all game logic, called once per frame)
 * ========================================================= */

static void updateGame(void) {
    g_frame++;

    if (g_state == STATE_TITLE) {
        updateClouds();
        g_titleBobT += 0.05f;
        g_titleBobY  = 8.f * sinf(g_titleBobT);
        /* Animate showcase bird wing */
        if (++g_bird.wingTimer >= WING_ANIM_RATE) {
            g_bird.wingTimer = 0;
            g_bird.wingFrame = (g_bird.wingFrame+1) % WING_FRAMES;
        }
        return;
    }

    if (g_state == STATE_PAUSED) return;

    if (g_state == STATE_GAMEOVER) {
        if (g_flashTicks > 0) g_flashTicks--;
        updateShake();
        return;
    }

    /* ---- STATE_PLAYING ---- */
    updateBird();
    updatePipes();
    updateClouds();
    g_groundScroll += g_pipeSpeed;

    if (checkCollision()) {
        g_state       = STATE_GAMEOVER;
        g_bird.alive  = 0;
        g_flashTicks  = 10;
        triggerShake(15);
        /* Sound hook: playSound(SFX_DIE); */
    }

    updateShake();
    if (g_flashTicks > 0) g_flashTicks--;
}

/* =========================================================
 *  TIMER CALLBACK  (drives 60 FPS game loop)
 * ========================================================= */

static void timerCallback(int val) {
    (void)val;
    updateGame();
    glutPostRedisplay();
    glutTimerFunc(FRAME_MS, timerCallback, 0);
}

/* =========================================================
 *  FLAP  (shared action: start, flap, restart)
 * ========================================================= */

static void doFlap(void) {
    if (g_state == STATE_TITLE || g_state == STATE_GAMEOVER) {
        resetGame();
        return;
    }
    if (g_state == STATE_PLAYING) {
        g_bird.vy        = FLAP_VEL;
        g_bird.angle     = TILT_UP_DEG;
        g_bird.wingFrame = 1;   /* "wing up" frame immediately */
        /* Sound hook: playSound(SFX_FLAP); */
    }
}

/* =========================================================
 *  INPUT CALLBACKS
 * ========================================================= */

static void keyboardInput(unsigned char key, int x, int y) {
    (void)x; (void)y;
    switch (key) {
        case ' ':            doFlap(); break;

        case 'r': case 'R':
            if (g_state == STATE_GAMEOVER || g_state == STATE_TITLE)
                resetGame();
            break;

        case 'p': case 'P':
            if      (g_state == STATE_PLAYING) g_state = STATE_PAUSED;
            else if (g_state == STATE_PAUSED)  g_state = STATE_PLAYING;
            break;

        case 27: exit(0); break;   /* ESC – quit */
        default: break;
    }
}

static void mouseInput(int button, int state, int x, int y) {
    (void)x; (void)y;
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
        doFlap();
}

/* =========================================================
 *  RESHAPE  (pillar-/letter-box to preserve aspect ratio)
 * ========================================================= */

static void reshape(int w, int h) {
    if (h == 0) h = 1;
    float worldAspect = WORLD_W / WORLD_H;
    float winAspect   = (float)w / (float)h;
    int vpX, vpY, vpW, vpH;

    if (winAspect > worldAspect) {
        vpH = h;  vpW = (int)(h*worldAspect);
        vpX = (w-vpW)/2;  vpY = 0;
    } else {
        vpW = w;  vpH = (int)(w/worldAspect);
        vpX = 0;  vpY = (h-vpH)/2;
    }

    glViewport(vpX, vpY, vpW, vpH);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, WORLD_W, 0.0, WORLD_H);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

/* =========================================================
 *  PROGRAM ENTRY POINT
 * ========================================================= */

int main(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);  /* Double-buffer for smooth rendering */
    glutInitWindowSize(WIN_W, WIN_H);
    glutInitWindowPosition(100, 80);
    glutCreateWindow("Flappy Bird  -  OpenGL Edition");

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboardInput);
    glutMouseFunc(mouseInput);

    glutTimerFunc(FRAME_MS, timerCallback, 0);  /* Start 60-FPS timer */

    init();           /* Seed random, set up GL state, init game objects */
    glutMainLoop();   /* Hand control to GLUT event loop (never returns) */
    return 0;
}

/* =========================================================
 *  SOUND HOOK STUBS  (add audio with Windows API or OpenAL)
 *
 *  #include <windows.h>
 *  #include <mmsystem.h>   // link: -lwinmm
 *
 *  #define SFX_FLAP  0
 *  #define SFX_SCORE 1
 *  #define SFX_DIE   2
 *
 *  void playSound(int id) {
 *      const char *files[] = {"flap.wav","score.wav","die.wav"};
 *      PlaySound(files[id], NULL, SND_FILENAME | SND_ASYNC);
 *  }
 * ========================================================= */
