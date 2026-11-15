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
/* 8-unit depth bins: projected far corners need no 32-bit division.
 * At z>=256, rounding error is <1.6% (subpixel at typical distances).
 * Near faces preserve exact perspective to avoid visual distortion.
 * Table baked at build time, zero startup math on the V20.
 */
static const unsigned short depth_inverse[1024]={
    0, 0, 4096, 2731, 2048, 1638, 1365, 1170, 1024, 910, 819, 745, 683, 630, 585, 546,
    512, 482, 455, 431, 410, 390, 372, 356, 341, 328, 315, 303, 293, 282, 273, 264,
    256, 248, 241, 234, 228, 221, 216, 210, 205, 200, 195, 191, 186, 182, 178, 174,
    171, 167, 164, 161, 158, 155, 152, 149, 146, 144, 141, 139, 137, 134, 132, 130,
    128, 126, 124, 122, 120, 119, 117, 115, 114, 112, 111, 109, 108, 106, 105, 104,
    102, 101, 100, 99, 98, 96, 95, 94, 93, 92, 91, 90, 89, 88, 87, 86,
    85, 84, 84, 83, 82, 81, 80, 80, 79, 78, 77, 77, 76, 75, 74, 74,
    73, 72, 72, 71, 71, 70, 69, 69, 68, 68, 67, 67, 66, 66, 65, 65,
    64, 64, 63, 63, 62, 62, 61, 61, 60, 60, 59, 59, 59, 58, 58, 57,
    57, 56, 56, 56, 55, 55, 55, 54, 54, 54, 53, 53, 53, 52, 52, 52,
    51, 51, 51, 50, 50, 50, 49, 49, 49, 48, 48, 48, 48, 47, 47, 47,
    47, 46, 46, 46, 46, 45, 45, 45, 45, 44, 44, 44, 44, 43, 43, 43,
    43, 42, 42, 42, 42, 42, 41, 41, 41, 41, 41, 40, 40, 40, 40, 40,
    39, 39, 39, 39, 39, 38, 38, 38, 38, 38, 38, 37, 37, 37, 37, 37,
    37, 36, 36, 36, 36, 36, 36, 35, 35, 35, 35, 35, 35, 35, 34, 34,
    34, 34, 34, 34, 34, 33, 33, 33, 33, 33, 33, 33, 33, 32, 32, 32,
    32, 32, 32, 32, 32, 31, 31, 31, 31, 31, 31, 31, 31, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
    28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 27, 27, 27, 27, 27, 27,
    27, 27, 27, 27, 27, 27, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
    26, 26, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 24,
    24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 23, 23, 23,
    23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 22, 22, 22,
    22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 21, 21,
    21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
    20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
    20, 20, 20, 20, 20, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
    19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 18, 18, 18, 18, 18,
    18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
    18, 18, 18, 18, 18, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
    17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
    17, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
    14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
    14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 13,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
    12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
    12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
    12, 12, 12, 12, 12, 12, 12, 12, 12, 11, 11, 11, 11, 11, 11, 11,
    11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
    11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
    11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
    11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 9,
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
    9, 9, 9, 9, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
};
#define MAX_H 200
#define RADIUS 32
/* Distant microscopic faces cost more than their visual contribution.
 * Preserve full-size, shaded, texture-mapped near cubes; MID draws 16
 * blocks of geometry, LOW 11, MAX 28. The outside sky remains cyan.
 */
