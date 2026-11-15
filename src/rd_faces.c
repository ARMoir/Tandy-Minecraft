/* Visible voxel faces, clipped and scan-converted with integer arithmetic.
 * Painter traversal uses Manhattan distance from the eye cell: every cell
 * along a ray is visited back-to-front, including overhangs and dug holes.
 * No per-pixel ray walks, float math, allocation or depth buffer.
 */
#include "rd_fast.h"
#include <string.h>
#include "rd_face_art.h"
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
#define MAX_H 200
#define RADIUS 32
typedef struct { int x,y,z,u,v; } vertex;
typedef struct { int x,y,u,v; } screen_vertex;
static int lx[MAX_H],rx[MAX_H];
static long lu[MAX_H],lv[MAX_H],ru[MAX_H],rv[MAX_H];
static int camx,camy,camz,sy,cy,sp,cp,fw,fh,fx,fy;
static rd_u8 *frame;
static const rd_u8 *blocks;
static rd_u8 exposed[RD_X*RD_Y*RD_Z];
static unsigned cached_revision;
static int cache_ready;
#define SURFACE_LIMIT 4096
static unsigned short surface[SURFACE_LIMIT],next_surface[SURFACE_LIMIT];
static unsigned short heads[RADIUS+1];
static unsigned surface_count;
static int surface_overflow;
/* Shared grid corners are transformed only once per frame, regardless of
 * how many adjacent block faces use them. Products stay 32-bit until shifted. */
#define GRID_CORNERS ((RD_X+1)*(RD_Z+1))
static unsigned short corner_stamp[GRID_CORNERS],frame_stamp;
static short corner_x[GRID_CORNERS];
static long corner_y[GRID_CORNERS],corner_z[GRID_CORNERS];
static long vertical_y[RD_Y+1],vertical_z[RD_Y+1];
static int voxel(int x,int y,int z) {
    if(x<0||x>=RD_X||y<0||y>=RD_Y||z<0||z>=RD_Z)return RD_AIR;
    return blocks[(y*RD_Z+z)*RD_X+x];
}
static void cache_faces(void) {
    int x,y,z;unsigned index=0,revision=rd_world_revision();
    if(cache_ready&&revision==cached_revision)return;
    surface_count=0;surface_overflow=0;
    for(y=0;y<RD_Y;y++)for(z=0;z<RD_Z;z++)for(x=0;x<RD_X;x++,index++) {
        int bits=0;
        if(blocks[index]!=RD_AIR) {
            if(!voxel(x-1,y,z))bits|=1;
            if(!voxel(x+1,y,z))bits|=2;
            if(!voxel(x,y,z-1))bits|=4;
            if(!voxel(x,y,z+1))bits|=8;
            if(!voxel(x,y+1,z))bits|=16;
            if(!voxel(x,y-1,z))bits|=32;
        }
        exposed[index]=(rd_u8)bits;
        if(bits) {
            if(surface_count<SURFACE_LIMIT)surface[surface_count++]=(unsigned short)index;
            else surface_overflow=1;
        }
    }
    cached_revision=revision;cache_ready=1;
}
static vertex transform(int x,int y,int z,int u,int v) {
    vertex a;
    unsigned corner=(unsigned)z*(RD_X+1)+(unsigned)x;
    if(corner_stamp[corner]!=frame_stamp) {
        int dx=x*256-camx,dz=z*256-camz;
        int forward=(int)(((long)dx*sy+(long)dz*cy)>>8);
        corner_x[corner]=(short)(((long)dx*cy-(long)dz*sy)>>8);
        corner_y[corner]=(long)forward*sp;
        corner_z[corner]=(long)forward*cp;
        corner_stamp[corner]=frame_stamp;
    }
    a.x=corner_x[corner];
    a.y=(int)((vertical_y[y]-corner_y[corner])>>8);
    a.z=(int)((corner_z[corner]+vertical_z[y])>>8);
    a.u=u;a.v=v;return a;
}
/* Clip the face before division. A face crossing the eye must never turn
 * inside out or disappear just because one corner is behind the camera. */
