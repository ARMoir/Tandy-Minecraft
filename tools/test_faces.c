/* Geometry and safety checks for the actual DOS face renderer. */
#include <stdio.h>
#include <string.h>
#include "../src/rd_fast.h"
static rd_u8 guarded[64002],before[64000];
static void put(int x,int y,int z,int mat) {
 rd_blocks()[(y*RD_Z+z)*RD_X+x]=(rd_u8)mat;
}
static int render(int id,int w,int h) {
 unsigned i,total=(unsigned)w*h;
 memset(guarded,0xa5,sizeof guarded);
 rd_faces_render(id,guarded+1,w,h);
 if(guarded[0]!=0xa5||guarded[total+1]!=0xa5)return 1;
 for(i=1;i<=total;i++)if(guarded[i]>15)return 2;
 return 0;
}
int main(void) {
 int id,x,y,z,i,changed=0;
 rd_init(132211UL,RD_HILLS);id=rd_player_add();
 if(render(id,320,200))return 1;
 memcpy(before,guarded+1,64000);
 {float px,py,pz,yaw,pitch;
  rd_player_pos(id,&px,&py,&pz);rd_player_angles(id,&yaw,&pitch);
  rd_player_set(id,px,py,pz,yaw+0.3f,pitch+0.2f);}
 if(render(id,320,200))return 2;
 for(i=0;i<64000;i++)changed+=before[i]!=guarded[i+1];
 if(changed<500)return 3;
 memset(rd_blocks(),0,rd_world_bytes());
 for(z=0;z<RD_Z;z++)for(x=0;x<RD_X;x++)put(x,4,z,RD_GRASS);
 for(x=12;x<=20;x++)for(y=5;y<=9;y++)put(x,y,16,RD_STONE);
 /* Open doorway: no heightfield can represent this opening correctly. */
 put(16,5,16,RD_AIR);put(16,6,16,RD_AIR);
 rd_relight();rd_player_set(id,16.5f,5,14.5f,0,0);
 if(render(id,320,200))return 4;
 if(guarded[1+100*320+168]!=11)return 5;
 put(16,6,16,RD_STONE);rd_relight();
 if(render(id,320,200))return 6;
 if(guarded[1+100*320+168]==11)return 7;
 put(16,6,16,RD_AIR);rd_relight();
 if(render(id,320,200)||guarded[1+100*320+168]!=11)return 8;
 /* Sweep through near-plane crossings and pitch extremes at every quality. */
 for(i=0;i<32;i++) {
   rd_player_set(id,15.8f+i*0.04f,5,15.9f,i*0.2f,(i-16)*0.09f);
   if(render(id,80,100)||render(id,160,100)||
      render(id,160,200)||render(id,320,200))return 9;
 }
 puts("PASS: 16 colors, 320x200 bounds, camera movement, visible doorway edits, near clipping at all qualities");
 return 0;
}
