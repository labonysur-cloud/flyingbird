/*
 * =========================================================
 *  FLAPPY BIRD CLONE  -  OpenGL / GLUT  -  C Language
 *  Enhanced Edition
 *
 *  Author  : Labony Sur
 *  Build   : Code::Blocks with MinGW + FreeGLUT
 *
 *  Features:
 *   - Full screen launch, press F11 to toggle windowed
 *   - Four dynamic weather modes: Day, Sunny, Rain, Night
 *   - Auto weather cycle every 30 seconds, press W to change manually
 *   - Rain mode: 220 particles, zigzag lightning bolts, fog overlay
 *   - Night mode: 110 twinkling stars, glowing crescent moon
 *   - Day/Sunny mode: animated sun with pulsing rays
 *   - Beautiful clouds made of 8 overlapping ellipses
 *   - Weather-aware colours for sky, pipes, ground and buildings
 *   - Easier physics: larger gap, slower difficulty ramp
 *
 *  Controls:
 *    SPACE or Left Click : Flap / Start / Restart
 *    W                   : Cycle weather mode
 *    P                   : Pause or unpause
 *    R                   : Restart from Game Over
 *    F11                 : Toggle full screen and windowed
 *    ESC                 : Quit
 *
 *  Compile:
 *    gcc flappy_bird.c -o flappy_bird.exe -lopengl32 -lglu32 -lfreeglut -lm -std=c99
 * =========================================================
 */

#include <GL/glut.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* =========================================================
 *  CONSTANTS
 * ========================================================= */

#define WIN_W            800
#define WIN_H            600
#define WORLD_W          800.0f
#define WORLD_H          600.0f

/* ---- Bird (easier tuning) ---- */
#define BIRD_X           160.0f
#define BIRD_RADIUS      18.0f
#define GRAVITY         -0.45f       /* Reduced from -0.55, easier feel  */
#define FLAP_VEL         10.0f
#define MAX_FALL_VEL    -13.0f
#define TILT_UP_DEG      25.0f
#define TILT_DOWN_DEG   -55.0f

/* ---- Pipes (easier tuning) ---- */
#define PIPE_W           70.0f
#define PIPE_GAP         190.0f      /* Increased from 155, larger gap = easier */
#define PIPE_SPACING     290.0f      /* More breathing room */
#define PIPE_COUNT       3
#define PIPE_MIN_H       80.0f
#define PIPE_CAP_H       20.0f
#define PIPE_BASE_SPEED  2.7f        /* Slightly slower start */
#define PIPE_SPEED_INC   0.002f      /* Much slower ramp-up */

/* ---- Ground ---- */
#define GROUND_Y         80.0f
#define GROUND_H         80.0f
#define GRASS_H_RATIO    0.30f

/* ---- Weather ---- */
#define WEATHER_COUNT        4
#define WEATHER_CYCLE_FRAMES 1800    /* Auto-change every 30 seconds */
#define WEATHER_FLASH_FRAMES 12

/* ---- Clouds ---- */
#define CLOUD_COUNT      7
#define CLOUD_SPEED_FAR  0.35f
#define CLOUD_SPEED_NEAR 0.80f

/* ---- City ---- */
#define BUILDING_COUNT   14

/* ---- Rain ---- */
#define RAIN_COUNT       220

/* ---- Stars (night) ---- */
#define STAR_COUNT       110

/* ---- Wing anim ---- */
#define WING_FRAMES      3
#define WING_ANIM_RATE   8

/* ---- FPS ---- */
#define TARGET_FPS       60
#define FRAME_MS         (1000 / TARGET_FPS)

/* =========================================================
 *  ENUMS
 * ========================================================= */

typedef enum { STATE_TITLE, STATE_PLAYING, STATE_PAUSED, STATE_GAMEOVER } GameState;

typedef enum {
    WEATHER_DAY    = 0,   /* Bright normal day  */
    WEATHER_SUNNY  = 1,   /* Warm golden hour   */
    WEATHER_RAIN   = 2,   /* Overcast + rain    */
    WEATHER_NIGHT  = 3    /* Dark night + stars */
} WeatherMode;

/* =========================================================
 *  STRUCTS
 * ========================================================= */

typedef struct {
    float x, y, scale, speed;
    int   layer;
} Cloud;

typedef struct {
    float x, gapCenterY;
    int   scored;
} Pipe;

typedef struct {
    float x, w, h, r, g, b;
} Building;

typedef struct {
    float y, vy, angle;
    int   wingFrame, wingTimer, alive;
} Bird;

typedef struct {
    float x, y, speed, alpha, len;
} RainDrop;

typedef struct {
    float x, y, size, phase;
} Star;

/* =========================================================
 *  WEATHER THEME  (sky colours + tints per mode)
 * ========================================================= */
typedef struct {
    /* Sky gradient: top and bottom colour */
    float topR, topG, topB;
    float botR, botG, botB;
    /* Cloud colour */
    float cldR, cldG, cldB, cldA;
    /* Grass colour */
    float grassR, grassG, grassB;
    /* Ambient darkness for buildings (0=normal, 1=dark) */
    float darkness;
    /* Name shown on screen */
    const char *name;
} WeatherTheme;

