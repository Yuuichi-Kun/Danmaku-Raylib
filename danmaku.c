// Library
#include "raylib.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Constants

#define PLAY_W 400
#define HUD_SIDE_W 100
#define PLAY_X HUD_SIDE_W
#define SCREEN_W (PLAY_X + PLAY_W + HUD_SIDE_W)
#define SCREEN_H 640
#define MAX_BULLETS 1200
#define MAX_ENEMY_BULLETS 1400
#define BOSS_MAX_HP 450
#define BOSS_RAGE_HP 380
#define BOSS_ENTER_Y 105.0f
#define BOSS_TRANSFORM_TIME 2.8f
#define MAX_ENEMIES 12
#define PLAYER_SPEED 240.0f
#define PLAYER_FOCUS 120.0f
#define BULLET_RADIUS 5.0f
#define ENEMY_BULLET_R 5.0f
#define PLAYER_RADIUS 5.0f
#define PLAYER_GFX_R 14.0f
#define PLAYER_MAX_BOMBS 3
#define BOMB_DURATION 2.5f
#define BOMB_RADIUS_MAX 300.0f
#define BOMB_STUN_TIME 0.15f
#define RAGE_PATTERNS_PER_PHASE 8
#define FPS 60
#define WAVE_COUNT (sizeof(WAVES) / sizeof(WAVES[0]))
#define DEV_CODE "Tohok"
#define DEV_INPUT_MAX 15

typedef enum {
    PHASE_MENU,
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

typedef enum {
    ENTER_TOP,
    ENTER_LEFT,
    ENTER_RIGHT
} EnemyEnter;

typedef enum {
    EXIT_BOTTOM,
    EXIT_LEFT,
    EXIT_RIGHT,
    EXIT_TOP
} EnemyExit;

typedef enum {
    MOVE_HOVER,
    MOVE_SWEEP,
    MOVE_ZIGZAG
} EnemyMove;

// Blueprint

typedef struct {
    Vector2 pos;
    float radius;
    int lives;
    int bombs;
    float invincTimer;
    float shootTimer;
    float bombTimer;
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
    float stayTimer;
    float stayDuration;
    bool leaving;
    bool active;
    Color color;
    EnemyEnter enterDir;
    EnemyExit exitDir;
    EnemyMove moveType;
    float baseX;
    float baseY;
    float stunTimer;
} Enemy;

typedef struct {
    Vector2 pos;
    float radius;
    int hp;
    int maxHp;
    float attackTimer;
    float burstTimer;
    float phaseTimer;
    float enterTimer;
    BossAttack currentAttack;
    int spiralAngle;
    float moveAngle;
    bool active;
    bool entered;
    int phase;
    int patternStep;
    bool rage;
    bool transforming;
    float transformTimer;
    float stunTimer;
    char name[24];
} Boss;

typedef struct {
    Vector2 pos;
    Vector2 vel;
    float life;
    float maxLife;
    float size;
    Color color;
    bool active;
} Particle;

typedef struct {
    float spawnTime;
    int startIndex;
    int count;
    float startX;
    float spacingX;
    float startY;
    float targetY;
    float shootInterval;
    float stayDuration;
    Color color;
    EnemyEnter enterDir;
    EnemyExit exitDir;
    EnemyMove moveType;
} EnemyWave;

#define MAX_PARTICLES 512

// Global State

static Player       player;
static Bullet       playerBullets[MAX_BULLETS];
static EnemyBullet  enemyBullets[MAX_ENEMY_BULLETS];
static Enemy        enemies[MAX_ENEMIES];
static Boss         boss;
static Particle     particles[MAX_PARTICLES];
static GamePhase    phase;
static int          score;
static float        stgTimer;    // Hitung sudah berapa lama karakter hidup untuk score
static bool         paused;
static int          nextWave = 0;
static bool         playerFocus = false;
static char         devInput[DEV_INPUT_MAX + 1] = {0};
static int          devLen = 0;
static bool         devMode = false;
static float        devMsgTimer = 0.0f;
static int          devFeedback = 0; /* 0=none 1=on 2=off 3=wrong */

static void BossEnterRage(void);
static void BossUpdatePhase(void);

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

static const EnemyWave WAVES[] = {
    // time,  idx, cnt, startX,        spacingX, startY,  targetY, interval, stay, color,                  enter,       exit,        move
    { 0.0f,   0,   6,  PLAY_X+40.0f,  60.0f,  -60.0f,   110.0f,  2.0f,  5.0f, {255, 80,  120, 255}, ENTER_TOP,   EXIT_BOTTOM, MOVE_HOVER   },
    { 6.0f,   6,   6,  PLAY_X+40.0f,  60.0f, -180.0f,   185.0f,  2.5f,  5.0f, {255, 160,  60, 255}, ENTER_TOP,   EXIT_BOTTOM, MOVE_HOVER   },
    { 18.0f,  0,   5,  -60.0f,        0.0f,    80.0f,   130.0f,  2.0f,  6.0f, {100, 180, 255, 255}, ENTER_LEFT,  EXIT_RIGHT,  MOVE_SWEEP   }, // sweep left→right
    { 24.0f,  6,   5,  PLAY_X+PLAY_W+60.0f, 0.0f, 185.0f, 185.0f, 2.0f, 6.0f, {180, 100, 255, 255}, ENTER_RIGHT, EXIT_LEFT,   MOVE_SWEEP   }, // sweep right→left
    { 36.0f,  0,   6,  PLAY_X+40.0f,  60.0f,  -60.0f,   110.0f,  1.8f,  4.0f, {255,  80, 120, 255}, ENTER_TOP,   EXIT_LEFT,   MOVE_HOVER   }, // exit left
    { 42.0f,  6,   6,  PLAY_X+40.0f,  60.0f, -180.0f,   185.0f,  1.8f,  4.0f, {255, 220,  60, 255}, ENTER_TOP,   EXIT_RIGHT,  MOVE_HOVER   }, // exit right
    { 54.0f,  0,   4,  -60.0f,        0.0f,    60.0f,   148.0f,  1.5f,  5.0f, {60,  220, 180, 255}, ENTER_LEFT,  EXIT_BOTTOM, MOVE_ZIGZAG  }, // from left, zigzag
    { 54.0f,  6,   4,  PLAY_X+PLAY_W+60.0f, 0.0f, 60.0f, 220.0f, 1.5f, 5.0f, {255, 120,  60, 255}, ENTER_RIGHT, EXIT_BOTTOM, MOVE_ZIGZAG  }, // from right, zigzag
    { 70.0f,  0,   6,  PLAY_X+40.0f,  60.0f,  -60.0f,   110.0f,  1.5f,  5.0f, {255,  80, 120, 255}, ENTER_TOP,   EXIT_BOTTOM, MOVE_HOVER   },
    { 76.0f,  6,   6,  PLAY_X+40.0f,  60.0f, -180.0f,   185.0f,  1.5f,  5.0f, {255, 160,  60, 255}, ENTER_TOP,   EXIT_BOTTOM, MOVE_HOVER   },
    { 90.0f,  0,   5,  -60.0f,        0.0f,   110.0f,   110.0f,  1.3f,  6.0f, {100, 180, 255, 255}, ENTER_LEFT,  EXIT_RIGHT,  MOVE_SWEEP   },
    { 96.0f,  6,   5,  PLAY_X+PLAY_W+60.0f, 0.0f, 185.0f, 185.0f, 1.3f, 6.0f, {180, 100, 255, 255}, ENTER_RIGHT, EXIT_LEFT,   MOVE_SWEEP   },
    { 108.0f, 0,   6,  PLAY_X+40.0f,  60.0f,  -60.0f,   110.0f,  1.3f,  4.0f, {255, 220,  60, 255}, ENTER_TOP,   EXIT_RIGHT,  MOVE_HOVER   },
    { 108.0f, 6,   6,  PLAY_X+40.0f,  60.0f, -180.0f,   185.0f,  1.3f,  4.0f, {255,  80, 120, 255}, ENTER_TOP,   EXIT_LEFT,   MOVE_HOVER   },
    { 124.0f, 0,   4,  -60.0f,        0.0f,    80.0f,   148.0f,  1.2f,  5.0f, {60,  220, 180, 255}, ENTER_LEFT,  EXIT_BOTTOM, MOVE_ZIGZAG  },
    { 124.0f, 6,   4,  PLAY_X+PLAY_W+60.0f, 0.0f, 220.0f, 220.0f, 1.2f, 5.0f, {255, 120,  60, 255}, ENTER_RIGHT, EXIT_BOTTOM, MOVE_ZIGZAG  },
    { 138.0f, 0,   6,  PLAY_X+40.0f,  60.0f,  -60.0f,   110.0f,  1.1f,  4.0f, {255,  80, 120, 255}, ENTER_TOP,   EXIT_BOTTOM, MOVE_HOVER   },
    { 144.0f, 6,   6,  PLAY_X+40.0f,  60.0f, -180.0f,   185.0f,  1.1f,  4.0f, {255, 160,  60, 255}, ENTER_TOP,   EXIT_BOTTOM, MOVE_HOVER   },
    { 153.0f, 0,   6,  PLAY_X+40.0f,  60.0f,  -60.0f,   110.0f,  1.0f,  3.5f, {255, 220, 255, 255}, ENTER_TOP,   EXIT_LEFT,   MOVE_HOVER   },
    { 153.0f, 6,   6,  PLAY_X+40.0f,  60.0f, -180.0f,   185.0f,  1.0f,  3.5f, {255, 255, 200, 255}, ENTER_TOP,   EXIT_RIGHT,  MOVE_HOVER   },
};

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

static void EnemyShootPattern(Enemy *e) {
    float spd = 3.2f;
    float r = ENEMY_BULLET_R - 1.0f;
    Color c = e->color;
    float aim = atan2f(player.pos.y - e->pos.y, player.pos.x - e->pos.x) * RAD2DEG;
    int pat = ((int)(e->moveTimer * 0.8f)) % 3;

    if (pat == 0) {
        FireAimed(e->pos, spd, r, c);
    } else if (pat == 1) {
        FireAimed(e->pos, spd, r, c);
        FireSpread(e->pos, aim, 28.0f, 3, spd * 0.9f, r - 0.5f, c);
    } else {
        FireSpread(e->pos, aim, 36.0f, 3, spd * 0.85f, r - 0.5f, c);
    }
}

// Player
static void InitPlayer(void) {
    // Inisialisasi player
    player.pos          = (Vector2){ PLAY_X + PLAY_W / 2.0f, SCREEN_H - 100.0f };  // tengah area bermain
    player.radius       = PLAYER_RADIUS;
    player.lives        = 3;
    player.invincTimer  = 0.0f;
    player.shootTimer   = 0.0f;
    player.bombTimer    = 0.0f;
    player.bombs        = PLAYER_MAX_BOMBS;
    player.dead         = false;
}

static Vector2 GetClosestEnemyTarget(void) {
    Vector2 target = (Vector2){ player.pos.x, player.pos.y - 200.0f };
    float bestDist = 1e9f;
    bool found = false;

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active || enemies[i].leaving) continue;
        float d = Dist(player.pos, enemies[i].pos);
        if (d < bestDist) {
            bestDist = d;
            target = enemies[i].pos;
            found = true;
        }
    }

    if (phase == PHASE_BOSS && boss.active && boss.entered) {
        float d = Dist(player.pos, boss.pos);
        if (d < bestDist) {
            target = boss.pos;
            found = true;
        }
    }

    (void)found;
    return target;
}

