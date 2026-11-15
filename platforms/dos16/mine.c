/* MINE.EXE - 8086/V20, real-mode DOS, Tandy 1000 320x200x16.
 * Uses the original rdgeneric engine; hardware-specific code lives here.
 * /LOW 80x100, normal 160x100, /DETAIL 160x200, /MAX 320x200; /BENCH four frames.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include <conio.h>
#include "../../src/rd.h"
#include "../../src/rd_fast.h"
#include "mine_keys.h"
#include "mine_hud.h"
#include "mine_map.h"

#define SCREEN_WIDTH_BYTES 160
#define RENDER_MAX_W 320
#define RENDER_MAX_H 200
#define BENCH_FRAMES 4

/* All arrays are under 64KB each for Open Watcom's large memory model. */
static rd_u8 indexed[RENDER_MAX_W * RENDER_MAX_H];
static rd_u8 packed[SCREEN_WIDTH_BYTES * RENDER_MAX_H];
static int video_valid;
static mine_keyboard keyboard;
static mine_snapshot snapshot;
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
        unsigned base=(unsigned)((unsigned)y&3U)*8192U+((unsigned)y/4)*160U;
        rd_u8 c=(rd_u8)(y<30?0x11:(y<50?0x33:(y<65?0xAA:0x00)));
        for(x=0;x<160;x++) video[base+(unsigned)x]=c;
    }
}
/* Mode 09h: 320x200x16, four 8KB banks, two distinct pixels per byte.
 * Pack once per logical row, replicate with bulk copies, skip unchanged rows.
 * Offline palette conversion + packed bulk writes follow WolfensteinCGA's
 * approach; this implementation is independent and handles four video banks. */
static void render_screen(int w,int h) {
    rd_u8 row[SCREEN_WIDTH_BYTES];
    rd_u8 __far *video=(rd_u8 __far *)MK_FP(0xB800,0);
    int sy,x,r,repeat=320/w,lines=200/h;
    for(sy=0;sy<h;sy++) {
        unsigned input=(unsigned)sy*(unsigned)w;
        for(x=0;x<160;x++) {
            int x0=2*x/repeat,x1=(2*x+1)/repeat;
            row[x]=(rd_u8)((indexed[input+x0]<<4)|indexed[input+x1]);
        }
        for(r=0;r<lines;r++) {
            unsigned y=(unsigned)(sy*lines+r),offset=y*160U;
            if(!video_valid||memcmp(packed+offset,row,160)!=0) {
                memcpy(packed+offset,row,160);
                _fmemcpy(video+(y&3U)*8192U+(y>>2)*160U,row,160);
            }
        }
    }
    video_valid=1;
}
/* MINE.PIC is baked from the actual default world by the host renderer.
 * /ART displays native 320x200 art with zero world generation/rendering. */
static int show_art(void) {
    FILE *f=fopen("MINE.PIC","rb");
    unsigned old,bank;
    rd_u8 __far *video=(rd_u8 __far *)MK_FP(0xB800,0);
    if(!f){puts("MINE.PIC missing. Extract the complete release ZIP.");return 1;}
    old=mode_get();mode_set(0x09);
    if(mode_get()!=0x09){fclose(f);mode_set(old);return 1;}
    for(bank=0;bank<4;bank++) {
        if(fread(packed,1,8192,f)!=8192){fclose(f);mode_set(old);return 1;}
        _fmemcpy(video+bank*8192U,packed,8192);
    }
    fclose(f);getch();mode_set(old);return 0;
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
    mine_key_feed(&keyboard,sc);
    outp(0x20,0x20);
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
    /* Eight rows from top to bottom:
     * W A S D LEFT RIGHT UP DOWN. Draw directly in the two interlaced
     * Tandy banks; don't accidentally alternate at the wrong offset.
     */
    static const unsigned char scan[8]={0x11,0x1E,0x1F,0x20,0x4B,0x4D,0x48,0x50};
    rd_u8 __far *video=(rd_u8 __far *)MK_FP(0xB800,0);
    int bar,y,x;
    for(bar=0;bar<8;bar++) {
        rd_u8 color=keyboard.down[scan[bar]]?0xAA:
                    (keyboard.edge[scan[bar]]?0xDD:0x11);
        for(y=6+bar*23;y<21+bar*23;y++) {
            unsigned off=((unsigned)y&3U)*8192U+((unsigned)y>>2)*160U;
            for(x=16;x<144;x++)video[off+(unsigned)x]=color;
        }
    }
}

