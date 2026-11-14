/* MINE.EXE - 8086/V20, real-mode DOS, Tandy 1000 160x200x16.
 * Uses the original rdgeneric engine; hardware-specific code lives here.
 * /HI = 80x50 internal raycast (default 40x25), /BENCH = eight frames.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include <conio.h>
#include "../../src/rd.h"

#define SCREEN_WIDTH_BYTES 80
#define RENDER_MAX_W 80
#define RENDER_MAX_H 50
#define BENCH_FRAMES 8

/* All arrays are under 64KB each for Open Watcom's large memory model. */
static rd_u32 rgb[RENDER_MAX_W * RENDER_MAX_H];
static rd_u8 packed[SCREEN_WIDTH_BYTES * RENDER_MAX_H];
static volatile rd_u8 keys[128];
static rd_u8 previous[128];
static void (__interrupt __far *previous_irq1)(void);

static int absi(int x) { return x < 0 ? -x : x; }

/* Approximate RGB to the standard Tandy/PCjr 16-color palette.
 * Done only once per render pixel, not once per scaled display pixel.
 */
static rd_u8 nearest_tandy(rd_u32 rgbval) {
    static const rd_u8 palette[16][3] = {
        {0,0,0},{0,0,170},{0,170,0},{0,170,170},
        {170,0,0},{170,0,170},{170,85,0},{170,170,170},
        {85,85,85},{85,85,255},{85,255,85},{85,255,255},
        {255,85,85},{255,85,255},{255,255,85},{255,255,255}
    };
    int r=(int)((rgbval>>16)&255UL);
    int g=(int)((rgbval>>8)&255UL);
    int b=(int)(rgbval&255UL);
    int i, best=0, best_error=10000;
    for(i=0;i<16;i++) {
        int error=absi(r-palette[i][0])+absi(g-palette[i][1])+absi(b-palette[i][2]);
        if(error<best_error) {best_error=error;best=i;}
    }
    return (rd_u8)best;
}

static void mode_set(unsigned mode) {
    union REGS r;
    r.h.ah=0;
    r.h.al=(unsigned char)mode;
    int86(0x10,&r,&r);
}
static unsigned mode_get(void) {
    union REGS r;
    r.h.ah=0x0F;
    int86(0x10,&r,&r);
    return r.h.al;
}

/* BIOS graphics mode 08h: 160x200 16-color, two 8KB interleaved banks.
 * Each byte is two four-bit pixels, rows split at B800:0000/2000.
 * BIOS initializes the graphics-page mapping on the Tandy 1000.
 */
static void render_screen(int w,int h) {
    rd_u8 __far *video=(rd_u8 __far *)MK_FP(0xB800,0);
    int sx,sy,byte_x,y;
    int replicate=SCREEN_WIDTH_BYTES/w;
    int lines=200/h;
    for(sy=0;sy<h;sy++) {
        unsigned row=(unsigned)sy*SCREEN_WIDTH_BYTES;
        unsigned pixrow=(unsigned)sy*(unsigned)w;
        for(byte_x=0;byte_x<SCREEN_WIDTH_BYTES;byte_x++) {
            sx=byte_x/replicate;
            {
                rd_u8 ink=nearest_tandy(rgb[pixrow+(unsigned)sx]);
                packed[row+(unsigned)byte_x]=(rd_u8)((ink<<4)|ink);
            }
        }
    }
    for(y=0;y<200;y++) {
        unsigned base=(unsigned)((y&1)?0x2000:0)+((unsigned)y/2)*SCREEN_WIDTH_BYTES;
        unsigned src=(unsigned)(y/lines)*SCREEN_WIDTH_BYTES;
        for(byte_x=0;byte_x<SCREEN_WIDTH_BYTES;byte_x++)
            video[base+(unsigned)byte_x]=packed[src+(unsigned)byte_x];
    }
}

/* Read make/break scan codes, including keys held across rendered frames.
 * We own IRQ1 only during play and restore the BIOS handler before exit.
 * The original IBM XT keyboard acknowledge and PIC EOI are required.
 */
static void __interrupt __far mine_irq1(void) {
    unsigned char sc=(unsigned char)inp(0x60);
    unsigned char ctrl=(unsigned char)inp(0x61);
    outp(0x61,ctrl|0x80);
    outp(0x61,ctrl);
    if(sc!=0xE0 && sc!=0xE1)
        keys[sc&0x7F]=(sc&0x80)?0:1;
    outp(0x20,0x20);
}
static int pressed(int scan) {
    return keys[scan] && !previous[scan];
}
static void capture_keys(void) {
    int i;
    for(i=0;i<128;i++) previous[i]=keys[i];
}