static void FirePlayerBullet(void) {
    float offsets[] = { -8.0f, 8.0f };
    Vector2 target = {0};
    bool aimTarget = playerFocus;
    if (aimTarget) target = GetClosestEnemyTarget();

    for (int k = 0; k < 2; k++) {
        Vector2 spawn = (Vector2){ player.pos.x + offsets[k], player.pos.y - 10.0f };
        Vector2 vel = aimTarget ? DirectionTo(spawn, target, 14.0f) : (Vector2){ 0, -14.0f };

        for (int i = 0; i < MAX_BULLETS; i++) {
            if (!playerBullets[i].active) {
                playerBullets[i].pos = spawn;
                playerBullets[i].vel = vel;
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

static void SpawnParticleEx(Vector2 pos, Vector2 vel, float life, float size, Color color) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].active) {
            particles[i] = (Particle){ pos, vel, life, life, size, color, true };
            return;
        }
    }
}

static void SpawnParticleBurst(Vector2 center, int count, float speedMin, float speedMax,
                               float lifeMin, float lifeMax, float sizeMin, float sizeMax, Color base) {
    for (int i = 0; i < count; i++) {
        float a = (360.0f / (float)count * i + (float)GetRandomValue(0, 40)) * DEG2RAD;
        float spd = speedMin + (float)GetRandomValue(0, (int)((speedMax - speedMin) * 100.0f)) / 100.0f;
        Vector2 vel = { cosf(a) * spd, sinf(a) * spd };
        float life = lifeMin + (float)GetRandomValue(0, (int)((lifeMax - lifeMin) * 100.0f)) / 100.0f;
        float size = sizeMin + (float)GetRandomValue(0, (int)((sizeMax - sizeMin) * 100.0f)) / 100.0f;
        int tint = GetRandomValue(-35, 35);
        Color c = {
            (unsigned char)Clampf((float)base.r + tint, 0, 255),
            (unsigned char)Clampf((float)base.g + tint, 0, 255),
            (unsigned char)Clampf((float)base.b + tint, 0, 255),
            base.a
        };
        SpawnParticleEx(center, vel, life, size, c);
    }
}

static void SpawnHitSpark(Vector2 pos, Color tint) {
    SpawnParticleBurst(pos, 8, 60.0f, 160.0f, 0.12f, 0.28f, 1.5f, 3.0f, tint);
}

static void SpawnDeathBurst(Vector2 pos, Color tint) {
    SpawnParticleBurst(pos, 22, 40.0f, 180.0f, 0.35f, 0.75f, 2.0f, 5.0f, tint);
    SpawnParticleBurst(pos, 10, 20.0f, 70.0f, 0.5f, 0.9f, 3.0f, 6.0f, WHITE);
}

static void SpawnBossHitBurst(Vector2 pos, bool rage) {
    Color main = rage ? (Color){ 255, 80, 100, 255 } : (Color){ 180, 220, 255, 255 };
    SpawnParticleBurst(pos, 14, 50.0f, 140.0f, 0.2f, 0.45f, 2.0f, 4.5f, main);
}

static void SpawnPlayerHitBurst(Vector2 pos) {
    SpawnParticleBurst(pos, 20, 70.0f, 200.0f, 0.25f, 0.55f, 2.0f, 5.0f, (Color){ 255, 100, 120, 255 });
    SpawnParticleBurst(pos, 8, 30.0f, 90.0f, 0.35f, 0.7f, 2.5f, 4.0f, (Color){ 255, 255, 255, 255 });
}

static void SpawnRageTransformBurst(Vector2 pos) {
    SpawnParticleBurst(pos, 56, 90.0f, 260.0f, 0.5f, 1.1f, 2.0f, 6.0f, (Color){ 255, 40, 60, 255 });
    SpawnParticleBurst(pos, 24, 40.0f, 120.0f, 0.6f, 1.2f, 3.0f, 7.0f, (Color){ 255, 200, 200, 255 });
}

