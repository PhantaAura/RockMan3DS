# Source player target-selection to Buster bridge

`source_player_select_and_spawn_80034eb8` now joins three previously separate
source-backed components:

1. `0x80035508` consumes and projects active proxy leases with the bounded PS1
   GTE model.
2. `0x80034EB8` projects the player and shared proxy bank, advances paired
   proxy rows, selects the closest eligible record in each of its two windows,
   and rejects target flag `0x80`.
3. `0x80035724` allocates the corresponding native source Buster record and
   target lease.

The private five-halfword window at `0x8008994C` is represented by a versioned,
CRC-checked 44-byte `L3TG` profile. It must be cooked from owner data; no retail
values are compiled into the runtime. Missing profile, player point, camera,
projection controls, proxy ownership, muzzle pose, table subtype, counter, or
player fields holds the path explicitly.

The unchanged original `0x80034EB8` instructions passed six authored broad-lane
cases (1,648 instructions and 12 modeled RTPS commands): center choice, nearest
of two, screen rejection, depth rejection, target flag rejection, and composite
row advancement. `0x80035508` was a return-only prerequisite hook and
`0x80035724` an argument-capture hook. The private target-window values were not
exported, so the second lane remains pending owner-profile cooking and a direct
differential.

This is a real native bridge, but it is not a completed combat encounter.
Player muzzle ownership, projectile callback/motion, source hit delivery,
damage, reaction and defeat still remain open WCA gates.