static unsigned long timer_ticks(void) {
    union REGS r;
    r.h.ah=0;
    int86(0x1A,&r,&r);
    return ((unsigned long)r.x.cx<<16)|(unsigned long)r.x.dx;
}
static void read_controls(rd_input *in) {
    int i;
    memset(in,0,sizeof(*in));
    in->forward=(keys[0x11]?1.0f:0.0f)-(keys[0x1F]?1.0f:0.0f);
    in->strafe=(keys[0x20]?1.0f:0.0f)-(keys[0x1E]?1.0f:0.0f);
    in->jump=keys[0x39]!=0;
    in->yaw=((keys[0x4D]?1.0f:0.0f)-(keys[0x4B]?1.0f:0.0f))*0.09f;
    in->pitch=((keys[0x48]?1.0f:0.0f)-(keys[0x50]?1.0f:0.0f))*0.07f;
    in->dig=pressed(0x21); /* F */
    in->place=pressed(0x12); /* E */
    in->reset=pressed(0x13); /* R */
    for(i=0;i<7;i++) if(pressed(0x02+i)) in->select=i+1;
}
static void load_world(void) {
    FILE *f=fopen("MINE.WLD","rb");
    if(!f) return;
    if(fread(rd_blocks(),1,(unsigned)rd_world_bytes(),f)==(unsigned)rd_world_bytes())
        rd_relight();
    fclose(f);
}
static void save_world(void) {
    FILE *f=fopen("MINE.WLD","wb");
    if(!f) return;
    fwrite(rd_blocks(),1,(unsigned)rd_world_bytes(),f);
    fclose(f);
}

int main(int argc,char **argv) {
    int hi=0,bench=0,player,frames=0,i,steps;
    unsigned previous_mode;
    unsigned long began,last,now,elapsed,fps10;
    rd_input in;
    for(i=1;i<argc;i++) {
        if(!strcmp(argv[i],"/HI") || !strcmp(argv[i],"/hi")) hi=1;
        if(!strcmp(argv[i],"/BENCH") || !strcmp(argv[i],"/bench")) bench=1;
    }
    printf("MINE - Minecraft for Tandy 1000 EX / V20\n");
    printf("World: 32x24x32, video: Tandy 160x200 16 colors.\n");
    printf("WASD move, arrows look, SPACE jump, F dig, E place, 1-7 blocks, R reset.\n");
    printf("ESC/Q exit, H toggle detail. World saved as MINE.WLD on exit.\n");
    rd_init(132211UL,RD_HILLS);
    player=rd_player_add();
    if(player<0) {puts("No free player slot.");return 1;}
    load_world();
    rd_set_view_distance(12);
    previous_mode=mode_get();
    mode_set(0x08);
    if(mode_get()!=0x08) {
        mode_set(previous_mode);
        puts("Tandy/PCjr BIOS mode 08h unavailable. MINE requires Tandy graphics.");
        return 1;
    }
    previous_irq1=_dos_getvect(0x09);
    _dos_setvect(0x09,mine_irq1);
    began=last=timer_ticks();
    while(!keys[0x01] && !keys[0x10]) { /* ESC or Q */
        int w=hi?80:40, h=hi?50:25;
        now=timer_ticks();
        elapsed=now-last;
        last=now;
        steps=(int)(elapsed*60UL/18UL);
        if(steps<1) steps=1;
        if(steps>12) steps=12;
        if(pressed(0x23)) hi=!hi; /* H */
        read_controls(&in);
        rd_player_input(player,&in);
        for(i=0;i<steps;i++) rd_tick();
        rd_render(player,rgb,w,h,w);
        render_screen(w,h);
        frames++;
        if(bench && frames>=BENCH_FRAMES) break;
        capture_keys();
    }
    _dos_setvect(0x09,previous_irq1);
    mode_set(previous_mode);
    save_world();
    elapsed=timer_ticks()-began;
    if(!elapsed) elapsed=1;
    fps10=(unsigned long)frames*182UL/elapsed;
    printf("MINE %d frames in %lu timer ticks (%lu.%lu fps).\n",
           frames,elapsed,fps10/10UL,fps10%10UL);
    return 0;
}
