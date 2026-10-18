# Commands and results — 01444

Source: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/Legends3DS_0.0.14.41_WorldComesAlive`. No Git repository, branch or commit. Parent: `/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/deliverables/Legends3DS_0.0.14.43_Source.zip`.

Actual successful build pipeline:

```sh
python3 -B /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/Legends3DS_0.0.14.41_WorldComesAlive/tools/build_hardware_test.py --pack /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01440/pack-runtime-r1 --out /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261015-worldalive-01444/native-r1 --host-build /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261014-worldalive-01441/native-r1/host
```

This configured Release, built the inherited host suite, ran139/139CTest checks (4.72seconds), validated the existing pack, built/probed a neutral package wrapper, and compiled the actual ARMv6K/mpcore target. No extractor/cooker ran. Exact expanded commands, exit codes and log paths are in `build-result.json`; copies of all build/test logs are beside it.280native source input hashes match the staged and mutable sources. Native output885,952bytes, SHA256`1177b985a18b63c6b1eae044606630877776c7e6e21937b5750253638dd5d948`. No warning/error diagnostic lines occurred in either final host/native build log. Touched native modules use warnings-as-errors where specified in the Makefile; this is not a claim every project unit usesWerror.

Failures retained: the interim run passed138/139; only`native_checkpoint_version`failed because metadata still said43while the header was44. Corrected metadata, then reran the full suite139/139. No assertion was removed. `contact-before.log` and `contact-rollback-before.log` retain expected failures against the preserved43runtime; the corrected implementation passes both regressions in the final suite. Baseline inherited139/139passed before edits (5.81seconds).

Additional owner-reference observations were performed with the reproducible probes and JSON outputs in`docs/research_01444`: player_floor_order_probe.py (explicit floor-return hook), player_map_edge_probe.py (nohooks sampled map query), source_head_uv_reset_probe.py (587originalinstructions plus81authoredcases), source_handoff_spawn_probe.py (64unchanged-instructioncases), source_event_command2_probe.py (boundeddispatch), and source_event_command2_pause_boundary_probe.py (36phase0cases/646,240instructions; declared transport boundary). Their source identities/arguments/limits are recorded in each probe/proof. Tests do not infer whole-gameparity.

Azahar2126.0: Oldmodel, CPU100%, Vulkan, resolution1. Same compiled binary exercised Atextured, Xsafe and an explicit gatedsource scene. Eight original screenshots and three traces are indexed in`emulator-observation.json`. Source phase7reset/orderedpause/area commit were not reached naturally. All three runs exited normally and logged zero packagebytes after unload. Owned PIDs41430/41880 were absent after GUIquit. No physical44test or separate desktopvisual test was performed. Shared portable code was exercised by the full host suite.

Local pack/profile/SD may contain owner-local assets/DSP; these remain outside releases. App-only ZIP contains onlyREADME/3dsx/smdh. Whole sourceZIP CRC and every source manifest entry are verified by the packaging receipt outside the archive.
