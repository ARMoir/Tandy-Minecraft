MINE.EXE - Minecraft for Tandy 1000 EX
==========================================
Target: 8088 / NEC V20 (7 MHz), DOS, Tandy/PCjr BIOS video mode 08h.
Compiler: Open Watcom C/16, 8086 instructions, large memory model.

RUN:
  MINE.EXE           Default adaptive quality: 40x25 -> 20x25 if slow.
  MINE.EXE /LOW      Force fastest 20x25, expanded to 160x200.
  MINE.EXE /HI       Force 80x50 internal texture detail (slowest).
  MINE.EXE /AUTO     Enable adaptive 20x25 / 40x25 mode.
  MINE.EXE /OLD      Use previous quarter-block integer renderer.
  MINE.EXE /BENCH    Timed fixed 12-frame camera sweep, print FPS.
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
  wcl -bcl=dos -0 -ox -ml -zt256 -zastd=c99 -q -dRD_DOS16 -dRD_X=32 -dRD_Y=24 -dRD_Z=32 -dRD_MAX_PLAYERS=1 -fe=MINE.EXE platforms/dos16/mine.c src/rd.c src/rd_fast.c src/rd_legacy.c

See .github/workflows/mine-dos.yml for the canonical automated build.
The original rdgeneric platforms are preserved. License: see LICENSE.
