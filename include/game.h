#ifndef GAME_H
#define GAME_H
#include <stdbool.h>
#include "maze.h"

typedef enum { DIR_N=0, DIR_E=1, DIR_S=2, DIR_W=3 } Direction;

typedef enum { PLAYER_A=0, PLAYER_B=1, PLAYER_C=2, PLAYER_COUNT=3 } PlayerId;

typedef struct {
    int floor;
    int x; // width axis 0..9
    int y; // length axis 0..24
    bool inMaze; // entered maze from starting area
    Direction dir;
    int movementPoints; // Rule 12 starts at 100
    int skipTurns; // for Bawana incapacitation
    bool randomDisoriented; // Rule 7 disoriented
    bool triggered; // double speed effect
    int disorientedTurnsRemaining;
    int bawanaEffectType; // -1 none, 0 poisoning,1 disoriented,2 triggered,3 happy,4 randomMP
    int bawanaEffectTurns; // remaining for timed effects
    // Stats
    unsigned stepsMoved;
    unsigned capturesDone;
    unsigned timesCaptured;
    unsigned stairsUsed;
    unsigned polesUsed;
    unsigned bawanaVisits;
} Player;

typedef struct {
    int stairCycleRounds;          // e.g. 5
    bool forceOneWay;              // if true all stairs become one-way each cycle
    bool alternateDirections;      // if true alternate up/down per cycle globally
} GameConfig;

typedef struct {
    Maze maze;
    Player players[PLAYER_COUNT];
    unsigned rng;
    int roundNumber; // each full set of 3 turns
    int directionRollCounter[PLAYER_COUNT];
    int movesSinceEntry[PLAYER_COUNT];
    int stairDirectionChangeCountdown; // for Rule 6 (every 5 rounds)
    int stairDirectionMode; // toggles
    int bawanaTypes[BAWANA_X_MAX-BAWANA_X_MIN+1][BAWANA_Y_MAX-BAWANA_Y_MIN+1]; // mapping interior cells to type codes
    GameConfig config;
    unsigned totalStairCycles;
} Game;

void game_init(Game* g, unsigned seed);
void game_print_status(const Game* g);
bool game_take_turn(Game* g, PlayerId p);
void game_print_help();
void game_round_end(Game* g); // call after all players acted each round
void game_print_summary(const Game* g);

#endif
