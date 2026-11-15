#include <stdio.h>
#include <string.h>
#include "../src/rd_fast.h"
static rd_u8 pixels[64000],pic[32768];
static const unsigned char rgb[16][3]={
 {0,0,0},{0,0,170},{0,170,0},{0,170,170},{170,0,0},{170,0,170},
 {170,85,0},{170,170,170},{85,85,85},{85,85,255},{85,255,85},
 {85,255,255},{255,85,85},{255,85,255},{255,255,85},{255,255,255}};
static void output(const char *name,int id,int old) {
 FILE *f=fopen(name,"wb");unsigned i;
 if(!f)return;
 if(old)rd_terrain_render(id,pixels,40,40);
 else rd_faces_render(id,pixels,320,200);
 fprintf(f,"P6\n320 200\n255\n");
 for(i=0;i<64000U;i++) {
   int c=old?pixels[(i/320/5)*40+(i%320)/8]:pixels[i];
   fwrite(rgb[c&15],1,3,f);
 }
 fclose(f);
}
int main(void) {
 int id,x,y,z;rd_u8 *w;
 rd_init(132211UL,RD_HILLS);id=rd_player_add();
 output("before.ppm",id,1);output("after.ppm",id,0);
 {FILE *f=fopen("HOST.RAW","wb");if(!f)return 1;fwrite(pixels,1,64000,f);fclose(f);}
 {FILE *f=fopen("MINE.PIC","wb");
  for(y=0;y<200;y++)for(x=0;x<160;x++)
   pic[(y&3)*8192+(y/4)*160+x]=(rd_u8)((pixels[y*320+x*2]<<4)|pixels[y*320+x*2+1]);
  if(!f)return 1;fwrite(pic,1,32768,f);fclose(f);}

 /* Controlled block scene: all materials plus a wall with a real opening. */
 w=rd_blocks();memset(w,0,rd_world_bytes());
 for(z=0;z<RD_Z;z++)for(x=0;x<RD_X;x++)
   for(y=0;y<5;y++)w[(y*RD_Z+z)*RD_X+x]=y==4?RD_GRASS:RD_DIRT;
 for(z=13;z<=16;z++)for(x=12;x<=17;x++)
   for(y=5;y<9;y++)if(z==16||x==12||x==17)
     if(!(z==16&&x==14&&y<7))w[(y*RD_Z+z)*RD_X+x]=RD_PLANKS;
 for(y=5;y<9;y++)w[(y*RD_Z+14)*RD_X+20]=RD_LOG;
 for(z=13;z<=15;z++)for(x=19;x<=21;x++)
   for(y=8;y<10;y++)w[(y*RD_Z+z)*RD_X+x]=RD_LEAVES;
 for(x=9;x<12;x++)w[(5*RD_Z+12)*RD_X+x]=RD_COBBLE;
 rd_relight();rd_player_set(id,16.5f,5.0f,8.5f,0,-0.12f);
 output("blocks.ppm",id,0);
 rd_player_set(id,15.5f,5.0f,14.5f,0,-0.1f);
 output("doorway.ppm",id,0);
 return 0;
}
