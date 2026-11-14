MINE.EXE - Tandy 1000 EX DOS port
================================
Hardware target: NEC V20 or 8088, DOS, Tandy/PCjr graphics BIOS mode 08h
Original C engine: rdgeneric (Minecraft rd-132211 recreation).

MINE.EXE         Launch 40x25 textured DDA renderer scaled to 160x200x16.
MINE.EXE /LOW    Fastest 20x25 integer voxel renderer.\nMINE.EXE /HI     Detailed 80x50 textured DDA renderer (slower).
MINE.EXE /BENCH  Timed 12-frame camera sweep, bypasses save file.\nMINE.EXE /OLD    Previous simpler renderer for fallback and comparison.

Keyboard
--------
W A S D      Move
Arrow keys   Look/turn
SPACE        Jump
F / E        Dig / place a block
1 through 7  Select a block type
R            Respawn
H            Cycle low/normal/high detail
ESC or Q     Save and quit

MINE.WLD is a raw 32x24x32 block world created on exit in the current
directory. Keep a copy of it before replacing this experimental build.

IMPORTANT: This first build has not been verified on physical Tandy hardware.
The original core's floating-point renderer was too slow to show a first frame\non a 7MHz V20. This port now uses a separate integer ray marcher and writes\na color startup pattern immediately after entering Tandy video mode.\nThe original world, collision, physics, and block interactions remain intact.
Serial-mouse support is not yet implemented.

This build uses Tandy BIOS graphics mode 08h and video memory at B800:0000.
Do not attempt to run it in an ordinary VGA-only DOS machine.

Open Watcom C/16 build (see GitHub Actions for the canonical command):
  cc -std=c99 -O2 -DRD_X=32 -DRD_Y=24 -DRD_Z=32 -DRD_MAX_PLAYERS=1 -o atlasgen tools/make_tandy_atlas.c
  atlasgen src/rd_tandy_atlas.h
  wcl -bcl=dos -0 -ox -ml -zt256 -zastd=c99 -q -dRD_DOS16 -dRD_X=32 \
    -dRD_Y=24 -dRD_Z=32 -dRD_MAX_PLAYERS=1 -fe=MINE.EXE \
    platforms/dos16/mine.c src/rd.c src/rd_fast.c src/rd_legacy.c

The original platforms, tests and source code remain in the repository.
License: see LICENSE.

WolfensteinCGA performance work: 3D voxel boundary stepping rather than
quarter-block probing; per-column/row perspective projection tables;
lookup reciprocal tables; original block textures color-converted on build
host with face shading and ordered dithering; bulk video-memory row copies.
The original 3D game rules, physics and level saving remain in rdgeneric.
Use /LOW /BENCH to establish a baseline on real 7MHz V20 hardware.
