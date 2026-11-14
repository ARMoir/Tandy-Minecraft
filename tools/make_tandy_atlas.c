/* Converts the exact rdgeneric 16x16 texture generator into
 * Tandy palette indices and face shading on the build host. The 7MHz V20
 * never computes RGB textures at startup or during rendering.
 */
#include <stdio.h>
#include "../src/rd.c"
static const unsigned char palette[16][3]={
 {0,0,0},{0,0,170},{0,170,0},{0,170,170},
 {170,0,0},{170,0,170},{170,85,0},{170,170,170},
 {85,85,85},{85,85,255},{85,255,85},{85,255,255},
 {255,85,85},{255,85,255},{255,255,85},{255,255,255}
};
static const unsigned char bayer[16]={0,8,2,10,12,4,14,6,3,11,1,9,15,7,13,5};
static int clamp(int a){return a<0?0:a>255?255:a;}
static int nearest(rd_u32 color,int x,int y){
    int r,g,b,best=0,i,dr,dg,db,offset;
    long error,closest=0x7fffffffL;
    offset=((int)bayer[(y&3)*4+(x&3)]-8)*3;
    r=clamp((int)((color>>16)&255U)+offset);
    g=clamp((int)((color>>8)&255U)+offset);
    b=clamp((int)(color&255U)+offset);
    for(i=0;i<16;i++) {
        dr=r-palette[i][0];dg=g-palette[i][1];db=b-palette[i][2];
        error=(long)dr*dr+(long)dg*dg+(long)db*db;
        if(error<closest){closest=error;best=i;}
    }
    return best;
}
int main(int argc,char **argv){
    FILE *f;int block,face,x,y,v,texture,index;
    float shade;
    if(argc!=2){puts("usage: atlasgen output-header");return 2;}
    make_textures(); /* Static function from the original rd.c. */
    f=fopen(argv[1],"wb");if(!f){perror(argv[1]);return 1;}
    fprintf(f,"/* Generated from original rdgeneric textures. */\n");
    fprintf(f,"#ifndef RD_TANDY_ATLAS_H\n#define RD_TANDY_ATLAS_H\n");
    fprintf(f,"static const rd_u8 rd_tandy_atlas[RD_NUM_BLOCKS][8][256]={\n");
    for(block=0;block<RD_NUM_BLOCKS;block++) {
        fprintf(f," {\n");
        for(face=0;face<8;face++) {
            texture=face%4==1?FACE_TOP:face%4==3?FACE_BOTTOM:FACE_SIDE;
            shade=face%4==0?0.6f:face%4==2?0.8f:1.0f;
            if(face>=4)shade*=0.6f;
            fprintf(f,"  {\n");
            for(y=0;y<16;y++) {
                fprintf(f,"   ");
                for(x=0;x<16;x++) {
                    v=y*16+x;
                    if(block==RD_AIR)index=0;
                    else if(tex[block][texture][v]==TRANSPARENT)index=16;
                    else index=nearest(shade_rgb(tex[block][texture][v],shade),x,y);
                    fprintf(f,"%d%s",index,v==255?"":",");
                }
                fprintf(f,"\n");
            }
            fprintf(f,"  }%s\n",face==7?"":",");
        }
        fprintf(f," }%s\n",block==RD_NUM_BLOCKS-1?"":",");
    }
    fprintf(f,"};\n#endif\n");
    if(fclose(f))return 1;
    puts("16-color original rdgeneric texture atlas generated.");
    return 0;
}
