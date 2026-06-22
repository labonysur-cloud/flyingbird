/*
 * ================================================================
 *  FLAPPY BIRD CLONE  -  OpenGL / GLUT  -  C Language
 *  Final Edition
 *
 *  Author  : Labony Sur
 *  Build   : Code::Blocks + MinGW + FreeGLUT
 *
 *  Linker flags required:
 *    -lopengl32 -lglu32 -lfreeglut -lwinmm -lm -std=c99
 *
 *  New features in this version:
 *    - Interactive weather selector on title screen (click to choose)
 *    - Play Again button on game over (with 1.5 second delay)
 *    - Hover effects on all buttons
 *    - Full sound effects: flap, score, die, click, hover, weather
 *    - Sound is generated in memory, no WAV files needed
 *    - Mouse coordinate conversion for accurate hit detection
 *
 *  Controls:
 *    SPACE or Left Click : Flap / Start / Restart
 *    W                   : Cycle weather mode
 *    P                   : Pause or unpause
 *    R                   : Restart (after delay on game over)
 *    F11                 : Toggle full screen
 *    ESC                 : Quit
 * ================================================================
 */

/* Windows audio */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

/* OpenGL */
#include <GL/glut.h>

/* Standard */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ================================================================
 *  CONSTANTS
 * ================================================================ */

#define WIN_W            800
#define WIN_H            600
#define WORLD_W          800.0f
#define WORLD_H          600.0f

/* Bird */
#define BIRD_X           160.0f
#define BIRD_RADIUS      18.0f
#define GRAVITY         -0.45f
#define FLAP_VEL         10.0f
#define MAX_FALL_VEL    -13.0f
#define TILT_UP_DEG      25.0f
#define TILT_DOWN_DEG   -55.0f

/* Pipes */
#define PIPE_W           70.0f
#define PIPE_GAP         190.0f
#define PIPE_SPACING     290.0f
#define PIPE_COUNT       3
#define PIPE_MIN_H       80.0f
#define PIPE_CAP_H       20.0f
#define PIPE_BASE_SPEED  2.7f
#define PIPE_SPEED_INC   0.002f

/* Ground */
#define GROUND_Y         80.0f
#define GROUND_H         80.0f
#define GRASS_H_RATIO    0.30f

/* Weather */
#define WEATHER_COUNT        4
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
#define WBTN_W           128.0f
#define WBTN_H            70.0f
#define WBTN_GAP          10.0f
/* Total width = 4*128 + 3*10 = 542; start X = (800-542)/2 = 129 */
#define WBTN_STARTX      129.0f
#define WBTN_Y           265.0f   /* bottom edge of buttons */

/* Play Again button (game over screen) */
#define PLAY_BTN_W       200.0f
#define PLAY_BTN_H        46.0f

/* Sound sample rate */
#define SFX_RATE         22050
#define SFX_BUF          65536   /* 64 KB per sound, enough for ~1.5 sec */

/* Sound IDs */
#define SFX_FLAP    0
#define SFX_SCORE   1
#define SFX_DIE     2
#define SFX_CLICK   3
#define SFX_HOVER   4
#define SFX_WEATHER 5
#define SFX_COUNT   6

/* ================================================================
 *  ENUMS
 * ================================================================ */

typedef enum { STATE_TITLE, STATE_PLAYING, STATE_PAUSED, STATE_GAMEOVER } GameState;

typedef enum {
    WEATHER_DAY   = 0,
    WEATHER_SUNNY = 1,
    WEATHER_RAIN  = 2,
    WEATHER_NIGHT = 3
} WeatherMode;

/* ================================================================
 *  STRUCTS
 * ================================================================ */

typedef struct { float x, y, scale, speed; int layer; } Cloud;
typedef struct { float x, gapCenterY; int scored; } Pipe;
typedef struct { float x, w, h, r, g, b; } Building;
typedef struct { float y, vy, angle; int wingFrame, wingTimer, alive; } Bird;
typedef struct { float x, y, speed, alpha, len; } RainDrop;
typedef struct { float x, y, size, phase; } Star;

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

static const WeatherTheme g_themes[4] = {
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
      0.12f,0.15f,0.32f,  "Night", "Stars, moon, city lights" }
};

/* ================================================================
 *  GLOBALS
 * ================================================================ */

/* Game state */
static GameState  g_state     = STATE_TITLE;
static Bird       g_bird;
static Pipe       g_pipes[PIPE_COUNT];
static Cloud      g_clouds[CLOUD_COUNT];
static Building   g_buildings[BUILDING_COUNT];
static RainDrop   g_rain[RAIN_COUNT];
static Star       g_stars[STAR_COUNT];
static Particle   g_particles[PARTICLE_COUNT];

static int        g_score     = 0;
static int        g_highScore = 0;
static float      g_pipeSpeed = PIPE_BASE_SPEED;

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

/* Mouse tracking (world space) */
static float       g_mouseX = 0, g_mouseY = 0;

/* Hover states */
static int         g_hoveredWeather   = -1;  /* -1 = none */
static int         g_hoveredPlayAgain = 0;

/* Viewport (for mouse coordinate conversion) */
static int         g_vpX = 0, g_vpY = 0, g_vpW = WIN_W, g_vpH = WIN_H;
static int         g_winH = WIN_H;

/* Sound buffers */
static unsigned char g_sfxBuf[SFX_COUNT][SFX_BUF];
static int           g_sfxSize[SFX_COUNT];

/* ================================================================
 *  UTILITY
 * ================================================================ */

static float lerpf(float a,float b,float t){ return a+(b-a)*t; }

static float clampf(float v,float lo,float hi){
    return v<lo?lo:v>hi?hi:v;
}

static float randf(float lo,float hi){
    return lo+((float)rand()/(float)RAND_MAX)*(hi-lo);
}

/* Rectangle hit test in world space */
static int isInRect(float mx,float my,float rx,float ry,float rw,float rh){
    return mx>=rx && mx<=rx+rw && my>=ry && my<=ry+rh;
}

/* Convert GLUT screen pixel to world coordinate */
static void screenToWorld(int sx,int sy,float *wx,float *wy){
    /* GLUT Y is from top; OpenGL is from bottom */
    int sy_gl = g_winH - sy;
    float rx = (float)(sx - g_vpX) / (float)g_vpW;
    float ry = (float)(sy_gl - g_vpY) / (float)g_vpH;
    *wx = rx * WORLD_W;
    *wy = ry * WORLD_H;
}

/* ================================================================
 *  DRAWING PRIMITIVES
 * ================================================================ */

static void col(int r,int g,int b)
    { glColor3f(r/255.f,g/255.f,b/255.f); }
static void col4(int r,int g,int b,int a)
    { glColor4f(r/255.f,g/255.f,b/255.f,a/255.f); }
static void colF(float r,float g,float b)
    { glColor3f(r,g,b); }
static void colF4(float r,float g,float b,float a)
    { glColor4f(r,g,b,a); }

static void fillRect(float x,float y,float w,float h){
    glBegin(GL_QUADS);
        glVertex2f(x,y);    glVertex2f(x+w,y);
        glVertex2f(x+w,y+h); glVertex2f(x,y+h);
    glEnd();
}

static void outlineRect(float x,float y,float w,float h,float t){
    fillRect(x,y,w,t);
    fillRect(x,y+h-t,w,t);
    fillRect(x,y,t,h);
    fillRect(x+w-t,y,t,h);
}

/* Rounded rectangle (corner radius r, approximated as rect + circles) */
static void fillRoundRect(float x,float y,float w,float h,float rc){
    fillRect(x+rc, y,    w-rc*2, h);         /* centre strip  */
    fillRect(x,    y+rc, w,      h-rc*2);    /* wide strip    */
    /* Four corner circles */
    int seg=10;
    float corners[4][2]={{x+rc,y+rc},{x+w-rc,y+rc},{x+w-rc,y+h-rc},{x+rc,y+h-rc}};
    float startA[4]={180,270,0,90};
    for(int c=0;c<4;c++){
        glBegin(GL_TRIANGLE_FAN);
            glVertex2f(corners[c][0],corners[c][1]);
            for(int i=0;i<=seg;i++){
                float a=(startA[c]+90.f*i/seg)*3.14159f/180.f;
                glVertex2f(corners[c][0]+cosf(a)*rc,corners[c][1]+sinf(a)*rc);
            }
        glEnd();
    }
}

static void fillCircle(float cx,float cy,float r,int segs){
    glBegin(GL_TRIANGLE_FAN);
        glVertex2f(cx,cy);
        for(int i=0;i<=segs;i++){
            float a=2.f*3.14159265f*i/segs;
            glVertex2f(cx+cosf(a)*r,cy+sinf(a)*r);
        }
    glEnd();
}

