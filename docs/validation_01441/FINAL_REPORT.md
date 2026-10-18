# 0.0.14.41 hardware investigation result

Source project:/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/Legends3DS_0.0.14.41_WorldComesAlive. Baseline0.0.14.40sourceZIP SHA427fe849595090087efb23cdeadc0b7053dbf903e2a7360858314a12546685fc. NoGit repository, branch or commit.

Toolchain:devkitARM r68-1/GCC16.1.0,libctru2.7.0-1,Citro3D1.7.1-2;actualARMv6K/mpcorehardfloat. Finalbinary:/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/deliverables/Legends3DS_DisplayTest_01441.3dsx;SHAb1dfc9531a3dbc43aa73588ae7470d2869769d2e556d3fd4f173bcef55e56a82;868008bytes. All279nativeinputs verified against current source.

Outcome: automaticCPUcolour test and shader-freeGPUcyan test run before any game assets; prompts explicitly identify14.41; X/B select fallback/texturesoff after the GPUtest. Fixed a new selector-order regression by binding the existing matching camera after the choice. Trusted0143geometry/shader/model math unchanged. Recorded GPUcommand usage after completed submissions. Originalhardwareblack-screen cause is still unknown.

Tests:full138/138hostPASS27.49s; final138/138PASS41.79s. Nativefullbuild and finalincrementalbuild PASS. Exact commands/logs in TEST_COMMANDS.md. No shareddesktopcorechange; hostcore/regressions pass, desktopgame notrun.

Old-modelAzahar2126.0 finalbinary visibly showsCPUbars, GPUcyan, and fullplayer/bottommap after explicitXfallback. Three genuine unedited400x480captures in capture-validation.json. Priorr2emptySDtest proved diagnostic opens withoutassets/config. r2oversizedfallbackcapture preserved and correctly markedfailed; r3matchinglegacycamera capture is final.

Physical: user reports14.39/14.40topblack, bottommapworks, then clarifies no14.40colourtestprompts. Fallbackselection was never confirmed.14.41physicaltest pending. No physicalscreenshots, framepacing, memorypeak or GPUcommand usage acquired. This remains a technicalprototype and WorldComesAlive isunfinished. Inheritedcombat/event/RNG/progression blockers are preserved, not markedcomplete. No newretailasset extraction/cooking or usage reset.

NewSDinstallZIP:/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/deliverables/Legends3DS_01441_DisplayTest_SD.zip; SHA759fc3f43ad687f029843f232cfb5c6c1c44375314dc475a0e5602cfe6432d0c. It contains onlyREADME and a separatelynamed3DSX/SMDH app, no game data. SourceZIP:/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/deliverables/Legends3DS_0.0.14.41_Source.zip; complete cumulativecode/project/buildscripts/docs/authoredfixtures. Excludes retailassets/binaries/cookedpayloads/saves/DSP and reproduciblehost/nativeoutputs/caches; those reside outside the source tree. FullsourceSHAmanifest, ZIPCRC andexpectedsourceentries are verified during packaging. ZIPhash is stored alongside it.
