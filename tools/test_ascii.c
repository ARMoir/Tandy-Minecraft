/* True 3D text-mode raycast regression for DOS V20.
 * Tests exact 80x25 character/attribute boundaries, real 3D walls,
 * camera turn, looking up/down and low/high ray budgets.
 */
#include <stdio.h>
#include <string.h>
#include "../src/rd_ascii.h"
#define SIZE RD_ASCII_FRAME_BYTES
static rd_u8 buf[SIZE+2],first[SIZE];
static int render(int me,int cols,int rows) {
    unsigned i;
    memset(buf,0x7d,sizeof buf);
    rd_ascii_render(me,buf+1,cols,rows);
    if(buf[0]!=0x7d || buf[SIZE+1]!=0x7d) {
        puts("ASCII video output buffer overflow");return 1;
    }
    return 0;
}
static int differences(void) {
    int i,n=0;
    for(i=0;i<SIZE;i++)if(first[i]!=buf[i+1])n++;
    return n;
}
int main(void) {
    rd_u8 *world;
    int me,i,sky=0,solid=0,turns,tilts,wall;
    FILE *f;
    float x,y,z,yaw,pitch;
    rd_init(132211UL,RD_HILLS);
    me=rd_player_add();if(me<0)return 1;
    if(render(me,32,16))return 2;
    for(i=0;i<SIZE;i+=2) {
        int ch=buf[i+1],a=buf[i+2];
        if(ch==' ' && (a>>4)==3)sky++;
        if(ch>=176&&ch<=219)solid++;
    }
    if(sky<60||solid<40) {
        fprintf(stderr,"Expected 3D terrain + sky: sky=%d solid=%d\n",sky,solid);
        return 3;
    }
    memcpy(first,buf+1,SIZE);
    rd_player_pos(me,&x,&y,&z);rd_player_angles(me,&yaw,&pitch);
    rd_player_set(me,x,y,z,yaw+0.55f,pitch);
    if(render(me,32,16))return 4;
    turns=differences();
    if(turns<60){fprintf(stderr,"Yaw didn't alter ASCII scene: %d bytes\n",turns);return 5;}
    rd_player_set(me,x,y,z,yaw,pitch+0.5f);
    if(render(me,32,16))return 6;
    tilts=differences();
    if(tilts<60){fprintf(stderr,"Pitch didn't alter ASCII scene: %d bytes\n",tilts);return 7;}
    if(render(me,20,10)||render(me,40,20))return 8;
    /* A wall directly in the player's line of sight must show as actual
     * 3D voxels and disappear when removed. This isn't a heightfield.
     */
    world=rd_blocks();memset(world,RD_AIR,(unsigned)rd_world_bytes());
    for(i=12;i<18;i++){
        int z0=19,h;
        for(h=12;h<17;h++)
            world[(h*RD_Z+z0)*RD_X+i]=RD_STONE;
    }
    rd_relight();
    rd_player_set(me,16.5f,12.0f,16.5f,0.0f,0.0f);
    if(render(me,32,16))return 9;
    memcpy(first,buf+1,SIZE);
    memset(world,RD_AIR,(unsigned)rd_world_bytes());rd_relight();
    if(render(me,32,16))return 10;
    wall=differences();
    if(wall<60){fprintf(stderr,"Voxel wall didn't change raycast: %d bytes\n",wall);return 11;}
    /* The first generated view is a human-reviewable reference:
     * map DOS CP437 glyphs to printable levels for CI/artifacts.
     */
    rd_init(132211UL,RD_HILLS);me=rd_player_add();
    if(me<0||render(me,32,16))return 12;
    f=fopen("ASCII-PREVIEW.TXT","wb");
    if(!f)return 13;
    for(i=0;i<23;i++){
        int j;
        for(j=0;j<80;j++){
            int ch=buf[1+(i*80+j)*2],a=buf[2+(i*80+j)*2];
            char out=ch==219?'#':ch==178?'O':ch==177?'o':
                     ch==176?'.':ch=='+':'+':(a>>4)==3?' ':' ';
            fputc(out,f);
        }
        fputc('\n',f);
    }
    fclose(f);
    printf("PASS: ASCII 80x25 CP437 sky=%d solid=%d yaw=%d pitch=%d removed-wall=%d; 20x10/32x16/40x20\n",
           sky,solid,turns,tilts,wall);
    return 0;
}