static const WeatherTheme g_themes[4] = {
    /* DAY   */ { 0.22f,0.60f,0.90f,  0.45f,0.83f,0.97f,  1.00f,1.00f,1.00f,0.92f,  0.49f,0.76f,0.26f,  0.0f, "Day"   },
    /* SUNNY */ { 0.12f,0.45f,0.92f,  0.98f,0.74f,0.22f,  1.00f,0.97f,0.82f,0.88f,  0.55f,0.82f,0.22f,  0.0f, "Sunny" },
    /* RAIN  */ { 0.22f,0.26f,0.36f,  0.34f,0.38f,0.50f,  0.72f,0.78f,0.84f,0.72f,  0.38f,0.52f,0.30f,  0.3f, "Rain"  },
    /* NIGHT */ { 0.02f,0.03f,0.18f,  0.05f,0.10f,0.34f,  0.80f,0.86f,0.96f,0.65f,  0.12f,0.32f,0.14f,  0.7f, "Night" }
};

/* =========================================================
 *  GLOBALS
 * ========================================================= */

static GameState  g_state       = STATE_TITLE;
static Bird       g_bird;
static Pipe       g_pipes[PIPE_COUNT];
static Cloud      g_clouds[CLOUD_COUNT];
static Building   g_buildings[BUILDING_COUNT];
static RainDrop   g_rain[RAIN_COUNT];
static Star       g_stars[STAR_COUNT];

static int        g_score       = 0;
static int        g_highScore   = 0;
static float      g_pipeSpeed   = PIPE_BASE_SPEED;

/* Screen shake */
static float      g_shakeX = 0, g_shakeY = 0;
static int        g_shakeTicks  = 0;

/* Flash effects */
static int        g_flashTicks  = 0;   /* white death flash */
static int        g_wFlashTicks = 0;   /* weather-change flash */

/* Title bob */
static float      g_titleBobY = 0, g_titleBobT = 0;

/* Ground scroll */
static float      g_groundScroll = 0;

/* Frame counter */
static int        g_frame = 0;

/* Weather */
static WeatherMode g_weather      = WEATHER_DAY;
static int         g_weatherTimer = 0;
static int         g_weatherNameTimer = 0;

/* Lightning (rain mode) */
static int         g_lightning  = 0;
static float       g_boltX      = 400.f;
static float       g_boltSegs[8];   /* y positions of bolt segments */

/* Fullscreen toggle */
static int         g_fullscreen = 1;

/* =========================================================
 *  UTILITY
 * ========================================================= */

static float lerpf(float a,float b,float t){ return a+(b-a)*t; }
static float clampf(float v,float lo,float hi){ return v<lo?lo:v>hi?hi:v; }
static float randf(float lo,float hi){ return lo+((float)rand()/(float)RAND_MAX)*(hi-lo); }

/* =========================================================
 *  DRAWING PRIMITIVES
 * ========================================================= */

static void col(int r,int g,int b)         { glColor3f(r/255.f,g/255.f,b/255.f); }
static void col4(int r,int g,int b,int a)  { glColor4f(r/255.f,g/255.f,b/255.f,a/255.f); }
static void colF(float r,float g,float b)  { glColor3f(r,g,b); }
static void colF4(float r,float g,float b,float a){ glColor4f(r,g,b,a); }

static void fillRect(float x,float y,float w,float h){
    glBegin(GL_QUADS);
        glVertex2f(x,y); glVertex2f(x+w,y);
        glVertex2f(x+w,y+h); glVertex2f(x,y+h);
    glEnd();
}

