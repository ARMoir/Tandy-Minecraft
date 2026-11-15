/* Tandy EX dedicated arrows differ from IBM keypad arrows. E0-prefixed
 * enhanced-keyboard arrows are accepted too, as are legacy keypad arrows.
 * The DOS IRQ handler is responsible for ACK and PIC EOI.
 */
#include "mine_keys.h"
void mine_key_feed(mine_keyboard *s,rd_u8 scan) {
    unsigned code;
    if(s->e1_skip) {s->e1_skip--;return;}
    if(scan==0xE1) {s->e1_skip=5;s->e0=0;return;}
    if(scan==0xE0) {s->e0=1;return;}
    code=scan&0x7FU;
    /* Native 90-key Tandy EX/HX: UP=29 LEFT=2B DOWN=4A RIGHT=4E.
     * BIOS translates these to IBM arrow codes, but our IRQ reads raw bytes.
     * Normalize make AND break before latching. Keep IBM/E0 arrows too.
     * Source: Tandy technical manuals; verified by Adrian Black's adapter:
     * github.com/misterblack1/ps2-to-tandy1000-keyboard */
    if(!s->e0) {
        if(code==0x29)code=0x48;
        else if(code==0x2B)code=0x4B;
        else if(code==0x4A)code=0x50;
        else if(code==0x4E)code=0x4D;
    }
    s->e0=0;
    if(code>=128)return;
    if(scan&0x80U) s->down[code]=0;
    else {
        if(!s->down[code])s->edge[code]=1;
        s->down[code]=1;
    }
}
void mine_key_snapshot(mine_keyboard *s,mine_snapshot *snapshot) {
    unsigned i;
    for(i=0;i<128;i++) {
        snapshot->down[i]=s->down[i];
        snapshot->edge[i]=s->edge[i];
        s->edge[i]=0;
    }
}
static int active(const mine_snapshot *s,unsigned scan) {
    return s->down[scan]!=0||s->edge[scan]!=0;
}
static int amount(const mine_snapshot *s,unsigned scan,int ticks) {
    return s->down[scan]?ticks:(s->edge[scan]?1:0);
}
void mine_key_controls(const mine_snapshot *s,unsigned long ticks,
                       rd_input *input,mine_commands *cmd) {
    int i,scale;
    /* Apply look movement based on elapsed time, not rendered frame count.
     * A released tap earns just one step, never a full frame's wait time.
     */
    scale=ticks>8UL?8:(ticks<1UL?1:(int)ticks);
    input->forward=(float)(active(s,0x11)-active(s,0x1F));
    input->strafe=(float)(active(s,0x20)-active(s,0x1E));
    input->jump=active(s,0x39);
    input->yaw=(float)(amount(s,0x4D,scale)-amount(s,0x4B,scale))*0.10f;
    input->pitch=(float)(amount(s,0x48,scale)-amount(s,0x50,scale))*0.09f;
    input->dig=s->edge[0x21]!=0;
    input->place=s->edge[0x12]!=0;
    input->reset=s->edge[0x13]!=0;
    input->select=0;
    for(i=0;i<7;i++)if(s->edge[0x02+i])input->select=i+1;
    cmd->quality_cycle=s->edge[0x23]!=0;
    cmd->quit=active(s,0x01)||active(s,0x10);
}
