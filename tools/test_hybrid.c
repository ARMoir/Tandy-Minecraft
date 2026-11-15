/* A real hybrid rendering smoke: source scene, yaw response, edited
 * near doorway, framebuffer guard and palette range. Runs on CI host.
 */
#include <stdio.h>
#include <string.h>
#include "../src/rd_fast.h"
#define W 80
#define H 50
static rd_u8 guarded[W*H+2],before[W*H];
static int render(int player) {
    unsigned i;
    memset(guarded,0x7d,sizeof guarded);
    rd_hybrid_render(player,guarded+1,W,H);
    if(guarded[0]!=0x7d||guarded[W*H+1]!=0x7d)return 1;
    for(i=1;i<=W*H;i++)if(guarded[i]>15)return 2;
    return 0;
}
static int changed(void){
    int i,n=0;
    for(i=0;i<W*H;i++)if(before[i]!=guarded[i+1])n++;
    return n;
}
int main(void){
    rd_u8 *world;
    int me,x,z,y,n,land=0;
    rd_init(132211UL,RD_HILLS);
    me=rd_player_add();
    if(me<0||render(me))return 1;
    memcpy(before,guarded+1,W*H);
    for(n=0;n<W*H;n++)if(before[n]!=11)land++;
    if(land<100){fprintf(stderr,"No landscape in hybrid: %d\n",land);return 2;}
    {float px,py,pz,yaw,pitch;
     rd_player_pos(me,&px,&py,&pz);
     rd_player_angles(me,&yaw,&pitch);
     rd_player_set(me,px,py,pz,yaw+0.45f,pitch);
    }
    if(render(me)||changed()<20){puts("Yaw had no visible effect");return 3;}
    world=rd_blocks();memset(world,RD_AIR,(unsigned)rd_world_bytes());
    for(z=0;z<RD_Z;z++)for(x=0;x<RD_X;x++)for(y=0;y<5;y++)
        world[(y*RD_Z+z)*RD_X+x]=(rd_u8)(y==4?RD_GRASS:RD_DIRT);
    for(x=13;x<=19;x++)for(y=5;y<=9;y++)
        world[(y*RD_Z+16)*RD_X+x]=RD_STONE;
    world[(5*RD_Z+16)*RD_X+16]=RD_AIR;
    world[(6*RD_Z+16)*RD_X+16]=RD_AIR;
    rd_relight();rd_player_set(me,16.5f,5.0f,14.5f,0.0f,0.0f);
    if(render(me))return 4;
    memcpy(before,guarded+1,W*H);
    world[(6*RD_Z+16)*RD_X+16]=RD_STONE;rd_relight();
    if(render(me))return 5;
    n=changed();
    if(n<3){fprintf(stderr,"Near cube edit invisible (%d pixels)\n",n);return 6;}
    /* Verify the original full 3D mode remains callable and safe. */
    rd_faces_render(me,guarded+1,W,H);
    for(n=1;n<=W*H;n++)if(guarded[n]>15)return 7;
    puts("PASS: 80x50 hybrid shows distant landscape, camera yaw, live near-block edit and /FACES fallback");
    return 0;
}
