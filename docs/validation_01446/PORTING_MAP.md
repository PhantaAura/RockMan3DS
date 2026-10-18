# 01446 porting map

Reuse the existing shared candidate RNG, imported scene resources, guarded source handoff, destination staging and both unchanged shader/layout paths.

Fix area-local scene resources surviving a committed handoff when the recorded-phase gate no longer invokes the actor update lambda. Release outgoing CPU models and GPU textures after successful commit, outside the simulation/replay gate; preserve them on rejected staging. Stop running old-pool cleanup and source-view projection after their area owner expires. Keep missing destination transition/counter inputs unknown; no invented completion or camera clamp.

First milestone: run the same candidate intro to Area 1, observe actual scene resources released and no repeated InvalidPool cleanup; compare memory and handoff against 01445. Build actual Old 3DS and run inherited host checks. Camera obstruction, physical acceptance and full WCA remain unverified.
