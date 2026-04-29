#include <GL/glut.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PI 3.14159265358979323846
#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 700


//  GLOBALS

bool isNight = false;
float dayFactor = 1.0f;

float cloud1X = -600, cloud2X = -100, cloud3X = 300, cloud4X = 550;
float cloudSpeed = 0.5f;

float bfX = -100, bfY = -50, bfWingAngle = 0, bfTime = 0;
bool bfGoingRight = true;
float bf2X = 200, bf2Y = -30, bf2WingAngle = 0.5f, bf2Time = 2;
bool bf2GoingRight = false;

float bird1X = -500, bird1Y = 210, bird2X = -250, bird2Y = 235;
float birdWing1 = 0, birdWing2 = 0.5f;

float flowerSway = 0, swayDir = 1;

bool rainEnabled = false;
float rainDrops[200][2];

float animSpeed = 1.0f, glowPulse = 0, waterShimmer = 0;

float fish1X = 0, fish1Dir = 1, fish2X = 0, fish2Dir = -1, fish3X = 0, fish3Dir = 1;
float fishTime = 0;

float fireflies[20][3];

// Airplane globals
float planeX = -800;
float planeY = 280;
float planeSpeed = 1.2f;
bool planeGoingRight = true;

// Dragonfly
float dfX = 300, dfY = -100, dfTime = 0;
bool dfGoingRight = false;

// STORM GLOBALS 
bool stormEnabled = false;
float stormIntensity = 0.0f;      // 0..1 ramp
float stormTargetIntensity = 0.0f;
float lightningFlash = 0.0f;      // brightness 0..1
float lightningTimer = 0.0f;
float thunderRumble = 0.0f;
float stormWindOffset = 0.0f;
float stormRainDrops[400][2];     // heavier rain for storm
float stormCloudDarkness = 0.0f;

// CAT GLOBALS 
// Cat house is at right side outside fence: x=620, y=-280
float catHouseX = 480, catHouseY = -320;

// Cat states
enum CatState {
    CAT_SITTING,       // on bench, idle
    CAT_RUNNING_TO_HOUSE,  // running toward cat house
    CAT_IN_HOUSE,      // hiding in house
    CAT_RUNNING_TO_BENCH   // returning to bench
};

CatState catState = CAT_SITTING;
float catX = 65, catY = -152;       // current cat position (bench top)
float catBenchX = 65, catBenchY = -170;  // bench sitting position
float catTargetX, catTargetY;
float catRunSpeed = 2.5f;
float catAnimTime = 0;
float catLegPhase = 0;
bool catFacingRight = true;
float catTailWag = 0;

// Track previous weather for cat triggers
bool catWasTriggered = false;


//  FORWARD DECLARATIONS

void drawFish(float x, float y, float scale, float dir,
    float r, float g, float b);
void initStormRain();

//============================================================
//  ALGORITHM 1: DDA LINE
//============================================================
void DDA_Line(float x1, float y1, float x2, float y2)
{
    float dx = x2 - x1, dy = y2 - y1, steps;
    if (fabs(dx) >= fabs(dy)) steps = fabs(dx); else steps = fabs(dy);
    if (steps == 0) { glBegin(GL_POINTS); glVertex2f(x1, y1); glEnd(); return; }
    float xI = dx / steps, yI = dy / steps, x = x1, y = y1;
    glBegin(GL_POINTS);
    for (int i = 0; i <= (int)steps; i++) { glVertex2f(x, y); x += xI; y += yI; }
    glEnd();
}

//============================================================
//  ALGORITHM 2: BRESENHAM LINE
//============================================================
void Bresenham_Line(int x1, int y1, int x2, int y2)
{
    int dx = abs(x2 - x1), dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1, sy = (y1 < y2) ? 1 : -1, err = dx - dy;
    glBegin(GL_POINTS);
    while (true) {
        glVertex2i(x1, y1);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx) { err += dx; y1 += sy; }
    }
    glEnd();
}

//============================================================
//  ALGORITHM 3: MIDPOINT CIRCLE
//============================================================
void MidpointCircle(float cx, float cy, float r)
{
    float x = 0, y = r, p = 1 - r;
    glBegin(GL_POINTS);
    while (x <= y) {
        glVertex2f(cx + x, cy + y); glVertex2f(cx - x, cy + y);
        glVertex2f(cx + x, cy - y); glVertex2f(cx - x, cy - y);
        glVertex2f(cx + y, cy + x); glVertex2f(cx - y, cy + x);
        glVertex2f(cx + y, cy - x); glVertex2f(cx - y, cy - x);
        x++;
        if (p < 0) p += 2 * x + 1; else { y--; p += 2 * (x - y) + 1; }
    }
    glEnd();
}


//  HELPERS

void FilledCircle(float cx, float cy, float r,
    float R, float G, float B, float A = 1.0f)
{
    glColor4f(R, G, B, A);
    glBegin(GL_TRIANGLE_FAN); glVertex2f(cx, cy);
    for (int i = 0; i <= 360; i += 3) {
        float a = i * PI / 180.0f;
        glVertex2f(cx + r * cos(a), cy + r * sin(a));
    }
    glEnd();
}

void GlowCircle(float cx, float cy, float r,
    float R, float G, float B, float cA, float eA)
{
    glBegin(GL_TRIANGLE_FAN);
    glColor4f(R, G, B, cA); glVertex2f(cx, cy);
    glColor4f(R, G, B, eA);
    for (int i = 0; i <= 360; i += 3) {
        float a = i * PI / 180.0f;
        glVertex2f(cx + r * cos(a), cy + r * sin(a));
    }
    glEnd();
}

void FilledEllipse(float cx, float cy, float rx, float ry,
    float R, float G, float B, float A = 1.0f)
{
    glColor4f(R, G, B, A);
    glBegin(GL_TRIANGLE_FAN); glVertex2f(cx, cy);
    for (int i = 0; i <= 360; i += 3) {
        float a = i * PI / 180.0f;
        glVertex2f(cx + rx * cos(a), cy + ry * sin(a));
    }
    glEnd();
}

//============================================================
//  2D TRANSFORMATIONS
//============================================================
void applyTranslation(float tx, float ty) { glTranslatef(tx, ty, 0); }
void applyRotation(float a) { glRotatef(a, 0, 0, 1); }
void applyScaling(float sx, float sy) { glScalef(sx, sy, 1); }
void applyReflectionX() { glScalef(1, -1, 1); }
void applyReflectionY() { glScalef(-1, 1, 1); }
void applyShear(float shx, float shy) {
    float m[16] = { 1,shy,0,0,shx,1,0,0,0,0,1,0,0,0,0,1 };
    glMultMatrixf(m);
}

//============================================================
//  STORM INTENSITY HELPER
//============================================================
float getStormDarkening()
{
    // Returns 0..1 where 1 = full storm darkness
    return stormIntensity * 0.55f;
}

float getEffectiveDayFactor()
{
    // Storm darkens the scene even during day
    float f = dayFactor;
    float dark = getStormDarkening();
    return f * (1.0f - dark);
}

//============================================================
//  DRAW: SKY
//============================================================
void drawSky()
{
    float f = dayFactor;
    float sd = getStormDarkening();

    float tR = (0.12f * f + 0.01f * (1 - f)) * (1 - sd * 0.7f);
    float tG = (0.30f * f + 0.01f * (1 - f)) * (1 - sd * 0.7f);
    float tB = (0.72f * f + 0.08f * (1 - f)) * (1 - sd * 0.5f);
    float bR = (0.55f * f + 0.04f * (1 - f)) * (1 - sd * 0.6f);
    float bG = (0.75f * f + 0.04f * (1 - f)) * (1 - sd * 0.6f);
    float bB = (0.98f * f + 0.18f * (1 - f)) * (1 - sd * 0.4f);
    float hR = (0.70f * f + 0.05f * (1 - f)) * (1 - sd * 0.6f);
    float hG = (0.60f * f + 0.04f * (1 - f)) * (1 - sd * 0.6f);
    float hB = (0.85f * f + 0.15f * (1 - f)) * (1 - sd * 0.4f);

    // Storm: add purplish/grey tint
    if (stormIntensity > 0.01f) {
        float st = stormIntensity;
        tR += 0.06f * st; tG += 0.04f * st; tB += 0.08f * st;
        bR += 0.05f * st; bG += 0.04f * st; bB += 0.06f * st;
    }

    // Lightning flash overlay
    float lf = lightningFlash * 0.4f;

    glBegin(GL_QUADS);
    glColor3f(hR + lf, hG + lf, hB + lf);
    glVertex2f(-600, -50); glVertex2f(600, -50);
    glColor3f(bR + lf, bG + lf, bB + lf);
    glVertex2f(600, 120); glVertex2f(-600, 120);
    glEnd();

    glBegin(GL_QUADS);
    glColor3f(bR + lf, bG + lf, bB + lf);
    glVertex2f(-600, 120); glVertex2f(600, 120);
    glColor3f(tR + lf, tG + lf, tB + lf);
    glVertex2f(600, 350); glVertex2f(-600, 350);
    glEnd();

    if (f > 0.3f && stormIntensity < 0.5f)
        GlowCircle(0, -30, 500, 1, 0.85f, 0.65f, 0.06f * f * (1 - stormIntensity), 0);
}

//============================================================
//  DRAW: STARS
//============================================================
void drawStars()
{
    if (dayFactor > 0.6f) return;
    if (stormIntensity > 0.3f) return; // stars hidden in storm
    float alpha = 1 - dayFactor;
    float stars[][2] = {
        {-500,300},{-450,320},{-380,290},{-300,340},{-200,310},{-100,330},
        {50,300},{150,340},{250,310},{350,330},{450,300},{500,320},
        {-550,270},{-420,260},{-150,280},{100,270},{300,260},{480,275},
        {-250,250},{20,255},{400,248},{-480,340},{-350,310},{-50,345},
        {200,335},{380,295},{520,310},{-520,295},{-280,330},{80,325},
        {-570,310},{-330,265},{-180,345},{60,285},{170,295},{330,345},
        {470,265},{540,290},{-400,248},{-220,275},{130,310},{280,340},
        {-460,295},{-140,258},{40,340},{220,268},{360,285},{490,315}
    };
    int n = sizeof(stars) / sizeof(stars[0]);
    for (int i = 0; i < n; i++) {
        float tw = 0.5f + 0.5f * sin(glowPulse * 2 + i * 1.7f);
        float sz = 1.5f + 1.0f * sin(glowPulse * 1.5f + i * 2.3f);
        glPointSize(sz);
        glColor4f(1, 1, 0.92f, alpha * tw);
        glBegin(GL_POINTS); glVertex2f(stars[i][0], stars[i][1]); glEnd();
    }
    for (int i = 0; i < 8; i++) {
        float bx = stars[i * 5][0], by = stars[i * 5][1];
        GlowCircle(bx, by, 6, 1, 1, 0.85f, 0.08f * alpha, 0);
    }
    glPointSize(1);
}

//============================================================
//  DRAW: SUN
//============================================================
void drawSun()
{
    if (dayFactor < 0.3f) return;
    if (stormIntensity > 0.6f) return; // sun hidden in heavy storm
    float a = dayFactor * (1 - stormIntensity * 0.8f);
    float p = 1 + 0.04f * sin(glowPulse * 2);
    glPushMatrix(); applyTranslation(-380, 260);
    GlowCircle(0, 0, 120 * p, 1, 0.85f, 0.3f, 0.05f * a, 0);
    GlowCircle(0, 0, 85 * p, 1, 0.8f, 0.2f, 0.1f * a, 0);
    GlowCircle(0, 0, 60 * p, 1, 0.9f, 0.4f, 0.18f * a, 0);
    GlowCircle(0, 0, 45 * p, 1, 0.95f, 0.5f, 0.3f * a, 0);
    FilledCircle(0, 0, 32, 1, 0.92f, 0.3f, a);
    GlowCircle(0, 0, 32, 1, 1, 0.85f, 0.85f * a, 0);
    glColor4f(1, 0.7f, 0, 0.5f * a); glPointSize(2);
    MidpointCircle(0, 0, 32); glPointSize(1);
    for (int i = 0; i < 14; i++) {
        float ang = (i * 25.7f + glowPulse * 8) * PI / 180;
        float r1 = 38, r2 = 50 + 4 * sin(glowPulse * 3 + i);
        glColor4f(1, 0.85f, 0.2f, 0.4f * a);
        DDA_Line(r1 * cos(ang), r1 * sin(ang), r2 * cos(ang), r2 * sin(ang));
    }
    glPopMatrix();
}

//============================================================
//  DRAW: MOON
//============================================================
void drawMoon()
{
    if (dayFactor > 0.7f) return;
    if (stormIntensity > 0.5f) return; // moon hidden in storm
    float a = (1 - dayFactor) * (1 - stormIntensity);
    float p = 1 + 0.03f * sin(glowPulse * 1.5f);
    glPushMatrix(); applyTranslation(400, 275);
    GlowCircle(0, 0, 140 * p, 0.6f, 0.7f, 1, 0.03f * a, 0);
    GlowCircle(0, 0, 100 * p, 0.7f, 0.8f, 1, 0.06f * a, 0);
    GlowCircle(0, 0, 70 * p, 0.8f, 0.85f, 1, 0.1f * a, 0);
    GlowCircle(0, 0, 50 * p, 0.9f, 0.92f, 1, 0.18f * a, 0);
    GlowCircle(0, 0, 40 * p, 0.95f, 0.95f, 1, 0.3f * a, 0);
    FilledCircle(0, 0, 30, 0.92f, 0.92f, 0.85f, a);
    GlowCircle(0, 0, 30, 1, 1, 0.95f, 0.75f * a, 0.1f * a);
    FilledCircle(7, 9, 5, 0.82f, 0.82f, 0.78f, 0.4f * a);
    FilledCircle(-8, 3, 4, 0.82f, 0.82f, 0.78f, 0.35f * a);
    FilledCircle(4, -7, 4, 0.82f, 0.82f, 0.78f, 0.38f * a);
    FilledCircle(-3, -10, 3, 0.82f, 0.82f, 0.78f, 0.3f * a);
    FilledCircle(10, -2, 2.5f, 0.82f, 0.82f, 0.78f, 0.32f * a);
    glPopMatrix();
}

//============================================================
//  DRAW: LIGHTNING BOLT
//============================================================
void drawLightning()
{
    if (lightningFlash < 0.05f) return;

    // Draw a jagged bolt from sky
    float boltX = -200 + 400 * sin(glowPulse * 7.3f);
    float alpha = lightningFlash;

    glColor4f(1, 1, 0.9f, alpha);
    glLineWidth(3.0f);

    // Main bolt
    float x = boltX, y = 340;
    glBegin(GL_LINE_STRIP);
    glVertex2f(x, y);
    for (int i = 0; i < 8; i++) {
        x += (rand() % 30 - 15);
        y -= 40 + rand() % 20;
        glVertex2f(x, y);
    }
    glEnd();

    // Glow around bolt
    glColor4f(0.8f, 0.85f, 1, alpha * 0.3f);
    glLineWidth(8.0f);
    x = boltX; y = 340;
    glBegin(GL_LINE_STRIP);
    glVertex2f(x, y);
    srand((unsigned)(glowPulse * 100)); // consistent bolt shape per flash
    for (int i = 0; i < 8; i++) {
        x += (rand() % 30 - 15);
        y -= 40 + rand() % 20;
        glVertex2f(x, y);
    }
    glEnd();
    srand((unsigned)time(NULL));

    // Branch
    glColor4f(1, 1, 0.95f, alpha * 0.6f);
    glLineWidth(1.5f);
    float bx = boltX + 10, by = 200;
    glBegin(GL_LINE_STRIP);
    glVertex2f(bx, by);
    for (int i = 0; i < 4; i++) {
        bx += 12 + rand() % 10;
        by -= 25 + rand() % 15;
        glVertex2f(bx, by);
    }
    glEnd();

    glLineWidth(1);
}

//============================================================
//  DRAW: STORM CLOUDS (dark, heavy)
//============================================================
void drawStormCloud(float cx, float cy, float scale)
{
    if (stormIntensity < 0.05f) return;
    glPushMatrix(); applyTranslation(cx, cy); applyScaling(scale, scale);
    float si = stormIntensity;
    float bR = 0.15f * si, bG = 0.14f * si, bB = 0.18f * si;
    float alpha = si * 0.85f;

    // Bottom layer - dark and menacing
    FilledCircle(0, -8, 45, bR * 0.5f, bG * 0.5f, bB * 0.5f, alpha);
    FilledCircle(30, -6, 35, bR * 0.55f, bG * 0.52f, bB * 0.55f, alpha);
    FilledCircle(-30, -6, 35, bR * 0.52f, bG * 0.50f, bB * 0.52f, alpha);
    FilledCircle(15, -4, 40, bR * 0.58f, bG * 0.55f, bB * 0.58f, alpha);
    FilledCircle(-15, -5, 38, bR * 0.56f, bG * 0.53f, bB * 0.56f, alpha);

    // Mid layer
    FilledCircle(0, 4, 42, bR * 0.7f, bG * 0.68f, bB * 0.72f, alpha);
    FilledCircle(25, 6, 32, bR * 0.68f, bG * 0.65f, bB * 0.70f, alpha);
    FilledCircle(-25, 5, 32, bR * 0.66f, bG * 0.63f, bB * 0.68f, alpha);

    // Top layer
    FilledCircle(0, 14, 35, bR * 0.8f, bG * 0.78f, bB * 0.82f, alpha);
    FilledCircle(18, 12, 25, bR * 0.78f, bG * 0.75f, bB * 0.80f, alpha);
    FilledCircle(-18, 11, 25, bR * 0.76f, bG * 0.73f, bB * 0.78f, alpha);
    FilledCircle(35, 8, 20, bR * 0.72f, bG * 0.70f, bB * 0.75f, alpha);
    FilledCircle(-35, 7, 20, bR * 0.70f, bG * 0.68f, bB * 0.73f, alpha);

    glPopMatrix();
}

void drawStormClouds()
{
    if (stormIntensity < 0.05f) return;
    // Many overlapping storm clouds covering the sky
    drawStormCloud(-450, 285, 1.3f);
    drawStormCloud(-250, 295, 1.5f);
    drawStormCloud(-50, 280, 1.4f);
    drawStormCloud(150, 290, 1.6f);
    drawStormCloud(350, 285, 1.3f);
    drawStormCloud(500, 295, 1.2f);
    // Lower layer for extra thickness
    drawStormCloud(-350, 260, 1.1f);
    drawStormCloud(-100, 255, 1.2f);
    drawStormCloud(100, 258, 1.0f);
    drawStormCloud(300, 262, 1.15f);
    drawStormCloud(450, 250, 1.05f);
}