static void SpawnBossDefeatBurst(Vector2 pos) {
    SpawnParticleBurst(pos, 64, 100.0f, 320.0f, 0.6f, 1.4f, 2.5f, 7.0f, (Color){ 255, 220, 120, 255 });
    SpawnParticleBurst(pos, 32, 50.0f, 180.0f, 0.8f, 1.6f, 3.0f, 8.0f, WHITE);
}

static void SpawnBombBurst(Vector2 center) {
    SpawnParticleBurst(center, 40, 80.0f, 220.0f, 0.4f, 0.8f, 2.0f, 5.0f, (Color){ 255, 255, 255, 255 });
    SpawnParticleBurst(center, 24, 50.0f, 150.0f, 0.5f, 1.0f, 2.5f, 6.0f, (Color){ 120, 200, 255, 255 });
    SpawnParticleBurst(center, 16, 30.0f, 100.0f, 0.6f, 1.1f, 3.0f, 5.5f, (Color){ 255, 220, 120, 255 });
}

static void BossOnHpDepleted(void) {
    if (!boss.rage) {
        BossEnterRage();
    } else {
        SpawnBossDefeatBurst(boss.pos);
        boss.active = false;
        score += 50000;
        phase = PHASE_WIN;
    }
}

static void UseBomb(void) {
    if (player.dead || player.bombs <= 0 || player.bombTimer > 0.0f) return;
    if (phase != PHASE_ENEMIES && phase != PHASE_BOSS) return;

    player.bombs--;
    player.bombTimer = BOMB_DURATION;
    if (player.invincTimer < BOMB_DURATION) {
        player.invincTimer = BOMB_DURATION;
    }

    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        enemyBullets[i].active = false;
    }

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i].active) {
            enemies[i].stunTimer = BOMB_STUN_TIME;
        }
    }

    if (phase == PHASE_BOSS && boss.active && !boss.transforming) {
        boss.stunTimer = BOMB_STUN_TIME;
    }

    SpawnBombBurst(player.pos);
}

static void UpdateParticles(float dt) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].active) continue;
        particles[i].life -= dt;
        if (particles[i].life <= 0.0f) {
            particles[i].active = false;
            continue;
        }
        particles[i].pos.x += particles[i].vel.x * dt;
        particles[i].pos.y += particles[i].vel.y * dt;
        particles[i].vel.x *= 0.94f;
        particles[i].vel.y = particles[i].vel.y * 0.94f + 28.0f * dt;
    }
}

static void DrawParticles(void) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].active) continue;
        float t = particles[i].life / particles[i].maxLife;
        Color c = particles[i].color;
        c.a = (unsigned char)(255.0f * t);
        float radius = particles[i].size * (0.4f + t * 0.6f);
        DrawCircleV(particles[i].pos, radius, c);
        if (t > 0.5f) {
            Color core = c;
            core.a = (unsigned char)(c.a * 0.55f);
            DrawCircleV(particles[i].pos, radius * 0.45f, core);
        }
    }
}

static void DrawBombEffect(void) {
    if (player.bombTimer <= 0.0f) return;

    float t = player.bombTimer / BOMB_DURATION;
    float radius = BOMB_RADIUS_MAX * (1.0f - t * 0.65f);
    unsigned char alpha = (unsigned char)(90.0f * t);

    DrawCircleV(player.pos, radius, (Color){ 255, 255, 255, alpha });
    DrawCircleV(player.pos, radius * 0.72f, (Color){ 180, 220, 255, (unsigned char)(alpha * 0.8f) });
    DrawCircleLines((int)player.pos.x, (int)player.pos.y, radius * 0.45f, (Color){ 255, 255, 255, (unsigned char)(alpha * 1.2f) });

    DrawRectangle(PLAY_X, 0, PLAY_W, SCREEN_H, (Color){ 255, 255, 255, (unsigned char)(35.0f * t) });
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

    playerFocus = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    float speed = playerFocus ? PLAYER_FOCUS : PLAYER_SPEED;
    float boundR = playerFocus ? player.radius : PLAYER_GFX_R;

    if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) player.pos.y -= speed * dt;
    if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) player.pos.y += speed * dt;
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) player.pos.x -= speed * dt;
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) player.pos.x += speed * dt;

    player.pos.x = Clampf(player.pos.x, PLAY_X + boundR, PLAY_X + PLAY_W - boundR);
    player.pos.y = Clampf(player.pos.y, boundR, SCREEN_H - boundR);

    player.shootTimer -= dt;
    if (IsKeyDown(KEY_Z) && player.shootTimer <= 0) {
        FirePlayerBullet();
        player.shootTimer = 0.08f;
    }

    if (player.invincTimer > 0.0f) player.invincTimer -= dt;
    if (player.bombTimer > 0.0f) player.bombTimer -= dt;

    if (IsKeyPressed(KEY_X) && (phase == PHASE_ENEMIES || phase == PHASE_BOSS)) {
        UseBomb();
    }
}

static void DrawPlayer(void) {
    if (!player.dead) {
        bool blink = (int)(player.invincTimer / 0.1f) % 2 == 0;
        if (player.invincTimer <= 0 || blink) {
            if (!playerFocus) {
                DrawCircleV(player.pos, PLAYER_GFX_R, BLUE);
            }
            DrawCircleV(player.pos, player.radius, (Color){ 255, 255, 255, playerFocus ? 230 : 180 });
            if (playerFocus) {
                DrawCircleLines((int)player.pos.x, (int)player.pos.y, player.radius, RED);
                DrawCircleLines((int)player.pos.x, (int)player.pos.y, player.radius + 2.0f, (Color){ 255, 120, 120, 180 });
            }
        }
    }
}

// Enemies
static void InitEnemies(void) {
    for (int i = 0; i < MAX_ENEMIES; i++) enemies[i].active = false;
    nextWave = 0;
}

static void SpawnWave(int waveIndex) {
    const EnemyWave *w = &WAVES[waveIndex];
    for (int i = 0; i < w->count; i++) {
        int slot = (w->startIndex + i) % MAX_ENEMIES;
        Enemy *e = &enemies[slot];

        // Starting position depends on entry direction
        if (w->enterDir == ENTER_TOP) {
            e->pos = (Vector2){ w->startX + i * w->spacingX, w->startY - i * 15.0f };
            e->baseX = w->startX + i * w->spacingX;
            e->baseY = w->targetY;
        } else if (w->enterDir == ENTER_LEFT) {
            // Space enemies vertically when entering from side
            e->pos = (Vector2){ w->startX, w->targetY + i * 28.0f };
            e->baseX = PLAY_X + 60.0f + i * 60.0f;  // target X positions spread out
            e->baseY = w->targetY + i * 28.0f;
        } else {  // ENTER_RIGHT
            e->pos = (Vector2){ w->startX, w->targetY + i * 28.0f };
            e->baseX = PLAY_X + PLAY_W - 60.0f - i * 60.0f;
            e->baseY = w->targetY + i * 28.0f;
        }

        e->radius        = 12.0f;
        e->hp            = e->maxHp = 5;
        e->shootTimer    = 1.0f + i * 0.2f;
        e->shootInterval = w->shootInterval * 0.82f;
        e->moveTimer     = 0;
        e->stayTimer     = 0;
        e->stayDuration  = w->stayDuration;
        e->leaving       = false;
        e->active        = true;
        e->color         = w->color;
        e->enterDir      = w->enterDir;
        e->exitDir       = w->exitDir;
        e->moveType      = w->moveType;
        e->stunTimer     = 0.0f;
    }
}

