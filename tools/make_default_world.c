/* Offline build-host generator: exact original rdgeneric world, compressed
 * as unsigned-byte (run length,block ID) pairs for standalone DOS MINE.EXE.
 */
#include <stdio.h>
#include "../src/rd.h"
int main(int argc,char **argv) {
    const rd_u8 *world;
    unsigned i=0,n=0,run=0;
    unsigned total=(unsigned)(RD_X*RD_Y*RD_Z);
    FILE *f;
    if(argc!=2){fprintf(stderr,"usage: make_default_world output-header\n");return 2;}
    rd_init(132211UL,RD_HILLS);
    world=rd_blocks();
    f=fopen(argv[1],"wb");if(!f){perror(argv[1]);return 1;}
    fprintf(f,"/* Generated from rdgeneric seed 132211, RD_HILLS; DO NOT EDIT. */\n");
    fprintf(f,"#ifndef RD_WORLD_RLE_H\n#define RD_WORLD_RLE_H\n");
    fprintf(f,"static const rd_u8 rd_world_rle[]={\n");
    while(i<total) {
        rd_u8 block=world[i];
        run=1;
        while(i+run<total && run<255 && world[i+run]==block)run++;
        if(n%12==0)fprintf(f," ");
        fprintf(f,"%u,%u,",(unsigned)run,(unsigned)block);
        n+=2;
        if(n%12==0)fprintf(f,"\n");
        i+=run;
    }
    fprintf(f,"\n};\n#define RD_WORLD_RLE_BYTES %uU\n#endif\n",n);
    if(fclose(f))return 1;
    printf("Prebuilt world %u bytes, RLE %u bytes (%.1f%% of raw)\n",
           total,n,100.0*(double)n/(double)total);
    return 0;
}