static void fillEllipse(float cx,float cy,float rx,float ry,int segs){
    glPushMatrix();
        glTranslatef(cx,cy,0.f);
        glScalef(rx,ry,1.f);
        glBegin(GL_TRIANGLE_FAN);
            glVertex2f(0,0);
            for(int i=0;i<=segs;i++){
                float a=2.f*3.14159265f*i/segs;
                glVertex2f(cosf(a),sinf(a));
            }
        glEnd();
    glPopMatrix();
}

/* ================================================================
 *  ALGORITHM 1 : DDA LINE  (Digital Differential Analyser)
 *
 *  Principle: Compute the number of steps = max(|dx|,|dy|),
 *  then increment x and y by dx/steps and dy/steps each step.
 *  This guarantees exactly one pixel plotted per step.
 *
 *  Usage in this project:
 *    - Draws the four outline edges of every pipe body and cap.
 * ================================================================ */
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

/* ================================================================
 *  ALGORITHM 2 : MIDPOINT CIRCLE  (Bresenham's Circle Algorithm)
 *
 *  Principle: Start at (0, r). Use an integer decision variable
 *  d = 1 - r to choose between East (x++) and South-East
 *  (x++, y--) at each step. Exploits 8-fold symmetry so only
 *  one octant needs to be computed.
 *
 *  Usage in this project:
 *    - Draws the bird's circular body outline every frame.
 *    - Also used for the semi-transparent reflection outline.
 * ================================================================ */
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

/* ================================================================
 *  ALGORITHM 3 : BRESENHAM LINE  (integer error-accumulation)
 *
 *  Principle: Maintain an error term err = dx - dy. Each step,
 *  advance in the major axis and conditionally step in the minor
 *  axis when the accumulated error exceeds 0. No floating-point
 *  arithmetic needed.
 *
 *  Usage in this project:
 *    - Draws the lightning bolt zigzag segments in RAIN mode.
 * ================================================================ */
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

/* ================================================================
 *  TEXT
 * ================================================================ */

static void bitmapText(float x,float y,void *font,const char *s){
    glRasterPos2f(x,y);
    while(*s) glutBitmapCharacter(font,*s++);
}

static void strokeText(float x,float y,float scale,const char *s){
    glPushMatrix();
        glTranslatef(x,y,0.f);
        glScalef(scale,scale,1.f);
        while(*s) glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN,*s++);
    glPopMatrix();
}

static float strokeWidth(const char *s,float scale){
    int w=0;
    while(*s) w+=glutStrokeWidth(GLUT_STROKE_MONO_ROMAN,*s++);
    return w*scale;
}

/* ================================================================
 *  SOUND SYSTEM
 *
 *  Generates PCM WAV data in memory and plays via PlaySound.
 *  No external WAV files required.
 * ================================================================ */

#define PI2  6.28318530f

/*
 * buildWav - generate a multi-segment WAV with frequency sweeps.
 *
 *   freqs   : array of frequencies (one per segment)
 *   durs    : array of durations in seconds (one per segment)
 *   count   : number of segments
 *   vol     : volume 0.0 - 1.0
 *   harmonics : 0 = pure sine,  1 = sine + two harmonics (richer tone)
 *
 * Within each segment the frequency linearly sweeps from freqs[i]
 * toward freqs[i+1] (portamento). The last segment holds its pitch.
 * Each segment has a short attack and release envelope.
 */
static void buildWav(int id,
                     float *freqs,float *durs,int count,
                     float vol,int harmonics){
    /* Count total samples */
    int totalSamples=0;
    for(int i=0;i<count;i++)
        totalSamples+=(int)(durs[i]*SFX_RATE);
    int dataBytes=totalSamples*2;

    /* Write WAV header (44 bytes) byte-by-byte to avoid padding issues */
    unsigned char *p=g_sfxBuf[id];
    int riffSz=36+dataBytes;
    memcpy(p,   "RIFF",4); memcpy(p+4, &riffSz,4);
    memcpy(p+8, "WAVE",4);
    memcpy(p+12,"fmt ",4);
    int fmtSz=16;           memcpy(p+16,&fmtSz,4);
    short one=1;            memcpy(p+20,&one,2);   /* PCM         */
                            memcpy(p+22,&one,2);   /* mono        */
    int   sr=SFX_RATE;      memcpy(p+24,&sr,4);
    int   br=SFX_RATE*2;    memcpy(p+28,&br,4);    /* byteRate    */
    short ba=2;             memcpy(p+32,&ba,2);    /* blockAlign  */
    short bps=16;           memcpy(p+34,&bps,2);
    memcpy(p+36,"data",4);  memcpy(p+40,&dataBytes,4);

    short *sam=(short*)(p+44);
    int si=0;
    double phase=0.0;

    for(int seg=0;seg<count;seg++){
        int n=(int)(durs[seg]*SFX_RATE);
        float f0=freqs[seg];
        float f1=(seg+1<count)?freqs[seg+1]:freqs[seg];

        for(int j=0;j<n;j++){
            float progress=(float)j/n;
            float freq=f0+(f1-f0)*progress;

            /* Envelope */
            float env;
            if(j<n*0.05f)     env=(float)j/(n*0.05f);
            else if(j>n*0.80f) env=1.f-(progress-0.80f)/0.20f;
            else               env=1.f;

            /* Phase accumulation for continuous waveform */
            phase+=freq/SFX_RATE;
            if(phase>1.0) phase-=1.0;
            float s=(float)sinf((float)(PI2*phase));

            if(harmonics){
                s=s*0.65f
                 +(float)sinf((float)(PI2*phase*2))*0.22f
                 +(float)sinf((float)(PI2*phase*3))*0.13f;
            }

            float sampleF=s*vol*env*32000.f;
            sam[si++]=(short)clampf(sampleF,-32000.f,32000.f);
        }
    }
    g_sfxSize[id]=44+dataBytes;
}

static void playSound(int id){
    if(id<0||id>=SFX_COUNT||g_sfxSize[id]==0) return;
    PlaySound((LPCSTR)g_sfxBuf[id],NULL,
              SND_MEMORY|SND_ASYNC|SND_NODEFAULT);
}

static void initSounds(void){
    /* --- Flap: quick rising chirp --- */
    { float f[]={280.f,560.f,320.f};
      float d[]={0.04f,0.04f,0.04f};
      buildWav(SFX_FLAP,f,d,3,0.35f,0); }

    /* --- Score: happy ascending C-E-G --- */
    { float f[]={523.f,659.f,784.f,784.f};
      float d[]={0.07f,0.07f,0.14f,0.01f};
      buildWav(SFX_SCORE,f,d,4,0.45f,0); }

    /* --- Die: sad descending melody --- */
    { float f[]={440.f,370.f,294.f,220.f,165.f};
      float d[]={0.09f,0.10f,0.11f,0.13f,0.22f};
      buildWav(SFX_DIE,f,d,5,0.50f,1); }

    /* --- Click: sharp button press --- */
    { float f[]={700.f,350.f};
      float d[]={0.025f,0.035f};
      buildWav(SFX_CLICK,f,d,2,0.30f,0); }

    /* --- Hover: soft tick --- */
    { float f[]={900.f,700.f};
      float d[]={0.018f,0.018f};
      buildWav(SFX_HOVER,f,d,2,0.12f,0); }

    /* --- Weather change: rising arpeggio --- */
    { float f[]={262.f,330.f,392.f,523.f};
      float d[]={0.07f,0.07f,0.07f,0.18f};
      buildWav(SFX_WEATHER,f,d,4,0.42f,0); }
}

/* ================================================================
 *  INIT: SUB-SYSTEMS
 * ================================================================ */

static void initClouds(void){
    for(int i=0;i<CLOUD_COUNT;i++){
        g_clouds[i].layer=i%2;
        g_clouds[i].x=randf(0.f,WORLD_W);
        g_clouds[i].y=randf(GROUND_Y+GROUND_H+70.f,WORLD_H-50.f);
        g_clouds[i].scale=randf(0.9f,1.7f);
        g_clouds[i].speed=(g_clouds[i].layer==0)?CLOUD_SPEED_FAR:CLOUD_SPEED_NEAR;
    }
}

static void initBuildings(void){
    float pal[4][3]={{0.25f,0.35f,0.50f},{0.20f,0.30f,0.45f},
                     {0.30f,0.40f,0.55f},{0.18f,0.28f,0.42f}};
    float x=0.f;
    for(int i=0;i<BUILDING_COUNT;i++){
        float w=randf(40.f,80.f); float h=randf(40.f,140.f);
        int p=rand()%4;
        g_buildings[i].x=x; g_buildings[i].w=w+4.f; g_buildings[i].h=h;
        g_buildings[i].r=pal[p][0]; g_buildings[i].g=pal[p][1]; g_buildings[i].b=pal[p][2];
        x+=w-randf(0.f,20.f);
    }
}

