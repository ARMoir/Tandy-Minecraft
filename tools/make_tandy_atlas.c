/* Converts the exact rdgeneric 16x16 texture generator into
 * Tandy palette indices and face shading on the build host. The 7MHz V20
 * never computes RGB textures at startup or during rendering.
 */
#include <stdio.h>
#include "../src/rd.c"
/* Palette choices are material-aware. Naive nearest-RGB causes the
 * very characteristic original grass green (98,168,63) to quantize to
 * EGA grey (85,85,85)! On a 40x40 Tandy image that looks like visual noise.
 *
 * Source rdgeneric texture RGB still supplies hue/brightness, seams,
 * transparency and grain. Only the OUTPUT COLOR FAMILY is constrained:
 * grass/leaves green, dirt/logs brown, rock grey, planks wood/yellow.
 *
 * 2/10 = green, 6 = brown, 7/8 = grey, 14 = yellow, 16 = transparent.
 * This is precomputed by Actions: no palette math executes on the V20.
 */
static const unsigned char bayer[16]={
    0,8,2,10,12,4,14,6,3,11,1,9,15,7,13,5
};
static int material_color(int block,int face,int shade_face,
                          rd_u32 rgb,int x,int y) {
    int r=(int)((rgb>>16)&255UL);
    int g=(int)((rgb>>8)&255UL);
    int b=(int)(rgb&255UL);
    int lum=(r*3+g*4+b)/8;
    int grain=bayer[((y&3)<<2)|(x&3)];
    int shadow=shade_face>=4;
    switch(block) {
    case RD_GRASS:
        /* The original side texture has a green lip and brown underside. */
        if(face==FACE_TOP || (face==FACE_SIDE && g>r+12 && g>b+24))
            return (!shadow && g>145 && grain>=13) ? 10:2;
        return 6;
    case RD_LEAVES:
        return (!shadow && g>135 && grain>=12)?10:2;
    case RD_DIRT: return 6;
    case RD_STONE:
    case RD_COBBLE:
        return (!shadow && lum>120 && grain>=9)?7:8;
    case RD_LOG:
        return (!shadow && face!=FACE_SIDE && lum>120 && grain>=14)?14:6;
    case RD_PLANKS:
        return (!shadow && lum>140 && grain>=12)?14:6;
    default:return 0;
    }
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
                    else index=material_color(block,texture,face,
                                shade_rgb(tex[block][texture][v],shade),x,y);
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
