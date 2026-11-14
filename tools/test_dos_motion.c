/* DOS-specific rdgeneric low-FPS movement and camera regression. */
#include <stdio.h>
#include "../src/rd.h"
int main(void){
 rd_input in={0};
 int id,i;
 float x,y,z,yaw,pitch;
 rd_init(132211UL,RD_HILLS);
 id=rd_player_add();if(id<0)return 1;
 rd_player_set(id,16.5f,12.0f,15.5f,0.0f,0.0f);
 in.forward=1.0f;in.yaw=0.5f;in.select=RD_STONE;
 rd_player_input(id,&in);
 for(i=0;i<4;i++)rd_tick();
 rd_player_pos(id,&x,&y,&z);
 rd_player_angles(id,&yaw,&pitch);
 if(yaw<0.45f||yaw>0.55f){fprintf(stderr,"DOS yaw failed: %f\n",yaw);return 2;}
 if(z<15.51f&&x<16.51f){fprintf(stderr,"DOS movement failed: x=%f z=%f\n",x,z);return 3;}
 if(rd_selected(id)!=RD_STONE){puts("DOS select failed");return 4;}
 printf("DOS input core: yaw=%f, moved to x=%f z=%f\n",yaw,x,z);
 return 0;
}