static void UpdateEnemies(float dt) {
    while (nextWave < WAVE_COUNT && stgTimer >= WAVES[nextWave].spawnTime) {
        SpawnWave(nextWave);
        nextWave++;
    }

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) continue;
        Enemy *e = &enemies[i];

        if (e->stunTimer > 0.0f) {
            e->stunTimer -= dt;
            continue;
        }

        // --- LEAVING: move toward exit ---
        if (e->leaving) {
            float exitSpd = 3.5f;
            if      (e->exitDir == EXIT_BOTTOM) e->pos.y += exitSpd;
            else if (e->exitDir == EXIT_TOP)    e->pos.y -= exitSpd;
            else if (e->exitDir == EXIT_LEFT)   e->pos.x -= exitSpd;
            else if (e->exitDir == EXIT_RIGHT)  e->pos.x += exitSpd;

            // Deactivate once fully off screen
            if (e->pos.x < PLAY_X - 60  || e->pos.x > PLAY_X + PLAY_W + 60 ||
                e->pos.y < -60           || e->pos.y > SCREEN_H + 60) {
                e->active = false;
            }
            continue;
        }

        // --- ENTERING: slide toward base position ---
        bool arrived = false;
        if (e->enterDir == ENTER_TOP) {
            if (e->pos.y < e->baseY) e->pos.y += 2.5f;
            else arrived = true;
        } else if (e->enterDir == ENTER_LEFT) {
            if (e->pos.x < e->baseX) e->pos.x += 3.0f;
            else arrived = true;
        } else {  // ENTER_RIGHT
            if (e->pos.x > e->baseX) e->pos.x -= 3.0f;
            else arrived = true;
        }

        if (!arrived) {
            e->moveTimer = 0;
            e->stayTimer = 0;
        } else {
            // --- ARRIVED: apply movement pattern ---
            e->moveTimer += dt;
            e->stayTimer += dt;

            if (e->moveType == MOVE_HOVER) {
                e->pos.x = e->baseX + sinf(e->moveTimer * 1.5f) * 18.0f;
                e->pos.x = Clampf(e->pos.x, PLAY_X + e->radius, PLAY_X + PLAY_W - e->radius);

            } else if (e->moveType == MOVE_SWEEP) {
                // Sweep smoothly across the screen
                float sweepDir = (e->enterDir == ENTER_LEFT) ? 1.0f : -1.0f;
                e->pos.x += sweepDir * 1.8f;

            } else if (e->moveType == MOVE_ZIGZAG) {
                // Zigzag slowly downward
                e->pos.x = e->baseX + sinf(e->moveTimer * 3.0f) * 40.0f;
                e->pos.y += 0.4f;
                e->pos.x = Clampf(e->pos.x, PLAY_X + e->radius, PLAY_X + PLAY_W - e->radius);
            }

            e->shootTimer -= dt;
            if (e->shootTimer <= 0) {
                e->shootTimer = e->shootInterval;
                EnemyShootPattern(e);
            }

            // --- LEAVE when stayTimer expires or sweeper exits bounds ---
            bool sweepDone = (e->moveType == MOVE_SWEEP) &&
                             (e->pos.x < PLAY_X - 20 || e->pos.x > PLAY_X + PLAY_W + 20);
            if (e->stayTimer >= e->stayDuration || sweepDone) {
                e->leaving = true;
            }
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
        Color body = enemies[i].color;
        if (enemies[i].stunTimer > 0.0f) {
            body = (Color){ 180, 180, 200, 255 };
        }
        DrawCircleV(enemies[i].pos, enemies[i].radius, body);
        DrawCircleV(enemies[i].pos, enemies[i].radius - 4.0f, (Color){255, 255, 255, 200});

        float hpRatio = (float)enemies[i].hp / enemies[i].maxHp;
        DrawRectangle((int)enemies[i].pos.x - 18, (int)enemies[i].pos.y + 22, 36, 4, DARKGRAY);
        DrawRectangle((int)enemies[i].pos.x - 18, (int)enemies[i].pos.y + 22, (int)(36 * hpRatio), 4, GREEN);
    }
}

// Boss
static Color BossBulletMain(void) {
    if (boss.rage) return (Color){ 255, 50, 70, 255 };
    return (Color){ 180, 220, 255, 255 };
}

static Color BossBulletAccent(void) {
    if (boss.rage) return (Color){ 160, 10, 30, 255 };
    return (Color){ 255, 60, 100, 255 };
}

static void BossClearBullets(void) {
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) enemyBullets[i].active = false;
}

static void BossEnterRage(void) {
    boss.rage = true;
    boss.transforming = true;
    boss.transformTimer = BOSS_TRANSFORM_TIME;
    boss.maxHp = BOSS_RAGE_HP;
    boss.hp = boss.maxHp;
    boss.phase = 0;
    boss.patternStep = 0;
    boss.attackTimer = BOSS_TRANSFORM_TIME + 0.5f;
    boss.phaseTimer = 0.0f;
    strncpy(boss.name, "Letty - Blood Lunatic", sizeof(boss.name) - 1);
    boss.name[sizeof(boss.name) - 1] = '\0';
    BossClearBullets();
    SpawnRageTransformBurst(boss.pos);
    score += 15000;
}

static void BossUpdatePhase(void) {
    float ratio = (float)boss.hp / (float)boss.maxHp;
    int newPhase;
    if (ratio > 0.70f)      newPhase = 0;
    else if (ratio > 0.45f) newPhase = 1;
    else if (ratio > 0.20f) newPhase = 2;
    else                    newPhase = 3;

    if (newPhase != boss.phase) {
        boss.phase = newPhase;
        boss.patternStep = 0;
    }
}

static float BossBulletSpeed(void) {
    float spd = 2.8f + boss.phase * 0.45f;
    if (boss.rage) spd += 0.65f + boss.phase * 0.2f;
    return spd;
}

static float BossBulletRadius(void) {
    float r = 3.8f - boss.phase * 0.15f;
    if (boss.rage) r -= 0.25f;
    return r;
}

static float BossAimDeg(void) {
    return atan2f(player.pos.y - boss.pos.y, player.pos.x - boss.pos.x) * RAD2DEG;
}

static void BossFireSpiralOnce(int arms, float spinStep) {
    float spd = BossBulletSpeed();
    float r = BossBulletRadius();
    for (int i = 0; i < arms; i++) {
        float deg = boss.spiralAngle + (360.0f / arms) * i;
        float a = deg * DEG2RAD;
        Color c = (i % 2 == 0) ? BossBulletMain() : BossBulletAccent();
        FireEnemyBullet(boss.pos, (Vector2){ cosf(a) * spd, sinf(a) * spd }, r, c);
    }
    boss.spiralAngle += spinStep;
}