static void outlineRect(float x,float y,float w,float h,float t){
    fillRect(x,y,w,t); fillRect(x,y+h-t,w,t);
    fillRect(x,y,t,h); fillRect(x+w-t,y,t,h);
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

/* Draw an ellipse (scaled circle) */
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

/* =========================================================
 *  TEXT
 * ========================================================= */

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

/* =========================================================
 *  INIT: RAIN
 * ========================================================= */

static void initRain(void){
    for(int i=0;i<RAIN_COUNT;i++){
        g_rain[i].x     = randf(0.f, WORLD_W);
        g_rain[i].y     = randf(0.f, WORLD_H);
        g_rain[i].speed = randf(9.f, 16.f);
        g_rain[i].alpha = randf(0.35f, 0.75f);
        g_rain[i].len   = randf(10.f, 20.f);
    }
}

/* =========================================================
 *  INIT: STARS
 * ========================================================= */

static void initStars(void){
    for(int i=0;i<STAR_COUNT;i++){
        g_stars[i].x     = randf(0.f, WORLD_W);
        g_stars[i].y     = randf(GROUND_Y+GROUND_H+30.f, WORLD_H-8.f);
        g_stars[i].size  = randf(0.8f, 2.8f);
        g_stars[i].phase = randf(0.f, 6.28f);
    }
}

/* =========================================================
 *  INIT: CLOUDS
 * ========================================================= */

static void initClouds(void){
    for(int i=0;i<CLOUD_COUNT;i++){
        g_clouds[i].layer = i%2;
        g_clouds[i].x     = randf(0.f,WORLD_W);
        g_clouds[i].y     = randf(GROUND_Y+GROUND_H+70.f,WORLD_H-50.f);
        g_clouds[i].scale = randf(0.9f,1.7f);
        g_clouds[i].speed = (g_clouds[i].layer==0)?CLOUD_SPEED_FAR:CLOUD_SPEED_NEAR;
    }
}

/* =========================================================
 *  INIT: BUILDINGS
 * ========================================================= */

static void initBuildings(void){
    float pal[4][3]={
        {0.25f,0.35f,0.50f},{0.20f,0.30f,0.45f},
        {0.30f,0.40f,0.55f},{0.18f,0.28f,0.42f}
    };
    float x=0.f;
    for(int i=0;i<BUILDING_COUNT;i++){
        float w=randf(40.f,80.f);
        float h=randf(40.f,140.f);
        int p=rand()%4;
        g_buildings[i].x=x; g_buildings[i].w=w+4.f; g_buildings[i].h=h;
        g_buildings[i].r=pal[p][0]; g_buildings[i].g=pal[p][1]; g_buildings[i].b=pal[p][2];
        x+=w-randf(0.f,20.f);
    }
}

/* =========================================================
 *  INIT: BIRD / PIPES
 * ========================================================= */

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

/* =========================================================
 *  GAME LIFECYCLE
 * ========================================================= */

static void resetGame(void){
    g_score=0; g_pipeSpeed=PIPE_BASE_SPEED;
    g_shakeTicks=0; g_flashTicks=0; g_groundScroll=0.f;
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
    initRain();   initStars();

    /* Generate initial lightning bolt segments */
    for(int i=0;i<8;i++) g_boltSegs[i]=randf(-15.f,15.f);

    g_state=STATE_TITLE;
    g_weather=WEATHER_DAY;
    g_weatherTimer=0;
}

/* =========================================================
 *  SCREEN SHAKE
 * ========================================================= */

static void triggerShake(int t){ g_shakeTicks=t; }
static void updateShake(void){
    if(g_shakeTicks>0){
        float m=6.f*((float)g_shakeTicks/15.f);
        g_shakeX=randf(-m,m); g_shakeY=randf(-m,m);
        g_shakeTicks--;
    } else { g_shakeX=g_shakeY=0.f; }
}

/* =========================================================
 *  WEATHER CYCLE
 * ========================================================= */

static void nextWeather(void){
    g_weather=(WeatherMode)((g_weather+1)%WEATHER_COUNT);
    g_weatherTimer=0;
    g_weatherNameTimer=180;    /* show name for 3 seconds */
    g_wFlashTicks=WEATHER_FLASH_FRAMES;
    /* Reinit rain/stars for the new mode */
    initRain();
}

static void updateWeather(void){
    g_weatherTimer++;
    if(g_weatherTimer>=WEATHER_CYCLE_FRAMES) nextWeather();

    /* Weather name fade */
    if(g_weatherNameTimer>0) g_weatherNameTimer--;

    /* Weather flash decay */
    if(g_wFlashTicks>0) g_wFlashTicks--;

    /* Lightning in rain mode */
    if(g_weather==WEATHER_RAIN){
        if(g_lightning>0){
            g_lightning--;
        } else if(rand()%240==0){
            /* Trigger a lightning bolt */
            g_lightning=10;
            g_boltX=randf(80.f,WORLD_W-80.f);
            for(int i=0;i<8;i++) g_boltSegs[i]=randf(-25.f,25.f);
        }
    } else {
        g_lightning=0;
    }
}

/* =========================================================
 *  DRAW: SKY BACKGROUND  (weather-aware gradient)
 * ========================================================= */

static void drawBackground(void){
    const WeatherTheme *t=&g_themes[g_weather];
    glBegin(GL_QUADS);
        /* Bottom of sky (horizon) */
        glColor3f(t->botR,t->botG,t->botB);
        glVertex2f(0,GROUND_Y+GROUND_H);
        glVertex2f(WORLD_W,GROUND_Y+GROUND_H);
        /* Top of sky (zenith) */
        glColor3f(t->topR,t->topG,t->topB);
        glVertex2f(WORLD_W,WORLD_H);
        glVertex2f(0,WORLD_H);
    glEnd();
}

/* =========================================================
 *  DRAW: SUN  (Day + Sunny modes)
 * ========================================================= */

static void drawSun(void){
    /* Position: upper right area */
    float cx=WORLD_W-90.f, cy=WORLD_H-80.f, r=32.f;

    /* Outer glow */
    col4(255,235,100,35);
    fillCircle(cx,cy,r*2.2f,20);
    col4(255,240,120,55);
    fillCircle(cx,cy,r*1.55f,20);

    /* Animated rays */
    int rayN=14;
    for(int i=0;i<rayN;i++){
        float a=2.f*3.14159f*i/rayN + g_frame*0.008f;
        float pulse=1.f+0.18f*sinf(g_frame*0.05f+i*0.8f);
        float r1=r+6.f, r2=r+22.f*pulse;
        glLineWidth(2.8f);
        col4(255,220,60,160);
        glBegin(GL_LINES);
            glVertex2f(cx+cosf(a)*r1,cy+sinf(a)*r1);
            glVertex2f(cx+cosf(a)*r2,cy+sinf(a)*r2);
        glEnd();
        glLineWidth(2.f);
    }

    /* Sun body */
    col(255,240,80);
    fillCircle(cx,cy,r,22);

    /* Highlight spot */
    col(255,255,200);
    fillCircle(cx-r*0.25f,cy+r*0.25f,r*0.42f,14);

    /* Warm tint edge */
    col4(255,160,30,60);
    fillCircle(cx+r*0.15f,cy-r*0.1f,r*0.55f,14);
}

/* =========================================================
 *  DRAW: MOON  (Night mode)
 * ========================================================= */

static void drawMoon(void){
    float cx=WORLD_W-95.f, cy=WORLD_H-75.f, r=26.f;

    /* Soft glow around moon */
    col4(200,210,255,18);
    fillCircle(cx,cy,r*2.8f,20);
    col4(210,220,255,30);
    fillCircle(cx,cy,r*1.8f,20);

    /* Moon body */
    col(250,248,210);
    fillCircle(cx,cy,r,22);

    /* Craters */
    col(225,218,185);
    fillCircle(cx+r*0.30f,cy+r*0.22f,r*0.22f,12);
    fillCircle(cx-r*0.25f,cy-r*0.28f,r*0.14f,10);
    fillCircle(cx+r*0.05f,cy-r*0.10f,r*0.10f,10);
    fillCircle(cx-r*0.42f,cy+r*0.12f,r*0.10f,8);

    /* Crescent shadow (dark circle offset) - simulate with themed sky color */
    const WeatherTheme *th=&g_themes[WEATHER_NIGHT];
    glColor3f(th->topR,th->topG,th->topB);
    fillCircle(cx+r*0.40f,cy,r*0.82f,18);
}

/* =========================================================
 *  DRAW: STARS  (Night mode)
 * ========================================================= */

static void drawStars(void){
    for(int i=0;i<STAR_COUNT;i++){
        float twinkle=0.55f+0.45f*sinf(g_frame*0.04f+g_stars[i].phase);
        col4(255,255,230,(int)(twinkle*245));
        float sz=g_stars[i].size;
        /* Draw as a small 4-point cross for bigger stars */
        if(sz>2.0f){
            glBegin(GL_LINES);
                glVertex2f(g_stars[i].x-sz*1.8f, g_stars[i].y);
                glVertex2f(g_stars[i].x+sz*1.8f, g_stars[i].y);
                glVertex2f(g_stars[i].x, g_stars[i].y-sz*1.8f);
                glVertex2f(g_stars[i].x, g_stars[i].y+sz*1.8f);
            glEnd();
        }
        fillCircle(g_stars[i].x,g_stars[i].y,sz,6);
    }
}

/* =========================================================
 *  DRAW: RAIN PARTICLES
 * ========================================================= */

static void drawRain(void){
    glLineWidth(1.5f);
    for(int i=0;i<RAIN_COUNT;i++){
        float a=g_rain[i].alpha;
        col4(180,205,230,(int)(a*210));
        float x=g_rain[i].x, y=g_rain[i].y, len=g_rain[i].len;
        glBegin(GL_LINES);
            glVertex2f(x,y);
            glVertex2f(x-len*0.28f,y-len);   /* slight rightward angle */
        glEnd();
    }
    glLineWidth(2.f);
}

/* =========================================================
 *  DRAW: LIGHTNING BOLT
 * ========================================================= */

static void drawLightning(void){
    if(g_lightning<=0) return;
    float a=(float)g_lightning/10.f;

    /* Full-screen flash */
    col4(210,225,255,(int)(a*65));
    fillRect(0,0,WORLD_W,WORLD_H);

    /* Zigzag bolt */
    float bx=g_boltX, by=WORLD_H;
    float segH=(WORLD_H-GROUND_Y-GROUND_H)*0.13f;
    glLineWidth(3.5f);
    col4(240,248,255,(int)(a*240));
    glBegin(GL_LINE_STRIP);
        glVertex2f(bx,by);
        for(int i=0;i<8;i++){
            bx+=g_boltSegs[i];
            by-=segH;
            glVertex2f(bx,by);
        }
    glEnd();
    /* Glow pass */
    glLineWidth(7.f);
    col4(180,210,255,(int)(a*80));
    bx=g_boltX; by=WORLD_H;
    glBegin(GL_LINE_STRIP);
        glVertex2f(bx,by);
        for(int i=0;i<8;i++){
            bx+=g_boltSegs[i];
            by-=segH;
            glVertex2f(bx,by);
        }
    glEnd();
    glLineWidth(2.f);
}

/* =========================================================
 *  DRAW: FOG OVERLAY  (Rain mode)
 * ========================================================= */

static void drawFog(void){
    /* Layered horizontal fog bands */
    for(int i=0;i<3;i++){
        float fy=GROUND_Y+GROUND_H+(i*60.f);
        col4(160,170,185,28-i*5);
        fillRect(0,fy,WORLD_W,80.f);
    }
    /* Slight global haze */
    col4(140,155,170,18);
    fillRect(0,0,WORLD_W,WORLD_H);
}

/* =========================================================
 *  DRAW: BEAUTIFUL CLOUDS
 *  8 overlapping ellipses + shadow base + bright highlight
 * ========================================================= */

static void drawCloud(float cx,float cy,float sc){
    /*
     * Layout: {offset-x, offset-y, radius-x, radius-y}
     * Shadow base blob, 6 body blobs, 1 inner highlight
     */
    static const float body[7][4]={
        {  0.f,  0.f, 34.f, 22.f },   /* centre main       */
        { 30.f,  4.f, 26.f, 18.f },   /* right lobe        */
        {-30.f,  2.f, 24.f, 16.f },   /* left lobe         */
        { 15.f, 16.f, 22.f, 16.f },   /* upper right puff  */
        {-15.f, 14.f, 20.f, 15.f },   /* upper left puff   */
        { 48.f, -2.f, 16.f, 12.f },   /* far right wisp    */
        {-46.f, -1.f, 15.f, 11.f },   /* far left wisp     */
    };

    const WeatherTheme *t=&g_themes[g_weather];

    /* === Shadow layer at base === */
    colF4(t->cldR*0.72f, t->cldG*0.72f, t->cldB*0.78f, t->cldA*0.55f);
    fillEllipse(cx, cy-6.f*sc, 36.f*sc, 8.f*sc, 14);

    /* === Main body blobs === */
    colF4(t->cldR, t->cldG, t->cldB, t->cldA);
    for(int i=0;i<7;i++)
        fillEllipse(cx+body[i][0]*sc, cy+body[i][1]*sc,
                    body[i][2]*sc,    body[i][3]*sc, 16);

    /* === Mid tint (slightly grey) for depth === */
    colF4(t->cldR*0.88f, t->cldG*0.88f, t->cldB*0.90f, t->cldA*0.50f);
    fillEllipse(cx, cy, 30.f*sc, 18.f*sc, 14);

    /* === Bright highlight on top-left === */
    colF4(1.f, 1.f, 1.f, t->cldA*0.70f);
    fillEllipse(cx-8.f*sc, cy+12.f*sc, 16.f*sc, 9.f*sc, 12);

    /* === Soft inner glow centre === */
    colF4(1.f, 1.f, 1.f, t->cldA*0.25f);
    fillEllipse(cx, cy+4.f*sc, 22.f*sc, 14.f*sc, 12);
}

static void drawClouds(void){
    for(int i=0;i<CLOUD_COUNT;i++)
        drawCloud(g_clouds[i].x, g_clouds[i].y, g_clouds[i].scale);
}

/* =========================================================
 *  DRAW: CITY SILHOUETTE  (weather-aware brightness)
 * ========================================================= */

static void drawCitySilhouette(void){
    float groundSurface=GROUND_Y+GROUND_H*GRASS_H_RATIO;
    float dark=g_themes[g_weather].darkness;

    for(int i=0;i<BUILDING_COUNT;i++){
        float bx=g_buildings[i].x, bw=g_buildings[i].w;
        float bh=g_buildings[i].h, by=groundSurface-2.f;
        float br=g_buildings[i].r*(1.f-dark);
        float bg=g_buildings[i].g*(1.f-dark);
        float bb=g_buildings[i].b*(1.f-dark*0.7f);
        glColor3f(br,bg,bb);
        fillRect(bx,by,bw,bh);

        /* Windows - different colours at night vs day */
        float wr,wg,wb,wa;
        if(g_weather==WEATHER_NIGHT){
            wr=1.0f; wg=0.85f; wb=0.45f; wa=0.90f; /* warm orange glow */
        } else {
            wr=0.84f; wg=0.93f; wb=1.0f; wa=0.70f; /* cool blue-white */
        }
        colF4(wr,wg,wb,wa);
        float wSz=5.f,wGX=14.f,wGY=18.f;
        for(float wy2=by+10.f;wy2+wSz<by+bh-10.f;wy2+=wGY)
            for(float wx2=bx+8.f;wx2+wSz<bx+bw-8.f;wx2+=wGX)
                if((int)(wx2*7+wy2*13)%3!=0){
                    /* Night: window glow */
                    if(g_weather==WEATHER_NIGHT){
                        col4(255,200,80,35);
                        fillRect(wx2-2.f,wy2-2.f,wSz+4.f,wSz+4.f);
                    }
                    colF4(wr,wg,wb,wa);
                    fillRect(wx2,wy2,wSz,wSz);
                }
    }
}

/* =========================================================
 *  DRAW: GROUND  (weather-aware)
 * ========================================================= */

static void drawGround(void){
    float grassH=GROUND_H*GRASS_H_RATIO;
    const WeatherTheme *t=&g_themes[g_weather];

    /* Dirt */
    if(g_weather==WEATHER_NIGHT){
        col(100,80,55);
    } else if(g_weather==WEATHER_RAIN){
        col(150,120,90);
    } else {
        col(222,184,135);
    }
    fillRect(0,0,WORLD_W,GROUND_Y);

    /* Grass */
    colF(t->grassR,t->grassG,t->grassB);
    fillRect(0,GROUND_Y,WORLD_W,grassH);

    /* Scrolling tile details */
    colF(t->grassR*0.82f,t->grassG*0.82f,t->grassB*0.82f);
    float tileW=40.f;
    float scroll=fmodf(g_groundScroll,tileW*2.f);
    for(float tx=-tileW*2.f+scroll;tx<WORLD_W+tileW;tx+=tileW*2.f)
        fillRect(tx,GROUND_Y+4.f,tileW-4.f,8.f);

    /* Grass edge */
    colF(t->grassR*0.65f,t->grassG*0.65f,t->grassB*0.65f);
    fillRect(0,GROUND_Y+grassH-2.f,WORLD_W,3.f);

    /* Puddles in rain mode */
    if(g_weather==WEATHER_RAIN){
        col4(120,145,175,90);
        for(int i=0;i<8;i++){
            float px=fmodf(i*137.f+22.f,WORLD_W-40.f);
            float pw=20.f+fmodf(i*53.f,30.f);
            fillEllipse(px+pw/2.f,GROUND_Y-4.f,pw/2.f,5.f,12);
        }
    }

    /* Pebbles */
    col4(170,140,100,180);
    for(int i=0;i<20;i++){
        float px=fmodf(i*123.7f+37.f,WORLD_W);
        float py=12.f+fmodf(i*71.3f,GROUND_Y-20.f);
        fillRect(px,py,5.f,3.f);
    }
}

/* =========================================================
 *  DRAW: BIRD
 * ========================================================= */

static void drawBird(void){
    glPushMatrix();
    glTranslatef(BIRD_X,g_bird.y,0.f);
    glRotatef(g_bird.angle,0.f,0.f,1.f);

    /* Body */
    col(255,195,50);
    fillCircle(0,0,BIRD_RADIUS,20);
    col(200,120,20);
    glLineWidth(2.5f);
    glBegin(GL_LINE_LOOP);
        for(int i=0;i<20;i++){
            float a=2.f*3.14159265f*i/20;
            glVertex2f(cosf(a)*BIRD_RADIUS,sinf(a)*BIRD_RADIUS);
        }
    glEnd();
    glLineWidth(2.f);

    /* Wing */
    float wOff[3]={0.f,8.f,-8.f};
    float wy=wOff[g_bird.wingFrame];
    col(230,155,20);
    fillEllipse(-6.f,wy,10.f,6.f,14);
    col(190,110,10);
    glPushMatrix();
        glTranslatef(-6.f,wy,0.f);
        glScalef(10.f,6.f,1.f);
        glBegin(GL_LINE_LOOP);
            for(int s=0;s<14;s++){float a=2.f*3.14159265f*s/14;glVertex2f(cosf(a),sinf(a));}
        glEnd();
    glPopMatrix();

    /* Belly */
    col(255,230,150);
    fillEllipse(3.f,-3.f,10.f,8.f,14);

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
    col(200,80,10);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
        glVertex2f(BIRD_RADIUS-2.f,3.5f);
        glVertex2f(BIRD_RADIUS+12.f,0.f);
        glVertex2f(BIRD_RADIUS-2.f,-3.5f);
    glEnd();
    glLineWidth(2.f);
    glPopMatrix();
}

/* =========================================================
 *  DRAW: PIPES  (weather-tinted)
 * ========================================================= */

static void drawSinglePipe(float x,float y1,float y2,int flipped){
    float dark=g_themes[g_weather].darkness;
    float dr=1.f-dark*0.4f, dg=1.f-dark*0.3f, db=1.f-dark*0.2f;

    float capExtraW=14.f, capOff=capExtraW/2.f, capW=PIPE_W+capExtraW;

    /* Shaft */
    colF(80/255.f*dr, 200/255.f*dg, 80/255.f*db);
    fillRect(x,y1,PIPE_W,y2-y1);
    colF(120/255.f*dr, 230/255.f*dg, 100/255.f*db);
    fillRect(x+8.f,y1,12.f,y2-y1);
    colF(40/255.f*dr, 140/255.f*dg, 40/255.f*db);
    fillRect(x+PIPE_W-10.f,y1,10.f,y2-y1);
    colF(30/255.f*dr, 110/255.f*dg, 30/255.f*db);
    outlineRect(x,y1,PIPE_W,y2-y1,2.5f);

    /* Cap */
    float capY=flipped?y1-4.f:y2-PIPE_CAP_H, capH=PIPE_CAP_H+4.f;
    colF(80/255.f*dr, 200/255.f*dg, 80/255.f*db);
    fillRect(x-capOff,capY,capW,capH);
    colF(120/255.f*dr, 230/255.f*dg, 100/255.f*db);
    fillRect(x-capOff+8.f,capY,14.f,capH);
    colF(40/255.f*dr, 140/255.f*dg, 40/255.f*db);
    fillRect(x-capOff+capW-12.f,capY,12.f,capH);
    colF(30/255.f*dr, 110/255.f*dg, 30/255.f*db);
    outlineRect(x-capOff,capY,capW,capH,2.5f);
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

/* =========================================================
 *  DRAW: HUD
 * ========================================================= */

static void drawHUD(void){
    char buf[32];
    float cx=WORLD_W/2.f, cy=WORLD_H-55.f;
    sprintf(buf,"%d",g_score);
    float sw=strokeWidth(buf,0.20f);
    col(40,40,40);
    strokeText(cx-sw/2.f+2.f,cy-2.f,0.20f,buf);
    col(255,255,255);
    strokeText(cx-sw/2.f,cy,0.20f,buf);

    sprintf(buf,"BEST: %d",g_highScore);
    col(50,50,50);
    bitmapText(10.f,WORLD_H-22.f,GLUT_BITMAP_HELVETICA_18,buf);

    /* Weather indicator (top right) */
    const char *wName=g_themes[g_weather].name;
    /* Small icon + name */
    col(255,255,255);
    char wBuf[32]; sprintf(wBuf,"[W] %s",wName);
    bitmapText(WORLD_W-110.f,WORLD_H-22.f,GLUT_BITMAP_HELVETICA_12,wBuf);
}

/* =========================================================
 *  DRAW: WEATHER NAME ANNOUNCEMENT
 * ========================================================= */

static void drawWeatherName(void){
    if(g_weatherNameTimer<=0) return;
    float alpha=clampf((float)g_weatherNameTimer/60.f,0.f,1.f);
    const char *name=g_themes[g_weather].name;

    /* Panel */
    col4(0,0,0,(int)(alpha*130));
    float pw=200.f, ph=55.f;
    float px=WORLD_W/2.f-pw/2.f, py=WORLD_H*0.72f;
    fillRect(px,py,pw,ph);
    col4(255,255,255,(int)(alpha*60));
    outlineRect(px,py,pw,ph,2.f);

    /* Text */
    col4(255,255,255,(int)(alpha*255));
    float tw=strokeWidth(name,0.16f);
    strokeText(WORLD_W/2.f-tw/2.f,py+18.f,0.16f,name);
}

/* =========================================================
 *  DRAW: TITLE SCREEN
 * ========================================================= */

static void drawTitleScreen(void){
    float tY=WORLD_H*0.62f+g_titleBobY;
    float sc=0.30f;
    float w1=strokeWidth("FLAPPY",sc), w2=strokeWidth("BIRD",sc);

    col(50,80,20);
    strokeText(WORLD_W/2.f-w1/2.f+3.f,tY-3.f,sc,"FLAPPY");
    strokeText(WORLD_W/2.f-w2/2.f+3.f,tY-55.f-3.f,sc,"BIRD");
    col(255,220,40);
    strokeText(WORLD_W/2.f-w1/2.f,tY,sc,"FLAPPY");
    col(120,230,60);
    strokeText(WORLD_W/2.f-w2/2.f,tY-55.f,sc,"BIRD");

    float bx=WORLD_W/2.f-160.f, by=tY-70.f;
    col(255,255,255); outlineRect(bx,by,320.f,110.f,4.f);
    col(50,120,20);   outlineRect(bx+4.f,by+4.f,312.f,102.f,3.f);

    float pulse=0.6f+0.4f*sinf(g_frame*0.08f);
    col4(255,255,255,(int)(pulse*255));
    const char *p="Press SPACE or Click to Start";
    strokeText(WORLD_W/2.f-strokeWidth(p,0.085f)/2.f,WORLD_H*0.30f,0.085f,p);

    /* W key hint */
    col4(255,255,255,160);
    bitmapText(WORLD_W/2.f-80.f,WORLD_H*0.24f,
               GLUT_BITMAP_HELVETICA_12,"W = Change Weather   F11 = Fullscreen");

    /* Showcase bird */
    float sy=g_bird.y, sa=g_bird.angle;
    g_bird.y=tY-25.f; g_bird.angle=0.f;
    drawBird();
    g_bird.y=sy; g_bird.angle=sa;

    col(30,60,10);
    bitmapText(WORLD_W/2.f-80.f,GROUND_Y+GROUND_H+10.f,
               GLUT_BITMAP_HELVETICA_12,"P = Pause   ESC = Quit");
}

/* =========================================================
 *  DRAW: GAME OVER
 * ========================================================= */

static void drawGameOverScreen(void){
    col4(0,0,0,105); fillRect(0,0,WORLD_W,WORLD_H);
    float px=WORLD_W/2.f-180.f, py=WORLD_H/2.f-80.f;
    col4(250,240,200,242); fillRect(px,py,360.f,175.f);
    col(90,60,20);  outlineRect(px,py,360.f,175.f,4.f);
    col(180,130,50);outlineRect(px+4.f,py+4.f,352.f,167.f,3.f);

    const char *go="GAME OVER"; float gsc=0.20f;
    float gw=strokeWidth(go,gsc);
    col(200,40,40);  strokeText(WORLD_W/2.f-gw/2.f+2.f,py+125.f-2.f,gsc,go);
    col(255,80,80);  strokeText(WORLD_W/2.f-gw/2.f,py+125.f,gsc,go);

    char buf[32];
    sprintf(buf,"Score: %d",g_score);
    col(60,40,10); bitmapText(WORLD_W/2.f-55.f,py+90.f,GLUT_BITMAP_HELVETICA_18,buf);
    sprintf(buf,"Best:  %d",g_highScore);
    bitmapText(WORLD_W/2.f-55.f,py+65.f,GLUT_BITMAP_HELVETICA_18,buf);

    float pulse=0.55f+0.45f*sinf(g_frame*0.10f);
    col4(80,40,0,(int)(pulse*255));
    const char *rp="Press SPACE or R to Restart";
    strokeText(WORLD_W/2.f-strokeWidth(rp,0.065f)/2.f,py+18.f,0.065f,rp);
}

/* =========================================================
 *  DRAW: PAUSE
 * ========================================================= */

static void drawPauseScreen(void){
    col4(0,0,0,80); fillRect(0,0,WORLD_W,WORLD_H);
    const char *pt="PAUSED"; float psc=0.25f;
    col(255,255,255);
    strokeText(WORLD_W/2.f-strokeWidth(pt,psc)/2.f,WORLD_H/2.f-10.f,psc,pt);
    col(200,200,200);
    bitmapText(WORLD_W/2.f-75.f,WORLD_H/2.f-50.f,GLUT_BITMAP_HELVETICA_18,"Press P to Resume");
}

/* =========================================================
 *  DISPLAY CALLBACK
 * ========================================================= */

static void display(void){
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();
    glTranslatef(g_shakeX,g_shakeY,0.f);

    /* --- Background layer --- */
    drawBackground();

    /* Weather-specific sky features */
    if(g_weather==WEATHER_SUNNY || g_weather==WEATHER_DAY) drawSun();
    if(g_weather==WEATHER_NIGHT) { drawStars(); drawMoon(); }

    drawClouds();
    drawCitySilhouette();

    /* --- Game content --- */
    switch(g_state){
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

    /* --- Weather overlays --- */
    if(g_weather==WEATHER_RAIN){
        drawFog();
        drawRain();
        drawLightning();
    }

    /* Weather name announcement */
    drawWeatherName();

    /* Weather-change flash */
    if(g_wFlashTicks>0){
        float a=(float)g_wFlashTicks/WEATHER_FLASH_FRAMES;
        col4(255,255,255,(int)(a*180));
        fillRect(0,0,WORLD_W,WORLD_H);
    }

    /* Death flash */
    if(g_flashTicks>0){
        float a=(float)g_flashTicks/10.f;
        col4(255,255,255,(int)(a*200));
        fillRect(0,0,WORLD_W,WORLD_H);
    }

    glutSwapBuffers();
}

/* =========================================================
 *  COLLISION DETECTION
 * ========================================================= */

static int checkCollision(void){
    float bx=BIRD_X, by=g_bird.y, br=BIRD_RADIUS-4.f;
    float gs=GROUND_Y+GROUND_H*GRASS_H_RATIO;
    if(by-br<=gs)     return 1;
    if(by+br>=WORLD_H)return 1;
    for(int i=0;i<PIPE_COUNT;i++){
        float px=g_pipes[i].x;
        float gT=g_pipes[i].gapCenterY+PIPE_GAP/2.f;
        float gB=g_pipes[i].gapCenterY-PIPE_GAP/2.f;
        float cOff=7.f;
        if(bx+br>px-cOff && bx-br<px+PIPE_W+cOff){
            if(by-br<gB) return 1;
            if(by+br>gT) return 1;
        }
    }
    return 0;
}

/* =========================================================
 *  UPDATE: BIRD
 * ========================================================= */

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

/* =========================================================
 *  UPDATE: PIPES
 * ========================================================= */

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
        if(!g_pipes[i].scored && g_pipes[i].x+PIPE_W/2.f<BIRD_X){
            g_pipes[i].scored=1;
            g_score++;
            if(g_score>g_highScore) g_highScore=g_score;
        }
    }
    g_pipeSpeed+=PIPE_SPEED_INC;
}

/* =========================================================
 *  UPDATE: CLOUDS + RAIN
 * ========================================================= */

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

/* =========================================================
 *  MASTER UPDATE
 * ========================================================= */

static void updateGame(void){
    g_frame++;

    /* Weather update runs in all states */
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
        if(g_weather==WEATHER_RAIN) updateRainDrops();
        updateClouds();
        updateShake();
        return;
    }

    /* --- Playing --- */
    updateBird();
    updatePipes();
    updateClouds();
    if(g_weather==WEATHER_RAIN) updateRainDrops();
    g_groundScroll+=g_pipeSpeed;

    if(checkCollision()){
        g_state=STATE_GAMEOVER;
        g_bird.alive=0;
        g_flashTicks=10;
        triggerShake(15);
    }
    updateShake();
    if(g_flashTicks>0) g_flashTicks--;
}

