# First scene RNG hold

The native01436 scene reaches all13class40states and20requests. Its first80100D6C callback holds atframe2609. Later sparse logs show drift because other diagnostic ticks continue after that hold. The firstscratch9336 agrees with the recordedboundary; actor Z is inferred as −575 or −574 from sampled rounded position and exact state12 steps; unlogged low16 means it is not an exact match.

No effect131 update or release occurs before thehold. Nativeconstructor creates family80slot0/subtype2;80100920 frame96 creates slot1/subtype2; actorstate4 releases slot0. Remainingbitmap40000000 differs from recordedE0000000.

Original800172F4 sweeps subtype<128 through scratch1F800064. Overlay80100360 writes table8011A604; subtype2 selects80116E68. That handler calls allocator80031618, publishes a subtype2child, and has RNGreturnaddresses80116F60/80116F98/80116FE4. The existing privateoriginaltrace contains427returns at each address between constructor and the first80100D6C callback. This route is demonstrably active in the reference and is absent from nativeworldupdate. Exactliveconditions/lifecycle remain unknown.

Next: log firstcontextmismatch once with actualactor/timer/pool, recover the bounded80116E68/subtype2 lifecycle and sourceRNGreceipts, then add originaldifferentialtests and wire800172F4 before800173B4 at the provedcallerboundary. Do not copy recordedpool/position values or bypassreceiptguards.

Private evidence: work/pass-20261013-worldalive-01436/scene-rng-context-r1/FINDINGS.md and context-evidence.json. No freshbinaryexecution, instructionpatches, or decompiledcode distribution in this diagnosis.
