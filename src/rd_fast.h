/* Integer voxel renderer for 16-bit DOS; independent of graphics hardware. */
#ifndef RD_FAST_H
#define RD_FAST_H
#include "rd.h"
/* Produces one 4-bit Tandy color index per internal output pixel. */
void rd_fast_render(int id, rd_u8 *pixels, int w, int h);
#endif