static void sample_keys(void) {
    /* Snapshot + consume at the START of a frame, not the end. Any IRQ
     * arrivals during rendering survive for the next frame.
     */
    _disable();
    mine_key_snapshot(&keyboard,&snapshot);
    _enable();
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
    int quality=1,auto_quality=0,auto_frames=0,bench=0,render_mode=3,keytest=0,player,frames=0,i,steps;
    int hud_action=RD_ACTION_NONE,hud_action_frames=0,hud_direction=MINE_DIR_NONE,hud_direction_frames=0;
    unsigned previous_mode,last_revision=0;
    int last_w=0,last_h=0,have_frame=0,last_hud=0;
    float last_x=0,last_y=0,last_z=0,last_yaw=0,last_pitch=0;
    unsigned long began,last,now,elapsed,fps10,auto_since,setup_start,setup_ticks;
    unsigned long render_ticks=0,video_ticks=0,measured;
    rd_input in;
    mine_commands commands;
    for(i=1;i<argc;i++) {
        if(!strcmp(argv[i],"/ART") || !strcmp(argv[i],"/art")) return show_art();
        if(!strcmp(argv[i],"/MAX") || !strcmp(argv[i],"/max")) {quality=3;auto_quality=0;}
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
    printf("World: 32x24x32, video: Tandy 320x200 16 colors.\n");
    printf("WASD move, arrows LEFT/RIGHT turn UP/DOWN look, SPACE jump.\n");
    printf("F dig, E place, 1-7 blocks, R reset view, H detail, ESC/Q exit.\n");
    printf("HUD: DIG/PUT=world changed; MISS=no target; BLOCK=can't place.\n");
    printf("Map: bottom-right, white=you, yellow=facing; /LOW hides map.\n");
    printf("ESC/Q quit, H detail, /MID 160x100, /MAX 320x200, /ART prebuilt scene.\n");
    if(keytest) puts("KEYTEST rows: W A S D LEFT RIGHT UP DOWN. Green=held, pink=tapped.");
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
    mode_set(0x09);
    if(mode_get()!=0x09) {
        mode_set(previous_mode);
        puts("Tandy/PCjr BIOS mode 09h unavailable. MINE requires Tandy graphics.");
        return 1;
    }
    draw_startup();
    previous_irq1=_dos_getvect(0x09);
    _dos_setvect(0x09,mine_irq1);
    if(keytest) {
        /* No raycasting or physics. The bars must change immediately. */
        while(!keyboard.down[0x01]&&!keyboard.edge[0x01]) {
            keytest_bars();
        }
        _dos_setvect(0x09,previous_irq1);
        mode_set(previous_mode);
        puts("Keyboard test finished; no world changes.");
        return 0;
    }
    began=last=auto_since=timer_ticks();
    while(!keyboard.down[0x01] && !keyboard.down[0x10] &&
          !keyboard.edge[0x01] && !keyboard.edge[0x10]) {
        int w=render_mode==3?(quality==3?320:(quality==0?80:160)):
                              (quality==3?80:(quality==0?20:40));
        int h=render_mode==3?(quality>=2?200:100):
                              (quality==0?25:(quality==1?40:50));
        now=timer_ticks();
        elapsed=now-last;
        if(!elapsed&&have_frame&&!bench)continue;
        last=now;
        /* Each physics tick is costly on software-floating V20.
         * Time-based look acceleration prevents camera rotation from
         * slowing down in proportion to FPS; bound long pauses.
         */
        steps=(int)(elapsed*60UL/18UL);
        if(steps<1) steps=1;
        if(steps>4) steps=4;
        sample_keys();
        mine_key_controls(&snapshot,elapsed,&in,&commands);
        if(commands.quit) break;
        if(commands.quality_cycle) {quality=(quality+1)%4;auto_quality=0;}
        /* Report ALL arrow keys and WASD even if view changes are subtle.
         * Give short presses time to remain visible at ~1 FPS.
         */
        if(in.pitch>0)hud_direction=MINE_DIR_UP;
        else if(in.pitch<0)hud_direction=MINE_DIR_DOWN;
        else if(in.yaw>0)hud_direction=MINE_DIR_RIGHT;
        else if(in.yaw<0)hud_direction=MINE_DIR_LEFT;
        else if(in.forward>0)hud_direction=MINE_DIR_W;
        else if(in.forward<0)hud_direction=MINE_DIR_S;
        else if(in.strafe>0)hud_direction=MINE_DIR_D;
        else if(in.strafe<0)hud_direction=MINE_DIR_A;
        else hud_direction=MINE_DIR_NONE;
        if(hud_direction!=MINE_DIR_NONE)hud_direction_frames=3;
        else if(hud_direction_frames>0)hud_direction_frames--;
        if(bench) {
            float x,y,z,yaw,pitch;
            rd_player_pos(player,&x,&y,&z);
            rd_player_angles(player,&yaw,&pitch);
            rd_player_set(player,x,y,z,yaw+0.12f,pitch);
        } else {
            rd_player_input(player,&in);
            for(i=0;i<steps;i++) rd_tick();
        }
        /* A stationary scene needs no redraw. Poll keys/advance physics,
         * then draw only changed camera/world/HUD state. Short taps therefore
         * don't wait behind another identical expensive frame. */
        {
            float px,py,pz,yaw,pitch;
            unsigned revision=rd_world_revision();
            rd_player_pos(player,&px,&py,&pz);
            rd_player_angles(player,&yaw,&pitch);
            if(!bench&&have_frame&&w==last_w&&h==last_h&&
               revision==last_revision&&px==last_x&&py==last_y&&pz==last_z&&
               yaw==last_yaw&&pitch==last_pitch&&
               !last_hud&&hud_action_frames==0&&hud_direction_frames==0&&
               !in.dig&&!in.place&&!in.select)continue;
            last_x=px;last_y=py;last_z=pz;last_yaw=yaw;last_pitch=pitch;
            last_revision=revision;last_w=w;last_h=h;have_frame=1;
        }
        measured=timer_ticks();
        if(render_mode==2)rd_legacy_render(player,indexed,w,h);
        else if(render_mode==1)rd_fast_render(player,indexed,w,h);
        else if(render_mode==0)rd_terrain_render(player,indexed,w,h);
        else rd_faces_render(player,indexed,w,h);
        /* Actual core mutation result, never merely a claimed key press. */
        if(!bench) {
            int result=rd_take_action(player);
            if(result!=RD_ACTION_NONE){hud_action=result;hud_action_frames=8;}
            else if(hud_action_frames>0)hud_action_frames--;
            /* Minimap visualizes BOTH strafing and turning in real world
             * coordinates. Hidden in /LOW to preserve the tiny view.
             */
            mine_map_draw(player,indexed,w,h);
            mine_hud_draw(indexed,w,h,hud_action,hud_action_frames,
                          hud_direction,hud_direction_frames);
        }
        last_hud=(hud_action_frames>0||hud_direction_frames>0);
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
        if(keyboard.edge[0x01]||keyboard.edge[0x10]) break; /* ESC/Q during render */
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