static void BossRunPhasePattern(void) {
    float spd = BossBulletSpeed();
    float r = BossBulletRadius();
    Vector2 from = boss.pos;
    float aim = BossAimDeg();
    int step = boss.patternStep++;

    switch (boss.phase) {
        case 0:
            switch (step % 4) {
                case 0:
                    FireRing(from, 10, spd * 0.9f, (float)boss.spiralAngle, r, BossBulletMain());
                    break;
                case 1:
                    FireSpread(from, aim, 32.0f, 3, spd, r, BossBulletMain());
                    break;
                case 2:
                    FireAimed(from, spd, r, BossBulletAccent());
                    FireAimed(from, spd * 0.95f, r, BossBulletMain());
                    break;
                default:
                    FireRing(from, 8, spd * 0.85f, (float)boss.spiralAngle + 22.0f, r, BossBulletAccent());
                    break;
            }
            break;

        case 1:
            switch (step % 4) {
                case 0:
                    FireRing(from, 14, spd, (float)boss.spiralAngle, r, BossBulletMain());
                    FireRing(from, 10, spd * 0.8f, (float)boss.spiralAngle + 18.0f, r - 0.3f, BossBulletAccent());
                    break;
                case 1:
                    FireSpread(from, aim, 55.0f, 5, spd, r, BossBulletMain());
                    break;
                case 2:
                    FireSpread(from, aim - 55.0f, 40.0f, 3, spd * 0.9f, r, BossBulletAccent());
                    FireSpread(from, aim + 55.0f, 40.0f, 3, spd * 0.9f, r, BossBulletAccent());
                    break;
                default:
                    BossFireSpiralOnce(3, 22);
                    break;
            }
            break;

        case 2:
            switch (step % 4) {
                case 0:
                    FireRing(from, 16, spd, (float)boss.spiralAngle, r, BossBulletMain());
                    FireRing(from, 12, spd * 0.75f, (float)boss.spiralAngle + 15.0f, r, BossBulletAccent());
                    break;
                case 1:
                    BossFireSpiralOnce(4, 28);
                    FireSpread(from, aim, 40.0f, 3, spd + 0.3f, r, BossBulletMain());
                    break;
                case 2:
                    FireSpread(from, aim, 70.0f, 7, spd, r, BossBulletMain());
                    FireSpread(from, aim + 180.0f, 50.0f, 5, spd * 0.9f, r, BossBulletAccent());
                    break;
                default:
                    for (int n = 0; n < 4; n++) {
                        float jitter = (float)GetRandomValue(-12, 12);
                        float a = (aim + jitter) * DEG2RAD;
                        FireEnemyBullet(from, (Vector2){ cosf(a) * spd, sinf(a) * spd }, r, BossBulletAccent());
                    }
                    break;
            }
            break;

        default:
            switch (step % 4) {
                case 0:
                    FireRing(from, 18, spd, (float)boss.spiralAngle, r, BossBulletMain());
                    FireRing(from, 14, spd * 0.78f, (float)boss.spiralAngle + 12.0f, r, BossBulletAccent());
                    break;
                case 1:
                    BossFireSpiralOnce(5, 32);
                    FireSpread(from, aim, 50.0f, 5, spd, r, BossBulletMain());
                    break;
                case 2:
                    FireSpread(from, aim, 90.0f, 9, spd, r, BossBulletMain());
                    FireRing(from, 10, spd * 0.7f, (float)boss.spiralAngle + 36.0f, r - 0.2f, BossBulletAccent());
                    break;
                default:
                    FireSpread(from, aim + 90.0f, 60.0f, 5, spd * 0.95f, r, BossBulletAccent());
                    FireSpread(from, aim - 90.0f, 60.0f, 5, spd * 0.95f, r, BossBulletMain());
                    break;
            }
            break;
    }
}

static void BossFireRingGaps(Vector2 from, int count, float speed, float offsetDeg, float rad, Color c) {
    for (int i = 0; i < count; i++) {
        if (i % 2 != 0) continue;
        float a = (360.0f / count * i + offsetDeg) * DEG2RAD;
        FireEnemyBullet(from, (Vector2){ cosf(a) * speed, sinf(a) * speed }, rad, c);
    }
}

static void BossFirePincer(Vector2 from, float aim, float spd, float rad) {
    FireSpread(from, aim - 52.0f, 38.0f, 3, spd, rad, BossBulletMain());
    FireSpread(from, aim + 52.0f, 38.0f, 3, spd, rad, BossBulletAccent());
}

static void BossFireLaneWall(Vector2 from, float aim, float spd, float rad, int lanes) {
    float half = (lanes - 1) * 0.5f;
    for (int i = 0; i < lanes; i++) {
        float laneAim = aim + (i - half) * 16.0f;
        FireSpread(from, laneAim, 14.0f, 2, spd, rad,
            (i % 2 == 0) ? BossBulletMain() : BossBulletAccent());
    }
}

static void BossFireCorkscrew(int arms, float spinA, float spinB) {
    BossFireSpiralOnce(arms, spinA);
    BossFireSpiralOnce(arms, spinB);
}

static void BossFireStarburst(Vector2 from, float aim, float spd, float rad, int points) {
    for (int k = 0; k < points; k++) {
        FireSpread(from, aim + (360.0f / points) * k, 18.0f, 2, spd, rad,
            (k % 2 == 0) ? BossBulletMain() : BossBulletAccent());
    }
}

static void BossFireTripleRing(Vector2 from, float spd, float rad) {
    float off = (float)boss.spiralAngle;
    FireRing(from, 12, spd, off, rad, BossBulletMain());
    FireRing(from, 12, spd * 0.9f, off + 15.0f, rad, BossBulletAccent());
    FireRing(from, 10, spd * 0.82f, off + 30.0f, rad, BossBulletMain());
    boss.spiralAngle += 24;
}

static void BossFireAimedBurst(Vector2 from, float aim, float spd, float rad, int count, int jitter) {
    for (int n = 0; n < count; n++) {
        float j = (float)GetRandomValue(-jitter, jitter);
        float a = (aim + j) * DEG2RAD;
        Color c = (n % 2 == 0) ? BossBulletAccent() : BossBulletMain();
        FireEnemyBullet(from, (Vector2){ cosf(a) * spd, sinf(a) * spd }, rad, c);
    }
}

