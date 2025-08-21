#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

static unsigned lcg(unsigned *state){ *state = (*state * 1664525u + 1013904223u); return *state; }

static const char* dir_name(Direction d){ return (const char*[4]){"N","E","S","W"}[d]; }

static void players_init(Game* g){
    // Starting area positions (outside maze), we store negative y to differentiate
    // Use given starting area coords (A:6,12 first cell 5,12 dir N etc)
    g->players[PLAYER_A]=(Player){0,6,12,false,DIR_N,100,0,false,false,0,-1,0,0,0,0,0,0,0};
    g->players[PLAYER_B]=(Player){0,9,8,false,DIR_W,100,0,false,false,0,-1,0,0,0,0,0,0,0};
    g->players[PLAYER_C]=(Player){0,9,16,false,DIR_E,100,0,false,false,0,-1,0,0,0,0,0,0,0};
    // stats already zeroed via initializer
    for(int i=0;i<PLAYER_COUNT;++i) g->directionRollCounter[i]=0;
    for(int i=0;i<PLAYER_COUNT;++i) g->movesSinceEntry[i]=0;
}

void game_init(Game* g, unsigned seed){
    g->rng = seed?seed:(unsigned)time(NULL);
    maze_init(&g->maze,g->rng^0xABCDEFu);
    players_init(g);
    g->roundNumber=0;
    g->stairDirectionChangeCountdown=5; // every 5 rounds
    g->stairDirectionMode=0;
    g->config = (GameConfig){ .stairCycleRounds=5, .forceOneWay=true, .alternateDirections=true };
    g->stairDirectionChangeCountdown = g->config.stairCycleRounds;
    g->totalStairCycles=0;
    // Initialize deterministic Bawana cell type distribution
    int temp[16]; int idx=0;
    for(int t=0;t<4;++t) for(int k=0;k<3;++k) temp[idx++]=t; // 12 cells of special types (0..3)
    for(int k=0;k<4;++k) temp[idx++]=4; // 4 random MP cells
    for(int i=15;i>0;--i){ unsigned r = lcg(&g->rng)%(unsigned)(i+1); int swap=temp[i]; temp[i]=temp[r]; temp[r]=swap; }
    idx=0;
    for(int x=BAWANA_X_MIN;x<=BAWANA_X_MAX;++x){
        for(int y=BAWANA_Y_MIN;y<=BAWANA_Y_MAX;++y){
            g->bawanaTypes[x-BAWANA_X_MIN][y-BAWANA_Y_MIN]=temp[idx++];
        }
    }
}

// Stair direction toggling logic will be integrated later (Rule 6 placeholder removed to avoid warnings)

static int roll_movement_die(Game* g){ return (int)(lcg(&g->rng)%6)+1; }
static int roll_direction_die(Game* g){ return (int)(lcg(&g->rng)%6)+1; }

static Direction direction_from_face(int face, Direction current){
    switch(face){
        case 2: return DIR_N; case 3: return DIR_E; case 4: return DIR_S; case 5: return DIR_W; default: return current; }
}

static bool attempt_enter_maze(Game* g, Player* p, PlayerId pid, int moveDie){
    (void)g; // currently unused
    if(p->inMaze) return false;
    if(moveDie==6){
        if(pid==PLAYER_A){ p->x=5; p->y=12; }
        else if(pid==PLAYER_B){ p->x=9; p->y=7; }
        else { p->x=9; p->y=17; }
        p->inMaze=true;
        printf("Player %d enters maze at (%d,%d)\n", pid, p->x,p->y);
        return true;
    }
    return false;
}

