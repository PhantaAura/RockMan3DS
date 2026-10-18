# Outgoing scene lifetime — 01446

The 01445 candidate handoff invalidates the old phase-5 resource binding and recorded constructor/replay admission. Actor/E0 retirement was inside service_scene_actors, but replay scheduling invokes that lambda only for an admitted recorded scene and a ready phase gate. After replacement admission is false, so retirement may never run. Area terrain rebuilding already releases outgoing GPU textures; the demonstrated defect is retained CPU actor resources and repeated old-pool cleanup, not a proven GPU texture leak.

01446 retires stale actor/E0 CPU resources and clears their presentation readiness immediately after a successful transition commit, outside replay scheduling. Release calls are made while the GPU is already idle, using the existing native release helpers. Rejected staging does not execute this CPU retirement block. Existing GPU restoration limitations are unchanged; no complete scene-texture restoration is newly claimed.

Old effect cleanup and scripted scene-view binding now require the outgoing source resource owner still to be resident. Expired pools are not interpreted as a new scene or initialized with guessed values. This does not invent destination transition completion: the new event poll still requires its unknown original transition-status owner.

The 01445 recorded boot confirms retail_baseline optics H=384/radial512/pitch48; the destination view obstruction cannot be attributed to a legacy-distance calibration. No FOV/distance, spawn, geometry, physics, RNG scalar, shader or vertex-layout change was made here. Camera obstruction, fresh clock and full WCA remain open.
