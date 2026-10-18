# Automatic Old 3DS XL display test

The user reported a black top screen with a working bottom map in 14.39 and 14.40. They clarified that the 14.40 L+R colour-test prompts never appeared. Consequently, neither colour test has a physical result yet; SELECT fallback selection was not confirmed either. Treat the previous fallback-failure inference as withdrawn.

This separately labelled 14.41 diagnostic build opens the following tests automatically, before loading any game package or initializing a shader. No button chord, configuration change, or new assets are required.

1. The bottom screen must identify LEGENDS3DS 0.0.14.41 / TOP SCREEN TEST 1/2. The top should show red, green and blue bands.
2. Press and release A. Test 2/2 should show a bright cyan top screen, using only the native GPU render target and clear/display transfer.
3. Press and release X to continue using the safe0143 renderer. A keeps the selected renderer; B selects textures off.

Report which tests appear, their top-screen colours, and whether gameplay remains black. If there are no 14.41 prompts, confirm the exact newly named file was launched; it cannot intentionally skip both tests. Preserve boot_01441.log and hardware_01441_*.csv under /3ds/Legends3DS/ if available.

This is an investigation build, not a confirmed black-screen fix. A successful emulator result cannot establish physical device success. Existing gameplay, owner assets and the trusted renderer remain cumulative and available after the tests.

## SD installation

The diagnostic SD ZIP contains a separate `/3ds/LegendsDisplayTest/` application, with the title `Legends3DS 01441 Display Test`. Extract it to the SD root and launch that entry normally. It reads the existing game data from `/3ds/Legends3DS/data/` only after both tests, so no retail assets are duplicated or included in the diagnostic archive. The two colour tests themselves require no game data.