static void resolve_stair_or_pole(Game* g, Player* p, bool midMove){
    // Check stairs
    for(int i=0;i<g->maze.stairCount;++i){
        Stair* s=&g->maze.stairs[i];
        if(p->floor==s->startFloor && p->x==s->startX && p->y==s->startY){
            if(s->enabledUp) { p->floor=s->endFloor; p->x=s->endX; p->y=s->endY; p->stairsUsed++; return; }
        }
        if(p->floor==s->endFloor && p->x==s->endX && p->y==s->endY){
            if(s->enabledDown){ p->floor=s->startFloor; p->x=s->startX; p->y=s->startY; p->stairsUsed++; return; }
        }
    }
    // Poles (always down if from higher floor)
    for(int i=0;i<g->maze.poleCount;++i){
        Pole* pl=&g->maze.poles[i];
        if(p->x==pl->x && p->y==pl->y){
            if(p->floor==pl->fromFloor){ p->floor=pl->toFloor; p->polesUsed++; return; }
            if(p->floor==pl->fromFloor-1){ p->floor=pl->toFloor; p->polesUsed++; return; }
        }
    }
    (void)midMove;
}

static bool movement_blocked_wall(const Game* g,int f,int x,int y,Direction d){
    unsigned w = g->maze.walls[f][x][y];
    switch(d){
        case DIR_N: return (w & CELL_WALL_N)!=0; case DIR_S: return (w & CELL_WALL_S)!=0; case DIR_E: return (w & CELL_WALL_E)!=0; case DIR_W: return (w & CELL_WALL_W)!=0;
    }
    return false;
}

static bool can_step(const Game* g,int f,int x,int y,Direction d){
    if(movement_blocked_wall(g,f,x,y,d)) return false;
    int nx=x, ny=y;
    switch(d){ case DIR_N: nx--; break; case DIR_S: nx++; break; case DIR_E: ny++; break; case DIR_W: ny--; break; }
    if(!maze_cell_exists(&g->maze,f,nx,ny)) return false;
    if(movement_blocked_wall(g,f,nx,ny,(Direction)((d+2)%4))) return false; // opposite wall
    return true;
}

static void send_to_bawana(Player* p){
    p->floor=0; p->x=BAWANA_ENTRANCE_X; p->y=BAWANA_ENTRANCE_Y; p->dir=DIR_N; p->randomDisoriented=false; p->triggered=false; p->disorientedTurnsRemaining=0; p->bawanaEffectType=-1; p->bawanaEffectTurns=0; }

static void assign_bawana_effect(Game* g, Player* p){
    int typeIndexX = p->x-BAWANA_X_MIN;
    int typeIndexY = p->y-BAWANA_Y_MIN;
    if(typeIndexX>=0 && typeIndexX<= (BAWANA_X_MAX-BAWANA_X_MIN) && typeIndexY>=0 && typeIndexY<=(BAWANA_Y_MAX-BAWANA_Y_MIN)){
        p->bawanaEffectType = g->bawanaTypes[typeIndexX][typeIndexY];
    } else {
        // fallback
        p->bawanaEffectType = 4;
    }
    int type = p->bawanaEffectType;
    switch(type){
        case 0: // Food Poisoning
            p->skipTurns=3; p->bawanaEffectTurns=3; printf("Bawana effect: Food Poisoning (skip 3).\n"); break;
        case 1: // Disoriented
            p->movementPoints +=50; p->randomDisoriented=true; p->disorientedTurnsRemaining=4; printf("Bawana effect: Disoriented (random 4 turns +50 MP).\n"); break;
        case 2: // Triggered
            p->movementPoints +=50; p->triggered=true; p->bawanaEffectTurns=0; printf("Bawana effect: Triggered (+50 MP double speed).\n"); break;
        case 3: // Happy
            p->movementPoints +=200; printf("Bawana effect: Happy (+200 MP).\n"); break;
        case 4: // Random MP 10-100
            p->movementPoints += (int)(lcg(&g->rng)%91)+10; printf("Bawana effect: Bonus MP random.\n"); break;
    }
}

