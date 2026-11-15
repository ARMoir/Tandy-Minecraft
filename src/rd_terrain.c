/* MINE.EXE readable Tandy terrain renderer.
 * Heightfield rasterization: one horizontal ray per output column, front-to-
 * back terrain horizon clipping, original rdgeneric Tandy-indexed textures.
 * VISUAL MODE: draws top faces for walkable grass, dirt/stone cliff sides,
 * and separates leaves/logs into tree silhouettes; no caves or overhangs.
 * No floating-point calculations in any ray or textured-pixel loop.
 * /FULL keeps the original textured 3D DDA renderer.
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

static const unsigned short zoom40[TERRAIN_STEPS]={
    6656, 4437, 3328, 2662, 2219, 1902, 1664, 1479, 1331, 1210, 1109, 1024, 951, 887, 832, 783, 740, 701, 666, 634, 605, 579, 555, 532
};
static int heading(float radians) {
    return (int)(((long)(radians*40.743665f))&255L);
}
static int clamp_screen(int y,int h) {
    return y<0?0:y>h?h:y;
}
static int abs_int(int x) {return x<0?-x:x;}
static int project(int height,int eye,int center,unsigned short scale,int screenh) {
    long dy=((long)height<<8)-(long)eye;
    long yy=(long)center-((dy*(long)scale)>>16);
    if(yy<0)return 0;
    if(yy>screenh)return screenh;
    return (int)yy;
}
/* A terrain cell's surface is NOT the top of its tree canopy.
 * Scan under leaf/log layers to preserve grassy ground plus distinct trees.
 * Existing rdgeneric world mutations are reflected without a cache rebuild.
 */
