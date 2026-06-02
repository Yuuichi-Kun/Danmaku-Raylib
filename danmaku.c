// Library
#include "raylib.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Constants

#define SCREEN_W 480
#define SCREEN_H 640
#define PLAY_X 40
#define PLAY_W 400
#define MAX_BULLETS 1200
#define MAX_ENEMY_BULLETS 800
#define MAX_ENEMIES 12
#define PLAYER_SPEED 240.0f
#define PLAYER_FOCUS 120.0f
#define BULLET_RADIUS 5.0f
#define ENEMY_BULLET_R 5.0f
#define PLAYER_RADIUS 5.0f
#define PLAYER_GFX_R 14.0f
#define FPS 60

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

static Vector2 DirectionTo(Vector2 from, Vector2 to, float speed) {
    float dx = to.x - from.x;
    float dy = to.y - from.y;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.001f) return (Vector2){0, speed};
    return (Vector2){dx / len * speed, dy / len * speed};
}

static float Clampf(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

// NPC Bullets
static void FireEnemyBullet(Vector2 pos, Vector2 vel, float radius, Color color) {
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (!enemyBullets[i].active) {
            enemyBullets[i] = (EnemyBullet){pos, vel, radius, color, true};
            return;
        }
    }
}

static void FireAimed(Vector2 from, float speed, float r, Color c) {
    Vector2 vel = DirectionTo(from, player.pos, speed);
    FireEnemyBullet(from, vel, r, c);
}

static void FireSpread (Vector2 from, float centerDeg, float spreadDeg, int count, float speed, float r, Color c) {
    float startAngle = centerDeg - spreadDeg / 2.0f;
    float step = (count > 1) ? spreadDeg / (float)(count - 1) : 0;
    for (int i = 0; i < count; i++) {
        float a = (startAngle + i * step) * DEG2RAD;
        Vector2 vel = (Vector2){ cosf(a) * speed, sinf(a) * speed };
        FireEnemyBullet(from, vel, r, c);
    }
}

static void FireRing (Vector2 from, int count, float speed, float offsetDeg, float r, Color c) {
    for (int i = 0; i < count; i++) {
        float a = (360.0f / count * i + offsetDeg) * DEG2RAD;
        Vector2 vel = { cosf(a) * speed, sinf(a) * speed };
        FireEnemyBullet(from, vel, r, c);
    }
}

// Player
static void InitPlayer(void) {
    // Inisialisasi player
    player.pos          = (Vector2){ PLAY_X + PLAY_W / 2.0f, SCREEN_H - 100.0f };  // tengah area bermain
    player.radius       = PLAYER_RADIUS;
    player.lives        = 3;
    player.bombs        = 3;
    player.invincTimer  = 0.0f;
    player.shootTimer   = 0.0f;
    player.dead         = false;
}

static void FirePlayerBullet(void) {
    float offsets[] = { -8.0f, 8.0f };
    for (int k = 0; k < 2; k++) {
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (!playerBullets[i].active) {
                playerBullets[i].pos = (Vector2){ player.pos.x + offsets[k], player.pos.y - 10 };
                playerBullets[i].vel = (Vector2){ 0, -14.0f, };
                playerBullets[i].active = true;
                break;
            }
        }
    }
}

static void UpdatePlayerBullets(void) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!playerBullets[i].active) continue;
        playerBullets[i].pos.x += playerBullets[i].vel.x;
        playerBullets[i].pos.y += playerBullets[i].vel.y;
        if (playerBullets[i].pos.y < -10) playerBullets[i].active = false;
    }
}

static void DrawPlayerBullets(void) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!playerBullets[i].active) continue;
        DrawCircleV(playerBullets[i].pos, BULLET_RADIUS, SKYBLUE);
        DrawCircleV(playerBullets[i].pos, BULLET_RADIUS - 2.0f, WHITE);
    }
}

static void UpdatePlayer(float dt) {
// --- Update: baca input ---
    if (player.dead) return;

    // focus mode saat shift
    bool focus = IsKeyDown(KEY_LEFT_SHIFT);
    float speed = focus ? PLAYER_FOCUS : PLAYER_SPEED;

     // move player
    if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) player.pos.y -= speed * dt;
    if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) player.pos.y += speed * dt;
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) player.pos.x -= speed * dt;
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) player.pos.x += speed * dt;

    // Clamp dalam area bermain
    player.pos.x = Clampf(player.pos.x, PLAY_X + PLAYER_GFX_R, PLAY_X + PLAY_W - PLAYER_GFX_R);
    player.pos.y = Clampf(player.pos.y, PLAYER_GFX_R, SCREEN_H - PLAYER_GFX_R);

    player.shootTimer -= dt;
    if (IsKeyDown(KEY_Z) && player.shootTimer <= 0) {
        FirePlayerBullet();
        player.shootTimer = 0.08f;
    }

    // Turunkan timer
    if (player.invincTimer > 0) player.invincTimer -= dt;
}

