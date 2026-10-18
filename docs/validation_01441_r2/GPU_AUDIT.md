# Draw-submission repair and evidence limits

Both renderer paths opened an immediate projectile draw with no active projectile. Matching-empty player material groups and empty actor/model parts could also open zero-vertex batches. Skip them before touching draw state. Normal nonempty submissions, shader math,0143 geometry and camera calibration are preserved. Texture failures still permit0143.

Citro3D immediate begin/end does not guard a zero-vertex group. This source observation is a reason to remove empty draws; it does NOT establish that they caused the physical cyan screen. Official implementation: https://raw.githubusercontent.com/devkitPro/citro3d/master/source/immediate.c

The old scene shader follows the normal two-matrix dp4 pattern: https://raw.githubusercontent.com/devkitPro/3ds-examples/master/graphics/gpu/textured_cube/source/vshader.v.pica . No speculative shader replacement is made.

All/terrain/player choices bind their renderer before camera calibration. Isolated paths suppress only draw submissions; runtime updates and package ownership remain intact. Actor inspection camera is disabled in the isolated modes. Error-console buffers now flush each wait iteration. Durable first-submission sync labels avoid treating a queued command buffer as a completed GPU frame.

No physical performance or memory measurements were collected for01441r2. Emulator telemetry is documented separately and cannot prove real GPU compatibility or speed. World Comes Alive remains incomplete.
