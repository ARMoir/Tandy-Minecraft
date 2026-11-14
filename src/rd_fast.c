/* 8086/V20 Minecraft: real 3-D voxel DDA, fixed 8.8 point traversal.
 * Techniques from WolfensteinCGA: boundary stepping, precomputed reciprocal
 * and perspective tables, indexed textures. No floating point in pixel loop.
 */
#include "rd_fast.h"
#include "rd_tandy_atlas.h"
#define INF 65535U
#define MAX_DISTANCE 3072U
#define MAX_VISITS 48
#define MAX_DIRECTION 768
static const short sintable[256]={
    0, 6, 13, 19, 25, 31, 38, 44, 50, 56, 62, 68, 74, 80, 86, 92,
    98, 104, 109, 115, 121, 126, 132, 137, 142, 147, 152, 157, 162, 167, 172, 177,
    181, 185, 190, 194, 198, 202, 206, 209, 213, 216, 220, 223, 226, 229, 231, 234,
    237, 239, 241, 243, 245, 247, 248, 250, 251, 252, 253, 254, 255, 255, 256, 256,
    256, 256, 256, 255, 255, 254, 253, 252, 251, 250, 248, 247, 245, 243, 241, 239,
    237, 234, 231, 229, 226, 223, 220, 216, 213, 209, 206, 202, 198, 194, 190, 185,
    181, 177, 172, 167, 162, 157, 152, 147, 142, 137, 132, 126, 121, 115, 109, 104,
    98, 92, 86, 80, 74, 68, 62, 56, 50, 44, 38, 31, 25, 19, 13, 6,
    0, -6, -13, -19, -25, -31, -38, -44, -50, -56, -62, -68, -74, -80, -86, -92,
    -98, -104, -109, -115, -121, -126, -132, -137, -142, -147, -152, -157, -162, -167, -172, -177,
    -181, -185, -190, -194, -198, -202, -206, -209, -213, -216, -220, -223, -226, -229, -231, -234,
    -237, -239, -241, -243, -245, -247, -248, -250, -251, -252, -253, -254, -255, -255, -256, -256,
    -256, -256, -256, -255, -255, -254, -253, -252, -251, -250, -248, -247, -245, -243, -241, -239,
    -237, -234, -231, -229, -226, -223, -220, -216, -213, -209, -206, -202, -198, -194, -190, -185,
    -181, -177, -172, -167, -162, -157, -152, -147, -142, -137, -132, -126, -121, -115, -109, -104,
    -98, -92, -86, -80, -74, -68, -62, -56, -50, -44, -38, -31, -25, -19, -13, -6
};
static unsigned short inverse[769];
static int inverse_ready=0;
static void init_inverse(void) {
    unsigned i;
    if(inverse_ready)return;
    inverse[0]=INF;
    for(i=1;i<=MAX_DIRECTION;i++)
        inverse[i]=(unsigned short)(65536UL/(unsigned long)i);
    inverse_ready=1;
}
static int turn_index(float rad) {return (int)(((long)(rad*40.743665f))&255L);}
static unsigned short inverse_for(int d) {
    unsigned a=(unsigned)(d<0?-d:d);
    return inverse[a>MAX_DIRECTION?MAX_DIRECTION:a];
}
static unsigned short first_edge(int pos,int dir,unsigned short delta) {
    unsigned frac=(unsigned)pos&255U;
    unsigned remain=dir>0?256U-frac:frac;
    return (unsigned short)(((unsigned long)remain*delta)>>8);
}
static unsigned short sum_sat(unsigned short a,unsigned short b) {
    return a>(unsigned short)(INF-b)?(unsigned short)INF:(unsigned short)(a+b);
}
static rd_u8 cast(const rd_u8 *world,const rd_u8 *light,
                  int ox,int oy,int oz,int dx,int dy,int dz) {
    int x=ox>>8,y=oy>>8,z=oz>>8;
    int sx=dx<0?-1:1,sy=dy<0?-1:1,sz=dz<0?-1:1;
    unsigned short ddx=inverse_for(dx),ddy=inverse_for(dy),ddz=inverse_for(dz);
    unsigned short tx=dx?first_edge(ox,dx,ddx):INF;
    unsigned short ty=dy?first_edge(oy,dy,ddy):INF;
    unsigned short tz=dz?first_edge(oz,dz,ddz):INF;
    unsigned short t;
    int n,px,py,pz,axis,block,index,wx,wy,wz,u,v,face;
    rd_u8 color;
    if(x<0||y<0||z<0||x>=RD_X||y>=RD_Y||z>=RD_Z)return 11;
    for(n=0;n<MAX_VISITS;n++) {
        px=x;py=y;pz=z;
        if(tx<=ty&&tx<=tz){
            axis=0;t=tx;
            if(t>MAX_DISTANCE)return 11;
            x+=sx;tx=sum_sat(tx,ddx);
        } else if(ty<=tz){
            axis=1;t=ty;
            if(t>MAX_DISTANCE)return 11;
            y+=sy;ty=sum_sat(ty,ddy);
        } else {
            axis=2;t=tz;
            if(t>MAX_DISTANCE)return 11;
            z+=sz;tz=sum_sat(tz,ddz);
        }
        if(x<0||y<0||z<0||x>=RD_X||y>=RD_Y||z>=RD_Z)return 11;
        block=world[(y*RD_Z+z)*RD_X+x];
        if(block<=RD_AIR||block>=RD_NUM_BLOCKS)continue;
        /* Two hit coordinates select the original 16x16 texture texel. */
        wx=ox+(int)(((long)dx*t)>>8);
        wy=oy+(int)(((long)dy*t)>>8);
        wz=oz+(int)(((long)dz*t)>>8);
        if(axis==0){u=wz;v=-wy;face=0;}
        else if(axis==2){u=wx;v=-wy;face=2;}
        else {u=wx;v=wz;face=sy<0?1:3;}
        if(px>=0&&px<RD_X&&pz>=0&&pz<RD_Z&&py<(int)light[px+pz*RD_X])face+=4;
        index=((((unsigned)v&255U)>>4)*16)+(((unsigned)u&255U)>>4);
        color=rd_tandy_atlas[block][face][index];
        if(color!=16)return color; /* transparent leaves */
    }
    return 11;
}
void rd_fast_render(int id,rd_u8 *pixels,int w,int h) {
    float px,py,pz,yaw,pitch;
    const rd_u8 *world=rd_blocks(),*light=rd_light_depth();
    int a,b,ys,cs,ps,pc,fx,fy,fz,rx,rz,ux,uy,uz;
    int ox,oy,oz,i,j,slope;
    int colx[80],colz[80],rowx[50],rowy[50],rowz[50];
    unsigned out=0;
    if(!pixels||w<2||w>80||h<2||h>50)return;
    init_inverse();
    rd_player_pos(id,&px,&py,&pz);
    rd_player_angles(id,&yaw,&pitch);
    ox=(int)(px*256.0f);oy=(int)(py*256.0f)+415;oz=(int)(pz*256.0f);
    a=turn_index(yaw);b=turn_index(pitch);
    ys=sintable[a];cs=sintable[(a+64)&255];
    ps=sintable[b];pc=sintable[(b+64)&255];
    fx=(int)(((long)ys*pc)>>8);fy=ps;fz=(int)(((long)cs*pc)>>8);
    rx=cs;rz=-ys;
    ux=-(int)(((long)ys*ps)>>8);uy=pc;uz=-(int)(((long)cs*ps)>>8);
    /* Scale view rays by column and row ONCE, not per screen pixel. */
    for(i=0;i<w;i++) {
        slope=(int)((long)(2*i+1-w)*205L/w);
        colx[i]=(int)(((long)rx*slope)>>8);
        colz[i]=(int)(((long)rz*slope)>>8);
    }
    for(j=0;j<h;j++) {
        slope=(int)((long)(h-2*j-1)*159L/h);
        rowx[j]=(int)(((long)ux*slope)>>8);
        rowy[j]=(int)(((long)uy*slope)>>8);
        rowz[j]=(int)(((long)uz*slope)>>8);
    }
    for(j=0;j<h;j++)for(i=0;i<w;i++)
        pixels[out++]=cast(world,light,ox,oy,oz,fx+colx[i]+rowx[j],
                            fy+rowy[j],fz+colz[i]+rowz[j]);
    /* Crosshair and selected block retained. */
    pixels[(unsigned)(h/2)*w+(unsigned)(w/2)]=15;
    if(h>3){
        pixels[(unsigned)(h/2-1)*w+(unsigned)(w/2)]=15;
        pixels[(unsigned)(h/2+1)*w+(unsigned)(w/2)]=15;
    }
    if(w>3){
        pixels[(unsigned)(h/2)*w+(unsigned)(w/2-1)]=15;
        pixels[(unsigned)(h/2)*w+(unsigned)(w/2+1)]=15;
    }
    if(h>4&&w>6){
        int selected=rd_selected(id);
        rd_u8 ink=selected>0&&selected<RD_NUM_BLOCKS?
             rd_tandy_atlas[selected][1][136]:15;
        if(ink==16)ink=10;
        for(j=h-4;j<h-1;j++)for(i=2;i<6;i++)
            pixels[(unsigned)j*w+(unsigned)i]=
                (i==2||i==5||j==h-4||j==h-2)?0:ink;
    }
}
