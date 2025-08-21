<div align="center">

# Maze Runner UCSC

Comprehensive C implementation of the three‑floor maze / flag capture game with movement points, dynamic stairs, per‑cell effects, Bawana special zone, and full statistics reporting.

</div>

---

## 1. At a Glance
Goal: Up to 3 players race inside a multi‑floor maze to capture a randomly placed flag while managing Movement Points (MP), navigating stairs/poles, and surviving Bawana effects.

Core Mechanics Implemented:
- Exact turn structure (entry requires rolling 6; entry consumes the turn).
- Movement die every turn; direction die every 4th in‑maze turn (empty faces = maintain direction).
- Per‑step effect application (consumables, additive, multiplier) directly modifies MP.
- Bawana teleport when MP <= 0 with randomized effect type (deterministic distribution mapping).
- Dynamic stair cycle system (configurable) with optional forced one‑way + global up/down alternation.
- Capture sends victim to start & resets direction; capture counts in statistics.
- Rich end‑of‑game summary (steps, captures, visits, stairs/poles usage, Bawana visits, cycles).

---

## 2. Quick Start (Windows PowerShell)
Prerequisite: GCC (MinGW‑w64) or any C11 compiler in PATH.

Install via Chocolatey (optional):
```powershell
choco install mingw -y
$env:Path += ";C:\ProgramData\chocolatey\lib\mingw\tools\install\mingw64\bin"  # current session only
```

Enter project directory (note spaces & ampersand):
```powershell
Set-Location "Your Folder location"
```

Build (without make):
```powershell
gcc -std=c11 -Wall -Wextra -O2 -Iinclude src\*.c -o maze_game.exe
```

Run (time seed) / reproducible seed / debug fast flag:
```powershell
./maze_game.exe              # random seed (time)
./maze_game.exe 42           # fixed seed
./maze_game.exe 42 fastflag  # place flag near Player A for fast testing
```

Using make (if installed):
```powershell
make
make run
```

Stop: Program exits automatically once a player captures the flag or safety round cap reached.

---

## 3. Step‑by‑Step Run Guide
1. Install compiler (see above).
2. Open PowerShell.
3. Navigate to project root (quoted path).
4. (Optional) Clean: `del maze_game.exe` if rebuilding from scratch.
5. Compile (see command above). Ensure no warnings.
6. Execute with a chosen seed for repeatability.
7. Observe per‑turn log: movement die, direction changes, stair/pole traversals, Bawana events.
8. On flag capture, review printed summary statistics.
9. Adjust seeds and rerun to explore alternate paths.

---

## 4. Mapping Rules to Implementation
| Rule | Summary | Implementation Notes |
|------|---------|----------------------|
| 1 | 3 Floors layout | `maze.c::mark_floor_layout` sets `valid` mask; constants in `maze.h` |
| 2 | Random flag | Chosen in `maze_init` on a valid cell (`flagFloor/X/Y`). `fastflag` overrides. |
| 3 | Player entry (roll 6) | `attempt_enter_maze` logic inside `game_take_turn`; entry consumes turn. |
| 4 | Mid‑path interaction (stairs/poles) | Step loop in `perform_move` checks landing & traversal; `resolve_stair_or_pole`. |
| 5 | Capture | After movement & on entry cell check; resets victim fields; counts stats. |
| 6 | Stair direction changes | `game_round_end` cycles every `config.stairCycleRounds`; supports one‑way & alternation. |
| 7 | Bawana special zone | Coordinates macros in `maze.h`; mapping & effects in `assign_bawana_effect`. |
| 8 | Movement continues until MP 0 or path complete | Per‑step MP updates; MP <=0 teleports to Bawana. |
| 9 | Direction die periodic | `directionRollCounter` & `movesSinceEntry`; every 4th in‑maze turn. |
|10 | Cell effects distribution | Deterministic quota shuffle in `maze_init` with exact counts. |
|11 | Enter Bawana if MP 0 | Check after each step; relocation & effect assignment. |
|12 | Blocked move penalty | If wall/invalid next cell: subtract 2 MP; remaining steps forfeited. |

Additional: Statistics & summary (`game_print_summary`), config (`GameConfig` in `game.h`).

---