static void initRain(void){
    for(int i=0;i<RAIN_COUNT;i++){
        g_rain[i].x=randf(0.f,WORLD_W);
        g_rain[i].y=randf(0.f,WORLD_H);
        g_rain[i].speed=randf(9.f,16.f);
        g_rain[i].alpha=randf(0.35f,0.75f);
        g_rain[i].len=randf(10.f,20.f);
    }
}

static void initStars(void){
    for(int i=0;i<STAR_COUNT;i++){
        g_stars[i].x=randf(0.f,WORLD_W);
        g_stars[i].y=randf(GROUND_Y+GROUND_H+30.f,WORLD_H-8.f);
        g_stars[i].size=randf(0.8f,2.8f);
        g_stars[i].phase=randf(0.f,6.28f);
    }
}

static void initBird(void){
    g_bird.y=WORLD_H/2.f; g_bird.vy=0.f; g_bird.angle=0.f;
    g_bird.wingFrame=0; g_bird.wingTimer=0; g_bird.alive=1;
}

static void initPipes(void){
    float minC=GROUND_Y+GROUND_H*GRASS_H_RATIO+PIPE_MIN_H+PIPE_GAP/2.f;
    float maxC=WORLD_H-PIPE_MIN_H-PIPE_GAP/2.f;
    for(int i=0;i<PIPE_COUNT;i++){
        g_pipes[i].x=900.f+i*PIPE_SPACING;
        g_pipes[i].gapCenterY=randf(minC,maxC);
        g_pipes[i].scored=0;
    }
}

/* ================================================================
 *  GAME LIFECYCLE
 * ================================================================ */

static void resetGame(void){
    g_score=0; g_pipeSpeed=PIPE_BASE_SPEED;
    g_shakeTicks=0; g_flashTicks=0; g_groundScroll=0.f;
    g_gameOverDelay=0;
    initBird(); initPipes();
    g_state=STATE_PLAYING;
}

