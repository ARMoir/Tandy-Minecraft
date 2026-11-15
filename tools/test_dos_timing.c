#include <stdio.h>
#include <string.h>
#include "../src/rd.h"
static float walk(int id,unsigned step) {
 rd_input in={0};float x,y,z;unsigned t;
 rd_player_set(id,16.5f,5,8.5f,0,0);in.forward=1;
 for(t=0;t<18;t+=step){rd_player_input(id,&in);rd_dos_advance(step);}
 rd_player_pos(id,&x,&y,&z);return z;
}
int main(void) {
 int id,x,y,z;float fast,slow,blocked;
 rd_init(132211UL,RD_HILLS);id=rd_player_add();
 memset(rd_blocks(),0,rd_world_bytes());
 for(z=0;z<RD_Z;z++)for(x=0;x<RD_X;x++)
  rd_blocks()[(4*RD_Z+z)*RD_X+x]=RD_GRASS;
 rd_relight();fast=walk(id,1);slow=walk(id,6);
 if(fast<11.4f||fast>11.6f||slow-fast>0.02f||fast-slow>0.02f)return 1;
 for(y=5;y<9;y++)for(x=0;x<RD_X;x++)
  rd_blocks()[(y*RD_Z+10)*RD_X+x]=RD_STONE;
 rd_relight();blocked=walk(id,18);
 if(blocked>9.71f)return 2;
 printf("PASS: fast=%.3f slow=%.3f, wall stops at %.3f; elapsed-time walking without tunneling\n",fast,slow,blocked);
 return 0;
}
