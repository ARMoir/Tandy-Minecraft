/* Integer voxel renderer for 16-bit DOS; independent of graphics hardware. */
#ifndef RD_FAST_H
#define RD_FAST_H
#include "rd.h"
/* Produces one 4-bit Tandy color index per internal output pixel. */
void rd_fast_render(int id, rd_u8 *pixels, int w, int h);
void rd_terrain_render(int id, rd_u8 *pixels, int w, int h);
void rd_faces_render(int id, rd_u8 *pixels, int w, int h);
void rd_faces_render_near(int id, rd_u8 *pixels, int w, int h);
void rd_terrain_render_far(int id, rd_u8 *pixels, int w, int h);
void rd_hybrid_render(int id, rd_u8 *pixels, int w, int h);
rd_u8 rd_fast_texel(int block,int face,int index);
void rd_legacy_render(int id, rd_u8 *pixels, int w, int h);
#endif
