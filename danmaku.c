// Libary
#include "raylib.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// Constants

#define SCREEN_W 480
#define SCREEN_H 640
#define PLAY_X 40
#define PLAY_W 400
#define MAX_BULLETS 1200
#define MAX_ENEMY_BULLETS 800
#define MAX_ENEMIES 12
#define PLAYER_RADIUS 5.0f
#define PLAYER_GFX_RADIUS 14.0f

typedef enum {
    PHASE_WIN,
    PHASE_ENEMIES,
    PHASE_BOSS,
    PHASE_DEAD
} GamePhase;

typedef enum {
    ATTACK_SPREAD,
    ATTACK_SPIRAL,
    ATTACK_AIMED,
    ATTACK_RING
} BossAttack;

// Blueprint

typedef struct {
    Vector2 pos;
    float radius;
    int lives;
    int bombs;
    float invincTimer;
    float shootTimer;
    bool dead;
} Player;

typedef struct {
    Vector2 pos;
    Vector2 vel;
    bool active;
} Bullet;

typedef struct {
    Vector2 pos;
    Vector2 vel;
    float radius;
    Color color;
    bool active;
} EnemyBullet;

typedef struct {
    Vector2 pos;
    float radius;
    int hp;
    int maxHp;
    float shootTimer;
    float shootInterval;
    float moveTimer;
    bool active;
    Color color;
} Enemy;

typedef struct {
    Vector2 pos;
    float radius;
    int hp;
    int maxHp;
    float attackTimer;
    float phaseTimer;
    BossAttack currentAttack;
    int spiralAngle;
    float moveAngle;
    bool active;
    int phase;
} Boss;

typedef struct {
    Vector2 pos;
    Vector2 vel;
    float life;
    float maxLife;
    Color color;
    bool active;
} Particle;

#define MAX_PARTICLES 256

// Global State

static Player       player;
static Bullet       playerBullets[MAX_BULLETS];
static EnemyBullet  enemyBullets[MAX_ENEMY_BULLETS];
static Enemy        enemies[MAX_ENEMIES];
static Boss         boss;
static Particle     particles[MAX_PARTICLES];
static GamePhase    phase;
static int          score;
static float        stgTimer;    // counts how long the player has survived (for scoring)
static bool         paused;

// Fungsi Pembantu
static float Dist(Vector2 a, Vector2 b) {
    return sqrtf((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y));
}

static Vector2 DirectionTo(Vector2 from, Vector2 to) {
    Vector2 dir = {to.x - from.x, to.y -from.y};
    float len = sqrtf((dir.x * dir.x) + (dir.y * dir.y));
    if (len == 0) return (Vector2){0, 0};
    return (Vector2){dir.x / len, dir.y / len};
}

static float Clampf(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

// HUD
int DrawHUD(void) {
    // Right Border
    DrawRectangle(PLAY_X + PLAY_W, 0, SCREEN_W - PLAY_X - PLAY_W, SCREEN_H, (Color){15, 10, 25, 255});
    DrawRectangleLines(PLAY_X + PLAY_W, 0, SCREEN_W - PLAY_X - PLAY_W, SCREEN_H, DARKGRAY);

    // Left Border
    DrawRectangle(0, 0, PLAY_X, SCREEN_H, (Color){15, 10, 25, 255});
    DrawRectangleLines(0, 0, PLAY_X, SCREEN_H, DARKGRAY);
}

int main(void) {
    InitWindow(SCREEN_W, SCREEN_H, "Danmaku Game");
    InitAudioDevice();
    
    Music music = LoadMusicStream("assets/Touhou 7 - Paradise  Deep Mountain (Stage 1).mp3");
    music.looping = true;
    PlayMusicStream(music);

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        UpdateMusicStream(music);

        BeginDrawing();

        DrawHUD();
        ClearBackground((Color){20, 20, 40, 255});
        DrawText("Window OK — You can see the border now", 60, SCREEN_H / 2 - 10, 18,
                 RAYWHITE);
        DrawText("Press ESC or close the window to quit", 60, SCREEN_H / 2 + 20, 18, LIGHTGRAY);
        EndDrawing();
    }
    
    UnloadMusicStream(music);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}