//============================================================
//  DRAW: AIRPLANE
//============================================================
void drawAirplane(float x, float y, float sc, bool goRight)
{
    if (stormIntensity > 0.4f) return; // plane avoids storm

    glPushMatrix();
    applyTranslation(x, y);
    applyScaling(sc, sc);
    if (!goRight) applyReflectionY();

    float f = dayFactor;
    float bodyR = 0.85f * f + 0.4f * (1 - f);
    float bodyG = 0.85f * f + 0.4f * (1 - f);
    float bodyB = 0.88f * f + 0.5f * (1 - f);
    float alpha = 0.85f;

    // Contrail
    glColor4f(1, 1, 1, 0.08f * f + 0.04f * (1 - f));
    for (int i = 1; i <= 8; i++) {
        float trailX = -20 - i * 12;
        float trailR = 2 + i * 1.5f;
        FilledCircle(trailX, -1 + sin(i * 0.8f + glowPulse) * 0.5f, trailR,
            1, 1, 1, (0.12f - i * 0.012f) * f + 0.03f * (1 - f));
    }

    // Fuselage
    FilledEllipse(0, 0, 22, 5, bodyR, bodyG, bodyB, alpha);
    FilledEllipse(0, 1, 16, 1.5f, 0.55f * f + 0.25f, 0.72f * f + 0.3f, 0.92f * f + 0.4f, 0.5f * alpha);

    // Cockpit
    FilledEllipse(20, 0, 6, 4, bodyR * 0.9f, bodyG * 0.9f, bodyB * 0.9f, alpha);
    FilledEllipse(22, 1, 3, 2, 0.4f * f + 0.2f, 0.6f * f + 0.3f, 0.9f * f + 0.4f, 0.7f);

    // Main wings
    glColor4f(bodyR * 0.95f, bodyG * 0.95f, bodyB * 0.95f, alpha);
    glBegin(GL_TRIANGLES);
    glVertex2f(-2, 5); glVertex2f(6, 5); glVertex2f(-8, 22);
    glEnd();
    glBegin(GL_TRIANGLES);
    glVertex2f(-2, -5); glVertex2f(6, -5); glVertex2f(-8, -22);
    glEnd();

    // Wing tips
    FilledCircle(-8, 22, 1.5f, 0.9f, 0.15f, 0.1f, 0.8f);
    FilledCircle(-8, -22, 1.5f, 0.1f, 0.8f, 0.15f, 0.8f);

    // Tail fin
    glColor4f(bodyR * 0.85f, bodyG * 0.85f, bodyB * 0.85f, alpha);
    glBegin(GL_TRIANGLES);
    glVertex2f(-18, 5); glVertex2f(-22, 5); glVertex2f(-22, 16);
    glEnd();
    glColor4f(0.85f, 0.15f, 0.12f, 0.7f);
    glBegin(GL_TRIANGLES);
    glVertex2f(-20, 8); glVertex2f(-22, 8); glVertex2f(-22, 15);
    glEnd();

    // Horizontal tail
    glColor4f(bodyR * 0.9f, bodyG * 0.9f, bodyB * 0.9f, alpha);
    glBegin(GL_TRIANGLES);
    glVertex2f(-17, 3); glVertex2f(-20, 3); glVertex2f(-22, 10);
    glEnd();
    glBegin(GL_TRIANGLES);
    glVertex2f(-17, -3); glVertex2f(-20, -3); glVertex2f(-22, -10);
    glEnd();

    // Engine pods
    FilledEllipse(-2, 10, 4, 2.2f, 0.5f * f + 0.2f, 0.5f * f + 0.2f, 0.55f * f + 0.25f, alpha * 0.8f);
    FilledEllipse(-2, -10, 4, 2.2f, 0.5f * f + 0.2f, 0.5f * f + 0.2f, 0.55f * f + 0.25f, alpha * 0.8f);

    // Nav light
    float blink = 0.5f + 0.5f * sin(glowPulse * 6);
    if (blink > 0.7f) {
        FilledCircle(-22, 5, 1.5f, 1, 0.2f, 0.1f, 0.9f);
        GlowCircle(-22, 5, 5, 1, 0.2f, 0.1f, 0.2f, 0);
    }

    glPopMatrix();
}

//============================================================
//  DRAW: FIREFLIES
//============================================================
void drawFireflies()
{
    if (dayFactor > 0.5f) return;
    if (stormIntensity > 0.3f) return; // fireflies hide in storm
    float a = 1 - dayFactor;
    for (int i = 0; i < 20; i++) {
        float br = 0.5f + 0.5f * sin(fireflies[i][2]);
        GlowCircle(fireflies[i][0], fireflies[i][1], 12, 1, 1, 0.4f, 0.08f * a * br, 0);
        GlowCircle(fireflies[i][0], fireflies[i][1], 6, 1, 1, 0.5f, 0.15f * a * br, 0);
        FilledCircle(fireflies[i][0], fireflies[i][1], 1.5f, 1, 1, 0.5f, 0.8f * a * br);
    }
}

//============================================================
//  DRAW: SOLID CLOUD
//============================================================
void drawCloud(float cx, float cy, float scale)
{
    if (stormIntensity > 0.7f) return; // replaced by storm clouds
    glPushMatrix(); applyTranslation(cx, cy); applyScaling(scale, scale);
    float bR, bG, bB, sR, sG, sB;
    float sd = stormIntensity * 0.5f;
    if (isNight) {
        bR = 0.18f - sd * 0.1f; bG = 0.18f - sd * 0.1f; bB = 0.22f - sd * 0.08f;
        sR = 0.12f - sd * 0.06f; sG = 0.12f - sd * 0.06f; sB = 0.15f - sd * 0.05f;
    }
    else {
        bR = 0.94f - sd * 0.4f; bG = 0.94f - sd * 0.4f; bB = 0.96f - sd * 0.3f;
        sR = 0.78f - sd * 0.35f; sG = 0.80f - sd * 0.35f; sB = 0.83f - sd * 0.25f;
    }

    FilledCircle(4, -6, 30, sR, sG, sB);  FilledCircle(28, -5, 22, sR, sG, sB);
    FilledCircle(-24, -5, 20, sR, sG, sB); FilledCircle(14, -2, 26, sR, sG, sB);
    FilledCircle(-12, -3, 24, sR, sG, sB);
    FilledCircle(0, 4, 30, bR, bG, bB);   FilledCircle(26, 5, 24, bR, bG, bB);
    FilledCircle(-26, 5, 22, bR, bG, bB); FilledCircle(14, 16, 24, bR, bG, bB);
    FilledCircle(-14, 15, 22, bR, bG, bB); FilledCircle(0, 20, 22, bR, bG, bB);
    FilledCircle(36, 10, 16, bR, bG, bB); FilledCircle(-36, 8, 15, bR, bG, bB);
    FilledCircle(8, 8, 28, bR, bG, bB);   FilledCircle(-8, 7, 26, bR, bG, bB);
    if (!isNight && stormIntensity < 0.3f) {
        FilledCircle(0, 22, 14, 1, 1, 1);
        FilledCircle(12, 20, 10, 1, 1, 1);
    }
    glPopMatrix();
}

void drawClouds() {
    drawCloud(cloud1X, 245, 1.0f); drawCloud(cloud2X, 280, 0.65f);
    drawCloud(cloud3X, 258, 1.1f); drawCloud(cloud4X, 290, 0.55f);
}

//============================================================
//  DRAW: MOUNTAINS
//============================================================
void drawMountains()
{
    float f = getEffectiveDayFactor();
    float lf = lightningFlash * 0.15f;

    // Far distant mountain
    float ffR = 0.38f * f + 0.06f + lf, ffG = 0.42f * f + 0.08f + lf, ffB = 0.58f * f + 0.14f + lf;
    glBegin(GL_TRIANGLE_FAN); glColor3f(ffR, ffG, ffB); glVertex2f(0, -50);
    for (int x = -600; x <= 600; x += 3) {
        float h = 100 * sin(x * 0.004f + 0.3f) + 60 * sin(x * 0.006f + 1.0f) + 35 * sin(x * 0.012f + 1.8f) + 65;
        float hFade = ffR + 0.04f * (h / 160.0f);
        glColor3f(hFade, ffG + 0.02f * (h / 160.0f), ffB + 0.03f * (h / 160.0f));
        glVertex2f(x, -50 + h);
    }
    glVertex2f(600, -50); glEnd();

    // Mid mountain
    float fR = 0.32f * f + 0.08f + lf, fG = 0.40f * f + 0.10f + lf, fB = 0.52f * f + 0.14f + lf;
    glBegin(GL_TRIANGLE_FAN); glColor3f(fR, fG, fB); glVertex2f(0, -50);
    for (int x = -600; x <= 600; x += 3) {
        float h = 80 * sin(x * 0.005f + 0.5f) + 50 * sin(x * 0.008f + 1.2f) + 30 * sin(x * 0.015f + 2) + 40;
        glVertex2f(x, -50 + h);
    }
    glVertex2f(600, -50); glEnd();

    // Snow caps
    if (f > 0.3f) {
        for (int x = -600; x <= 600; x += 3) {
            float h = 80 * sin(x * 0.005f + 0.5f) + 50 * sin(x * 0.008f + 1.2f) + 30 * sin(x * 0.015f + 2) + 40;
            if (h > 110) {
                float snowAlpha = (h - 110) / 50.0f * 0.3f * f;
                glColor4f(1, 1, 1, snowAlpha);
                glBegin(GL_POINTS); glVertex2f(x, -50 + h); glEnd();
            }
        }
    }

    // Near foothills
    float nR = 0.18f * f + 0.05f + lf, nG = 0.38f * f + 0.10f + lf, nB = 0.15f * f + 0.04f + lf;
    glBegin(GL_TRIANGLE_FAN); glColor3f(nR, nG, nB); glVertex2f(0, -50);
    for (int x = -600; x <= 600; x += 3) {
        float h = 40 * sin(x * 0.007f + 3) + 25 * sin(x * 0.012f + 1.5f) + 15 * sin(x * 0.02f + 4) + 20;
        glColor3f(nR + 0.05f, nG + 0.08f, nB + 0.03f);
        glVertex2f(x, -50 + h);
    }
    glVertex2f(600, -50); glEnd();
}

//============================================================
//  DRAW: GROUND
//============================================================
void drawGround()
{
    float f = getEffectiveDayFactor();
    float lf = lightningFlash * 0.08f;
    float sR = 0.18f * f + 0.05f + lf, sG = 0.12f * f + 0.03f + lf, sB = 0.06f * f + 0.02f + lf;
    glBegin(GL_QUADS);
    glColor3f(sR, sG, sB); glVertex2f(-600, -350); glVertex2f(600, -350);
    glColor3f(sR * 1.2f, sG * 1.2f, sB * 1.2f); glVertex2f(600, -280); glVertex2f(-600, -280);
    glEnd();

    float g1R = 0.14f * f + 0.04f + lf, g1G = 0.38f * f + 0.09f + lf, g1B = 0.09f * f + 0.03f + lf;
    glBegin(GL_QUADS);
    glColor3f(g1R, g1G, g1B); glVertex2f(-600, -280); glVertex2f(600, -280);
    glColor3f(g1R + 0.06f, g1G + 0.10f, g1B + 0.04f);
    glVertex2f(600, -155); glVertex2f(-600, -155);
    glEnd();

    float g2R = 0.18f * f + 0.05f + lf, g2G = 0.48f * f + 0.12f + lf, g2B = 0.11f * f + 0.03f + lf;
    glBegin(GL_QUADS);
    glColor3f(g2R, g2G, g2B); glVertex2f(-600, -155); glVertex2f(600, -155);
    glColor3f(g2R + 0.05f, g2G + 0.08f, g2B + 0.03f);
    glVertex2f(600, -50); glVertex2f(-600, -50);
    glEnd();

    // Storm puddles
    if (stormIntensity > 0.3f) {
        float pa = stormIntensity * 0.15f;
        FilledEllipse(-300, -220, 35, 8, 0.15f, 0.25f, 0.40f, pa);
        FilledEllipse(200, -235, 28, 6, 0.15f, 0.25f, 0.40f, pa * 0.8f);
        FilledEllipse(-100, -260, 22, 5, 0.15f, 0.25f, 0.40f, pa * 0.6f);
        FilledEllipse(350, -250, 30, 7, 0.15f, 0.25f, 0.40f, pa * 0.7f);
    }

    // Grass blades (bend more in storm)
    float windBend = stormIntensity * 8.0f;
    for (int x = -595; x < 595; x += 5) {
        float h = 6 + 4 * sin(x * 0.12f + flowerSway * 0.4f);
        float baseY = -155 + 2 * sin(x * 0.05f);
        float tipOffX = windBend * sin(glowPulse * 3 + x * 0.02f);
        glColor4f(0.08f * f + 0.02f, 0.40f * f + 0.08f, 0.06f * f + 0.02f, 0.55f);
        DDA_Line(x, baseY, x + 1 + tipOffX, baseY + h);
    }

    // Ground texture
    for (int i = 0; i < 40; i++) {
        float sx = -580 + (i * 29.3f);
        float sy = -285 + 15 * sin(i * 1.7f);
        float ssz = 1.5f + sin(i * 2.1f);
        FilledCircle(sx, sy, ssz,
            0.22f * f + 0.06f, 0.17f * f + 0.04f, 0.10f * f + 0.03f, 0.3f);
    }
}

//============================================================
//  DRAW: REALISTIC ROAD
//============================================================
void drawPath()
{
    float f = getEffectiveDayFactor();
    float pR = 0.45f * f + 0.12f, pG = 0.35f * f + 0.09f, pB = 0.22f * f + 0.06f;
    float lf = lightningFlash * 0.06f;
    pR += lf; pG += lf; pB += lf;

    // Upper road section
    glBegin(GL_QUADS);
    glColor3f(pR * 0.70f, pG * 0.70f, pB * 0.70f);
    glVertex2f(-12, -50);  glVertex2f(12, -50);
    glColor3f(pR * 0.78f, pG * 0.78f, pB * 0.78f);
    glVertex2f(42, -155);  glVertex2f(-42, -155);
    glEnd();

    glBegin(GL_QUADS);
    glColor3f(pR * 0.82f, pG * 0.82f, pB * 0.82f);
    glVertex2f(-8, -50); glVertex2f(8, -50);
    glColor3f(pR * 0.92f, pG * 0.92f, pB * 0.92f);
    glVertex2f(32, -155); glVertex2f(-32, -155);
    glEnd();

    glColor3f(pR * 0.35f, pG * 0.35f, pB * 0.35f);
    glPointSize(2.0f);
    DDA_Line(-12, -50, -42, -155);
    DDA_Line(12, -50, 42, -155);
    glPointSize(1);

    // Grass tufts along upper road
    glColor4f(0.10f * f + 0.03f, 0.44f * f + 0.10f, 0.06f * f + 0.02f, 0.5f);
    for (int y = -60; y >= -150; y -= 10) {
        float t = (-y - 50.0f) / (155.0f - 50.0f);
        float rw = 12 + (42 - 12) * t;
        DDA_Line(-rw - 3, y, -rw - 1, y + 4);
        DDA_Line(-rw - 5, y, -rw - 2, y + 5);
        DDA_Line(rw + 3, y, rw + 1, y + 4);
        DDA_Line(rw + 5, y, rw + 2, y + 5);
    }

    // Stones on upper road
    for (int row = 0; row < 6; row++) {
        float y = -60 - row * 16;
        float t = (-y - 50.0f) / (155.0f - 50.0f);
        float roadW = 12 + (42 - 12) * t;
        for (int col = -1; col <= 1; col++) {
            float cx = col * (roadW * 0.35f);
            if (fabs(cx) < roadW - 6) {
                float shade = 0.85f + 0.15f * sin(cx * 0.3f + y * 0.2f);
                FilledEllipse(cx, y, 5 * (1 - t * 0.4f), 3.5f * (1 - t * 0.4f),
                    pR * shade, pG * shade, pB * shade, 0.6f);
            }
        }
    }

    // Lower road section
    glBegin(GL_QUADS);
    glColor3f(pR * 0.75f, pG * 0.75f, pB * 0.75f);
    glVertex2f(-42, -155); glVertex2f(42, -155);
    glColor3f(pR * 0.85f, pG * 0.85f, pB * 0.85f);
    glVertex2f(52, -350); glVertex2f(-52, -350);
    glEnd();

    glBegin(GL_QUADS);
    glColor3f(pR * 0.90f, pG * 0.90f, pB * 0.90f);
    glVertex2f(-32, -155); glVertex2f(32, -155);
    glColor3f(pR, pG, pB);
    glVertex2f(40, -350); glVertex2f(-40, -350);
    glEnd();

    glColor3f(pR * 0.4f, pG * 0.4f, pB * 0.4f);
    glPointSize(2.5f);
    DDA_Line(-42, -155, -52, -350);
    DDA_Line(42, -155, 52, -350);
    glPointSize(1);

    // Cobblestone
    for (int row = 0; row < 12; row++) {
        float y = -165 - row * 16;
        float roadWidthAtY = 42 + (52 - 42) * ((-y - 155.0f) / (350.0f - 155.0f));
        int offset = (row % 2) ? 8 : 0;
        for (int col = -2; col <= 2; col++) {
            float cx = col * 16 + offset;
            if (fabs(cx) < roadWidthAtY - 8) {
                float shade = 0.85f + 0.15f * sin(cx * 0.3f + y * 0.2f);
                FilledEllipse(cx + 1, y - 1, 7, 5, pR * 0.5f, pG * 0.5f, pB * 0.5f, 0.2f);
                FilledEllipse(cx, y, 7, 5, pR * shade, pG * shade, pB * shade);
                FilledEllipse(cx - 1, y + 1, 4, 2.5f,
                    pR * shade * 1.15f, pG * shade * 1.1f, pB * shade * 1.05f, 0.3f);
            }
        }
    }

    // Road edge grass tufts lower
    glColor4f(0.10f * f + 0.03f, 0.42f * f + 0.10f, 0.06f * f + 0.02f, 0.6f);
    for (int y = -160; y >= -340; y -= 12) {
        float rw = 42 + (52 - 42) * ((-y - 155.0f) / (350.0f - 155.0f));
        DDA_Line(-rw - 3, y, -rw - 1, y + 5);
        DDA_Line(-rw - 6, y, -rw - 3, y + 6);
        DDA_Line(rw + 3, y, rw + 1, y + 5);
        DDA_Line(rw + 6, y, rw + 3, y + 6);
    }

    // Wet road in storm
    if (stormIntensity > 0.2f) {
        float wet = stormIntensity * 0.12f;
        float shimmer = sin(waterShimmer * 2) * 0.03f;
        glColor4f(0.3f + shimmer, 0.35f + shimmer, 0.5f + shimmer, wet);
        glBegin(GL_QUADS);
        glVertex2f(-42, -155); glVertex2f(42, -155);
        glVertex2f(52, -350); glVertex2f(-52, -350);
        glEnd();
    }
}

//============================================================
//  DRAW: FISH
//============================================================
void drawFish(float x, float y, float scale, float dir,
    float r, float g, float b)
{
    glPushMatrix();
    applyTranslation(x, y); applyScaling(scale * dir, scale);
    float f = getEffectiveDayFactor();
    float minBright = 0.35f;
    float fishR = r * f + r * minBright * (1 - f);
    float fishG = g * f + g * minBright * (1 - f);
    float fishB = b * f + b * minBright * (1 - f);
    float alpha = 0.7f * f + 0.55f * (1 - f);

    FilledEllipse(0, 0, 13, 5.5f, fishR, fishG, fishB, alpha);
    FilledEllipse(0, 0.5f, 10, 2, fishR * 1.15f, fishG * 1.1f, fishB * 1.05f, alpha * 0.4f);

    glColor4f(fishR * 0.8f, fishG * 0.8f, fishB * 0.8f, alpha);
    glBegin(GL_TRIANGLES);
    glVertex2f(-13, 0); glVertex2f(-20, 5); glVertex2f(-20, -5);
    glEnd();

    glColor4f(fishR * 0.7f, fishG * 0.7f, fishB * 0.7f, alpha * 0.7f);
    glBegin(GL_TRIANGLES);
    glVertex2f(-1, 5.5f); glVertex2f(5, 5.5f); glVertex2f(2, 10);
    glEnd();

    glColor4f(fishR * 0.65f, fishG * 0.65f, fishB * 0.65f, alpha * 0.6f);
    glBegin(GL_TRIANGLES);
    glVertex2f(1, -5.5f); glVertex2f(5, -5.5f); glVertex2f(3, -9);
    glEnd();

    FilledCircle(7, 1.5f, 2.2f, 1, 1, 1, alpha);
    FilledCircle(7.5f, 1.5f, 1.0f, 0, 0, 0, alpha);

    glPopMatrix();
}

//============================================================
//  DRAW: SMALL GROUND FLOWER
//============================================================
void drawSmallFlower(float x, float y,
    float pR, float pG, float pB, float sc)
{
    glPushMatrix(); applyTranslation(x, y); applyScaling(sc, sc);
    float f = getEffectiveDayFactor();
    float windBend = stormIntensity * 4.0f * sin(glowPulse * 2 + x * 0.1f);

    glColor3f(0.10f * f + 0.03f, 0.40f * f + 0.08f, 0.06f * f + 0.02f);
    glLineWidth(1.5f);
    glBegin(GL_LINES); glVertex2f(0, 0); glVertex2f(windBend * 0.3f, 8); glEnd();
    glLineWidth(1);

    FilledEllipse(7, 14, 5, 2.5f, 0.10f * f + 0.03f, 0.40f * f + 0.08f, 0.06f * f + 0.02f);
    glTranslatef(windBend * 0.3f, 0, 0);
    for (int i = 0; i < 5; i++) {
        glPushMatrix(); applyRotation(i * 72 + flowerSway * 2);
        FilledEllipse(0, 4, 2, 4, pR, pG, pB, 0.85f);
        glPopMatrix();
    }
    FilledCircle(0, 0, 2, 1, 0.85f, 0.1f, 0.8f);

    glPopMatrix();
}

