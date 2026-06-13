# 🎮 DANMAKU — Bullet Hell Game

A Touhou-inspired bullet hell game built in C using [raylib](https://www.raylib.com/), featuring one full stage, multiple enemy waves, a two-phase boss, particle effects, animated sprites, and a secret developer mode.

---

## 📋 Requirements

| Dependency | Version |
|---|---|
| [raylib](https://www.raylib.com/) | 4.x or later |
| GCC / MinGW | Any recent version |
| OS | Windows / Linux / macOS |

---

## 🔨 Build Instructions

```bash
gcc danmaku.c -o Danmaku -lraylib -lm
```

On Windows with MinGW:
```bash
gcc danmaku.c -o Danmaku.exe -lraylib -lm -lopengl32 -lgdi32 -lwinmm
```

---

## 📁 Required Assets

All assets go inside an `assets/` folder in the same directory as the executable.

### Audio
| File | Description |
|---|---|
| `Touhou 7 - Paradise  Deep Mountain (Stage 1).mp3` | Stage BGM (plays during enemy phase, 2:39) |
| `Touhou 7 - Letty Whiterock's Theme - Crystallized Silver (Boss 1).mp3` | Boss BGM (plays during boss phase, 2:08) |

### Sprites
| File | Description |
|---|---|
| `SakuyaIzayoi.png` | Player sprite sheet — 2 rows, 4 frames each, frame size 32×48 px |
| `EnemySprite.png` | Enemy sprite sheet — 1 row, 6 frames, frame size 32×36 px |
| `BossSprite.png` | Boss idle/main sprite sheet — 1 row, 6 frames, 128×128 px per frame |
| `Boss2ndPhaseTransition.png` | Boss rage transition — 1 row, 4 frames, 128×128 px |
| `BossDeath.png` | Boss death animation — 1 row, 8 frames, 128×128 px |

> **Note:** Frame sizes can be adjusted via the `#define` constants at the top of `danmaku.c` (`PLAYER_FRAME_W`, `PLAYER_FRAME_H`, `ENEMY_FRAME_W`, etc.)

---

## 🕹️ Controls

| Key | Action |
|---|---|
| `Arrow Keys` / `WASD` | Move player |
| `Z` (hold) | Shoot |
| `X` | Use bomb — clears all enemy bullets |
| `Shift` (hold) | Focus mode — slower movement, smaller effective hitbox |
| `P` | Pause / Resume |
| `ESC` | Quit (from menu) |
| `Enter` / `Space` / `Z` | Start game (from menu) |

---

## 🎮 Gameplay

### Stage Flow

```
PHASE_ENEMIES  →  PHASE_BOSS  →  PHASE_BOSS_DEATH  →  PHASE_WIN
                                         ↓
                                    PHASE_DEAD (if lives = 0)
```

- **Enemy Phase** lasts exactly as long as the stage BGM (159 seconds). Enemies spawn in timed waves regardless of whether previous waves have been cleared.
- **Boss Phase** begins automatically when the music switches. The boss slides in from the top and begins attacking.
- **Boss Rage** is triggered when the boss reaches 0 HP for the first time. HP is refilled, the boss transforms visually, and attack patterns become significantly more aggressive.

### Scoring

| Event | Points |
|---|---|
| Hit an enemy | +50 |
| Destroy an enemy | +200 |
| Hit the boss (normal) | +80 |
| Hit the boss (rage) | +100 |
| Boss first phase cleared (rage triggered) | +15,000 |
| Boss fully defeated | +50,000 |

### Player
- **3 lives** by default — losing all lives ends the game.
- **3 bombs** by default — each bomb clears all enemy bullets on screen and grants brief invincibility.
- **Focus mode** (hold Shift): moves slower, bullets aim toward the nearest enemy/boss, hitbox becomes visible as a small red dot.

### Boss — Letty Whiterock
The boss has **4 sub-phases** determined by remaining HP percentage (70% / 45% / 20% thresholds). Each phase uses different bullet patterns:

| Phase | HP % | Patterns |
|---|---|---|
| 0 | >70% | Ring bursts, small spreads, aimed shots |
| 1 | 45–70% | Double rings, wide spreads, spiral |
| 2 | 20–45% | Combined spirals, starburst, pincer shots |
| 3 | <20% | All patterns combined, maximum density |

After rage activation, a separate set of 8 more aggressive patterns replaces the normal ones.

---

## 🔐 Secret Codes

Enter these in the text field on the main menu, then press `Enter`:

| Code | Effect |
|---|---|
| `Tohok` | Toggle **Dev Mode** — infinite lives and bombs, brief invincibility instead of dying |
| `Mikobrainrot` | Toggle **Hell Mode** — enemy fire rate ×1.67, boss HP ×1.8, boss bullet speed ×1.35 |

Active modes are shown in the bottom-left corner during gameplay (`[DEV MODE]` / `[HELL MODE]`).

---

## 🗂️ Code Structure

```
danmaku.c
├── Constants & Defines
├── Enums
│   ├── GamePhase        — MENU, ENEMIES, BOSS, BOSS_DEATH, WIN, DEAD
│   ├── BossAttack       — SPREAD, SPIRAL, AIMED, RING
│   ├── EnemyEnter       — TOP, LEFT, RIGHT
│   ├── EnemyExit        — BOTTOM, LEFT, RIGHT, TOP
│   └── EnemyMove        — HOVER, SWEEP, ZIGZAG
├── Structs
│   ├── Player, Bullet, EnemyBullet
│   ├── Enemy, Boss, Particle
│   └── EnemyWave
├── Global State
├── Helper Functions     — Dist, DirectionTo, Clampf
├── Wave Table (WAVES[]) — 20 timed enemy waves
├── Bullet Spawners      — FireEnemyBullet, FireAimed, FireSpread, FireRing
├── Player System        — Init, Update, Draw, Bullets
├── Particle System      — SpawnParticleEx, SpawnParticleBurst, Update, Draw
├── Enemy System         — InitEnemies, SpawnWave, UpdateEnemies, DrawEnemies
├── Boss System
│   ├── Normal patterns  — BossRunPhasePattern (4 phases × 4 patterns)
│   └── Rage patterns    — BossRunRagePhasePattern (4 phases × 8 patterns)
├── Collision Detection  — handleCollisions, HandlePlayerHits, DamagePlayer
├── HUD                  — DrawHUD, DrawEndScreen
├── Background           — DrawBackground (scrolling stars)
├── Menu System          — DrawMenu, UpdateMenu, UpdateMenuInput
└── main()               — Game loop
```

---

## ⚙️ Configuration

Key values you can tweak at the top of `danmaku.c`:

| Constant | Default | Description |
|---|---|---|
| `BOSS_MAX_HP` | 450 | Boss HP before rage |
| `BOSS_RAGE_HP` | 380 | Boss HP during rage |
| `PLAYER_SPEED` | 240.0 | Normal movement speed (px/s) |
| `PLAYER_FOCUS` | 120.0 | Focus mode movement speed (px/s) |
| `PLAYER_MAX_BOMBS` | 3 | Starting bomb count |
| `BOMB_DURATION` | 2.5 | Seconds the bomb effect lasts |
| `PLAYER_FRAME_W/H` | 32 / 48 | Player sprite frame dimensions |
| `ENEMY_FRAME_W/H` | 32 / 36 | Enemy sprite frame dimensions |
| `ENEMY_FRAME_COUNT` | 6 | Number of animation frames per enemy |
| `DEV_CODE` | `"Tohok"` | Secret code for dev mode |
| `HELL_CODE` | `"Mikobrainrot"` | Secret code for hell mode |

---

## 📝 License

This project is for educational purposes. Music and character names are from the Touhou Project by ZUN / Team Shanghai Alice.
