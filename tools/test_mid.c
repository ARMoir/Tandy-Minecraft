/* Middle-detail geometry regression for 7MHz V20 normal AUTO mode. */
#include <stdio.h>
#include <string.h>
#include "../src/rd_fast.h"
static rd_u8 first[40*40],turned[40*40],empty[40*40];
static int changed(rd_u8*a,rd_u8*b){int k,count=0;for(k=0;k<1600;k++)count+=(a[k]!=b[k]);return count;}
int main(void){
 int me,i,seen[16]={0},n=0;
 rd_u8 *world;
 rd_init(132211UL,RD_HILLS);
 me=rd_player_add();if(me<0)return 1;
 rd_player_set(me,16.5f,12.0f,15.5f,0.2f,-0.1f);
 rd_terrain_render(me,first,40,40);
 for(i=0;i<1600;i++){if(first[i]>15)return 2;seen[first[i]]=1;}
 for(i=0;i<16;i++)n+=seen[i];
 if(n<4){fprintf(stderr,"Middle mode only %d colors\n",n);return 3;}
 rd_player_set(me,16.5f,12.0f,15.5f,1.2f,-0.1f);
 rd_terrain_render(me,turned,40,40);
 if(changed(first,turned)<70){puts("Middle-mode yaw failed");return 4;}
 world=rd_blocks();memset(world,RD_AIR,(unsigned)rd_world_bytes());
 rd_relight();
 rd_player_set(me,16.5f,12.0f,15.5f,0.2f,-0.1f);
 rd_terrain_render(me,empty,40,40);
 if(changed(first,empty)<100){puts("Middle-mode terrain edit failed");return 5;}
 printf("40x40 responsive middle mode: %d colors, %d yaw changes, %d terrain changes\n",n,changed(first,turned),changed(first,empty));
 return 0;
}
