# Area visibility and diagnostic entry audit

This audit uses the inherited 0.0.14.10 owner-local boot package and the inherited native frontend. It does not establish retail camera, entry, contact or physics parity.

The inherited frontend spawns at the first positive **grid-cell** center with yaw zero. Positive render geometry uses authored **IDX** XYZ. In the fifth diagnostic area, the first grid entry differs from its IDX origin, lies on the minimum-Z boundary, and the camera faces outward toward negative Z. Geometry exists and is submitted; the old view admits no terrain vertices. This explains that area's empty default view without assigning an unsupported GPU or material fault.

The inherited world conversion is `(x/512, -y/512, z/512)`. The inspection audit uses the same conversion, 62-degree projection, 400:240 aspect, near/far 0.05/180 and existing follow-camera offsets: distance 5.2, eye height 3.0, target height 1.1. It counts primitive records with at least one original vertex inside that analytic frustum. A positive count establishes admission, while an emulator or hardware capture must establish actual rasterization.

| Diagnostic area | Positive primitives | Primitive records admitted by old default view | Records admitted by diagnostic entry and heading |
|---|---:|---:|---:|
| 0 | 488 | 20 | 478 |
| 1 | 526 | 15 | 392 |
| 2 | 674 | 57 | 472 |
| 3 | 120 | 48 | 119 |
| 4 | 3,028 | 0 | 879 |

These counts use positive geometry from the previous package and precede the new negative-terrain binding. The cumulative inspection probe also enumerates bound negative geometry from new packages and reports absent default/special bindings explicitly.

The shared diagnostic helper takes admitted primitive records, collects their bounds, selects an in-area coordinate candidate near the bounds center, and chooses one of 64 headings that admits the most records. Positive candidates retain authored IDX XYZ. Negative candidates retain the established default-branch grid origin and packed height, including halfword storage. Unsupported special branches are excluded. This is a **CANDIDATE inspection entry**, not a retail spawn, floor solver or new retail camera rule.

The helper allocates only during area inspection. It accepts an explicit primitive admission limit, caps growth requests, exposes actual vector storage bytes and releases temporary geometry before gameplay. Previous-package positive inspection storage was 23,424 / 25,248 / 32,352 / 5,760 / 145,344 bytes. Allocation bookkeeping and intermediate growth peaks are separate from these resident capacities. No allocation or inspection heading search is required per frame.

At audit time, native rendering uses its own follow-camera matrix and does not consume `RuntimeSession` camera ownership/handoff state. `RuntimeSession::tick` integrates the translated horizontal motion branch but does not invoke the existing retail contact response bridge automatically. The former negative terrain omission and the absent contact integration therefore need separate status tracking. A rendered floor does not prove collision, gravity, grounding or wall response. `PHYSICS_1TO1` remains false.

Focused tests cover inward/outward frustum admission, authored origins differing from grid aliases, negative candidate height/origin, malformed map records, missing descriptors, bounded admission, integer extremes and explicit release. The new helper and fixtures pass warnings-as-errors, AddressSanitizer and UndefinedBehaviorSanitizer. The real-package probe emits aggregate counts only; private owner coordinates and payloads remain outside the source checkpoint.
