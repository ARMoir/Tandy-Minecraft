/* Tandy hybrid voxel renderer. Preconverted small distant landscape,
 * then nearby real Minecraft cube faces with the baked source textures.
 * No dynamic allocations or per-pixel float calculations in DOS.
 */
#include "rd_fast.h"
#include <string.h>
#define DIST_W 40
#define DIST_H 25
static rd_u8 distant[DIST_W*DIST_H];
static int initialized=0,lastx,lastz,lastyaw,lastpitch;
static unsigned lastrev;
void rd_hybrid_render(int id,rd_u8 *image,int w,int h) {
    float px,py,pz,yaw,pitch;
    int x,z,a,b,xs,ys,sx,yy,xx,srcy;
    unsigned rev;
    if(!image||w<80||w>320||h<50||h>200)return;
    if(w%DIST_W || h%DIST_H)return;
    rd_player_pos(id,&px,&py,&pz);
    rd_player_angles(id,&yaw,&pitch);
    /* Quantized caching: far scenery can lag by a fraction of a block or
     * a few degrees while nearby cube geometry is redrawn every frame. */
    x=(int)(px*8.0f);z=(int)(pz*8.0f);
    a=(int)(yaw*16.0f);b=(int)(pitch*16.0f);
    rev=rd_world_revision();
    if(!initialized||x!=lastx||z!=lastz||a!=lastyaw||
       b!=lastpitch||rev!=lastrev) {
        rd_terrain_render_far(id,distant,DIST_W,DIST_H);
        lastx=x;lastz=z;lastyaw=a;lastpitch=b;lastrev=rev;
        initialized=1;
    }
    /* Duplicate premapped palette indices; no division per output pixel.
     * The near 3D faces are drawn at full chosen internal resolution. */
    xs=w/DIST_W;ys=h/DIST_H;
    for(srcy=0;srcy<DIST_H;srcy++)for(yy=0;yy<ys;yy++) {
        unsigned dst=(unsigned)(srcy*ys+yy)*(unsigned)w;
        unsigned base=(unsigned)srcy*DIST_W;
        for(sx=0;sx<DIST_W;sx++) {
            rd_u8 c=distant[base+(unsigned)sx];
            unsigned off=dst+(unsigned)(sx*xs);
            for(xx=0;xx<xs;xx++)image[off+(unsigned)xx]=c;
        }
    }
    rd_faces_render_near(id,image,w,h);
}
