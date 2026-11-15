/* Tandy compact top-down 11x11 map, one byte/pixel in indexed framebuffer.
 * North is -Z (top). Player in white, facing direction in yellow. This
 * draws real voxel-world top blocks, so edits also change the minimap.
 * 121 simple memory lookups, no raycasts, trig or per-pixel floating math.
 */
#include "mine_map.h"
#define MAP_SIDE 11
#define MAP_FRAME 13
static rd_u8 color_of(int block) {
    switch(block){
        case RD_GRASS:return 10;
        case RD_LEAVES:return 2;
        case RD_DIRT:return 6;
        case RD_LOG:return 6;
        case RD_PLANKS:return 14;
        case RD_STONE:return 8;
        case RD_COBBLE:return 7;
        default:return 11;
    }
}
void mine_map_draw(int player_id,rd_u8 *im,int w,int h) {
    const rd_u8 *world=rd_blocks(),*top=rd_light_depth();
    float px,py,pz,yaw,pitch;
    int x0,y0,cx,cz,x,z,mx,mz,dx=0,dz=1,wx,wz,ht,block;
    if(!im||w<32||h<32)return;
    rd_player_pos(player_id,&px,&py,&pz);
    rd_player_angles(player_id,&yaw,&pitch);
    cx=(int)px;cz=(int)pz;
    x0=w-MAP_FRAME;y0=h-MAP_FRAME;
    /* Pitch is intentionally ignored: this is a ground-position map. */
    (void)py;(void)pitch;
    for(z=0;z<MAP_FRAME;z++)for(x=0;x<MAP_FRAME;x++)
        im[(unsigned)(y0+z)*w+(unsigned)(x0+x)]=0;
    for(mz=-5;mz<=5;mz++)for(mx=-5;mx<=5;mx++) {
        wx=cx+mx;wz=cz+mz;block=RD_AIR;
        if(wx>=0&&wx<RD_X&&wz>=0&&wz<RD_Z) {
            ht=top[wx+wz*RD_X];
            if(ht>0)
                block=world[((ht-1)*RD_Z+wz)*RD_X+wx];
        }
        im[(unsigned)(y0+mz+6)*w+(unsigned)(x0+mx+6)]=color_of(block);
    }
    /* Quantize yaw to a 4-way compass with just a few comparisons.
     * The actual yaw is in radians: 0=+Z, +PI/2=+X.
     */
    while(yaw>3.1415926f)yaw-=6.2831853f;
    while(yaw< -3.1415926f)yaw+=6.2831853f;
    if(yaw>0.785398f&&yaw<2.356194f){dx=1;dz=0;}
    else if(yaw< -0.785398f&&yaw> -2.356194f){dx=-1;dz=0;}
    else if(yaw>=2.356194f||yaw<=-2.356194f){dx=0;dz=-1;}
    im[(unsigned)(y0+6)*w+(unsigned)(x0+6)]=15;
    im[(unsigned)(y0+6+dz)*w+(unsigned)(x0+6+dx)]=14;
}
