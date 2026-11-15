/* V20 text-mode 3D Minecraft. It samples actual solid voxels along
 * 3D camera rays, then paints an 80x25 DOS text character/attribute image.
 * No polygons, no video graphics, no trigonometry per ray, no floats
 * inside any sampling loop, and only short fixed-point position updates.
 *
 * Ray counts: 20x10, 32x16, 40x20; max 28 half-block samples per ray.
 * Tandy native text mode 03h: B800 80x25x2 = 4000 display bytes.
 * Face orientation determines dirt sides vs green grass tops.
 */
#include "rd_ascii.h"

#define VIEW_ROWS 23
#define MAX_RAY_COLUMNS 40
#define MAX_RAY_ROWS 20
#define MAX_STEPS 28
#define FP 256
static const short sine256[256]={
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
static int div256(int value) {
    /* Division by 256 with floor for negative world-space coordinates,
     * avoiding implementation-defined signed right shift. */
    if(value>=0)return value>>8;
    return -(((-value)+255)>>8);
}
void rd_ascii_write(rd_u8 *cells,int x,int y,const char *str,rd_u8 attr) {
    unsigned off;
    if(!cells||!str||x<0||y<0||x>=80||y>=25)return;
    off=(unsigned)y*160U+(unsigned)x*2U;
    while(*str&&x++<80) {
        cells[off++]=(rd_u8)*str++;
        cells[off++]=attr;
    }
}
static void hit_appearance(int material,int top,int step,rd_u8 *ch,rd_u8 *attr) {
    rd_u8 fg,bg;
    switch(material) {
        case RD_GRASS: if(top) {fg=10;bg=2;} else {fg=14;bg=6;} break;
        case RD_LEAVES: fg=10;bg=2;break;
        case RD_DIRT: fg=14;bg=6;break;
        case RD_LOG: fg=14;bg=6;break;
        case RD_PLANKS: fg=14;bg=6;break;
        case RD_STONE: fg=7;bg=0;break;
        case RD_COBBLE: fg=8;bg=0;break;
        default: fg=7;bg=0;break;
    }
    /* CP437 solid / dense / medium / light shade. Color stays in the
     * original Tandy families. More distant terrain has less visual detail.
     */
    if(step<7)*ch=219;
    else if(step<14)*ch=178;
    else if(step<21)*ch=177;
    else *ch=176;
    *attr=(rd_u8)((bg<<4)|fg);
}
static void cast(const rd_u8 *blocks,int ox,int oy,int oz,
                 int dx,int dy,int dz,rd_u8 *ch,rd_u8 *attr) {
    int x=ox,y=oy,z=oz,prevy=div256(oy);
    int lastx=div256(ox),lastz=div256(oz);
    int step;
    *ch=' ';*attr=0x30; /* cyan sky */
    dx=dx/2;dy=dy/2;dz=dz/2;
    for(step=0;step<MAX_STEPS;step++){
        int bx,by,bz,idx,mat;
        x+=dx;y+=dy;z+=dz;
        bx=div256(x);by=div256(y);bz=div256(z);
        if(bx<0||bx>=RD_X||bz<0||bz>=RD_Z||by<0||by>=RD_Y)break;
        idx=(by*RD_Z+bz)*RD_X+bx;
        mat=(int)blocks[idx];
        if(mat!=RD_AIR){
            /* Sparse leaf holes hint at foliage transparency without
             * texture sampling; limit holes to 1/4 of foliage samples.
             */
            if(mat==RD_LEAVES && ((bx+by+bz+step)&3)==0) {
                lastx=bx;prevy=by;lastz=bz;
                continue;
            }
            hit_appearance(mat,by<prevy,step,ch,attr);
            return;
        }
        lastx=bx;prevy=by;lastz=bz;
    }
    (void)lastx;(void)lastz;
}
static void plot_cell(rd_u8 *dest,int x0,int x1,int y0,int y1,rd_u8 ch,rd_u8 attr) {
    int row,col;
    for(row=y0;row<y1;row++){
        unsigned o=(unsigned)row*160U+(unsigned)x0*2U;
        for(col=x0;col<x1;col++){
            dest[o++]=ch;
            dest[o++]=attr;
        }
    }
}
void rd_ascii_render(int player,rd_u8 *out,int ray_columns,int ray_rows) {
    const rd_u8 *world=rd_blocks();
    float px,py,pz,yaw,pitch;
    int phase,sp,cp,pitch256,ox,oy,oz;
    int col,row,x;
    int left[41],top[21],dx[40],dz[40],dy[20];
    int cw,bearing;
    static const char *directions[4]={"S","E","N","W"};
    if(!out || (ray_columns!=20&&ray_columns!=32&&ray_columns!=40) ||
       (ray_rows!=10&&ray_rows!=16&&ray_rows!=20))return;
    rd_player_pos(player,&px,&py,&pz);
    rd_player_angles(player,&yaw,&pitch);
    ox=(int)(px*256.0f);
    oy=(int)((py+1.62f)*256.0f);
    oz=(int)(pz*256.0f);
    phase=((int)(yaw*40.743665f))&255;
    sp=sine256[phase];
    cp=sine256[(phase+64)&255];
    pitch256=(int)(pitch*256.0f);
    /* Precalculate screen divisions and ray directions ONCE per frame.
     * Actual 3D voxel sampling uses only 16-bit additions, shifts and
     * block-index arithmetic. No floats or divisions in the sample loop.
     */
    for(col=0;col<=ray_columns;col++)left[col]=col*80/ray_columns;
    for(row=0;row<=ray_rows;row++)top[row]=row*VIEW_ROWS/ray_rows;
    for(col=0;col<ray_columns;col++){
        int screen=(2*col+1-ray_columns)*160/ray_columns;
        dx[col]=sp+(int)(((long)cp*(long)screen)>>8);
        dz[col]=cp-(int)(((long)sp*(long)screen)>>8);
    }
    for(row=0;row<ray_rows;row++)
        dy[row]=pitch256+(ray_rows-1-2*row)*20;
    for(row=0;row<ray_rows;row++)for(col=0;col<ray_columns;col++){
        rd_u8 ch,attr;
        cast(world,ox,oy,oz,dx[col],dy[row],dz[col],&ch,&attr);
        plot_cell(out,left[col],left[col+1],top[row],top[row+1],ch,attr);
    }
    /* White aiming reticle, centered in the true 3D view. */
    cw=(unsigned)(VIEW_ROWS/2)*160U+40U*2U;
    out[cw]='+';
    out[cw+1]=0x0F;
    plot_cell(out,0,80,23,25,' ',0x07);
    bearing=((phase+32)&255)>>6;
    rd_ascii_write(out,0,23,"MINE ASCII 3D  V20  80X25 TEXT   FACE: ",0x0F);
    rd_ascii_write(out,38,23,directions[bearing],0x0E);
    rd_ascii_write(out,43,23,
                   ray_columns==20?"20X10 RAYS":ray_columns==32?"32X16 RAYS":"40X20 RAYS",
                   0x0A);
    rd_ascii_write(out,0,24,"WASD MOVE  ARROWS LOOK  F DIG  E PLACE  1-7 BLOCKS  H DETAIL  ESC/Q EXIT",0x07);
}
