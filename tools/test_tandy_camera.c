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
    /* Empty world renders deterministic sky=11, horizon=9, ground=2.
     * Look for the first row that isn't blue at the left edge.
     */
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
    /* Clear world to isolate the sky/ground horizon. */
    memset(rd_blocks(),RD_AIR,(unsigned)rd_world_bytes());rd_relight();
    rd_player_set(id,16.5f,12.0f,16.5f,0.0f,0.0f);
    base=horizon(id);
    tap(0x48,8,id);up=horizon(id);
    if(up<=base+3){fprintf(stderr,"UP did not move horizon down: base=%d up=%d\n",base,up);return 2;}
    tap(0x50,8,id);tap(0x50,8,id);
    down=horizon(id);
    if(down>=base-3){fprintf(stderr,"DOWN did not move horizon up: base=%d down=%d\n",base,down);return 3;}
    rd_player_angles(id,&yaw,&pitch);
    if(pitch>=0){puts("DOWN failed to change pitch sign");return 4;}
    tap(0x4D,5,id);rd_player_angles(id,&yaw,&pitch);
    if(yaw<0.45f){puts("RIGHT failed to turn camera");return 5;}
    tap(0x4B,5,id);rd_player_angles(id,&yaw,&pitch);
    if(yaw>0.05f||yaw< -0.05f){puts("LEFT failed to turn back");return 6;}
    printf("PASS: horizon base=%d Up=%d Down=%d and Left/Right yaw=%.2f\n",base,up,down,yaw);
    return 0;
}
