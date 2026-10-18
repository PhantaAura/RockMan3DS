cmake_minimum_required(VERSION 3.16)
project(Legends3DSTutorialCore LANGUAGES C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(legends3ds_core STATIC
    src/world/st00_source.cpp
    src/world/stage_spawn.cpp
    src/world/st00_spawn.cpp
    src/world/stage_runtime.cpp
    src/world/stage_catalog.cpp
    src/world/resource_catalog.cpp
    src/world/audio_catalog.cpp
    src/world/stage_object_catalog.cpp
    src/world/ebd_directory.cpp
    src/world/actor_resource.cpp
    src/world/actor_contact.cpp
    src/world/actor_detection.cpp
    src/world/actor_detection_profile.cpp
    src/world/actor_detection_math.cpp
    src/world/actor_detection_segment.cpp
    src/world/actor_detection_preparation.cpp
    src/world/actor_action4_contact.cpp
    src/world/actor_action9_contact.cpp
    src/world/actor_action3_contact.cpp
    src/world/source_combat_profile.cpp
    src/world/source_audio_queue.cpp
    src/world/class60_06.cpp
    src/world/source_profile_class60_06.cpp
    src/world/class20_03_hit.cpp
    src/world/source_hit_proxy.cpp
    src/world/source_actor_consumers.cpp
    src/world/child_runtime.cpp
    src/world/source_rng_service.cpp
    src/flow/source_fade.cpp
    src/flow/source_event_phase5.cpp
    src/flow/source_event_phase6.cpp
    src/flow/source_event_random_effect.cpp
    src/flow/source_event_random_tape.cpp
    src/world/scene_e0.cpp
    src/render/scene_e0_material.cpp
    src/world/source_scene_view.cpp
    src/world/source_scene_actor40.cpp
    src/world/source_scene_actor40_pose.cpp
    src/world/source_scene_actor40_init.cpp
    src/world/source_scene_actor40_update.cpp
    src/world/source_scene_actor40_tail.cpp
    src/flow/source_main_loop.cpp
    src/flow/source_loop_snapshot.cpp
    src/world/class20_03_reaction.cpp
    src/world/source_reaction_profile.cpp
    src/world/actor_reaction_contact.cpp
    src/flow/source_event_pipeline.cpp
    src/flow/source_event_consumer.cpp
    src/flow/source_event_phase2.cpp
    src/flow/source_event_package.cpp
    src/flow/source_event_command.cpp
    src/flow/source_event_command_package.cpp
    src/world/source_player_proxy.cpp
    src/world/source_player_buster.cpp
    src/flow/gameplay_gates.cpp
    src/world/source_player_pose.cpp
    src/world/source_buster_profile.cpp
    src/world/source_camera.cpp
    src/world/xa_stream.cpp
    src/world/xa_transport.cpp
    src/world/source_effects.cpp
    src/world/source_effect2.cpp
    src/world/source_effect34.cpp
    src/world/source_effect_dispatch.cpp
    src/world/source_prop_lift.cpp
    src/world/source_prop_attach.cpp
    src/world/actor_pose_runtime.cpp
    src/world/class20_03_proxy.cpp
    src/world/class20_03_action4.cpp
    src/world/retail_rng_tape.cpp
    src/world/class20_03_action9.cpp
    src/world/class20_03_action3.cpp
    src/world/actor_runtime.cpp
    src/world/class20_03_behavior.cpp
    src/world/source_angle.cpp
    src/world/actor_animation_bank.cpp
    src/world/actor_sequence.cpp
    src/world/ebd_dynamic_selector.cpp
    src/world/tutorial_runtime.cpp
    src/world/runtime_session.cpp
    src/world/l3map.cpp
    src/world/st00_render.cpp
    src/world/area_package.cpp
    src/world/map_initialization.cpp
    src/world/tile_transform.c
    src/model/pbd_model.cpp
    src/model/player_composition.cpp
    src/texture/ps1_texture.cpp
    src/texture/material_resolver.cpp
    src/texture/material_cache.cpp
    src/render/terrain_geometry_candidate.cpp
    src/render/area_placement.cpp
    src/render/positive_terrain.cpp
    src/render/negative_terrain.cpp
    src/render/negative_materials.cpp
    src/render/area_inspection.cpp
    src/render/frontend_view.cpp
    src/render/visual_bridge.cpp
    src/render/ctr_texture_layout.cpp
    src/render/actor_material.cpp
    src/animation/pbd_animation.cpp
    src/animation/player_pose.cpp
    src/flow/mml_message.cpp
    src/flow/st00_flow.cpp
    src/flow/st00_event.cpp
    src/flow/save_state.cpp
    src/input/classic_input.c
    src/player/classic_motion.c
    src/player/player_runtime.c
    src/player/player_collision.c
    src/player/player_contact_runtime.cpp
    src/player/reference_tables.cpp
    src/ui/qol_settings.c
    src/ui/dialogue_history.cpp
    src/ui/bottom_screen.cpp
    src/ui/bottom_map.cpp
    src/ui/mockup_ui.cpp
)
target_include_directories(legends3ds_core PUBLIC include)
if(NOT MSVC)
  target_compile_options(legends3ds_core PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_executable(l3cook_st00 tools/l3cook_st00.cpp)
target_link_libraries(l3cook_st00 PRIVATE legends3ds_core)

add_executable(l3inspect_map tools/l3inspect_map.cpp)
target_link_libraries(l3inspect_map PRIVATE legends3ds_core)

add_executable(l3cook_pbd tools/l3cook_pbd.cpp)
target_link_libraries(l3cook_pbd PRIVATE legends3ds_core)

add_executable(l3inspect_model tools/l3inspect_model.cpp)
target_link_libraries(l3inspect_model PRIVATE legends3ds_core)

add_executable(l3cook_texture tools/l3cook_texture.cpp)
target_link_libraries(l3cook_texture PRIVATE legends3ds_core)

add_executable(l3inspect_texture tools/l3inspect_texture.cpp)
target_link_libraries(l3inspect_texture PRIVATE legends3ds_core)

add_executable(l3cook_palette tools/l3cook_palette.cpp)
target_link_libraries(l3cook_palette PRIVATE legends3ds_core)

add_executable(l3inspect_palette tools/l3inspect_palette.cpp)
target_link_libraries(l3inspect_palette PRIVATE legends3ds_core)

add_executable(l3cook_anim tools/l3cook_anim.cpp)
target_link_libraries(l3cook_anim PRIVATE legends3ds_core)

add_executable(l3inspect_anim tools/l3inspect_anim.cpp)
target_link_libraries(l3inspect_anim PRIVATE legends3ds_core)

add_executable(l3cook_skeleton tools/l3cook_skeleton.cpp)
target_link_libraries(l3cook_skeleton PRIVATE legends3ds_core)

add_executable(l3inspect_skeleton tools/l3inspect_skeleton.cpp)
target_link_libraries(l3inspect_skeleton PRIVATE legends3ds_core)

add_executable(l3cook_render tools/l3cook_render.cpp)
target_link_libraries(l3cook_render PRIVATE legends3ds_core)

add_executable(l3inspect_render tools/l3inspect_render.cpp)
target_link_libraries(l3inspect_render PRIVATE legends3ds_core)


add_executable(l3cook_player_comp tools/l3cook_player_comp.cpp)
target_link_libraries(l3cook_player_comp PRIVATE legends3ds_core)

add_executable(l3inspect_player_comp tools/l3inspect_player_comp.cpp)
target_link_libraries(l3inspect_player_comp PRIVATE legends3ds_core)

add_executable(l3scan_materials tools/l3scan_materials.cpp)
target_link_libraries(l3scan_materials PRIVATE legends3ds_core)

add_executable(l3cook_material_cache tools/l3cook_material_cache.cpp)
target_link_libraries(l3cook_material_cache PRIVATE legends3ds_core)

add_executable(l3inspect_material_cache tools/l3inspect_material_cache.cpp)
target_link_libraries(l3inspect_material_cache PRIVATE legends3ds_core)

add_executable(l3cook_player_material_cache tools/l3cook_player_material_cache.cpp)
target_link_libraries(l3cook_player_material_cache PRIVATE legends3ds_core)
add_executable(l3cook_actor_material_cache tools/l3cook_actor_material_cache.cpp)
target_link_libraries(l3cook_actor_material_cache PRIVATE legends3ds_core)
target_compile_options(l3cook_actor_material_cache PRIVATE -Wall -Wextra -Wpedantic -Werror)


add_executable(l3visual_probe tools/l3visual_probe.cpp)
target_link_libraries(l3visual_probe PRIVATE legends3ds_core)

add_executable(l3cook_msg tools/l3cook_msg.cpp)
target_link_libraries(l3cook_msg PRIVATE legends3ds_core)

add_executable(l3inspect_msg tools/l3inspect_msg.cpp)
target_link_libraries(l3inspect_msg PRIVATE legends3ds_core)

add_executable(l3cook_st00_flow tools/l3cook_st00_flow.cpp)
target_link_libraries(l3cook_st00_flow PRIVATE legends3ds_core)

add_executable(l3inspect_st00_flow tools/l3inspect_st00_flow.cpp)
target_link_libraries(l3inspect_st00_flow PRIVATE legends3ds_core)


add_executable(l3runtime_probe tools/l3runtime_probe.cpp)
target_link_libraries(l3runtime_probe PRIVATE legends3ds_core)

add_executable(l3runtime_pack_probe tools/l3runtime_pack_probe.cpp)
target_link_libraries(l3runtime_pack_probe PRIVATE legends3ds_core)

add_executable(l3runtime_msg_probe tools/l3runtime_msg_probe.cpp)
target_link_libraries(l3runtime_msg_probe PRIVATE legends3ds_core)

add_executable(l3runtime_session_probe tools/l3runtime_session_probe.cpp)
target_link_libraries(l3runtime_session_probe PRIVATE legends3ds_core)

add_executable(l3inspect_stage_catalog tools/l3inspect_stage_catalog.cpp)
target_link_libraries(l3inspect_stage_catalog PRIVATE legends3ds_core)

add_executable(l3game_pack_probe tools/l3game_pack_probe.cpp)
target_link_libraries(l3game_pack_probe PRIVATE legends3ds_core)

add_executable(l3script_pack_probe tools/l3script_pack_probe.cpp)
target_link_libraries(l3script_pack_probe PRIVATE legends3ds_core)

add_executable(l3inspect_resource_catalog tools/l3inspect_resource_catalog.cpp)
target_link_libraries(l3inspect_resource_catalog PRIVATE legends3ds_core)

add_executable(l3inspect_audio_catalog tools/l3inspect_audio_catalog.cpp)
target_link_libraries(l3inspect_audio_catalog PRIVATE legends3ds_core)

add_executable(l3stage_object_pack_probe tools/l3stage_object_pack_probe.cpp)
target_link_libraries(l3stage_object_pack_probe PRIVATE legends3ds_core)

add_executable(l3player_runtime_probe tools/l3player_runtime_probe.cpp)
target_link_libraries(l3player_runtime_probe PRIVATE legends3ds_core)

add_executable(l3flight_render_probe tools/l3flight_render_probe.cpp)
target_link_libraries(l3flight_render_probe PRIVATE legends3ds_core)

add_executable(l3flight_material_probe tools/l3flight_material_probe.cpp)
target_link_libraries(l3flight_material_probe PRIVATE legends3ds_core)

add_executable(l3flight_animation_probe tools/l3flight_animation_probe.cpp)
target_link_libraries(l3flight_animation_probe PRIVATE legends3ds_core)

add_executable(l3cook_ebd tools/l3cook_ebd.cpp)
target_link_libraries(l3cook_ebd PRIVATE legends3ds_core)

add_executable(l3inspect_ebd tools/l3inspect_ebd.cpp)
target_link_libraries(l3inspect_ebd PRIVATE legends3ds_core)

add_executable(test_core tests/test_core.cpp)
target_link_libraries(test_core PRIVATE legends3ds_core m)

if(NOT MSVC)
  foreach(t l3cook_st00 l3inspect_map l3cook_pbd l3inspect_model l3cook_texture l3inspect_texture l3cook_palette l3inspect_palette l3cook_anim l3inspect_anim l3cook_skeleton l3inspect_skeleton l3cook_render l3inspect_render l3cook_player_comp l3inspect_player_comp l3scan_materials l3cook_material_cache l3inspect_material_cache l3cook_player_material_cache l3visual_probe l3cook_msg l3inspect_msg l3cook_st00_flow l3inspect_st00_flow l3runtime_probe l3runtime_pack_probe l3runtime_msg_probe l3runtime_session_probe l3inspect_stage_catalog l3game_pack_probe l3script_pack_probe l3inspect_resource_catalog l3inspect_audio_catalog l3stage_object_pack_probe l3player_runtime_probe l3flight_render_probe l3flight_material_probe l3cook_ebd l3inspect_ebd test_core)
    target_compile_options(${t} PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endforeach()
endif()

enable_testing()
add_executable(l3cook_map_init tools/l3cook_map_init.cpp)
target_link_libraries(l3cook_map_init PRIVATE legends3ds_core)
add_executable(test_map_initialization tests/test_map_initialization.cpp)
target_link_libraries(test_map_initialization PRIVATE legends3ds_core)
add_test(NAME map_initialization COMMAND test_map_initialization)
if(NOT MSVC)
  target_compile_options(l3cook_map_init PRIVATE -Wall -Wextra -Wpedantic -Werror)
  target_compile_options(test_map_initialization PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
endif()
add_test(NAME core COMMAND test_core)
add_executable(test_map_sharing tests/test_map_sharing.cpp)
target_link_libraries(test_map_sharing PRIVATE legends3ds_core)
add_test(NAME map_sharing COMMAND test_map_sharing)
add_executable(test_ps1_geometry tests/test_ps1_geometry.cpp)
target_link_libraries(test_ps1_geometry PRIVATE legends3ds_core)
add_test(NAME ps1_geometry COMMAND test_ps1_geometry)
add_executable(test_area_package tests/test_area_package.cpp)
target_link_libraries(test_area_package PRIVATE legends3ds_core)
add_test(NAME area_package COMMAND test_area_package)

add_executable(test_terrain_geometry_candidate tests/test_terrain_geometry_candidate.cpp)
target_link_libraries(test_terrain_geometry_candidate PRIVATE legends3ds_core)
add_test(NAME terrain_geometry_candidate COMMAND test_terrain_geometry_candidate)
add_executable(test_map_cell_alignment tests/test_map_cell_alignment.cpp)
target_link_libraries(test_map_cell_alignment PRIVATE legends3ds_core)
add_test(NAME map_cell_alignment COMMAND test_map_cell_alignment)
add_executable(test_area_placement tests/test_area_placement.cpp)
target_link_libraries(test_area_placement PRIVATE legends3ds_core)
add_test(NAME area_placement COMMAND test_area_placement)
add_executable(test_positive_terrain tests/test_positive_terrain.cpp)
target_link_libraries(test_positive_terrain PRIVATE legends3ds_core)
add_test(NAME positive_terrain COMMAND test_positive_terrain)
if(MSVC)
  target_compile_options(test_terrain_geometry_candidate PRIVATE /UNDEBUG)
  target_compile_options(test_map_cell_alignment PRIVATE /UNDEBUG)
  target_compile_options(test_area_placement PRIVATE /UNDEBUG)
  target_compile_options(test_positive_terrain PRIVATE /UNDEBUG)
else()
  target_compile_options(test_terrain_geometry_candidate PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
  target_compile_options(test_map_cell_alignment PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
  target_compile_options(test_area_placement PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
  target_compile_options(test_positive_terrain PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
endif()

if(MSVC)
  target_compile_options(test_core PRIVATE /UNDEBUG)
  target_compile_options(test_map_sharing PRIVATE /UNDEBUG)
  target_compile_options(test_ps1_geometry PRIVATE /UNDEBUG)
  target_compile_options(test_area_package PRIVATE /UNDEBUG)
else()
  target_compile_options(test_core PRIVATE -UNDEBUG)
  target_compile_options(test_map_sharing PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
  target_compile_options(test_ps1_geometry PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
  target_compile_options(test_area_package PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
endif()

# Push-2 cumulative subsystem bridge. These modules are compiled and regression-tested
# as a separate library so unverified behavior cannot silently alter the Classic runtime.
add_library(mml3ds_push2 STATIC
    src/p2/mml_bottom_ui_v2.c
    src/p2/mml_combat_v2.c
    src/p2/mml_cutscene_v2.c
    src/p2/mml_enemy_bridge_v2.c
    src/p2/mml_event_vm_v2.c
    src/p2/mml_h4_runtime.c
    src/p2/mml_loader_guard_v2.c
    src/p2/mml_native_save_v2.c
    src/p2/mml_package_v2.c
    src/p2/mml_player_material.c
    src/p2/mml_qol_v2.c
    src/p2/mml_renderer_recovery.c
    src/p2/mml_retail_anim_request.c
    src/p2/mml_retail_motion.c
    src/p2/mml_retail_save_v2.c
    src/p2/mml_runtime_diag_v2.c
    src/p2/mml_world_v2.c
)
target_include_directories(mml3ds_push2 PUBLIC include/legends3ds/p2)
if(NOT MSVC)
  target_compile_options(mml3ds_push2 PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_executable(test_push2 tests/p2/test_all.c)
target_link_libraries(test_push2 PRIVATE mml3ds_push2)
if(NOT MSVC)
  target_compile_options(test_push2 PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
endif()
add_test(NAME push2 COMMAND test_push2)

add_executable(l3replay tools/l3replay.cpp)
target_link_libraries(l3replay PRIVATE legends3ds_core)
add_executable(l3area_validate tools/l3area_validate.cpp)
target_link_libraries(l3area_validate PRIVATE legends3ds_core mml3ds_push2)
add_executable(l3area_runtime_probe tools/l3area_runtime_probe.cpp)
target_link_libraries(l3area_runtime_probe PRIVATE legends3ds_core)
add_executable(l3area_placement_probe tools/l3area_placement_probe.cpp)
target_link_libraries(l3area_placement_probe PRIVATE legends3ds_core)
if(NOT MSVC)
  target_compile_options(l3replay PRIVATE -Wall -Wextra -Wpedantic -Werror)
  target_compile_options(l3area_validate PRIVATE -Wall -Wextra -Wpedantic -Werror)
  target_compile_options(l3area_runtime_probe PRIVATE -Wall -Wextra -Wpedantic -Werror)
  target_compile_options(l3area_placement_probe PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_executable(test_integrity tests/p2/test_integrity.c)
target_link_libraries(test_integrity PRIVATE mml3ds_push2)
if(NOT MSVC)
  target_compile_options(test_integrity PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
endif()
add_test(NAME integrity COMMAND test_integrity)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_test(NAME pipeline_python COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_pipeline.py)
add_test(NAME cooked_area_python COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_cooked_area.py)
add_test(NAME owner_data_intake_python COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_owner_data_intake.py)
add_test(NAME local_install_python COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_local_install.py)
add_test(NAME source_tools_python COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_source_tools.py)
add_executable(l3animation_evidence tools/l3animation_evidence.cpp)
target_link_libraries(l3animation_evidence PRIVATE legends3ds_core)
if(NOT MSVC)
  target_compile_options(l3animation_evidence PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_executable(test_negative_package tests/test_negative_package.cpp)
target_link_libraries(test_negative_package PRIVATE legends3ds_core)
add_test(NAME negative_package COMMAND test_negative_package)
if(NOT MSVC)
  target_compile_options(test_negative_package PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
else()
  target_compile_options(test_negative_package PRIVATE /UNDEBUG)
endif()
add_executable(test_negative_terrain tests/test_negative_terrain.cpp)
target_link_libraries(test_negative_terrain PRIVATE legends3ds_core)
add_test(NAME negative_terrain COMMAND test_negative_terrain)
if(NOT MSVC)
  target_compile_options(test_negative_terrain PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
else()
  target_compile_options(test_negative_terrain PRIVATE /UNDEBUG)
endif()
add_executable(test_negative_material tests/test_negative_material.cpp)
target_link_libraries(test_negative_material PRIVATE legends3ds_core)
add_test(NAME negative_material COMMAND test_negative_material)
if(NOT MSVC)
  target_compile_options(test_negative_material PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
else()
  target_compile_options(test_negative_material PRIVATE /UNDEBUG)
endif()
add_executable(test_material_vram tests/test_material_vram.cpp)
target_link_libraries(test_material_vram PRIVATE legends3ds_core)
add_test(NAME material_vram COMMAND test_material_vram)
if(NOT MSVC)
  target_compile_options(test_material_vram PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
else()
  target_compile_options(test_material_vram PRIVATE /UNDEBUG)
endif()

add_executable(test_texture_sector_layout tests/test_texture_sector_layout.cpp)
target_link_libraries(test_texture_sector_layout PRIVATE legends3ds_core)
add_test(NAME texture_sector_layout COMMAND test_texture_sector_layout)
if(NOT MSVC)
  target_compile_options(test_texture_sector_layout PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
else()
  target_compile_options(test_texture_sector_layout PRIVATE /UNDEBUG)
endif()
add_executable(test_area_inspection tests/test_area_inspection.cpp)
target_link_libraries(test_area_inspection PRIVATE legends3ds_core)
add_test(NAME area_inspection COMMAND test_area_inspection)
if(NOT MSVC)
  target_compile_options(test_area_inspection PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
else()
  target_compile_options(test_area_inspection PRIVATE /UNDEBUG)
endif()
add_executable(l3cook_negative_materials tools/l3cook_negative_materials.cpp)
target_link_libraries(l3cook_negative_materials PRIVATE legends3ds_core)
add_executable(l3area_inspection_probe tools/l3area_inspection_probe.cpp)
target_link_libraries(l3area_inspection_probe PRIVATE legends3ds_core)
add_executable(l3area_visibility_audit tools/l3area_visibility_audit.cpp)
target_link_libraries(l3area_visibility_audit PRIVATE legends3ds_core)

add_executable(test_frontend_view tests/test_frontend_view.cpp)
target_link_libraries(test_frontend_view PRIVATE legends3ds_core)
add_test(NAME frontend_view COMMAND test_frontend_view)
add_executable(test_mockup_ui tests/test_mockup_ui.cpp platform/3ds/source/mockup_hud.cpp)
target_link_libraries(test_mockup_ui PRIVATE legends3ds_core)
target_include_directories(test_mockup_ui PRIVATE include/legends3ds/p2)
add_test(NAME mockup_ui COMMAND test_mockup_ui)
if(MSVC)
  target_compile_options(test_frontend_view PRIVATE /UNDEBUG)
  target_compile_options(test_mockup_ui PRIVATE /UNDEBUG)
else()
  target_compile_options(test_frontend_view PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
  target_compile_options(test_mockup_ui PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
endif()

add_executable(test_player_visual_storage tests/test_player_visual_storage.cpp)
target_link_libraries(test_player_visual_storage PRIVATE legends3ds_core)
add_test(NAME player_visual_storage COMMAND test_player_visual_storage)
if(MSVC)
  target_compile_options(test_player_visual_storage PRIVATE /UNDEBUG)
else()
  target_compile_options(test_player_visual_storage PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
endif()

add_executable(test_ebd_resources tests/test_ebd_resources.cpp)
target_link_libraries(test_ebd_resources PRIVATE legends3ds_core)
add_test(NAME ebd_resources COMMAND test_ebd_resources)
add_executable(test_ebd_sharing tests/test_ebd_sharing.cpp)
target_link_libraries(test_ebd_sharing PRIVATE legends3ds_core)
target_compile_options(test_ebd_sharing PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME ebd_sharing COMMAND test_ebd_sharing)
add_executable(test_actor_resources tests/test_actor_resources.cpp)
target_link_libraries(test_actor_resources PRIVATE legends3ds_core)
target_compile_options(test_actor_resources PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME actor_resources COMMAND test_actor_resources)
add_executable(test_actor_materials tests/test_actor_materials.cpp)
target_link_libraries(test_actor_materials PRIVATE legends3ds_core)
target_compile_options(test_actor_materials PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME actor_materials COMMAND test_actor_materials)
add_test(NAME parity_trace_python COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_parity_trace.py)
add_executable(test_companion_ui tests/test_companion_ui.cpp platform/3ds/source/mockup_hud.cpp)
target_link_libraries(test_companion_ui PRIVATE legends3ds_core)
target_include_directories(test_companion_ui PRIVATE include/legends3ds/p2)
target_compile_options(test_companion_ui PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME companion_ui COMMAND test_companion_ui)
add_executable(test_runtime_contact tests/test_runtime_contact.cpp)
target_link_libraries(test_runtime_contact PRIVATE legends3ds_core)
target_compile_options(test_runtime_contact PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME runtime_contact COMMAND test_runtime_contact ${CMAKE_CURRENT_SOURCE_DIR}/docs/validation_01415/floor08-source-cases.tsv)

find_program(L3_GCC_RENDERER_GUARD arm-none-eabi-gcc
  HINTS "$ENV{DEVKITARM}/bin" "/opt/devkitpro/devkitARM/bin")
if(L3_GCC_RENDERER_GUARD)
  add_test(NAME renderer_gcc_warning COMMAND ${Python3_EXECUTABLE} -B
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/check_renderer_gate_gcc.py
    --compiler ${L3_GCC_RENDERER_GUARD})
endif()
if(MSVC)
  target_compile_options(test_ebd_resources PRIVATE /UNDEBUG)
else()
  target_compile_options(test_ebd_resources PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
endif()

add_executable(l3probe_actor_resources tools/l3probe_actor_resources.cpp)
target_link_libraries(l3probe_actor_resources PRIVATE legends3ds_core)
add_executable(l3probe_actor_class20_03 tools/l3probe_actor_class20_03.cpp)
target_link_libraries(l3probe_actor_class20_03 PRIVATE legends3ds_core)
add_executable(test_gameplay_clock tests/test_gameplay_clock.cpp)
target_link_libraries(test_gameplay_clock PRIVATE legends3ds_core)
if(NOT MSVC)
  target_compile_options(test_gameplay_clock PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
endif()
add_test(NAME gameplay_clock COMMAND test_gameplay_clock)

add_executable(test_running_jump_source tests/test_running_jump_source.cpp)
target_link_libraries(test_running_jump_source PRIVATE legends3ds_core)
if(MSVC)
  target_compile_options(test_running_jump_source PRIVATE /UNDEBUG)
else()
  target_compile_options(test_running_jump_source PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
endif()
add_test(NAME running_jump_source COMMAND test_running_jump_source ${CMAKE_CURRENT_SOURCE_DIR}/docs/validation_01415/running-jump-source-cases.tsv)

add_executable(test_class20_03_behavior tests/test_class20_03_behavior.cpp)
target_link_libraries(test_class20_03_behavior PRIVATE legends3ds_core)
if(MSVC)
  target_compile_options(test_class20_03_behavior PRIVATE /UNDEBUG)
else()
  target_compile_options(test_class20_03_behavior PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
endif()
add_test(NAME class20_03_behavior COMMAND test_class20_03_behavior
  ${CMAKE_CURRENT_SOURCE_DIR}/docs/validation_01415/class20-03-action-source-cases.tsv
  ${CMAKE_CURRENT_SOURCE_DIR}/docs/validation_01415/class20-03-distance-source-cases.tsv)

add_executable(test_source_angle tests/test_source_angle.cpp)
target_link_libraries(test_source_angle PRIVATE legends3ds_core)
if(MSVC)
  target_compile_options(test_source_angle PRIVATE /UNDEBUG)
else()
  target_compile_options(test_source_angle PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
endif()
add_test(NAME source_angle COMMAND test_source_angle
  ${CMAKE_CURRENT_SOURCE_DIR}/docs/validation_01416/source-angle-synthetic-cases.tsv)

add_executable(test_actor_sequence tests/test_actor_sequence.cpp)
target_link_libraries(test_actor_sequence PRIVATE legends3ds_core)
if(MSVC)
  target_compile_options(test_actor_sequence PRIVATE /UNDEBUG)
else()
  target_compile_options(test_actor_sequence PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
endif()
add_test(NAME actor_sequence COMMAND test_actor_sequence
  ${CMAKE_CURRENT_SOURCE_DIR}/docs/validation_01415/actor-sequence-source-cases.tsv)

add_executable(l3cook_st00_spawn tools/l3cook_st00_spawn.cpp)
target_link_libraries(l3cook_st00_spawn PRIVATE legends3ds_core)
if(NOT MSVC)
  target_compile_options(l3cook_st00_spawn PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_executable(test_st00_spawn tests/test_st00_spawn.cpp)
target_link_libraries(test_st00_spawn PRIVATE legends3ds_core)
add_test(NAME st00_spawn COMMAND test_st00_spawn)
if(NOT MSVC)
  target_compile_options(test_st00_spawn PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
endif()

add_executable(test_runtime_spawns tests/test_runtime_spawns.cpp)
target_link_libraries(test_runtime_spawns PRIVATE legends3ds_core)
if(NOT MSVC)
  target_compile_options(test_runtime_spawns PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
endif()
add_test(NAME runtime_spawns COMMAND test_runtime_spawns)

add_executable(l3runtime_spawn_probe tools/l3runtime_spawn_probe.cpp)
target_link_libraries(l3runtime_spawn_probe PRIVATE legends3ds_core)

add_executable(test_map_initialization_package tests/test_map_initialization_package.cpp)
target_link_libraries(test_map_initialization_package PRIVATE legends3ds_core)
add_test(NAME map_initialization_package COMMAND test_map_initialization_package)
if(NOT MSVC)
  target_compile_options(test_map_initialization_package PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
endif()

add_executable(test_runtime_transactions tests/test_runtime_transactions.cpp)
target_link_libraries(test_runtime_transactions PRIVATE legends3ds_core)
add_test(NAME runtime_transactions COMMAND test_runtime_transactions)
if(NOT MSVC)
  target_compile_options(test_runtime_transactions PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
endif()

add_executable(test_actor_floor_init tests/test_actor_floor_init.cpp)
target_link_libraries(test_actor_floor_init PRIVATE legends3ds_core)
add_test(NAME actor_floor_init COMMAND test_actor_floor_init)
if(NOT MSVC)
  target_compile_options(test_actor_floor_init PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
endif()

add_executable(l3actor_animation_bank_probe tools/l3actor_animation_bank_probe.cpp)
target_link_libraries(l3actor_animation_bank_probe PRIVATE legends3ds_core)
target_compile_options(l3actor_animation_bank_probe PRIVATE -Wall -Wextra -Wpedantic -Werror)
add_executable(l3test_actor_animation_bank tests/test_actor_animation_bank.cpp)
target_link_libraries(l3test_actor_animation_bank PRIVATE legends3ds_core)
target_compile_options(l3test_actor_animation_bank PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME actor_animation_bank COMMAND l3test_actor_animation_bank)

add_executable(l3test_actor_behavior_runtime tests/test_actor_behavior_runtime.cpp)
target_link_libraries(l3test_actor_behavior_runtime PRIVATE legends3ds_core)
target_compile_options(l3test_actor_behavior_runtime PRIVATE -Wall -Wextra -Wpedantic -Werror)
add_test(NAME actor_behavior_runtime COMMAND l3test_actor_behavior_runtime)
add_executable(l3test_class20_03_callback tests/test_class20_03_callback.cpp)
target_link_libraries(l3test_class20_03_callback PRIVATE legends3ds_core)
target_compile_options(l3test_class20_03_callback PRIVATE -Wall -Wextra -Wpedantic -Werror)
add_test(NAME class20_03_callback COMMAND l3test_class20_03_callback ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/class20_03_transactions_01418.tsv)

add_executable(l3test_actor_contact tests/test_actor_contact.cpp)
target_link_libraries(l3test_actor_contact PRIVATE legends3ds_core)
target_compile_options(l3test_actor_contact PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME actor_contact COMMAND l3test_actor_contact ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/contact_01418/authored)
add_executable(l3actor_behavior_runtime_probe tools/l3actor_behavior_runtime_probe.cpp)
target_link_libraries(l3actor_behavior_runtime_probe PRIVATE legends3ds_core)
target_compile_options(l3actor_behavior_runtime_probe PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)

add_executable(test_actor_detection tests/test_actor_detection.cpp)
target_link_libraries(test_actor_detection PRIVATE legends3ds_core)
target_compile_options(test_actor_detection PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME actor_detection COMMAND test_actor_detection ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/detection_01419/authored-detection.tsv ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/detection_01419/authored-globals.tsv)
add_executable(test_actor_detection_profile tests/test_actor_detection_profile.cpp)
target_link_libraries(test_actor_detection_profile PRIVATE legends3ds_core)
target_compile_options(test_actor_detection_profile PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME actor_detection_profile COMMAND test_actor_detection_profile)
add_executable(test_class20_03_action3 tests/test_class20_03_action3.cpp)
target_link_libraries(test_class20_03_action3 PRIVATE legends3ds_core)
target_compile_options(test_class20_03_action3 PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME class20_03_action3 COMMAND test_class20_03_action3 ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/class20_03_action3_01419.tsv)

add_executable(test_actor_detection_math tests/test_actor_detection_math.cpp)
target_link_libraries(test_actor_detection_math PRIVATE legends3ds_core)
target_compile_options(test_actor_detection_math PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME actor_detection_math COMMAND test_actor_detection_math ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/detection_01419/authored-normalization.tsv ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/detection_01419/authored-interpolation.tsv)

add_executable(test_actor_detection_segment tests/test_actor_detection_segment.cpp)
target_link_libraries(test_actor_detection_segment PRIVATE legends3ds_core)
target_compile_options(test_actor_detection_segment PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME actor_detection_segment COMMAND test_actor_detection_segment ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/actor_detection_segment_01419.tsv)

add_executable(test_source_audio_queue tests/test_source_audio_queue.cpp)
target_link_libraries(test_source_audio_queue PRIVATE legends3ds_core)
target_compile_options(test_source_audio_queue PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME source_audio_queue COMMAND test_source_audio_queue ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/detection_01419/authored-audio-queue.tsv)

foreach(t actor_detection_preparation actor_action3_contact source_combat_profile)
  add_executable(test_${t} tests/test_${t}.cpp)
  target_link_libraries(test_${t} PRIVATE legends3ds_core)
  target_compile_options(test_${t} PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
endforeach()
add_test(NAME actor_detection_preparation COMMAND test_actor_detection_preparation ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/detection_01419/authored-preparation.tsv)
add_test(NAME actor_action3_contact COMMAND test_actor_action3_contact ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/actor_action3_contact_01419.tsv ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/actor_sound_pointed_01419.tsv)
add_test(NAME source_combat_profile COMMAND test_source_combat_profile)
add_test(NAME cook_actor_combat COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_cook_actor_combat.py)
add_executable(test_runtime_source_context tests/test_runtime_source_context.cpp)
target_link_libraries(test_runtime_source_context PRIVATE legends3ds_core)
target_compile_options(test_runtime_source_context PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME runtime_source_context COMMAND test_runtime_source_context)
add_executable(test_actor_source_transaction tests/test_actor_source_transaction.cpp)
target_link_libraries(test_actor_source_transaction PRIVATE legends3ds_core)
target_compile_options(test_actor_source_transaction PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME actor_source_transaction COMMAND test_actor_source_transaction)

foreach(t class20_03_action9 actor_action9_contact actor_detection_segment_extended)
  add_executable(test_${t} tests/test_${t}.cpp)
  target_link_libraries(test_${t} PRIVATE legends3ds_core)
  target_compile_options(test_${t} PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
endforeach()
add_test(NAME class20_03_action9 COMMAND test_class20_03_action9 ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/class20_03_action9_01420.tsv)
add_test(NAME actor_action9_contact COMMAND test_actor_action9_contact ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/actor_action9_contact_01420.tsv)
add_test(NAME actor_detection_segment_extended COMMAND test_actor_detection_segment_extended ${CMAKE_CURRENT_SOURCE_DIR}/docs/research_01420/actor-segment-extended-authored.tsv)

add_executable(test_actor_action9_runtime tests/test_actor_action9_runtime.cpp)
target_link_libraries(test_actor_action9_runtime PRIVATE legends3ds_core)
target_compile_options(test_actor_action9_runtime PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME actor_action9_runtime COMMAND test_actor_action9_runtime)

add_test(NAME rng_reference_trace COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_rng_reference.py)

add_executable(test_retail_rng_tape tests/test_retail_rng_tape.cpp)
target_link_libraries(test_retail_rng_tape PRIVATE legends3ds_core)
target_compile_options(test_retail_rng_tape PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME retail_rng_tape COMMAND test_retail_rng_tape)

add_executable(test_actor_action4_runtime tests/test_actor_action4_runtime.cpp)
target_link_libraries(test_actor_action4_runtime PRIVATE legends3ds_core)
target_compile_options(test_actor_action4_runtime PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME actor_action4_runtime COMMAND test_actor_action4_runtime)

add_executable(test_class20_03_action4 tests/test_class20_03_action4.cpp)
target_link_libraries(test_class20_03_action4 PRIVATE legends3ds_core)
target_compile_options(test_class20_03_action4 PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME class20_03_action4 COMMAND test_class20_03_action4
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/class20_03_action4_01421.tsv
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/class60_child_pool_01421.tsv
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/class60_child_release_01421.tsv)
add_executable(test_actor_action4_contact tests/test_actor_action4_contact.cpp)
target_link_libraries(test_actor_action4_contact PRIVATE legends3ds_core)
target_compile_options(test_actor_action4_contact PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME actor_action4_contact COMMAND test_actor_action4_contact
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/actor_action4_contact_01421.tsv)

foreach(t class60_06 source_profile_class60_06 class20_03_hit source_event_pipeline)
    add_executable(test_${t} tests/test_${t}.cpp)
    target_link_libraries(test_${t} PRIVATE legends3ds_core)
    target_compile_options(test_${t} PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
endforeach()
add_test(NAME source_profile_class60_06 COMMAND test_source_profile_class60_06)
add_test(NAME class20_03_hit COMMAND test_class20_03_hit ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/class20_03_hit_01422.tsv)
add_test(NAME source_event_pipeline COMMAND test_source_event_pipeline ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/source_event_pipeline_01422.tsv)

add_executable(test_source_hit_proxy tests/test_source_hit_proxy.cpp)
target_link_libraries(test_source_hit_proxy PRIVATE legends3ds_core)
target_compile_options(test_source_hit_proxy PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME source_hit_proxy COMMAND test_source_hit_proxy ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/source_hit_proxy_01422.tsv)

foreach(t class60_06_contact source_actor_consumers runtime_source_events)
    add_executable(test_${t} tests/test_${t}.cpp)
    target_link_libraries(test_${t} PRIVATE legends3ds_core)
    target_compile_options(test_${t} PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
endforeach()
add_test(NAME class60_06 COMMAND test_class60_06
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/class60_06_callback_01422.tsv
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/class60_06_motion_01422.tsv
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/class60_06_primitive_01422.tsv)
add_test(NAME class60_06_contact COMMAND test_class60_06_contact ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/class60_06_contact_01422.tsv)
add_test(NAME source_actor_consumers COMMAND test_source_actor_consumers
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/source_collision_registry_01422.tsv
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/source_shadow_01422.tsv)
add_test(NAME runtime_source_events COMMAND test_runtime_source_events)
add_test(NAME cook_hit_proxy COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_cook_hit_proxy.py)

add_executable(test_source_rng_service tests/test_source_rng_service.cpp)
target_link_libraries(test_source_rng_service PRIVATE legends3ds_core)
target_compile_options(test_source_rng_service PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME source_rng_service COMMAND test_source_rng_service ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/source_rng_openbios_01422.tsv)
add_test(NAME cook_source_rng_profile COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_cook_source_rng_profile.py)
add_executable(test_source_event_consumer tests/test_source_event_consumer.cpp)
target_link_libraries(test_source_event_consumer PRIVATE legends3ds_core)
target_compile_options(test_source_event_consumer PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME source_event_consumer COMMAND test_source_event_consumer ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/source_event_consumer_01422.tsv)

add_executable(test_child_runtime tests/test_child_runtime.cpp)
target_link_libraries(test_child_runtime PRIVATE legends3ds_core)
target_compile_options(test_child_runtime PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME child_runtime COMMAND test_child_runtime ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/class60_shadow_floor_01422.tsv)

add_executable(test_actor_child_delivery tests/test_actor_child_delivery.cpp)
target_link_libraries(test_actor_child_delivery PRIVATE legends3ds_core)
target_compile_options(test_actor_child_delivery PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME actor_child_delivery COMMAND test_actor_child_delivery)

add_executable(test_actor_defeat_runtime tests/test_actor_defeat_runtime.cpp)
target_link_libraries(test_actor_defeat_runtime PRIVATE legends3ds_core)
target_compile_options(test_actor_defeat_runtime PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME actor_defeat_runtime COMMAND test_actor_defeat_runtime)

add_executable(test_source_event_package tests/test_source_event_package.cpp)
target_link_libraries(test_source_event_package PRIVATE legends3ds_core)
target_compile_options(test_source_event_package PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME source_event_package COMMAND test_source_event_package)
add_test(NAME cook_source_event_phase2 COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_cook_source_event_phase2.py)

foreach(t actor_pose_proxy source_effects source_event_phase2)
    add_executable(test_${t} tests/test_${t}.cpp)
    target_link_libraries(test_${t} PRIVATE legends3ds_core)
    target_compile_options(test_${t} PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
endforeach()
add_test(NAME actor_pose_proxy COMMAND test_actor_pose_proxy
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/actor_pose_collision_01423.tsv
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/class20_03_proxy_01423.tsv)
add_test(NAME source_effects COMMAND test_source_effects ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/source_effects_01423 ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/source_effects_01436/callback-known-lanes.tsv)

add_executable(test_source_effect2 tests/test_source_effect2.cpp)
target_link_libraries(test_source_effect2 PRIVATE legends3ds_core)
target_compile_options(test_source_effect2 PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME source_effect2 COMMAND test_source_effect2 ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/source_effect2_01438/callback-authored.tsv)
add_test(NAME source_event_phase2 COMMAND test_source_event_phase2 ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/source_event_phase2_01423.tsv)
add_executable(test_actor_pose_proxy_runtime tests/test_actor_pose_proxy_runtime.cpp)
target_link_libraries(test_actor_pose_proxy_runtime PRIVATE legends3ds_core)
target_compile_options(test_actor_pose_proxy_runtime PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME actor_pose_proxy_runtime COMMAND test_actor_pose_proxy_runtime)

foreach(t source_event_command source_event_command_package source_player_proxy)
    add_executable(test_${t} tests/test_${t}.cpp)
    target_link_libraries(test_${t} PRIVATE legends3ds_core)
    target_compile_options(test_${t} PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
endforeach()
add_test(NAME source_event_command COMMAND test_source_event_command ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/source_event_command_01424.tsv)
add_test(NAME source_event_command_package COMMAND test_source_event_command_package)
add_test(NAME source_player_proxy COMMAND test_source_player_proxy ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/source_player_lease_01424.tsv)
add_executable(test_source_player_buster tests/test_source_player_buster.cpp)
target_link_libraries(test_source_player_buster PRIVATE legends3ds_core)
target_compile_options(test_source_player_buster PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME source_player_buster COMMAND test_source_player_buster)
add_executable(test_source_player_pose tests/test_source_player_pose.cpp)
target_link_libraries(test_source_player_pose PRIVATE legends3ds_core)
target_compile_options(test_source_player_pose PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME source_player_pose COMMAND test_source_player_pose)
add_executable(test_source_buster_profile tests/test_source_buster_profile.cpp)
target_link_libraries(test_source_buster_profile PRIVATE legends3ds_core)
target_compile_options(test_source_buster_profile PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME source_buster_profile COMMAND test_source_buster_profile)
add_executable(test_player_fire_state tests/test_player_fire_state.cpp)
target_link_libraries(test_player_fire_state PRIVATE legends3ds_core)
target_compile_options(test_player_fire_state PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME player_fire_state COMMAND test_player_fire_state)
add_test(NAME cook_source_event_command COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_cook_source_event_command.py)
add_test(NAME player_lease_reference COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_player_lease_reference.py)

add_executable(test_runtime_command_lease tests/test_runtime_command_lease.cpp)
target_link_libraries(test_runtime_command_lease PRIVATE legends3ds_core)
target_compile_options(test_runtime_command_lease PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME runtime_command_lease COMMAND test_runtime_command_lease)

add_executable(test_xa_stream tests/test_xa_stream.cpp)
target_link_libraries(test_xa_stream PRIVATE legends3ds_core)
target_compile_options(test_xa_stream PRIVATE -Wall -Wextra -Wpedantic -Werror)
add_test(NAME xa_stream COMMAND test_xa_stream)
add_executable(test_source_camera tests/test_source_camera.cpp)
target_link_libraries(test_source_camera PRIVATE legends3ds_core)
target_compile_options(test_source_camera PRIVATE -Wall -Wextra -Wpedantic -Werror)
add_test(NAME source_camera COMMAND test_source_camera ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/source_camera_direct_01425.tsv ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/source_projection_control_01425.tsv)
add_test(NAME cook_xa_stream COMMAND ${Python3_EXECUTABLE} -B -m unittest discover -s ${CMAKE_CURRENT_SOURCE_DIR}/tests -p test_cook_xa_stream.py)

add_executable(test_source_fade tests/test_source_fade.cpp)
target_link_libraries(test_source_fade PRIVATE legends3ds_core)
target_compile_options(test_source_fade PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME source_fade COMMAND test_source_fade ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/source_fade_startup_01426.tsv)

add_executable(test_source_fade_callback tests/test_source_fade_callback.cpp)
target_link_libraries(test_source_fade_callback PRIVATE legends3ds_core)
target_compile_options(test_source_fade_callback PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME source_fade_callback COMMAND test_source_fade_callback
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/source_fade_callback_01428.tsv
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/source_fade_presentation_01428.tsv)

add_executable(test_runtime_source_fade tests/test_runtime_source_fade.cpp)
target_link_libraries(test_runtime_source_fade PRIVATE legends3ds_core)
target_compile_options(test_runtime_source_fade PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME runtime_source_fade COMMAND test_runtime_source_fade)

add_executable(test_source_event_phase5 tests/test_source_event_phase5.cpp)
target_link_libraries(test_source_event_phase5 PRIVATE legends3ds_core)
target_compile_options(test_source_event_phase5 PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME source_event_phase5 COMMAND test_source_event_phase5
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/source-event-phase5.tsv)
add_test(NAME source_event_phase5_cooker COMMAND ${Python3_EXECUTABLE} -B
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_source_event_phase5_cooker.py)

add_executable(test_source_event_phase5_resources tests/test_source_event_phase5_resources.cpp)
target_link_libraries(test_source_event_phase5_resources PRIVATE legends3ds_core)
target_compile_options(test_source_event_phase5_resources PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME source_event_phase5_resources COMMAND test_source_event_phase5_resources)

add_executable(test_source_event_phase6 tests/test_source_event_phase6.cpp)
target_link_libraries(test_source_event_phase6 PRIVATE legends3ds_core)
target_compile_options(test_source_event_phase6 PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME source_event_phase6 COMMAND test_source_event_phase6
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/source-event-phase6-prefix.tsv)
add_test(NAME source_event_phase6_cooker COMMAND ${Python3_EXECUTABLE} -B
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_source_event_phase6_cooker.py)

add_test(NAME native_trace_summary COMMAND ${Python3_EXECUTABLE} -B
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_native_trace_summary.py)

add_executable(test_test_boot_policy tests/test_test_boot_policy.cpp)
target_compile_options(test_test_boot_policy PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
target_include_directories(test_test_boot_policy PRIVATE include)
add_test(NAME test_boot_policy COMMAND test_test_boot_policy)

add_executable(test_runtime_fade tests/test_runtime_fade.cpp)
target_link_libraries(test_runtime_fade PRIVATE legends3ds_core)
target_compile_options(test_runtime_fade PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME runtime_fade COMMAND test_runtime_fade)

add_test(NAME source_player_head COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_source_player_head.py)

foreach(t source_event_caller scene_e0)
  add_executable(test_${t} tests/test_${t}.cpp)
  target_link_libraries(test_${t} PRIVATE legends3ds_core)
  target_compile_options(test_${t} PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
endforeach()
add_test(NAME source_event_caller COMMAND test_source_event_caller
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/source-event-caller-predicate.tsv)
add_test(NAME scene_e0 COMMAND test_scene_e0
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/scene_e0_initializer.tsv)

add_executable(test_source_render_color tests/test_source_render_color.cpp)
target_link_libraries(test_source_render_color PRIVATE legends3ds_core)
target_compile_options(test_source_render_color PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME source_render_color COMMAND test_source_render_color
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/source_render_color_01430.tsv)

add_executable(l3cook_scene_e0_material tools/l3cook_scene_e0_material.cpp)
target_link_libraries(l3cook_scene_e0_material PRIVATE legends3ds_core)
target_compile_options(l3cook_scene_e0_material PRIVATE -Wall -Wextra -Wpedantic -Werror)
add_executable(test_scene_e0_material tests/test_scene_e0_material.cpp)
target_link_libraries(test_scene_e0_material PRIVATE legends3ds_core)
target_compile_options(test_scene_e0_material PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME scene_e0_material COMMAND test_scene_e0_material)

add_executable(test_source_scene_view tests/test_source_scene_view.cpp)
target_link_libraries(test_source_scene_view PRIVATE legends3ds_core)
target_compile_options(test_source_scene_view PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME source_scene_view COMMAND test_source_scene_view ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/source-scene-view.tsv)
add_test(NAME source_scene_view_cooker COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_source_scene_view_cooker.py)

add_executable(test_class20_03_reaction tests/test_class20_03_reaction.cpp)
target_link_libraries(test_class20_03_reaction PRIVATE legends3ds_core)
target_compile_options(test_class20_03_reaction PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME class20_03_reaction COMMAND test_class20_03_reaction)
add_executable(test_source_main_loop tests/test_source_main_loop.cpp)
target_link_libraries(test_source_main_loop PRIVATE legends3ds_core)
target_compile_options(test_source_main_loop PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME source_main_loop COMMAND test_source_main_loop ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/main_loop_01432)

add_executable(test_source_scene_actor40 tests/test_source_scene_actor40.cpp)
target_link_libraries(test_source_scene_actor40 PRIVATE legends3ds_core)
target_compile_options(test_source_scene_actor40 PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME source_scene_actor40 COMMAND test_source_scene_actor40 ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/source-scene-actor40-script-01432.tsv)

add_executable(test_runtime_source_pad tests/test_runtime_source_pad.cpp)
target_link_libraries(test_runtime_source_pad PRIVATE legends3ds_core)
target_compile_options(test_runtime_source_pad PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME runtime_source_pad COMMAND test_runtime_source_pad)

add_executable(test_source_scene_actor40_pose tests/test_source_scene_actor40_pose.cpp)
target_link_libraries(test_source_scene_actor40_pose PRIVATE legends3ds_core)
target_compile_options(test_source_scene_actor40_pose PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME source_scene_actor40_pose COMMAND test_source_scene_actor40_pose ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/source-scene-actor40-pose-01433.tsv)
add_executable(l3cook_scene_actor40_material tools/l3cook_scene_actor40_material.cpp)
target_link_libraries(l3cook_scene_actor40_material PRIVATE legends3ds_core)
target_compile_options(l3cook_scene_actor40_material PRIVATE -Wall -Wextra -Wpedantic -Werror)

add_executable(l3probe_source_scene_actor40_pose tools/l3probe_source_scene_actor40_pose.cpp)
target_link_libraries(l3probe_source_scene_actor40_pose PRIVATE legends3ds_core)
target_compile_options(l3probe_source_scene_actor40_pose PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)

add_executable(test_source_scene_actor40_init tests/test_source_scene_actor40_init.cpp)
target_link_libraries(test_source_scene_actor40_init PRIVATE legends3ds_core)
target_compile_options(test_source_scene_actor40_init PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME source_scene_actor40_init COMMAND test_source_scene_actor40_init ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/source-scene-actor40-mirrors-01434.tsv)
add_executable(test_source_scene_actor40_update tests/test_source_scene_actor40_update.cpp)
target_link_libraries(test_source_scene_actor40_update PRIVATE legends3ds_core)
target_compile_options(test_source_scene_actor40_update PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME source_scene_actor40_update COMMAND test_source_scene_actor40_update)
add_executable(test_source_scene_actor40_tail tests/test_source_scene_actor40_tail.cpp)
target_link_libraries(test_source_scene_actor40_tail PRIVATE legends3ds_core)
target_compile_options(test_source_scene_actor40_tail PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME source_scene_actor40_tail COMMAND test_source_scene_actor40_tail ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/source-scene-actor40-tail-mirrors-01436.tsv)

add_executable(l3probe_source_scene_actor40_init tools/l3probe_source_scene_actor40_init.cpp)
target_link_libraries(l3probe_source_scene_actor40_init PRIVATE legends3ds_core)
target_compile_options(l3probe_source_scene_actor40_init PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)

add_executable(test_source_loop_snapshot tests/test_source_loop_snapshot.cpp)
target_link_libraries(test_source_loop_snapshot PRIVATE legends3ds_core)
target_compile_options(test_source_loop_snapshot PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME source_loop_snapshot COMMAND test_source_loop_snapshot)
add_test(NAME source_loop_snapshot_cook COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_source_loop_snapshot_cook.py)
add_executable(test_runtime_source_loop_snapshot tests/test_runtime_source_loop_snapshot.cpp)
target_link_libraries(test_runtime_source_loop_snapshot PRIVATE legends3ds_core)
target_compile_options(test_runtime_source_loop_snapshot PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME runtime_source_loop_snapshot COMMAND test_runtime_source_loop_snapshot)

add_executable(test_source_event_random_effect tests/test_source_event_random_effect.cpp)
target_link_libraries(test_source_event_random_effect PRIVATE legends3ds_core)
target_compile_options(test_source_event_random_effect PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME source_event_random_effect COMMAND test_source_event_random_effect)

add_executable(test_source_event_random_tape tests/test_source_event_random_tape.cpp)
target_link_libraries(test_source_event_random_tape PRIVATE legends3ds_core)
target_compile_options(test_source_event_random_tape PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME source_event_random_tape COMMAND test_source_event_random_tape)

add_test(NAME source_scene_rng_cook COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_source_scene_rng_cook.py)
add_executable(test_runtime_source_scene_rng tests/test_runtime_source_scene_rng.cpp)
target_link_libraries(test_runtime_source_scene_rng PRIVATE legends3ds_core)
target_compile_options(test_runtime_source_scene_rng PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME runtime_source_scene_rng COMMAND test_runtime_source_scene_rng)
add_test(NAME native_checkpoint_version COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_native_checkpoint_version.py)

add_executable(test_source_effect_dispatch tests/test_source_effect_dispatch.cpp)
target_link_libraries(test_source_effect_dispatch PRIVATE legends3ds_core)
target_compile_options(test_source_effect_dispatch PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME source_effect_dispatch COMMAND test_source_effect_dispatch)

add_executable(test_source_effect34 tests/test_source_effect34.cpp)
target_link_libraries(test_source_effect34 PRIVATE legends3ds_core)
target_compile_options(test_source_effect34 PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME source_effect34 COMMAND test_source_effect34)

add_executable(test_source_prop_attach tests/test_source_prop_attach.cpp)
target_link_libraries(test_source_prop_attach PRIVATE legends3ds_core)
target_compile_options(test_source_prop_attach PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME source_prop_attach COMMAND test_source_prop_attach ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/source_prop_attach_01439/attachment-authored.tsv)

add_executable(test_source_prop_lift tests/test_source_prop_lift.cpp)
target_link_libraries(test_source_prop_lift PRIVATE legends3ds_core)
target_compile_options(test_source_prop_lift PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME source_prop_lift COMMAND test_source_prop_lift ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/source_prop_lift_01439/callback-authored.tsv)

add_executable(test_source_prop_foreign_globals tests/test_source_prop_foreign_globals.cpp)
target_link_libraries(test_source_prop_foreign_globals PRIVATE legends3ds_core)
target_compile_options(test_source_prop_foreign_globals PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME source_prop_foreign_globals COMMAND test_source_prop_foreign_globals)

add_executable(test_source_event_handoff tests/test_source_event_handoff.cpp)
target_link_libraries(test_source_event_handoff PRIVATE legends3ds_core)
target_compile_options(test_source_event_handoff PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME source_event_handoff COMMAND test_source_event_handoff
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/source_event_handoff_01440.tsv
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/source_handoff_spawn_01444.tsv)

add_executable(test_gpu_frame_wait tests/test_gpu_frame_wait.cpp)
target_link_libraries(test_gpu_frame_wait PRIVATE legends3ds_core)
if(MSVC)
  target_compile_options(test_gpu_frame_wait PRIVATE /UNDEBUG)
else()
  target_compile_options(test_gpu_frame_wait PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
endif()
add_test(NAME gpu_frame_wait COMMAND test_gpu_frame_wait)

add_executable(test_gameplay_gates tests/test_gameplay_gates.cpp)
target_link_libraries(test_gameplay_gates PRIVATE legends3ds_core)
target_compile_options(test_gameplay_gates PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME gameplay_gates COMMAND test_gameplay_gates)

add_executable(test_combat_owner tests/test_combat_owner.cpp)
target_link_libraries(test_combat_owner PRIVATE legends3ds_core)
target_compile_options(test_combat_owner PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
add_test(NAME combat_owner COMMAND test_combat_owner)
