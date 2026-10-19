.SUFFIXES:

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. Install devkitPro/devkitARM first.")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITARM)/3ds_rules

TARGET      := Legends3DS_01510_WorldComesAlive
BUILD       := build_3ds
SOURCES     := source \
               ../../src/world ../../src/model ../../src/texture ../../src/render \
               ../../src/animation ../../src/flow ../../src/input ../../src/player ../../src/ui ../../src/p2
INCLUDES    := ../../include ../../include/legends3ds/p2

ARCH        := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS      := -g -Wall -Wextra -O2 -mword-relocations -ffunction-sections -fdata-sections $(ARCH)
CFLAGS      += $(INCLUDE) -D__3DS__
# Core runtime uses C++ exceptions for cooker/runtime validation. Do NOT copy the
# usual sample project's -fno-exceptions flag here.
CXXFLAGS    := $(CFLAGS) -std=gnu++17
ASFLAGS     := -g $(ARCH)
LDFLAGS     := -specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map),--gc-sections
LIBS        := -lcitro3d -lctru -lm
LIBDIRS     := $(CTRULIB)
APP_TITLE   := Legends3DS 0.0.15.10 Source Lock-on
APP_DESCRIPTION := Automatic top-screen diagnostics
APP_AUTHOR  := Legends3DS Project

ifneq ($(BUILD),$(notdir $(CURDIR)))
export OUTPUT := $(CURDIR)/$(TARGET)
export TOPDIR := $(CURDIR)
export VPATH  := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)

# Keep the first hardware probe intentionally small: compile only the portable
# runtime pieces it actually exercises, rather than pulling host-only catalog/
# filesystem utilities onto Old 3DS.
CFILES       := tile_transform.c player_runtime.c player_collision.c qol_settings.c mml_renderer_recovery.c mml_runtime_diag_v2.c
CPPFILES     := main.cpp hardware_test.cpp mockup_hud.cpp mockup_ui.cpp frontend_view.cpp terrain_geometry_candidate.cpp positive_terrain.cpp negative_terrain.cpp negative_materials.cpp area_inspection.cpp area_placement.cpp terrain_descriptor_patch.cpp area_package.cpp map_initialization.cpp l3map.cpp st00_render.cpp pbd_model.cpp player_composition.cpp \
                pbd_animation.cpp player_pose.cpp visual_bridge.cpp ctr_texture_layout.cpp reference_tables.cpp \
                runtime_session.cpp tutorial_runtime.cpp mml_message.cpp st00_flow.cpp \
                st00_event.cpp source_door_event.cpp st00_spawn.cpp ebd_directory.cpp actor_resource.cpp actor_contact.cpp actor_detection.cpp actor_detection_profile.cpp actor_detection_math.cpp actor_detection_segment.cpp actor_detection_preparation.cpp actor_action9_contact.cpp actor_action4_contact.cpp actor_action3_contact.cpp source_combat_profile.cpp source_audio_queue.cpp class20_03_action9.cpp class20_03_action4.cpp class60_06.cpp source_profile_class60_06.cpp class20_03_hit.cpp source_hit_proxy.cpp source_actor_consumers.cpp child_runtime.cpp source_rng_service.cpp source_fade.cpp source_event_phase5.cpp source_event_phase6.cpp source_event_random_effect.cpp source_event_random_tape.cpp scene_e0.cpp scene_e0_material.cpp source_scene_view.cpp source_scene_actor40.cpp source_scene_actor40_pose.cpp source_scene_actor40_init.cpp source_scene_actor40_update.cpp source_scene_actor40_tail.cpp source_main_loop.cpp source_loop_snapshot.cpp class20_03_reaction.cpp source_reaction_profile.cpp actor_reaction_contact.cpp source_event_pipeline.cpp source_event_consumer.cpp source_event_phase2.cpp source_event_package.cpp source_event_command.cpp source_event_command_package.cpp source_player_proxy.cpp source_player_buster.cpp gameplay_gates.cpp source_player_pose.cpp source_buster_profile.cpp source_camera.cpp source_gameplay_camera.cpp xa_stream.cpp xa_transport.cpp native_xa_audio.cpp source_effects.cpp source_effect2.cpp source_effect34.cpp source_effect_dispatch.cpp source_prop_lift.cpp source_prop_attach.cpp actor_pose_runtime.cpp class20_03_proxy.cpp class20_03_action3.cpp actor_runtime.cpp actor_material.cpp class20_03_behavior.cpp source_angle.cpp actor_sequence.cpp actor_animation_bank.cpp player_contact_runtime.cpp stage_runtime.cpp stage_spawn.cpp \
                bottom_screen.cpp bottom_map.cpp dialogue_history.cpp material_cache.cpp material_resolver.cpp ps1_texture.cpp
SFILES       :=
PICAFILES    := vshader0143.v.pica vshader_uv.v.pica

export LD := $(CXX)
export OFILES_SOURCES := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES_BIN := $(PICAFILES:.v.pica=.shbin.o)
export OFILES := $(OFILES_BIN) $(OFILES_SOURCES)
export HFILES := $(PICAFILES:.v.pica=_shbin.h)
export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
                  $(foreach dir,$(LIBDIRS),-I$(dir)/include) -I$(CURDIR)/$(BUILD)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)
export _3DSXDEPS := $(OUTPUT).smdh
export _3DSXFLAGS += --smdh=$(CURDIR)/$(TARGET).smdh

.PHONY: all clean
all: $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile
$(BUILD):
	@mkdir -p $@
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).3dsx $(TARGET).smdh $(TARGET).elf $(TARGET).map

else
# Keep the renderer-gate enum bounds warning fatal on genuine native GCC.
# Host Clang does not diagnose GCC's unsigned-enum -Wtype-limits case.
mml_renderer_recovery.o: CFLAGS += -Werror
source_event_command.o xa_transport.o native_xa_audio.o player_contact_runtime.o source_door_event.o: CXXFLAGS += -Werror -fstack-usage
scene_e0.o scene_e0_material.o source_scene_view.o source_scene_actor40.o source_scene_actor40_pose.o source_scene_actor40_init.o source_scene_actor40_update.o source_scene_actor40_tail.o source_main_loop.o source_loop_snapshot.o class20_03_reaction.o source_reaction_profile.o actor_reaction_contact.o source_fade.o source_event_phase5.o source_event_phase6.o source_event_random_effect.o source_event_random_tape.o source_effect2.o source_effect34.o source_effect_dispatch.o source_prop_lift.o source_prop_attach.o source_player_pose.o source_buster_profile.o source_rng_service.o runtime_session.o hardware_test.o main.o: CXXFLAGS += -Werror -fstack-usage
$(OUTPUT).3dsx: $(OUTPUT).elf $(_3DSXDEPS)
$(OFILES_SOURCES): $(HFILES)
$(OUTPUT).elf: $(OFILES)
.PRECIOUS: %.shbin
%.shbin.o %_shbin.h: %.shbin
	$(SILENTMSG) $(notdir $<)
	$(bin2o)
-include $(DEPSDIR)/*.d
endif
