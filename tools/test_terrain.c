/* Screen-column renderer smoke, deterministic camera/terrain edits. */
#include <stdio.h>
#include <string.h>
#include "../src/rd_fast.h"
static rd_u8 original[20*25],turned[20*25],empty[20*25];
static int diffs(const rd_u8 *a,const rd_u8 *b){int n=0,i;for(i=0;i<20*25;i++)n+=a[i]!=b[i];return n;}
int main(void){
    rd_u8 *world;int id,x,z,y,distinct=0,i,used[16]={0};
    rd_init(132211UL,RD_HILLS);id=rd_player_add();
    world=rd_blocks();
    rd_player_set(id,16.5f,12.0f,15.5f,0.0f,0.0f);
    rd_terrain_render(id,original,20,25);
    for(i=0;i<500;i++){if(original[i]>15)return 1;used[original[i]]=1;}
    for(i=0;i<16;i++)distinct+=used[i];
    if(distinct<3){fprintf(stderr,"Only %d terrain colors\n",distinct);return 2;}
    rd_player_set(id,16.5f,12.0f,15.5f,1.57f,0.0f);
    rd_terrain_render(id,turned,20,25);
    if(diffs(original,turned)<10){puts("Terrain yaw not reflected");return 3;}
    memset(world,RD_AIR,(unsigned)rd_world_bytes());rd_relight();
    rd_player_set(id,16.5f,12.0f,15.5f,0.0f,0.0f);
    rd_terrain_render(id,empty,20,25);
    if(diffs(original,empty)<30){puts("Terrain edits not reflected");return 4;}
    printf("Terrain renderer OK: %d colors, %d yaw differences, %d world-edit differences\n",distinct,diffs(original,turned),diffs(original,empty));
    return 0;
}