/* =========================================================
 *  TIMER  (60 FPS)
 * ========================================================= */

static void timerCallback(int v){
    (void)v;
    updateGame();
    glutPostRedisplay();
    glutTimerFunc(FRAME_MS,timerCallback,0);
}

/* =========================================================
 *  FLAP
 * ========================================================= */

static void doFlap(void){
    if(g_state==STATE_TITLE||g_state==STATE_GAMEOVER){ resetGame(); return; }
    if(g_state==STATE_PLAYING){
        g_bird.vy=FLAP_VEL;
        g_bird.angle=TILT_UP_DEG;
        g_bird.wingFrame=1;
    }
}

/* =========================================================
 *  INPUT
 * ========================================================= */

static void keyboardInput(unsigned char key,int x,int y){
    (void)x;(void)y;
    switch(key){
        case ' ':           doFlap(); break;
        case 'r': case 'R':
            if(g_state==STATE_GAMEOVER||g_state==STATE_TITLE) resetGame();
            break;
        case 'p': case 'P':
            if(g_state==STATE_PLAYING)      g_state=STATE_PAUSED;
            else if(g_state==STATE_PAUSED)  g_state=STATE_PLAYING;
            break;
        case 'w': case 'W':
            nextWeather();
            break;
        case 27: exit(0); break;
        default: break;
    }
}

