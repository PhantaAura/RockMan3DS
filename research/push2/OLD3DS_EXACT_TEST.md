# Exact first Old-3DS validation gate

Fresh boot for every renderer stage:

1. trusted 0.0.14.3 fallback
2. logging only
3. UV shader/input
4. material batching with textures disabled
5. one terrain texture
6. all ordinary terrain textures
7. Mega Man texture/material path
8. alpha handling
9. animation rendering

Then run 10 tutorial transitions while recording frame time, loading spikes, heap,
linear memory, VRAM, texture count/bytes and per-frame allocations. Finally test
sleep/resume once during gameplay and once across a loading boundary.

No stage may be called hardware-verified until observed on the user's real Old 3DS/XL.
