# Native player storage integration

In `platform/3ds/source/main.cpp`, immediately after building `local_visuals`,
create storage whose lifetime covers the frame loop:

```cpp
const auto player_material_words = l3::collect_player_visual_material_words(local_visuals);
std::vector<l3::PosedPlayerVisualPrimitive> player_pose_storage(local_visuals.size());
```

Add two parameters after `local_visuals` in `draw_player_uv`:

```cpp
const std::vector<uint32_t>& material_words,
std::vector<l3::PosedPlayerVisualPrimitive>& posed,
```

Replace the value-returning pose call in that function with:

```cpp
l3::pose_player_visuals_into(local_visuals, matrices, posed);
for (uint32_t word : material_words)
    draw_player_material_group(posed, word, find_player_gpu_material(word), refs, session);
```

Remove the frame-local `words` vector, `reserve`, population, sort and unique
operations from that function. Pass the new startup variables to its call:

```cpp
draw_player_uv(local_visuals, player_material_words, player_pose_storage,
               skeleton, fallback_action, animation, player_track, refs, session);
```

Leave `draw_player_0143` and its call intact to preserve the independent renderer
fallback. Area changes may replace GPU material resources; primitive material
identities remain part of the immutable player geometry, so no scratch rebuild
is required for those transitions. If player geometry is replaced later, resize
the pose storage and recollect material words at that model-load boundary.

Add the focused host target/test to the root CMake file:

```cmake
add_executable(test_player_visual_storage tests/test_player_visual_storage.cpp)
target_link_libraries(test_player_visual_storage PRIVATE legends3ds_core)
add_test(NAME player_visual_storage COMMAND test_player_visual_storage)
if(MSVC)
  target_compile_options(test_player_visual_storage PRIVATE /UNDEBUG)
else()
  target_compile_options(test_player_visual_storage PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
endif()
```

The shared source/header is already compiled by host CMake and native Makefile;
no new runtime compilation unit is required. For owner-data coverage, invoke
the same test executable with the tutorial pack directory as its sole argument.
The read-only run emits JSON aggregate evidence and does not create asset files.

For the UV draw path's final actor transform, build one shared matrix per draw
using source world units before the existing 1/512 conversion:

```cpp
const auto& raw = session.compat_player().bytes;
const uint16_t source_work_0116 = uint16_t(raw[0x116]) |
    (uint16_t(raw[0x117]) << 8u);
const auto actor_matrix = l3::build_player_actor_render_matrix(
    {float(actor_x), float(actor_y), float(actor_z)}, actor_yaw, source_work_0116);
const auto world = l3::player_pose_transform_point(actor_matrix, local_posed_point);
```

Replace the UV path's separate inverse-yaw equations and actor addition with
that transformed point. Preserve its existing source-Y inversion and 1/512
world-unit conversion. The helper applies source-backed local scale 1/8 and
root yaw `(actor_yaw - source_work_0116) & 0xfff`, leaves actor translation
unscaled, and agrees with the shared hierarchy's Y rotation convention.
Source fixed-point coefficient/translation rounding and the semantic role of
raw work+0116 remain partial/unknown. Use this in UV only; keep 0143 exact.
The focused test additionally checks 98 actor transform cases.