## 5. Configuration (GameConfig)
Structure (see `game.h`):
- `stairCycleRounds` (int) – Rounds between stair direction cycles (default e.g. 5).
- `forceOneWay` (bool) – If true, stairs operate in a single permitted direction per cycle.
- `alternateDirections` (bool) – When one‑way, alternates global up vs down orientation each cycle.

To tweak: modify defaults in `game_init` before compilation (future enhancement: expose via CLI).

---

## 6. Statistics Collected
Per player:
- Steps moved
- Captures done / times captured
- Stairs used / poles used
- Bawana visits
- Final MP, floor, position
Global:
- Total stair cycles performed

Printed automatically via `game_print_summary` at program end.

---

## 7. Source Layout
| Path | Purpose |
|------|---------|
| `include/maze.h` | Maze topology, cell effects, stairs/poles, Bawana macros |
| `include/game.h` | Game state, players, config, public API |
| `src/maze.c` | Initialization: layout, effect quota distribution, flag placement |
| `src/game.c` | Turn logic, movement engine, effects, Bawana, stair cycles, summary |
| `src/main.c` | Program entry, CLI seed parsing, loop, fastflag debug |
| `src/random.c` | Pseudo‑random generator (LCG) utilities |
| `src/utils.c` | Misc helpers (if any future additions) |
| `Makefile` | Optional build automation |

---

## 8. Deterministic Effect Distribution
Rather than probabilistic assignment, coordinates are collected, shuffled with LCG seeded by game seed, then quotas for each effect category are assigned exactly (ensuring reproducible percentages). This supports consistent testing and fairness across seeds.

---

## 9. Bawana Effects (Type Codes)
| Code | Name | Effect |
|------|------|--------|
| 0 | Food Poisoning | Skip next N turns (config inside code); relocated randomly in Bawana each application |
| 1 | Disoriented | Random direction each movement turn for duration |
| 2 | Triggered | Double movement speed (effectively extra MP per step) for duration |
| 3 | Happy | Immediate MP boost |
| 4 | Random MP | Random MP grant (range defined in code) |

Effect mapping stored in `Game.bawanaTypes` for deterministic reuse.

---

## 10. Debug / Testing Aids
- `fastflag` argument: Forces flag near Player A to accelerate win path verification.
- Fixed seed: Allows reproducible scenario debugging.
- Large safety round cap prevents infinite loops while allowing complex exploration.

Recommended manual test cases:
1. Seed causing early capture (verify summary counts increment).
2. Force MP depletion (observe Bawana teleport & effect message).
3. Multiple stair cycles (set `stairCycleRounds=1` temporarily and watch enabled directions flip).
4. Blocked move penalty (place walls or attempt out‑of‑bounds to confirm -2 MP application).

---

## 11. Extensibility Ideas
- CLI flags to adjust config (cycles, distribution percentages).
- Save/restore state snapshots for analysis.
- Visualization (ASCII map per floor with player markers & flag).
- Unit test harness comparing deterministic traces across seeds.
- Data‑driven layout using external map files.

---

## 12. Assumptions & Simplifications
- Floor geometries inferred where spec ambiguous (documented inline in `maze.c`).
- Minimal set of stairs/poles; can be expanded without core logic changes.
- One Bawana entrance coordinate; interior enclosed by walls for movement clarity.
- Movement points interpreted as flexible resource consumed per penalty/effect, not strictly “speed units”.

---

## 13. Viva / Oral Exam Preparation
Below are potential questions with concise model answers.

### Game Design & Rules
Q1: How is player entry handled?
A: A player outside the maze must roll a 6 on the movement die; entry consumes the entire turn and places them at their start coordinates facing initial direction.

Q2: When does the direction die roll occur?
A: Every 4th turn that the player is inside the maze (tracked by a counter). Blank faces retain the current direction.

Q3: How are movement points (MP) adjusted per step?
A: Each traversed cell applies its effect: subtract (consumable), add, or multiply (2x/3x). Penalties for blocked moves subtract 2 MP immediately.

Q4: What triggers a Bawana teleport?
A: MP <= 0 after applying a step’s effect or a penalty; player is then moved into the Bawana zone and assigned an effect type.

