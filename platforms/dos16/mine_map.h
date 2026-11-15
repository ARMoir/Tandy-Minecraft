/* A tiny map keeps movement intelligible when rendering 3D terrain at 1 FPS. */
#ifndef MINE_MAP_H
#define MINE_MAP_H
#include "../../src/rd.h"
void mine_map_draw(int player_id, rd_u8 *image,int width,int height);
#endif
