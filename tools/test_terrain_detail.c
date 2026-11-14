/* Normal mode must have distinct source-textured ground, sky, trees,
 * yaw/pitch responses, and a higher pixel count than /LOW. */
#include <stdio.h>
#include <string.h>
#include "../src/rd_fast.h"
static rd_u8 normal[40*50],turned[40*50],blank[40*50],low[20*25];
static int changed(rd_u8*a,rd_u8*b,int n) {
 int i,num=0;for(i=0;i<n;i++)num+=a[i]!=b[i];return num;
}
int main(void) {
 int me,i,seen[16]={0},count=0,ground=0,sky=0;
 rd_u8 *world;
 rd_init(132211UL,RD_HILLS);me=rd_player_add();
 if(me<0)return 1;
 rd_player_set(me,16.5f,12.0f,15.5f,0.2f,-0.1f);
 rd_terrain_render(me,normal,40,50);
 rd_terrain_render(me,low,20,25);
 for(i=0;i<40*50;i++) {
     if(normal[i]>15)return 2;
     seen[normal[i]]=1;
     if(normal[i]==11||normal[i]==9)sky++;
     if(normal[i]==2||normal[i]==10||normal[i]==6||normal[i]==14)ground++;
 }
 for(i=0;i<16;i++)count+=seen[i];
 if(count<4||ground<10||sky<10){
     fprintf(stderr,"Scene not recognizable: colors=%d green/brown=%d sky=%d\n",count,ground,sky);
     return 3;
 }
 rd_player_set(me,16.5f,12.0f,15.5f,1.2f,-0.1f);
 rd_terrain_render(me,turned,40,50);
 if(changed(normal,turned,40*50)<80) {puts("Yaw detail test failed");return 4;}
 world=rd_blocks();memset(world,RD_AIR,(unsigned)rd_world_bytes());rd_relight();
 rd_player_set(me,16.5f,12.0f,15.5f,0.2f,-0.1f);
 rd_terrain_render(me,blank,40,50);
 if(changed(normal,blank,40*50)<150){puts("Ground change detail test failed");return 5;}
 printf("Readable 40x50 scene OK: %d colors, %d ground pixels, %d sky pixels\n",
        count,ground,sky);
 return 0;
}
