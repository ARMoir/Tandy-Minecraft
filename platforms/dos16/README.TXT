MINE.EXE - Minecraft for Tandy 1000 EX
==========================================
Target: 8088 / NEC V20 (7 MHz), DOS, Tandy/PCjr BIOS video mode 08h.
Compiler: Open Watcom C/16, 8086 instructions, large memory model.

RUN:
  MINE.EXE           FAST terrain view, adaptive 40x25 -> 20x25 if slow.
  MINE.EXE /DETAIL   Force clear 40x50 internal image (4x4 pixels).\n  MINE.EXE /LOW      Force fastest 20x25 (8x8 pixels).
  MINE.EXE /HI       Force 80x50 internal texture detail (slowest).
  MINE.EXE /AUTO     Enable adaptive 20x25 / 40x25 mode.
  MINE.EXE /FULL     Full 3D DDA textured voxels (much slower).\n  MINE.EXE /OLD      Earlier quarter-block 3D renderer (slowest).
  MINE.EXE /BENCH    Timed fixed four-frame camera sweep, print FPS.
                     Combine with /LOW, /HI or /OLD; no world load/save.

CONTROLS:
  W/A/S/D       Move
  Arrow keys    Look left/right/up/down
  SPACE         Jump
  F / E         Dig / place block
  1 through 7   Choose block
  R             Respawn
  H             Manual quality cycling (disables adaptive mode)
  ESC or Q      Save world and quit

MINE.WLD is a raw 32x24x32 world in the current directory (24,576 bytes).
The game loads this file at startup and saves when you quit normally.
Make a backup before testing experimental builds. /BENCH never overwrites it.

The native DOS renderer:
- Uses true 3D grid-boundary voxel ray traversal, not repeated quarter-cell
  sampling. It preserves Minecraft's real solid voxel shapes.
- Uses original rdgeneric procedural 16x16 stone/grass/dirt/wood/leaves
  textures, palette-converted/dithered and pre-shaded by the host build.
- Uses per-column and per-row perspective tables, integer reciprocals,
  and 8.8 fixed-point math in the inner ray loop.
- Copies prepared 16-color Tandy scanlines with optimized far-memory
  bulk operations to the B800/BA00 graphics banks.
- Leaves the original world generation, digging, placing, and physics.
- Excludes unused original RGB texture generation and RGB renderer from
  the DOS binary. /OLD is still available as a fallback renderer.

TESTING:
This version has passed Open Watcom compilation, host renderer tests and
release verification. Physical 7 MHz V20 frame rates remain unverified.
Run MINE.EXE /LOW /BENCH first if normal gameplay still feels slow.
If the screen shows colored bands but no world, it is in graphics mode and
still rendering. The first banded test pattern is intentional.

BUILD:
An up-to-date Linux/Windows host with C99 compiler and Open Watcom 2:
  cc -std=c99 -O2 -DRD_X=32 -DRD_Y=24 -DRD_Z=32 -DRD_MAX_PLAYERS=1 -o atlasgen tools/make_tandy_atlas.c
  atlasgen src/rd_tandy_atlas.h
  wcl -bcl=dos -0 -ox -ml -zt256 -zastd=c99 -q -dRD_DOS16 -dRD_X=32 -dRD_Y=24 -dRD_Z=32 -dRD_MAX_PLAYERS=1 -fe=MINE.EXE platforms/dos16/mine.c src/rd.c src/rd_fast.c src/rd_terrain.c src/rd_legacy.c

See .github/workflows/mine-dos.yml for the canonical automated build.
The original rdgeneric platforms are preserved. License: see LICENSE.

Startup improvement: the 32x24x32 default world is procedurally generated
on the CI host, run-length compressed and EMBEDDED inside MINE.EXE.
No external data file is required. Saves in MINE.WLD override the default
world during normal play. /BENCH always uses the embedded world.

CRITICAL PERFORMANCE UPDATE:
The default uses a Wolfenstein/Comanche-style terrain-column renderer.
It uses one horizontal ray per screen column to project real Minecraft
column heights and original preconverted block textures. This is an
APPROXIMATE 2.5D landscape: caves, overhangs and underside faces do not
render correctly; use /FULL for genuine 3D voxel views. Digging and placing
blocks still use the original voxel world and physics, and visual edits to
the top surface are visible. /FULL retains the genuine 3D DDA for later
optimization and graphics comparisons. The original source art is kept in
both paths. The renderer choice is configurable with /TERRAIN, /FULL, /OLD.

Input and benchmark improvements:
- Hardware keyboard make events are latched until the next frame so a quick
  dig/place/quality/exit press is not lost during slow rendering.
- /BENCH now runs four frames and reports separate setup, raycasting and
  video output BIOS timer ticks. BENCH camera motion is AUTOMATIC: it does
  not respond to movement keys. Run MINE.EXE /LOW for interactive testing.
- Startup prints "World ready" and elapsed ticks before graphics mode.

GRAPHICS PASS: terrain mode now distinguishes original textured grass top
surfaces, dirt/stone faces on steeper terrain and leafy canopies/logs.
Former 20x25 view stretched pixels to 8x8 and often showed repetitive
stone columns with green noise. Default is now 40x50 for legibility;
/LOW remains available, and adaptive quality can switch there if needed.
This costs some FPS but avoids the multi-minute true 3D renderer.
For a fair test use MINE.EXE /DETAIL /BENCH and MINE.EXE /LOW /BENCH.