//============================================================
//  DRAW: TINY WILDFLOWER
//============================================================
void drawTinyFlower(float x, float y, float r, float g, float b, float sc)
{
    glPushMatrix(); applyTranslation(x, y); applyScaling(sc, sc);
    float f = getEffectiveDayFactor();
    float windBend = stormIntensity * 3.0f * sin(glowPulse * 2.5f + x * 0.1f);

    glColor4f(0.10f * f + 0.03f, 0.38f * f + 0.08f, 0.06f * f + 0.02f, 0.7f);
    glLineWidth(1.0f);
    glBegin(GL_LINES); glVertex2f(0, 0); glVertex2f(windBend * 0.2f, 5); glEnd();
    glLineWidth(1);

    glTranslatef(windBend * 0.2f, 0, 0);
    for (int i = 0; i < 4; i++) {
        glPushMatrix(); applyRotation(i * 90 + flowerSway * 3);
        FilledEllipse(0, 2.5f, 1.2f, 2.5f, r, g, b, 0.8f);
        glPopMatrix();
    }
    FilledCircle(0, 0, 1.2f, 1, 0.9f, 0.2f, 0.7f);

    glPopMatrix();
}

//============================================================
//  DRAW: POND
//============================================================
void drawPond()
{
    float pondX = -400, pondY = -238;
    float f = getEffectiveDayFactor();
    float lf = lightningFlash * 0.05f;

    // Mud edge
    FilledEllipse(pondX, pondY, 118, 52,
        0.25f * f + 0.08f, 0.18f * f + 0.05f, 0.08f * f + 0.03f);

    // Rocky edge
    for (int i = 0; i < 24; i++) {
        float angle = i * 15.0f * PI / 180.0f;
        float rx = pondX + 112 * cos(angle);
        float ry = pondY + 47 * sin(angle);
        float sz = 3 + 2.5f * sin(i * 2.3f);
        FilledCircle(rx, ry, sz,
            0.35f * f + 0.10f, 0.28f * f + 0.08f, 0.18f * f + 0.05f, 0.6f);
    }

    // Water body
    float wR = 0.08f * f + 0.03f + lf, wG = 0.28f * f + 0.06f + lf, wB = 0.52f * f + 0.15f + lf;
    // Storm makes water darker and choppier
    if (stormIntensity > 0.1f) {
        wR *= (1 - stormIntensity * 0.3f);
        wG *= (1 - stormIntensity * 0.2f);
    }
    FilledEllipse(pondX, pondY, 108, 45, wR, wG, wB);
    FilledEllipse(pondX, pondY, 70, 28, wR * 0.7f, wG * 0.7f, wB * 0.85f, 0.4f);

    // Water shimmer (more agitated in storm)
    float agitation = 1 + stormIntensity * 3;
    float sh = sin(waterShimmer * agitation) * (0.06f + stormIntensity * 0.08f);
    GlowCircle(pondX, pondY, 65,
        wR + 0.08f + sh, wG + 0.08f + sh, wB + 0.05f, 0.18f, 0);

    // Ripples
    float rippleExtra = stormIntensity * 5;
    FilledEllipse(pondX + 10, pondY - 5,
        25 + 3 * sin(waterShimmer) + rippleExtra, 10 + sin(waterShimmer) + rippleExtra * 0.3f,
        wR + 0.10f, wG + 0.10f, wB + 0.07f, 0.08f + stormIntensity * 0.04f);
    FilledEllipse(pondX - 20, pondY + 8,
        18 + 2 * sin(waterShimmer + 1.5f) + rippleExtra * 0.7f, 7 + 0.8f * sin(waterShimmer + 1.5f),
        wR + 0.08f, wG + 0.08f, wB + 0.06f, 0.06f + stormIntensity * 0.03f);

    // Storm rain drops hitting water
    if (stormIntensity > 0.3f) {
        for (int i = 0; i < 8; i++) {
            float rx = pondX - 60 + (i * 17 + (int)(glowPulse * 30 + i * 37) % 120);
            float ry = pondY - 20 + (int)(glowPulse * 20 + i * 23) % 40;
            float dist = sqrt((rx - pondX) * (rx - pondX) / (108 * 108) + (ry - pondY) * (ry - pondY) / (45 * 45));
            if (dist < 0.9f) {
                float splashR = 3 + 2 * sin(glowPulse * 5 + i * 2);
                FilledEllipse(rx, ry, splashR, splashR * 0.4f,
                    wR + 0.15f, wG + 0.15f, wB + 0.10f, 0.12f * stormIntensity);
            }
        }
    }

    // Moon reflection
    if (f < 0.5f && stormIntensity < 0.3f) {
        float moonA = (1 - dayFactor) * 0.12f * (1 - stormIntensity);
        GlowCircle(pondX + 15, pondY, 20, 0.8f, 0.85f, 1, moonA, 0);
        FilledCircle(pondX + 15, pondY, 6, 0.9f, 0.9f, 0.85f, moonA * 2);
    }

    // Tree reflection
    glPushMatrix();
    applyTranslation(pondX - 25, pondY + 5);
    applyReflectionX(); applyScaling(0.18f, 0.12f);
    glColor4f(0.12f, 0.07f, 0.02f, 0.12f);
    glBegin(GL_QUADS);
    glVertex2f(-4, 0); glVertex2f(4, 0); glVertex2f(4, 35); glVertex2f(-4, 35);
    glEnd();
    FilledCircle(0, 42, 18, 0.04f, 0.15f, 0.03f, 0.10f);
    glPopMatrix();

    // Lily pads
    FilledEllipse(pondX - 30, pondY + 10, 13, 7, 0.08f, 0.38f, 0.06f);
    FilledEllipse(pondX + 35, pondY - 8, 10, 5, 0.10f, 0.40f, 0.07f);
    FilledEllipse(pondX - 8, pondY - 16, 9, 5, 0.09f, 0.37f, 0.05f);
    FilledEllipse(pondX + 18, pondY + 14, 8, 4.5f, 0.10f, 0.39f, 0.06f);
    FilledEllipse(pondX - 45, pondY - 3, 7, 4, 0.08f, 0.36f, 0.05f);
    FilledEllipse(pondX + 50, pondY + 5, 8, 4, 0.09f, 0.37f, 0.06f);

    // Fish
    drawFish(pondX + fish1X, pondY - 3, 0.75f, fish1Dir, 1, 0.5f, 0.1f);
    drawFish(pondX + fish2X, pondY + 8, 0.55f, fish2Dir, 0.9f, 0.25f, 0.2f);
    drawFish(pondX + fish3X, pondY - 10, 0.65f, fish3Dir, 1, 0.7f, 0.2f);

    // Ground flowers around pond
    drawSmallFlower(pondX - 120, pondY + 20, 0.9f, 0.4f, 0.9f, 0.7f);
    drawSmallFlower(pondX - 112, pondY + 38, 1.0f, 0.8f, 0.2f, 0.65f);
    drawSmallFlower(pondX - 105, pondY - 32, 0.4f, 0.4f, 1.0f, 0.7f);
    drawSmallFlower(pondX - 98, pondY + 45, 0.9f, 0.2f, 0.3f, 0.6f);
    drawSmallFlower(pondX + 110, pondY + 18, 1.0f, 0.6f, 0.1f, 0.7f);
    drawSmallFlower(pondX + 105, pondY - 28, 0.8f, 0.3f, 0.8f, 0.65f);
    drawSmallFlower(pondX + 100, pondY + 40, 0.5f, 0.8f, 0.3f, 0.7f);
    drawSmallFlower(pondX + 115, pondY - 15, 1.0f, 0.4f, 0.5f, 0.6f);
    drawSmallFlower(pondX - 65, pondY + 50, 0.9f, 0.9f, 0.2f, 0.65f);
    drawSmallFlower(pondX + 55, pondY + 48, 0.4f, 0.7f, 0.9f, 0.7f);
    drawSmallFlower(pondX - 25, pondY + 50, 0.8f, 0.2f, 0.6f, 0.65f);
    drawSmallFlower(pondX + 30, pondY - 45, 0.9f, 0.5f, 0.1f, 0.6f);
    drawSmallFlower(pondX - 85, pondY - 40, 0.3f, 0.8f, 0.5f, 0.7f);
    drawSmallFlower(pondX + 80, pondY - 38, 1.0f, 0.3f, 0.7f, 0.65f);

    // Tiny wildflowers
    drawTinyFlower(pondX - 130, pondY + 10, 1.0f, 0.5f, 0.8f, 0.6f);
    drawTinyFlower(pondX - 125, pondY - 18, 0.9f, 0.9f, 0.3f, 0.55f);
    drawTinyFlower(pondX + 125, pondY + 5, 0.6f, 0.4f, 1.0f, 0.6f);
    drawTinyFlower(pondX + 120, pondY - 22, 1.0f, 0.3f, 0.4f, 0.55f);
    drawTinyFlower(pondX - 90, pondY + 52, 0.8f, 0.6f, 0.9f, 0.5f);
    drawTinyFlower(pondX + 90, pondY + 50, 1.0f, 0.7f, 0.3f, 0.5f);
    drawTinyFlower(pondX - 50, pondY - 48, 0.5f, 0.9f, 0.5f, 0.55f);
    drawTinyFlower(pondX + 45, pondY - 48, 0.9f, 0.2f, 0.8f, 0.5f);
    drawTinyFlower(pondX - 10, pondY + 52, 1.0f, 0.4f, 0.2f, 0.6f);
    drawTinyFlower(pondX + 15, pondY + 50, 0.7f, 0.8f, 1.0f, 0.55f);
    drawTinyFlower(pondX - 75, pondY + 48, 0.95f, 0.35f, 0.6f, 0.5f);
    drawTinyFlower(pondX + 70, pondY + 46, 0.4f, 0.9f, 0.4f, 0.55f);

    // Cattails
    float reedColor = 0.08f * f + 0.03f;
    for (int i = 0; i < 5; i++) {
        float rx = pondX - 100 + i * 8;
        float ry = pondY + 40 + i * 3;
        glColor3f(reedColor, 0.30f * f + 0.08f, reedColor);
        glLineWidth(1.8f);
        glBegin(GL_LINES);
        glVertex2f(rx, ry); glVertex2f(rx + 1, ry + 22 + i * 2);
        glEnd(); glLineWidth(1);
        FilledEllipse(rx + 1, ry + 24 + i * 2, 2, 5,
            0.30f * f + 0.08f, 0.18f * f + 0.05f, 0.06f * f + 0.02f, 0.8f);
    }
    for (int i = 0; i < 4; i++) {
        float rx = pondX + 90 + i * 7;
        float ry = pondY + 35 + i * 2;
        glColor3f(reedColor, 0.30f * f + 0.08f, reedColor);
        glLineWidth(1.8f);
        glBegin(GL_LINES);
        glVertex2f(rx, ry); glVertex2f(rx - 1, ry + 20 + i * 2);
        glEnd(); glLineWidth(1);
        FilledEllipse(rx - 1, ry + 22 + i * 2, 2, 4.5f,
            0.30f * f + 0.08f, 0.18f * f + 0.05f, 0.06f * f + 0.02f, 0.8f);
    }
}

//============================================================
//  DRAW: FENCE
//============================================================
void drawFence()
{
    float f = getEffectiveDayFactor();
    float lf = lightningFlash * 0.05f;
    float wR = 0.48f * f + 0.14f + lf, wG = 0.30f * f + 0.09f + lf, wB = 0.13f * f + 0.04f + lf;
    float dR = wR * 0.7f, dG = wG * 0.7f, dB = wB * 0.7f;
    float lR = wR * 1.2f, lG = wG * 1.15f, lB = wB * 1.1f;

    float fBot = -300, fTop = -215, fPt = -202;
    float rY1 = -238, rY2 = -272;

    // Left fence
    for (int x = -580; x <= -80; x += 35) {
        glBegin(GL_QUADS);
        glColor3f(dR, dG, dB); glVertex2f(x, fBot);
        glColor3f(wR, wG, wB); glVertex2f(x + 16, fBot);
        glColor3f(lR, lG, lB); glVertex2f(x + 16, fTop);
        glColor3f(wR, wG, wB); glVertex2f(x, fTop); glEnd();
        glBegin(GL_TRIANGLES); glColor3f(wR, wG, wB); glVertex2f(x, fTop);
        glVertex2f(x + 16, fTop); glColor3f(lR, lG, lB); glVertex2f(x + 8, fPt); glEnd();
        glColor4f(dR * 0.8f, dG * 0.8f, dB * 0.8f, 0.3f);
        Bresenham_Line(x + 4, (int)fBot + 2, x + 4, (int)fTop - 2);
        Bresenham_Line(x + 10, (int)fBot + 2, x + 10, (int)fTop - 2);
        FilledCircle(x + 8, rY1, 2, dR * 0.5f, dG * 0.5f, dB * 0.5f);
        FilledCircle(x + 8, rY2, 2, dR * 0.5f, dG * 0.5f, dB * 0.5f);
    }
    for (int rail = 0; rail < 2; rail++) {
        float ry = (rail == 0) ? rY1 : rY2;
        glBegin(GL_QUADS); glColor3f(dR, dG, dB); glVertex2f(-585, ry);
        glColor3f(wR, wG, wB); glVertex2f(-75, ry); glVertex2f(-75, ry + 7);
        glColor3f(dR, dG, dB); glVertex2f(-585, ry + 7); glEnd();
        glColor4f(dR * 0.6f, dG * 0.6f, dB * 0.6f, 0.4f); glPointSize(1.5f);
        DDA_Line(-585, ry, -75, ry); DDA_Line(-585, ry + 7, -75, ry + 7); glPointSize(1);
    }

    // Right fence
    for (int x = 80; x <= 580; x += 35) {
        glBegin(GL_QUADS);
        glColor3f(dR, dG, dB); glVertex2f(x, fBot);
        glColor3f(wR, wG, wB); glVertex2f(x + 16, fBot);
        glColor3f(lR, lG, lB); glVertex2f(x + 16, fTop);
        glColor3f(wR, wG, wB); glVertex2f(x, fTop); glEnd();
        glBegin(GL_TRIANGLES); glColor3f(wR, wG, wB); glVertex2f(x, fTop);
        glVertex2f(x + 16, fTop); glColor3f(lR, lG, lB); glVertex2f(x + 8, fPt); glEnd();
        glColor4f(dR * 0.8f, dG * 0.8f, dB * 0.8f, 0.3f);
        Bresenham_Line(x + 4, (int)fBot + 2, x + 4, (int)fTop - 2);
        Bresenham_Line(x + 10, (int)fBot + 2, x + 10, (int)fTop - 2);
        FilledCircle(x + 8, rY1, 2, dR * 0.5f, dG * 0.5f, dB * 0.5f);
        FilledCircle(x + 8, rY2, 2, dR * 0.5f, dG * 0.5f, dB * 0.5f);
    }
    for (int rail = 0; rail < 2; rail++) {
        float ry = (rail == 0) ? rY1 : rY2;
        glBegin(GL_QUADS); glColor3f(dR, dG, dB); glVertex2f(75, ry);
        glColor3f(wR, wG, wB); glVertex2f(585, ry); glVertex2f(585, ry + 7);
        glColor3f(dR, dG, dB); glVertex2f(75, ry + 7); glEnd();
        glColor4f(dR * 0.6f, dG * 0.6f, dB * 0.6f, 0.4f); glPointSize(1.5f);
        DDA_Line(75, ry, 585, ry); DDA_Line(75, ry + 7, 585, ry + 7); glPointSize(1);
    }
}