static void init(void){
    srand((unsigned int)time(NULL));
    glClearColor(0.05f,0.10f,0.25f,1.f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(2.f);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluOrtho2D(0.0,WORLD_W,0.0,WORLD_H);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();

    initClouds(); initBuildings(); initBird(); initPipes();
    initRain(); initStars();

    for(int i=0;i<8;i++) g_boltSegs[i]=randf(-25.f,25.f);

    initSounds();

    g_state=STATE_TITLE;
    g_weather=WEATHER_DAY;
    g_weatherTimer=0;
    g_hoveredWeather=-1;
    g_hoveredPlayAgain=0;
}

/* ================================================================
 *  SCREEN SHAKE
 * ================================================================ */

static void triggerShake(int t){ g_shakeTicks=t; }

static void updateShake(void){
    if(g_shakeTicks>0){
        float m=6.f*((float)g_shakeTicks/15.f);
        g_shakeX=randf(-m,m); g_shakeY=randf(-m,m);
        g_shakeTicks--;
    } else { g_shakeX=g_shakeY=0.f; }
}

/* ================================================================
 *  WEATHER SYSTEM
 * ================================================================ */

static void nextWeather(void){
    g_weather=(WeatherMode)((g_weather+1)%WEATHER_COUNT);
    g_weatherTimer=0;
    g_weatherNameTimer=150;
    g_wFlashTicks=12;
    initRain();
    playSound(SFX_WEATHER);
}

static void setWeather(WeatherMode w){
    g_weather=w;
    g_weatherTimer=0;
    g_weatherNameTimer=120;
    g_wFlashTicks=10;
    initRain();
    playSound(SFX_WEATHER);
}

static void updateWeather(void){
    g_weatherTimer++;
    if(g_weatherTimer>=WEATHER_CYCLE_FRAMES) nextWeather();
    if(g_weatherNameTimer>0) g_weatherNameTimer--;
    if(g_wFlashTicks>0) g_wFlashTicks--;

    /* Lightning in rain mode */
    if(g_weather==WEATHER_RAIN){
        if(g_lightning>0) g_lightning--;
        else if(rand()%240==0){
            g_lightning=10;
            g_boltX=randf(80.f,WORLD_W-80.f);
            for(int i=0;i<8;i++) g_boltSegs[i]=randf(-25.f,25.f);
        }
    } else { g_lightning=0; }
}

/* ================================================================
 *  DRAW: SKY BACKGROUND
 * ================================================================ */

static void drawBackground(void){
    const WeatherTheme *t=&g_themes[g_weather];
    glBegin(GL_QUADS);
        glColor3f(t->botR,t->botG,t->botB);
        glVertex2f(0,GROUND_Y+GROUND_H);
        glVertex2f(WORLD_W,GROUND_Y+GROUND_H);
        glColor3f(t->topR,t->topG,t->topB);
        glVertex2f(WORLD_W,WORLD_H);
        glVertex2f(0,WORLD_H);
    glEnd();
}

/* ================================================================
 *  DRAW: SUN  (Day + Sunny)
 * ================================================================ */

static void drawSun(void){
    float cx=WORLD_W-90.f, cy=WORLD_H-80.f, r=32.f;

    /* Outer glow rings */
    col4(255,235,100,30); fillCircle(cx,cy,r*2.3f,20);
    col4(255,240,120,50); fillCircle(cx,cy,r*1.6f,20);

    /* Rays */
    int rayN=14;
    for(int i=0;i<rayN;i++){
        float a=PI2*i/rayN+g_frame*0.008f;
        float pulse=1.f+0.18f*sinf(g_frame*0.05f+i*0.8f);
        float r1=r+6.f, r2=r+22.f*pulse;
        glLineWidth(2.8f);
        col4(255,215,55,155);
        glBegin(GL_LINES);
            glVertex2f(cx+cosf(a)*r1,cy+sinf(a)*r1);
            glVertex2f(cx+cosf(a)*r2,cy+sinf(a)*r2);
        glEnd();
        glLineWidth(2.f);
    }

    /* Body */
    col(255,238,75); fillCircle(cx,cy,r,22);
    /* Highlight */
    col(255,255,200); fillCircle(cx-r*0.25f,cy+r*0.25f,r*0.42f,14);
}

/* ================================================================
 *  DRAW: MOON  (Night)
 * ================================================================ */

static void drawMoon(void){
    float cx=WORLD_W-95.f, cy=WORLD_H-75.f, r=26.f;
    /* Glow */
    col4(200,210,255,18); fillCircle(cx,cy,r*2.8f,20);
    col4(210,220,255,30); fillCircle(cx,cy,r*1.8f,20);
    /* Moon */
    col(250,248,210); fillCircle(cx,cy,r,22);
    /* Craters */
    col(225,218,185);
    fillCircle(cx+r*0.30f,cy+r*0.22f,r*0.22f,12);
    fillCircle(cx-r*0.25f,cy-r*0.28f,r*0.14f,10);
    fillCircle(cx+r*0.05f,cy-r*0.10f,r*0.10f,10);
    /* Crescent cutout using sky colour */
    const WeatherTheme *th=&g_themes[WEATHER_NIGHT];
    glColor3f(th->topR,th->topG,th->topB);
    fillCircle(cx+r*0.40f,cy,r*0.82f,18);
}

/* ================================================================
 *  DRAW: STARS  (Night)
 * ================================================================ */

static void drawStars(void){
    for(int i=0;i<STAR_COUNT;i++){
        float tw=0.55f+0.45f*sinf(g_frame*0.04f+g_stars[i].phase);
        col4(255,255,225,(int)(tw*245));
        float sz=g_stars[i].size;
        if(sz>2.0f){
            glBegin(GL_LINES);
                glVertex2f(g_stars[i].x-sz*1.8f,g_stars[i].y);
                glVertex2f(g_stars[i].x+sz*1.8f,g_stars[i].y);
                glVertex2f(g_stars[i].x,g_stars[i].y-sz*1.8f);
                glVertex2f(g_stars[i].x,g_stars[i].y+sz*1.8f);
            glEnd();
        }
        fillCircle(g_stars[i].x,g_stars[i].y,sz,6);
    }
}

/* ================================================================
 *  DRAW: RAIN
 * ================================================================ */

static void drawRain(void){
    glLineWidth(1.5f);
    for(int i=0;i<RAIN_COUNT;i++){
        col4(180,205,230,(int)(g_rain[i].alpha*210));
        float x=g_rain[i].x, y=g_rain[i].y, len=g_rain[i].len;
        glBegin(GL_LINES);
            glVertex2f(x,y);
            glVertex2f(x-len*0.28f,y-len);
        glEnd();
    }
    glLineWidth(2.f);
}

/* ================================================================
 *  DRAW: LIGHTNING
 * ================================================================ */

static void drawLightning(void){
    if(g_lightning<=0) return;
    float a=(float)g_lightning/10.f;

    col4(210,225,255,(int)(a*65)); fillRect(0,0,WORLD_W,WORLD_H);

    float bx=g_boltX, by=WORLD_H;
    float segH=(WORLD_H-GROUND_Y-GROUND_H)*0.13f;

    /* ALGORITHM 3 — Bresenham Line: outer glow pass */
    col4(180,210,255,(int)(a*80));
    glPointSize(6.5f);
    for(int i=0;i<8;i++){
        float nx=bx+g_boltSegs[i], ny=by-segH;
        bresenhamLine((int)bx,(int)by,(int)nx,(int)ny);
        bx=nx; by=ny;
    }

    /* ALGORITHM 3 — Bresenham Line: inner bright core pass */
    bx=g_boltX; by=WORLD_H;
    col4(240,248,255,(int)(a*240));
    glPointSize(2.5f);
    for(int i=0;i<8;i++){
        float nx=bx+g_boltSegs[i], ny=by-segH;
        bresenhamLine((int)bx,(int)by,(int)nx,(int)ny);
        bx=nx; by=ny;
    }
    glPointSize(1.0f);
}

/* ================================================================
 *  DRAW: FOG  (Rain)
 * ================================================================ */

static void drawFog(void){
    for(int i=0;i<3;i++){
        float fy=GROUND_Y+GROUND_H+(i*60.f);
        col4(160,170,185,26-i*4);
        fillRect(0,fy,WORLD_W,80.f);
    }
    col4(140,155,170,16); fillRect(0,0,WORLD_W,WORLD_H);
}

/* ================================================================
 *  DRAW: BEAUTIFUL CLOUDS  (8-blob per cloud)
 * ================================================================ */

static void drawCloud(float cx,float cy,float sc){
    static const float body[7][4]={
        {  0.f,  0.f, 34.f, 22.f },
        { 30.f,  4.f, 26.f, 18.f },
        {-30.f,  2.f, 24.f, 16.f },
        { 15.f, 16.f, 22.f, 16.f },
        {-15.f, 14.f, 20.f, 15.f },
        { 48.f, -2.f, 16.f, 12.f },
        {-46.f, -1.f, 15.f, 11.f },
    };
    const WeatherTheme *t=&g_themes[g_weather];

    /* Shadow */
    colF4(t->cldR*0.70f,t->cldG*0.70f,t->cldB*0.76f,t->cldA*0.50f);
    fillEllipse(cx,cy-6.f*sc,36.f*sc,8.f*sc,14);

    /* Body blobs */
    colF4(t->cldR,t->cldG,t->cldB,t->cldA);
    for(int i=0;i<7;i++)
        fillEllipse(cx+body[i][0]*sc,cy+body[i][1]*sc,
                    body[i][2]*sc,body[i][3]*sc,16);

    /* Depth tint */
    colF4(t->cldR*0.88f,t->cldG*0.88f,t->cldB*0.90f,t->cldA*0.48f);
    fillEllipse(cx,cy,30.f*sc,18.f*sc,14);

    /* Top-left highlight */
    colF4(1.f,1.f,1.f,t->cldA*0.70f);
    fillEllipse(cx-8.f*sc,cy+12.f*sc,16.f*sc,9.f*sc,12);

    /* Inner glow */
    colF4(1.f,1.f,1.f,t->cldA*0.24f);
    fillEllipse(cx,cy+4.f*sc,22.f*sc,14.f*sc,12);
}

static void drawClouds(void){
    for(int i=0;i<CLOUD_COUNT;i++)
        drawCloud(g_clouds[i].x,g_clouds[i].y,g_clouds[i].scale);
}

/* ================================================================
 *  DRAW: CITY SILHOUETTE
 * ================================================================ */

static void drawCitySilhouette(void){
    float gs=GROUND_Y+GROUND_H*GRASS_H_RATIO;
    float dark=g_themes[g_weather].darkness;
    for(int i=0;i<BUILDING_COUNT;i++){
        float bx=g_buildings[i].x, bw=g_buildings[i].w;
        float bh=g_buildings[i].h, by=gs-2.f;
        glColor3f(g_buildings[i].r*(1.f-dark),
                  g_buildings[i].g*(1.f-dark),
                  g_buildings[i].b*(1.f-dark*0.7f));
        fillRect(bx,by,bw,bh);

        float wr,wg,wb,wa;
        if(g_weather==WEATHER_NIGHT){wr=1.0f;wg=0.85f;wb=0.45f;wa=0.90f;}
        else                        {wr=0.84f;wg=0.93f;wb=1.0f;wa=0.70f;}
        float wSz=5.f,wGX=14.f,wGY=18.f;
        for(float wy=by+10.f;wy+wSz<by+bh-10.f;wy+=wGY)
            for(float wx=bx+8.f;wx+wSz<bx+bw-8.f;wx+=wGX)
                if((int)(wx*7+wy*13)%3!=0){
                    if(g_weather==WEATHER_NIGHT){
                        col4(255,200,80,32);
                        fillRect(wx-2.f,wy-2.f,wSz+4.f,wSz+4.f);
                    }
                    colF4(wr,wg,wb,wa);
                    fillRect(wx,wy,wSz,wSz);
                }
    }
}

/* ================================================================
 *  DRAW: GROUND
 * ================================================================ */

static void drawGround(void){
    float grassH=GROUND_H*GRASS_H_RATIO;
    const WeatherTheme *t=&g_themes[g_weather];

    if(g_weather==WEATHER_NIGHT) col(100,80,55);
    else if(g_weather==WEATHER_RAIN) col(150,120,90);
    else col(222,184,135);
    fillRect(0,0,WORLD_W,GROUND_Y);

    colF(t->grassR,t->grassG,t->grassB);
    fillRect(0,GROUND_Y,WORLD_W,grassH);

    colF(t->grassR*0.82f,t->grassG*0.82f,t->grassB*0.82f);
    float tileW=40.f;
    float scroll=fmodf(g_groundScroll,tileW*2.f);
    for(float tx=-tileW*2.f+scroll;tx<WORLD_W+tileW;tx+=tileW*2.f)
        fillRect(tx,GROUND_Y+4.f,tileW-4.f,8.f);

    colF(t->grassR*0.65f,t->grassG*0.65f,t->grassB*0.65f);
    fillRect(0,GROUND_Y+grassH-2.f,WORLD_W,3.f);

    /* Rain puddles */
    if(g_weather==WEATHER_RAIN){
        col4(120,145,175,90);
        for(int i=0;i<8;i++){
            float px=fmodf(i*137.f+22.f,WORLD_W-40.f);
            float pw=20.f+fmodf(i*53.f,30.f);
            fillEllipse(px+pw/2.f,GROUND_Y-4.f,pw/2.f,5.f,12);
        }
    }

    /* Pebbles */
    col4(170,140,100,175);
    for(int i=0;i<20;i++){
        float px=fmodf(i*123.7f+37.f,WORLD_W);
        float py=12.f+fmodf(i*71.3f,GROUND_Y-20.f);
        fillRect(px,py,5.f,3.f);
    }
}

/* ================================================================
 *  DRAW: BIRD
 * ================================================================ */

static void drawBird(void){
    glPushMatrix();
    glTranslatef(BIRD_X,g_bird.y,0.f);
    glRotatef(g_bird.angle,0.f,0.f,1.f);

    /* Body — filled with GL_TRIANGLE_FAN (polygon primitive) */
    col(255,195,50); fillCircle(0,0,BIRD_RADIUS,20);
    /* ALGORITHM 2 — Midpoint Circle: plots the crisp body outline */
    col(200,120,20);
    glPointSize(2.5f);
    midpointCircle(0, 0, (int)BIRD_RADIUS);
    glPointSize(1.0f);

    /* Wing */
    float wOff[3]={0.f,8.f,-8.f};
    float wy=wOff[g_bird.wingFrame];
    col(230,155,20); fillEllipse(-6.f,wy,10.f,6.f,14);
    col(190,110,10);
    glPushMatrix();
        glTranslatef(-6.f,wy,0.f); glScalef(10.f,6.f,1.f);
        glBegin(GL_LINE_LOOP);
            for(int s=0;s<14;s++){float a=PI2*s/14;glVertex2f(cosf(a),sinf(a));}
        glEnd();
    glPopMatrix();

    /* Belly */
    col(255,230,150); fillEllipse(3.f,-3.f,10.f,8.f,14);

    /* Eye */
    col(255,255,255); fillCircle(6.f,5.f,6.5f,14);
    col(30,30,30);    fillCircle(7.5f,4.5f,3.5f,12);
    col(255,255,255); fillCircle(8.5f,5.5f,1.2f,8);

    /* Beak */
    col(255,120,30);
    glBegin(GL_TRIANGLES);
        glVertex2f(BIRD_RADIUS-2.f,3.5f);
        glVertex2f(BIRD_RADIUS+12.f,0.f);
        glVertex2f(BIRD_RADIUS-2.f,-3.5f);
    glEnd();
    col(200,80,10); glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
        glVertex2f(BIRD_RADIUS-2.f,3.5f);
        glVertex2f(BIRD_RADIUS+12.f,0.f);
        glVertex2f(BIRD_RADIUS-2.f,-3.5f);
    glEnd();
    glLineWidth(2.f);
    glPopMatrix();
}

/* ================================================================
 *  DRAW: BIRD REFLECTION  (2D Reflection Transformation)
 *
 *  Reflects the bird about the grass surface line (y = grassY).
 *  Formula:  y' = 2 * grassY - y
 *  Implemented via glScalef(1, -1, 1) after translating the
 *  coordinate origin to the reflection axis (grassY).
 *  Demonstrates the REFLECTION transformation from the syllabus.
 * ================================================================ */
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
        col4(255, 255, 255, 30); fillCircle(6.f,  5.f,  6.5f, 14);
        col4( 30,  30,  30, 30); fillCircle(7.5f, 4.5f, 3.5f, 12);
    glPopMatrix();
}