static void BossRunRagePhasePattern(void) {
    float spd = BossBulletSpeed();
    float r = BossBulletRadius();
    Vector2 from = boss.pos;
    float aim = BossAimDeg();
    int step = boss.patternStep++ % RAGE_PATTERNS_PER_PHASE;

    switch (boss.phase) {
        case 0:
            switch (step) {
                case 0:
                    FireRing(from, 14, spd, (float)boss.spiralAngle, r, BossBulletMain());
                    FireRing(from, 10, spd * 0.82f, (float)boss.spiralAngle + 20.0f, r, BossBulletAccent());
                    break;
                case 1:
                    FireSpread(from, aim, 48.0f, 5, spd, r, BossBulletMain());
                    break;
                case 2:
                    BossFireSpiralOnce(4, 26);
                    break;
                case 3:
                    BossFireAimedBurst(from, aim, spd, r, 3, 6);
                    break;
                case 4:
                    BossFirePincer(from, aim, spd, r);
                    break;
                case 5:
                    BossFireRingGaps(from, 14, spd * 0.9f, (float)boss.spiralAngle, r, BossBulletMain());
                    break;
                case 6:
                    FireSpread(from, aim + 90.0f, 50.0f, 4, spd * 0.88f, r, BossBulletAccent());
                    FireSpread(from, aim - 90.0f, 50.0f, 4, spd * 0.88f, r, BossBulletAccent());
                    break;
                default:
                    BossFireSpiralOnce(3, 20);
                    FireRing(from, 8, spd * 0.85f, (float)boss.spiralAngle + 40.0f, r, BossBulletMain());
                    break;
            }
            break;

        case 1:
            switch (step) {
                case 0:
                    FireRing(from, 16, spd, (float)boss.spiralAngle, r, BossBulletMain());
                    FireSpread(from, aim, 60.0f, 5, spd, r, BossBulletAccent());
                    break;
                case 1:
                    BossFireCorkscrew(4, 24, 48);
                    break;
                case 2:
                    BossFireLaneWall(from, aim, spd, r, 5);
                    break;
                case 3:
                    FireSpread(from, aim + 180.0f, 55.0f, 5, spd * 0.9f, r, BossBulletAccent());
                    break;
                case 4:
                    BossFireSpiralOnce(5, 28);
                    FireRing(from, 10, spd * 0.78f, (float)boss.spiralAngle + 32.0f, r, BossBulletAccent());
                    break;
                case 5:
                    FireSpread(from, aim, 40.0f, 4, spd, r, BossBulletMain());
                    FireSpread(from, aim + 90.0f, 40.0f, 4, spd, r, BossBulletAccent());
                    break;
                case 6:
                    BossFireAimedBurst(from, aim, spd, r, 4, 14);
                    break;
                default:
                    BossFireTripleRing(from, spd, r);
                    break;
            }
            break;

        case 2:
            switch (step) {
                case 0:
                    FireRing(from, 18, spd, (float)boss.spiralAngle, r, BossBulletMain());
                    FireRing(from, 14, spd * 0.8f, (float)boss.spiralAngle + 12.0f, r, BossBulletAccent());
                    break;
                case 1:
                    BossFireCorkscrew(5, 26, 50);
                    FireSpread(from, aim, 45.0f, 4, spd, r, BossBulletMain());
                    break;
                case 2:
                    BossFireStarburst(from, aim, spd * 0.92f, r, 8);
                    break;
                case 3:
                    BossFirePincer(from, aim, spd, r);
                    FireSpread(from, aim, 35.0f, 3, spd * 0.95f, r, BossBulletMain());
                    break;
                case 4:
                    FireRing(from, 12, spd, (float)boss.spiralAngle, r, BossBulletMain());
                    FireSpread(from, aim + 180.0f, 48.0f, 4, spd * 0.88f, r, BossBulletAccent());
                    break;
                case 5:
                    BossFireLaneWall(from, aim, spd, r, 6);
                    break;
                case 6:
                    FireSpread(from, aim + 70.0f, 55.0f, 5, spd, r, BossBulletAccent());
                    FireSpread(from, aim - 70.0f, 55.0f, 5, spd, r, BossBulletMain());
                    break;
                default:
                    BossFireRingGaps(from, 18, spd, (float)boss.spiralAngle + 8.0f, r, BossBulletAccent());
                    BossFireSpiralOnce(4, 30);
                    break;
            }
            break;

        default:
            switch (step) {
                case 0:
                    BossFireTripleRing(from, spd, r);
                    break;
                case 1:
                    BossFireCorkscrew(6, 30, 58);
                    FireSpread(from, aim, 75.0f, 6, spd, r, BossBulletMain());
                    break;
                case 2:
                    BossFireStarburst(from, aim, spd, r, 10);
                    break;
                case 3:
                    BossFireLaneWall(from, aim, spd, r, 7);
                    FireSpread(from, aim + 180.0f, 50.0f, 4, spd * 0.9f, r, BossBulletAccent());
                    break;
                case 4:
                    FireRing(from, 20, spd, (float)boss.spiralAngle, r, BossBulletMain());
                    BossFireRingGaps(from, 16, spd * 0.85f, (float)boss.spiralAngle + 11.0f, r, BossBulletAccent());
                    break;
                case 5:
                    BossFirePincer(from, aim, spd, r);
                    BossFireAimedBurst(from, aim, spd + 0.3f, r, 4, 18);
                    FireSpread(from, aim + 90.0f, 55.0f, 4, spd, r, BossBulletMain());
                    break;
                case 6:
                    FireSpread(from, aim, 95.0f, 8, spd, r, BossBulletMain());
                    BossFireSpiralOnce(5, 34);
                    break;
                default:
                    BossFireCorkscrew(5, 22, 44);
                    FireRing(from, 14, spd * 0.75f, (float)boss.spiralAngle + 25.0f, r, BossBulletAccent());
                    FireSpread(from, aim - 90.0f, 60.0f, 5, spd * 0.95f, r, BossBulletMain());
                    FireSpread(from, aim + 90.0f, 60.0f, 5, spd * 0.95f, r, BossBulletAccent());
                    break;
            }
            break;
    }
}

static void InitBoss(void) {
    boss.pos = (Vector2){ PLAY_X + PLAY_W / 2.0f, -90.0f };
    boss.radius = 38.0f;
    boss.hp = boss.maxHp = BOSS_MAX_HP;
    boss.attackTimer = 0.4f;
    boss.burstTimer = 0.0f;
    boss.phaseTimer = 0.0f;
    boss.enterTimer = 0.0f;
    boss.spiralAngle = 0;
    boss.moveAngle = 0.0f;
    boss.currentAttack = ATTACK_RING;
    boss.patternStep = 0;
    boss.rage = false;
    boss.transforming = false;
    boss.transformTimer = 0.0f;
    boss.stunTimer = 0.0f;
    boss.active = true;
    boss.entered = false;
    boss.phase = 0;
    strncpy(boss.name, "Letty Whiterock", sizeof(boss.name) - 1);
    boss.name[sizeof(boss.name) - 1] = '\0';
}

static void UpdateBoss(float dt) {
    if (!boss.active) return;

    if (!boss.entered) {
        boss.pos.y += 95.0f * dt;
        if (boss.pos.y >= BOSS_ENTER_Y) {
            boss.pos.y = BOSS_ENTER_Y;
            boss.entered = true;
            boss.attackTimer = 0.1f;
        }
        return;
    }

    if (boss.transforming) {
        boss.transformTimer -= dt;
        if (boss.transformTimer <= 0.0f) {
            boss.transforming = false;
            boss.attackTimer = 0.4f;
        }
        return;
    }

    if (boss.stunTimer > 0.0f) {
        boss.stunTimer -= dt;
        return;
    }

    boss.moveAngle += dt * (1.0f + boss.phase * 0.2f + (boss.rage ? 0.35f : 0.0f));
    float moveAmp = 90.0f + boss.phase * 12.0f + (boss.rage ? 25.0f : 0.0f);
    float bobAmp = 16.0f + boss.phase * 4.0f + (boss.rage ? 8.0f : 0.0f);
    boss.pos.x = PLAY_X + PLAY_W / 2.0f + sinf(boss.moveAngle) * moveAmp;
    boss.pos.y = BOSS_ENTER_Y + sinf(boss.moveAngle * 2.1f) * bobAmp;
    boss.pos.x = Clampf(boss.pos.x, PLAY_X + boss.radius, PLAY_X + PLAY_W - boss.radius);

    BossUpdatePhase();
    boss.spiralAngle += (int)(dt * 80.0f);

    boss.attackTimer -= dt;
    float interval = 1.05f - boss.phase * 0.12f;
    if (boss.rage) interval = 0.82f - boss.phase * 0.1f;
    if (boss.attackTimer <= 0.0f) {
        boss.attackTimer = interval;
        if (boss.rage) BossRunRagePhasePattern();
        else BossRunPhasePattern();
    }

    boss.phaseTimer += dt;
    float aimInterval = 1.35f - boss.phase * 0.15f;
    if (boss.rage) aimInterval = 1.0f - boss.phase * 0.12f;
    if (boss.phaseTimer >= aimInterval) {
        boss.phaseTimer = 0.0f;
        int shots = 1 + boss.phase / 2;
        if (boss.rage) shots = 2 + boss.phase / 2;
        for (int i = 0; i < shots; i++) {
            FireAimed(boss.pos, BossBulletSpeed() * 0.95f, BossBulletRadius(), BossBulletMain());
        }
    }
}

