/* Real output sanity for the centered DOS viewpoint. Checks appearance,
 * not just variety of colors: top grass should not be grey, leaves green,
 * sky blue, and surface changes should follow world edits.
 */
#include <stdio.h>
#include <string.h>
#include "../src/rd_fast.h"
#include "../src/rd_tandy_atlas.h"
static rd_u8 a[40*40],b[40*40];
static int change(void) {int i,n=0;for(i=0;i<1600;i++)n+=(a[i]!=b[i]);return n;}
int main(void){
 int id,i,sky=0,green=0,grey=0,blue_ground=0,edited;
 rd_u8 *world;
 rd_init(132211UL,RD_HILLS);
 id=rd_player_add();if(id<0)return 1;
 rd_terrain_render(id,a,40,40);
 for(i=0;i<1600;i++){
     if(a[i]>15)return 2;
     if(i<400&&a[i]==11)sky++;
     if(i>=800&&(a[i]==2||a[i]==10))green++;
     if(i>=800&&(a[i]==7||a[i]==8))grey++;
     if(i>=800&&a[i]==11)blue_ground++;
 }
 if(sky<120 || green<280 || grey>240 || blue_ground>400){
     fprintf(stderr,"No readable landscape: blue sky=%d green ground=%d grey ground=%d sky at bottom=%d\n",sky,green,grey,blue_ground);
     return 3;
 }
 world=rd_blocks();memset(world,RD_AIR,(unsigned)rd_world_bytes());
 rd_relight();rd_terrain_render(id,b,40,40);
 edited=change();
 if(edited<120){puts("Terrain does not reflect block edits");return 4;}
 printf("PASS: image sky=%d, green ground=%d, grey=%d, edits=%d\n",
        sky,green,grey,edited);
 return 0;
}