/* ================================================================
 *  DRAW: SCORE SPARKLE PARTICLES  (GL_POINTS primitive demo)
 *
 *  Each live particle is rendered as a single GL_POINT with an
 *  alpha value tied to its remaining life — clearly demonstrating
 *  the GL_POINTS primitive type as required by the rubric.
 * ================================================================ */
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

/* ================================================================
 *  DRAW: PIPES
 * ================================================================ */

static void drawSinglePipe(float x,float y1,float y2,int flipped){
    float dark=g_themes[g_weather].darkness;
    float dr=1.f-dark*0.4f, dg=1.f-dark*0.3f, db=1.f-dark*0.2f;
    float capOff=7.f, capW=PIPE_W+14.f;

    colF(80/255.f*dr,200/255.f*dg,80/255.f*db);
    fillRect(x,y1,PIPE_W,y2-y1);
    colF(120/255.f*dr,230/255.f*dg,100/255.f*db);
    fillRect(x+8.f,y1,12.f,y2-y1);
    colF(40/255.f*dr,140/255.f*dg,40/255.f*db);
    fillRect(x+PIPE_W-10.f,y1,10.f,y2-y1);
    /* ALGORITHM 1 — DDA Line: draws the 4-edge outline of the pipe body */
    colF(30/255.f*dr,110/255.f*dg,30/255.f*db);
    glPointSize(2.0f);
    ddaOutlineRect(x, y1, PIPE_W, y2 - y1);
    glPointSize(1.0f);

    float capY=flipped?y1-4.f:y2-PIPE_CAP_H, capH=PIPE_CAP_H+4.f;
    colF(80/255.f*dr,200/255.f*dg,80/255.f*db);
    fillRect(x-capOff,capY,capW,capH);
    colF(120/255.f*dr,230/255.f*dg,100/255.f*db);
    fillRect(x-capOff+8.f,capY,14.f,capH);
    colF(40/255.f*dr,140/255.f*dg,40/255.f*db);
    fillRect(x-capOff+capW-12.f,capY,12.f,capH);
    /* ALGORITHM 1 — DDA Line: draws the 4-edge outline of the pipe cap */
    colF(30/255.f*dr,110/255.f*dg,30/255.f*db);
    glPointSize(2.0f);
    ddaOutlineRect(x - capOff, capY, capW, capH);
    glPointSize(1.0f);
}

static void drawPipes(void){
    float gs=GROUND_Y+GROUND_H*GRASS_H_RATIO;
    for(int i=0;i<PIPE_COUNT;i++){
        float gT=g_pipes[i].gapCenterY+PIPE_GAP/2.f;
        float gB=g_pipes[i].gapCenterY-PIPE_GAP/2.f;
        drawSinglePipe(g_pipes[i].x,gs,gB,0);
        drawSinglePipe(g_pipes[i].x,gT,WORLD_H+10.f,1);
    }
}

/* ================================================================
 *  DRAW: HUD
 * ================================================================ */

static void drawHUD(void){
    char buf[32];
    float cx=WORLD_W/2.f, cy=WORLD_H-55.f;
    sprintf(buf,"%d",g_score);
    float sw=strokeWidth(buf,0.20f);
    col(40,40,40); strokeText(cx-sw/2.f+2.f,cy-2.f,0.20f,buf);
    col(255,255,255); strokeText(cx-sw/2.f,cy,0.20f,buf);
    sprintf(buf,"BEST: %d",g_highScore);
    col(50,50,50); bitmapText(10.f,WORLD_H-22.f,GLUT_BITMAP_HELVETICA_18,buf);
    col(200,200,200);
    bitmapText(WORLD_W-115.f,WORLD_H-22.f,GLUT_BITMAP_HELVETICA_12,"[W] Weather");
}

/* ================================================================
 *  DRAW: WEATHER SELECTOR (title screen interactive buttons)
 *
 *  Four buttons side by side, each showing:
 *   - Background colour matching the weather theme
 *   - A small weather icon
 *   - Weather name
 *   - Selected state: bright border + glow
 *   - Hovered state: lighter background
 * ================================================================ */

/* Draw a small icon for each weather mode inside a button */
static void drawWeatherIcon(WeatherMode w,float cx,float cy){
    switch(w){
        case WEATHER_DAY:{
            /* Simple sun: circle + short rays */
            col(255,230,60); fillCircle(cx,cy,10.f,16);
            col4(255,220,40,200);
            for(int i=0;i<8;i++){
                float a=PI2*i/8+g_frame*0.01f;
                glLineWidth(2.f);
                glBegin(GL_LINES);
                    glVertex2f(cx+cosf(a)*12.f,cy+sinf(a)*12.f);
                    glVertex2f(cx+cosf(a)*17.f,cy+sinf(a)*17.f);
                glEnd();
            }
        } break;
        case WEATHER_SUNNY:{
            /* Larger warm sun */
            col(255,200,30); fillCircle(cx,cy,13.f,16);
            col(255,255,180); fillCircle(cx-4.f,cy+4.f,5.f,10);
            col4(255,180,20,180);
            for(int i=0;i<10;i++){
                float a=PI2*i/10+g_frame*0.012f;
                glLineWidth(2.5f);
                glBegin(GL_LINES);
                    glVertex2f(cx+cosf(a)*15.f,cy+sinf(a)*15.f);
                    glVertex2f(cx+cosf(a)*21.f,cy+sinf(a)*21.f);
                glEnd();
            }
            glLineWidth(2.f);
        } break;
        case WEATHER_RAIN:{
            /* Small cloud + three rain drops */
            col4(180,195,210,230);
            fillEllipse(cx,cy+6.f,18.f,10.f,12);
            fillEllipse(cx-10.f,cy+2.f,10.f,7.f,10);
            fillEllipse(cx+10.f,cy+3.f,10.f,7.f,10);
            /* Rain drops */
            col4(100,150,210,220);
            glLineWidth(2.f);
            for(int i=0;i<3;i++){
                float rx=cx-10.f+i*10.f;
                glBegin(GL_LINES);
                    glVertex2f(rx,cy-5.f);
                    glVertex2f(rx-3.f,cy-14.f);
                glEnd();
            }
        } break;
        case WEATHER_NIGHT:{
            /* Stars + crescent moon */
            col(250,245,200); fillCircle(cx+4.f,cy+2.f,11.f,16);
            /* Cutout circle for crescent */
            col4(0.12f*255,0.15f*255,0.32f*255,255);
            fillCircle(cx+9.f,cy+2.f,8.5f,14);
            /* Stars */
            col4(255,255,200,210);
            fillCircle(cx-10.f,cy+8.f,2.f,6);
            fillCircle(cx-14.f,cy,1.5f,6);
            fillCircle(cx-8.f,cy-8.f,1.8f,6);
        } break;
    }
    glLineWidth(2.f);
}