//============================================================
//  DRAW: REALISTIC TREE 
//============================================================
void drawTree(float x, float y, float scale)
{
    glPushMatrix(); applyTranslation(x, y); applyScaling(scale, scale);
    float f = getEffectiveDayFactor();
    float lf = lightningFlash * 0.04f;
    float tR = 0.30f * f + 0.08f + lf, tG = 0.18f * f + 0.05f + lf, tB = 0.07f * f + 0.02f + lf;
    float lR = 0.12f * f + 0.03f + lf, lG = 0.42f * f + 0.10f + lf, lB = 0.08f * f + 0.02f + lf;

    // Wind effect on canopy
    float windOff = stormIntensity * 6.0f * sin(glowPulse * 2.5f + x * 0.01f);

    // Ground shadow
    FilledEllipse(12, -4, 40 + stormIntensity * 5, 8, 0, 0, 0, 0.10f * (1 - stormIntensity * 0.5f));

    // Root flares
    glColor3f(tR * 0.65f, tG * 0.65f, tB * 0.65f);
    glBegin(GL_TRIANGLES);
    glVertex2f(-14, 0); glVertex2f(-10, 0); glVertex2f(-11, 12); glEnd();
    glBegin(GL_TRIANGLES);
    glVertex2f(14, 0); glVertex2f(10, 0); glVertex2f(11, 12); glEnd();
    glBegin(GL_TRIANGLES);
    glVertex2f(-17, 2); glVertex2f(-12, 4); glVertex2f(-9, 10); glEnd();
    glBegin(GL_TRIANGLES);
    glVertex2f(17, 2); glVertex2f(12, 4); glVertex2f(9, 10); glEnd();

    // Exposed roots on ground surface
    glColor3f(tR * 0.55f, tG * 0.55f, tB * 0.55f);
    glLineWidth(2.5f);
    glBegin(GL_LINES); glVertex2f(-14, 2); glVertex2f(-25, -2); glEnd();
    glBegin(GL_LINES); glVertex2f(14, 2); glVertex2f(22, -1); glEnd();
    glLineWidth(1.5f);
    glBegin(GL_LINES); glVertex2f(-17, 3); glVertex2f(-28, 0); glEnd();
    glBegin(GL_LINES); glVertex2f(17, 3); glVertex2f(26, 1); glEnd();
    glLineWidth(1);

    // Main trunk with taper, slight curve and realistic width
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= 12; i++) {
        float t = i / 12.0f;
        float trunkY = t * 82;
        float widthBase = 12, widthTop = 4.5f;
        float w = widthBase + (widthTop - widthBase) * t;
        float curve = 1.8f * sin(t * PI * 0.5f) + windOff * t * 0.15f;
        float shade = 0.65f + 0.35f * t;
        glColor3f(tR * shade, tG * shade, tB * shade);
        glVertex2f(-w + curve, trunkY);
        glVertex2f(w + curve, trunkY);
    }
    glEnd();

    // Bark texture lines
    glColor4f(tR * 0.30f, tG * 0.30f, tB * 0.30f, 0.35f); glPointSize(1.3f);
    Bresenham_Line(-6, 8, -5, 74);
    Bresenham_Line(-1, 5, 0, 78);
    Bresenham_Line(5, 8, 4, 74);
    Bresenham_Line(-9, 10, -7, 55);
    Bresenham_Line(8, 12, 7, 58);
    // Horizontal bark cracks
    Bresenham_Line(-8, 25, 8, 26);
    Bresenham_Line(-7, 45, 6, 44);
    Bresenham_Line(-5, 60, 5, 61);
    glPointSize(1);

    // Bark knots
    FilledCircle(3, 32, 3.2f, tR * 0.45f, tG * 0.45f, tB * 0.45f, 0.35f);
    FilledCircle(3, 32, 1.8f, tR * 0.35f, tG * 0.35f, tB * 0.35f, 0.45f);
    FilledCircle(-4, 52, 2.5f, tR * 0.40f, tG * 0.40f, tB * 0.40f, 0.3f);

    float bwOff = windOff * 0.3f;
    glColor3f(tR * 0.75f, tG * 0.75f, tB * 0.75f);

    // Right main branch
    glLineWidth(4.0f);
    glBegin(GL_LINES); glVertex2f(4, 58); glVertex2f(36 + bwOff, 82); glEnd();
    glLineWidth(2.5f);
    glBegin(GL_LINES); glVertex2f(36 + bwOff, 82); glVertex2f(52 + bwOff, 92); glEnd();
    glBegin(GL_LINES); glVertex2f(36 + bwOff, 82); glVertex2f(44 + bwOff, 98); glEnd();

    // Left main branch
    glLineWidth(4.0f);
    glBegin(GL_LINES); glVertex2f(-3, 53); glVertex2f(-34 + bwOff, 78); glEnd();
    glLineWidth(2.5f);
    glBegin(GL_LINES); glVertex2f(-34 + bwOff, 78); glVertex2f(-47 + bwOff, 90); glEnd();
    glBegin(GL_LINES); glVertex2f(-34 + bwOff, 78); glVertex2f(-40 + bwOff, 95); glEnd();

    // Upper branches
    glLineWidth(3.0f);
    glBegin(GL_LINES); glVertex2f(2, 68); glVertex2f(22 + bwOff, 93); glEnd();
    glBegin(GL_LINES); glVertex2f(-1, 66); glVertex2f(-20 + bwOff, 90); glEnd();
    glLineWidth(2.2f);
    glBegin(GL_LINES); glVertex2f(1, 73); glVertex2f(14 + bwOff, 100); glEnd();
    glBegin(GL_LINES); glVertex2f(0, 70); glVertex2f(-12 + bwOff, 96); glEnd();
    // Vertical top branch
    glLineWidth(2.5f);
    glBegin(GL_LINES); glVertex2f(1, 75); glVertex2f(3 + bwOff * 0.5f, 108); glEnd();

    // Twigs
    glLineWidth(1.3f);
    glColor3f(tR * 0.68f, tG * 0.68f, tB * 0.68f);
    glBegin(GL_LINES); glVertex2f(22 + bwOff, 93); glVertex2f(30 + bwOff, 106); glEnd();
    glBegin(GL_LINES); glVertex2f(-20 + bwOff, 90); glVertex2f(-28 + bwOff, 103); glEnd();
    glBegin(GL_LINES); glVertex2f(52 + bwOff, 92); glVertex2f(56 + bwOff, 102); glEnd();
    glBegin(GL_LINES); glVertex2f(-47 + bwOff, 90); glVertex2f(-52 + bwOff, 100); glEnd();
    glBegin(GL_LINES); glVertex2f(14 + bwOff, 100); glVertex2f(18 + bwOff, 114); glEnd();
    glBegin(GL_LINES); glVertex2f(-12 + bwOff, 96); glVertex2f(-16 + bwOff, 110); glEnd();
    glLineWidth(1);

    // === CANOPY ===
    // Built from many irregular ellipses, no symmetric circles
    // All shifted by windOff for storm bending

    float wo = windOff * 0.4f; // canopy wind offset

    // Deep shadow base (just a shadow, NOT a visible green circle)
    FilledEllipse(5 + wo, 90, 52, 30, lR * 0.25f, lG * 0.28f, lB * 0.25f, 0.20f);

    // Bottom hanging foliage - irregular shapes
    FilledEllipse(-30 + wo, 80, 18, 14, lR * 0.52f, lG * 0.55f, lB * 0.52f, 0.85f);
    FilledEllipse(28 + wo, 82, 20, 13, lR * 0.55f, lG * 0.57f, lB * 0.53f, 0.85f);
    FilledEllipse(-10 + wo, 78, 22, 12, lR * 0.50f, lG * 0.53f, lB * 0.50f, 0.80f);
    FilledEllipse(12 + wo, 80, 20, 14, lR * 0.53f, lG * 0.56f, lB * 0.52f, 0.82f);
    FilledEllipse(-20 + wo, 84, 16, 12, lR * 0.54f, lG * 0.57f, lB * 0.54f, 0.80f);
    FilledEllipse(20 + wo, 84, 17, 11, lR * 0.56f, lG * 0.58f, lB * 0.55f, 0.82f);

    // Mid-dark layer - denser, overlapping
    FilledEllipse(-36 + wo, 88, 20, 16, lR * 0.62f, lG * 0.65f, lB * 0.61f, 0.88f);
    FilledEllipse(38 + wo, 90, 21, 15, lR * 0.64f, lG * 0.67f, lB * 0.63f, 0.88f);
    FilledEllipse(-16 + wo, 90, 24, 17, lR * 0.66f, lG * 0.70f, lB * 0.65f, 0.90f);
    FilledEllipse(18 + wo, 92, 23, 16, lR * 0.68f, lG * 0.71f, lB * 0.67f, 0.90f);
    FilledEllipse(0 + wo, 88, 26, 18, lR * 0.64f, lG * 0.68f, lB * 0.63f, 0.88f);

    // Mid layer
    FilledEllipse(-28 + wo, 96, 22, 16, lR * 0.75f, lG * 0.78f, lB * 0.74f, 0.90f);
    FilledEllipse(26 + wo, 98, 23, 15, lR * 0.77f, lG * 0.80f, lB * 0.76f, 0.90f);
    FilledEllipse(-6 + wo, 98, 26, 18, lR * 0.79f, lG * 0.82f, lB * 0.78f, 0.92f);
    FilledEllipse(10 + wo, 100, 25, 17, lR * 0.81f, lG * 0.84f, lB * 0.80f, 0.92f);

    // Mid-bright
    FilledEllipse(-20 + wo, 104, 20, 15, lR * 0.85f, lG * 0.88f, lB * 0.84f, 0.92f);
    FilledEllipse(20 + wo, 106, 21, 14, lR * 0.87f, lG * 0.90f, lB * 0.86f, 0.92f);
    FilledEllipse(0 + wo, 108, 24, 16, lR * 0.89f, lG * 0.92f, lB * 0.88f, 0.93f);

    // Bright top (sunlit canopy crown)
    FilledEllipse(-12 + wo, 114, 18, 13, lR * 0.93f, lG * 0.95f, lB * 0.91f, 0.93f);
    FilledEllipse(12 + wo, 112, 19, 14, lR * 0.95f, lG * 0.97f, lB * 0.93f, 0.93f);
    FilledEllipse(0 + wo, 118, 20, 14, lR * 0.97f, lG, lB * 0.95f, 0.94f);

    // Crown tip clusters (pointed, natural top)
    FilledEllipse(-6 + wo, 126, 14, 10, lR, lG * 1.02f, lB * 0.98f, 0.92f);
    FilledEllipse(8 + wo, 124, 15, 11, lR * 1.02f, lG * 1.04f, lB, 0.92f);
    FilledEllipse(1 + wo, 132, 11, 9, lR * 1.04f, lG * 1.06f, lB * 1.01f, 0.90f);
    FilledEllipse(0 + wo, 138, 7, 6, lR * 1.06f, lG * 1.08f, lB * 1.02f, 0.85f);

    // Side clusters (organic widening)
    FilledEllipse(-44 + wo, 92, 14, 12, lR * 0.70f, lG * 0.73f, lB * 0.69f, 0.85f);
    FilledEllipse(46 + wo, 94, 15, 12, lR * 0.72f, lG * 0.75f, lB * 0.71f, 0.85f);
    FilledEllipse(-50 + wo, 86, 10, 9, lR * 0.65f, lG * 0.68f, lB * 0.64f, 0.78f);
    FilledEllipse(52 + wo, 88, 11, 9, lR * 0.67f, lG * 0.70f, lB * 0.66f, 0.78f);

    // Hanging lower clusters (draping effect)
    FilledEllipse(-24 + wo, 76, 10, 8, lR * 0.48f, lG * 0.50f, lB * 0.47f, 0.65f);
    FilledEllipse(22 + wo, 77, 11, 8, lR * 0.50f, lG * 0.52f, lB * 0.49f, 0.65f);

    // Highlight spots (sunlit patches on top)
    if (!isNight && stormIntensity < 0.3f) {
        FilledEllipse(-4 + wo, 134, 5, 4, lR + 0.15f, lG + 0.18f, lB + 0.08f, 0.25f);
        FilledEllipse(10 + wo, 122, 6, 5, lR + 0.12f, lG + 0.15f, lB + 0.06f, 0.22f);
        FilledEllipse(-16 + wo, 108, 5, 4, lR + 0.10f, lG + 0.12f, lB + 0.05f, 0.18f);
        FilledEllipse(24 + wo, 100, 5, 3, lR + 0.08f, lG + 0.10f, lB + 0.04f, 0.16f);
    }

    // Dark depth spots in canopy (adds 3D feeling)
    FilledEllipse(-8 + wo, 92, 6, 5, lR * 0.30f, lG * 0.32f, lB * 0.30f, 0.18f);
    FilledEllipse(14 + wo, 96, 5, 4, lR * 0.32f, lG * 0.34f, lB * 0.32f, 0.15f);
    FilledEllipse(-18 + wo, 100, 4, 3, lR * 0.28f, lG * 0.30f, lB * 0.28f, 0.12f);

    // Leaf detail dots on canopy edges
    glPointSize(2.2f);
    for (int i = 0; i < 36; i++) {
        float angle = i * 10.0f * PI / 180.0f;
        float cr = 36 + 10 * sin(i * 1.5f);
        float lx = wo + cr * cos(angle);
        float ly = 102 + cr * sin(angle) * 0.72f;
        float shade = 0.75f + 0.25f * sin(i * 2.3f);
        glColor4f(lR * shade, lG * shade, lB * shade, 0.45f);
        glBegin(GL_POINTS); glVertex2f(lx, ly); glEnd();
    }
    glPointSize(1);

    glPopMatrix();
}

//============================================================
//  DRAW: FRUIT TREE
//============================================================
void drawFruitTree(float x, float y, float scale,
    float frR, float frG, float frB, float frSz)
{
    glPushMatrix(); applyTranslation(x, y); applyScaling(scale, scale);
    float f = getEffectiveDayFactor();
    float lf = lightningFlash * 0.04f;
    float tR = 0.32f * f + 0.09f + lf, tG = 0.20f * f + 0.06f + lf, tB = 0.08f * f + 0.03f + lf;

    float windOff = stormIntensity * 5.0f * sin(glowPulse * 2.5f + x * 0.01f);
    float wo = windOff * 0.4f;

    // Ground shadow
    FilledEllipse(10, -4, 42, 9, 0, 0, 0, 0.10f * (1 - stormIntensity * 0.5f));

    // Root flares
    glColor3f(tR * 0.6f, tG * 0.6f, tB * 0.6f);
    glBegin(GL_TRIANGLES); glVertex2f(-15, 0); glVertex2f(-11, 0); glVertex2f(-12, 14); glEnd();
    glBegin(GL_TRIANGLES); glVertex2f(15, 0); glVertex2f(11, 0); glVertex2f(12, 14); glEnd();
    glBegin(GL_TRIANGLES); glVertex2f(-18, 3); glVertex2f(-13, 5); glVertex2f(-10, 12); glEnd();
    glBegin(GL_TRIANGLES); glVertex2f(18, 3); glVertex2f(13, 5); glVertex2f(10, 12); glEnd();

    // Exposed roots
    glColor3f(tR * 0.5f, tG * 0.5f, tB * 0.5f);
    glLineWidth(2.0f);
    glBegin(GL_LINES); glVertex2f(-15, 2); glVertex2f(-24, -1); glEnd();
    glBegin(GL_LINES); glVertex2f(15, 2); glVertex2f(23, 0); glEnd();
    glLineWidth(1);

    // Trunk
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= 12; i++) {
        float t = i / 12.0f;
        float trunkY = t * 78;
        float widthBase = 13, widthTop = 5;
        float w = widthBase + (widthTop - widthBase) * t;
        float curve = -1.2f * sin(t * PI * 0.5f) + windOff * t * 0.12f;
        float shade = 0.62f + 0.38f * t;
        glColor3f(tR * shade, tG * shade, tB * shade);
        glVertex2f(-w + curve, trunkY);
        glVertex2f(w + curve, trunkY);
    }
    glEnd();

    // Bark texture
    glColor4f(tR * 0.30f, tG * 0.30f, tB * 0.30f, 0.28f); glPointSize(1.5f);
    Bresenham_Line(-7, 8, -6, 70);
    Bresenham_Line(0, 5, 0, 74);
    Bresenham_Line(6, 8, 5, 70);
    Bresenham_Line(-10, 12, -8, 50);
    Bresenham_Line(9, 14, 8, 52);
    Bresenham_Line(-7, 30, 7, 31);
    Bresenham_Line(-6, 50, 5, 49);
    glPointSize(1);

    // Bark knot
    FilledCircle(-3, 38, 2.8f, tR * 0.42f, tG * 0.42f, tB * 0.42f, 0.35f);

    // === BRANCHES ===
    float bwOff = windOff * 0.3f;
    glColor3f(tR * 0.72f, tG * 0.72f, tB * 0.72f);
    glLineWidth(4.0f);
    glBegin(GL_LINES); glVertex2f(3, 56); glVertex2f(38 + bwOff, 80); glEnd();
    glBegin(GL_LINES); glVertex2f(-4, 50); glVertex2f(-32 + bwOff, 76); glEnd();
    glLineWidth(2.8f);
    glBegin(GL_LINES); glVertex2f(38 + bwOff, 80); glVertex2f(50 + bwOff, 92); glEnd();
    glBegin(GL_LINES); glVertex2f(-32 + bwOff, 76); glVertex2f(-44 + bwOff, 88); glEnd();
    glLineWidth(2.5f);
    glBegin(GL_LINES); glVertex2f(1, 63); glVertex2f(20 + bwOff, 88); glEnd();
    glBegin(GL_LINES); glVertex2f(-2, 60); glVertex2f(-18 + bwOff, 86); glEnd();
    glLineWidth(2.2f);
    glBegin(GL_LINES); glVertex2f(0, 68); glVertex2f(12 + bwOff, 95); glEnd();
    glBegin(GL_LINES); glVertex2f(-1, 66); glVertex2f(-10 + bwOff, 92); glEnd();
    glLineWidth(2.5f);
    glBegin(GL_LINES); glVertex2f(1, 72); glVertex2f(2 + bwOff * 0.5f, 105); glEnd();
    // Twigs
    glLineWidth(1.2f);
    glColor3f(tR * 0.65f, tG * 0.65f, tB * 0.65f);
    glBegin(GL_LINES); glVertex2f(50 + bwOff, 92); glVertex2f(54 + bwOff, 102); glEnd();
    glBegin(GL_LINES); glVertex2f(-44 + bwOff, 88); glVertex2f(-50 + bwOff, 98); glEnd();
    glBegin(GL_LINES); glVertex2f(20 + bwOff, 88); glVertex2f(26 + bwOff, 104); glEnd();
    glBegin(GL_LINES); glVertex2f(-18 + bwOff, 86); glVertex2f(-24 + bwOff, 102); glEnd();
    glLineWidth(1);

    // === FOLIAGE (same technique as drawTree) ===
    float lR = 0.11f * f + 0.03f + lf, lG = 0.40f * f + 0.10f + lf, lB = 0.08f * f + 0.02f + lf;

    // Shadow base
    FilledEllipse(5 + wo, 88, 50, 28, lR * 0.28f, lG * 0.30f, lB * 0.28f, 0.18f);

    // Bottom foliage
    FilledEllipse(-28 + wo, 78, 18, 13, lR * 0.54f, lG * 0.57f, lB * 0.54f, 0.82f);
    FilledEllipse(26 + wo, 80, 20, 12, lR * 0.56f, lG * 0.59f, lB * 0.55f, 0.82f);
    FilledEllipse(-8 + wo, 80, 22, 13, lR * 0.52f, lG * 0.55f, lB * 0.52f, 0.80f);
    FilledEllipse(10 + wo, 82, 20, 14, lR * 0.55f, lG * 0.58f, lB * 0.54f, 0.80f);

    // Mid-dark
    FilledEllipse(-34 + wo, 86, 20, 15, lR * 0.64f, lG * 0.67f, lB * 0.63f, 0.86f);
    FilledEllipse(36 + wo, 88, 21, 14, lR * 0.66f, lG * 0.69f, lB * 0.65f, 0.86f);
    FilledEllipse(-14 + wo, 88, 24, 16, lR * 0.68f, lG * 0.72f, lB * 0.67f, 0.88f);
    FilledEllipse(16 + wo, 90, 23, 15, lR * 0.70f, lG * 0.73f, lB * 0.69f, 0.88f);
    FilledEllipse(0 + wo, 86, 26, 17, lR * 0.66f, lG * 0.70f, lB * 0.65f, 0.86f);

    // Mid layer
    FilledEllipse(-26 + wo, 94, 22, 15, lR * 0.78f, lG * 0.80f, lB * 0.76f, 0.90f);
    FilledEllipse(24 + wo, 96, 23, 14, lR * 0.80f, lG * 0.82f, lB * 0.78f, 0.90f);
    FilledEllipse(-4 + wo, 96, 26, 17, lR * 0.82f, lG * 0.85f, lB * 0.80f, 0.92f);
    FilledEllipse(8 + wo, 98, 25, 16, lR * 0.84f, lG * 0.87f, lB * 0.82f, 0.92f);

    // Mid-bright
    FilledEllipse(-18 + wo, 104, 20, 14, lR * 0.87f, lG * 0.90f, lB * 0.86f, 0.92f);
    FilledEllipse(18 + wo, 106, 21, 13, lR * 0.89f, lG * 0.92f, lB * 0.88f, 0.92f);
    FilledEllipse(0 + wo, 108, 24, 15, lR * 0.91f, lG * 0.94f, lB * 0.90f, 0.93f);

    // Bright top
    FilledEllipse(-10 + wo, 114, 17, 12, lR * 0.94f, lG * 0.96f, lB * 0.92f, 0.93f);
    FilledEllipse(10 + wo, 112, 18, 13, lR * 0.96f, lG * 0.98f, lB * 0.94f, 0.93f);
    FilledEllipse(0 + wo, 118, 19, 13, lR * 0.98f, lG, lB * 0.96f, 0.93f);

    // Crown tips
    FilledEllipse(-5 + wo, 124, 13, 9, lR, lG * 1.02f, lB * 0.98f, 0.90f);
    FilledEllipse(6 + wo, 122, 14, 10, lR * 1.02f, lG * 1.04f, lB, 0.90f);
    FilledEllipse(0 + wo, 130, 10, 8, lR * 1.04f, lG * 1.06f, lB * 1.01f, 0.88f);
    FilledEllipse(0 + wo, 136, 6, 5, lR * 1.06f, lG * 1.08f, lB * 1.02f, 0.82f);

    // Side clusters
    FilledEllipse(-42 + wo, 90, 13, 11, lR * 0.68f, lG * 0.71f, lB * 0.67f, 0.82f);
    FilledEllipse(44 + wo, 92, 14, 11, lR * 0.70f, lG * 0.73f, lB * 0.69f, 0.82f);

    // Hanging drapes
    FilledEllipse(-22 + wo, 74, 10, 7, lR * 0.46f, lG * 0.48f, lB * 0.45f, 0.60f);
    FilledEllipse(20 + wo, 75, 11, 7, lR * 0.48f, lG * 0.50f, lB * 0.47f, 0.60f);

    // Highlights
    if (!isNight && stormIntensity < 0.3f) {
        FilledEllipse(-2 + wo, 132, 5, 4, lR + 0.12f, lG + 0.15f, lB + 0.06f, 0.24f);
        FilledEllipse(12 + wo, 116, 6, 4, lR + 0.10f, lG + 0.12f, lB + 0.05f, 0.20f);
    }

    // Depth spots
    FilledEllipse(-6 + wo, 90, 5, 4, lR * 0.28f, lG * 0.30f, lB * 0.28f, 0.15f);
    FilledEllipse(12 + wo, 94, 4, 3, lR * 0.30f, lG * 0.32f, lB * 0.30f, 0.12f);

    // === FRUITS  ===
    float fA = 0.88f * dayFactor + 0.25f;
    float fruitPos[][2] = {
        {-22 + wo, 82}, {18 + wo, 84}, {-10 + wo, 94}, {20 + wo, 92},
        {-16 + wo, 88}, {12 + wo, 90}, {26 + wo, 86}, {-28 + wo, 82},
        {4 + wo, 104}, {-8 + wo, 108}, {14 + wo, 106}, {6 + wo, 114},
        {-18 + wo, 100}, {22 + wo, 98}, {-36 + wo, 88}, {38 + wo, 90},
        {-4 + wo, 118}, {8 + wo, 116}
    };

    for (int i = 0; i < 18; i++) {
        float fx = fruitPos[i][0], fy = fruitPos[i][1];

        // Stem
        glColor4f(tR * 0.6f, tG * 0.6f, tB * 0.6f, fA * 0.7f);
        glLineWidth(1.2f);
        DDA_Line(fx, fy + frSz, fx, fy + frSz + 3);
        glLineWidth(1);

        // Shadow
        FilledCircle(fx + 0.5f, fy - 0.5f, frSz,
            frR * 0.25f, frG * 0.25f, frB * 0.25f, fA * 0.3f);
        // Body
        FilledCircle(fx, fy, frSz, frR, frG, frB, fA);
        // Highlight
        FilledCircle(fx - frSz * 0.3f, fy + frSz * 0.3f, frSz * 0.3f,
            1, 1, 1, 0.28f * fA);
    }

    // Leaf edge dots
    glPointSize(2.0f);
    for (int i = 0; i < 28; i++) {
        float angle = i * 13.0f * PI / 180.0f;
        float cr = 34 + 8 * sin(i * 1.7f);
        float lx = wo + cr * cos(angle);
        float ly = 96 + cr * sin(angle) * 0.65f;
        float shade = 0.72f + 0.28f * sin(i * 2.1f);
        glColor4f(lR * shade, lG * shade, lB * shade, 0.42f);
        glBegin(GL_POINTS); glVertex2f(lx, ly); glEnd();
    }
    glPointSize(1);

    glPopMatrix();
}

