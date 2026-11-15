# Quality-of-life options (plan, 2026-11-14)

Status: PLAN. Every item is an option, off by default unless noted, so the
original 1:1 behaviour stays the baseline. User-approved list.

## For 1.0
1. **Data anywhere (save).** Talking to Data (the monkey) stays the way to
   save, with his original dialogue and save menu. The QoL change: Data can
   be called from anywhere (a bottom-screen button / touch), not only where
   he stands in the original. Saving from the field records the current
   stage, area, position, heading and camera as well as what the original
   save keeps, and loading resumes at that spot.
   - Needs the original save system ported first (Data's save menu, the
     memory-card save data, CONTINUE on the title, front-end state 5).
   - Field saves go to a 3DS save file; their format carries a version and
     the original save block so an original-style save stays possible.
   - Option: "Call Data anywhere" (on/off). Off = only where Data stands.
2. **Modern controls.** Circle pad moves Mega Man relative to the camera;
   L/R or ZL/ZR turn the camera; C-stick camera on New 3DS. The original
   tank controls stay selectable (and the OPTION menu's own Turn setting
   keeps working).
3. **Bottom-screen touch shortcuts.** Switch special weapon, use an item,
   open the map, without pausing.

## After 1.0 (or when cheap)
- Lock-on as a toggle; button remapping.
- Map markers (doors, objective, explored areas) on the bottom-screen map.
- Dialogue history on the bottom screen (`dialogue_history.cpp` exists).
- Faster text; skip cutscenes already seen.
- Shorter loads where the original only waited for the CD.
- Sleep mode / suspend that resumes cleanly.
- Stereoscopic 3D with the depth slider.
- Presentation toggle: original PS1 look vs cleaned-up look.

## Order
Port the save system with the M3 work that first needs Data (see
`M3_ROADMAP.md`), then add "Data anywhere" on top. Modern controls and
touch shortcuts can go in any time after the M1/M3 merges settle.