static void drawWeatherSelector(void){
    /* Label above buttons */
    col(255,255,255);
    const char *label="Choose your weather:";
    float lw=strokeWidth(label,0.075f);
    strokeText(WORLD_W/2.f-lw/2.f, WBTN_Y+WBTN_H+14.f, 0.075f, label);

    for(int i=0;i<WEATHER_COUNT;i++){
        float bx=WBTN_STARTX+i*(WBTN_W+WBTN_GAP);
        float by=WBTN_Y;
        int selected=(g_weather==(WeatherMode)i);
        int hovered=(g_hoveredWeather==i);
        const WeatherTheme *t=&g_themes[i];

        /* Button background */
        float alpha=selected?0.95f:hovered?0.80f:0.65f;
        colF4(t->btnR,t->btnG,t->btnB,alpha);
        fillRoundRect(bx,by,WBTN_W,WBTN_H,8.f);

        /* Selected glow */
        if(selected){
            float pulse=0.7f+0.3f*sinf(g_frame*0.10f);
            col4(255,240,100,(int)(pulse*160));
            outlineRect(bx-3.f,by-3.f,WBTN_W+6.f,WBTN_H+6.f,3.f);
            col4(255,255,200,(int)(pulse*80));
            outlineRect(bx-5.f,by-5.f,WBTN_W+10.f,WBTN_H+10.f,2.f);
        } else if(hovered){
            col4(255,255,255,70);
            fillRoundRect(bx,by,WBTN_W,WBTN_H,8.f);
        }

        /* Border */
        if(selected) col(255,240,100);
        else         col(200,210,220);
        outlineRect(bx,by,WBTN_W,WBTN_H,2.f);

        /* Weather icon (centred, upper half of button) */
        drawWeatherIcon((WeatherMode)i, bx+WBTN_W/2.f, by+WBTN_H*0.60f);

        /* Weather name text (lower portion) */
        if(selected) col(255,240,100);
        else         col(240,245,255);
        const char *nm=g_themes[i].name;
        float nw=strokeWidth(nm,0.060f);
        strokeText(bx+WBTN_W/2.f-nw/2.f, by+8.f, 0.060f, nm);
    }

    /* Description of selected weather */
    const char *desc=g_themes[g_weather].desc;
    col4(220,230,255,200);
    float dw=strokeWidth(desc,0.055f);
    strokeText(WORLD_W/2.f-dw/2.f, WBTN_Y-22.f, 0.055f, desc);
}

/* ================================================================
 *  DRAW: TITLE SCREEN
 * ================================================================ */

static void drawTitleScreen(void){
    float tY=WORLD_H*0.74f+g_titleBobY;   /* Title anchor, moved higher */
    float sc=0.30f;
    float w1=strokeWidth("FLAPPY",sc), w2=strokeWidth("BIRD",sc);

    /* Title shadow */
    col(50,80,20);
    strokeText(WORLD_W/2.f-w1/2.f+3.f,tY-3.f,sc,"FLAPPY");
    strokeText(WORLD_W/2.f-w2/2.f+3.f,tY-55.f-3.f,sc,"BIRD");
    /* Title text */
    col(255,220,40);
    strokeText(WORLD_W/2.f-w1/2.f,tY,sc,"FLAPPY");
    col(120,230,60);
    strokeText(WORLD_W/2.f-w2/2.f,tY-55.f,sc,"BIRD");
    /* Border box */
    float bx=WORLD_W/2.f-160.f, by=tY-70.f;
    col(255,255,255); outlineRect(bx,by,320.f,110.f,4.f);
    col(50,120,20);   outlineRect(bx+4.f,by+4.f,312.f,102.f,3.f);

    /* Showcase bird near title */
    /* SCALING TRANSFORM — title bird pulses in and out (uniform scale) */
    float sy=g_bird.y, sa=g_bird.angle;
    g_bird.y=tY-22.f; g_bird.angle=5.f*sinf(g_frame*0.05f);
    float titleScale = 1.0f + 0.12f * sinf(g_frame * 0.07f);
    glPushMatrix();
        glTranslatef(BIRD_X, tY - 22.f, 0.f);          /* move to bird centre  */
        glScalef(titleScale, titleScale, 1.0f);          /* scale about centre   */
        glTranslatef(-BIRD_X, -(tY - 22.f), 0.f);       /* restore origin       */
        drawBird();
    glPopMatrix();
    g_bird.y=sy; g_bird.angle=sa;

    /* Weather selector */
    drawWeatherSelector();

    /* Start prompt */
    float pulse=0.65f+0.35f*sinf(g_frame*0.09f);
    col4(255,255,255,(int)(pulse*255));
    const char *prompt="Press SPACE or Click to Start";
    float pw=strokeWidth(prompt,0.082f);
    strokeText(WORLD_W/2.f-pw/2.f, WBTN_Y-48.f, 0.082f, prompt);

    /* Controls hint */
    col4(200,210,225,180);
    bitmapText(WORLD_W/2.f-100.f, GROUND_Y+GROUND_H+8.f,
               GLUT_BITMAP_HELVETICA_12,
               "P = Pause    W = Weather    F11 = Fullscreen    ESC = Quit");
}

/* ================================================================
 *  DRAW: PLAY AGAIN BUTTON
 *  Shown after g_gameOverDelay reaches 0.
 *  Shows a progress indicator while waiting.
 * ================================================================ */

static void drawPlayAgainButton(float px,float py){
    /* px,py = bottom-left of game over panel */
    float btnW=PLAY_BTN_W, btnH=PLAY_BTN_H;
    float btnX=WORLD_W/2.f-btnW/2.f;
    float btnY=py+12.f;

    if(g_gameOverDelay>0){
        /* Waiting: show a small loading arc */
        float progress=1.f-(float)g_gameOverDelay/GAMEOVER_DELAY;
        float arcAngle=progress*360.f;
        col4(150,150,150,120);
        /* Draw circular progress as a series of arcs (approximated) */
        float r=14.f, ccx=WORLD_W/2.f, ccy=btnY+btnH/2.f;
        fillCircle(ccx,ccy,r+4.f,20);
        col4(60,60,60,180); fillCircle(ccx,ccy,r,20);
        col4(100,200,100,200);
        int arcSegs=(int)(arcAngle/360.f*20.f);
        glBegin(GL_TRIANGLE_FAN);
            glVertex2f(ccx,ccy);
            for(int i=0;i<=arcSegs;i++){
                float a=-PI2/4.f+PI2*i/20;
                glVertex2f(ccx+cosf(a)*r,ccy+sinf(a)*r);
            }
        glEnd();
    } else {
        /* Button ready */
        int hov=g_hoveredPlayAgain;

        /* Shadow */
        col4(0,0,0,60);
        fillRoundRect(btnX+3.f,btnY-3.f,btnW,btnH,10.f);

        /* Button body */
        if(hov){
            col(80,210,90);
        } else {
            col(55,185,65);
        }
        fillRoundRect(btnX,btnY,btnW,btnH,10.f);

        /* Hover shimmer */
        if(hov){
            col4(255,255,255,50);
            fillRoundRect(btnX,btnY+btnH/2.f,btnW,btnH/2.f,10.f);
        }

        /* Border */
        col(30,120,35); outlineRect(btnX,btnY,btnW,btnH,2.5f);
        /* Top highlight */
        col4(255,255,255,hov?100:70);
        fillRect(btnX+12.f,btnY+btnH-8.f,btnW-24.f,4.f);

        /* Text */
        const char *txt="PLAY AGAIN";
        float tw=strokeWidth(txt,0.085f);
        /* Shadow */
        col4(0,80,0,150);
        strokeText(WORLD_W/2.f-tw/2.f+1.5f,btnY+btnH/2.f-5.f,0.085f,txt);
        /* Text */
        col(240,255,240);
        strokeText(WORLD_W/2.f-tw/2.f,btnY+btnH/2.f-4.f,0.085f,txt);

        /* Keyboard hint below button */
        col4(120,120,120,180);
        bitmapText(WORLD_W/2.f-50.f,btnY-16.f,
                   GLUT_BITMAP_HELVETICA_12,"or press SPACE / R");
    }
}

/* ================================================================
 *  DRAW: GAME OVER SCREEN
 * ================================================================ */