static void DrawPlayer(void) {
    if (!player.dead) {
        // Kedip saat invinc
        bool blink = (int)(player.invincTimer / 0.1f) % 2 == 0;
        if (player.invincTimer <= 0 || blink) {
            DrawCircleV(player.pos, PLAYER_GFX_R, BLUE);
            DrawCircleV(player.pos, PLAYER_RADIUS, WHITE); // hitbox kecil
        }
    }
}

// Enemies
static void InitEnemies(void) {
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

        enemies[i].shootTimer -= dt;
        if (enemies[i].shootTimer <= 0) {
            enemies[i].shootTimer = enemies[i].shootInterval;
            FireAimed(enemies[i].pos, 3.5f, ENEMY_BULLET_R - 1, enemies[i].color);
        }

        if (enemies[i].pos.y > SCREEN_H + 30) {
            enemies[i].active = false;
        }
    }

    if (!anyActive && phase == PHASE_ENEMIES) {
        phase = PHASE_BOSS;
        for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
            enemyBullets[i].active = false;
        }
    }
}

static void UpdateEnemyBullets(float dt) {
    (void)dt;
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (!enemyBullets[i].active) continue;
        enemyBullets[i].pos.x += enemyBullets[i].vel.x;
        enemyBullets[i].pos.y += enemyBullets[i].vel.y;

        if (enemyBullets[i].pos.x < PLAY_X - 20 ||
            enemyBullets[i].pos.x > PLAY_X + PLAY_W + 20 ||
            enemyBullets[i].pos.y < -20 ||
            enemyBullets[i].pos.y > SCREEN_H + 20) {
            enemyBullets[i].active = false;
        }
    }
}

static void DrawEnemyBullets(void) {
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (!enemyBullets[i].active) continue;
        float r = enemyBullets[i].radius;
        DrawCircleV(enemyBullets[i].pos, r, enemyBullets[i].color);
        DrawCircleV(enemyBullets[i].pos, r * 0.45f, WHITE);
    }
}

static void DrawEnemies(void) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) continue;
        DrawCircleV(enemies[i].pos, enemies[i].radius, enemies[i].color);
        DrawCircleV(enemies[i].pos, enemies[i].radius - 4.0f, (Color){255, 255, 255, 200});

        float hpRatio = (float)enemies[i].hp / enemies[i].maxHp;
        DrawRectangle((int)enemies[i].pos.x - 18, (int)enemies[i].pos.y + 22, 36, 4, DARKGRAY);
        DrawRectangle((int)enemies[i].pos.x - 18, (int)enemies[i].pos.y + 22, (int)(36 * hpRatio), 4, GREEN);
    }
}

// Collision Detection
static void handleCollisions(void) {
    // Player Bullet vs Enemies
    for (int b = 0; b < MAX_BULLETS; b++) {
        if (!playerBullets[b].active) continue;
        for (int e = 0; e < MAX_ENEMIES; e++) {
            if (!enemies[e].active) continue;
            if (Dist(playerBullets[b].pos, enemies[e].pos) < BULLET_RADIUS + enemies[e].radius) {
                playerBullets[b].active = false;
                enemies[e].hp--;
                score += 50;
                if (enemies[e].hp <= 0) {
                    enemies[e].active = false;
                    score += 200;   
                }
                break;
            }
        }
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
        if (!paused) {
            starY[i] += 0.8f + (i % 3) * 0.4f;
            if (starY[i] > SCREEN_H) starY[i] = 0;
        }

        int x = (i * 173 + 31) % PLAY_W + PLAY_X;

        int b = 120 + (i % 5) * 27;
        DrawPixel(x, (int)starY[i], (Color){b, b, b, 255});
    }
}

static void GameInit(void) {
    InitPlayer();
    InitEnemies();
    // Reset state global lainnya
    score     = 0;
    phase     = PHASE_ENEMIES;
    paused    = false;
    stgTimer  = 0.0f;

    memset(particles, 0, sizeof(particles));
    memset(playerBullets, 0, sizeof(playerBullets));
    memset(enemyBullets, 0, sizeof(enemyBullets));
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
            handleCollisions();
            UpdatePlayer(dt);
            UpdatePlayerBullets();
            UpdateEnemies(dt);
            UpdateEnemyBullets(dt);
            stgTimer += dt;
        }

        BeginDrawing();
        DrawBackground();
        DrawPlayerBullets();
        DrawEnemies();
        DrawEnemyBullets();
        DrawPlayer();
        DrawHUD();

        if (paused) {
            DrawRectangle(PLAY_X, 0, PLAY_W, SCREEN_H, (Color){0, 0, 0, 140});
            const char *msg = "PAUSED";
            int fs = 30;
            int w = MeasureText(msg, fs);
            DrawText(msg, PLAY_X + (PLAY_W - w) / 2, SCREEN_H / 2 - fs / 2, fs, WHITE);
        }

        EndDrawing();
    }
    
    UnloadMusicStream(musicEnemy);
    UnloadMusicStream(musicBoss);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}