static void DrawBoss(void) {
    if (!boss.active) return;

    Color glow, body, core;
    Color barFill, nameCol;

    if (boss.rage) {
        bool pulse = boss.transforming && ((int)(boss.transformTimer * 8.0f) % 2 == 0);
        glow = pulse ? (Color){ 255, 200, 200, 120 } : (Color){ 120, 0, 20, 100 };
        body = pulse ? (Color){ 255, 120, 120, 255 } : (Color){ 200, 25, 45, 255 };
        core = (Color){ 80, 0, 10, 255 };
        barFill = (Color){ 255, 30, 50, 255 };
        nameCol = (Color){ 255, 140, 140, 255 };
    } else {
        glow = (Color){ 120, 180, 255, 80 };
        body = (Color){ 200, 230, 255, 255 };
        core = (Color){ 255, 255, 255, 220 };
        barFill = (Color){ 255, 80, 120, 255 };
        nameCol = (Color){ 200, 220, 255, 255 };
    }

    if (boss.stunTimer > 0.0f) {
        glow = (Color){ 200, 200, 220, 90 };
        body = (Color){ 160, 160, 180, 255 };
        core = (Color){ 220, 220, 240, 220 };
    }

    DrawCircleV(boss.pos, boss.radius + 6.0f, glow);
    DrawCircleV(boss.pos, boss.radius, body);
    DrawCircleV(boss.pos, boss.radius - 10.0f, core);

    float hpRatio = (float)boss.hp / (float)boss.maxHp;
    int barW = PLAY_W - 40;
    int barX = PLAY_X + 20;
    DrawRectangle(barX, 14, barW, 10, (Color){ 30, 20, 40, 255 });
    DrawRectangle(barX, 14, (int)(barW * hpRatio), 10, barFill);
    DrawRectangleLines(barX, 14, barW, 10, WHITE);

    int nameW = MeasureText(boss.name, 14);
    DrawText(boss.name, PLAY_X + (PLAY_W - nameW) / 2, 28, 14, nameCol);

    if (boss.transforming) {
        const char *warn = "PATHETIC MORTAL!!!";
        int w = MeasureText(warn, 22);
        DrawText(warn, PLAY_X + (PLAY_W - w) / 2, SCREEN_H / 2 - 40, 22, (Color){ 255, 60, 80, 255 });
    }
}

// Collision Detection
static void DamagePlayer(void) {
    if (player.dead || player.invincTimer > 0.0f || player.bombTimer > 0.0f) return;
    if (devMode) {
        player.invincTimer = 1.0f;  // brief invinc but no life lost
        return;
    }
    SpawnPlayerHitBurst(player.pos);
    player.lives--;
    player.invincTimer = 2.5f;
    if (player.lives <= 0) {
        player.lives = 0;
        player.dead = true;
        phase = PHASE_DEAD;
    }
}

static void HandlePlayerHits(void) {
    if (player.dead || player.invincTimer > 0.0f) return;

    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (!enemyBullets[i].active) continue;
        float hitR = player.radius + enemyBullets[i].radius;
        if (Dist(player.pos, enemyBullets[i].pos) < hitR) {
            Vector2 hitPos = enemyBullets[i].pos;
            enemyBullets[i].active = false;
            SpawnHitSpark(hitPos, enemyBullets[i].color);
            DamagePlayer();
            return;
        }
    }

    if (phase == PHASE_BOSS && boss.active && boss.entered) {
        if (Dist(player.pos, boss.pos) < player.radius + boss.radius * 0.65f) {
            DamagePlayer();
        }
    }
}

static void handleCollisions(void) {
    for (int b = 0; b < MAX_BULLETS; b++) {
        if (!playerBullets[b].active) continue;

        if (phase == PHASE_BOSS && boss.active && !boss.transforming) {
            if (Dist(playerBullets[b].pos, boss.pos) < BULLET_RADIUS + boss.radius) {
                Vector2 hitPos = playerBullets[b].pos;
                playerBullets[b].active = false;
                SpawnBossHitBurst(hitPos, boss.rage);
                boss.hp--;
                score += boss.rage ? 100 : 80;
                BossUpdatePhase();
                if (boss.hp <= 0) {
                    BossOnHpDepleted();
                }
                continue;
            }
        }

        for (int e = 0; e < MAX_ENEMIES; e++) {
            if (!enemies[e].active) continue;
            if (Dist(playerBullets[b].pos, enemies[e].pos) < BULLET_RADIUS + enemies[e].radius) {
                Vector2 hitPos = playerBullets[b].pos;
                playerBullets[b].active = false;
                enemies[e].hp--;
                score += 50;
                if (enemies[e].hp <= 0) {
                    SpawnDeathBurst(enemies[e].pos, enemies[e].color);
                    enemies[e].active = false;
                    score += 200;
                } else {
                    SpawnHitSpark(hitPos, SKYBLUE);
                }
                break;
            }
        }
    }

    HandlePlayerHits();
}

// HUD
static void DrawHUD(void) {
    int rightX = PLAY_X + PLAY_W;
    int rightW = SCREEN_W - rightX;

    DrawRectangle(rightX, 0, rightW, SCREEN_H, (Color){15, 10, 25, 255});
    DrawRectangleLines(rightX, 0, rightW, SCREEN_H, DARKGRAY);

    DrawRectangle(0, 0, PLAY_X, SCREEN_H, (Color){15, 10, 25, 255});
    DrawRectangleLines(0, 0, PLAY_X, SCREEN_H, DARKGRAY);

    const char *scoreTxt = TextFormat("SCORE %d", score);
    DrawText(scoreTxt, rightX + 8, 24, 16, WHITE);

    const char *lifeTxt = TextFormat("LIVES %d", player.lives);
    DrawText(lifeTxt, rightX + 8, 48, 16, (Color){255, 120, 160, 255});

    const char *bombTxt = TextFormat("BOMBS %d", player.bombs);
    DrawText(bombTxt, rightX + 8, 72, 16, (Color){255, 220, 120, 255});

    if (phase == PHASE_BOSS) {
        if (boss.rage) {
            DrawText("RAGE", 8, 24, 16, (Color){255, 60, 80, 255});
        } else {
            DrawText("BOSS", 8, 24, 16, (Color){255, 80, 120, 255});
        }
    }

    if (devMode) {
        DrawText("[DEV MODE]", 8, SCREEN_H - 24, 14, (Color){100, 255, 150, 200});
    }
}

static void DrawEndScreen(const char *msg, Color color) {
    DrawRectangle(PLAY_X, 0, PLAY_W, SCREEN_H, (Color){0, 0, 0, 170});
    int fs = 34;
    int w = MeasureText(msg, fs);
    DrawText(msg, PLAY_X + (PLAY_W - w) / 2, SCREEN_H / 2 - 50, fs, color);
    const char *sub = "ENTER - menu";
    int subFs = 18;
    int sw = MeasureText(sub, subFs);
    DrawText(sub, PLAY_X + (PLAY_W - sw) / 2, SCREEN_H / 2 + 10, subFs, (Color){180, 170, 200, 255});
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
    nextWave  = 0;
    score     = 0;
    phase     = PHASE_ENEMIES;
    paused    = false;
    stgTimer  = 0.0f;

    if (devMode) {
        player.lives = 999;
        player.bombs = 999;
    }

    memset(particles, 0, sizeof(particles));
    memset(playerBullets, 0, sizeof(playerBullets));
    memset(enemyBullets, 0, sizeof(enemyBullets));
    boss.active = false;
    boss.entered = false;
}

static bool UpdateMenuInput(float dt) {
    if (devMsgTimer > 0.0f) devMsgTimer -= dt;

    int ch = GetCharPressed();
    while (ch > 0) {
        if (ch >= 32 && ch <= 126 && devLen < DEV_INPUT_MAX) {
            devInput[devLen++] = (char)ch;
            devInput[devLen] = '\0';
        }
        ch = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE) && devLen > 0) {
        devLen--;
        devInput[devLen] = '\0';
    }

    if (IsKeyPressed(KEY_ENTER) && devLen > 0) {
        if (strcmp(devInput, DEV_CODE) == 0) {
            devMode = !devMode;
            devFeedback = devMode ? 1 : 2;
            devMsgTimer = 2.5f;
        } else {
            devFeedback = 3;
            devMsgTimer = 1.5f;
        }

        devLen = 0;
        devInput[0] = '\0';
        return true;
    }

    if (IsKeyPressed(KEY_ESCAPE) && devLen > 0) {
        devLen = 0;
        devInput[0] = '\0';
    }

    return false;
}

static void UpdateMenu(void) {
    bool enterForCode = UpdateMenuInput(GetFrameTime());

    if (!enterForCode && devLen == 0 &&
        (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_Z))) {
        GameInit();
    }
}

