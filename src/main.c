#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

int main(int argc, char** argv){
    unsigned seed = (unsigned)time(NULL);
    if(argc>1) seed = (unsigned)strtoul(argv[1],NULL,10);
    Game g; game_init(&g,seed);
    if(argc>2 && strcmp(argv[2],"fastflag")==0){
        g.maze.flagFloor=0; g.maze.flagX=5; g.maze.flagY=12; // adjacent to Player A's entry path
        printf("[FastFlag mode active: flag set to F0 (5,12)]\n");
    }
    printf("=====Maze Runner UCSC (seed %u)=====By Dilki ishara=====\n", seed);
    game_print_status(&g);
    int winner=-1;
    int maxRounds = 200000; // large safety cap
    while(winner<0 && g.roundNumber < maxRounds){
        for(int p=0;p<PLAYER_COUNT && winner<0;++p){
            printf("-- Player %d turn --\n", p);
            if(game_take_turn(&g,(PlayerId)p)) winner=p;
            game_print_status(&g);
        }
        if(winner>=0) break;
        g.roundNumber++;
        game_round_end(&g);
    }
        if(winner>=0) printf("Winner: Player %d\n", winner); else printf("No winner within %d rounds (flag uncaptured).\n", g.roundNumber);
        game_print_summary(&g);
    return 0;
}
