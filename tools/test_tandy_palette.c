/* Validate that Tandy texture conversion preserves MATERIAL IDs/colors.
 * The original naive RGB-distance mapper wrongly classified grass as grey.
 */
#include <stdio.h>
#include "../src/rd.h"
#include "../src/rd_tandy_atlas.h"
static int only(int block,int variant,int a,int b,int c) {
    int i,count=0;
    for(i=0;i<256;i++) {
        rd_u8 p=rd_tandy_atlas[block][variant][i];
        if(p!=a&&p!=b&&p!=c) {
            fprintf(stderr,"wrong color: material %d face %d texel %d => %d\n",
                    block,variant,i,(int)p);
            return 0;
        }
        count++;
    }
    return count==256;
}
int main(void) {
    int i,brown=0,green=0,transparent=0;
    if(!only(RD_GRASS,1,2,10,2))return 1;
    if(!only(RD_GRASS,5,2,10,2))return 2;
    if(!only(RD_DIRT,1,6,6,6))return 3;
    if(!only(RD_STONE,1,7,8,7))return 4;
    if(!only(RD_COBBLE,0,7,8,7))return 5;
    if(!only(RD_PLANKS,2,6,14,6))return 6;
    if(!only(RD_LOG,2,6,14,6))return 7;
    if(!only(RD_LEAVES,1,2,10,16))return 8;
    for(i=0;i<256;i++) {
        rd_u8 g=rd_tandy_atlas[RD_GRASS][1][i];
        rd_u8 d=rd_tandy_atlas[RD_DIRT][1][i];
        rd_u8 l=rd_tandy_atlas[RD_LEAVES][1][i];
        green+=(g==2||g==10);
        brown+=d==6;
        transparent+=l==16;
    }
    if(green!=256||brown!=256||transparent<5) {
        fprintf(stderr,"palette coverage failed: green=%d brown=%d leaf holes=%d\n",green,brown,transparent);
        return 9;
    }
    printf("PASS: grass stays GREEN, dirt/log brown, stone GREY, foliage GREEN (%d holes)\n",
           transparent);
    return 0;
}
