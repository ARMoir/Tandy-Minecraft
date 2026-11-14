/* Hardware-independent XT/Tandy scan-code and control-contract tests.
 * Test make/break, held, tap, extended arrows and every advertised key.
 */
#include <stdio.h>
#include <string.h>
#include "../platforms/dos16/mine_keys.h"
#define OK(expr) do{if(!(expr)){fprintf(stderr,"key test failed line %d: %s\n",__LINE__,#expr);return 1;}}while(0)
static mine_keyboard keys;
static mine_snapshot snap;
static rd_input input;
static mine_commands cmd;
static void event(unsigned char scan){mine_key_feed(&keys,scan);}
static void check(unsigned long ticks){
    mine_key_snapshot(&keys,&snap);
    mine_key_controls(&snap,ticks,&input,&cmd);
}
static void release(unsigned char scan){event(scan|0x80U);}
int main(void){
    int k;
    check(1);OK(!input.forward&&!input.strafe&&!input.yaw&&!input.pitch);
    event(0x11);check(4);OK(input.forward==1.0f&&!input.strafe);
    check(4);OK(input.forward==1.0f);
    release(0x11);check(4);OK(input.forward==0.0f);
    event(0x1f);check(1);OK(input.forward<0);release(0x1f);check(1);
    event(0x1e);check(1);OK(input.strafe<0);release(0x1e);check(1);
    event(0x20);check(1);OK(input.strafe>0);release(0x20);check(1);
    event(0xE0);event(0x4D);check(6);OK(input.yaw>0.55f&&input.yaw<0.65f);
    event(0xE0);event(0xCD);check(6);OK(input.yaw==0);
    event(0x4B);event(0xCB);check(8);
    OK(input.yaw<0&&input.yaw> -0.12f); /* Short LEFT taps not lost. */
    event(0xE0);event(0x48);check(6);OK(input.pitch>0.50f);
    event(0xE0);event(0xC8);check(1);OK(!input.pitch);
    event(0x50);event(0xD0);check(1);OK(input.pitch<0);
    event(0x39);event(0xB9);check(5);OK(input.jump);check(1);OK(!input.jump);
    event(0x21);event(0xA1);check(1);OK(input.dig==1);check(1);OK(!input.dig);
    event(0x12);event(0x92);check(1);OK(input.place==1);
    event(0x13);event(0x93);check(1);OK(input.reset==1);
    event(0x23);event(0xA3);check(1);OK(cmd.quality_cycle);
    check(1);OK(!cmd.quality_cycle);
    for(k=0;k<7;k++){
        event((unsigned char)(0x02+k));check(1);OK(input.select==k+1);
        release((unsigned char)(0x02+k));check(1);
    }
    event(0x10);event(0x90);check(1);OK(cmd.quit);
    check(1);OK(!cmd.quit);
    event(0x01);check(1);OK(cmd.quit);
    puts("PASS: WASD, E0 arrows, legacy arrows, sky pitch, tap/hold, SPACE F E R H 1-7 Q ESC");
    return 0;
}
