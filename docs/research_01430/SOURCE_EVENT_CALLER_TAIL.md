# Source scene caller and fullscreen color owner

The supported native scene update now reaches the original caller tail after
callback80100738, camera8001EC0C, and timeline8001F070. Previously the native
continuation remained held at801011E8 after its first counter update.

Unchanged original instructions prove that8001DE84 tests a raw stage byte and
source story flag896+event before it reads controller halfword800C0C2A. If the
stage byte is zero and the flag is false, the predicate returns false without a
controller read. The explicit fresh-executable subset owns stage byte800C1B6A=0
through the documented BSS clear and source flag897=false through the existing
fresh story-bank initialization. It therefore admits this false branch without
guessing live input. Input bit8 retains its address/bit label.

Original80100614 submits an untextured320×240 semitransparent TILE62 and a draw
mode packet. Mode2 producesE100004F and destination-minus-source RGB blending.
It consumes80 bytes in the original frame packet allocator and links both
packets at ordering-table byte116. This is a fullscreen scene operation,
separate from actor tint, GTE fog, background color, and material shading.

The native owner publishes a fixed RGB descriptor with a native renderer lease;
it does not fabricate original packet pointers or ordering-table addresses.
The renderer must submit that presentation and acknowledge its exact lease.
Only then does the caller release the next scene update. Pending presentations
hold without repeating callback or timeline stores. The serial is process-wide,
monotonic on the single engine thread, and never reset by session assignment,
save/area loading, or reconstruction at the same address. A session identity
token and independently renewed copy/move generation prevent another copied
session or same-request assignment accepting an old receipt. Current profile
and active EBD ownership are checked before publication and acknowledgement.

Other color modes remain unsupported. A true8001DE84 result, or source selector19
at counter44, still requires the original type10 fade80017C30. Neither the fade
nor the following phase advance is acknowledged. Later timeline callbacks,
source-camera rendering, the full scene, and natural tutorial progression remain
separate incomplete owners.

Evidence is `caller-tail-source-proof.json` and the three authored fixture files
`source-event-caller-{predicate,color,caller}.tsv`. The probe executes15,105
unchanged original instructions across192 predicate,48 packet, and64 caller
cases, with zero instruction patches or helper hooks. These are authored source
differentials, not live retail parity or hardware evidence.

The E0 first-update diagnostic service is independently enabled by the frontend.
Original800158F0 processes events at80015904 before invoking world updates;
80015DB4 reaches the E0 sweep at80015E0C. Main phase0 suppresses that sweep while
the fade is busy. The native main-phase/busy ownership is not complete, so the
service reports CANDIDATE chronology and must never run implicitly inside the
scene constructor. Its callback stores and resource bounds have independent
SOURCE_BACKED_PARTIAL evidence.

Exact focused commands, from the source root:

```sh
cmake -S . -B /path/to/new/host -DCMAKE_BUILD_TYPE=Debug
cmake --build /path/to/new/host --target test_source_event_phase6 test_source_event_caller -j4
ctest --test-dir /path/to/new/host -R 'source_event_phase6$|source_event_caller$' --output-on-failure
python3 docs/research_01430/source_event_caller_probe.py --exe /owner/ROCK_NEO.EXE --st00 /owner/ST00.BIN --out /new/private/caller-evidence
python3 docs/research_01430/scene_update_order_probe.py --exe /owner/ROCK_NEO.EXE --st00 /owner/ST00.BIN --out /new/private/update-order-evidence
```

Native submission, image output, GPU quantization, and Old3DS performance require
the frontend build/emulator and hardware checks recorded by the checkpoint.