static void face(vertex *a,int material,int shade) {
    vertex b[6];screen_vertex p[6];
    int i,j,n=0,lo=fh,hi=-1,y,minx=32767,maxx=-32767,flat;
    const rd_u8 *texture=rd_face_art[material][shade];
    for(i=0;i<4;i++) {
        vertex s=a[i],e=a[(i+1)&3];
        if((s.z>=64)!=(e.z>=64)) {
            vertex t;long num=64-s.z,den=e.z-s.z;
            t.x=s.x+(int)((long)(e.x-s.x)*num/den);
            t.y=s.y+(int)((long)(e.y-s.y)*num/den);
            t.z=64;
            t.u=s.u+(int)((long)(e.u-s.u)*num/den);
            t.v=s.v+(int)((long)(e.v-s.v)*num/den);
            b[n++]=t;
        }
        if(e.z>=64)b[n++]=e;
    }
    if(n<3)return;
    for(i=0;i<n;i++) {
        p[i].x=fw/2+(int)((long)b[i].x*fx/b[i].z);
        p[i].y=fh/2-(int)((long)b[i].y*fy/b[i].z);
        if(p[i].x<minx)minx=p[i].x;
        if(p[i].x>maxx)maxx=p[i].x;
        p[i].u=b[i].u;p[i].v=b[i].v;
        if(p[i].y<lo)lo=p[i].y;
        if(p[i].y>hi)hi=p[i].y;
    }
    if(maxx<0||minx>=fw)return;
    flat=(fw<=160&&((maxx-minx)<=4||(hi-lo)<=3));
    if(lo<0)lo=0;if(hi>=fh)hi=fh-1;
    if(lo>hi)return;
    for(y=lo;y<=hi;y++){lx[y]=32767;rx[y]=-32767;}
    for(i=0;i<n;i++) {
        screen_vertex s=p[i],e=p[(i+1)%n],t;
        long x,u,v,dx,du,dv;int start,end,dy;
        if(s.y==e.y)continue;
        if(s.y>e.y){t=s;s=e;e=t;}
        start=s.y;if(start<lo)start=lo;
        end=e.y-1;if(end>hi)end=hi;
        if(start>end)continue;
        dy=e.y-s.y;
        dx=(long)(e.x-s.x)*256/dy;
        du=flat?0:(long)(e.u-s.u)*256/dy;
        dv=flat?0:(long)(e.v-s.v)*256/dy;
        x=(long)s.x*256+dx*(start-s.y);
        u=(long)s.u*256+du*(start-s.y);
        v=(long)s.v*256+dv*(start-s.y);
        for(y=start;y<=end;y++,x+=dx,u+=du,v+=dv){
            int xx=(int)(x>>8);
            if(xx<lx[y]){lx[y]=xx;lu[y]=u;lv[y]=v;}
            if(xx>rx[y]){rx[y]=xx;ru[y]=u;rv[y]=v;}
        }
    }
    for(y=lo;y<=hi;y++) {
        int left=lx[y],right=rx[y],span;
        long u,v,du,dv;unsigned offset;
        if(left>right||right<0||left>=fw)continue;
        if(flat) {
            if(left<0)left=0;if(right>=fw)right=fw-1;
            memset(frame+(unsigned)y*(unsigned)fw+(unsigned)left,texture[136],right-left+1);
            continue;
        }
        span=right-left;
        u=lu[y];v=lv[y];
        du=span?(ru[y]-u)/span:0;dv=span?(rv[y]-v)/span:0;
        if(left<0){u-=du*left;v-=dv*left;left=0;}
        if(right>=fw)right=fw-1;
        offset=(unsigned)y*(unsigned)fw+(unsigned)left;
        for(j=left;j<=right;j++,u+=du,v+=dv) {
            int tu=(int)(u>>16),tv=(int)(v>>16);
            if(tu<0)tu=0;if(tu>15)tu=15;
            if(tv<0)tv=0;if(tv>15)tv=15;
            frame[offset++]=texture[tv*16+tu];
        }
    }
}
static void quad(int x,int y,int z,int axis,int side,int mat) {
    vertex a[4];int shade;
    if(axis==1) {
        a[0]=transform(x,y+side,z,0,0);
        a[1]=transform(x+1,y+side,z,4095,0);
        a[2]=transform(x+1,y+side,z+1,4095,4095);
        a[3]=transform(x,y+side,z+1,0,4095);
        shade=side?0:2;
    }else if(axis==0){
        a[0]=transform(x+side,y+1,z,0,0);
        a[1]=transform(x+side,y+1,z+1,4095,0);
        a[2]=transform(x+side,y,z+1,4095,4095);
        a[3]=transform(x+side,y,z,0,4095);
        shade=2;
    }else{
        a[0]=transform(x,y+1,z+side,0,0);
        a[1]=transform(x+1,y+1,z+side,4095,0);
        a[2]=transform(x+1,y,z+side,4095,4095);
        a[3]=transform(x,y,z+side,0,4095);
        shade=1;
    }
    face(a,mat,shade);
}
static void draw_cell(int bx,int by,int bz,int ex,int ey,int ez) {
    int mat,mask,dx=bx-ex,dz=bz-ez;
                vertex mid;
                if(by<0||by>=RD_Y)return;
                mask=exposed[(by*RD_Z+bz)*RD_X+bx];
                if(!mask)return;
                mat=voxel(bx,by,bz);if(mat==RD_AIR||mat>=RD_NUM_BLOCKS)return;
                /* Buried cubes are the majority: reject them before any
                 * transforms, projection divisions, or texture access. */
                if(!((dx>0&&(mask&1))||
                     (dx<0&&(mask&2))||
                     (dz>0&&(mask&4))||
                     (dz<0&&(mask&8))||
                     (by<ey&&(mask&16))||
                     (by>ey&&(mask&32))))return;
                mid=transform(bx,by,bz,0,0);
                if(mid.z< -448)return;
                if(mid.x>mid.z+768||mid.x< -mid.z-768)return;
                if(dx>0&&(mask&1))quad(bx,by,bz,0,0,mat);
                if(dx<0&&(mask&2))quad(bx,by,bz,0,1,mat);
                if(dz>0&&(mask&4))quad(bx,by,bz,2,0,mat);
                if(dz<0&&(mask&8))quad(bx,by,bz,2,1,mat);
                if(by<ey&&(mask&16))quad(bx,by,bz,1,1,mat);
                if(by>ey&&(mask&32))quad(bx,by,bz,1,0,mat);
}
void rd_faces_render(int id,rd_u8 *image,int width,int height) {
    float x,y,z,yaw,pitch;int ex,ey,ez,phase,ring,dx,dy,dz,sign;
    unsigned k,total;
    if(!image||width<8||width>320||height<8||height>MAX_H)return;
    frame=image;fw=width;fh=height;blocks=rd_blocks();
    cache_faces();
    fx=width*5/7;fy=height*20/21; /* 4:3 CRT aspect, ~70 degree horizontal FOV */
    rd_player_pos(id,&x,&y,&z);rd_player_angles(id,&yaw,&pitch);
    camx=(int)(x*256);camy=(int)((y+1.62f)*256);camz=(int)(z*256);
    ex=camx>>8;ey=camy>>8;ez=camz>>8;
    phase=(int)((long)(yaw*40.743665f)&255L);
    sy=sintable[phase];cy=sintable[(phase+64)&255];
    phase=(int)((long)(pitch*40.743665f)&255L);
    sp=sintable[phase];cp=sintable[(phase+64)&255];
    frame_stamp++;
    if(!frame_stamp) {
        for(k=0;k<GRID_CORNERS;k++)corner_stamp[k]=0;
        frame_stamp=1;
    }
    for(k=0;k<=RD_Y;k++) {
        int dy=(int)k*256-camy;
        vertical_y[k]=(long)dy*cp;vertical_z[k]=(long)dy*sp;
    }
    total=(unsigned)width*(unsigned)height;
    for(k=0;k<total;k++)image[k]=11;
    /* Each ray moves monotonically away from the eye in each grid axis.
     * Therefore Manhattan shells give a valid painter order without sorting.
     * Radius is deliberately bounded for an 8088/V20. */
    if(!surface_overflow) {
        /* Bucket only exposed cells instead of scanning the entire volume.
         * Distance ties cannot occlude each other except shared edge pixels. */
        for(ring=0;ring<=RADIUS;ring++)heads[ring]=65535U;
        for(k=0;k<surface_count;k++) {
            unsigned index=surface[k];
            int bx=index%RD_X,bz=(index/RD_X)%RD_Z,by=index/(RD_X*RD_Z);
            int dx=bx-ex,dy=by-ey,dz=bz-ez;
            ring=(dx<0?-dx:dx)+(dy<0?-dy:dy)+(dz<0?-dz:dz);
            if(ring>RADIUS)continue;
            next_surface[k]=heads[ring];heads[ring]=(unsigned short)k;
        }
        for(ring=RADIUS;ring>=0;ring--) {
            unsigned node=heads[ring];
            while(node!=65535U) {
                unsigned index=surface[node];
                draw_cell(index%RD_X,index/(RD_X*RD_Z),(index/RD_X)%RD_Z,ex,ey,ez);
                node=next_surface[node];
            }
        }
    } else
    for(ring=RADIUS;ring>=0;ring--)for(dx=-ring;dx<=ring;dx++) {
        int ax=dx<0?-dx:dx,remain=ring-ax;
        int bx=ex+dx;
        if(bx<0||bx>=RD_X)continue;
        for(dz=-remain;dz<=remain;dz++) {
            int az=dz<0?-dz:dz,bz=ez+dz;
            if(bz<0||bz>=RD_Z)continue;
            dy=remain-az;
            for(sign=0;sign<(dy?2:1);sign++) {
                draw_cell(bx,ey+(sign?-dy:dy),bz,ex,ey,ez);
            }
        }
    }
    /* One physical-size reticle, with dark backing for grass/stone. */
    {int cx=width/2,yy=height/2,i;
     for(i=-2;i<=2;i++){
         image[(unsigned)yy*width+cx+i]=0;
         image[(unsigned)(yy+i)*width+cx]=0;
     }
     image[(unsigned)yy*width+cx]=15;}
}
