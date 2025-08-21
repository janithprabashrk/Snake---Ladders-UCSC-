#ifndef MAZE_H
#define MAZE_H
#include <stdint.h>
#include <stdbool.h>

#define FLOORS 3
#define WIDTH 10
#define LENGTH 25

// Cell feature flags
#define CELL_WALL_N  (1u<<0)
#define CELL_WALL_E  (1u<<1)
#define CELL_WALL_S  (1u<<2)
#define CELL_WALL_W  (1u<<3)

// Bonus / consumable types
typedef enum {
    CELL_EFFECT_NONE=0,
    CELL_EFFECT_CONSUME, // value subtract 1-4
    CELL_EFFECT_ADD,     // add 1-5
    CELL_EFFECT_MULTIPLY,// multiply by 2 or 3
} CellEffectType;

typedef struct {
    CellEffectType type;
    int value; // meaning depends on type
} CellEffect;

// Stair definition (possibly dynamic direction)
typedef struct {
    int startFloor, startX, startY;
    int endFloor, endX, endY;
    bool bidirectional; // for base rules true, for Rule 6 can toggle
    bool enabledUp;     // dynamic
    bool enabledDown;   // dynamic
} Stair;

// Pole definition
typedef struct {
    int fromFloor; // higher floor
    int toFloor;   // lower floor
    int x, y;
} Pole;

#define MAX_STAIRS 64
#define MAX_POLES 32

typedef struct {
    CellEffect effects[FLOORS][WIDTH][LENGTH];
    bool valid[FLOORS][WIDTH][LENGTH]; // cells that exist per floor design
    unsigned walls[FLOORS][WIDTH][LENGTH]; // bitmask of CELL_WALL_*
    int flagFloor, flagX, flagY;

    Stair stairs[MAX_STAIRS];
    int stairCount;
    Pole poles[MAX_POLES];
    int poleCount;
} Maze;

void maze_init(Maze* m, unsigned seed);
bool maze_in_bounds(int f,int x,int y);
bool maze_cell_exists(const Maze* m,int f,int x,int y);
int  maze_apply_effect(const Maze* m,int f,int x,int y,int movementPoints,int* consumed);
int  maze_random_effect_value(CellEffectType t, unsigned* rngState);
void maze_add_wall_line(Maze* m,int f,int x1,int y1,int x2,int y2);

// Bawana area (assumptions documented in README)
#define BAWANA_X_MIN 6
#define BAWANA_X_MAX 9
#define BAWANA_Y_MIN 21  // interior starts one row below north wall at y=20
#define BAWANA_Y_MAX 24
#define BAWANA_ENTRANCE_X 9
#define BAWANA_ENTRANCE_Y 19 // entrance just outside north wall per spec (9,19)
static inline bool is_bawana_cell(int f,int x,int y){
    return f==0 && x>=BAWANA_X_MIN && x<=BAWANA_X_MAX && y>=BAWANA_Y_MIN && y<=BAWANA_Y_MAX;
}
static inline bool is_bawana_entrance(int f,int x,int y){ return f==0 && x==BAWANA_ENTRANCE_X && y==BAWANA_ENTRANCE_Y; }

#endif
