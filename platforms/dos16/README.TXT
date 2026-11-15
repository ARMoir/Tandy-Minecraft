MINE.EXE - TANDY 1000 EX / V20 / 16-BIT DOS
==============================================
Experimental HYBRID renderer (2 FPS real-hardware target).
Video mode 09h: 320x200, 16 colors. Native 8086 machine code.

DEFAULT    Hybrid: original shaded/textured cubes nearby; compact scenery
           farther away. Uses internal 80x50 pixels for speed.
 /LOW      Hybrid 80x50 (fastest 3D cubes)
 /MID      Hybrid 80x100 (sharper vertical definition)
 /DETAIL   Hybrid 160x100 (slower)
 /MAX      Hybrid 160x200 (significantly slower)
 /HYBRID   Explicitly choose the hybrid renderer
 /FACES    Preserve Build #30 full-radius detailed face rendering
 /TERRAIN  Earlier 2.5D landscape (does not show nearby cube faces)
 /FULL     Older full 3D voxel DDA renderer
 /OLD      First integer ray marcher
 /AUTO     Auto-select MID/LOW when frame times rise
 /BENCH    Four rotating frames, then DOS timing stats. No save.
 /ART      Display pre-generated high-res scene from MINE.PIC; press key.
 /KEYTEST  Independent keyboard test
 /CAPTURE  Full-face renderer comparison; writes MINE.RAW

WASD forward/back/strafe; arrows rotate/look; SPACE jump; F dig, E place,
1-7 select block; R respawn; H cycle detail; ESC/Q save and exit.
MINE.WLD loads automatically when present and is saved at normal exit.
The minimap and DIG/PUT/MISS/BLOCK feedback remain available.

HYBRID ARCHITECTURE:
1. 40x25 coarse distant terrain using original 16-color textures,
   cached until quantized camera orientation or position/world changes.
2. A small number of Manhattan shells of true textured, clipped 3D
   blocks are drawn over that scene. Near edits/holes stay voxel-accurate.
3. Video writes are skipped for scanlines identical to the last frame.
4. World logic, collisions and save format are unchanged.
5. /FACES selects the previous full-face renderer for visual comparison.

LIMITATIONS:
- Distant foliage/hills are approximate; caves and overhangs are true 3D
  only nearby. Quantized far scenery can step during fast turning.
- Blocky 80x50 is intentional, not equivalent to full 320x200 artwork.
- 2 FPS HAS NOT been confirmed on a real 7 MHz V20.
- This is a measured experiment, not a production-ready Minecraft port.

Recommended test: MINE /LOW /BENCH, then MINE /MID /BENCH,
followed by MINE /LOW for gameplay responsiveness.