static int ground_surface(const rd_u8 *world,int x,int z,int height) {
    while(height>0) {
        int b=world[((height-1)*RD_Z+z)*RD_X+x];
        if(b!=RD_LEAVES&&b!=RD_LOG)break;
        height--;
    }
    return height;
}
static void paint_ground(rd_u8 *image,const rd_u8 *world,
                        int width,int height,int col,int from,int to,
                        int bx,int bz,int fixedx,int fixedz,int top,
                        int samples_per_block,int step,int facing,int cliff) {
    int y,base=(bx+bz*RD_X),u,v,mat,worldy,level,face;
    int coord=facing?(fixedz>>4):(fixedx>>4);
    for(y=from;y<to;y++){
        int within=y-from;
        level=within/samples_per_block;
        worldy=top-1-level;
        if(worldy<0)worldy=0;
        mat=world[(worldy*RD_Z+bz)*RD_X+bx];
        if(mat==RD_AIR)continue;
        if(mat==RD_LOG||mat==RD_LEAVES)mat=RD_GRASS;
        /* Flat/rolling ground exposes the GRASS TOP, not a single stone
         * side stretched to the bottom of the screen. Only sharp rises
         * show stratified dirt/stone walls.
         */
        if(!cliff) {
            mat=world[((top-1)*RD_Z+bz)*RD_X+bx];
            face=1; /* original top texture */
            /* Stable world-space texture mapping, without scrolling
             * two full texels per screen row (which caused green/gray fuzz).
             * Each surface cell has one coarse Tandy-readable texture.
             */
            u=(fixedx>>4)&15;
            v=((fixedz>>4)+(y-from)/3)&15;
        } else {
            /* Even a SINGLE block rise exposes a dirt side. Otherwise
             * a typical stair-stepped Minecraft hill is indistinguishable
             * from a flat green sheet. Keep a two-pixel grass cap.
             */
            if(within>=2 && mat==RD_GRASS)mat=RD_DIRT;
            face=(within<2)?1:(facing?0:2);
            u=coord&15;
            v=((worldy<<2)+(y-from)/2)&15;
        }
        if(mat==RD_AIR||mat>=RD_NUM_BLOCKS)continue;
        {rd_u8 pixel=rd_fast_texel(mat,face,(v<<4)|u);
         if(pixel!=16)image[(unsigned)y*(unsigned)width+(unsigned)col]=pixel;}
    }
    (void)base;(void)step;(void)height;
}
static void paint_tree(rd_u8 *image,const rd_u8 *world,
                       int width,int col,int from,int to,
                       int bx,int bz,int x,int z,int leaf_top,
                       int samples_per_block) {
    int y;
    for(y=from;y<to;y++){
        int worldy=leaf_top-1-(y-from)/samples_per_block;
        int mat,u,v,face;
        rd_u8 pixel;
        if(worldy<0||worldy>=RD_Y)continue;
        mat=world[(worldy*RD_Z+bz)*RD_X+bx];
        if(mat!=RD_LEAVES&&mat!=RD_LOG)continue;
        /* World-space foliage, not rapidly alternating cyan/grey noise. */
        u=((x>>4)+(y-from)/4)&15;
        v=((z>>4)+(y-from)/3)&15;
        face=mat==RD_LEAVES?1:2;
        pixel=rd_fast_texel(mat,face,(v<<4)|u);
        if(pixel!=16)image[(unsigned)y*(unsigned)width+(unsigned)col]=pixel;
    }
}
void rd_terrain_render(int id,rd_u8 *image,int width,int height) {
    float px,py,pz,yaw,pitch;
    const rd_u8 *world=rd_blocks(),*tops=rd_light_depth();
    const unsigned short *zoom;
    int ox,oz,eye,phase,sn,cs,center,i,j,depth;
    if(!image||width<2||width>80||height<2||height>50)return;
    rd_player_pos(id,&px,&py,&pz);
    rd_player_angles(id,&yaw,&pitch);
    ox=(int)(px*256.0f);oz=(int)(pz*256.0f);
    eye=(int)((py+1.62f)*256.0f);
    phase=heading(yaw);
    sn=sintable[phase];cs=sintable[(phase+64)&255];
    /* In the 2.5-D terrain renderer this IS the visible sky/ground horizon.
     * Use the full vertical view scale so Up/Down visibly tilt the scene.
     */
    center=height/2+(int)(pitch*((float)height/2.0f));
    zoom=height==50?zoom50:(height==40?zoom40:zoom25);
    /* No fake ground! The world geometry must provide the land horizon.
     * Previously half the screen was painted green even with NO blocks.
     */
    for(j=0;j<height;j++)
        for(i=0;i<width;i++)image[(unsigned)j*width+(unsigned)i]=11;
    for(i=0;i<width;i++){
        int angle=(2*i+1-width)*180/width;
        int vx=sn+(int)(((long)cs*angle)>>8);
        int vz=cs-(int)(((long)sn*angle)>>8);
        int prevx=-1,prevz=-1,prevground=-1;
        int clip=height,face_x=abs_int(vx)>abs_int(vz);
        for(depth=0;depth<TERRAIN_STEPS;depth++){
            unsigned dist=(unsigned)(256+depth*128);
            int x=ox+(int)(((long)vx*dist)>>8);
            int z=oz+(int)(((long)vz*dist)>>8);
            int bx,bz,top,ground,pyground,ptree;
            int blockheight=zoom[depth]>>8;
            int cliff;
            if(x<0||z<0||x>=RD_X*256||z>=RD_Z*256)break;
            bx=x>>8;bz=z>>8;
            if(bx==prevx&&bz==prevz)continue;
            prevx=bx;prevz=bz;
            top=tops[bx+bz*RD_X];
            if(!top)continue;
            ground=ground_surface(world,bx,bz,top);
            if(!ground)continue;
            if(!blockheight)blockheight=1;
            pyground=project(ground,eye,center,zoom[depth],height);
            cliff=prevground>=0 && ground>prevground; /* one-block stairs have visible sides */
            if(pyground<clip){
                paint_ground(image,world,width,height,i,pyground,clip,bx,bz,
                             x,z,ground,blockheight,depth,face_x,cliff);
                clip=pyground;
            }
            if(top>ground) {
                ptree=project(top,eye,center,zoom[depth],height);
                if(ptree<clip){
                    /* Actual source-world leaf/log materials drive tree color. */
                    paint_tree(image,world,width,i,ptree,clip,bx,bz,x,z,
                               top,blockheight);
                    clip=ptree;
                }
            }
            prevground=ground;
            if(clip==0)break;
        }
    }
    /* Tiny top-right compass: yellow dot moves as Left/Right changes yaw.
     * This makes camera rotation obvious even when facing empty sky.
     */
    if(width>=20&&height>=16) {
        int cx=width-4,cy=3,dx=(sn>95?1:(sn< -95?-1:0));
        int dy=(cs>95?-1:(cs< -95?1:0));
        int tx,ty;
        for(ty=-2;ty<=2;ty++)
            for(tx=-2;tx<=2;tx++)
                image[(unsigned)(cy+ty)*width+(unsigned)(cx+tx)]=
                    (tx==-2||tx==2||ty==-2||ty==2)?8:0;
        image[(unsigned)cy*width+(unsigned)cx]=15;
        image[(unsigned)(cy+dy)*width+(unsigned)(cx+dx)]=14;
    }
    /* Small, high-contrast center reticle rather than a giant /LOW cross. */
    i=width/2;j=height/2;
    image[(unsigned)j*(unsigned)width+(unsigned)i]=15;
    if(width>=40) {
        if(i>0)image[(unsigned)j*width+(unsigned)(i-1)]=0;
        if(i+1<width)image[(unsigned)j*width+(unsigned)(i+1)]=0;
        if(j>0)image[(unsigned)(j-1)*width+(unsigned)i]=0;
        if(j+1<height)image[(unsigned)(j+1)*width+(unsigned)i]=0;
    }
    if(height>4&&width>6){
        int block=rd_selected(id);
        rd_u8 ink=rd_fast_texel(block,1,136);
        if(ink==16)ink=10;
        for(j=height-3;j<height-1;j++)
            for(i=2;i<5;i++)
                image[(unsigned)j*width+(unsigned)i]=(i==2||i==4||j==height-3)?0:ink;
    }
}
