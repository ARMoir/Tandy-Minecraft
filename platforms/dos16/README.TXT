MINE - Tandy 1000 EX / NEC V20 / 16-bit DOS

VISUAL RESCUE BUILD
Video: native Tandy BIOS mode 09h, 320x200, 16 colors.
Default: textured voxel faces at 160x100, enlarged 2x2 for the screen.
Unlike the old heightfield, this draws real block sides, openings and undersides.

MINE /ART     Show pre-generated 320x200 scene; press a key to exit.
              Requires MINE.PIC from this ZIP. Does not load/save your world.
MINE /MAX     Live 320x200 rendering. Highest detail; speed unverified on V20.
MINE /HI      Alias for /MAX.
MINE /DETAIL  Live 160x200 rendering.
MINE /MID     Live 160x100 rendering (default).
MINE /LOW     Live 80x100 rendering.
MINE /AUTO    Explicit adaptive quality; may switch MID to LOW.
MINE /BENCH   Four camera frames; reports setup/render/video timing; no save.
MINE /KEYTEST Independent keyboard diagnostic; Escape exits.
MINE /TERRAIN Old fast heightfield for comparison.
MINE /FULL    Old per-pixel DDA renderer for comparison.
MINE /OLD     Original integer ray marcher for comparison.

WASD move; arrows turn/look; Space jump; F dig; E place; 1-7 select block;
R reset view; H cycle detail; Escape/Q save MINE.WLD and exit.
DIG/PUT confirm actual edits. MISS/BLOCK mean no edit occurred.

Artwork is baked on the build host into 24 small 16-color face textures.
MINE.PIC is a real render of the default world, stored in four-bank video
layout (32768 bytes). It is a static preview, not a gameplay or FPS demo.
The live renderer uses integer transforms, near-plane clipping, exposed-face
caching, back-to-front voxel ordering, and packed changed-row video copies.
Still views are not repeatedly rendered. Block edits invalidate the face cache.

LIMITS: 32x24x32 world; bounded draw radius; affine texture mapping; leaf
blocks are opaque cubes. The world edge is visible. No real V20 frame-rate
claim is made until tested on hardware. Try /MID /BENCH before /MAX /BENCH.
Existing MINE.WLD is preserved. /ART and /BENCH never save it.

Inspired by WolfensteinCGA's offline art conversion, lookup tables, packed
bulk video writes and timedemo approach. Its wall scalers were not copied:
Minecraft has variable-height blocks, holes, pitch and changing geometry.
https://github.com/jhhoward/WolfensteinCGA

INPUT AND SPEED CORRECTION
Native EX/HX arrows (29/2B/4A/4E) are normalized before the IRQ latch.
IBM/E0 arrows remain supported. Walking uses elapsed BIOS time, not four
60Hz ticks per rendered frame. Swept collisions still prevent wall tunneling.
The renderer buckets exposed blocks, reuses shared transformed corners, and
uses solid material shading only for tiny distant faces at LOW/MID/DETAIL.
MAX retains full texturing. Physical V20 performance still requires testing.
