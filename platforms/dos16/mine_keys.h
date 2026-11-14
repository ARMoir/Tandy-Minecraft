/* MINE.EXE XT/Tandy keyboard scan set 1, shared by DOS and host tests.
 * 'down' holds the present state; 'edge' latches short make events between
 * slow video frames. No C runtime or DOS headers are needed here.
 */
#ifndef MINE_KEYS_H
#define MINE_KEYS_H
#include "../../src/rd.h"
typedef struct {
    volatile rd_u8 down[128];
    volatile rd_u8 edge[128];
    volatile rd_u8 e0;
    volatile rd_u8 e1_skip;
} mine_keyboard;
typedef struct {
    rd_u8 down[128];
    rd_u8 edge[128];
} mine_snapshot;
typedef struct {
    int quit;
    int quality_cycle;
} mine_commands;
void mine_key_feed(mine_keyboard *state,rd_u8 scan);
void mine_key_snapshot(mine_keyboard *state,mine_snapshot *snapshot);
void mine_key_controls(const mine_snapshot *snapshot,unsigned long ticks,
                      rd_input *input,mine_commands *commands);
#endif
