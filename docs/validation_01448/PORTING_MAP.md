# Porting map — 0.0.14.48

Reuse: generalized actor proxy bank, exact bounded GTE projection, child/Buster
pool and lease ownership, area loader, event/transition work, renderer gates and
the 0.0.14.3 safe path.

Changed: the disconnected proxy/GTE and Buster producer paths now have the
source `0x80034EB8` two-lane target selector between them. The target window is a
versioned owner-cooked profile and malformed or missing inputs hold explicitly.

Cannot yet verify: live source camera/projection ownership, muzzle endpoints,
Buster counters/firing chronology, projectile callback/motion, enemy hit/HP,
reaction/defeat, strict-window retail differential, physical performance and
whole-scene RNG order.

Next testable milestone: establish the player pose-to-muzzle owner and current
source projection controls, then exercise one imported proxy through target
selection, native Buster allocation, projectile motion and a guarded hit receipt.