//============================================================
//  DRAW: BUSH 
//============================================================
void drawBush(float x, float y, float sc)
{
    glPushMatrix(); applyTranslation(x, y); applyScaling(sc, sc);
    float f = getEffectiveDayFactor();
    float bR = 0.10f * f + 0.03f, bG = 0.38f * f + 0.09f, bB = 0.07f * f + 0.02f;
    float wo = stormIntensity * 3.0f * sin(glowPulse * 2.5f + x * 0.02f);

    FilledEllipse(2, -5, 32, 9, 0, 0, 0, 0.08f);
    FilledCircle(0 + wo * 0.3f, 3, 26, bR * 0.75f, bG * 0.78f, bB * 0.75f);
    FilledCircle(20 + wo * 0.3f, 5, 19, bR * 0.72f, bG * 0.75f, bB * 0.72f);
    FilledCircle(-20 + wo * 0.3f, 5, 19, bR * 0.70f, bG * 0.73f, bB * 0.70f);
    FilledCircle(0 + wo * 0.4f, 8, 24, bR * 0.88f, bG * 0.90f, bB * 0.88f);
    FilledCircle(16 + wo * 0.4f, 10, 17, bR * 0.85f, bG * 0.88f, bB * 0.85f);
    FilledCircle(-16 + wo * 0.4f, 9, 17, bR * 0.83f, bG * 0.86f, bB * 0.83f);
    FilledCircle(0 + wo * 0.5f, 14, 18, bR, bG, bB);
    FilledCircle(10 + wo * 0.5f, 16, 14, bR * 1.05f, bG * 1.05f, bB);
    FilledCircle(-8 + wo * 0.5f, 15, 14, bR * 1.02f, bG * 1.02f, bB);
    FilledCircle(12, 14, 2.2f, 0.85f, 0.15f, 0.15f, 0.65f);
    FilledCircle(-10, 12, 2.0f, 0.85f, 0.15f, 0.15f, 0.60f);
    FilledCircle(5, 18, 1.8f, 0.88f, 0.18f, 0.12f, 0.55f);
    FilledCircle(-4, 16, 1.6f, 0.90f, 0.12f, 0.18f, 0.50f);
    if (!isNight && stormIntensity < 0.3f)
        FilledCircle(2, 18, 6, bR + 0.10f, bG + 0.12f, bB + 0.05f, 0.20f);
    glPopMatrix();
}

//============================================================
//  FLOWER TYPES 
//============================================================
void drawRose(float x, float y, float sw, float sc) {
    glPushMatrix(); applyTranslation(x, y); applyScaling(sc, sc); applyRotation(sw);
    float f = getEffectiveDayFactor(), sR = 0.10f * f + 0.03f, sG = 0.45f * f + 0.10f, sB = 0.07f * f + 0.02f;
    glColor3f(sR, sG, sB); glLineWidth(2.5f);
    glBegin(GL_LINES); glVertex2f(0, 0); glVertex2f(0, 28); glEnd(); glLineWidth(1);
    FilledEllipse(7, 14, 5, 2.5f, sR, sG, sB);
    glTranslatef(0, 30, 0);
    for (int i = 0; i < 6; i++) { glPushMatrix(); applyRotation(i * 60); FilledEllipse(0, 6, 4.5f, 7.5f, 0.75f, 0.05f, 0.1f); glPopMatrix(); }
    for (int i = 0; i < 6; i++) { glPushMatrix(); applyRotation(i * 60 + 30); FilledEllipse(0, 4.5f, 3.5f, 5.5f, 0.9f, 0.1f, 0.15f); glPopMatrix(); }
    FilledCircle(0, 0, 3, 0.65f, 0.02f, 0.05f); glPopMatrix();
}

void drawTulip(float x, float y, float sw, float sc, float pR, float pG, float pB) {
    glPushMatrix(); applyTranslation(x, y); applyScaling(sc, sc); applyRotation(sw);
    float f = getEffectiveDayFactor(), sR = 0.10f * f + 0.03f, sG = 0.45f * f + 0.10f, sB = 0.07f * f + 0.02f;
    glColor3f(sR, sG, sB); glLineWidth(2.5f);
    glBegin(GL_LINES); glVertex2f(0, 0); glVertex2f(0, 25); glEnd(); glLineWidth(1);
    FilledEllipse(6, 10, 5, 2.5f, sR, sG, sB);
    glTranslatef(0, 25, 0);
    glColor4f(pR * 0.7f, pG * 0.7f, pB * 0.7f, 0.9f);
    glBegin(GL_TRIANGLE_FAN); glVertex2f(0, 0);
    for (int i = 30; i <= 150; i += 5) { float a = i * PI / 180; glVertex2f(8 * cos(a), 12 * sin(a)); } glEnd();
    glColor4f(pR, pG, pB, 0.95f);
    glBegin(GL_TRIANGLE_FAN); glVertex2f(0, 0);
    for (int i = 0; i <= 120; i += 5) { float a = i * PI / 180; glVertex2f(7 * cos(a) + 1, 11 * sin(a)); } glEnd();
    glPopMatrix();
}

void drawDaisy(float x, float y, float sw, float sc) {
    glPushMatrix(); applyTranslation(x, y); applyScaling(sc, sc); applyRotation(sw);
    float f = getEffectiveDayFactor(), sR = 0.10f * f + 0.03f, sG = 0.45f * f + 0.10f, sB = 0.07f * f + 0.02f;
    glColor3f(sR, sG, sB); glLineWidth(2);
    glBegin(GL_LINES); glVertex2f(0, 0); glVertex2f(0, 22); glEnd(); glLineWidth(1);
    FilledEllipse(5, 9, 4, 2, sR, sG, sB);
    glTranslatef(0, 24, 0);
    for (int i = 0; i < 10; i++) { glPushMatrix(); applyRotation(i * 36); FilledEllipse(0, 6, 2.8f, 6.5f, 0.95f, 0.95f, 0.92f); glPopMatrix(); }
    FilledCircle(0, 0, 4, 1.0f, 0.82f, 0.0f); glPopMatrix();
}

void drawSunflower(float x, float y, float sw, float sc) {
    glPushMatrix(); applyTranslation(x, y); applyScaling(sc, sc); applyRotation(sw);
    float f = getEffectiveDayFactor(), sR = 0.10f * f + 0.03f, sG = 0.42f * f + 0.10f, sB = 0.07f * f + 0.02f;
    glColor3f(sR, sG, sB); glLineWidth(3);
    glBegin(GL_LINES); glVertex2f(0, 0); glVertex2f(0, 38); glEnd(); glLineWidth(1);
    FilledEllipse(10, 18, 9, 3.5f, sR, sG, sB); FilledEllipse(-8, 12, 7, 3, sR * 0.95f, sG * 0.95f, sB);
    glTranslatef(0, 40, 0);
    for (int i = 0; i < 14; i++) { glPushMatrix(); applyRotation(i * 25.7f); FilledEllipse(0, 11, 3.2f, 9, 1, 0.78f, 0); glPopMatrix(); }
    for (int i = 0; i < 14; i++) { glPushMatrix(); applyRotation(i * 25.7f + 13); FilledEllipse(0, 8, 2.5f, 7, 0.95f, 0.7f, 0); glPopMatrix(); }
    FilledCircle(0, 0, 7, 0.30f, 0.18f, 0.05f); glPopMatrix();
}

void drawLavender(float x, float y, float sw, float sc) {
    glPushMatrix(); applyTranslation(x, y); applyScaling(sc, sc); applyRotation(sw);
    float f = getEffectiveDayFactor(), sR = 0.10f * f + 0.03f, sG = 0.42f * f + 0.10f, sB = 0.07f * f + 0.02f;
    glColor3f(sR, sG, sB); glLineWidth(1.8f);
    glBegin(GL_LINES); glVertex2f(0, 0); glVertex2f(0, 28); glEnd(); glLineWidth(1);
    FilledEllipse(3, 7, 3.5f, 1.5f, sR, sG, sB);
    for (int i = 0; i < 7; i++) {
        float py = 28 + i * 3.2f, px = 1 * sin(i * 1.2f), sz = 2.8f - i * 0.25f;
        if (sz < 1.3f) sz = 1.3f;
        FilledCircle(px, py, sz, 0.55f, 0.30f, 0.70f, 0.85f - i * 0.05f);
    }
    glPopMatrix();
}

//============================================================
//  DRAW: 
//============================================================
void drawAllFlowers()
{
    float sw = flowerSway;
    // Extra storm sway
    float stormSway = stormIntensity * 3.0f;
    sw += stormSway * sin(glowPulse * 3);

    // LEFT SIDE
    drawSunflower(-570, -162, sw * 0.3f, 0.8f); drawSunflower(-540, -165, -sw * 0.35f, 0.85f);
    drawSunflower(-510, -160, sw * 0.4f, 0.9f);
    drawRose(-480, -163, -sw * 0.7f, 0.85f); drawRose(-455, -165, sw * 0.8f, 0.9f);
    drawTulip(-430, -162, -sw * 0.6f, 0.9f, 1, 0.3f, 0.5f); drawTulip(-405, -165, sw * 0.7f, 0.85f, 0.9f, 0.1f, 0.6f);
    drawDaisy(-380, -160, -sw * 0.5f, 0.9f); drawDaisy(-355, -163, sw * 0.6f, 0.85f);
    drawLavender(-330, -165, -sw * 0.4f, 0.9f); drawLavender(-310, -162, sw * 0.5f, 0.85f);
    drawRose(-290, -165, -sw * 0.75f, 0.9f); drawTulip(-270, -160, sw * 0.65f, 0.85f, 0.4f, 0.2f, 0.9f);
    drawDaisy(-250, -163, -sw * 0.55f, 0.9f); drawLavender(-230, -165, sw * 0.45f, 0.85f);
    drawRose(-210, -162, -sw * 0.8f, 0.9f); drawTulip(-190, -165, sw * 0.7f, 0.9f, 1, 0.5f, 0.8f);
    drawDaisy(-170, -160, -sw * 0.6f, 0.85f); drawSunflower(-150, -163, sw * 0.35f, 0.8f);
    drawLavender(-130, -165, -sw * 0.5f, 0.9f); drawRose(-110, -162, sw * 0.8f, 0.85f);
    drawTulip(-90, -165, -sw * 0.65f, 0.9f, 0.95f, 0.2f, 0.3f); drawDaisy(-70, -160, sw * 0.55f, 0.85f);

    drawRose(-565, -175, sw * 0.75f, 0.85f); drawTulip(-540, -178, -sw * 0.7f, 0.9f, 0.8f, 0.1f, 0.7f);
    drawDaisy(-515, -173, sw * 0.55f, 0.85f); drawLavender(-490, -176, -sw * 0.45f, 0.9f);
    drawRose(-465, -178, sw * 0.8f, 0.85f); drawTulip(-440, -174, -sw * 0.65f, 0.9f, 1, 0.7f, 0.2f);
    drawDaisy(-415, -177, sw * 0.5f, 0.85f); drawSunflower(-390, -174, -sw * 0.3f, 0.75f);
    drawRose(-365, -177, sw * 0.7f, 0.9f); drawLavender(-340, -174, -sw * 0.5f, 0.85f);
    drawTulip(-315, -177, sw * 0.6f, 0.85f, 0.5f, 0.1f, 0.85f); drawDaisy(-290, -174, -sw * 0.55f, 0.9f);
    drawRose(-265, -177, sw * 0.8f, 0.85f); drawTulip(-240, -174, -sw * 0.7f, 0.9f, 1, 0.4f, 0.4f);
    drawDaisy(-215, -177, sw * 0.6f, 0.85f); drawLavender(-195, -174, -sw * 0.5f, 0.9f);
    drawRose(-175, -177, sw * 0.75f, 0.85f); drawTulip(-155, -174, -sw * 0.65f, 0.9f, 0.3f, 0.7f, 0.3f);
    drawDaisy(-135, -177, sw * 0.5f, 0.85f); drawLavender(-115, -174, -sw * 0.45f, 0.9f);
    drawRose(-95, -177, sw * 0.8f, 0.85f); drawDaisy(-75, -174, -sw * 0.55f, 0.9f);

    drawLavender(-560, -188, sw * 0.4f, 0.8f); drawDaisy(-535, -186, -sw * 0.5f, 0.75f);
    drawRose(-510, -189, sw * 0.7f, 0.8f); drawLavender(-485, -186, -sw * 0.4f, 0.8f);
    drawDaisy(-460, -189, sw * 0.55f, 0.75f); drawRose(-435, -186, -sw * 0.75f, 0.8f);
    drawLavender(-410, -189, sw * 0.45f, 0.8f); drawDaisy(-385, -186, -sw * 0.5f, 0.75f);
    drawRose(-360, -189, sw * 0.8f, 0.8f); drawLavender(-335, -186, -sw * 0.45f, 0.8f);
    drawDaisy(-310, -189, sw * 0.55f, 0.75f); drawRose(-285, -186, -sw * 0.7f, 0.8f);
    drawLavender(-260, -189, sw * 0.4f, 0.8f); drawDaisy(-235, -186, -sw * 0.5f, 0.75f);
    drawRose(-210, -189, sw * 0.75f, 0.8f); drawLavender(-185, -186, -sw * 0.45f, 0.8f);
    drawDaisy(-160, -189, sw * 0.55f, 0.75f); drawRose(-135, -186, -sw * 0.7f, 0.8f);
    drawLavender(-110, -189, sw * 0.4f, 0.8f); drawDaisy(-85, -186, -sw * 0.5f, 0.75f);

    // RIGHT SIDE
    drawSunflower(80, -162, -sw * 0.35f, 0.85f); drawSunflower(110, -165, sw * 0.4f, 0.8f);
    drawRose(140, -160, -sw * 0.75f, 0.9f); drawTulip(170, -163, sw * 0.65f, 0.85f, 1, 0.3f, 0.5f);
    drawDaisy(200, -165, -sw * 0.55f, 0.9f); drawLavender(230, -162, sw * 0.5f, 0.85f);
    drawRose(260, -165, -sw * 0.8f, 0.9f); drawTulip(290, -160, sw * 0.7f, 0.85f, 0.4f, 0.2f, 0.9f);
    drawDaisy(320, -163, -sw * 0.6f, 0.9f); drawSunflower(350, -165, sw * 0.3f, 0.8f);
    drawRose(380, -162, -sw * 0.75f, 0.85f); drawTulip(410, -165, sw * 0.65f, 0.9f, 0.95f, 0.2f, 0.3f);
    drawDaisy(440, -160, -sw * 0.5f, 0.85f); drawLavender(470, -163, sw * 0.45f, 0.9f);
    drawRose(500, -165, -sw * 0.8f, 0.85f); drawTulip(530, -162, sw * 0.7f, 0.9f, 1, 0.5f, 0.8f);
    drawDaisy(560, -165, -sw * 0.55f, 0.85f);

    drawRose(75, -175, -sw * 0.7f, 0.85f); drawTulip(100, -178, sw * 0.65f, 0.9f, 0.8f, 0.1f, 0.7f);
    drawDaisy(125, -173, -sw * 0.5f, 0.85f); drawLavender(150, -176, sw * 0.45f, 0.9f);
    drawRose(175, -178, -sw * 0.8f, 0.85f); drawSunflower(200, -174, sw * 0.35f, 0.75f);
    drawTulip(225, -177, -sw * 0.6f, 0.85f, 1, 0.7f, 0.2f); drawDaisy(250, -174, sw * 0.55f, 0.9f);
    drawRose(275, -177, -sw * 0.75f, 0.85f); drawLavender(300, -174, sw * 0.5f, 0.9f);
    drawTulip(325, -177, -sw * 0.65f, 0.85f, 0.5f, 0.1f, 0.85f); drawDaisy(350, -174, sw * 0.5f, 0.9f);
    drawRose(375, -177, -sw * 0.8f, 0.85f); drawLavender(400, -174, sw * 0.45f, 0.9f);
    drawTulip(425, -177, -sw * 0.7f, 0.85f, 1, 0.4f, 0.4f); drawDaisy(450, -174, sw * 0.55f, 0.9f);
    drawRose(475, -177, -sw * 0.75f, 0.85f); drawLavender(500, -174, sw * 0.5f, 0.9f);
    drawDaisy(525, -177, -sw * 0.55f, 0.85f); drawRose(550, -174, sw * 0.8f, 0.9f);

    drawLavender(85, -188, -sw * 0.4f, 0.8f); drawDaisy(115, -186, sw * 0.5f, 0.75f);
    drawRose(145, -189, -sw * 0.7f, 0.8f); drawLavender(175, -186, sw * 0.45f, 0.8f);
    drawDaisy(205, -189, -sw * 0.55f, 0.75f); drawRose(235, -186, sw * 0.75f, 0.8f);
    drawLavender(265, -189, -sw * 0.4f, 0.8f); drawDaisy(295, -186, sw * 0.5f, 0.75f);
    drawRose(325, -189, -sw * 0.8f, 0.8f); drawLavender(355, -186, sw * 0.45f, 0.8f);
    drawDaisy(385, -189, -sw * 0.55f, 0.75f); drawRose(415, -186, sw * 0.7f, 0.8f);
    drawLavender(445, -189, -sw * 0.4f, 0.8f); drawDaisy(475, -186, sw * 0.5f, 0.75f);
    drawRose(505, -189, -sw * 0.75f, 0.8f); drawLavender(535, -186, sw * 0.45f, 0.8f);
    drawDaisy(565, -189, -sw * 0.5f, 0.75f);
}

