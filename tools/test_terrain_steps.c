/* A one-block Minecraft step MUST render a brown dirt face.
 * Earlier 'ground > previous + 1' missed ordinary one-block rises.
 */
#include <stdio.h>
#include <string.h>
#include "../src/rd_fast.h"
static rd_u8 step_scene[40*40],flat_scene[40*40];
static int count_brown(const rd_u8 *img){int i,n=0;for(i=0;i<1600;i++)n+=(img[i]==6);return n;}
static int difference(void){int i,n=0;for(i=0;i<1600;i++)n+=(step_scene[i]!=flat_scene[i]);return n;}
int main(void){
    rd_u8 *world;
    int id,x,z,y,a,b,delta;
    rd_init(132211UL,RD_HILLS);
    id=rd_player_add();if(id<0)return 1;
    world=rd_blocks();memset(world,RD_AIR,(unsigned)rd_world_bytes());
    for(z=0;z<RD_Z;z++)for(x=0;x<RD_X;x++){
        for(y=0;y<12;y++)
            world[(y*RD_Z+z)*RD_X+x]=(rd_u8)(y==11?RD_GRASS:RD_DIRT);
        if(z>=20){
            world[(11*RD_Z+z)*RD_X+x]=RD_DIRT;
            world[(12*RD_Z+z)*RD_X+x]=RD_GRASS;
        }
    }
    rd_relight();
    rd_player_set(id,16.5f,12.0f,16.5f,0.0f,-0.15f);
    rd_terrain_render(id,step_scene,40,40);
    a=count_brown(step_scene);
    for(z=20;z<RD_Z;z++)for(x=0;x<RD_X;x++){
        world[(11*RD_Z+z)*RD_X+x]=RD_GRASS;
        world[(12*RD_Z+z)*RD_X+x]=RD_AIR;
    }
    rd_relight();
    rd_terrain_render(id,flat_scene,40,40);
    b=count_brown(flat_scene);delta=difference();
    if(a<=b+12 || delta<60) {
        fprintf(stderr,"One-block step lacked a distinct dirt face: stepped brown=%d flat brown=%d changed=%d\n",
                a,b,delta);
        return 2;
    }
    printf("PASS: 1-block ledge shows brown side: stepped=%d flat=%d changed=%d\n",
            a,b,delta);
    return 0;
}