static void drawGameOverScreen(void){
    /* Dark overlay */
    col4(0,0,0,110); fillRect(0,0,WORLD_W,WORLD_H);

    /* Panel */
    float px=WORLD_W/2.f-180.f, py=WORLD_H/2.f-80.f;
    col4(250,242,205,245); fillRect(px,py,360.f,180.f);
    col(90,60,20);   outlineRect(px,py,360.f,180.f,4.f);
    col(180,130,50); outlineRect(px+4.f,py+4.f,352.f,172.f,3.f);

    /* GAME OVER title */
    const char *go="GAME OVER"; float gsc=0.20f;
    float gw=strokeWidth(go,gsc);
    col(200,40,40);  strokeText(WORLD_W/2.f-gw/2.f+2.f,py+132.f-2.f,gsc,go);
    col(255,80,80);  strokeText(WORLD_W/2.f-gw/2.f,py+132.f,gsc,go);

    /* Scores */
    char buf[32];
    sprintf(buf,"Score: %d",g_score); col(60,40,10);
    bitmapText(WORLD_W/2.f-58.f,py+96.f,GLUT_BITMAP_HELVETICA_18,buf);
    sprintf(buf,"Best:  %d",g_highScore);
    bitmapText(WORLD_W/2.f-58.f,py+72.f,GLUT_BITMAP_HELVETICA_18,buf);

    /* Play Again button area */
    drawPlayAgainButton(px,py);
}

/* ================================================================
 *  DRAW: PAUSE
 * ================================================================ */

static void drawPauseScreen(void){
    col4(0,0,0,80); fillRect(0,0,WORLD_W,WORLD_H);
    const char *pt="PAUSED"; float psc=0.25f;
    col(255,255,255);
    strokeText(WORLD_W/2.f-strokeWidth(pt,psc)/2.f,WORLD_H/2.f-10.f,psc,pt);
    col(200,200,200);
    bitmapText(WORLD_W/2.f-75.f,WORLD_H/2.f-50.f,
               GLUT_BITMAP_HELVETICA_18,"Press P to Resume");
}

/* ================================================================
 *  DRAW: WEATHER NAME ANNOUNCEMENT
 * ================================================================ */

static void drawWeatherName(void){
    if(g_weatherNameTimer<=0) return;
    float alpha=clampf((float)g_weatherNameTimer/60.f,0.f,1.f);
    const char *name=g_themes[g_weather].name;
    float pw=220.f, ph=52.f;
    float ppx=WORLD_W/2.f-pw/2.f, ppy=WORLD_H*0.72f;
    col4(0,0,0,(int)(alpha*120)); fillRect(ppx,ppy,pw,ph);
    col4(255,255,255,(int)(alpha*55)); outlineRect(ppx,ppy,pw,ph,2.f);
    col4(255,255,255,(int)(alpha*255));
    float tw=strokeWidth(name,0.16f);
    strokeText(WORLD_W/2.f-tw/2.f,ppy+16.f,0.16f,name);
}

/* ================================================================
 *  DISPLAY CALLBACK
 * ================================================================ */

static void display(void){
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();
    glTranslatef(g_shakeX,g_shakeY,0.f);

    drawBackground();
    if(g_weather==WEATHER_SUNNY||g_weather==WEATHER_DAY) drawSun();
    if(g_weather==WEATHER_NIGHT){ drawStars(); drawMoon(); }
    drawClouds();
    drawCitySilhouette();

    switch(g_state){
        case STATE_TITLE:
            drawGround();
            drawTitleScreen();
            break;
        case STATE_PLAYING:
            /* drawBirdReflection() — Reflection Transform — must come AFTER
               drawGround() so it renders inside the ground strip, then
               drawBird() renders the real bird on top. */
            drawPipes(); drawGround(); drawBirdReflection(); drawBird(); drawHUD();
            break;
        case STATE_PAUSED:
            drawPipes(); drawGround(); drawBirdReflection(); drawBird(); drawHUD();
            drawPauseScreen();
            break;
        case STATE_GAMEOVER:
            drawPipes(); drawGround(); drawBirdReflection(); drawBird(); drawHUD();
            drawGameOverScreen();
            break;
    }

    if(g_weather==WEATHER_RAIN){ drawFog(); drawRain(); drawLightning(); }
    drawParticles();      /* GL_POINTS — score sparkle burst */
    drawWeatherName();

    /* Weather change flash */
    if(g_wFlashTicks>0){
        float a=(float)g_wFlashTicks/12.f;
        col4(255,255,255,(int)(a*170)); fillRect(0,0,WORLD_W,WORLD_H);
    }
    /* Death flash */
    if(g_flashTicks>0){
        float a=(float)g_flashTicks/10.f;
        col4(255,255,255,(int)(a*200)); fillRect(0,0,WORLD_W,WORLD_H);
    }

    glutSwapBuffers();
}

/* ================================================================
 *  COLLISION DETECTION
 * ================================================================ */

static int checkCollision(void){
    float bx=BIRD_X, by=g_bird.y, br=BIRD_RADIUS-4.f;
    float gs=GROUND_Y+GROUND_H*GRASS_H_RATIO;
    if(by-br<=gs)     return 1;
    if(by+br>=WORLD_H)return 1;
    for(int i=0;i<PIPE_COUNT;i++){
        float px=g_pipes[i].x;
        float gT=g_pipes[i].gapCenterY+PIPE_GAP/2.f;
        float gB=g_pipes[i].gapCenterY-PIPE_GAP/2.f;
        if(bx+br>px-7.f && bx-br<px+PIPE_W+7.f){
            if(by-br<gB) return 1;
            if(by+br>gT) return 1;
        }
    }
    return 0;
}

/* ================================================================
 *  PARTICLE SYSTEM — trigger, update, (draw is in draw section)
 * ================================================================ */

