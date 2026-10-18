# Porting map — 0.0.14.49

Reuse: the imported primary player skeleton/animation, reference trig table,
target selector, Buster allocation, generalized packages, renderer gates,
events, and the 0.0.14.3 safe renderer.

Changed: the runtime can now produce the ordinary source-backed integer player
pose, bind the proven part-4/part-7 muzzle endpoints, load a strict owner-cooked
Buster profile, initialize all 16 source selection counters, and apply direct
fire actions 0–3.

Cannot yet verify: alternate limb-pose banks, action 4–7 helper results,
projectile callback/motion, live hit delivery, full enemy HP/reaction/defeat,
retail RNG chronology, or physical Old 3DS behavior for this checkpoint.

Next small milestone: recover the alternate limb bank and fire helpers, then
exercise one imported enemy through projectile motion, hit, HP, reaction, and
defeat without hardcoding the room.
