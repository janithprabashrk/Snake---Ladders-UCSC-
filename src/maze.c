#include "maze.h"
#include <stdio.h>
#include <stdlib.h>

static unsigned lcg(unsigned *state){
    *state = (*state * 1664525u + 1013904223u);
    return *state;
}

bool maze_in_bounds(int f,int x,int y){
    return f>=0 && f<FLOORS && x>=0 && x<WIDTH && y>=0 && y<LENGTH;
}

bool maze_cell_exists(const Maze* m,int f,int x,int y){
    if(!maze_in_bounds(f,x,y)) return false;
    return m->valid[f][x][y];
}

int maze_random_effect_value(CellEffectType t, unsigned* rngState){
    unsigned r = lcg(rngState);
    switch(t){
        case CELL_EFFECT_CONSUME: return (int)(r%4)+1; // 1-4
        case CELL_EFFECT_ADD: return (int)(r%5)+1; // 1-5
        case CELL_EFFECT_MULTIPLY: return (r & 1)?2:3; // 2 or 3
        default: return 0;
    }
}

// Return new movement points after applying effect; consumed holds cost of consumables along path if needed
int maze_apply_effect(const Maze* m,int f,int x,int y,int movementPoints,int* consumed){
    (void)consumed;
    const CellEffect *ce = &m->effects[f][x][y];
    if(ce->type==CELL_EFFECT_CONSUME){
        return movementPoints - ce->value;
    } else if(ce->type==CELL_EFFECT_ADD){
        return movementPoints + ce->value;
    } else if(ce->type==CELL_EFFECT_MULTIPLY){
        return movementPoints * ce->value;
    }
    return movementPoints;
}

static void mark_floor_layout(Maze* m){
    // Default all invalid then mark existing according to description
    for(int f=0; f<FLOORS; ++f){
        for(int x=0;x<WIDTH;++x)
            for(int y=0;y<LENGTH;++y)
                m->valid[f][x][y]=false;
    }
    // Floor 0: width 10 length 25 all
    for(int x=0;x<WIDTH;++x)
        for(int y=0;y<LENGTH;++y)
            m->valid[0][x][y]=true;
    // Floor1: two rectangles 10x8 connected by bridge 4x9 above starting area (y positions?)
    // Ambiguity: lacking exact coordinates; we assume first rectangle y:0-7, second rectangle y:16-23, bridge width x:3-6 y:7-15
    for(int x=0;x<WIDTH;++x){
        for(int y=0;y<8;++y) m->valid[1][x][y]=true;
        for(int y=16;y<24;++y) m->valid[1][x][y]=true;
    }
    for(int x=3;x<7;++x)
        for(int y=8;y<17;++y)
            m->valid[1][x][y]=true;
    // Floor2: rectangle 10x9 above standing area (assume y:8-16 x:0-9)
    for(int x=0;x<WIDTH;++x)
        for(int y=8;y<17;++y)
            m->valid[2][x][y]=true;
}

