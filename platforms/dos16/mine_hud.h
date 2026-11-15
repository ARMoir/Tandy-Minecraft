/* Tandy HUD: compact 3x5 font and truthful gameplay action feedback.
 * Runs on indexed 20x25, 40x40, 40x50 or 80x50 framebuffers.
 */
#ifndef MINE_HUD_H
#define MINE_HUD_H
#include "../../src/rd.h"
enum { MINE_DIR_NONE=0, MINE_DIR_UP, MINE_DIR_DOWN, MINE_DIR_LEFT,
       MINE_DIR_RIGHT, MINE_DIR_W, MINE_DIR_A, MINE_DIR_S, MINE_DIR_D };
void mine_hud_draw(rd_u8 *image,int width,int height,
                   int action,int action_frames,int direction,int direction_frames);
#endif