//============================================================
//  DRAW: BENCH
//============================================================
void drawBench(float x, float y) {
    glPushMatrix(); applyTranslation(x, y);
    float f = getEffectiveDayFactor(), wR = 0.42f * f + 0.11f, wG = 0.26f * f + 0.07f, wB = 0.11f * f + 0.04f;
    float lf = lightningFlash * 0.04f;
    wR += lf; wG += lf; wB += lf;

    FilledEllipse(4, -28, 52, 7, 0, 0, 0, 0.12f);
    glColor3f(wR * 0.6f, wG * 0.6f, wB * 0.6f);
    glBegin(GL_QUADS);
    glVertex2f(-48, -25); glVertex2f(-43, -25); glVertex2f(-43, 0); glVertex2f(-48, 0);
    glVertex2f(43, -25); glVertex2f(48, -25); glVertex2f(48, 0); glVertex2f(43, 0);
    glVertex2f(-46, 0); glVertex2f(-41, 0); glVertex2f(-41, 34); glVertex2f(-46, 34);
    glVertex2f(41, 0); glVertex2f(46, 0); glVertex2f(46, 34); glVertex2f(41, 34);
    glEnd();
    for (int i = 0; i < 3; i++) {
        float py = i * 6, sh = 1 - i * 0.06f;
        glBegin(GL_QUADS); glColor3f(wR * sh, wG * sh, wB * sh);
        glVertex2f(-50, py); glVertex2f(50, py);
        glColor3f(wR * sh * 1.08f, wG * sh * 1.04f, wB * sh);
        glVertex2f(50, py + 4); glVertex2f(-50, py + 4); glEnd();
    }
    for (int i = 0; i < 2; i++) {
        float py = 22 + i * 7, sh = 1 - i * 0.05f;
        glBegin(GL_QUADS); glColor3f(wR * sh, wG * sh, wB * sh);
        glVertex2f(-48, py); glVertex2f(48, py);
        glColor3f(wR * sh * 1.06f, wG * sh * 1.03f, wB * sh);
        glVertex2f(48, py + 5); glVertex2f(-48, py + 5); glEnd();
    }

    // Wet bench in storm/rain
    if (stormIntensity > 0.2f || rainEnabled) {
        float wetA = (stormIntensity > 0.1f ? stormIntensity : 0.3f) * 0.08f;
        glColor4f(0.3f, 0.35f, 0.5f, wetA);
        glBegin(GL_QUADS);
        glVertex2f(-50, 0); glVertex2f(50, 0);
        glVertex2f(50, 18); glVertex2f(-50, 18);
        glEnd();
    }

    glPopMatrix();
}

//============================================================
//  DRAW: LAMP POST 
//============================================================
void drawLampPost(float x, float y, float sc)
{
    glPushMatrix(); applyTranslation(x, y); applyScaling(sc, sc);
    float f = getEffectiveDayFactor();
    float lf = lightningFlash * 0.04f;
    float mR = 0.28f * f + 0.12f + lf, mG = 0.28f * f + 0.12f + lf, mB = 0.30f * f + 0.14f + lf;

    // Shadow
    FilledEllipse(5, -4, 18, 5, 0, 0, 0, 0.08f);

    // Base - wider and more detailed
    glColor3f(mR * 0.5f, mG * 0.5f, mB * 0.5f);
    glBegin(GL_QUADS);
    glVertex2f(-12, 0); glVertex2f(12, 0);
    glVertex2f(8, 8); glVertex2f(-8, 8);
    glEnd();
    // Base detail
    glBegin(GL_QUADS);
    glColor3f(mR * 0.55f, mG * 0.55f, mB * 0.55f);
    glVertex2f(-9, 8); glVertex2f(9, 8);
    glVertex2f(6, 14); glVertex2f(-6, 14);
    glEnd();

    // Main pole - taller
    glBegin(GL_QUADS);
    glColor3f(mR * 0.65f, mG * 0.65f, mB * 0.65f);
    glVertex2f(-3.5f, 14); glVertex2f(3.5f, 14);
    glColor3f(mR * 0.9f, mG * 0.9f, mB * 0.9f);
    glVertex2f(2.5f, 85); glVertex2f(-2.5f, 85);
    glEnd();

    // Pole highlight stripe
    glColor4f(mR * 1.2f, mG * 1.2f, mB * 1.2f, 0.2f);
    glBegin(GL_QUADS);
    glVertex2f(-1, 14); glVertex2f(0.5f, 14);
    glVertex2f(0, 85); glVertex2f(-0.5f, 85);
    glEnd();

    // Decorative ring at top of pole
    glColor3f(mR * 0.7f, mG * 0.7f, mB * 0.7f);
    glBegin(GL_QUADS);
    glVertex2f(-5, 82); glVertex2f(5, 82);
    glVertex2f(4, 86); glVertex2f(-4, 86);
    glEnd();

    // Curved arm (longer)
    glColor3f(mR * 0.85f, mG * 0.85f, mB * 0.85f);
    glLineWidth(3.5f);
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= 12; i++) {
        float t = i / 12.0f;
        float ax = t * 18;
        float ay = 86 + 10 * sin(t * PI);
        glVertex2f(ax, ay);
    }
    glEnd();
    // Second arm (other side)
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= 12; i++) {
        float t = i / 12.0f;
        float ax = -t * 18;
        float ay = 86 + 10 * sin(t * PI);
        glVertex2f(ax, ay);
    }
    glEnd();
    glLineWidth(1);

    // Lamp housing right
    glColor3f(mR * 0.75f, mG * 0.75f, mB * 0.75f);
    glBegin(GL_QUADS);
    glVertex2f(14, 86); glVertex2f(24, 86);
    glVertex2f(22, 94); glVertex2f(16, 94);
    glEnd();
    // Lamp housing left
    glBegin(GL_QUADS);
    glVertex2f(-24, 86); glVertex2f(-14, 86);
    glVertex2f(-16, 94); glVertex2f(-22, 94);
    glEnd();

    // Lamp glass / light
    float lampBright = (1 - dayFactor) * 0.9f + 0.15f;
    // Night glow
    if (dayFactor < 0.5f || stormIntensity > 0.3f) {
        float glowStr = (1 - dayFactor) + stormIntensity * 0.5f;
        if (glowStr > 1) glowStr = 1;
        // Right lamp
        GlowCircle(19, 88, 45, 1, 0.88f, 0.45f, 0.05f * glowStr, 0);
        GlowCircle(19, 88, 28, 1, 0.88f, 0.5f, 0.10f * glowStr, 0);
        GlowCircle(19, 88, 15, 1, 0.9f, 0.6f, 0.18f * glowStr, 0);
        // Left lamp
        GlowCircle(-19, 88, 45, 1, 0.88f, 0.45f, 0.05f * glowStr, 0);
        GlowCircle(-19, 88, 28, 1, 0.88f, 0.5f, 0.10f * glowStr, 0);
        GlowCircle(-19, 88, 15, 1, 0.9f, 0.6f, 0.18f * glowStr, 0);
        // Ground light cones
        GlowCircle(19, -5, 50, 1, 0.9f, 0.5f, 0.04f * glowStr, 0);
        GlowCircle(-19, -5, 50, 1, 0.9f, 0.5f, 0.04f * glowStr, 0);
    }
    // Lamp bulbs
    FilledCircle(19, 90, 4.5f, 1 * lampBright, 0.92f * lampBright, 0.5f * lampBright, 0.92f);
    FilledCircle(-19, 90, 4.5f, 1 * lampBright, 0.92f * lampBright, 0.5f * lampBright, 0.92f);

    glPopMatrix();
}