void maze_init(Maze* m, unsigned seed){
    m->stairCount=0; m->poleCount=0; m->flagFloor=0; m->flagX=0; m->flagY=0;
    unsigned rng = seed;
    mark_floor_layout(m);

    // Clear walls
    for(int f=0;f<FLOORS;++f)
        for(int x=0;x<WIDTH;++x)
            for(int y=0;y<LENGTH;++y)
                m->walls[f][x][y]=0u;

    // Exact quota distribution per Rule 10: 25% none, 35% consumables (1-4), 25% add (1-2), 10% add (3-5), 5% multipliers (2 or 3)
    int total=0; for(int f=0;f<FLOORS;++f) for(int x=0;x<WIDTH;++x) for(int y=0;y<LENGTH;++y) if(m->valid[f][x][y]) ++total;
    int quotaNone = (total*25)/100;
    int quotaConsume = (total*35)/100;
    int quotaAddSmall = (total*25)/100;
    int quotaAddLarge = (total*10)/100;
    int quotaMult = total - (quotaNone+quotaConsume+quotaAddSmall+quotaAddLarge); // remainder (multipliers quota)
    // Fill list of coordinates
    int cap = total; int *cx = malloc(sizeof(int)*cap); int *cy = malloc(sizeof(int)*cap); int *cf = malloc(sizeof(int)*cap); int idx=0;
    for(int f=0;f<FLOORS;++f) for(int x=0;x<WIDTH;++x) for(int y=0;y<LENGTH;++y) if(m->valid[f][x][y]){ cf[idx]=f; cx[idx]=x; cy[idx]=y; idx++; }
    // Shuffle
    for(int i=cap-1;i>0;--i){ unsigned r = lcg(&rng)%(unsigned)(i+1); int tf=cf[i]; cf[i]=cf[r]; cf[r]=tf; int tx=cx[i]; cx[i]=cx[r]; cx[r]=tx; int ty=cy[i]; cy[i]=cy[r]; cy[r]=ty; }
    idx=0;
    for(int k=0;k<quotaNone && idx<cap;++k){ m->effects[cf[idx]][cx[idx]][cy[idx]]=(CellEffect){CELL_EFFECT_NONE,0}; idx++; }
    for(int k=0;k<quotaConsume && idx<cap;++k){ m->effects[cf[idx]][cx[idx]][cy[idx]]=(CellEffect){CELL_EFFECT_CONSUME,(int)(lcg(&rng)%4)+1}; idx++; }
    for(int k=0;k<quotaAddSmall && idx<cap;++k){ m->effects[cf[idx]][cx[idx]][cy[idx]]=(CellEffect){CELL_EFFECT_ADD,(int)(lcg(&rng)%2)+1}; idx++; }
    for(int k=0;k<quotaAddLarge && idx<cap;++k){ m->effects[cf[idx]][cx[idx]][cy[idx]]=(CellEffect){CELL_EFFECT_ADD,(int)(lcg(&rng)%3)+3}; idx++; }
    for(int k=0;idx<cap && k<quotaMult;++k,++idx){ m->effects[cf[idx]][cx[idx]][cy[idx]]=(CellEffect){CELL_EFFECT_MULTIPLY,(lcg(&rng)&1)?2:3}; }
    free(cx); free(cy); free(cf);

    // Stairs network: 0<->1 and 1<->2
    if(m->stairCount < MAX_STAIRS){ // floor 0 to 1
        m->stairs[m->stairCount++] = (Stair){0,4,5,1,4,5,true,true,true};
    }
    if(m->stairCount < MAX_STAIRS){ // floor 1 to 2
        m->stairs[m->stairCount++] = (Stair){1,4,10,2,4,10,true,true,true};
    }
    if(m->poleCount < MAX_POLES){
        m->poles[m->poleCount++] = (Pole){2,0,5,24};
    }

    // Random flag on existing cell (ensure reachable floor: any floor since connectivity provided)
    while(1){
        int f = (int)(lcg(&rng)%FLOORS);
        int x = (int)(lcg(&rng)%WIDTH);
        int y = (int)(lcg(&rng)%LENGTH);
        if(m->valid[f][x][y]){ m->flagFloor=f; m->flagX=x; m->flagY=y; break; }
    }

    // Bawana walls (north wall from x=6..9 at y=20; west wall from y=20..24 at x=6)
    maze_add_wall_line(m,0,6,20,9,20); // horizontal
    maze_add_wall_line(m,0,6,20,6,24); // vertical
}

void maze_add_wall_line(Maze* m,int f,int x1,int y1,int x2,int y2){
    if(x1==x2){
        if(y2<y1){int t=y1; y1=y2; y2=t;}
        for(int y=y1;y<=y2;++y){
            if(maze_cell_exists(m,f,x1,y)){
                if(y>y1 && maze_cell_exists(m,f,x1,y-1)){ m->walls[f][x1][y] |= CELL_WALL_N; m->walls[f][x1][y-1] |= CELL_WALL_S; }
            }
        }
    } else if(y1==y2){
        if(x2<x1){int t=x1; x1=x2; x2=t;}
        for(int x=x1;x<=x2;++x){
            if(maze_cell_exists(m,f,x,y1)){
                if(x> x1 && maze_cell_exists(m,f,x-1,y1)){ m->walls[f][x][y1] |= CELL_WALL_W; m->walls[f][x-1][y1] |= CELL_WALL_E; }
            }
        }
    }
}
