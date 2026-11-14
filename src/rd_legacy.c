/* Fixed-point voxel ray marcher for slow 8088/V20 processors.
 * Sampling is deliberately simple: <=48 quarter-block samples per ray.
 * No floating point within a pixel or a ray. The original rdgeneric
 * world generation, physics, collision and block interaction remain intact.
 */
#include "rd_fast.h"
#define FIX 256
#define MAX_STEPS 48
static const short sinus[256] = {
       0,    6,   13,   19,   25,   31,   38,   44,   50,   56,   62,   68,   74,   80,   86,   92,
      98,  104,  109,  115,  121,  126,  132,  137,  142,  147,  152,  157,  162,  167,  172,  177,
     181,  185,  190,  194,  198,  202,  206,  209,  213,  216,  220,  223,  226,  229,  231,  234,
     237,  239,  241,  243,  245,  247,  248,  250,  251,  252,  253,  254,  255,  255,  256,  256,
     256,  256,  256,  255,  255,  254,  253,  252,  251,  250,  248,  247,  245,  243,  241,  239,
     237,  234,  231,  229,  226,  223,  220,  216,  213,  209,  206,  202,  198,  194,  190,  185,
     181,  177,  172,  167,  162,  157,  152,  147,  142,  137,  132,  126,  121,  115,  109,  104,
      98,   92,   86,   80,   74,   68,   62,   56,   50,   44,   38,   31,   25,   19,   13,    6,
       0,   -6,  -13,  -19,  -25,  -31,  -38,  -44,  -50,  -56,  -62,  -68,  -74,  -80,  -86,  -92,
     -98, -104, -109, -115, -121, -126, -132, -137, -142, -147, -152, -157, -162, -167, -172, -177,
    -181, -185, -190, -194, -198, -202, -206, -209, -213, -216, -220, -223, -226, -229, -231, -234,
    -237, -239, -241, -243, -245, -247, -248, -250, -251, -252, -253, -254, -255, -255, -256, -256,
    -256, -256, -256, -255, -255, -254, -253, -252, -251, -250, -248, -247, -245, -243, -241, -239,
    -237, -234, -231, -229, -226, -223, -220, -216, -213, -209, -206, -202, -198, -194, -190, -185,
    -181, -177, -172, -167, -162, -157, -152, -147, -142, -137, -132, -126, -121, -115, -109, -104,
     -98,  -92,  -86,  -80,  -74,  -68,  -62,  -56,  -50,  -44,  -38,  -31,  -25,  -19,  -13,   -6
};
static const rd_u8 tops[RD_NUM_BLOCKS]  ={11,7,10,6,7,14,6,10};
static const rd_u8 sides[RD_NUM_BLOCKS] ={11,8, 6,6,8, 6,6, 2};
static const rd_u8 darks[RD_NUM_BLOCKS] ={11,8, 2,4,8, 6,4, 2};

static int angle256(float radians) {
    long n=(long)(radians*40.743665f);
    return (int)(n & 255L);
}
static rd_u8 ray(const rd_u8 *world,int ox,int oy,int oz,
                 int dx,int dy,int dz,int screen_x,int screen_y) {
    int x=ox,y=oy,z=oz;
    int lastx=ox>>8,lasty=oy>>8,lastz=oz>>8;
    int count;
    int stepx=dx/4,stepy=dy/4,stepz=dz/4;
    for(count=0;count<MAX_STEPS;count++) {
        int bx,by,bz,index,b;
        x+=stepx; y+=stepy; z+=stepz;
        if(x<0 || y<0 || z<0 || x>=RD_X*FIX ||
           y>=RD_Y*FIX || z>=RD_Z*FIX) return 11;
        bx=x>>8; by=y>>8; bz=z>>8;
        index=(by*RD_Z+bz)*RD_X+bx;
        b=world[index];
        if(b>RD_AIR && b<RD_NUM_BLOCKS) {
            rd_u8 color;
            if(b==RD_LEAVES && ((bx*3+bz*5+by*7+count)&7)==0)
                continue;
            color=by!=lasty?tops[b]:(bz!=lastz?sides[b]:darks[b]);
            /* Stipple in some source texture variation without float/RGB. */
            if(b==RD_STONE && ((bx*7+by*3+bz*5+screen_x)&7)==0)
                color=8;
            if(b==RD_LEAVES && ((bx+by+bz+screen_y)&3)==0)
                color=10;
            if(count>35 && ((screen_x+screen_y+count)&7)<(count-35)/3)
                return 11;
            return color;
        }
        lastx=bx; lasty=by; lastz=bz;
    }
    return 11;
}
void rd_legacy_render(int id,rd_u8 *pixels,int w,int h) {
    float px,py,pz,yaw,pitch;
    const rd_u8 *world=rd_blocks();
    int ys,cs,ps,pc,fx,fy,fz,rx,rz,ux,uy,uz;
    int ox,oy,oz,i,j;
    int u_values[80],v_values[50];
    if(!pixels || w<2 || w>80 || h<2 || h>50) return;
    rd_player_pos(id,&px,&py,&pz);
    rd_player_angles(id,&yaw,&pitch);
    ox=(int)(px*FIX);oy=(int)(py*FIX)+415;oz=(int)(pz*FIX);
    ys=sinus[angle256(yaw)];
    cs=sinus[(angle256(yaw)+64)&255];
    ps=sinus[angle256(pitch)];
    pc=sinus[(angle256(pitch)+64)&255];
    fx=(int)((long)ys*pc/FIX);fy=ps;fz=(int)((long)cs*pc/FIX);
    rx=cs;rz=-ys;
    ux=-(int)((long)ys*ps/FIX);uy=pc;uz=-(int)((long)cs*ps/FIX);
    for(i=0;i<w;i++)
        u_values[i]=(int)((long)(2*i+1-w)*205L/w);
    for(j=0;j<h;j++)
        v_values[j]=(int)((long)(h-2*j-1)*159L/h);
    for(j=0;j<h;j++) {
        int v=v_values[j];
        for(i=0;i<w;i++) {
            int u=u_values[i];
            int dx=fx+(int)(((long)rx*u+(long)ux*v)/FIX);
            int dy=fy+(int)((long)uy*v/FIX);
            int dz=fz+(int)(((long)rz*u+(long)uz*v)/FIX);
            pixels[(unsigned)j*(unsigned)w+(unsigned)i]=
                ray(world,ox,oy,oz,dx,dy,dz,i,j);
        }
    }
    /* Crosshair and selected block at the lower-left. */
    pixels[(unsigned)(h/2)*(unsigned)w+(unsigned)(w/2)]=15;
    if(h>3) pixels[(unsigned)(h/2-1)*(unsigned)w+(unsigned)(w/2)]=15;
    if(h>3) pixels[(unsigned)(h/2+1)*(unsigned)w+(unsigned)(w/2)]=15;
    if(w>3) pixels[(unsigned)(h/2)*(unsigned)w+(unsigned)(w/2-1)]=15;
    if(w>3) pixels[(unsigned)(h/2)*(unsigned)w+(unsigned)(w/2+1)]=15;
    if(h>4 && w>6) {
        int block=rd_selected(id),s=0;
        rd_u8 ink=block>=0 && block<RD_NUM_BLOCKS?tops[block]:15;
        for(j=h-4;j<h-1;j++)
            for(i=2;i<6;i++)
                pixels[(unsigned)j*(unsigned)w+(unsigned)i]=
                    (i==2 || j==h-4 || i==5 || j==h-2)?15:ink;
        (void)s;
    }
}