Q5: How is capture resolved?
A: After a move or entry, if two players occupy the same coordinates and floor, the moving player captures the other; captured player is reset to start, direction reset, stats updated.

Q6: Can a player both enter and move in the same turn?
A: No; rolling a 6 only allows entry that turn—movement starts from the next turn.

Q7: How are stairs direction changes implemented?
A: A cycle counter triggers every `stairCycleRounds`; when `forceOneWay` is true all stairs are set to a single up or down direction, alternating if `alternateDirections` is enabled.

### Data Structures & Implementation
Q8: How is the maze represented?
A: 3D arrays indexed [floor][x][y] for validity, effects, walls; arrays for stairs and poles; flag coordinates stored separately.

Q9: Describe the random generator approach.
A: A lightweight linear congruential generator seeded from user input/time ensures deterministic sequences for repeatable tests.

Q10: How are exact effect distribution percentages enforced?
A: Collect valid coordinates, shuffle, then slice the list into contiguous segments matching quota counts for each effect category.

Q11: How is per‑step logic structured?
A: Movement iterates up to die result; for each intended step, check walls/bounds, apply penalty if blocked; else move, apply cell effect, handle stairs/poles, detect MP depletion.

Q12: How is Bawana effect duration managed?
A: Each effect has a remaining turns counter in the player struct; decremented per turn and cleared when zero.

Q13: What ensures no partial movement on obstacle?
A: A blocked step immediately applies the -2 MP penalty and terminates the remaining steps for that turn (no partial rollback needed).

### Performance & Reliability
Q14: Complexity per turn?
A: O(S) where S is movement die roll (max small constant); initialization O(N) across cells for one-time shuffle.

Q15: Potential sources of nondeterminism?
A: Only the RNG seed; all random decisions route through the seeded LCG.

Q16: How are infinite loops prevented?
A: Safety cap on total rounds plus guaranteed termination upon flag capture.

### Extensibility & Testing
Q17: How would you add a new cell effect type?
A: Extend `CellEffectType`, adjust distribution quotas, implement handling in effect application switch.

Q18: How to unit test movement?
A: Seed RNG, script a sequence of dice rolls (or stub RNG), compare resulting player coordinates and MP deltas against expected trace.

Q19: How to visualize the maze?
A: Render ASCII layers (floors) with characters for players, flag, stairs/poles, and effect initials; update after each turn.

Q20: How to expose config at runtime?
A: Parse additional CLI args (e.g., `--cycles 3 --oneway --alt`) before `game_init` and apply to `Game.config`.

### Edge Cases
Q21: What if a player lands exactly at MP 0 but on the flag?
A: Flag capture is checked before MP depletion triggers Bawana relocation; capture therefore still succeeds.

Q22: Multiple players enter same cell simultaneously?
A: Sequential turn order prevents simultaneous entry; capture only occurs on the active player’s resolution.

Q23: Stair traversal mid‑move onto a cell with another player?
A: Arrival after stair traversal triggers capture check just like normal movement.

Q24: What if stairs disabled directionally mid‑cycle while a player stands on them?
A: State changes only affect future traversal attempts; stationary players are unaffected until they attempt movement.

Q25: Can Bawana effect stack with previous one?
A: New assignment overwrites previous effect fields (single active Bawana status at a time).

---

## 14. Sample Session (Excerpt)
```
=====Maze Runner UCSC (seed 42)=====By Dilki ishara=====
Round 0 Flag at F2 (5,12)
-- Player 0 turn --
... (movement / effects / possible captures)
Winner: Player 0
== Summary ==
Player 0 steps:12 captures:1 stairs:2 poles:0 bawana:0 MP:87
...
```

---

## 15. Maintenance Checklist
- [ ] Update `stairCycleRounds` and rebuild after tuning.
- [ ] Add new stairs/poles in `maze_init` and increment counts.
- [ ] Validate no compiler warnings (`-Wall -Wextra`).
- [ ] Run deterministic seed for regression scenarios.

---

## 16. License / Academic Integrity
For educational purposes. When submitting academically, cite any external inspirations and ensure compliance with institutional policies.

---

## 17. Credits
Core implementation & documentation prepared for the UCSC Maze Runner assignment context.

---

Happy exploring the maze!
