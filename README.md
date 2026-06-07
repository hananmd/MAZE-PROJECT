<!-- MAZE OF UCSC — README -->

<div align="center">

```
███╗   ███╗ █████╗ ███████╗███████╗     ██████╗ ███████╗    ██╗   ██╗ ██████╗███████╗ ██████╗
████╗ ████║██╔══██╗╚══███╔╝██╔════╝    ██╔═══██╗██╔════╝    ██║   ██║██╔════╝██╔════╝██╔════╝
██╔████╔██║███████║  ███╔╝ █████╗      ██║   ██║█████╗      ██║   ██║██║     ███████╗██║
██║╚██╔╝██║██╔══██║ ███╔╝  ██╔══╝      ██║   ██║██╔══╝      ██║   ██║██║     ╚════██║██║
██║ ╚═╝ ██║██║  ██║███████╗███████╗    ╚██████╔╝██║         ╚██████╔╝╚██████╗███████║╚██████╗
╚═╝     ╚═╝╚═╝  ╚═╝╚══════╝╚══════╝     ╚═════╝ ╚═╝          ╚═════╝  ╚═════╝╚══════╝ ╚═════╝
```

### 🧩 Snake & Ladders Variant — *"Maze to Savor!"*

![Status](https://img.shields.io/badge/Status-Fully%20Implemented-00ff88?style=flat-square&logo=checkmarx&logoColor=white)
![Language](https://img.shields.io/badge/Language-C-00e5ff?style=flat-square&logo=c&logoColor=white)
![Players](https://img.shields.io/badge/Players-3-ffb800?style=flat-square)
![Floors](https://img.shields.io/badge/Floors-3-7c5cbf?style=flat-square)
![Tests](https://img.shields.io/badge/Tests-7%2F7%20Passed-00ff88?style=flat-square&logo=pytest&logoColor=white)
![Module](https://img.shields.io/badge/Module-SCS1301-ff4757?style=flat-square)

*A turn-based maze game where dice rolls, stairs, poles, and the cursed Bawana zone decide your fate.*

</div>

---

## 📖 Overview

**Maze of UCSC** is a 3-player, 3-floor turn-based maze game implemented in pure C. Players roll dice to move through a complex maze, utilize stairs and poles for teleportation, avoid walls, survive randomized **Bawana** effects, and race to capture the flag.

The game is built with robust error handling, BFS-based flag validation, infinite loop detection, and a full logging system — making it not just a game, but a demonstration of solid data structures and program design.

---

## ⚡ Core Systems

| System | Description |
|:--|:--|
| 🎲 **Dice Movement** | Direction-aware movement with MP cost per cell traversed |
| 🪜 **Dynamic Stairs** | Direction flips every 5 rounds — UP / DOWN / BIDIRECTIONAL |
| 🪝 **One-Way Poles** | Slide down only; higher priority than stairs on the same cell |
| 🧱 **Wall Obstacles** | Configurable walls that block movement; auto-sanitized on spawn zones |
| 🌀 **Bawana Zone** | 12 special cells, 6 chaotic effects — enter only at MP = 0 |
| ⚡ **MP System** | Movement Points with bonuses, penalties, and a 250 cap |
| 💥 **Player Capture** | Land on an opponent → they reset to `[0,6,12]` facing NORTH |
| 🛡️ **Robust Error Handling** | Infinite loop detection, wall sanitization, BFS flag validation |
| 📝 **Logging System** | All events, assumptions, and warnings written to `log.txt` |

---

## 🚀 Compile & Run

> Requires `gcc`. The `-lm` flag links the math library.

**Linux / macOS**
```bash
gcc -o maze main.c game.c -lm
./maze
```

**Windows (MinGW / MSYS2)**
```bash
gcc -o maze.exe main.c game.c -lm
maze.exe
```

Place all files in the same directory as the executable and press **Enter** after each turn when prompted.

---

## 📂 Input Files

All config files are optional. Place them beside the executable.

| File | Format | Example |
|:--|:--|:--|
| `seed.txt` | Single integer for `rand()` seeding | `12345` |
| `stairs.txt` | `[start floor, start w, start l, end floor, end w, end l]` | `[0,5,10,1,5,10]` |
| `poles.txt` | `[start floor, end floor, w, l]` | `[2,0,5,24]` |
| `walls.txt` | `[floor, start w, start l, end w, end l]` | `[1,0,2,8,2]` |
| `flag.txt` | `[floor, w, l]` | `[1,3,8]` |

> ⚠️ Invalid or missing files load defaults and log warnings.

---

## 🌀 Bawana Zone Effects

> The Bawana area consists of 12 cells at `[0, 6–9, 20–24]`. Entry requires **MP = 0** and forces direction **NORTH**.

| Effect | Duration | What Happens |
|:--|:--|:--|
| 🤢 **Food Poisoning** | — | Skip 3 turns → placed randomly inside Bawana → new effect |
| 😵 **Disoriented** | 4 turns | Random direction each move |
| ⚡ **Triggered** | 4 turns | Move 2× the rolled amount |
| 😄 **Happy** | Instant | +200 MP, no lasting effect |
| 🎲 **Random MP** | 4 turns | Gain +10 to +100 MP per turn |
| 💀 **MP = 0** | — | Sent to random Bawana interior cell → new effect applied |

---

## ⚙️ Design Decisions & Assumptions

<details>
<summary><b>Click to expand full assumptions table</b></summary>

| Feature | Logic |
|:--|:--|
| **Bawana Interior** | 12 cells at `[0, 6–9, 20–24]`. Contains 2 of each effect type + 4 random MP cells |
| **Bawana Entrance** | Cells `[0,9,19]`. Enterable **only** when MP = 0. Forces direction NORTH |
| **Starting Area** | `[0, 6–9, 8–16]`. Players must roll a **6** to enter the maze |
| **Player Entry** | A: `[0,5,12]` NORTH · B: `[0,9,7]` WEST · C: `[0,9,17]` EAST |
| **Captured Reset** | Reset to `[0,6,12]`, direction NORTH. MP preserved |
| **Movement Cost** | Each cell has a consumable value (0–4) deducted from MP |
| **MP Bonuses** | 25% → +1–2 MP · 10% → +3–5 MP · 5% → 2× or 3× MP (capped at 250) |
| **Flag Validation** | BFS at startup. Unreachable flag replaced with a valid cell |
| **Infinite Loops** | Detected if position repeats within 100 steps. Reset to `[0,6,12]` |
| **Wall Sanitization** | Walls on spawn/Bawana/start cells auto-disabled and logged |
| **Overlap Priority** | Pole > Stair. Tiebreak: Manhattan distance to flag. Deterministic |

</details>

---

## 🛡️ Special Game Mechanics

### 1. Infinite Loop Detection
Tracks the last 100 positions during movement. If a revisit occurs, the player is reset to `[0,6,12]` with MP preserved and direction set to NORTH.
```
LOOP DETECTION: Player B trapped in infinite loop at [1,5,10]...
```

### 2. Flag Capture Mid-Movement
The game ends **immediately** when the flag is captured — even if movement steps remain.
```
FLAG CAPTURE OVERRIDE: Player A captured flag at step 2...
```

### 3. Wall Sanitization
Walls overlapping critical zones (spawn, Bawana, start cells) are automatically removed at initialization.
```
ASSUMPTION: Wall at [0,5,12]-[5,12] disabled — overlaps spawn cell
```

### 4. Stair Direction Updates
Every 5 rounds, stair directions are randomly updated. Logged for transparency.
```
ASSUMPTION: Stair directions updated after 5 rounds.
```

### 5. Multi-Object Priority (Same Cell)
- **Priority:** Pole > Stair
- **Tiebreak:** Manhattan distance to flag
- **Determinism:** First encountered wins on distance tie — no randomness

---

## ✅ Critical Test Cases

| # | Test Case | Description | Result |
|:--|:--|:--|:--:|
| TC1 | Walls on Spawn Cells | Walls disabled → Players enter normally | ✅ PASS |
| TC2 | Walls Inside Bawana | Walls removed → Bawana accessible | ✅ PASS |
| TC3 | Infinite Stair Loop | Loop detected → Player reset, MP preserved | ✅ PASS |
| TC4 | Stair-Pole Infinite Loop | Combined loop resolved cleanly | ✅ PASS |
| TC5 | Flag in Unreachable Location | BFS fails → Flag replaced → Warning logged | ✅ PASS |
| TC6 | Overlapping Stairs and Poles | Pole priority respected, distance-based selection | ✅ PASS |
| TC7 | Max Stair Density (2 per cell) | Distance selection + direction permission validation | ✅ PASS |

---

## 📝 Logging System

All critical events are automatically written to `log.txt`.

```
ASSUMPTION: Wall at [0,5,12]-[5,12] disabled — overlaps spawn cell
WARNING: Flag in flag.txt at [1,5,5] is unreachable, replacing...
LOOP DETECTION: Player B trapped in infinite loop at [1,5,10]...
FLAG CAPTURE OVERRIDE: Player A captured flag at step 2...
ASSUMPTION: Stair directions updated after 5 rounds.
```

---

## 📁 Project Structure

```
maze-of-ucsc/
├── main.c          # Entry point, game loop
├── game.c          # Core game logic
├── game.h          # Structs, constants, declarations
├── seed.txt        # (optional) RNG seed
├── stairs.txt      # (optional) Stair configurations
├── poles.txt       # (optional) Pole configurations
├── walls.txt       # (optional) Wall configurations
├── flag.txt        # (optional) Flag position
└── log.txt         # Auto-generated runtime log
```

---

## 👤 Author

<div align="center">

**M.Y Hanan Mohamed**
University of Colombo School of Computing
SCS1301 — Data Structures and Program Design in C
September 14, 2025

</div>

---

<div align="center">

*Built with C. Powered by dice rolls and questionable luck in the Bawana zone.*

</div>
