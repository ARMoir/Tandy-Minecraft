/* Deterministic actual F/E modification test, not a simple keypress test. */
#include <stdio.h>
#include <string.h>
#include "../src/rd.h"
static int filled(const rd_u8 *w){
    int i,n=0;for(i=0;i<rd_world_bytes();i++)if(w[i]!=RD_AIR)n++;return n;
}
int main(void){
    rd_u8 *w;
    rd_input in={0};
    int id,x,z,y,before,after,status;
    rd_init(132211UL,RD_HILLS);
    id=rd_player_add();if(id<0)return 1;
    w=rd_blocks();memset(w,RD_AIR,(unsigned)rd_world_bytes());
    for(z=0;z<RD_Z;z++)for(x=0;x<RD_X;x++)for(y=0;y<12;y++)
        w[(y*RD_Z+z)*RD_X+x]=(rd_u8)(y==11?RD_GRASS:RD_DIRT);
    rd_relight();
    rd_player_set(id,16.5f,12.0f,16.5f,0.0f,-0.65f);
    before=filled(w);
    in.dig=1;
    rd_player_input(id,&in);rd_tick();
    status=rd_take_action(id);after=filled(w);
    if(status!=RD_ACTION_DUG||after!=before-1) {
        fprintf(stderr,"F did not remove an actual block: result=%d before=%d after=%d\n",status,before,after);
        return 2;
    }
    if(rd_take_action(id)!=RD_ACTION_NONE)return 3;
    in.dig=0;in.place=1;
    rd_player_input(id,&in);rd_tick();
    status=rd_take_action(id);
    if(status!=RD_ACTION_PLACED||filled(w)!=before) {
        fprintf(stderr,"E did not place actual block: result=%d count=%d expected=%d\n",
                status,filled(w),before);
        return 4;
    }
    in.place=0;in.dig=1;
    rd_player_set(id,16.5f,12.0f,16.5f,0.0f,1.3f);
    rd_player_input(id,&in);rd_tick();
    if(rd_take_action(id)!=RD_ACTION_MISS) {
        puts("F looking at empty sky should report MISS");return 5;
    }
    printf("PASS: F actually removed one block, E restored it; sky dig reports MISS\n");
    return 0;
}
