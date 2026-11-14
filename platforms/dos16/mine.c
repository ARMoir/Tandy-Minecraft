/* MINE.EXE - 8086/V20, real-mode DOS, Tandy 1000 160x200x16.
 * Uses the original rdgeneric engine; hardware-specific code lives here.
 * /LOW 20x25, normal 40x40, /DETAIL 40x50, /HI 80x50; /BENCH four frames.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include <conio.h>
#include "../../src/rd.h"
#include "../../src/rd_fast.h"

#define SCREEN_WIDTH_BYTES 80
#define RENDER_MAX_W 80
#define RENDER_MAX_H 50
#define BENCH_FRAMES 4

/* All arrays are under 64KB each for Open Watcom's large memory model. */
static rd_u8 indexed[RENDER_MAX_W * RENDER_MAX_H];
static rd_u8 packed[SCREEN_WIDTH_BYTES * RENDER_MAX_H];
static volatile rd_u8 keys[128];
static volatile rd_u8 edges[128]; /* preserve brief presses between slow frames */
static void (__interrupt __far *previous_irq1)(void);

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

/* Show a test pattern immediately, before the first ray is traced.
 * If the user sees this, the Tandy BIOS mode switch and B800 write worked.
 */
static void draw_startup(void) {
    rd_u8 __far *video=(rd_u8 __far *)MK_FP(0xB800,0);
    int y,x;
    for(y=0;y<200;y++) {
        unsigned base=(unsigned)((y&1)?0x2000:0)+((unsigned)y/2)*80U;
        rd_u8 c=(rd_u8)(y<30?0x11:(y<50?0x33:(y<65?0xAA:0x00)));
        for(x=0;x<80;x++) video[base+(unsigned)x]=c;
    }
}
/* Two 80-byte bulk transfers per Tandy odd/even scanline pair,
 * instead of 16,000 far-pointer byte writes per rendered frame.
 */
