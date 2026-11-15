/* DOS minimap shows real geography and turns independently of 3D FPS. */
#include <stdio.h>
#include <string.h>
#include "../platforms/dos16/mine_map.h"
#define W 40
#define H 40
static rd_u8 image[W*H],before[W*H];
static int at(int x,int y){return image[y*W+x];}
int main(void){
 rd_u8 *world;
 int me,x,z,y,changed=0,i;
 rd_init(132211UL,RD_HILLS);
 me=rd_player_add();if(me<0)return 1;
 world=rd_blocks();memset(world,RD_AIR,(unsigned)rd_world_bytes());
 for(z=0;z<RD_Z;z++)for(x=0;x<RD_X;x++)for(y=0;y<12;y++)
     world[(y*RD_Z+z)*RD_X+x]=(rd_u8)(y==11?RD_GRASS:RD_DIRT);
 rd_relight();
 memset(image,11,sizeof image);
 rd_player_set(me,16.5f,12.0f,16.5f,0.0f,0.0f);
 mine_map_draw(me,image,W,H);
 if(at(33,33)!=15||at(33,34)!=14||at(32,32)!=10){
     fprintf(stderr,"Map facing +Z failed: center=%d ahead=%d grass=%d\n",at(33,33),at(33,34),at(32,32));return 2;
 }
 memcpy(before,image,sizeof image);
 rd_player_set(me,16.5f,12.0f,16.5f,1.5707963f,0.0f);
 mine_map_draw(me,image,W,H);
 if(at(34,33)!=14||at(33,34)==14){puts("Map yaw arrow did not turn right");return 3;}
 /* Mutate one real column, and confirm the map changes. */
 memcpy(before,image,sizeof image);
 world[(11*RD_Z+16)*RD_X+18]=RD_STONE;
 rd_relight();
 mine_map_draw(me,image,W,H);
 for(i=0;i<W*H;i++)changed+=before[i]!=image[i];
 if(changed<1){puts("Map ignores real world edit");return 4;}
 memset(image,11,sizeof image);
 mine_map_draw(me,image,20,25);
 for(i=0;i<W*H;i++)if(image[i]!=11){puts("Map damaged /LOW layout");return 5;}
 printf("PASS: 11x11 minimap shows player, turning, geography and edits\n");
 return 0;
}
