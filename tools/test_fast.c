/* Fast renderer regression, known voxel scenery / camera positions. */
#include <stdio.h>
#include <stdlib.h>
#include "../src/rd_fast.h"
int main(void) {
    static rd_u8 img[40*25],other[40*25];
    int i,unique[16]={0},count=0,different=0;
    rd_u8 *blocks;
    int me;
    rd_init(1,RD_FLAT);
    me=rd_player_add();
    if(me<0)return 1;
    blocks=rd_blocks();
    for(i=0;i<rd_world_bytes();i++)blocks[i]=RD_AIR;
    /* A stone wall at z=19, directly in the player's view. */
    for(i=8;i<15;i++)blocks[(i*RD_Z+19)*RD_X+16]=RD_STONE;
    rd_relight();
    rd_player_set(me,16.5f,9.0f,14.5f,0.0f,0.0f);
    rd_fast_render(me,img,40,25);
    for(i=0;i<40*25;i++){if(img[i]>15)return 2;unique[img[i]]=1;}
    for(i=0;i<16;i++)count+=unique[i];
    if(count<2){fprintf(stderr,"No visible voxels: %d colors\n",count);return 3;}
    rd_player_set(me,16.5f,9.0f,14.5f,3.14159265f,0.0f);
    rd_fast_render(me,other,40,25);
    for(i=0;i<40*25;i++)if(img[i]!=other[i])different++;
    if(different<20){fprintf(stderr,"Camera turn did not alter rendered view (%d pixels)\n",different);return 4;}
    printf("Fast 16-color renderer: %d colors, %d changed on turning\n",count,different);
    return 0;
}
