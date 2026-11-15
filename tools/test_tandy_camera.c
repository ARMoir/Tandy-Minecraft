/* Camera look test: simulate keyboard Up/Down and ensure that the real
 * terrain renderer's horizon moves in the expected direction.
 */
#include <stdio.h>
#include <string.h>
#include "../src/rd_fast.h"
#include "../platforms/dos16/mine_keys.h"
#define W 40
#define H 40
static mine_keyboard keys;
static mine_snapshot snap;
static rd_input in;
static mine_commands cmd;
static rd_u8 image[W*H];
static int horizon(int id){
    int y;
    rd_terrain_render(id,image,W,H);
    /* Projected flat-world silhouette: first non-sky pixel. */
    for(y=0;y<H;y++)if(image[y*W]!=11)return y;
    return H;
}
static void tap(unsigned char code,unsigned long ticks,int id){
    mine_key_feed(&keys,code);
    mine_key_snapshot(&keys,&snap);
    mine_key_controls(&snap,ticks,&in,&cmd);
    rd_player_input(id,&in);
    rd_tick();
    mine_key_feed(&keys,(unsigned char)(code|0x80));
}
int main(void){
    int id,base,up,down;
    float x,y,z,yaw,pitch;
    rd_init(132211UL,RD_HILLS);
    id=rd_player_add();if(id<0)return 1;
    /* The horizon now comes from actual flat grass geometry. */
    {
        int x,z,y;
        rd_u8 *world=rd_blocks();
        memset(world,RD_AIR,(unsigned)rd_world_bytes());
        for(z=0;z<RD_Z;z++)for(x=0;x<RD_X;x++)
            for(y=0;y<12;y++)
                world[(y*RD_Z+z)*RD_X+x]=(rd_u8)(y==11?RD_GRASS:RD_DIRT);
        rd_relight();
    }
    rd_player_set(id,16.5f,12.0f,16.5f,0.0f,0.0f);
    base=horizon(id);
    tap(0x29,8,id);up=horizon(id);
    if(up<=base+3){fprintf(stderr,"UP did not move horizon down: base=%d up=%d\n",base,up);return 2;}
    tap(0x4A,8,id);tap(0x4A,8,id);
    down=horizon(id);
    if(down>=base-3){fprintf(stderr,"DOWN did not move horizon up: base=%d down=%d\n",base,down);return 3;}
    rd_player_angles(id,&yaw,&pitch);
    if(pitch>=0){puts("DOWN failed to change pitch sign");return 4;}
    tap(0x4E,5,id);rd_player_angles(id,&yaw,&pitch);
    if(yaw<0.45f){puts("RIGHT failed to turn camera");return 5;}
    tap(0x2B,5,id);rd_player_angles(id,&yaw,&pitch);
    if(yaw>0.05f||yaw< -0.05f){puts("LEFT failed to turn back");return 6;}
    printf("PASS: horizon base=%d Up=%d Down=%d and Left/Right yaw=%.2f\n",base,up,down,yaw);
    return 0;
}
