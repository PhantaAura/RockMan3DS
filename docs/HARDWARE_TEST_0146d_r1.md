Latest r1 native/host build passed; r1 gameplay remains untested. Earlier d replay proved native controls but showed rendering defects. This is a provisional technical test.

# Old 3DS scene-test handoff — 0.0.14.6d-r1

This is a technical tutorial-scene test, not the complete tutorial or full game. It continues the supplied native source. The local SD directory contains the previously authorized cooked pack and must stay local.

1. Copy the contents of the supplied `sd_out` directory to the SD card root, retaining the `3ds/Legends3DS/data/legends3ds/tutorial/` path. Copy the entire directory tree, not only the executable.
2. In an existing compatible Homebrew Launcher on the original Old 3DS, launch `3ds/Legends3DS/Legends3DS_0146d_r1_SceneTest.3dsx` without holding any buttons during startup. This selects the enhanced renderer. No system modification or exploit-installation steps are included here.
3. At the diagnostic start screen press A. Check that player and terrain appear. Circle Pad up or D-pad up moves forward. Circle Pad left/right, D-pad left/right, or L/R turns. Down/reverse and jump are not implemented. Y fires a projectile; there are no native enemy targets yet. SELECT manually cycles the five test areas. START exits. X at the start screen selects the preserved 0143 fallback for comparison.
4. Move, turn both ways and fire. Cycle through all five areas. Note broken textures, player facing, terrain gaps, loading stalls, crashes and any Log FAIL message. The largest area may be slow because terrain culling is not implemented.
5. Preserve photographs/captures of both screens and copy `3ds/Legends3DS/hardware_0146d_r1_*.csv` and `boot_0146d_r1.log` after exiting. A useful report includes the console model, displayed build, tested area, controls exercised and exact failure if any.

Logs identify the actual model, memory layout, reserved heaps, malloc usage, linear/VRAM space, submitted triangles/draws and sparse CPU timing. Sparse logs cannot prove worst-case frame pacing; they also have write/flush overhead. No successful hardware result is recorded yet.
