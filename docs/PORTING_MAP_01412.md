# 0.0.14.12 visual parity / native UI pass

Baseline: cumulative 0.0.14.11_Visibility, copied into a new source directory;
no Git repository/branch/commit. Inherited Release build and 20/20 CTest suites
passed before editing. Original checkpoints and synced sources remain intact.

Reuse: actual cooked terrain/player resources, generalized area loader,
material/VRAM resolution, runtime HP/input/map state, renderer feature gates,
0143 fallback, ARMv6K build and local-only SD installer.

Change: wire the user's mockup layout into a native 320x240 lower-screen
backend, with allocation-free map sampling and genuine runtime values. Use
code-drawn UI only; no screenshot artwork, fabricated portrait, inventory,
energy or currency. Share candidate camera/projection/world-scale definitions
between native rendering and inspection, with measurable projection helpers.
Audit every active presentation path and set up owner-local PS1 references.

Cannot verify yet: original camera/scale rules, object resource initialization,
original projectile presentation, dynamic palette state, actual floor/contact
movement and physical Old3DS performance. PS1 reference emulation needs a
compatible disc/BIOS configuration; compilation alone proves none of these.

First testable milestone: existing scene boots with a live mockup-inspired
HUD and working touch tabs; unified camera projection passes host tests and
native ARMv6K compilation. Exercise the final build in Azahar Old-model mode,
keep original captures and record limits. Later fidelity claims require retail
reference measurements. PHYSICS_1TO1 remains false.

Fresh usage: 92% five-hour / 30% weekly used at start. No reset authorized;
save a separately named complete code-only recovery ZIP if capacity runs low.

## Completed audit update

Original interpreter reference succeeded. Ordinary camera H384/offsets and final local player1/8 scale/Ysign are now source/live-backed and integratedUV only. Full camera/state/contact/pixel parity remains UNKNOWN. Final23/23 host suites and455340B native build pass; original topHUD resourcebinding absent; native raster observed with real occlusion failure. Original safe0143 draw bodies andshader preserved. See REPORT_01412/RESUME_01412 for current results; initial pending notes above are audit history.
