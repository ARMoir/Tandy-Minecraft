MINE.EXE - Tandy 1000 EX DOS port
================================
Hardware target: NEC V20 or 8088, DOS, Tandy/PCjr graphics BIOS mode 08h
Original C engine: rdgeneric (Minecraft rd-132211 recreation).

MINE.EXE         Launch 40x25 internal 3-D renderer scaled to 160x200x16.
MINE.EXE /HI     Launch 80x50 internal 3-D renderer (4x the raycast work).
MINE.EXE /BENCH  Render eight frames then print measured FPS.

Keyboard
--------
W A S D      Move
Arrow keys   Look/turn
SPACE        Jump
F / E        Dig / place a block
1 through 7  Select a block type
R            Respawn
H            Toggle low/high render detail
ESC or Q     Save and quit

MINE.WLD is a raw 32x24x32 block world created on exit in the current
directory. Keep a copy of it before replacing this experimental build.

IMPORTANT: This first build has not been verified on physical Tandy hardware.
It retains the C core's software-floating-point voxel renderer. A 7MHz V20
has no FPU, so the benchmark may reveal very slow rendering. Fixed-point
ray traversal and faster Tandy pixel conversion are the next optimizations.
Serial-mouse support is not yet implemented.

This build uses Tandy BIOS graphics mode 08h and video memory at B800:0000.
Do not attempt to run it in an ordinary VGA-only DOS machine.

Open Watcom C/16 build (see GitHub Actions for the canonical command):
  wcl -bcl=dos -0 -ml -zt256 -zastd=c99 -q -dRD_DOS16 -dRD_X=32 \
    -dRD_Y=24 -dRD_Z=32 -dRD_MAX_PLAYERS=1 -fe=MINE.EXE \
    platforms/dos16/mine.c src/rd.c

The original platforms, tests and source code remain in the repository.
License: see LICENSE.
