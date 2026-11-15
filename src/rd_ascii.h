/* True 3D DOS text-mode voxel ray sampler (no graphics or polygon path).
 * 80x25 cell/attribute buffer, CP437 block glyphs, integer ray samples.
 */
#ifndef RD_ASCII_H
#define RD_ASCII_H
#include "rd.h"
#define RD_ASCII_COLUMNS 80
#define RD_ASCII_ROWS 25
#define RD_ASCII_FRAME_BYTES 4000
void rd_ascii_render(int player,rd_u8 *cells,int ray_columns,int ray_rows);
void rd_ascii_write(rd_u8 *cells,int x,int y,const char *text,rd_u8 attr);
#endif