/* Spawn a radial burst of 20 sparkle particles at world pos (x,y). */
static void triggerParticles(float x, float y) {
    int spawned = 0;
    for (int i = 0; i < PARTICLE_COUNT && spawned < 20; i++) {
        if (!g_particles[i].active) {
            float angle           = randf(0.f, PI2);
            float speed           = randf(2.5f, 9.f);
            g_particles[i].x     = x;
            g_particles[i].y     = y;
            g_particles[i].vx    = cosf(angle) * speed;
            g_particles[i].vy    = sinf(angle) * speed;
            g_particles[i].r     = randf(0.85f, 1.0f);
            g_particles[i].g     = randf(0.70f, 1.0f);
            g_particles[i].b     = randf(0.0f,  0.35f);
            g_particles[i].life  = 1.0f;
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

/* ================================================================
 *  UPDATE FUNCTIONS
 * ================================================================ */

static void updateBird(void){
    g_bird.vy+=GRAVITY;
    if(g_bird.vy<MAX_FALL_VEL) g_bird.vy=MAX_FALL_VEL;
    g_bird.y+=g_bird.vy;
    float tgt;
    if(g_bird.vy>2.f){
        tgt=TILT_UP_DEG;
    } else {
        float t=clampf((g_bird.vy-MAX_FALL_VEL)/(2.f-MAX_FALL_VEL),0.f,1.f);
        tgt=lerpf(TILT_DOWN_DEG,0.f,t);
    }
    g_bird.angle+=(tgt-g_bird.angle)*0.25f;
    if(++g_bird.wingTimer>=WING_ANIM_RATE){
        g_bird.wingTimer=0;
        g_bird.wingFrame=(g_bird.wingFrame+1)%WING_FRAMES;
    }
}

static void updatePipes(void){
    float minC=GROUND_Y+GROUND_H*GRASS_H_RATIO+PIPE_MIN_H+PIPE_GAP/2.f;
    float maxC=WORLD_H-PIPE_MIN_H-PIPE_GAP/2.f;
    for(int i=0;i<PIPE_COUNT;i++){
        g_pipes[i].x-=g_pipeSpeed;
        if(g_pipes[i].x+PIPE_W<-20.f){
            float mx=g_pipes[i].x;
            for(int j=0;j<PIPE_COUNT;j++)
                if(j!=i&&g_pipes[j].x>mx) mx=g_pipes[j].x;
            g_pipes[i].x=mx+PIPE_SPACING;
            g_pipes[i].gapCenterY=randf(minC,maxC);
            g_pipes[i].scored=0;
        }
        if(!g_pipes[i].scored&&g_pipes[i].x+PIPE_W/2.f<BIRD_X){
            g_pipes[i].scored=1;
            g_score++;
            if(g_score>g_highScore) g_highScore=g_score;
            playSound(SFX_SCORE);
            triggerParticles(BIRD_X, g_bird.y); /* score sparkle (GL_POINTS) */
        }
    }
    g_pipeSpeed+=PIPE_SPEED_INC;
}

static void updateClouds(void){
    for(int i=0;i<CLOUD_COUNT;i++){
        g_clouds[i].x-=g_clouds[i].speed;
        if(g_clouds[i].x+100.f*g_clouds[i].scale<0.f){
            g_clouds[i].x=WORLD_W+70.f;
            g_clouds[i].y=randf(GROUND_Y+GROUND_H+60.f,WORLD_H-60.f);
        }
    }
}

static void updateRainDrops(void){
    for(int i=0;i<RAIN_COUNT;i++){
        g_rain[i].y-=g_rain[i].speed;
        g_rain[i].x-=g_rain[i].speed*0.28f;
        if(g_rain[i].y<-20.f){
            g_rain[i].y=WORLD_H+10.f;
            g_rain[i].x=randf(-50.f,WORLD_W+50.f);
        }
        if(g_rain[i].x<-20.f){
            g_rain[i].x=WORLD_W+randf(0.f,80.f);
            g_rain[i].y=randf(0.f,WORLD_H);
        }
    }
}

/* ================================================================
 *  MASTER UPDATE
 * ================================================================ */

static void updateGame(void){
    g_frame++;
    updateWeather();

    if(g_state==STATE_TITLE){
        updateClouds();
        if(g_weather==WEATHER_RAIN) updateRainDrops();
        g_titleBobT+=0.05f;
        g_titleBobY=8.f*sinf(g_titleBobT);
        if(++g_bird.wingTimer>=WING_ANIM_RATE){
            g_bird.wingTimer=0;
            g_bird.wingFrame=(g_bird.wingFrame+1)%WING_FRAMES;
        }
        return;
    }
    if(g_state==STATE_PAUSED) return;

    if(g_state==STATE_GAMEOVER){
        if(g_flashTicks>0) g_flashTicks--;
        if(g_gameOverDelay>0) g_gameOverDelay--;
        if(g_weather==WEATHER_RAIN) updateRainDrops();
        updateClouds();
        updateShake();
        return;
    }

    /* Playing */
    updateBird();
    updatePipes();
    updateParticles();     /* advance score-sparkle particles */
    updateClouds();
    if(g_weather==WEATHER_RAIN) updateRainDrops();
    g_groundScroll+=g_pipeSpeed;

    if(checkCollision()){
        g_state=STATE_GAMEOVER;
        g_bird.alive=0;
        g_flashTicks=10;
        g_gameOverDelay=GAMEOVER_DELAY;
        triggerShake(15);
        playSound(SFX_DIE);
    }
    updateShake();
    if(g_flashTicks>0) g_flashTicks--;
}

/* ================================================================
 *  TIMER
 * ================================================================ */

static void timerCallback(int v){
    (void)v;
    updateGame();
    glutPostRedisplay();
    glutTimerFunc(FRAME_MS,timerCallback,0);
}

/* ================================================================
 *  FLAP
 * ================================================================ */

static void doFlap(void){
    if(g_state==STATE_TITLE){
        resetGame();
        return;
    }
    if(g_state==STATE_GAMEOVER){
        if(g_gameOverDelay<=0){
            resetGame();
        }
        return;
    }
    if(g_state==STATE_PLAYING){
        g_bird.vy=FLAP_VEL;
        g_bird.angle=TILT_UP_DEG;
        g_bird.wingFrame=1;
        playSound(SFX_FLAP);
    }
}

/* ================================================================
 *  HOVER CHECK HELPER
 *  Returns which weather button (0-3) the mouse is over, or -1.
 * ================================================================ */

static int hoveredWeatherButton(void){
    for(int i=0;i<WEATHER_COUNT;i++){
        float bx=WBTN_STARTX+i*(WBTN_W+WBTN_GAP);
        if(isInRect(g_mouseX,g_mouseY,bx,WBTN_Y,WBTN_W,WBTN_H))
            return i;
    }
    return -1;
}

static int hoveredPlayAgainButton(void){
    float btnW=PLAY_BTN_W, btnH=PLAY_BTN_H;
    float btnX=WORLD_W/2.f-btnW/2.f;
    float py=WORLD_H/2.f-80.f;
    float btnY=py+12.f;
    return isInRect(g_mouseX,g_mouseY,btnX,btnY,btnW,btnH);
}

/* ================================================================
 *  KEYBOARD INPUT
 * ================================================================ */

static void keyboardInput(unsigned char key,int x,int y){
    (void)x;(void)y;
    switch(key){
        case ' ':
            doFlap();
            break;
        case 'r': case 'R':
            if(g_state==STATE_GAMEOVER && g_gameOverDelay<=0) resetGame();
            else if(g_state==STATE_TITLE) resetGame();
            break;
        case 'p': case 'P':
            if(g_state==STATE_PLAYING)     g_state=STATE_PAUSED;
            else if(g_state==STATE_PAUSED) g_state=STATE_PLAYING;
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

static void specialKeys(int key,int x,int y){
    (void)x;(void)y;
    if(key==GLUT_KEY_F11){
        g_fullscreen=!g_fullscreen;
        if(g_fullscreen) glutFullScreen();
        else{ glutReshapeWindow(WIN_W,WIN_H); glutPositionWindow(100,80); }
    }
}

/* ================================================================
 *  MOUSE INPUT  (click)
 * ================================================================ */

static void mouseInput(int button,int state,int x,int y){
    if(button!=GLUT_LEFT_BUTTON||state!=GLUT_DOWN) return;
    screenToWorld(x,y,&g_mouseX,&g_mouseY);

    if(g_state==STATE_TITLE){
        /* Check weather buttons first */
        int hw=hoveredWeatherButton();
        if(hw>=0){
            setWeather((WeatherMode)hw);
            playSound(SFX_CLICK);
            return;
        }
        /* Anywhere else on title = start */
        doFlap();
        return;
    }

    if(g_state==STATE_GAMEOVER && g_gameOverDelay<=0){
        if(hoveredPlayAgainButton()){
            playSound(SFX_CLICK);
            resetGame();
            return;
        }
    }

    if(g_state==STATE_PLAYING){
        doFlap();
    }
}

/* ================================================================
 *  PASSIVE MOTION  (mouse move without button - hover detection)
 * ================================================================ */

static void passiveMotion(int x,int y){
    screenToWorld(x,y,&g_mouseX,&g_mouseY);

    /* Weather button hover */
    if(g_state==STATE_TITLE){
        int prev=g_hoveredWeather;
        g_hoveredWeather=hoveredWeatherButton();
        if(g_hoveredWeather!=prev && g_hoveredWeather>=0)
            playSound(SFX_HOVER);
    } else {
        g_hoveredWeather=-1;
    }

    /* Play Again button hover */
    if(g_state==STATE_GAMEOVER && g_gameOverDelay<=0){
        int prev=g_hoveredPlayAgain;
        g_hoveredPlayAgain=hoveredPlayAgainButton();
        if(g_hoveredPlayAgain && !prev)
            playSound(SFX_HOVER);
    } else {
        g_hoveredPlayAgain=0;
    }
}

/* ================================================================
 *  RESHAPE  (store viewport for mouse coordinate conversion)
 * ================================================================ */

static void reshape(int w,int h){
    if(h==0) h=1;
    g_winH=h;
    float wa=WORLD_W/WORLD_H, ww=(float)w/(float)h;
    int vpX,vpY,vpW,vpH;
    if(ww>wa){ vpH=h; vpW=(int)(h*wa); vpX=(w-vpW)/2; vpY=0; }
    else      { vpW=w; vpH=(int)(w/wa); vpX=0;  vpY=(h-vpH)/2; }
    g_vpX=vpX; g_vpY=vpY; g_vpW=vpW; g_vpH=vpH;
    glViewport(vpX,vpY,vpW,vpH);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluOrtho2D(0.0,WORLD_W,0.0,WORLD_H);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
}

/* ================================================================
 *  MAIN
 * ================================================================ */

int main(int argc,char **argv){
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB);
    glutInitWindowSize(WIN_W,WIN_H);
    glutInitWindowPosition(50,30);
    glutCreateWindow("Flappy Bird  -  OpenGL Edition");

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
    glutTimerFunc(FRAME_MS,timerCallback,0);

    /* Initialise all sub-systems including sounds */
    init();

    glutMainLoop();
    return 0;
}

/*
 * ================================================================
 *  NOTE ON SOUND
 *
 *  Sound is generated in memory as 16-bit mono PCM WAV data
 *  and played through the Windows Multimedia API (PlaySound).
 *  No external .wav files are needed.
 *
 *  To compile you MUST add -lwinmm to the linker flags:
 *    In Code::Blocks: Project -> Build Options -> Linker ->
 *    Other linker options: -lopengl32 -lglu32 -lfreeglut -lwinmm -lm
 *
 *  Or on command line:
 *    gcc flappy_bird.c -o flappy_bird.exe -lopengl32 -lglu32 -lfreeglut -lwinmm -lm -std=c99
 * ================================================================
 */