static void render_screen(int w,int h) {
    rd_u8 __far *even=(rd_u8 __far *)MK_FP(0xB800,0);
    rd_u8 __far *odd=(rd_u8 __far *)MK_FP(0xBA00,0);
    int sy,x,y,repeat=SCREEN_WIDTH_BYTES/w,lines=200/h;
    for(sy=0;sy<h;sy++) {
        unsigned row=(unsigned)sy*SCREEN_WIDTH_BYTES;
        unsigned inrow=(unsigned)sy*(unsigned)w;
        for(x=0;x<SCREEN_WIDTH_BYTES;x++) {
            rd_u8 ink=indexed[inrow+(unsigned)(x/repeat)]&15;
            packed[row+(unsigned)x]=(rd_u8)((ink<<4)|ink);
        }
    }
    for(y=0;y<100;y++) {
        unsigned offset=(unsigned)y*SCREEN_WIDTH_BYTES;
        unsigned row_even=((unsigned)(2*y)/lines)*SCREEN_WIDTH_BYTES;
        unsigned row_odd=((unsigned)(2*y+1)/lines)*SCREEN_WIDTH_BYTES;
        _fmemcpy(even+offset,packed+row_even,SCREEN_WIDTH_BYTES);
        _fmemcpy(odd+offset,packed+row_odd,SCREEN_WIDTH_BYTES);
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
    if(sc!=0xE0 && sc!=0xE1) {
        unsigned code=sc&0x7F;
        if(sc&0x80) keys[code]=0;
        else {
            if(!keys[code]) edges[code]=1;
            keys[code]=1;
        }
    }
    outp(0x20,0x20);
}
static int pressed(int scan) {
    return edges[scan]!=0;
}
static void capture_keys(void) {
    int i;
    /* Clear one-shot makes only after all keys were processed this frame. */
    for(i=0;i<128;i++) edges[i]=0;
}

static unsigned long timer_ticks(void) {
    union REGS r;
    r.h.ah=0;
    int86(0x1A,&r,&r);
    return ((unsigned long)r.x.cx<<16)|(unsigned long)r.x.dx;
}
/* Visual hardware input check, independent of gameplay/render FPS.
 * Shows bright green if a key is held, magenta if a make event is queued.
 * Key bar layout: W A S D Left Right Up Down, in that order.
 */
static void keytest_bars(void) {
    static const int scan[8]={0x11,0x1E,0x1F,0x20,0x4B,0x4D,0x48,0x50};
    rd_u8 __far *even=(rd_u8 __far *)MK_FP(0xB800,0);
    rd_u8 __far *odd=(rd_u8 __far *)MK_FP(0xBA00,0);
    int bar,y,j;
    for(bar=0;bar<8;bar++){
        rd_u8 color=keys[scan[bar]]?0xAA:(edges[scan[bar]]?0xDD:0x11);
        for(y=0;y<10;y++){
            unsigned offset=(unsigned)(bar*12+y)*80U;
            rd_u8 __far *dst=y&1?odd:even; /* diagnostic only, two banks */
            for(j=0;j<42;j++)dst[offset+(unsigned)j]=color;
        }
    }
}
static void read_controls(rd_input *in) {
    int i;
    memset(in,0,sizeof(*in));
    in->forward=((keys[0x11]||edges[0x11])?1.0f:0.0f)-((keys[0x1F]||edges[0x1F])?1.0f:0.0f);
    in->strafe=((keys[0x20]||edges[0x20])?1.0f:0.0f)-((keys[0x1E]||edges[0x1E])?1.0f:0.0f);
    in->jump=keys[0x39]!=0 || edges[0x39]!=0;
    /* Look speeds are scaled in the main loop to real elapsed BIOS ticks.
     * IRQ key edges also count, even if released during the last render.
     */
    in->yaw=(((keys[0x4D]||edges[0x4D])?1.0f:0.0f)-((keys[0x4B]||edges[0x4B])?1.0f:0.0f))*0.06f;
    in->pitch=(((keys[0x48]||edges[0x48])?1.0f:0.0f)-((keys[0x50]||edges[0x50])?1.0f:0.0f))*0.04f;
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
    int quality=1,auto_quality=1,auto_frames=0,bench=0,render_mode=0,keytest=0,player,frames=0,i,steps;
    unsigned previous_mode;
    unsigned long began,last,now,elapsed,fps10,auto_since,setup_start,setup_ticks;
    unsigned long render_ticks=0,video_ticks=0,measured;
    rd_input in;
    for(i=1;i<argc;i++) {
        if(!strcmp(argv[i],"/HI") || !strcmp(argv[i],"/hi")) {quality=3;auto_quality=0;}
        if(!strcmp(argv[i],"/LOW") || !strcmp(argv[i],"/low")) {quality=0;auto_quality=0;}
        if(!strcmp(argv[i],"/DETAIL") || !strcmp(argv[i],"/detail")) {quality=2;auto_quality=0;}
        if(!strcmp(argv[i],"/MID") || !strcmp(argv[i],"/mid")) {quality=1;auto_quality=0;}
        if(!strcmp(argv[i],"/KEYTEST") || !strcmp(argv[i],"/keytest")) keytest=1;
        if(!strcmp(argv[i],"/AUTO") || !strcmp(argv[i],"/auto")) {quality=1;auto_quality=1;}
        if(!strcmp(argv[i],"/BENCH") || !strcmp(argv[i],"/bench")) bench=1;
        if(!strcmp(argv[i],"/OLD") || !strcmp(argv[i],"/old")) render_mode=2;
        if(!strcmp(argv[i],"/FULL") || !strcmp(argv[i],"/full")) render_mode=1;
        if(!strcmp(argv[i],"/TERRAIN") || !strcmp(argv[i],"/terrain")) render_mode=0;
    }
    printf("MINE - Minecraft for Tandy 1000 EX / V20\n");
    printf("World: 32x24x32, video: Tandy 160x200 16 colors.\n");
    printf("WASD move, arrows look, SPACE jump, F dig, E place, 1-7 blocks, R reset.\n");
    printf("ESC/Q quit, H detail, /MID 40x40, /DETAIL 40x50, /KEYTEST.\n");
    if(keytest) puts("KEYTEST: W A S D, then arrow keys. Green bars = held. ESC exits.");
    setup_start=timer_ticks();
    puts("Loading embedded Minecraft world...");
    rd_init(132211UL,RD_HILLS);
    player=rd_player_add();
    if(player<0) {puts("No free player slot.");return 1;}
    if(bench) auto_quality=0; /* fixed resolution for comparable benchmarks */
    if(!bench) load_world();
    rd_set_view_distance(12);
    setup_ticks=timer_ticks()-setup_start;
    printf("World ready after %lu timer ticks (about %lu seconds).\n",
           setup_ticks,setup_ticks/18UL);
    previous_mode=mode_get();
    mode_set(0x08);
    if(mode_get()!=0x08) {
        mode_set(previous_mode);
        puts("Tandy/PCjr BIOS mode 08h unavailable. MINE requires Tandy graphics.");
        return 1;
    }
    draw_startup();
    previous_irq1=_dos_getvect(0x09);
    _dos_setvect(0x09,mine_irq1);
    if(keytest) {
        /* No raycasting or physics. The bars must change immediately. */
        while(!keys[0x01]&&!edges[0x01]){
            keytest_bars();
        }
        _dos_setvect(0x09,previous_irq1);
        mode_set(previous_mode);
        puts("Keyboard test finished; no world changes.");
        return 0;
    }
    began=last=auto_since=timer_ticks();
    while(!keys[0x01] && !keys[0x10] && !edges[0x01] && !edges[0x10]) { /* ESC or Q */
        int w=quality==3?80:(quality==0?20:40);
        int h=quality==0?25:(quality==1?40:50);
        now=timer_ticks();
        elapsed=now-last;
        last=now;
        /* Each physics tick is costly on software-floating V20.
         * Time-based look acceleration prevents camera rotation from
         * slowing down in proportion to FPS; bound long pauses.
         */
        steps=(int)(elapsed*60UL/18UL);
        if(steps<1) steps=1;
        if(steps>4) steps=4;
        if(pressed(0x23)) {quality=(quality+1)%4;auto_quality=0;} /* H */
        read_controls(&in);
        {unsigned long look_ticks=elapsed>8UL?8UL:(elapsed<1UL?1UL:elapsed);
         in.yaw*= (float)look_ticks;
         in.pitch*= (float)look_ticks;}
        /* CRITICAL: consume edges BEFORE rendering. Presses arriving
         * during a long render remain latched for the next frame.
         * Old code cleared them after rendering and silently lost events.
         */
        capture_keys();
        if(bench) {
            float x,y,z,yaw,pitch;
            rd_player_pos(player,&x,&y,&z);
            rd_player_angles(player,&yaw,&pitch);
            rd_player_set(player,x,y,z,yaw+0.12f,pitch);
        } else {
            rd_player_input(player,&in);
            for(i=0;i<steps;i++) rd_tick();
        }
        measured=timer_ticks();
        if(render_mode==2)rd_legacy_render(player,indexed,w,h);
        else if(render_mode==1)rd_fast_render(player,indexed,w,h);
        else rd_terrain_render(player,indexed,w,h);
        render_ticks+=timer_ticks()-measured;
        measured=timer_ticks();
        render_screen(w,h);
        video_ticks+=timer_ticks()-measured;
        frames++;
        /* Default AUTO uses a hysteresis gap to avoid thrashing between
         * resolutions on the 18.2Hz BIOS clock. Never selects slow /HI.
         */
        if(auto_quality) {
            unsigned long sample_end=timer_ticks();
            auto_frames++;
            if(quality==1 && auto_frames>=2) {
                if(sample_end-auto_since>=18UL) quality=0;
                auto_since=sample_end;auto_frames=0;
            } else if(quality==0 && auto_frames>=8) {
                if(sample_end-auto_since<=16UL) quality=1;
                auto_since=sample_end;auto_frames=0;
            }
        }
        if(bench && frames>=BENCH_FRAMES) break;
        if(edges[0x01]||edges[0x10]) break; /* short ESC/Q while rendering */
    }
    _dos_setvect(0x09,previous_irq1);
    mode_set(previous_mode);
    if(!bench) save_world();
    elapsed=timer_ticks()-began;
    if(!elapsed) elapsed=1;
    fps10=(unsigned long)frames*182UL/elapsed;
    printf("MINE %d frames in %lu ticks (%lu.%lu fps).\n",
           frames,elapsed,fps10/10UL,fps10%10UL);
    printf("Setup %lu ticks, renderer %lu ticks, video %lu ticks.\n",
           setup_ticks,render_ticks,video_ticks);
    return 0;
}