//============================================================
//  DRAW: CAT HOUSE (outside fence, right side)
//============================================================
void drawCatHouse(float x, float y)
{
    glPushMatrix(); applyTranslation(x, y);
    applyScaling(1.4f, 1.4f);  // Make the house bigger
    float f = getEffectiveDayFactor();
    float lf = lightningFlash * 0.04f;

    // Wood colors
    float wR = 0.50f * f + 0.15f + lf, wG = 0.32f * f + 0.10f + lf, wB = 0.14f * f + 0.05f + lf;
    float dR = wR * 0.65f, dG = wG * 0.65f, dB = wB * 0.65f;

    // Shadow
    FilledEllipse(5, -3, 32, 7, 0, 0, 0, 0.10f);

    // Base / floor
    glBegin(GL_QUADS);
    glColor3f(dR * 0.8f, dG * 0.8f, dB * 0.8f);
    glVertex2f(-28, 0); glVertex2f(28, 0);
    glVertex2f(28, 4); glVertex2f(-28, 4);
    glEnd();

    // Back wall
    glBegin(GL_QUADS);
    glColor3f(dR, dG, dB);
    glVertex2f(-25, 4); glVertex2f(25, 4);
    glColor3f(wR * 0.9f, wG * 0.9f, wB * 0.9f);
    glVertex2f(25, 36); glVertex2f(-25, 36);
    glEnd();

    // Side wall left
    glBegin(GL_QUADS);
    glColor3f(dR * 0.85f, dG * 0.85f, dB * 0.85f);
    glVertex2f(-25, 4); glVertex2f(-22, 4);
    glColor3f(wR * 0.8f, wG * 0.8f, wB * 0.8f);
    glVertex2f(-22, 36); glVertex2f(-25, 36);
    glEnd();

    // Side wall right
    glBegin(GL_QUADS);
    glColor3f(dR * 0.85f, dG * 0.85f, dB * 0.85f);
    glVertex2f(22, 4); glVertex2f(25, 4);
    glColor3f(wR * 0.8f, wG * 0.8f, wB * 0.8f);
    glVertex2f(25, 36); glVertex2f(22, 36);
    glEnd();

    // Wood planks texture on front
    for (int i = 0; i < 4; i++) {
        float py = 6 + i * 8;
        glColor4f(dR * 0.5f, dG * 0.5f, dB * 0.5f, 0.3f);
        glBegin(GL_LINES);
        glVertex2f(-24, py); glVertex2f(24, py);
        glEnd();
    }

    // Door opening (dark hole)
    glColor3f(0.05f, 0.03f, 0.02f);
    glBegin(GL_QUADS);
    glVertex2f(-8, 4); glVertex2f(8, 4);
    glVertex2f(8, 22); glVertex2f(-8, 22);
    glEnd();
    // Door arch top
    FilledCircle(0, 22, 8, 0.05f, 0.03f, 0.02f);

    // Door frame
    glColor3f(wR * 0.6f, wG * 0.6f, wB * 0.6f);
    glLineWidth(2.5f);
    glBegin(GL_LINE_STRIP);
    glVertex2f(-8, 4); glVertex2f(-8, 22);
    for (int i = 0; i <= 10; i++) {
        float a = PI * i / 10.0f;
        glVertex2f(8 * cos(PI - a), 22 + 8 * sin(PI - a));
    }
    glVertex2f(8, 4);
    glEnd();
    glLineWidth(1);

    // Roof - triangular
    glColor3f(0.55f * f + 0.15f, 0.18f * f + 0.06f, 0.08f * f + 0.03f);
    glBegin(GL_TRIANGLES);
    glVertex2f(-30, 36); glVertex2f(30, 36); glVertex2f(0, 52);
    glEnd();

    // Roof edge highlight
    glColor3f(0.60f * f + 0.18f, 0.20f * f + 0.07f, 0.10f * f + 0.04f);
    glLineWidth(2);
    glBegin(GL_LINE_STRIP);
    glVertex2f(-30, 36); glVertex2f(0, 52); glVertex2f(30, 36);
    glEnd();
    glLineWidth(1);

    // Roof texture lines
    glColor4f(0.40f * f + 0.10f, 0.14f * f + 0.04f, 0.06f * f + 0.02f, 0.4f);
    for (int i = 0; i < 5; i++) {
        float ry = 38 + i * 3;
        float t = (ry - 36.0f) / (52.0f - 36.0f);
        float hw = 30 * (1 - t);
        glBegin(GL_LINES); glVertex2f(-hw, ry); glVertex2f(hw, ry); glEnd();
    }

    // "CAT" text sign on front (small wooden sign)
    glColor3f(wR * 1.1f, wG * 0.9f, wB * 0.7f);
    glBegin(GL_QUADS);
    glVertex2f(-12, 28); glVertex2f(12, 28);
    glVertex2f(12, 34); glVertex2f(-12, 34);
    glEnd();
    // Sign border
    glColor3f(dR * 0.5f, dG * 0.5f, dB * 0.5f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(-12, 28); glVertex2f(12, 28);
    glVertex2f(12, 34); glVertex2f(-12, 34);
    glEnd();
    glLineWidth(1);

    // If cat is inside, show eyes peeking from door
    if (catState == CAT_IN_HOUSE) {
        float eyeBlink = 0.5f + 0.5f * sin(glowPulse * 2);
        if (eyeBlink > 0.3f) {
            FilledCircle(-3, 14, 2, 0.85f, 0.80f, 0.20f, 0.8f);
            FilledCircle(3, 14, 2, 0.85f, 0.80f, 0.20f, 0.8f);
            FilledCircle(-3, 14, 0.8f, 0.1f, 0.1f, 0.05f, 0.9f);
            FilledCircle(3, 14, 0.8f, 0.1f, 0.1f, 0.05f, 0.9f);
        }
    }

    glPopMatrix();
}

//============================================================
//  DRAW: CAT (grey-white, realistic)
//============================================================
void drawCat(float x, float y, float sc, bool facingRight, bool running, float legPhase, float tailWag)
{
    glPushMatrix(); applyTranslation(x, y); applyScaling(sc, sc);
    if (!facingRight) applyReflectionY();

    float f = getEffectiveDayFactor();
    float lf = lightningFlash * 0.04f;

    // Cat colors - grey-white
    float bodyR = 0.72f * f + 0.22f + lf;
    float bodyG = 0.72f * f + 0.22f + lf;
    float bodyB = 0.74f * f + 0.24f + lf;
    float darkR = bodyR * 0.65f, darkG = bodyG * 0.65f, darkB = bodyB * 0.65f;
    float lightR = bodyR * 1.15f, lightG = bodyG * 1.15f, lightB = bodyB * 1.12f;
    if (lightR > 1) lightR = 1; if (lightG > 1) lightG = 1; if (lightB > 1) lightB = 1;

    float alpha = 0.95f;

    if (running) {
        // === RUNNING POSE ===
        float lp = legPhase;
        float frontLegAngle = sin(lp) * 35;
        float backLegAngle = sin(lp + PI) * 35;
        float bodyBob = sin(lp * 2) * 1.5f;
        float bodyStretch = 1.0f + 0.05f * sin(lp * 2);

        glTranslatef(0, bodyBob, 0);

        // Tail (flowing behind)
        float tailAngle = -30 + tailWag * 15 + sin(lp) * 20;
        glPushMatrix();
        applyTranslation(-18 * bodyStretch, 8);
        applyRotation(tailAngle);
        FilledEllipse(-12, 0, 14, 2.2f, darkR, darkG, darkB, alpha);
        // Tail tip (white)
        FilledCircle(-24, 1, 2.5f, lightR, lightG, lightB, alpha);
        glPopMatrix();

        // Body (stretched when running)
        FilledEllipse(0, 6, 18 * bodyStretch, 8, bodyR, bodyG, bodyB, alpha);
        // Belly (lighter)
        FilledEllipse(0, 3, 14 * bodyStretch, 4, lightR, lightG, lightB, alpha * 0.6f);

        // Back legs
        glPushMatrix();
        applyTranslation(-12, 2);
        applyRotation(backLegAngle);
        FilledEllipse(0, -6, 2.8f, 7, darkR, darkG, darkB, alpha);
        // Paw
        FilledEllipse(0, -13, 3, 2, lightR, lightG, lightB, alpha);
        glPopMatrix();

        glPushMatrix();
        applyTranslation(-8, 2);
        applyRotation(backLegAngle * 0.8f);
        FilledEllipse(0, -6, 2.5f, 6.5f, bodyR * 0.9f, bodyG * 0.9f, bodyB * 0.9f, alpha * 0.85f);
        FilledEllipse(0, -12.5f, 2.8f, 1.8f, lightR, lightG, lightB, alpha * 0.85f);
        glPopMatrix();

        // Front legs
        glPushMatrix();
        applyTranslation(12, 2);
        applyRotation(frontLegAngle);
        FilledEllipse(0, -6, 2.5f, 7, darkR, darkG, darkB, alpha);
        FilledEllipse(0, -13, 2.8f, 1.8f, lightR, lightG, lightB, alpha);
        glPopMatrix();

        glPushMatrix();
        applyTranslation(8, 2);
        applyRotation(frontLegAngle * 0.8f);
        FilledEllipse(0, -6, 2.2f, 6.5f, bodyR * 0.9f, bodyG * 0.9f, bodyB * 0.9f, alpha * 0.85f);
        FilledEllipse(0, -12.5f, 2.5f, 1.6f, lightR, lightG, lightB, alpha * 0.85f);
        glPopMatrix();

        // Head
        float headBob = sin(lp * 2 + 0.5f) * 1;
        FilledCircle(16 * bodyStretch, 10 + headBob, 7.5f, bodyR, bodyG, bodyB, alpha);
        // White muzzle
        FilledEllipse(20 * bodyStretch, 8 + headBob, 4, 3.5f, lightR, lightG, lightB, alpha);
        // Ears
        glColor4f(darkR, darkG, darkB, alpha);
        glBegin(GL_TRIANGLES);
        glVertex2f(12 * bodyStretch, 16 + headBob);
        glVertex2f(14 * bodyStretch, 16 + headBob);
        glVertex2f(11 * bodyStretch, 22 + headBob);
        glEnd();
        glBegin(GL_TRIANGLES);
        glVertex2f(17 * bodyStretch, 16 + headBob);
        glVertex2f(19 * bodyStretch, 16 + headBob);
        glVertex2f(20 * bodyStretch, 22 + headBob);
        glEnd();
        // Inner ears (pink)
        glColor4f(0.85f, 0.6f, 0.6f, alpha * 0.5f);
        glBegin(GL_TRIANGLES);
        glVertex2f(12.5f * bodyStretch, 16 + headBob);
        glVertex2f(13.5f * bodyStretch, 16 + headBob);
        glVertex2f(11.5f * bodyStretch, 20 + headBob);
        glEnd();
        glBegin(GL_TRIANGLES);
        glVertex2f(17.5f * bodyStretch, 16 + headBob);
        glVertex2f(18.5f * bodyStretch, 16 + headBob);
        glVertex2f(19.5f * bodyStretch, 20 + headBob);
        glEnd();
        // Eyes (alert when running)
        FilledCircle(14 * bodyStretch, 11 + headBob, 1.8f, 0.85f, 0.80f, 0.20f, alpha);
        FilledCircle(18 * bodyStretch, 11 + headBob, 1.8f, 0.85f, 0.80f, 0.20f, alpha);
        FilledCircle(14.3f * bodyStretch, 11 + headBob, 0.9f, 0.1f, 0.1f, 0.05f, alpha);
        FilledCircle(18.3f * bodyStretch, 11 + headBob, 0.9f, 0.1f, 0.1f, 0.05f, alpha);
        // Nose
        FilledCircle(21 * bodyStretch, 9 + headBob, 1, 0.85f, 0.55f, 0.55f, alpha);
        // Whiskers
        glColor4f(lightR, lightG, lightB, alpha * 0.6f);
        glLineWidth(0.8f);
        DDA_Line(21 * bodyStretch, 8.5f + headBob, 28 * bodyStretch, 10 + headBob);
        DDA_Line(21 * bodyStretch, 8 + headBob, 28 * bodyStretch, 7 + headBob);
        DDA_Line(21 * bodyStretch, 7.5f + headBob, 27 * bodyStretch, 5 + headBob);
        glLineWidth(1);

    }
    else {
        // === SITTING POSE ===
        // Tail curled around
        float tailAngle = tailWag * 10;
        glPushMatrix();
        applyTranslation(-10, 2);
        applyRotation(-20 + tailAngle);
        FilledEllipse(-10, 2, 12, 2, darkR, darkG, darkB, alpha);
        FilledCircle(-20, 4, 2.5f, lightR, lightG, lightB, alpha);
        glPopMatrix();

        // Body (sitting, more upright)
        FilledEllipse(0, 8, 12, 10, bodyR, bodyG, bodyB, alpha);
        // Chest (lighter)
        FilledEllipse(4, 6, 7, 8, lightR, lightG, lightB, alpha * 0.5f);

        // Front paws (visible, tucked)
        FilledEllipse(8, -1, 4, 2.5f, lightR, lightG, lightB, alpha);
        FilledEllipse(3, -1, 4, 2.5f, lightR * 0.95f, lightG * 0.95f, lightB * 0.95f, alpha * 0.9f);

        // Back legs (sitting, tucked under)
        FilledEllipse(-6, 1, 6, 4, bodyR * 0.9f, bodyG * 0.9f, bodyB * 0.9f, alpha * 0.7f);

        // Head
        FilledCircle(6, 18, 8, bodyR, bodyG, bodyB, alpha);
        // White face patch
        FilledEllipse(9, 16, 4.5f, 4, lightR, lightG, lightB, alpha * 0.7f);

        // Ears
        glColor4f(darkR, darkG, darkB, alpha);
        glBegin(GL_TRIANGLES);
        glVertex2f(1, 24); glVertex2f(4, 24); glVertex2f(0, 30);
        glEnd();
        glBegin(GL_TRIANGLES);
        glVertex2f(8, 24); glVertex2f(11, 24); glVertex2f(12, 30);
        glEnd();
        // Inner ears
        glColor4f(0.85f, 0.6f, 0.6f, alpha * 0.5f);
        glBegin(GL_TRIANGLES);
        glVertex2f(1.5f, 24); glVertex2f(3.5f, 24); glVertex2f(0.5f, 28);
        glEnd();
        glBegin(GL_TRIANGLES);
        glVertex2f(8.5f, 24); glVertex2f(10.5f, 24); glVertex2f(11.5f, 28);
        glEnd();

        // Eyes (relaxed, slightly squinted)
        float blink = sin(glowPulse * 0.5f);
        float eyeOpenness = (blink > 0.95f) ? 0.3f : 1.0f;
        FilledEllipse(3, 19, 1.8f, 1.8f * eyeOpenness, 0.85f, 0.80f, 0.20f, alpha);
        FilledEllipse(9, 19, 1.8f, 1.8f * eyeOpenness, 0.85f, 0.80f, 0.20f, alpha);
        if (eyeOpenness > 0.5f) {
            FilledCircle(3.3f, 19, 0.8f, 0.1f, 0.1f, 0.05f, alpha);
            FilledCircle(9.3f, 19, 0.8f, 0.1f, 0.1f, 0.05f, alpha);
        }

        // Nose
        FilledCircle(7, 16.5f, 1.2f, 0.85f, 0.55f, 0.55f, alpha);

        // Mouth lines
        glColor4f(darkR * 0.5f, darkG * 0.5f, darkB * 0.5f, alpha * 0.4f);
        DDA_Line(7, 15.5f, 6, 14.5f);
        DDA_Line(7, 15.5f, 8, 14.5f);

        // Whiskers
        glColor4f(lightR, lightG, lightB, alpha * 0.5f);
        glLineWidth(0.8f);
        DDA_Line(10, 16, 18, 18);
        DDA_Line(10, 15.5f, 18, 15);
        DDA_Line(10, 15, 17, 12);
        DDA_Line(2, 16, -5, 18);
        DDA_Line(2, 15.5f, -5, 15);
        DDA_Line(2, 15, -4, 12);
        glLineWidth(1);

        // Subtle stripe markings on back
        glColor4f(darkR * 0.8f, darkG * 0.8f, darkB * 0.8f, 0.15f);
        for (int i = 0; i < 3; i++) {
            float sy = 10 + i * 4;
            FilledEllipse(-2 + i, sy, 8, 1.2f, darkR * 0.7f, darkG * 0.7f, darkB * 0.7f, 0.12f);
        }
    }

    glPopMatrix();
}

//============================================================
//  DRAW: STEPPING STONES
//============================================================
void drawSteppingStones()
{
    float f = getEffectiveDayFactor();
    float sR = 0.38f * f + 0.10f, sG = 0.35f * f + 0.09f, sB = 0.28f * f + 0.07f;

    float stones[][3] = {
        {155, -200, 6}, {148, -215, 5.5f}, {138, -228, 5},
        {126, -238, 5.5f}, {114, -250, 6}, {105, -264, 5},
    };
    int n = sizeof(stones) / sizeof(stones[0]);
    for (int i = 0; i < n; i++) {
        FilledEllipse(stones[i][0] + 1, stones[i][1] - 1, stones[i][2] + 1, stones[i][2] * 0.6f,
            0, 0, 0, 0.08f);
        float shade = 0.9f + 0.1f * sin(i * 2.5f);
        FilledEllipse(stones[i][0], stones[i][1], stones[i][2], stones[i][2] * 0.6f,
            sR * shade, sG * shade, sB * shade, 0.8f);
        FilledEllipse(stones[i][0] - 1, stones[i][1] + 1, stones[i][2] * 0.5f, stones[i][2] * 0.3f,
            sR * 1.2f, sG * 1.15f, sB * 1.1f, 0.2f);
    }
}

//============================================================
//  DRAW: DRAGONFLY
//============================================================
void drawDragonfly(float x, float y, float sc, float time)
{
    if (stormIntensity > 0.4f) return; // dragonfly hides in storm
    glPushMatrix(); applyTranslation(x, y); applyScaling(sc, sc);

    FilledEllipse(0, 0, 2, 12, 0.15f, 0.35f, 0.65f, 0.85f);
    FilledEllipse(0, -14, 1.2f, 6, 0.12f, 0.30f, 0.55f, 0.68f);

    float wAngle = sin(time * 12) * 15;
    float wingAlpha = 0.35f + 0.1f * sin(time * 8);

    glPushMatrix(); applyRotation(wAngle + 20);
    FilledEllipse(12, 3, 14, 4, 0.6f, 0.8f, 1.0f, wingAlpha);
    glPopMatrix();
    glPushMatrix(); applyRotation(-wAngle - 20);
    FilledEllipse(-12, 3, 14, 4, 0.6f, 0.8f, 1.0f, wingAlpha);
    glPopMatrix();
    glPushMatrix(); applyRotation(wAngle + 35);
    FilledEllipse(10, -2, 11, 3.5f, 0.5f, 0.7f, 0.9f, wingAlpha * 0.8f);
    glPopMatrix();
    glPushMatrix(); applyRotation(-wAngle - 35);
    FilledEllipse(-10, -2, 11, 3.5f, 0.5f, 0.7f, 0.9f, wingAlpha * 0.8f);
    glPopMatrix();

    FilledCircle(0, 12, 2.5f, 0.12f, 0.30f, 0.55f, 0.85f);
    FilledCircle(-1.5f, 13, 1.2f, 0.2f, 0.7f, 0.3f, 0.85f);
    FilledCircle(1.5f, 13, 1.2f, 0.2f, 0.7f, 0.3f, 0.85f);

    glPopMatrix();
}

//============================================================
//  DRAW: BUTTERFLY
//============================================================
void drawButterfly(float x, float y, float wp, bool goR,
    float r1, float g1, float b1, float r2, float g2, float b2) {
    if (stormIntensity > 0.3f) return; // butterflies hide in storm
    glPushMatrix(); applyTranslation(x, y); applyScaling(0.8f, 0.8f);
    if (!goR) applyReflectionY();
    float fa = sin(wp) * 30;

    glPushMatrix(); applyRotation(fa);
    glColor4f(r1, g1, b1, 0.85f);
    glBegin(GL_TRIANGLE_FAN); glVertex2f(0, 0);
    glVertex2f(3, 2); glVertex2f(8, 8); glVertex2f(14, 14); glVertex2f(18, 18);
    glVertex2f(20, 20); glVertex2f(18, 22); glVertex2f(14, 22); glVertex2f(8, 20);
    glVertex2f(4, 16); glVertex2f(2, 10); glVertex2f(1, 5); glEnd();
    FilledCircle(12, 16, 3.5f, r2, g2, b2, 0.7f); FilledCircle(12, 16, 1.8f, 1, 1, 1, 0.5f);
    glPopMatrix();

    glPushMatrix(); applyReflectionY(); applyRotation(fa);
    glColor4f(r1, g1, b1, 0.85f);
    glBegin(GL_TRIANGLE_FAN); glVertex2f(0, 0);
    glVertex2f(3, 2); glVertex2f(8, 8); glVertex2f(14, 14); glVertex2f(18, 18);
    glVertex2f(20, 20); glVertex2f(18, 22); glVertex2f(14, 22); glVertex2f(8, 20);
    glVertex2f(4, 16); glVertex2f(2, 10); glVertex2f(1, 5); glEnd();
    FilledCircle(12, 16, 3.5f, r2, g2, b2, 0.7f); FilledCircle(12, 16, 1.8f, 1, 1, 1, 0.5f);
    glPopMatrix();

    glPushMatrix(); applyRotation(-fa * 0.6f);
    glColor4f(r2, g2, b2, 0.8f);
    glBegin(GL_TRIANGLE_FAN); glVertex2f(0, 0);
    glVertex2f(2, -2); glVertex2f(6, -6); glVertex2f(10, -10);
    glVertex2f(14, -12); glVertex2f(15, -10); glVertex2f(13, -6);
    glVertex2f(8, -3); glVertex2f(4, -1); glEnd(); glPopMatrix();

    glPushMatrix(); applyReflectionY(); applyRotation(-fa * 0.6f);
    glColor4f(r2, g2, b2, 0.8f);
    glBegin(GL_TRIANGLE_FAN); glVertex2f(0, 0);
    glVertex2f(2, -2); glVertex2f(6, -6); glVertex2f(10, -10);
    glVertex2f(14, -12); glVertex2f(15, -10); glVertex2f(13, -6);
    glVertex2f(8, -3); glVertex2f(4, -1); glEnd(); glPopMatrix();

    FilledEllipse(0, 0, 1.8f, 10, 0.12f, 0.08f, 0.04f);
    FilledCircle(0, 10, 2.5f, 0.12f, 0.08f, 0.04f);
    glColor3f(0.12f, 0.08f, 0.04f); glLineWidth(1.2f);
    DDA_Line(0, 12, 5, 20); DDA_Line(0, 12, -5, 20); glLineWidth(1);
    FilledCircle(5, 20, 1.2f, 0.12f, 0.08f, 0.04f);
    FilledCircle(-5, 20, 1.2f, 0.12f, 0.08f, 0.04f);
    glPopMatrix();
}

//============================================================
//  DRAW: BIRD
//============================================================
void drawBird(float x, float y, float sc, float wp) {
    if (stormIntensity > 0.5f) return; // birds flee in storm
    glPushMatrix(); applyTranslation(x, y); applyScaling(sc, sc);
    float wy = 5 * sin(wp), f = dayFactor;
    float bR = 0.1f * f + 0.9f * (1 - f), bG = 0.1f * f + 0.9f * (1 - f), bB = 0.1f * f + 0.95f * (1 - f);
    float a = 0.75f * f + 0.85f * (1 - f);
    glColor4f(bR, bG, bB, a); glLineWidth(2.2f);
    glBegin(GL_LINE_STRIP);
    glVertex2f(-18, 0); glVertex2f(-9, wy); glVertex2f(0, 0);
    glVertex2f(9, wy); glVertex2f(18, 0); glEnd();
    FilledCircle(0, 0, 1.5f, bR, bG, bB, a); glLineWidth(1);
    if (f < 0.4f) GlowCircle(0, 0, 8, 1, 1, 1, 0.06f * (1 - f), 0);
    glPopMatrix();
}

//============================================================
//  DRAW: RAIN 
//============================================================
void drawRain()
{
    if (!rainEnabled && stormIntensity < 0.1f) return;

    float intensity = rainEnabled ? 1.0f : 0.0f;
    intensity += stormIntensity;
    if (intensity > 1) intensity = 1;

    int dropCount = (int)(200 * intensity);
    float shearAmount = 0.15f + stormIntensity * 0.35f;

    glPushMatrix(); applyShear(shearAmount, 0.0f);

    for (int i = 0; i < dropCount; i++) {
        float baseAlpha = 0.55f + 0.2f * sin(i * 0.3f);
        // Make rain MUCH more visible in day: use dark rain with high contrast
        float dropLen = 18 + stormIntensity * 12;

        if (dayFactor > 0.5f) {
            // DAY RAIN: darker, more opaque streaks against bright sky
            float dayRainR = 0.35f, dayRainG = 0.45f, dayRainB = 0.65f;
            float dayAlpha = baseAlpha * (0.55f + stormIntensity * 0.3f);

            // Main drop - thicker and darker for day visibility
            glLineWidth(2.5f);
            glColor4f(dayRainR, dayRainG, dayRainB, dayAlpha);
            glBegin(GL_LINES);
            glVertex2f(rainDrops[i][0], rainDrops[i][1]);
            glVertex2f(rainDrops[i][0] + 2, rainDrops[i][1] - dropLen);
            glEnd();
            glLineWidth(1.2f);
            glColor4f(0.7f, 0.78f, 0.9f, dayAlpha * 0.6f);
            glBegin(GL_LINES);
            glVertex2f(rainDrops[i][0] + 0.5f, rainDrops[i][1] - 2);
            glVertex2f(rainDrops[i][0] + 1.5f, rainDrops[i][1] - dropLen + 4);
            glEnd();

        }
        else {
            // NIGHT RAIN: brighter, glowing
            float nightAlpha = baseAlpha * (0.45f + stormIntensity * 0.35f);

            glLineWidth(2.0f);
            glColor4f(0.55f, 0.72f, 1.0f, nightAlpha);
            glBegin(GL_LINES);
            glVertex2f(rainDrops[i][0], rainDrops[i][1]);
            glVertex2f(rainDrops[i][0] + 2, rainDrops[i][1] - dropLen);
            glEnd();
            glLineWidth(1.0f);
            glColor4f(0.7f, 0.82f, 1.0f, nightAlpha * 0.4f);
            glBegin(GL_LINES);
            glVertex2f(rainDrops[i][0] + 0.8f, rainDrops[i][1] - 2);
            glVertex2f(rainDrops[i][0] + 1.8f, rainDrops[i][1] - dropLen + 4);
            glEnd();


        }
    }
    glLineWidth(1);
    glPopMatrix();

    // Rain splash on ground
    float splashAlpha = dayFactor > 0.5f ? 0.20f : 0.12f;
    for (int i = 0; i < 35; i++) {
        float sx = rainDrops[i * 5 % dropCount][0] + rainDrops[i * 5 % dropCount][1] * shearAmount;
        float sy = -155 + 5 * sin(i * 3.2f + glowPulse * 5);
        if (sy > -162 && sy < -148) {
            float splashBr = splashAlpha + 0.08f * sin(glowPulse * 8 + i);
            float splashSz = 2 + sin(glowPulse * 6 + i * 2);
            if (dayFactor > 0.5f)
                FilledCircle(sx, sy, splashSz, 0.4f, 0.5f, 0.65f, splashBr);
            else
                FilledCircle(sx, sy, splashSz, 0.6f, 0.75f, 1.0f, splashBr * 0.8f);
        }
    }

    // Rain mist effect at ground level (day: visible fog)
    if (dayFactor > 0.5f && (rainEnabled || stormIntensity > 0.2f)) {
        float mistAlpha = 0.04f + stormIntensity * 0.06f;
        glColor4f(0.6f, 0.65f, 0.72f, mistAlpha);
        glBegin(GL_QUADS);
        glVertex2f(-600, -165); glVertex2f(600, -165);
        glColor4f(0.6f, 0.65f, 0.72f, 0);
        glVertex2f(600, -130); glVertex2f(-600, -130);
        glEnd();
    }
}

//============================================================
//  DRAW: STORM RAIN 
//============================================================
void drawStormRain()
{
    if (stormIntensity < 0.2f) return;

    int dropCount = (int)(400 * stormIntensity);
    float shearAmount = 0.3f + stormIntensity * 0.25f;

    glPushMatrix(); applyShear(shearAmount, 0.0f);

    for (int i = 0; i < dropCount; i++) {
        float baseAlpha = 0.4f + 0.15f * sin(i * 0.2f);
        float dropLen = 22 + stormIntensity * 10;

        if (dayFactor > 0.5f) {
            glColor4f(0.30f, 0.38f, 0.55f, baseAlpha * stormIntensity * 0.6f);
            glLineWidth(2.2f);
        }
        else {
            glColor4f(0.50f, 0.65f, 0.90f, baseAlpha * stormIntensity * 0.5f);
            glLineWidth(1.8f);
        }

        glBegin(GL_LINES);
        glVertex2f(stormRainDrops[i][0], stormRainDrops[i][1]);
        glVertex2f(stormRainDrops[i][0] + 3, stormRainDrops[i][1] - dropLen);
        glEnd();
    }
    glLineWidth(1);
    glPopMatrix();
}

//============================================================
//  DRAW: SCATTERED WILDFLOWERS
//============================================================
void drawGroundWildflowers()
{
    drawTinyFlower(-570, -210, 1.0f, 0.5f, 0.8f, 0.5f);
    drawTinyFlower(-555, -225, 0.9f, 0.8f, 0.4f, 0.45f);
    drawTinyFlower(-530, -215, 0.6f, 0.5f, 1.0f, 0.5f);
    drawTinyFlower(-505, -230, 1.0f, 0.3f, 0.5f, 0.48f);
    drawTinyFlower(-200, -210, 0.8f, 0.6f, 0.9f, 0.5f);
    drawTinyFlower(-180, -222, 0.9f, 0.3f, 0.6f, 0.52f);
    drawTinyFlower(-160, -208, 0.5f, 0.8f, 0.5f, 0.48f);
    drawTinyFlower(160, -212, 1.0f, 0.4f, 0.7f, 0.5f);
    drawTinyFlower(185, -225, 0.7f, 0.9f, 0.3f, 0.48f);
    drawTinyFlower(250, -218, 0.9f, 0.5f, 0.9f, 0.52f);
    drawTinyFlower(280, -230, 0.4f, 0.7f, 1.0f, 0.5f);
    drawTinyFlower(350, -215, 1.0f, 0.6f, 0.2f, 0.48f);
    drawTinyFlower(380, -228, 0.6f, 0.8f, 0.6f, 0.5f);
    drawTinyFlower(430, -210, 0.8f, 0.3f, 0.8f, 0.52f);
    drawTinyFlower(470, -220, 1.0f, 0.7f, 0.4f, 0.48f);
    drawTinyFlower(-65, -200, 0.8f, 0.5f, 0.7f, 0.45f);
    drawTinyFlower(60, -205, 0.9f, 0.7f, 0.3f, 0.48f);
    drawTinyFlower(-58, -240, 0.6f, 0.4f, 0.9f, 0.5f);
    drawTinyFlower(58, -235, 1.0f, 0.3f, 0.6f, 0.48f);
    drawTinyFlower(-400, -310, 0.7f, 0.6f, 0.8f, 0.4f);
    drawTinyFlower(-350, -305, 0.9f, 0.4f, 0.5f, 0.42f);
    drawTinyFlower(300, -312, 0.5f, 0.8f, 0.6f, 0.4f);
    drawTinyFlower(400, -308, 1.0f, 0.5f, 0.3f, 0.42f);
}

//============================================================
//  DRAW: GARDEN ARCH
//============================================================
void drawGardenArch()
{
    float f = getEffectiveDayFactor();
    float aR = 0.45f * f + 0.12f, aG = 0.28f * f + 0.08f, aB = 0.12f * f + 0.04f;

    // Left pillar
    glBegin(GL_QUADS);
    glColor3f(aR * 0.8f, aG * 0.8f, aB * 0.8f);
    glVertex2f(-48, -155); glVertex2f(-42, -155);
    glColor3f(aR, aG, aB);
    glVertex2f(-42, -100); glVertex2f(-48, -100);
    glEnd();

    // Right pillar
    glBegin(GL_QUADS);
    glColor3f(aR * 0.8f, aG * 0.8f, aB * 0.8f);
    glVertex2f(42, -155); glVertex2f(48, -155);
    glColor3f(aR, aG, aB);
    glVertex2f(48, -100); glVertex2f(42, -100);
    glEnd();

    // Arch curve
    glColor3f(aR, aG, aB);
    glLineWidth(4);
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= 20; i++) {
        float t = i / 20.0f;
        float angle = PI * t;
        float ax = -45 + 90 * t;
        float ay = -100 + 20 * sin(angle);
        glVertex2f(ax, ay);
    }
    glEnd(); glLineWidth(1);

    // Climbing vine/roses
    float lR = 0.10f * f + 0.03f, lG = 0.40f * f + 0.09f, lB = 0.06f * f + 0.02f;
    for (int i = 0; i <= 10; i++) {
        float t = i / 10.0f;
        float angle = PI * t;
        float ax = -45 + 90 * t;
        float ay = -100 + 20 * sin(angle);
        FilledEllipse(ax + 3, ay + 2, 3, 5, lR, lG, lB, 0.7f);
        FilledEllipse(ax - 3, ay - 3, 3, 4, lR * 0.9f, lG * 0.9f, lB, 0.65f);
        if (i % 3 == 0) {
            for (int p = 0; p < 5; p++) {
                glPushMatrix(); applyTranslation(ax, ay + 4);
                applyRotation(p * 72);
                FilledEllipse(0, 2.5f, 1.5f, 2.5f, 0.85f, 0.15f, 0.2f, 0.7f);
                glPopMatrix();
            }
            FilledCircle(ax, ay + 4, 1.5f, 0.7f, 0.05f, 0.08f, 0.65f);
        }
    }
}

