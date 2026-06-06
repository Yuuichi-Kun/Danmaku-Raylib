# DANMAKU BULLET HELL GAME - COMPLETE CODE EXPLANATION

## TABLE OF CONTENTS
1. [Project Overview](#project-overview)
2. [Screen Layout & Coordinate System](#screen-layout--coordinate-system)
3. [Core Data Structures](#core-data-structures)
4. [Helper Functions & Mathematics](#helper-functions--mathematics)
5. [Game Phases System](#game-phases-system)
6. [Player System](#player-system)
7. [Enemy System](#enemy-system)
8. [Boss System](#boss-system)
9. [Bullet Systems](#bullet-systems)
10. [Particle System](#particle-system)
11. [Collision Detection](#collision-detection)
12. [Game Loop Architecture](#game-loop-architecture)
13. [Special Features (Dev & Hell Mode)](#special-features)

---

## PROJECT OVERVIEW

This is a **Touhou-style Bullet Hell (Danmaku) game** built with Raylib in C. The game features:
- A player character that dodges enemy and boss bullets
- Progressive difficulty with enemy waves
- A final boss battle with multiple phases and patterns
- Dev mode and Hell mode for testing/enhanced difficulty
- Particle effects and visual feedback
- Score system based on survival time

**Key Dependencies:**
- `raylib.h` - Graphics and input library
- Standard C libraries: `math.h`, `stdio.h`, `stdlib.h`, `string.h`, `stdbool.h`

---

## SCREEN LAYOUT & COORDINATE SYSTEM

### Screen Dimensions
```c
#define PLAY_W 400        // Playable area width
#define HUD_SIDE_W 150    // Left and right HUD panel width
#define PLAY_X HUD_SIDE_W // Playable area starts at x=150
#define SCREEN_W (PLAY_X + PLAY_W + HUD_SIDE_W) // Total: 700 pixels
#define SCREEN_H 640      // Height: 640 pixels
```

### Visual Layout
```
┌─────────────────────────────────────────────────┐
│ LEFT HUD │        PLAY AREA (400x640)    │RIGHT HUD│
│ (150px)  │ ← PLAY_X=150                 │ (150px) │
│          │ Stats, Mode indicators        │ Score  │
│          │ Boss name, health bar        │ Lives  │
│ "BOSS"   │                               │ Bombs  │
│ "RAGE"   │                               │        │
│ "[DEV]"  │                               │        │
│ "[HELL]" │                               │        │
└─────────────────────────────────────────────────┘
  0        150       PLAY_X+PLAY_W=550      700
           Playable area only: X:[150,550], Y:[0,640]
```

### Coordinate System
- **Origin:** Top-left corner (0, 0)
- **X-axis:** 0→700 pixels (left to right)
- **Y-axis:** 0→640 pixels (top to bottom)
- Playable area is constrained to X:[150, 550], Y:[0, 640]

---

## CORE DATA STRUCTURES

### 1. Player Structure
```c
typedef struct {
    Vector2 pos;           // Current position
    float radius;          // Collision radius (5.0f)
    int lives;             // Number of lives (3 at start)
    int bombs;             // Bomb count (max 3)
    float invincTimer;     // Invincibility duration after hit
    float shootTimer;      // Cooldown between shots
    float bombTimer;       // Bomb activation cooldown
    bool dead;             // Is player dead?
} Player;
```

**Purpose:** Represents the player character
- Tracks position, health, resources (bombs)
- Manages shooting and bomb cooldowns
- Invincibility frames prevent rapid damage

### 2. Bullet & EnemyBullet Structures
```c
typedef struct {
    Vector2 pos;           // Position
    Vector2 vel;           // Velocity per frame
    bool active;           // Is bullet active?
} Bullet;

typedef struct {
    Vector2 pos;           // Position
    Vector2 vel;           // Velocity per frame
    float radius;          // Visual radius
    Color color;           // Bullet color
    bool active;           // Is bullet active?
} EnemyBullet;
```

**Purpose:** Represent projectiles
- Player bullets: white, aimed at enemies
- Enemy bullets: colored, various patterns
- Vel is applied directly to pos each frame (simple physics)

### 3. Enemy Structure
```c
typedef struct {
    Vector2 pos;           // Current position
    float radius;          // Collision radius
    int hp;                // Health points (5)
    int maxHp;             // Max HP
    float shootTimer;      // Countdown to next shot
    float shootInterval;   // Time between shots
    float moveTimer;       // Elapsed time for movement pattern
    float stayTimer;       // How long at target position
    float stayDuration;    // Duration to stay (from wave data)
    bool leaving;          // Currently exiting screen?
    bool active;           // Is enemy active?
    Color color;           // Enemy color
    EnemyEnter enterDir;   // Entry direction (TOP/LEFT/RIGHT)
    EnemyExit exitDir;     // Exit direction (BOTTOM/LEFT/RIGHT/TOP)
    EnemyMove moveType;    // Movement pattern (HOVER/SWEEP/ZIGZAG)
    float baseX, baseY;    // Target position when hovering
    float stunTimer;       // Stun duration from bomb
} Enemy;
```

**Purpose:** Represents enemy units spawned in waves
- Enters screen, hovers/moves using patterns
- Shoots at player based on intervals
- Can be stunned by bombs
- Exits when stayDuration expires

### 4. Boss Structure
```c
typedef struct {
    Vector2 pos;           // Position
    float radius;          // Collision radius
    int hp;                // Current health
    int maxHp;             // Max health
    float attackTimer;     // Countdown to next attack
    float burstTimer;      // Used for specific patterns
    float phaseTimer;      // Countdown to aimed shots
    float enterTimer;      // Entry animation timer
    BossAttack currentAttack; // Current attack type
    int spiralAngle;       // Angle for spiral patterns
    float moveAngle;       // For sine-wave movement
    bool active;           // Is boss alive?
    bool entered;          // Has boss entered play area?
    int phase;             // Boss phase (0-3, based on HP%)
    int patternStep;       // Which attack pattern in phase?
    bool rage;             // In rage mode?
    bool transforming;     // Currently transforming?
    float transformTimer;  // Transformation progress
    float stunTimer;       // Stun from bomb
    float deathTimer;      // Death animation timer
    float animTimer;       // Running sprite animation timer (seconds elapsed)
    char name[24];         // Boss name ("Letty Whiterock" or "Letty - Blood Lunatic")
} Boss;
```

**Purpose:** Represents the boss enemy
- Complex AI with multiple phases and attack patterns
- Transitions to "rage" mode at 50% HP
- `deathTimer` drives the death fade-out animation sequence
- `animTimer` is incremented every frame and drives idle sprite frame selection

### 5. Global Boss Textures
```c
static Texture2D texBossMain;        // Idle / rage sprite sheet (6 frames, 128×128 each)
static Texture2D texBossTransition;  // Phase-2 transformation sheet (4 frames)
static Texture2D texBossDeath;       // Death animation sheet (8 frames)
```

**Purpose:** Sprite sheets used by `DrawBoss()` for frame-based animation
- Loaded once at startup from the `assets/` directory
- Freed with `UnloadTexture()` before `CloseWindow()`
- All sheets are horizontal strips: frame N starts at pixel `N * 128`

**Asset files:**
| Variable | File | Frames |
|---|---|---|
| `texBossMain` | `assets/BossSprite.png` | 6 |
| `texBossTransition` | `assets/Boss2ndPhaseTransition.png` | 4 |
| `texBossDeath` | `assets/BossDeath.png` | 8 |

---

### 6. Particle Structure
```c
typedef struct {
    Vector2 pos;           // Position
    Vector2 vel;           // Velocity
    float life;            // Current life (decrements)
    float maxLife;         // Initial life
    float size;            // Visual size
    Color color;           // Color
    bool active;           // Is active?
} Particle;
```

**Purpose:** Visual effects
- Spawn on defeats, explosions, transformations
- Fade out as life decrements
- Used for 3D-like effects and impact feedback

### 7. EnemyWave Structure
```c
typedef struct {
    float spawnTime;       // When to spawn this wave (in game seconds)
    int startIndex;        // Which enemy slot to start filling
    int count;             // How many enemies to spawn
    float startX;          // Entry X position
    float spacingX;        // X spacing between enemies
    float startY;          // Entry Y position
    float targetY;         // Target Y position when hovering
    float shootInterval;   // How often enemies shoot
    float stayDuration;    // How long to stay on screen
    Color color;           // Enemy color
    EnemyEnter enterDir;   // Entry direction
    EnemyExit exitDir;     // Exit direction
    EnemyMove moveType;    // Movement pattern
} EnemyWave;
```

**Purpose:** Defines enemy wave parameters
- WAVES[] array contains 20 hardcoded waves
- Waves spawn at specific times (stgTimer)
- Controls difficulty progression

### 8. GamePhase Enum
```c
typedef enum {
    PHASE_MENU,        // Main menu
    PHASE_WIN,         // Player won (after boss death)
    PHASE_ENEMIES,     // Normal enemy wave phase
    PHASE_BOSS,        // Boss battle
    PHASE_BOSS_DEATH,  // Boss death animation (2.5 seconds)
    PHASE_DEAD         // Player died
} GamePhase;
```

---

## HELPER FUNCTIONS & MATHEMATICS

### 1. Dist() - Euclidean Distance
```c
static float Dist(Vector2 a, Vector2 b) {
    return sqrtf((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y));
}
```

**Mathematics:**
- Formula: `d = √((x₂-x₁)² + (y₂-y₁)²)`
- Calculates straight-line distance between two points
- Used for collision detection (distance < radius sum = collision)

**Usage:**
- Collision checks between bullets and targets
- Finding closest enemy for focused shot

### 2. DirectionTo() - Normalized Direction Vector
```c
static Vector2 DirectionTo(Vector2 from, Vector2 to, float speed) {
    float dx = to.x - from.x;
    float dy = to.y - from.y;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.001f) return (Vector2){0, speed};
    return (Vector2){dx / len * speed, dy / len * speed};
}
```

**Mathematics:**
- Calculate direction: `dir = (to - from)`
- Calculate magnitude: `len = √(dx² + dy²)`
- Normalize: `normalized = dir / len` (unit vector)
- Scale to speed: `result = normalized * speed`

**Result:** A velocity vector pointing from `from` toward `to` at given `speed`

**Usage:**
- Player's aimed shots toward enemies
- Enemy's aimed shots toward player
- Any "homing" bullet behavior

### 3. Clampf() - Constrain Value
```c
static float Clampf(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}
```

**Purpose:** Keep value within bounds [min, max]

**Usage:**
- Keep player, enemies, boss within screen bounds
- Prevent positions from going out of playable area

### Angle Conversion Constants
```c
DEG2RAD  // Convert degrees to radians: deg * π/180
RAD2DEG  // Convert radians to degrees: rad * 180/π
```

These are from Raylib and used in all rotational calculations.

---

## GAME PHASES SYSTEM

### Phase Transitions
```
PHASE_MENU
    ↓ (ENTER key)
PHASE_ENEMIES (waves spawn from t=0 to t≈153 seconds)
    ↓ (159 seconds elapsed, all enemies cleared)
PHASE_BOSS (boss enters and starts attacking)
    ↓ (boss.hp ≤ 0)
PHASE_BOSS_DEATH (2.5 second death animation)
    ↓ (deathTimer expires)
PHASE_WIN (victory screen)
    ↓ (ENTER key)
PHASE_MENU

    OR

Player takes damage with lives=0 → PHASE_DEAD
```

### Phase-Specific Behavior
- **PHASE_MENU:** Display menu, accept code input, wait for start
- **PHASE_ENEMIES:** Spawn enemy waves, update/draw enemies
- **PHASE_BOSS:** Update/draw boss, run attack patterns
- **PHASE_BOSS_DEATH:** Fade-out animation, show death particles
- **PHASE_WIN:** Display victory screen
- **PHASE_DEAD:** Display game over screen

---

## PLAYER SYSTEM

### Initialization (InitPlayer)
```c
player.pos = (Vector2){ PLAY_X + PLAY_W / 2.0f, SCREEN_H - 100.0f };
// Position: centered horizontally, 100px from bottom

player.radius = PLAYER_RADIUS;  // 5.0f pixels
player.lives = 3;
player.bombs = PLAYER_MAX_BOMBS;  // 3
player.invincTimer = 0.0f;
player.shootTimer = 0.0f;
player.bombTimer = 0.0f;
player.dead = false;
```

**Starting Position:** `(425, 540)` - Bottom center of play area

### Player Movement (UpdatePlayer)
```c
if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W))     player.pos.y -= PLAYER_SPEED * dt;
if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S))   player.pos.y += PLAYER_SPEED * dt;
if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A))   player.pos.x -= PLAYER_SPEED * dt;
if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D))  player.pos.x += PLAYER_SPEED * dt;
```

**Speed:** 240 pixels/second
- Diagonal movement: √(240² + 240²) ≈ 339 px/s
- Speed is independent of frame rate (multiplied by dt)

**Boundaries:**
```c
player.pos.x = Clampf(player.pos.x, PLAY_X + 8.0f, PLAY_X + PLAY_W - 8.0f);
player.pos.y = Clampf(player.pos.y, 8.0f, SCREEN_H - 8.0f);
```
- Prevents going off-screen

### Focus Mode (Shift key)
```c
playerFocus = IsKeyDown(KEY_LEFT_SHIFT);
```

When focused:
- Player speed reduced to 120 px/s (50%)
- Hitbox reduced (visual indication)
- Allows more precise dodging

### Shooting System
```c
if (IsKeyDown(KEY_Z)) {
    player.shootTimer -= dt;
    if (player.shootTimer <= 0.0f) {
        player.shootTimer = 0.1f;  // 10 shots/second
        FirePlayerBullet();
    }
}
```

**Fire Rate:** 10 bullets/second (0.1s cooldown)

**Bullet Spawn:** Two bullets offset left and right from player center
```c
float offsets[] = { -8.0f, 8.0f };
Vector2 spawn = (Vector2){ player.pos.x + offsets[k], player.pos.y - 10.0f };
```

**Aimed Shooting:**
- Normal: straight up (-Y direction)
- Focused: toward closest enemy using `DirectionTo()`

### Bomb System
```c
if (IsKeyPressed(KEY_X) && player.bombs > 0 && player.bombTimer <= 0.0f) {
    UseBomb();
}
```

**Bomb Effect:**
- Clears all enemy bullets
- Stuns all enemies (0.15s stun time)
- Creates explosion particle effect
- 2.5 second cooldown between uses
- Radius: 300 pixels

### Damage & Invincibility
```c
if (player.invincTimer <= 0.0f) {
    // Can take damage
    player.lives--;
    player.invincTimer = 3.0f;  // 3 seconds of invincibility
}

// During invincibility, player blinks (drawn every other frame)
if (((int)(player.invincTimer * 6.0f)) % 2 == 0) {
    DrawPlayer();
}
```

**Invincibility Duration:** 3 seconds (6 blinks)

### Player Drawing
```c
DrawCircleV(player.pos, PLAYER_RADIUS, (Color){200, 220, 255, 255});
DrawCircleV(player.pos, PLAYER_GFX_R, (Color){150, 200, 255, 150});
```

- Inner circle: 5px radius (collision)
- Outer circle: 14px radius (visual indicator)
- Colors: Light blue with transparency

---

## ENEMY SYSTEM

### Enemy Waves (WAVES Array)
20 predefined waves with parameters:
```c
{ spawnTime, startIdx, count, startX, spacingX, startY, targetY, interval, stay, 
  color, enterDir, exitDir, moveType }
```

**Example:** First wave spawns at t=0
- 6 enemies (indices 0-5)
- Enter from TOP at X=[40, 100, 160, 220, 280, 340]
- Target Y = 110
- Color: Pink (255, 80, 120)

### Wave Spawning (SpawnWave)
```c
// Run every frame, check if stgTimer >= WAVES[nextWave].spawnTime
while (nextWave < WAVE_COUNT && stgTimer >= WAVES[nextWave].spawnTime) {
    SpawnWave(nextWave);
    nextWave++;  // Advance to next wave definition
}
```

### Enemy States & Transitions

#### STATE 1: ENTERING
```c
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
```

**Entry Speed:** 2.5-3.0 px/frame (≈150-180 px/s)

#### STATE 2: HOVERING/MOVING
After reaching baseX/baseY, enter movement pattern:

**MOVE_HOVER:** Sine wave side-to-side
```c
e->pos.x = e->baseX + sinf(e->moveTimer * 1.5f) * 18.0f;
// Oscillates ±18px from baseX
// Frequency: 1.5 rad/s
```

**MOVE_SWEEP:** Linear sweep across screen
```c
float sweepDir = (e->enterDir == ENTER_LEFT) ? 1.0f : -1.0f;
e->pos.x += sweepDir * 1.8f;  // Move left to right or vice versa
```

**MOVE_ZIGZAG:** Diagonal downward with zigzag
```c
e->pos.x = e->baseX + sinf(e->moveTimer * 3.0f) * 40.0f;  // ±40px zigzag
e->pos.y += 0.4f;  // Slow descent
```

#### STATE 3: SHOOTING
```c
e->shootTimer -= dt;
if (e->shootTimer <= 0) {
    e->shootTimer = e->shootInterval;
    EnemyShootPattern(e);
}
```

**Shoot Patterns:** Based on movement elapsed time
- Pattern 0: Single aimed shot
- Pattern 1: Aimed + 3-bullet spread
- Pattern 2: 3-bullet spread only

#### STATE 4: LEAVING
```c
if (e->stayTimer >= e->stayDuration || sweepDone) {
    e->leaving = true;
}

if (e->leaving) {
    float exitSpd = 3.5f;
    if (e->exitDir == EXIT_BOTTOM) e->pos.y += exitSpd;
    // ... other directions
    
    if (e->pos.x < PLAY_X - 60 || e->pos.x > PLAY_X + PLAY_W + 60 ||
        e->pos.y < -60 || e->pos.y > SCREEN_H + 60) {
        e->active = false;  // Remove from game
    }
}
```

**Exit Speed:** 3.5 px/frame (≈210 px/s)

### Enemy HP & Stun
- **Base HP:** 5 points
- **Hit by player bullet:** -1 HP
- **Hit by bomb:** Stun for 0.15 seconds (can't shoot or move)

### Enemy Rendering
```c
DrawCircleV(e->pos, e->radius, body);  // Outer circle with color
DrawCircleV(e->pos, e->radius - 4.0f, (Color){255, 255, 255, 200});  // Inner white

// Health bar above enemy
float hpRatio = (float)e->hp / e->maxHp;
DrawRectangle(..., 36 * hpRatio, 4, GREEN);  // Green bar
```

---

## BOSS SYSTEM

### Boss Initialization (InitBoss)
```c
boss.pos = (Vector2){ PLAY_X + PLAY_W / 2.0f, -90.0f };  // Spawn off-screen top
boss.radius = 38.0f;
boss.hp = boss.maxHp = BOSS_MAX_HP;  // 450 HP
boss.phase = 0;  // Start at phase 0
boss.rage = false;
boss.entered = false;
```

**Hell Mode Adjustment:**
```c
if (hellMode) {
    boss.hp = boss.maxHp = (int)(BOSS_MAX_HP * 1.8f);  // 810 HP
}
```

### Boss Entry Animation (UpdateBoss)
```c
if (!boss.entered) {
    boss.pos.y += 95.0f * dt;  // Move down at 95 px/s
    if (boss.pos.y >= BOSS_ENTER_Y) {  // BOSS_ENTER_Y = 105
        boss.pos.y = BOSS_ENTER_Y;
        boss.entered = true;
    }
    return;  // Don't attack during entry
}
```

**Entry Time:** 105 / 95 ≈ 1.1 seconds to reach starting Y

### Boss Sine-Wave Movement
```c
boss.moveAngle += dt * (1.0f + boss.phase * 0.2f + (boss.rage ? 0.35f : 0.0f));
float moveAmp = 90.0f + boss.phase * 12.0f + (boss.rage ? 25.0f : 0.0f);
float bobAmp = 16.0f + boss.phase * 4.0f + (boss.rage ? 8.0f : 0.0f);

boss.pos.x = PLAY_X + PLAY_W / 2.0f + sinf(boss.moveAngle) * moveAmp;
boss.pos.y = BOSS_ENTER_Y + sinf(boss.moveAngle * 2.1f) * bobAmp;
```

**Movement Pattern:**
- Horizontal oscillation: centered ±(90-150)px
- Vertical bobbing: ±(16-28)px
- `moveAngle` increases over time creating smooth sine curves
- `moveAngle * 2.1` creates different frequency for Y (more bobbing)

**Amplitude Scaling:**
- Phase 0: ±90px horizontal, ±16px vertical
- Phase 3 + Rage: ±150px horizontal, ±28px vertical
- Increases difficulty: harder to predict where boss will be

### Boss HP Phases
```c
float ratio = (float)boss.hp / (float)boss.maxHp;
int newPhase;
if (ratio > 0.70f)      newPhase = 0;  // 70-100% HP
else if (ratio > 0.45f) newPhase = 1;  // 45-70% HP
else if (ratio > 0.20f) newPhase = 2;  // 20-45% HP
else                    newPhase = 3;  // 0-20% HP
```

**Phase Determines:**
- Attack patterns become more complex
- Bullet speed increases: 2.8 + phase*0.45 px/frame
- Attack rate increases
- Larger movement patterns (harder to dodge)

### Boss Rage Mode Transition
Triggered at 50% HP:
```c
if (!boss.rage && boss.hp <= boss.maxHp / 2) {
    BossEnterRage();
}
```

**BossEnterRage() Effects:**
- `boss.rage = true`
- `boss.transforming = true` (2.8s transformation)
- HP reset to `BOSS_RAGE_HP` (380, or 684 in hell mode)
- Clear all bullets
- Spawn visual effect particles
- Add 15000 score points
- New name: "Letty - Blood Lunatic"

**Rage Mode Changes:**
- Bullet speed: additional +0.65-0.85 px/frame
- Attack patterns become much more complex
- Attack rate increases (0.82s instead of 1.05s)

### Boss Attack System

#### Attack Interval
```c
boss.attackTimer -= dt;
float interval = 1.05f - boss.phase * 0.12f;  // 1.05 to 0.69 seconds
if (boss.rage) interval = 0.82f - boss.phase * 0.1f;  // 0.82 to 0.42 seconds
if (hellMode) interval *= 0.65f;  // 65% of normal
```

Each attack spawns a pattern of bullets. Interval determines frequency.

#### Attack Patterns (Normal Mode)
**Phase 0:**
- Spread patterns (5 bullets in arc)
- Ring patterns (bullets in circle)
- Aimed bursts at player

**Phase 1:**
- Triple rings (multiple concentric circles)
- Aimed bursts with more bullets
- Corkscrews (spiraling patterns)

**Phase 2-3:**
- Extreme patterns combining multiple types
- Starburst (bullets radiating from center + aimed)
- Lane walls (dense walls of bullets)
- Ring gaps (ring with gaps that shift)

#### Aimed Shots (Continuous)
```c
boss.phaseTimer += dt;
float aimInterval = 1.35f - boss.phase * 0.15f;  // 1.35 to 0.75 seconds
if (boss.rage) aimInterval = 1.0f - boss.phase * 0.12f;  // 1.0 to 0.52 seconds

if (boss.phaseTimer >= aimInterval) {
    boss.phaseTimer = 0.0f;
    int shots = 1 + boss.phase / 2;  // 1-3 shots
    if (boss.rage) shots = 2 + boss.phase / 2;  // 2-4 shots
    if (hellMode) shots = (int)(shots * 1.4f);
    
    for (int i = 0; i < shots; i++) {
        FireAimed(boss.pos, BossBulletSpeed() * 0.95f, ...);
    }
}
```

In addition to pattern attacks, boss fires aimed shots at player at separate intervals.

### Boss Bullet Properties

#### Speed Calculation
```c
float BossBulletSpeed(void) {
    float spd = 2.8f + boss.phase * 0.45f;  // 2.8 to 4.15 px/frame
    if (boss.rage) spd += 0.65f + boss.phase * 0.2f;  // Additional +0.85 to +1.45
    if (hellMode) spd *= 1.35f;  // Hell mode boost
    return spd;
}
```

**Speed Range:**
- Normal Phase 0: 2.8 px/frame (~170 px/s)
- Normal Phase 3: 4.15 px/frame (~250 px/s)
- Rage Phase 0: 3.45 px/frame (~210 px/s)
- Rage Phase 3: 5.6 px/frame (~340 px/s)
- Hell Mode: 1.35x multiplier on all

#### Bullet Size
```c
float BossBulletRadius(void) {
    float r = 3.8f - boss.phase * 0.15f;  // 3.8 to 3.2 px radius
    if (boss.rage) r -= 0.25f;  // Slightly smaller in rage
    return r;
}
```

Smaller in later phases (higher density patterns, harder dodging)

#### Bullet Colors
- **Main bullets (normal):** Light blue (180, 220, 255)
- **Accent bullets (normal):** Pink/red (255, 60, 100)
- **Main bullets (rage):** Red (255, 50, 70)
- **Accent bullets (rage):** Dark red (160, 10, 30)

### Boss Death Animation
```c
if (boss.hp <= 0) {
    BossOnHpDepleted();
}

static void BossOnHpDepleted(void) {
    if (!boss.rage) {
        BossEnterRage();  // If not in rage, enter it
    } else {
        SpawnBossDefeatBurst(boss.pos);  // Particle explosion
        boss.deathTimer = 2.5f;  // 2.5 second fade animation
        phase = PHASE_BOSS_DEATH;
        score += 50000;
    }
}
```

**Death Sequence:**
1. Boss must be in rage mode to die
2. When HP ≤ 0 in rage: spawn particles
3. Enter PHASE_BOSS_DEATH for 2.5s
4. During phase: `DrawBoss()` plays `texBossDeath` frames and fades alpha with `deathTimer`
5. After timer: transition to PHASE_WIN

### Boss Sprite Drawing (DrawBoss)
The boss is drawn using `DrawTexturePro()` with a 128×128 destination rectangle centered on `boss.pos`.
Three different states select a different texture and frame:

```c
Rectangle sourceRec = { (float)(frameIndex * 128), 0.0f, 128.0f, 128.0f };
Rectangle destRec   = { boss.pos.x, boss.pos.y, 128.0f, 128.0f };
Vector2 origin      = { 64.0f, 64.0f };  // Center pivot
DrawTexturePro(currentTex, sourceRec, destRec, origin, 0.0f, tint);
```

| State | Texture | Frames | Frame Formula | Tint |
|---|---|---|---|---|
| `PHASE_BOSS_DEATH` | `texBossDeath` | 8 | `((2.5f - deathTimer) / 2.5f) * 8` | Fades α with `deathTimer` |
| `boss.transforming` | `texBossTransition` | 4 | `(BOSS_TRANSFORM_TIME - transformTimer) * 10 % 4` | Pulses red↔white at 8 Hz |
| Normal / Rage | `texBossMain` | 6 | `(animTimer * 8) % 6` | Stunned=grey, Rage=slight red, else WHITE |

**`animTimer` usage:**
```c
// Updated every frame (even while transforming/entering)
boss.animTimer += dt;

// Frame selection in idle/rage state
frameIndex = (int)(boss.animTimer * 8.0f) % 6;  // 8 fps, 6 frames
```
This creates a looping 6-frame idle animation at approximately 8 frames per second, independent of boss state.

---

## BULLET SYSTEMS

### Player Bullets
```c
static Bullet playerBullets[MAX_BULLETS];  // Array of 1200 bullets
```

**Firing:**
```c
void FirePlayerBullet(void) {
    float offsets[] = { -8.0f, 8.0f };  // Left and right
    for (int k = 0; k < 2; k++) {
        Vector2 spawn = (Vector2){ player.pos.x + offsets[k], player.pos.y - 10.0f };
        
        Vector2 vel;
        if (playerFocus) {
            // Aimed at closest enemy
            Vector2 target = GetClosestEnemyTarget();
            vel = DirectionTo(spawn, target, 14.0f);
        } else {
            // Straight up
            vel = (Vector2){ 0, -14.0f };
        }
        
        // Add to array
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
```

**Update:**
```c
void UpdatePlayerBullets(void) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!playerBullets[i].active) continue;
        
        playerBullets[i].pos.x += playerBullets[i].vel.x;
        playerBullets[i].pos.y += playerBullets[i].vel.y;
        
        // Deactivate if off-screen
        if (playerBullets[i].pos.x < PLAY_X - 20 ||
            playerBullets[i].pos.x > PLAY_X + PLAY_W + 20 ||
            playerBullets[i].pos.y < -20) {
            playerBullets[i].active = false;
        }
    }
}
```

**Speed:** 14.0 px/frame (~840 px/s)
**Lifespan:** Removed when off-screen

### Enemy Bullets
```c
static EnemyBullet enemyBullets[MAX_ENEMY_BULLETS];  // Array of 1400
```

**Types:**
1. **FireAimed:** Aimed at player
   ```c
   Vector2 vel = DirectionTo(from, player.pos, speed);
   ```

2. **FireSpread:** Arc of bullets
   ```c
   float startAngle = centerDeg - spreadDeg / 2.0f;
   float step = spreadDeg / (count - 1);
   for (int i = 0; i < count; i++) {
       float angle = (startAngle + i * step) * DEG2RAD;
       Vector2 vel = { cosf(angle) * speed, sinf(angle) * speed };
   }
   ```
   
   **Example:** FireSpread(pos, 0.0f, 30.0f, 3, speed, ...) = 3 bullets from -15° to +15°

3. **FireRing:** Circle of bullets
   ```c
   for (int i = 0; i < count; i++) {
       float angle = (360.0f / count * i + offsetDeg) * DEG2RAD;
       Vector2 vel = { cosf(angle) * speed, sinf(angle) * speed };
   }
   ```
   
   **Example:** FireRing(pos, 8, speed, 0.0f, ...) = 8 bullets in circle, spaced 45° apart

**Update:**
```c
void UpdateEnemyBullets(float dt) {
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (!enemyBullets[i].active) continue;
        
        enemyBullets[i].pos.x += enemyBullets[i].vel.x;
        enemyBullets[i].pos.y += enemyBullets[i].vel.y;
        
        if (out of bounds) {
            enemyBullets[i].active = false;
        }
    }
}
```

**Speed Range:** 3.2 px/frame (enemies) to 5.6+ px/frame (boss rage)

---

## PARTICLE SYSTEM

### Particle Array
```c
static Particle particles[MAX_PARTICLES];  // 512 particles max
```

### Particle Types
1. **Defeat Burst** - When enemy dies
2. **Bomb Burst** - When bomb explodes
3. **Boss Defeat Burst** - When boss dies
4. **Rage Transform Burst** - When boss enters rage

### Spawning
```c
void SpawnParticleBurst(Vector2 pos, int count, float minSpeed, float maxSpeed, 
                        float minLife, float maxLife, float minSize, float maxSize, 
                        Color color) {
    for (int i = 0; i < count; i++) {
        // Find empty slot
        for (int j = 0; j < MAX_PARTICLES; j++) {
            if (!particles[j].active) {
                // Random direction
                float angle = (360.0f / count * i) * DEG2RAD;
                float speed = GetRandomValue(minSpeed * 100, maxSpeed * 100) / 100.0f;
                
                particles[j].pos = pos;
                particles[j].vel = { cosf(angle) * speed, sinf(angle) * speed };
                particles[j].life = GetRandomValue(minLife * 100, maxLife * 100) / 100.0f;
                particles[j].maxLife = particles[j].life;
                particles[j].size = GetRandomValue(minSize * 100, maxSize * 100) / 100.0f;
                particles[j].color = color;
                particles[j].active = true;
                break;
            }
        }
    }
}
```

### Update
```c
void UpdateParticles(float dt) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].active) continue;
        
        particles[i].pos.x += particles[i].vel.x;
        particles[i].pos.y += particles[i].vel.y;
        particles[i].life -= dt;
        
        if (particles[i].life <= 0.0f) {
            particles[i].active = false;
        }
    }
}
```

### Rendering
```c
void DrawParticles(void) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].active) continue;
        
        // Fade out as life decreases
        float alpha = (particles[i].life / particles[i].maxLife) * 255;
        Color c = particles[i].color;
        c.a = (unsigned char)alpha;
        
        DrawCircleV(particles[i].pos, particles[i].size, c);
    }
}
```

**Effect:** Particles spawn in circle, fly outward, fade out

---

## COLLISION DETECTION

### Player vs Enemy Bullets
```c
for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
    if (!enemyBullets[i].active) continue;
    float d = Dist(player.pos, enemyBullets[i].pos);
    if (d < player.radius + enemyBullets[i].radius) {
        enemyBullets[i].active = false;
        
        if (player.invincTimer <= 0.0f) {
            player.lives--;
            player.invincTimer = 3.0f;  // 3 seconds invincibility
            
            if (player.lives <= 0) {
                phase = PHASE_DEAD;
            }
        }
    }
}
```

**Collision Formula:** `distance(pos1, pos2) < radius1 + radius2`

### Player Bullet vs Enemy
```c
for (int i = 0; i < MAX_BULLETS; i++) {
    if (!playerBullets[i].active) continue;
    
    for (int j = 0; j < MAX_ENEMIES; j++) {
        if (!enemies[j].active) continue;
        
        float d = Dist(playerBullets[i].pos, enemies[j].pos);
        if (d < BULLET_RADIUS + enemies[j].radius) {
            playerBullets[i].active = false;
            enemies[j].hp--;
            
            score += 50;  // Points per hit
            
            if (enemies[j].hp <= 0) {
                enemies[j].active = false;
                SpawnEnemyDefeatBurst(enemies[j].pos);
                score += 500;  // Bonus for kill
            }
        }
    }
}
```

### Player Bullet vs Boss
```c
for (int i = 0; i < MAX_BULLETS; i++) {
    if (!playerBullets[i].active) continue;
    
    if (boss.active && boss.entered && !boss.transforming) {
        float d = Dist(playerBullets[i].pos, boss.pos);
        if (d < BULLET_RADIUS + boss.radius) {
            playerBullets[i].active = false;
            boss.hp--;
            
            score += 200;  // Points per boss hit
            
            if (boss.hp <= 0) {
                BossOnHpDepleted();
            }
        }
    }
}
```

### Bomb Effect
```c
void UseBomb(void) {
    if (player.dead || player.bombs <= 0 || player.bombTimer > 0.0f) return;
    
    player.bombs--;
    player.bombTimer = 2.5f;  // 2.5s cooldown
    
    // Stun all enemies
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i].active) {
            enemies[i].stunTimer = BOMB_STUN_TIME;  // 0.15s
        }
    }
    if (boss.active) {
        boss.stunTimer = BOMB_STUN_TIME;
    }
    
    // Clear all enemy bullets
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        enemyBullets[i].active = false;
    }
    
    SpawnBombBurst(player.pos);
}
```

**Bomb doesn't damage:** Only stuns and clears bullets

---

## GAME LOOP ARCHITECTURE

### Initialization (Main)
```c
InitWindow(SCREEN_W, SCREEN_H, "Danmaku Game");
InitAudioDevice();

// Load boss sprite sheets (must happen after InitWindow)
texBossMain       = LoadTexture("assets/BossSprite.png");
texBossTransition = LoadTexture("assets/Boss2ndPhaseTransition.png");
texBossDeath      = LoadTexture("assets/BossDeath.png");

// Load music
Music musicEnemy = LoadMusicStream("assets/Touhou 7 - Paradise  Deep Mountain (Stage 1).mp3");
Music musicBoss  = LoadMusicStream("assets/Touhou 7 - Letty Whiterock's Theme - Crystallized Silver (Boss 1).mp3");

SetTargetFPS(60);  // 60 FPS lock

phase = PHASE_MENU;
```

**Texture loading must occur after `InitWindow()`** — Raylib requires an active OpenGL context to upload textures to the GPU.

### Main Game Loop
```c
while (!WindowShouldClose()) {
    float dt = GetFrameTime();  // Delta time since last frame
    
    // PHASE HANDLING
    if (phase == PHASE_MENU) {
        UpdateMenu();
        if (IsKeyPressed(KEY_ESCAPE)) break;
        
        // Switch to game at right time
        if (phase != PHASE_MENU) {  // Changed by UpdateMenu
            // Initialize for game start
        }
    } else {
        // Update music
        UpdateMusicStream(currentTrack);
        
        // Handle pause
        if (IsKeyPressed(KEY_P)) {
            paused = !paused;
        }
        
        if (phase == PHASE_WIN || phase == PHASE_DEAD) {
            UpdateEndScreen();
        } else if (!paused) {
            // MUSIC SWITCH: Enemy waves → Boss (at 159 seconds)
            if (!musicSwitched && phase == PHASE_ENEMIES) {
                musicSwitchTimer += dt;
                if (musicSwitchTimer >= 159.0f) {
                    // Switch to boss music
                    musicSwitched = true;
                    // Clear bullets, start boss
                    phase = PHASE_BOSS;
                    InitBoss();
                }
            }
            
            // GAME UPDATES
            if (phase == PHASE_BOSS) {
                UpdateBoss(dt);
            } else if (phase == PHASE_ENEMIES) {
                UpdateEnemies(dt);
            } else if (phase == PHASE_BOSS_DEATH) {
                boss.deathTimer -= dt;
                if (boss.deathTimer <= 0.0f) {
                    phase = PHASE_WIN;
                }
            }
            
            // Handle collisions & player updates (normal gameplay only)
            if (phase == PHASE_ENEMIES || phase == PHASE_BOSS) {
                handleCollisions();
                UpdatePlayer(dt);
                UpdatePlayerBullets();
                UpdateEnemyBullets(dt);
                UpdateParticles(dt);
                stgTimer += dt;  // Increase survival timer
            }
            
            // Continue particles during boss death
            if (phase == PHASE_BOSS_DEATH) {
                UpdateParticles(dt);
            }
        }
    }
    
    // DRAWING
    BeginDrawing();
    DrawBackground();
    
    if (phase == PHASE_MENU) {
        DrawMenu();
    } else {
        // Draw gameplay
        DrawPlayerBullets();
        if (phase == PHASE_ENEMIES) DrawEnemies();
        DrawEnemyBullets();
        if (phase == PHASE_BOSS) DrawBoss();
        if (phase == PHASE_BOSS_DEATH) {
            // Fade out animation
            float fadeAlpha = (boss.deathTimer / 2.5f) * 255.0f;
            DrawBoss();
            DrawRectangle(PLAY_X, 0, PLAY_W, SCREEN_H, 
                         (Color){0, 0, 0, 255 - (int)fadeAlpha});
        }
        DrawParticles();
        DrawBombEffect();
        DrawPlayer();
        DrawHUD();
        
        if (paused) {
            // Pause overlay
        }
    }
    
    EndDrawing();
}

// Cleanup (after main loop)
UnloadTexture(texBossMain);
UnloadTexture(texBossTransition);
UnloadTexture(texBossDeath);
UnloadMusicStream(musicEnemy);
UnloadMusicStream(musicBoss);
CloseAudioDevice();
CloseWindow();
```

**Cleanup order:** Textures and audio streams are unloaded before `CloseWindow()` to avoid GPU/audio resource leaks.

### Update Order
```
1. Input handling
2. Physics updates (movement)
3. Collision detection
4. Score calculation
5. State changes (phase transitions)
6. Visual effects updates
7. Drawing
```

**Critical:** Updates happen BEFORE drawing to avoid frame lag

---

## SPECIAL FEATURES

### Dev Mode
**Activation:** Type "Tohok" at menu and press ENTER

**Effects:**
```c
if (devMode) {
    player.lives = 999;
    player.bombs = 999;
}
```

**Visual Indicator:**
- Shows "[DEV MODE]" at bottom left in green
- Shows "[DEV]" label at menu

**Uses:**
- Test game without dying
- Quick testing of later levels
- Boss pattern observation

### Hell Mode
**Activation:** Type "Mikobrainrot" at menu and press ENTER

**Effects on Enemies:**
```c
e->shootInterval *= 0.6f;  // Enemies shoot 67% faster
```

**Effects on Boss:**
```c
boss.hp *= 1.8f;  // 80% more HP
boss.shootInterval *= 0.65f;  // Attacks 54% faster
shots *= 1.4f;  // 40% more aimed shots
speed *= 1.35f;  // 35% faster bullets
```

**Visual Indicator:**
- Shows "[HELL MODE]" at bottom left in red
- Shows "[HELL]" label at menu

**Independence:**
- Can be combined with Dev Mode
- Can be toggled on/off independently
- Both modes toggle via same input system

### Code Input System
```c
bool UpdateMenuInput(float dt) {
    // Accumulate typed characters in codeInput[]
    int ch = GetCharPressed();
    while (ch > 0) {
        if (ch >= 32 && ch <= 126 && codeLen < HELL_INPUT_MAX) {
            codeInput[codeLen++] = (char)ch;
            codeInput[codeLen] = '\0';
        }
        ch = GetCharPressed();
    }
    
    // Backspace: remove last character
    if (IsKeyPressed(KEY_BACKSPACE) && codeLen > 0) {
        codeLen--;
        codeInput[codeLen] = '\0';
    }
    
    // Enter: check codes
    if (IsKeyPressed(KEY_ENTER) && codeLen > 0) {
        if (strcmp(codeInput, DEV_CODE) == 0) {
            devMode = !devMode;  // Toggle
            devFeedback = 1;
            devMsgTimer = 2.5f;
        } else if (strcmp(codeInput, HELL_CODE) == 0) {
            hellMode = !hellMode;  // Toggle
            hellFeedback = 1;
            hellMsgTimer = 2.5f;
        } else {
            devFeedback = 3;  // Wrong code feedback
            devMsgTimer = 1.5f;
        }
        
        codeLen = 0;
        codeInput[0] = '\0';
        return true;
    }
    
    return false;
}
```

---

## SCORE SYSTEM

```c
score += 50;      // Per player bullet hit on enemy
score += 500;     // Per enemy defeated
score += 200;     // Per player bullet hit on boss
score += 15000;   // When boss enters rage mode
score += 50000;   // When boss is defeated
```

**Total Score Range:**
- Minimum: ~15,000 (boss only, few hits)
- Maximum: ~100,000+ (perfect playthrough)

**Score Updated During:**
- Enemy/boss hits
- Enemy/boss defeats
- Boss phase transitions

---

## KEY CONSTANTS SUMMARY

| Constant | Value | Meaning |
|----------|-------|---------|
| PLAY_W | 400 | Playable area width |
| HUD_SIDE_W | 150 | Side panel width each |
| SCREEN_W | 700 | Total screen width |
| SCREEN_H | 640 | Screen height |
| PLAYER_SPEED | 240 px/s | Normal movement speed |
| PLAYER_FOCUS | 120 px/s | Focused movement speed |
| BOSS_MAX_HP | 450 | Boss normal HP |
| BOSS_RAGE_HP | 380 | Boss rage mode HP |
| BOMB_DURATION | 2.5s | Bomb cooldown |
| BOMB_STUN_TIME | 0.15s | Enemy stun from bomb |
| BOSS_TRANSFORM_TIME | 2.8s | Rage mode animation |
| MAX_BULLETS | 1200 | Player bullet pool |
| MAX_ENEMY_BULLETS | 1400 | Enemy bullet pool |
| MAX_ENEMIES | 12 | Simultaneous enemies |
| MAX_PARTICLES | 512 | Visual effects |
| FPS | 60 | Target frame rate |

---

## MATHEMATICAL CONCEPTS USED

### 1. Euclidean Distance (Collision)
`d = √((x₂-x₁)² + (y₂-y₁)²)`

### 2. Vector Normalization (Direction)
`normalized = vec / |vec|` where `|vec| = √(x² + y²)`

### 3. Trigonometric Functions (Angles & Movement)
- `sin(θ)` and `cos(θ)` for circular/oscillating movement
- `atan2(y, x)` for calculating angle toward target
- `DEG2RAD` conversion for calculations
- `RAD2DEG` conversion for output

### 4. Linear Interpolation (Fading)
`alpha = (currentLife / maxLife) * 255` for particle fading

### 5. Modulo Operation (Looping)
`step % RAGE_PATTERNS_PER_PHASE` for cycling attack patterns

### 6. Frame-Rate Independence
All movement uses `dt` (delta time) multiplier:
`position += velocity * dt`
Ensures consistent speed regardless of frame rate

---

## EXECUTION FLOW DIAGRAM

```
START
  ↓
Initialize Game
  ↓
[MAIN LOOP]
  ├─ Input Processing
  │   ├─ Player movement (arrow keys / WASD)
  │   ├─ Shooting (Z)
  │   ├─ Bomb (X)
  │   └─ Code input (typing)
  │
  ├─ Update Phase
  │   ├─ PHASE_MENU: Handle code input, wait for start
  │   ├─ PHASE_ENEMIES: Spawn waves, update enemies
  │   ├─ PHASE_BOSS: Update boss AI and patterns
  │   ├─ PHASE_BOSS_DEATH: 2.5s fade animation
  │   └─ PHASE_WIN/DEAD: Display screens
  │
  ├─ Collision Detection
  │   ├─ Player vs Enemy Bullets
  │   ├─ Player Bullets vs Enemies
  │   ├─ Player Bullets vs Boss
  │   └─ Bomb effects
  │
  ├─ State Updates
  │   ├─ Enemy health/status
  │   ├─ Boss health/phase
  │   ├─ Score calculation
  │   └─ Phase transitions
  │
  ├─ Draw Phase
  │   ├─ Background
  │   ├─ All bullets
  │   ├─ All enemies/boss
  │   ├─ Player
  │   ├─ Particles
  │   └─ HUD
  │
  └─ Repeat if !WindowShouldClose()

EXIT
```

---

This comprehensive explanation covers every aspect of the code. The game is a well-structured bullet hell that demonstrates:

- **Efficient pooling:** Pre-allocated arrays for bullets/enemies/particles
- **State machine:** Clear phase-based game flow
- **Mathematical accuracy:** Proper vector math for collision and movement
- **Frame-rate independence:** All timing based on dt
- **Progressive difficulty:** Waves increase in complexity, boss has multiple phases
- **Visual feedback:** Colors, particles, health bars for all entities