static void specialKeys(int key,int x,int y){
    (void)x;(void)y;
    if(key==GLUT_KEY_F11){
        g_fullscreen=!g_fullscreen;
        if(g_fullscreen){
            glutFullScreen();
        } else {
            glutReshapeWindow(WIN_W,WIN_H);
            glutPositionWindow(100,80);
        }
    }
}

static void mouseInput(int button,int state,int x,int y){
    (void)x;(void)y;
    if(button==GLUT_LEFT_BUTTON&&state==GLUT_DOWN) doFlap();
}

/* =========================================================
 *  RESHAPE
 * ========================================================= */

static void reshape(int w,int h){
    if(h==0)h=1;
    float wa=WORLD_W/WORLD_H, ww=(float)w/(float)h;
    int vpX,vpY,vpW,vpH;
    if(ww>wa){ vpH=h; vpW=(int)(h*wa); vpX=(w-vpW)/2; vpY=0; }
    else      { vpW=w; vpH=(int)(w/wa); vpX=0; vpY=(h-vpH)/2; }
    glViewport(vpX,vpY,vpW,vpH);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluOrtho2D(0.0,WORLD_W,0.0,WORLD_H);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
}

/* =========================================================
 *  MAIN
 * ========================================================= */

int main(int argc,char **argv){
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB);
    glutInitWindowSize(WIN_W,WIN_H);
    glutInitWindowPosition(50,30);
    glutCreateWindow("Flappy Bird  -  OpenGL Edition");

    /* Launch full screen */
    glutFullScreen();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboardInput);
    glutSpecialFunc(specialKeys);
    glutMouseFunc(mouseInput);
    glutTimerFunc(FRAME_MS,timerCallback,0);

    init();
    glutMainLoop();
    return 0;
}

/*
 * =========================================================
 *  SOUND HOOK STUBS
 *  Uncomment + add -lwinmm to linker to enable:
 *
 *  #include <windows.h>
 *  #include <mmsystem.h>
 *  void playSound(int id){
 *      const char *f[]={"flap.wav","score.wav","die.wav"};
 *      PlaySound(f[id],NULL,SND_FILENAME|SND_ASYNC);
 *  }
 * =========================================================
 */