//============================================================
//  DRAW: STORM OVERLAY 
//============================================================
void drawStormOverlay()
{
    if (stormIntensity < 0.05f) return;

    // Subtle dark overlay
    float oa = stormIntensity * 0.12f;
    glColor4f(0.02f, 0.03f, 0.06f, oa);
    glBegin(GL_QUADS);
    glVertex2f(-600, -350); glVertex2f(600, -350);
    glVertex2f(600, 350); glVertex2f(-600, 350);
    glEnd();

    // Wind streaks in air
    if (stormIntensity > 0.5f) {
        float windA = (stormIntensity - 0.5f) * 0.1f;
        glColor4f(0.6f, 0.65f, 0.7f, windA);
        glLineWidth(1.0f);
        for (int i = 0; i < 15; i++) {
            float wx = -500 + (i * 73 + (int)(glowPulse * 50) % 200);
            float wy = -100 + i * 30 + sin(i * 2.1f) * 40;
            float wlen = 30 + 20 * stormIntensity;
            DDA_Line(wx, wy, wx + wlen, wy - 3);
        }
        glLineWidth(1);
    }
}

//============================================================
//  HUD
//============================================================
void drawText(float x, float y, const char* t) {
    glColor4f(1, 1, 1, 0.85f); glRasterPos2f(x, y);
    for (int i = 0; t[i]; i++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, t[i]);
}

void drawHUD() {
    char buf[256];
    const char* modeStr = isNight ? "Night" : "Day";
    const char* rainStr = rainEnabled ? "ON" : "OFF";
    const char* stormStr = stormEnabled ? "ON" : "OFF";
    const char* catStr = "Bench";
    if (catState == CAT_RUNNING_TO_HOUSE) catStr = "Running!";
    else if (catState == CAT_IN_HOUSE) catStr = "In House";
    else if (catState == CAT_RUNNING_TO_BENCH) catStr = "Returning";

    sprintf(buf, "Mode: %s | Rain: %s | Storm: %s | Cat: %s | Wind: %.1f | Speed: %.1f",
        modeStr, rainStr, stormStr, catStr, cloudSpeed, animSpeed);
    drawText(-590, 332, buf);
    drawText(-590, 318, "R:Rain  N:Day/Night  T:Storm  W/S:Wind  +/-:Speed  ESC:Exit");
}

//============================================================
//  DISPLAY
//============================================================
void display() {
    glClear(GL_COLOR_BUFFER_BIT); glLoadIdentity();
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_POINT_SMOOTH); glEnable(GL_LINE_SMOOTH);

    drawSky(); drawStars(); drawSun(); drawMoon();

    // Lightning bolt (behind clouds)
    drawLightning();

    // Airplane
    drawAirplane(planeX, planeY, 1.2f, planeGoingRight);

    drawClouds();
    drawStormClouds();
    drawMountains(); drawGround();

    drawPond();

    // Fence drawn FIRST so it appears in the background
    drawFence();

    // Cat house drawn AFTER fence so it appears in the FOREGROUND (no overlap)
    drawCatHouse(catHouseX, catHouseY);
    drawPath();
    drawGardenArch();

    // Trees - realistic
    drawTree(-560, -155, 1.15f);
    drawFruitTree(-460, -155, 1.2f, 0.85f, 0.15f, 0.1f, 3.2f);
    drawTree(-250, -155, 1.1f); drawTree(-160, -155, 1.3f);
    drawFruitTree(120, -155, 1.15f, 0.95f, 0.55f, 0.05f, 2.8f);
    drawTree(220, -155, 1.35f);
    drawFruitTree(340, -155, 1.1f, 0.8f, 0.1f, 0.15f, 2.5f);
    drawTree(460, -155, 1.25f);
    drawFruitTree(560, -155, 0.95f, 0.9f, 0.8f, 0.1f, 2.8f);

    // Bushes
    drawBush(-530, -158, 0.85f); drawBush(-310, -160, 0.75f);
    drawBush(-130, -158, 0.8f); drawBush(170, -160, 0.85f);
    drawBush(290, -158, 0.7f); drawBush(400, -160, 0.8f); drawBush(520, -158, 0.75f);
    drawBush(-580, -162, 0.6f); drawBush(-420, -160, 0.65f);
    drawBush(60, -160, 0.55f); drawBush(580, -162, 0.6f);

    drawAllFlowers();
    drawGroundWildflowers();
    drawSteppingStones();

    // Bench (at x=65)
    drawBench(65, -175);

    // Cat (drawn on bench or at current position)
    if (catState != CAT_IN_HOUSE) {
        bool isRunning = (catState == CAT_RUNNING_TO_HOUSE || catState == CAT_RUNNING_TO_BENCH);
        drawCat(catX, catY, 0.9f, catFacingRight, isRunning, catLegPhase, catTailWag);
    }

    // Lamp posts (bigger now)
    drawLampPost(-65, -175, 0.85f);
    drawLampPost(400, -175, 0.80f);

    // Butterflies
    drawButterfly(bfX, bfY, bfWingAngle, bfGoingRight, 0.92f, 0.45f, 0.08f, 0.95f, 0.65f, 0.12f);
    drawButterfly(bf2X, bf2Y, bf2WingAngle, bf2GoingRight, 0.3f, 0.5f, 0.9f, 0.4f, 0.7f, 0.95f);

    // Dragonfly
    drawDragonfly(dfX, dfY, 0.9f, dfTime);

    // Birds
    drawBird(bird1X, bird1Y, 1.0f, birdWing1);
    drawBird(bird1X + 40, bird1Y - 12, 0.75f, birdWing1 + 0.5f);
    drawBird(bird1X + 22, bird1Y + 8, 0.85f, birdWing1 + 1.0f);
    drawBird(bird2X, bird2Y, 0.9f, birdWing2);
    drawBird(bird2X + 32, bird2Y + 10, 1.05f, birdWing2 + 0.7f);

    drawFireflies();

    // Rain (regular and storm)
    drawRain();
    drawStormRain();

    // Storm overlay (wind streaks, darkening)
    drawStormOverlay();

    drawHUD();

    glDisable(GL_BLEND); glutSwapBuffers();
}

//============================================================
//  INIT
//============================================================
void initRain() {
    for (int i = 0; i < 200; i++) {
        rainDrops[i][0] = (float)(rand() % 1400 - 700);
        rainDrops[i][1] = (float)(rand() % 800 - 100);
    }
}

void initStormRain() {
    for (int i = 0; i < 400; i++) {
        stormRainDrops[i][0] = (float)(rand() % 1400 - 700);
        stormRainDrops[i][1] = (float)(rand() % 800 - 100);
    }
}

void initFireflies() {
    for (int i = 0; i < 20; i++) {
        fireflies[i][0] = (float)(rand() % 1000 - 500);
        fireflies[i][1] = (float)(rand() % 150 - 280);
        fireflies[i][2] = (float)(rand() % 360) * PI / 180;
    }
}

//============================================================
//  CAT STATE MACHINE UPDATE
//============================================================
void updateCat()
{
    bool shouldHide = (rainEnabled || stormEnabled);

    catAnimTime += 0.03f * animSpeed;
    catTailWag = sin(catAnimTime * 2);

    switch (catState) {
    case CAT_SITTING:
        catX = catBenchX;
        catY = catBenchY;
        catFacingRight = true;
        if (shouldHide) {
            catState = CAT_RUNNING_TO_HOUSE;
            catFacingRight = true; // house is to the right
            catTargetX = catHouseX;
            catTargetY = catHouseY + 10; // near door
        }
        break;

    case CAT_RUNNING_TO_HOUSE:
    {
        catLegPhase += 0.35f * animSpeed;
        float dx = catTargetX - catX;
        float dy = catTargetY - catY;
        float dist = sqrt(dx * dx + dy * dy);
        if (dist < 5) {
            catState = CAT_IN_HOUSE;
        }
        else {
            float speed = catRunSpeed * animSpeed;
            // Faster in storm
            if (stormEnabled) speed *= 1.5f;
            catX += (dx / dist) * speed;
            catY += (dy / dist) * speed;
            catFacingRight = (dx > 0);
        }
        break;
    }

    case CAT_IN_HOUSE:
        if (!shouldHide) {
            catState = CAT_RUNNING_TO_BENCH;
            catTargetX = catBenchX;
            catTargetY = catBenchY;
            catX = catHouseX;
            catY = catHouseY + 10;
        }
        break;

    case CAT_RUNNING_TO_BENCH:
    {
        catLegPhase += 0.25f * animSpeed;
        float dx = catTargetX - catX;
        float dy = catTargetY - catY;
        float dist = sqrt(dx * dx + dy * dy);
        if (dist < 5) {
            catState = CAT_SITTING;
            catX = catBenchX;
            catY = catBenchY;
        }
        else {
            float speed = catRunSpeed * animSpeed * 0.8f; // walks back leisurely
            catX += (dx / dist) * speed;
            catY += (dy / dist) * speed;
            catFacingRight = (dx > 0);
        }
        // If it starts raining again while returning, go back to house
        if (shouldHide) {
            catState = CAT_RUNNING_TO_HOUSE;
            catTargetX = catHouseX;
            catTargetY = catHouseY + 10;
        }
        break;
    }
    }
}

//============================================================
//  UPDATE
//============================================================
void update(int v) {
    float tgt = isNight ? 0 : 1;
    if (dayFactor < tgt) dayFactor += 0.008f * animSpeed;
    else if (dayFactor > tgt) dayFactor -= 0.008f * animSpeed;
    if (dayFactor < 0) dayFactor = 0; if (dayFactor > 1) dayFactor = 1;

    glowPulse += 0.03f * animSpeed; if (glowPulse > 2 * PI * 100) glowPulse = 0;

    float stormWindBoost = 1 + stormIntensity * 3;
    cloud1X += 0.35f * cloudSpeed * animSpeed * stormWindBoost;
    cloud2X += 0.25f * cloudSpeed * animSpeed * stormWindBoost;
    cloud3X += 0.40f * cloudSpeed * animSpeed * stormWindBoost;
    cloud4X += 0.30f * cloudSpeed * animSpeed * stormWindBoost;
    if (cloud1X > 750) cloud1X = -750; if (cloud2X > 750) cloud2X = -750;
    if (cloud3X > 750) cloud3X = -750; if (cloud4X > 750) cloud4X = -750;

    // === STORM UPDATE ===
    stormTargetIntensity = stormEnabled ? 1.0f : 0.0f;
    if (stormIntensity < stormTargetIntensity)
        stormIntensity += 0.005f * animSpeed;
    else if (stormIntensity > stormTargetIntensity)
        stormIntensity -= 0.008f * animSpeed;
    if (stormIntensity < 0) stormIntensity = 0;
    if (stormIntensity > 1) stormIntensity = 1;

    // Lightning
    if (stormIntensity > 0.4f) {
        lightningTimer -= 0.016f * animSpeed;
        if (lightningTimer <= 0) {
            lightningFlash = 0.8f + 0.2f * (float)(rand() % 100) / 100.0f;
            lightningTimer = 2.0f + (float)(rand() % 500) / 100.0f; // 2-7 seconds
        }
    }
    if (lightningFlash > 0) {
        lightningFlash -= 0.08f * animSpeed;
        if (lightningFlash < 0) lightningFlash = 0;
    }

    // Storm rain drops
    if (stormIntensity > 0.1f) {
        int dropCount = (int)(400 * stormIntensity);
        float fallSpeed = 8.0f + stormIntensity * 6.0f;
        for (int i = 0; i < dropCount; i++) {
            stormRainDrops[i][1] -= fallSpeed * animSpeed;
            stormRainDrops[i][0] += stormIntensity * 2.0f * animSpeed;
            if (stormRainDrops[i][1] < -380) {
                stormRainDrops[i][1] = 380;
                stormRainDrops[i][0] = (float)(rand() % 1400 - 700);
            }
            if (stormRainDrops[i][0] > 700) stormRainDrops[i][0] = -700;
        }
    }

    // Airplane
    if (planeGoingRight) {
        planeX += planeSpeed * animSpeed;
        if (planeX > 800) { planeX = 800; planeGoingRight = false; }
    }
    else {
        planeX -= planeSpeed * animSpeed;
        if (planeX < -800) { planeX = -800; planeGoingRight = true; }
    }
    planeY = 280 + 15 * sin(planeX * 0.003f);

    bfTime += 0.04f * animSpeed;
    if (bfGoingRight) { bfX += 0.9f * animSpeed; bfY = -60 + 30 * sin(bfTime * 2); if (bfX > 280) bfGoingRight = false; }
    else { bfX -= 0.9f * animSpeed; bfY = -60 + 30 * sin(bfTime * 2); if (bfX < -480) bfGoingRight = true; }
    bfWingAngle += 0.22f * animSpeed;

    bf2Time += 0.035f * animSpeed;
    if (bf2GoingRight) { bf2X += 0.7f * animSpeed; bf2Y = -40 + 25 * sin(bf2Time * 1.8f); if (bf2X > 350) bf2GoingRight = false; }
    else { bf2X -= 0.7f * animSpeed; bf2Y = -40 + 25 * sin(bf2Time * 1.8f); if (bf2X < -400) bf2GoingRight = true; }
    bf2WingAngle += 0.18f * animSpeed;

    // Dragonfly
    dfTime += 0.03f * animSpeed;
    dfX = 300 + 180 * sin(dfTime * 0.8f);
    dfY = -110 + 40 * sin(dfTime * 1.6f);

    bird1X += 0.65f * animSpeed; bird2X += 0.45f * animSpeed;
    birdWing1 += 0.10f * animSpeed; birdWing2 += 0.13f * animSpeed;
    if (bird1X > 700) bird1X = -700; if (bird2X > 700) bird2X = -700;

    flowerSway += 0.035f * swayDir * animSpeed;
    float maxSway = 4 + stormIntensity * 6;
    if (flowerSway > maxSway) swayDir = -1; if (flowerSway < -maxSway) swayDir = 1;

    fishTime += 0.02f * animSpeed;
    fish1X = 35 * sin(fishTime * 1.2f); fish1Dir = cos(fishTime * 1.2f) > 0 ? 1 : -1;
    fish2X = 25 * sin(fishTime * 0.9f + 2); fish2Dir = cos(fishTime * 0.9f + 2) > 0 ? 1 : -1;
    fish3X = 30 * sin(fishTime * 1.5f + 4); fish3Dir = cos(fishTime * 1.5f + 4) > 0 ? 1 : -1;

    // Regular rain update
    if (rainEnabled || stormIntensity > 0.1f) {
        float rainSpeed = 6.0f + stormIntensity * 4.0f;
        for (int i = 0; i < 200; i++) {
            rainDrops[i][1] -= rainSpeed * animSpeed;
            rainDrops[i][0] += stormIntensity * 1.5f * animSpeed;
            if (rainDrops[i][1] < -380) {
                rainDrops[i][1] = 380;
                rainDrops[i][0] = (float)(rand() % 1400 - 700);
            }
            if (rainDrops[i][0] > 700) rainDrops[i][0] = -700;
        }
    }

    // Fireflies
    for (int i = 0; i < 20; i++) {
        fireflies[i][0] += 0.25f * sin(fireflies[i][2]) * animSpeed;
        fireflies[i][1] += 0.15f * cos(fireflies[i][2] * 1.3f) * animSpeed;
        fireflies[i][2] += 0.02f * animSpeed;
        if (fireflies[i][0] > 580) fireflies[i][0] = -580; if (fireflies[i][0] < -580) fireflies[i][0] = 580;
        if (fireflies[i][1] > -100) fireflies[i][1] = -300; if (fireflies[i][1] < -340) fireflies[i][1] = -120;
    }

    waterShimmer += 0.04f * animSpeed;

    // Cat state machine
    updateCat();

    glutPostRedisplay(); glutTimerFunc(16, update, 0);
}

//============================================================
//  KEYBOARD
//============================================================
void keyboard(unsigned char key, int x, int y) {
    switch (key) {
    case 'r':case 'R':
        rainEnabled = !rainEnabled;
        if (rainEnabled) initRain();
        printf("Rain: %s\n", rainEnabled ? "ON" : "OFF");
        break;
    case 'n':case 'N':
        isNight = !isNight;
        printf("Mode: %s\n", isNight ? "Night" : "Day");
        break;
    case 't':case 'T':
        stormEnabled = !stormEnabled;
        if (stormEnabled) {
            rainEnabled = true; // storm includes rain
            initRain();
            initStormRain();
        }
        printf("Storm: %s\n", stormEnabled ? "ON" : "OFF");
        break;
    case 'w':case 'W':
        cloudSpeed += 0.5f; if (cloudSpeed > 5) cloudSpeed = 5;
        printf("Wind: %.1f\n", cloudSpeed);
        break;
    case 's':case 'S':
        cloudSpeed -= 0.5f; if (cloudSpeed < 0.1f) cloudSpeed = 0.1f;
        printf("Wind: %.1f\n", cloudSpeed);
        break;
    case '+':case '=':
        animSpeed += 0.5f; if (animSpeed > 5) animSpeed = 5;
        printf("Speed: %.1f\n", animSpeed);
        break;
    case '-':case '_':
        animSpeed -= 0.5f; if (animSpeed < 0.1f) animSpeed = 0.1f;
        printf("Speed: %.1f\n", animSpeed);
        break;
    case 27:exit(0); break;
    }
    glutPostRedisplay();
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h); glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluOrtho2D(-600, 600, -350, 350); glMatrixMode(GL_MODELVIEW); glLoadIdentity();
}

void printInfo() {
    printf("\n================================================\n");
    printf("                    GARDEN SCENE                \n");
    printf("================================================\n");
    printf("   R:Rain  N:Day/Night  T:Storm                 \n");
    printf("   W/S:Wind  +/-:Speed  ESC:Exit                \n");
    printf("================================================\n");
    printf("   Features:                                     \n");
    printf("   - Realistic trees with natural canopy         \n");
    printf("   - Grey-white cat on bench                     \n");
    printf("   - Cat house outside fence                     \n");
    printf("   - Cat runs to house in rain/storm             \n");
    printf("   - Storm mode with lightning & heavy rain      \n");
    printf("   - Visible rain in both day and night          \n");
    printf("   - Bigger lamp posts with dual lights          \n");
    printf("================================================\n\n");
}

int main(int argc, char** argv) {
    srand((unsigned)time(NULL)); printInfo();
    glutInit(&argc, argv); glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT); glutInitWindowPosition(50, 20);
    glutCreateWindow("Interactive Garden Scene Simulation Using OpenGL");
    glClearColor(0, 0, 0, 1);
    glMatrixMode(GL_PROJECTION); glLoadIdentity(); gluOrtho2D(-600, 600, -350, 350);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_POINT_SMOOTH); glEnable(GL_LINE_SMOOTH);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST); glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    initRain(); initStormRain(); initFireflies();

    // Initialize cat position on bench
    catX = catBenchX;
    catY = catBenchY;
    catState = CAT_SITTING;

    glutDisplayFunc(display); glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard); glutTimerFunc(16, update, 0);
    glutMainLoop(); return 0;
}