static void perform_move(Game* g, Player* p, int steps, bool directionJustChanged){
    (void)directionJustChanged;
    for(int i=0;i<steps;++i){
        if(p->movementPoints<=0){ break; }
        if(!can_step(g,p->floor,p->x,p->y,p->dir)){
            // blocked
            p->movementPoints -=2; // penalty per rule 12 when not moving at all for this step
            break; // cannot proceed further
        }
        // step
        switch(p->dir){ case DIR_N: p->x--; break; case DIR_S: p->x++; break; case DIR_E: p->y++; break; case DIR_W: p->y--; break; }
    p->stepsMoved++;
        resolve_stair_or_pole(g,p,true);
        // Prevent entering Bawana interior except via entrance cell (one-way). If moved into interior directly without passing entrance, revert.
        if(is_bawana_cell(p->floor,p->x,p->y)){
            // Determine previous cell coords
            int bx=p->x, by=p->y; // current interior
            int prevx=bx, prevy=by;
            switch(p->dir){ case DIR_N: prevx++; break; case DIR_S: prevx--; break; case DIR_E: prevy--; break; case DIR_W: prevy++; break; }
            if(!is_bawana_entrance(p->floor,prevx,prevy)){
                // revert move and stop
                p->x=prevx; p->y=prevy; p->movementPoints -=2; break;
            }
        }
        p->movementPoints = maze_apply_effect(&g->maze,p->floor,p->x,p->y,p->movementPoints,NULL);
        if(p->movementPoints<=0){
            printf("Movement points depleted -> Bawana\n");
            send_to_bawana(p);
            assign_bawana_effect(g,p);
            return;
        }
        if(is_bawana_cell(p->floor,p->x,p->y)){
            // Landing inside Bawana triggers effect
            p->bawanaVisits++;
            assign_bawana_effect(g,p);
            // After certain effects (poison) player stays; others reposition to entrance with north direction
            if(p->bawanaEffectType!=0){ p->x=BAWANA_ENTRANCE_X; p->y=BAWANA_ENTRANCE_Y; p->dir=DIR_N; }
            return;
        }
    }
}

bool game_take_turn(Game* g, PlayerId pid){
    Player* p=&g->players[pid];
    // Expire disorientation
    if(p->disorientedTurnsRemaining>0){
        p->disorientedTurnsRemaining--;
        if(p->disorientedTurnsRemaining==0){ p->randomDisoriented=false; }
    }
    if(p->skipTurns>0){
        p->skipTurns--;
        printf("Player %d skips (remaining %d)\n",pid,p->skipTurns);
        if(p->skipTurns==0 && p->bawanaEffectType==0){
            // After poisoning period ends, relocate randomly and apply new effect immediately
            int rx = (int)(lcg(&g->rng)%(unsigned)(BAWANA_X_MAX-BAWANA_X_MIN+1)) + BAWANA_X_MIN;
            int ry = (int)(lcg(&g->rng)%(unsigned)(BAWANA_Y_MAX-BAWANA_Y_MIN+1)) + BAWANA_Y_MIN;
            p->x=rx; p->y=ry; p->floor=0; assign_bawana_effect(g,p); if(p->bawanaEffectType!=0){ p->x=BAWANA_ENTRANCE_X; p->y=BAWANA_ENTRANCE_Y; p->dir=DIR_N; }
        }
        return false;
    }
    int moveDie = roll_movement_die(g);
    bool rollDirection = false;
    if(p->inMaze){
        if(g->movesSinceEntry[pid]>0 && (g->movesSinceEntry[pid] % 4)==0){ rollDirection=true; }
    }
    int dirFace = 0;
    if(rollDirection){ dirFace = roll_direction_die(g); p->dir = direction_from_face(dirFace,p->dir); }
    g->directionRollCounter[pid]++;

    bool justEntered = attempt_enter_maze(g,p,pid,moveDie);
    if(!p->inMaze) return false; // still waiting to enter

    // If just entered, treat this turn as only the entry (no further movement per assignment wording)
    if(justEntered){
        g->movesSinceEntry[pid]=1; // first in-maze turn completed
        // Immediate flag capture if entry cell is flag
        if(p->floor==g->maze.flagFloor && p->x==g->maze.flagX && p->y==g->maze.flagY){
            printf("Player %d captured the flag on entry! Game Over.\n", pid);
            return true;
        }
        return false;
    }
    if(g->movesSinceEntry[pid]==0) g->movesSinceEntry[pid]=1; else g->movesSinceEntry[pid]++;

    // Pre-move flag check (in case effects / teleports placed player on flag previously)
    if(p->floor==g->maze.flagFloor && p->x==g->maze.flagX && p->y==g->maze.flagY){
        printf("Player %d captured the flag! Game Over.\n", pid);
        return true;
    }

    int steps = moveDie;
    if(p->triggered) steps*=2;
    if(p->randomDisoriented){ p->dir = (Direction)(lcg(&g->rng)%4); }
    perform_move(g,p,steps,rollDirection);

    // Capture logic
    for(int i=0;i<PLAYER_COUNT;++i){
        if((PlayerId)i == pid) continue;
        Player* op=&g->players[i];
        if(op->inMaze && p->inMaze && op->floor==p->floor && op->x==p->x && op->y==p->y){
            // send back to starting area
            printf("Player %d captured player %d!\n", pid, i);
            g->players[pid].capturesDone++;
            g->players[i].timesCaptured++;
            if(i==PLAYER_A){ op->x=6; op->y=12; op->inMaze=false; op->dir=DIR_N; }
            else if(i==PLAYER_B){ op->x=9; op->y=8; op->inMaze=false; op->dir=DIR_W; }
            else { op->x=9; op->y=16; op->inMaze=false; op->dir=DIR_E; }
        }
    }

    // Post-move flag capture
    if(p->floor==g->maze.flagFloor && p->x==g->maze.flagX && p->y==g->maze.flagY){
        printf("Player %d captured the flag! Game Over.\n", pid);
        return true;
    }
    return false;
}

