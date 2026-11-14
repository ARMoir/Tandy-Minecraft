/* Ultra-fast 2.5-D voxel terrain renderer for Tandy 1000 EX V20.
 * A Comanche/voxel-space style landscape algorithm, not full cube DDA:
 * ONE horizontal ray for each VIEW COLUMN and ~24 terrain samples per ray.
 * It projects the highest visible block in each terrain column.
 * Preserves source 16x16 Tandy atlas, material shading and player controls.
 *
 * Terrain is a deliberate visual approximation: caves/overhangs and
 * underside faces do not render. Use /FULL for the genuine 3D DDA.
 */
#include "rd_fast.h"
#define TERRAIN_STEPS 24
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
static const unsigned short zoom25[TERRAIN_STEPS]={
    4096, 2731, 2048, 1638, 1365, 1170, 1024, 910, 819, 745, 683, 630, 585, 546, 512, 482, 455, 431, 410, 390, 372, 356, 341, 328
};
static const unsigned short zoom50[TERRAIN_STEPS]={
    8192, 5461, 4096, 3277, 2731, 2341, 2048, 1820, 1638, 1489, 1365, 1260, 1170, 1092, 1024, 964, 910, 862, 819, 780, 745, 712, 683, 655
};
static int heading(float radians){
    return (int)(((long)(radians*40.743665f))&255L);
}
void rd_terrain_render(int id,rd_u8 *image,int width,int height){
    float px,py,pz,yaw,pitch;
    int ox,oz,eye,phase,sn,cs,half,i,j,depth,headroom;
    const rd_u8 *world=rd_blocks(),*ceiling=rd_light_depth();
    const unsigned short *zoom;
    if(!image||width<2||width>80||height<2||height>50)return;
    rd_player_pos(id,&px,&py,&pz);
    rd_player_angles(id,&yaw,&pitch);
    ox=(int)(px*256.0f); oz=(int)(pz*256.0f);
    eye=(int)((py+1.62f)*256.0f);
    phase=heading(yaw);
    sn=sintable[phase]; cs=sintable[(phase+64)&255];
    half=height/2+(int)(pitch*(float)(height/3));
    zoom=height==50?zoom50:zoom25;
    /* Sky and distant ground background. */
    for(j=0;j<height;j++)for(i=0;i<width;i++)
        image[(unsigned)j*width+i]=j<half?11:8;
    for(i=0;i<width;i++){
        int left=(2*i+1-width)*180/width;
        int vx=sn+(int)(((long)cs*left)>>8);
        int vz=cs-(int)(((long)sn*left)>>8);
        int prev_x=-1,prev_z=-1,front=height;
        for(depth=0;depth<TERRAIN_STEPS;depth++){
            unsigned d=(unsigned)(256+depth*128);
            int x=ox+(int)(((long)vx*d)>>8);
            int z=oz+(int)(((long)vz*d)>>8);
            int bx,bz,top,face,material,projected,y,idx;
            if(x<0||x>=RD_X*256||z<0||z>=RD_Z*256)break;
            bx=x>>8;bz=z>>8;
            if(bx==prev_x&&bz==prev_z)continue;
            prev_x=bx;prev_z=bz;
            top=(int)ceiling[bx+bz*RD_X];
            if(top==0)continue;
            material=(int)world[((top-1)*RD_Z+bz)*RD_X+bx];
            if(material<=RD_AIR||material>=RD_NUM_BLOCKS)continue;
            /* 16-bit screen coordinate, one signed 32-bit multiply.
             * Perspective factors are precomputed at compile time.
             */
            projected=half-(int)(((long)((top<<8)-eye)*zoom[depth])>>16);
            if(projected<0)projected=0;
            if(projected>front)continue;
            face=(depth>8)?2:1;
            for(y=projected;y<front&&y<height;y++){
                int u=((unsigned)x>>4)&15;
                int v=(((unsigned)z>>4)+(unsigned)y)&15;
                int tex=(v<<4)|u;
                rd_u8 pixel=rd_fast_texel(material,face,tex);
                if(pixel==16)pixel=10; /* Leaf surfaces stay green. */
                image[(unsigned)y*width+i]=pixel;
            }
            front=projected;
            if(front==0)break;
        }
    }
    /* UI - center crosshair and block selector, in the source palette. */
    image[(unsigned)(height/2)*width+(unsigned)(width/2)]=15;
    if(width>3){
        image[(unsigned)(height/2)*width+(unsigned)(width/2-1)]=15;
        image[(unsigned)(height/2)*width+(unsigned)(width/2+1)]=15;
    }
    if(height>3){
        image[(unsigned)(height/2-1)*width+(unsigned)(width/2)]=15;
        image[(unsigned)(height/2+1)*width+(unsigned)(width/2)]=15;
    }
    if(height>4&&width>6){
        int block=rd_selected(id);
        rd_u8 p=rd_fast_texel(block,1,136);
        if(p==16)p=10;
        for(j=height-4;j<height-1;j++)for(i=2;i<6;i++)
            image[(unsigned)j*width+i]=(i==2||i==5||j==height-4||j==height-2)?0:p;
    }
}
