// Library
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

// Enemies
static void SpawnEnemy(void) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        enemies[i].active = false;
    }
    // 1st Wave
    for (int i = 0; i < 6; i++) {
        enemies[i].pos = (Vector2){PLAY_X + 40.0f + i  * 65.0f, -60.0f - i * 15.0f};
        enemies[i].radius = 12.0f;
        enemies[i].hp = enemies[i].maxHp = 5;
        enemies[i].shootTimer = 1.0f + i * 0.3f;
        enemies[i].shootInterval = 2.0f;
        enemies[i].active = true;
        enemies[i].color = (Color){255, 80, 120, 255};
    }
    // 2nd Wave
    for (int i = 6; i < 12; i++) {
        enemies[i].pos = (Vector2){PLAY_X + 40.0f + (i - 6) * 60.0f, -180.0f - (i - 6) * 15.0f};
        enemies[i].radius = 12.0f;
        enemies[i].hp = enemies[i].maxHp = 5;
        enemies[i].shootTimer = 1.5f + (i - 6) * 0.2f;
        enemies[i].shootInterval = 2.5f;
        enemies[i].active = true;
        enemies[i].color = (Color){255, 160, 60, 255};
    }
}

static void UpdateEnemies(float dt) {
    bool anyActive = false;

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) continue;
        anyActive = true;

        float targetY = (i < 6) ? 100.0f : 170.0f;

        float baseX = (i < 6)
                ? PLAY_X + 40.0f + i * 65.0f
                : PLAY_X + 65.0f + (i - 6) * 60.0f;
                
        if (enemies[i].pos.y < targetY) {
            enemies[i].pos.y += 2.5f;
            enemies[i].pos.x  = baseX;
            enemies[i].moveTimer = 0;
        } else {
            enemies[i].moveTimer += dt;
            enemies[i].pos.x = baseX + sinf(enemies[i].moveTimer * 1.5f) * 10.0f;

            enemies[i].pos.x = Clampf(enemies[i].pos.x,
                PLAY_X + enemies[i].radius,
                PLAY_X + PLAY_W - enemies[i].radius);
        }
    }
}

static void DrawEnemies(void) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) continue;
        DrawCircleV(enemies[i].pos, enemies[i].radius, enemies[i].color);
        DrawCircleV(enemies[i].pos, enemies[i].radius - 4.0f, (Color){255, 255, 255, 200});
    }
}

// HUD
static void DrawHUD(void) {
    // Right Border
    DrawRectangle(PLAY_X + PLAY_W, 0, SCREEN_W - (PLAY_X + PLAY_W), SCREEN_H, (Color){15, 10, 25, 255});
    DrawRectangleLines(PLAY_X + PLAY_W, 0, SCREEN_W - (PLAY_X + PLAY_W), SCREEN_H, DARKGRAY);

    // Left Border
    DrawRectangle(0, 0, PLAY_X, SCREEN_H, (Color){15, 10, 25, 255});
    DrawRectangleLines(0, 0, PLAY_X, SCREEN_H, DARKGRAY);
}

// Background with Stars
static void DrawBackground(void) {
    static float starY[80] = {0};
    static bool starsInit = false;

        if (!starsInit) {
            for (int i = 0; i < 80; i++) {
                starY[i] = (float)GetRandomValue(0, SCREEN_H);
        }
        starsInit = true;
    }
    
    ClearBackground((Color){5, 0, 15, 255});

    for (int i = 0; i < 80; i++) {
        starY[i] += 0.8f + (i % 3) * 0.4f;

        if (starY[i] > SCREEN_H) starY[i] = 0;

        int x = (i * 173 + 31) % PLAY_W + PLAY_X;

        int b = 120 + (i % 5) * 27;
        DrawPixel(x, (int)starY[i], (Color){b, b, b, 255});
    }
}

static void GameInit(void) {
    SpawnEnemy();
    phase = PHASE_ENEMIES;
}

// Main Game Loop
int main(void) {
    InitWindow(SCREEN_W, SCREEN_H, "Danmaku Game");
    InitAudioDevice();
    
    Music musicEnemy = LoadMusicStream("assets/Touhou 7 - Paradise  Deep Mountain (Stage 1).mp3");
    Music musicBoss = LoadMusicStream("assets/Touhou 7 - Letty Whiterock's Theme - Crystallized Silver (Boss 1).mp3");

    Music currentTrack = musicEnemy;
    PlayMusicStream(musicEnemy);

    float musicSwitchTimer = 0.0f;
    float musicSwitchInterval = 120.0f; // Switch tracks every 120 seconds
    bool musicSwitched = false;

    SetTargetFPS(60);

    GameInit();

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        if (IsKeyPressed(KEY_P)) {
            paused = !paused;
            if (paused) {
                PauseMusicStream(currentTrack);
            } else {
                ResumeMusicStream(currentTrack);
            }
        }

        UpdateMusicStream(currentTrack);
        if (!paused) {
            if (!musicSwitched) {
                musicSwitchTimer += GetFrameTime();
                if (musicSwitchTimer >= musicSwitchInterval) {
                    StopMusicStream(currentTrack);
                    currentTrack = musicBoss;
                    PlayMusicStream(currentTrack);
                    musicSwitched = true;
                }
            }
            UpdateEnemies(dt);
        }
        
        BeginDrawing();
        DrawBackground();
        DrawEnemies();
        DrawHUD();

        if (paused) {
            DrawRectangle(PLAY_X, 0, PLAY_W, SCREEN_H, (Color){0,0,0,140});

            const char *msg = "PAUSED";
            int fs = 30;
            int w = MeasureText(msg, fs);
            DrawText(msg, PLAY_X + (PLAY_W -w) / 2, SCREEN_H / 2 - fs / 2, fs, WHITE);
        }

        EndDrawing();
    }
    
    UnloadMusicStream(musicEnemy);
    UnloadMusicStream(musicBoss);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}