static int render_radius=16;
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
        if(b[i].z>=256 && b[i].z<8188) {
            /* Replace two slow software 32-bit divides per distant vertex
             * with two signed 32-bit multiplies and a fixed shift. */
            unsigned index=(unsigned)(b[i].z+4)>>3;
            unsigned short inv=depth_inverse[index];
            p[i].x=fw/2+(int)(((long)b[i].x*fx*inv)>>16);
            p[i].y=fh/2-(int)(((long)b[i].y*fy*inv)>>16);
        } else {
            p[i].x=fw/2+(int)((long)b[i].x*fx/b[i].z);
            p[i].y=fh/2-(int)((long)b[i].y*fy/b[i].z);
        }
        if(p[i].x<minx)minx=p[i].x;
        if(p[i].x>maxx)maxx=p[i].x;
        p[i].u=b[i].u;p[i].v=b[i].v;
        if(p[i].y<lo)lo=p[i].y;
        if(p[i].y>hi)hi=p[i].y;
    }
    if(maxx<0||minx>=fw)return;
    /* Tiny faces show their original material color, not expensive
     * 16x16 texel interpolation invisible on the physical CRT.
     */
    flat=(fw<=160&&((maxx-minx)<=9||(hi-lo)<=6));
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
        /* UV is limited to 0..4095 (a 16x16 tile). Native 16-bit IDIV
         * suffices, removing TWO long 32-bit divides per visible edge. */
        du=flat?0:(long)((e.u-s.u)/dy)*256L;
        dv=flat?0:(long)((e.v-s.v)/dy)*256L;
        x=(long)s.x*256+dx*(start-s.y);
        u=(long)s.u*256+du*(start-s.y);
        v=(long)s.v*256+dv*(start-s.y);
        for(y=start;y<=end;y++,x+=dx,u+=du,v+=dv){
            int xx=(int)(x>>8);
            if(xx<lx[y]){lx[y]=xx;lu[y]=u;lv[y]=v;}
            if(xx>rx[y]){rx[y]=xx;ru[y]=u;rv[y]=v;}
        }
    }
    /* TEXEL SCAN: the original used two 32-bit signed divisions per
     * horizontal scanline, then 32-bit u/v additions PER PIXEL.
     * For a 16x16 atlas, 8.8 (0..4095) is entirely sufficient:
     * two native 16-bit signed divisions per row and 16-bit increments.
     * Preserve clipped near faces with long intermediate on left clipping.
     */
    for(y=lo;y<=hi;y++) {
        int left=lx[y],right=rx[y],span;
        int u,v,du,dv;unsigned offset;
        if(left>right||right<0||left>=fw)continue;
        if(flat) {
            if(left<0)left=0;if(right>=fw)right=fw-1;
            memset(frame+(unsigned)y*(unsigned)fw+(unsigned)left,texture[136],
                   (unsigned)(right-left+1));
            continue;
        }
        span=right-left;
        u=(int)(lu[y]>>8);
        v=(int)(lv[y]>>8);
        du=span?(int)((ru[y]-lu[y])>>8)/span:0;
        dv=span?(int)((rv[y]-lv[y])>>8)/span:0;
        if(left<0) {
            u=(int)((long)u-(long)du*(long)left);
            v=(int)((long)v-(long)dv*(long)left);
            left=0;
        }
        if(right>=fw)right=fw-1;
        offset=(unsigned)y*(unsigned)fw+(unsigned)left;
        for(j=left;j<=right;j++,u+=du,v+=dv) {
            int tu=u>>8,tv=v>>8;
            if(tu<0)tu=0;if(tu>15)tu=15;
            if(tv<0)tv=0;if(tv>15)tv=15;
            frame[offset++]=texture[(tv<<4)+tu];
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
static void faces_scene(int id,rd_u8 *image,int width,int height,int near_only) {
    float x,y,z,yaw,pitch;int ex,ey,ez,phase,ring,dx,dy,dz,sign;
    unsigned k,total;
    if(!image||width<8||width>320||height<8||height>MAX_H)return;
    frame=image;fw=width;fh=height;blocks=rd_blocks();
    /* Hybrid mode renders only the first few block shells in real 3D.
     * The far scene is provided by the cached heightfield approximation.
     * Full /FACES retains the prior radius and the original scene.
     */
    /* Focus high-detail cubes on the immediate build/mining range.
     * Far scenery has its own layer, so full-radius faces are unnecessary.
     */
    render_radius=near_only?(width<=80?(height<=50?5:6):8):
                             (width<=80?11:(width<=160?17:28));
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
    if(!near_only)for(k=0;k<total;k++)image[k]=11;
    /* Each ray moves monotonically away from the eye in each grid axis.
     * Therefore Manhattan shells give a valid painter order without sorting.
     * Radius is deliberately bounded for an 8088/V20. */
    if(!near_only&&!surface_overflow) {
        /* Bucket only exposed cells instead of scanning the entire volume.
         * Distance ties cannot occlude each other except shared edge pixels. */
        for(ring=0;ring<=RADIUS;ring++)heads[ring]=65535U;
        for(k=0;k<surface_count;k++) {
            unsigned index=surface[k];
            int bx=index%RD_X,bz=(index/RD_X)%RD_Z,by=index/(RD_X*RD_Z);
            int dx=bx-ex,dy=by-ey,dz=bz-ez;
            ring=(dx<0?-dx:dx)+(dy<0?-dy:dy)+(dz<0?-dz:dz);
            if(ring>render_radius)continue;
            next_surface[k]=heads[ring];heads[ring]=(unsigned short)k;
        }
        for(ring=render_radius;ring>=0;ring--) {
            unsigned node=heads[ring];
            while(node!=65535U) {
                unsigned index=surface[node];
                draw_cell(index%RD_X,index/(RD_X*RD_Z),(index/RD_X)%RD_Z,ex,ey,ez);
                node=next_surface[node];
            }
        }
    } else
    for(ring=render_radius;ring>=0;ring--)for(dx=-ring;dx<=ring;dx++) {
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

/* Build #30 full-distance face renderer, available through /FACES. */
void rd_faces_render(int id,rd_u8 *image,int w,int h) {
    faces_scene(id,image,w,h,0);
}
/* No sky clear. Draw shaded textured close voxels over far background. */
void rd_faces_render_near(int id,rd_u8 *image,int w,int h) {
    faces_scene(id,image,w,h,1);
}