void game_print_status(const Game* g){
    printf("Round %d Flag at F%d (%d,%d)\n", g->roundNumber,g->maze.flagFloor,g->maze.flagX,g->maze.flagY);
    for(int i=0;i<PLAYER_COUNT;++i){
        const Player* p=&g->players[i];
        printf("P%d F%d (%d,%d) %s MP:%d %s\n", i,p->floor,p->x,p->y, p->inMaze?dir_name(p->dir):"START", p->movementPoints, p->inMaze?"":"(waiting)");
    }
}

void game_print_help(){
    puts("Commands: (not interactive yet)");
}

void game_round_end(Game* g){
    g->stairDirectionChangeCountdown--;
    if(g->stairDirectionChangeCountdown<=0){
        g->stairDirectionChangeCountdown = g->config.stairCycleRounds;
        g->totalStairCycles++;
        // Determine global mode
        bool upMode = true;
        if(g->config.alternateDirections){ upMode = (g->totalStairCycles % 2)==1; } else { upMode = (lcg(&g->rng)&1u)==0; }
        for(int i=0;i<g->maze.stairCount;++i){
            Stair* s=&g->maze.stairs[i];
            if(g->config.forceOneWay){
                s->bidirectional=false;
                if(upMode){ s->enabledUp=true; s->enabledDown=false; }
                else { s->enabledUp=false; s->enabledDown=true; }
            } else {
                // Random toggling preserving potential bidirectionality
                if((lcg(&g->rng)&1u)==0){ s->enabledUp=true; s->enabledDown=true; s->bidirectional=true; }
                else { s->bidirectional=false; if((lcg(&g->rng)&1u)==0){ s->enabledUp=true; s->enabledDown=false; } else { s->enabledUp=false; s->enabledDown=true; } }
            }
        }
        printf("[Stair cycle %u -> %s]\n", g->totalStairCycles, upMode?"UP":"DOWN");
    }
}

void game_print_summary(const Game* g){
    puts("=== Game Summary ===");
    printf("Rounds: %d  Stair Cycles: %u\n", g->roundNumber, g->totalStairCycles);
    for(int i=0;i<PLAYER_COUNT;++i){
        const Player* p=&g->players[i];
        printf("P%d Steps:%u Captures:%u Captured:%u Bawana:%u MP:%d\n", i,p->stepsMoved,p->capturesDone,p->timesCaptured,p->bawanaVisits,p->movementPoints);
    }
}
