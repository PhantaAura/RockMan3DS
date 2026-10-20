# 0.0.15.12 results

| Check | Result |
|---|---|
| `test_source_reward_pickup` (new, authored tables) | PASS: RNG-ordered expansion (7 calls, exact launch of the first shard); discard and release; land at bounds-adjusted Y; 150-frame rest; 30..37-frame blink; despawn; collection once (money); energy capped at max HP; holds without RNG |
| `test_actor_defeat_runtime` (extended) | PASS: the defeat queues `0x01010202` once; the reward stays queued without trig/RNG |
| Host suite | 152/152 PASS |
| Native `tools/m1_mac_build.sh` | PASS, 0 warnings; 979,832 B, SHA-256 `81b97740fa1d7893e800975e6bc35b1c78485d9bddb488094b55ddd5787baeb5`; `reward.l3rw` (512 B) cooked into the pack |
| Azahar | PENDING: Azahar was in use by the owner when the build finished |
| Physical Old 3DS | Not run |
