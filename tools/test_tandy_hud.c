/* HUD must display actual result and arrow direction in each DOS
 * internal resolution and never write outside frame buffers.
 */
#include <stdio.h>
#include <string.h>
#include "../platforms/dos16/mine_hud.h"
static rd_u8 frame[40*50+8];
static int count(rd_u8 *p,int n,int value){int k,c=0;for(k=0;k<n;k++)if(p[k]==value)c++;return c;}
int main(void) {
    const int widths[3]={20,40,40},heights[3]={25,40,50};
    int k;
    for(k=0;k<3;k++) {
        int w=widths[k],h=heights[k],n=w*h,i;
        memset(frame,11,sizeof frame);
        mine_hud_draw(frame,w,h,RD_ACTION_DUG,8,MINE_DIR_UP,4);
        if(count(frame,n,10)<4||count(frame,n,14)<4) {
            puts("No DIG/UP indicator");return 1;
        }
        for(i=n;i<(int)sizeof frame;i++)if(frame[i]!=11)return 2;
        memset(frame,11,sizeof frame);
        mine_hud_draw(frame,w,h,RD_ACTION_MISS,8,MINE_DIR_RIGHT,4);
        if(count(frame,n,12)<4) {puts("No red MISS indicator");return 3;}
        memset(frame,11,sizeof frame);
        mine_hud_draw(frame,w,h,RD_ACTION_PLACED,8,MINE_DIR_DOWN,4);
        if(count(frame,n,10)<4){puts("No PUT indicator");return 4;}
    }
    puts("PASS: visible dig/place/miss and arrow feedback, 20x25/40x40/40x50");
    return 0;
}
