# How milestones are run (from 1.0 on, 2026-11-16)

User-approved working rules for every session.

1. **Map before porting.** Each milestone starts from a recomp roadmap
   (start/end flags, areas, bosses, savestates per checkpoint), as in
   `M3_ROADMAP.md`. No porting a stretch the roadmap has not mapped.
2. **Shared systems first.** Doors, NPC dialogue, shops, inventory, saving,
   special weapons and digging are built once, before the story content that
   uses them; no UI ahead of the system it shows.
3. **Test slices from save states.** Each slice is checked from a recomp/Azahar
   save state or a diagnostic stage boot. One uninterrupted full run per
   milestone, not per change.
4. **Compare every slice.** Run `tools/compare_recomp_3ds.py` on the slice's
   flags/events against the recomp when it lands, not only at the end.
5. **Two sessions at a time.** One critical-path session, one support
   session. Azahar and the recomp are shared: check `pgrep` first, never kill
   another session's process, back up and restore the Azahar SD folder.
6. **Small merges, often.** A slice that passes host ctest and
   `tools/m1_mac_build.sh` is pushed and reported as "slice ready: <name>
   <commit>"; the main session merges it at once.
7. **Real Old 3DS once per milestone.** The user plays the milestone build and
   sends `boot_01516.log` + `hardware_01516_*.csv`; fix what real hardware
   shows before moving on.
8. **Usage hygiene.** Model/effort per task (Sonnet for mechanical work, Opus
   for 1:1 porting); report results, not progress pings.
9. **Release per milestone.** Tag each milestone with a short changelog of
   what is playable and what is known missing.
