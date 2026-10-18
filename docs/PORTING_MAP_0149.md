# 0.0.14.9 local data installer: audit and first milestone

Parent: the complete 0.0.14.8 source checkpoint, preserved in its own tree. The
parent SOURCE_MANIFEST verifies all 394 listed files. Neither the project mirror
nor the source tree is a Git repository; branch and commit are unavailable.
The inherited Release host build and all ten CTest suites pass before editing.

Reuse the existing resource decoders, generalized MML2 package cooker/content
graph, tutorial sidecars, PS1 load-order-aware material caches, pack validator,
native source snapshot helper and ARMv6K/libctru/Citro3D runtime. Keep both frozen
0143/UV shaders and renderer feature gates. No gameplay/physics fidelity change
is claimed in this milestone.

Change the host installation path: accept a supported local extraction folder
or extraction ZIP, validate every file against a fingerprint profile, cook into
bounded local staging/cache directories, validate packages/sidecars, install the
generalized boot package and build the actual current native target. Existing
output, owner files and earlier checkpoints must survive failures. A code-only
rebuild should reuse independently verified cooked data. The old command's
native target name and source verifier are stale; package-validation logs are
overwritten, the generalized package is not deployed, and the tutorial cooker
can delete its output before validating input. Those are this pass's fixes.

User authorization in this conversation now permits local extraction and
conversion of the supplied game files. Input data and cooked packages stay on
the owner's computer/SD and are excluded from the source release. File hashes
identify a supported profile; they neither certify ownership nor establish
retail gameplay parity. No copy-protection bypass is included.

First testable milestone: run one command against the supplied supported loose
game files, generate and validate neutral packages, deploy a generalized
map/render package with byte-identical compatible sidecars, compile the Old 3DS
target, and verify a repeat run uses the cook cache. Also reject malformed ZIPs,
changed data and unsafe output paths using synthetic tests.

Direct CHD/BIN/CUE intake requires an independently tested disc/XA extraction
contract and is deferred. The installed native runtime binds generalized
map/render only; objects/events/audio/camera records do not become gameplay merely
because they are decoded. Existing geometry gaps, PHYSICS_1TO1=false, incomplete
natural progression and physical Old 3DS validation remain outstanding.
