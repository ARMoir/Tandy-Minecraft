/* Tiny high-contrast action and direction indicators for Tandy mode 08h.
 * Positive edit -> green DIG/PUT, failure -> red MISS/BLOCK. Camera arrows
 * -> yellow UP/DOWN/LEFT/RIGHT. Background is black to stay readable even
 * over textured grass. Unlike the earlier UI, this shows if F/E actually
 * changed a block in the underlying rdgeneric voxel world.
 */
#include "mine_hud.h"
static const unsigned short glyphs[26]={
    0x2bed, 0x6bae, 0x3923, 0x6b6e, 0x79a7, 0x79a4, 0x396b, 0x5bed, 0x7497, 0x126a, 0x5bad, 0x4927, 0x5fed, 0x5ffd, 0x2b6a, 0x6ba4, 0x2b59, 0x6bad, 0x388e, 0x7492, 0x5b6f, 0x5b6a, 0x5bfd, 0x5aad, 0x5a92, 0x72a7
};
static void glyph(rd_u8 *im,int w,int h,int x,int y,char ch,rd_u8 ink){
    unsigned short bits;
    int i,j;
    if(ch<'A'||ch>'Z')return;
    bits=glyphs[ch-'A'];
    for(j=0;j<5;j++)for(i=0;i<3;i++){
        int xx=x+i,yy=y+j;
        if(xx>=0&&xx<w&&yy>=0&&yy<h&&(bits&(1U<<(14-(j*3+i)))))
            im[(unsigned)yy*(unsigned)w+(unsigned)xx]=ink;
    }
}
static void write_text(rd_u8 *im,int w,int h,int y,const char *message,
                       rd_u8 ink){
    int len=0,i,x,xx,yy,rect_w;
    while(message[len]&&len<8)len++;
    rect_w=len*4+1;
    if(rect_w>w)rect_w=w;
    for(yy=y-1;yy<y+6&&yy<h;yy++)if(yy>=0)
        for(xx=0;xx<rect_w;xx++)im[(unsigned)yy*w+(unsigned)xx]=0;
    x=1;
    for(i=0;i<len;i++,x+=4)glyph(im,w,h,x,y,message[i],ink);
}
void mine_hud_draw(rd_u8 *im,int w,int h,
                   int action,int action_frames,int direction,int direction_frames){
    const char *message="";
    const char *heading="";
    rd_u8 action_ink=10;
    int cx=w/2,cy=h/2;
    if(!im||w<20||h<24)return;
    if(action_frames>0) {
        switch(action){
            case 1:message="DIG";break;
            case 2:message="PUT";break;
            case 3:message="MISS";action_ink=12;break;
            case 4:message="BLOCK";action_ink=12;break;
        }
        if(message[0]) {
            write_text(im,w,h,2,message,action_ink);
            /* A very visible crosshair flash after an attempted edit. */
            im[(unsigned)cy*w+(unsigned)cx]=(rd_u8)(action_ink==12?12:14);
        }
    }
    if(direction_frames>0) {
        switch(direction) {
            case MINE_DIR_UP:heading="UP";break;
            case MINE_DIR_DOWN:heading="DOWN";break;
            case MINE_DIR_LEFT:heading="LEFT";break;
            case MINE_DIR_RIGHT:heading="RIGHT";break;
            case MINE_DIR_W:heading="W";break;
            case MINE_DIR_A:heading="A";break;
            case MINE_DIR_S:heading="S";break;
            case MINE_DIR_D:heading="D";break;
        }
        if(heading[0]){
            /* LOW is only 25 high: fit all indicators without overlap. */
            write_text(im,w,h,10,heading,14);
        }
    }
}
