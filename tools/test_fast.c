/* Geometry, texture color, yaw/pitch and the older fallback. */
#include <stdio.h>
#include <string.h>
#include "../src/rd_fast.h"
static rd_u8 image[4000],turned[4000],other[4000];
static int colors(rd_u8 *p){int i,n=0,seen[16]={0};for(i=0;i<4000;i++){if(p[i]>15)return -1;seen[p[i]]=1;}for(i=0;i<16;i++)n+=seen[i];return n;}
static int changes(rd_u8 *a,rd_u8 *b){int i,n=0;for(i=0;i<4000;i++)if(a[i]!=b[i])n++;return n;}
int main(void) {
 int id,x,y,n;rd_u8 *world;
 rd_init(1,RD_FLAT);
 id=rd_player_add();if(id<0)return 1;
 world=rd_blocks();memset(world,RD_AIR,(unsigned)rd_world_bytes());
 for(y=7;y<=13;y++)for(x=13;x<=19;x++)world[(y*RD_Z+19)*RD_X+x]=((x+y)&1)?RD_STONE:RD_GRASS;
 rd_relight();rd_player_set(id,16.5f,9.0f,14.5f,0.0f,0.0f);
 rd_fast_render(id,image,80,50);
 if(colors(image)<3){fprintf(stderr,"Texture colors: %d\n",colors(image));return 2;}
 rd_player_set(id,16.5f,9.0f,14.5f,3.14159265f,0.0f);
 rd_fast_render(id,turned,80,50);
 n=changes(image,turned);if(n<100){puts("Yaw test failed");return 3;}
 rd_player_set(id,16.5f,9.0f,14.5f,0.0f,0.5f);
 rd_fast_render(id,other,80,50);
 if(changes(image,other)<50){puts("Pitch test failed");return 4;}
 memset(world,RD_AIR,(unsigned)rd_world_bytes());rd_relight();
 rd_player_set(id,16.5f,9.0f,14.5f,0.0f,0.0f);
 rd_fast_render(id,other,80,50);
 if(changes(image,other)<100){puts("World edit test failed");return 5;}
 rd_legacy_render(id,other,80,50);
 if(colors(other)<1){puts("Fallback test failed");return 6;}
 printf("Textured DDA pass, %d changed yaw pixels; pitch/edits/fallback pass\n",n);
 return 0;
}
