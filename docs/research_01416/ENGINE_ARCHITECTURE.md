# Original-engine research before implementation

The public PS1 decomp reference SHA1 exactly matches the owner executable. It is
unfinished and not shiftable; use pinned symbols as navigation, then verify every
behavior with original instructions. No public code or assets were copied.
[Reference hash](https://github.com/ChrisNonyminus/mml1/blob/964bad3096a2d7a4a3d6cc65678d8440b2ea9cb1/hash/us/rock_neo.BIN.sha1).

The original separates main code, stage program overlays, messages, map indices,
map cells, EBD resources and stage geometry at fixed PS1 partitions. This is an
engine with table-dispatched states and staged loading. Geometry alone does not
include the actor/event ownership needed for a faithful area. The native port
should retain this logical separation through bounded area-local records, without
requiring original PS1 addresses as native pointers.
[Pinned symbols](https://github.com/ChrisNonyminus/mml1/blob/964bad3096a2d7a4a3d6cc65678d8440b2ea9cb1/config/syms.us.rock_neo.txt),
[game-state research](https://github.com/ChrisNonyminus/mml1/blob/964bad3096a2d7a4a3d6cc65678d8440b2ea9cb1/src/rock_neo/game.c).

MML1-specific viewer code independently agrees with our EBD directory/mesh layout:
16-byte directory rows, hierarchy at descriptor+0x10, detail pointers+0x70/+74/+78,
20-byte parts,8-byte vertices and12-byte triangle/quad packets. Keep the existing
bounded decoder and original callback/sequence evidence. The viewer has pointer
spacing guesses and timing/rotation approximations, so its playback cannot prove
retail fidelity. Exported static geometry is insufficient for original animation.
[EBD viewer](https://gitlab.com/megamanlegends/mml1/-/blob/b2078b2601fbf1f627710fcd9435cca71933f0df/public/js/EbdLoader.js).

Original MML1 texture resolution depends on simultaneous image-page and CLUT state
in VRAM. Keep ordered uploads, palette identities and last-writer ownership; an
external resource name is only a lead. Owner EM0300.TIM is a candidate for first
class03 material and must match actual material words/live state before use.
[Framebuffer viewer](https://gitlab.com/megamanlegends/mml1/-/blob/b2078b2601fbf1f627710fcd9435cca71933f0df/public/js/Framebuffer.js),
[author texture inventory](https://gitlab.com/megamanlegends/mml1-psx/-/blob/c4a66ae0b56dd27935ccb3eb5ecc73e6c52cd4df/read/models.js).

IDX raw6 is mutable original state. Low12 bits select collision lists; the whole
field also selects terrain geometry. Our static cooked record0 and measured live
record1 differ for three tutorial tiles. This can affect both floors and what is
rendered. The external map viewer corroborates alternate descriptor banks but
never identifies the writer; force-setting every record would be unsupported.
[Map viewer](https://gitlab.com/megamanlegends/mml1/-/blob/b2078b2601fbf1f627710fcd9435cca71933f0df/public/js/MdtLoader.js).

The efficient next step is observing the original writer, not rewriting renderer
or physics. Install bounded read-only write breakpoints during a fresh isolated
New Game/load. Verify writer PC, exact inputs, domain and timing against owner
EXE/overlay. Implement proven initialization before creating both RuntimeSession
and native render resources, since they currently own separate map copies.
[Official observer API](https://pcsx-redux.consoledev.net/Lua/breakpoints/).

After initialization, continue original actor scheduler/state/HP/hit ownership.
Existing generic projectile constants and player HP are integration values, not
retail measurements. Do not use those defaults to claim completed enemy combat.
Do not treat MML2 documentation or generic PS1 layouts as MML1 evidence.