static void UpdateEndScreen(void) {
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        phase = PHASE_MENU;
        for (int i = 0; i < MAX_ENEMY_BULLETS; i++) enemyBullets[i].active = false;
        for (int i = 0; i < MAX_BULLETS; i++) playerBullets[i].active = false;
        boss.active = false;
    }
}

static void DrawMenu(void) {
    DrawRectangle(0, 0, SCREEN_W, SCREEN_H, (Color){15, 10, 25, 200});

    const char *title = "DANMAKU";
    int titleSize = 52;
    int titleW = MeasureText(title, titleSize);
    DrawText(title, (SCREEN_W - titleW) / 2, 140, titleSize, (Color){255, 120, 200, 255});

    const char *sub = "BULLET HELL";
    int subSize = 18;
    int subW = MeasureText(sub, subSize);
    DrawText(sub, (SCREEN_W - subW) / 2, 200, subSize, (Color){180, 160, 220, 255});

    if (((int)(GetTime() * 2.0f)) % 2 == 0) {
        const char *start = "PRESS ENTER TO START";
        int startSize = 22;
        int startW = MeasureText(start, startSize);
        DrawText(start, (SCREEN_W - startW) / 2, 320, startSize, WHITE);
    }

    int hintSize = 16;
    const char *hints[] = {
        "Move:  Arrow Keys / WASD",
        "Shoot: Z    Bomb: X",
        "Focus: Shift      Pause: P",
        "Quit:  ESC"
    };
    for (int i = 0; i < 3; i++) {
        int hw = MeasureText(hints[i], hintSize);
        DrawText(hints[i], (SCREEN_W - hw) / 2, 420 + i * 28, hintSize, (Color){140, 130, 170, 255});
    }
    // --- Dev code input bar ---
    const int barW = 220;
    const int barH = 30;
    const int barX = (SCREEN_W - barW) / 2;
    const int barY = SCREEN_H - 80;
    const int padX = 10;
    const int fontSize = 18;
    const bool cursorOn = ((int)(GetTime() * 2.0f)) % 2 == 0;

    DrawRectangle(barX, barY, barW, barH, (Color){30, 20, 50, 220});
    DrawRectangleLines(barX, barY, barW, barH,
        devMode ? (Color){100, 255, 150, 255} : (Color){100, 80, 130, 255});

    int textX = barX + padX;
    int textY = barY + (barH - fontSize) / 2;
    int cursorX = textX;

    if (devLen > 0) {
        DrawText(devInput, textX, textY, fontSize, WHITE);
        cursorX = textX + MeasureText(devInput, fontSize);
    } else {
        DrawText("enter code...", textX, textY, fontSize, (Color){90, 80, 110, 255});
    }

    if (cursorOn) {
        int cx = Clampf((float)cursorX, (float)(barX + padX), (float)(barX + barW - padX - 2));
        DrawRectangle((int)cx, barY + 6, 2, barH - 12, (Color){220, 200, 255, 230});
    }

    if (devMsgTimer > 0.0f && devFeedback > 0) {
        const char *msg = "";
        Color msgColor = WHITE;
        if (devFeedback == 1) {
            msg = "DEV MODE ON";
            msgColor = (Color){100, 255, 150, 255};
        } else if (devFeedback == 2) {
            msg = "DEV MODE OFF";
            msgColor = (Color){180, 200, 255, 255};
        } else if (devFeedback == 3) {
            msg = "WRONG CODE";
            msgColor = (Color){255, 100, 100, 255};
        }
        int msgW = MeasureText(msg, 16);
        DrawText(msg, (SCREEN_W - msgW) / 2, barY - 26, 16, msgColor);
    }

    if (devMode) {
        DrawText("[DEV]", barX + barW + 6, barY + 7, 14, (Color){100, 255, 150, 255});
    }
}

// Main Game Loop
int main(void) {
    InitWindow(SCREEN_W, SCREEN_H, "Danmaku Game");
    InitAudioDevice();
    
    Music musicEnemy = LoadMusicStream("assets/Touhou 7 - Paradise  Deep Mountain (Stage 1).mp3");
    Music musicBoss = LoadMusicStream("assets/Touhou 7 - Letty Whiterock's Theme - Crystallized Silver (Boss 1).mp3");

    Music currentTrack = musicEnemy;

    float musicSwitchTimer = 0.0f;
    float musicSwitchInterval = 159.0f; // Switch tracks every 120 seconds
    bool musicSwitched = false;

    SetTargetFPS(60);

    phase = PHASE_MENU;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        if (phase == PHASE_MENU) {
            UpdateMenu();
            if (IsKeyPressed(KEY_ESCAPE)) break;

            if (phase != PHASE_MENU) {
                StopMusicStream(currentTrack);
                currentTrack = musicEnemy;
                PlayMusicStream(musicEnemy);
                musicSwitched = false;
                musicSwitchTimer = 0.0f;
            }
        } else {
            if (IsKeyPressed(KEY_P)) {
                paused = !paused;
                if (paused) {
                    PauseMusicStream(currentTrack);
                } else {
                    ResumeMusicStream(currentTrack);
                }
            }

            UpdateMusicStream(currentTrack);

            if (phase == PHASE_WIN || phase == PHASE_DEAD) {
                UpdateEndScreen();
            } else if (!paused) {
                if (!musicSwitched && phase == PHASE_ENEMIES) {
                    musicSwitchTimer += dt;
                    if (musicSwitchTimer >= musicSwitchInterval) {
                        StopMusicStream(currentTrack);
                        currentTrack = musicBoss;
                        PlayMusicStream(currentTrack);
                        musicSwitched = true;

                        for (int i = 0; i < MAX_ENEMIES; i++) enemies[i].active = false;
                        for (int i = 0; i < MAX_ENEMY_BULLETS; i++) enemyBullets[i].active = false;
                        for (int i = 0; i < MAX_BULLETS; i++) playerBullets[i].active = false;
                        phase = PHASE_BOSS;
                        InitBoss();
                    }
                }

                if (phase == PHASE_BOSS) {
                    UpdateBoss(dt);
                } else if (phase == PHASE_ENEMIES) {
                    UpdateEnemies(dt);
                }

                if (phase == PHASE_ENEMIES || phase == PHASE_BOSS) {
                    handleCollisions();
                    UpdatePlayer(dt);
                    UpdatePlayerBullets();
                    UpdateEnemyBullets(dt);
                    UpdateParticles(dt);
                    stgTimer += dt;
                }
            }

            if (phase == PHASE_MENU) {
                StopMusicStream(currentTrack);
                musicSwitched = false;
                musicSwitchTimer = 0.0f;
            }
        }

        BeginDrawing();
        DrawBackground();

        if (phase == PHASE_MENU) {
            DrawMenu();
        } else {
            DrawPlayerBullets();
            if (phase == PHASE_ENEMIES) DrawEnemies();
            DrawEnemyBullets();
            if (phase == PHASE_BOSS) DrawBoss();
            DrawParticles();
            DrawBombEffect();
            DrawPlayer();
            DrawHUD();

            if (paused) {
                DrawRectangle(PLAY_X, 0, PLAY_W, SCREEN_H, (Color){0, 0, 0, 140});
                const char *msg = "PAUSED";
                int fs = 30;
                int w = MeasureText(msg, fs);
                DrawText(msg, PLAY_X + (PLAY_W - w) / 2, SCREEN_H / 2 - fs / 2, fs, WHITE);
            } else if (phase == PHASE_WIN) {
                DrawEndScreen("STAGE CLEAR!", (Color){120, 255, 200, 255});
            } else if (phase == PHASE_DEAD) {
                DrawEndScreen("GAME OVER", (Color){255, 90, 90, 255});
            }
        }

        EndDrawing();
    }
    
    UnloadMusicStream(musicEnemy);
    UnloadMusicStream(musicBoss);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}