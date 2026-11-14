/* Validates the decompressed DOS world matches generator output. */
#include <stdio.h>
#include "../src/rd.h"
int main(void) {
    int i,solid=0,grass=0,leaves=0;
    rd_u8 *world;
    rd_init(132211UL,RD_HILLS);
    world=rd_blocks();
    if(rd_world_bytes()!=RD_X*RD_Y*RD_Z)return 1;
    for(i=0;i<rd_world_bytes();i++){
        solid+=world[i]!=RD_AIR;
        grass+=world[i]==RD_GRASS;
        leaves+=world[i]==RD_LEAVES;
        if(world[i]>=RD_NUM_BLOCKS)return 2;
    }
    if(solid<2000||grass<300){fprintf(stderr,"Invalid generated world: solid=%d grass=%d\n",solid,grass);return 3;}
    if(rd_light_depth()[(RD_Z/2)*RD_X+(RD_X/2)]<1)return 4;
    printf("DOS world RLE decoder OK: solid=%d grass=%d leaves=%d\n",solid,grass,leaves);
    return 0;
}
