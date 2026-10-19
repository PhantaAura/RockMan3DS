#include <algorithm>
#include "legends3ds/ctr_texture_layout.h"
#include "legends3ds/classic_input.h"
#include "legends3ds/classic_motion.h"
#include "legends3ds/l3map.h"
#include "legends3ds/material_resolver.h"
#include "legends3ds/material_cache.h"
#include "legends3ds/mml_message.h"
#include "legends3ds/pbd_animation.h"
#include "legends3ds/player_pose.h"
#include "legends3ds/player_runtime.h"
#include "legends3ds/player_collision.h"
#include "legends3ds/pbd_model.h"
#include "legends3ds/player_composition.h"
#include "legends3ds/ps1_texture.h"
#include "legends3ds/st00_render.h"
#include "legends3ds/st00_flow.h"
#include "legends3ds/stage_spawn.h"
#include "legends3ds/stage_runtime.h"
#include "legends3ds/stage_catalog.h"
#include "legends3ds/tutorial_runtime.h"
#include "legends3ds/ebd_directory.h"
#include "legends3ds/ebd_dynamic_selector.h"
#include "legends3ds/endian_io.h"
#include "legends3ds/st00_event.h"
#include "legends3ds/tile_transform.h"
#include "legends3ds/visual_bridge.h"
#include "legends3ds/qol_settings.h"
#include "legends3ds/dialogue_history.h"
#include "legends3ds/bottom_screen.h"
#include "legends3ds/bottom_map.h"
#include "legends3ds/save_state.h"
#include "legends3ds/runtime_session.h"
#include "legends3ds/reference_tables.h"
#include <array>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <iostream>
#include <cmath>
#include <vector>

static void put16(std::vector<uint8_t>& b, size_t o, uint16_t v) {
    b[o] = static_cast<uint8_t>(v);
    b[o + 1] = static_cast<uint8_t>(v >> 8);
}
static void put32(std::vector<uint8_t>& b, size_t o, uint32_t v) {
    b[o] = static_cast<uint8_t>(v);
    b[o + 1] = static_cast<uint8_t>(v >> 8);
    b[o + 2] = static_cast<uint8_t>(v >> 16);
    b[o + 3] = static_cast<uint8_t>(v >> 24);
}

int main() {
    // Original player firing gate helpers (ROCK_NEO.EXE 0x800423A0 / 0x800423C8).
    L3PlayerCompatWork player_work{};
    player_work.bytes[0x175] = 0xA5;
    player_work.bytes[0x174] = 0x7E;
    player_work.bytes[0x177] = 0x7F;
    player_work.bytes[0xAC] = 0x31;
    player_work.bytes[0xA4] = 0x42;
    assert(l3_pl00_shot_is_enabled(&player_work));
    l3_pl00_shot_enable_off(&player_work);
    assert(!l3_pl00_shot_is_enabled(&player_work));
    assert(player_work.bytes[0x174] == 0 && player_work.bytes[0x177] == 0);
    assert(player_work.bytes[0x175] == 0xA4);
    assert(player_work.bytes[0x104] == 0x31 && player_work.bytes[0x100] == 0x42);
    l3_pl00_shot_enable_on(&player_work);
    assert(l3_pl00_shot_is_enabled(&player_work) && player_work.bytes[0x175] == 0xA5);

    auto pw_put16 = [](L3PlayerCompatWork& w, size_t off, uint16_t v) {
        w.bytes[off] = static_cast<uint8_t>(v);
        w.bytes[off + 1] = static_cast<uint8_t>(v >> 8);
    };
    auto pw_get16 = [](const L3PlayerCompatWork& w, size_t off) -> uint16_t {
        return static_cast<uint16_t>(w.bytes[off] | (uint16_t(w.bytes[off + 1]) << 8));
    };
    auto pw_gets16 = [&](const L3PlayerCompatWork& w, size_t off) -> int16_t {
        return static_cast<int16_t>(pw_get16(w, off));
    };

    auto pw_put32 = [](L3PlayerCompatWork& w, size_t off, int32_t v) {
        const uint32_t u = static_cast<uint32_t>(v);
        for (int i = 0; i < 4; ++i) w.bytes[off + size_t(i)] = static_cast<uint8_t>(u >> (i * 8));
    };
    auto pw_gets32 = [](const L3PlayerCompatWork& w, size_t off) -> int32_t {
        uint32_t u = 0;
        for (int i = 0; i < 4; ++i) u |= uint32_t(w.bytes[off + size_t(i)]) << (i * 8);
        return static_cast<int32_t>(u);
    };

    // USA 0x80042154: positive turn ramp and the original one-frame reversal zero crossing.
    player_work = {};
    pw_put16(player_work, 0x11C, 0x0020);
    pw_put16(player_work, 0x56, 0x0FF0);
    assert(l3_pl00_turn_step_80042154(&player_work, 0x0020, 0x10, 0x40) == 1);
    assert(pw_gets16(player_work, 0xB4) == 0x10 && pw_get16(player_work, 0x56) == 0x0000);
    assert(pw_gets16(player_work, 0xB6) == 0x10);
    pw_put16(player_work, 0x11C, 0);
    assert(l3_pl00_turn_step_80042154(&player_work, 0x0020, 0x10, 0x40) == 2);
    assert(pw_gets16(player_work, 0xB4) == 0); // reversal frame: no negative step yet
    assert(pw_get16(player_work, 0x56) == 0x0000);
    assert(l3_pl00_turn_step_80042154(&player_work, 0x0020, 0x10, 0x40) == 2);
    assert(pw_gets16(player_work, 0xB4) == -0x10 && pw_get16(player_work, 0x56) == 0x0FF0);
    assert(pw_gets16(player_work, 0xB6) == -0x10);

    // USA 0x80041F54: directional acceleration integrates into a +/-0x200 axis.
    player_work = {};
    pw_put16(player_work, 0x11C, 0x0010);
    for (int i = 0; i < 20; ++i) l3_pl00_axis_step_80041f54(&player_work, 0x0010, 0x28, 0x280);
    assert(pw_gets16(player_work, 0x10C) == 0x280);
    assert(pw_gets16(player_work, 0x108) == 0x200);
    pw_put16(player_work, 0x11C, 0);
    l3_pl00_axis_step_80041f54(&player_work, 0x0010, 0x28, 0x280);
    assert(pw_gets16(player_work, 0x10C) == 0); // exact reversal zero crossing
    l3_pl00_axis_step_80041f54(&player_work, 0x0010, 0x28, 0x280);
    assert(pw_gets16(player_work, 0x10C) == -0x28);

    // USA 0x80042044: the two-mask wrapper ignores opposing/no input and clears turn ramp.
    player_work = {};
    pw_put16(player_work, 0x128, 0x0010); pw_put16(player_work, 0x12A, 0x0020);
    pw_put16(player_work, 0x11C, 0x0020);
    assert(l3_pl00_turn_pair_80042044(&player_work, 0x10, 0x40) == 1);
    assert(pw_gets16(player_work, 0xB4) == 0x10);
    pw_put16(player_work, 0x11C, 0x0030);
    assert(l3_pl00_turn_pair_80042044(&player_work, 0x10, 0x40) == 0);
    assert(pw_gets16(player_work, 0xB4) == 0);

    // USA 0x80041EF4: pair selection mirrors global orientation explicitly.
    player_work = {};
    pw_put16(player_work, 0x124, 0x0040); pw_put16(player_work, 0x126, 0x0080);
    pw_put16(player_work, 0x11C, 0x0040);
    l3_pl00_axis_pair_80041ef4(&player_work, 0x20, 0x100, 0);
    assert(pw_gets16(player_work, 0x10C) == 0x20);
    pw_put16(player_work, 0x11C, 0);
    l3_pl00_axis_pair_80041ef4(&player_work, 0x20, 0x100, 0);
    assert(pw_gets16(player_work, 0x10C) == 0);
    pw_put16(player_work, 0x11C, 0x0080);
    l3_pl00_axis_pair_80041ef4(&player_work, 0x20, 0x100, 1);
    assert(pw_gets16(player_work, 0x10C) == 0x20);

    // USA 0x8004324C busy-state predicate.
    player_work = {};
    assert(!l3_pl00_state_busy_8004324c(&player_work));
    player_work.bytes[0x174] = 0x40;
    assert(l3_pl00_state_busy_8004324c(&player_work));
    player_work.bytes[0x174] = 0; player_work.bytes[0x177] = 0xFF;
    assert(l3_pl00_state_busy_8004324c(&player_work));

    // USA 0x80042528 class-83 helper seed: three proved source position selectors.
    player_work = {};
    pw_put16(player_work, 0x14, static_cast<uint16_t>(int16_t(-20)));
    pw_put16(player_work, 0x16, 100); pw_put16(player_work, 0x18, 300);
    pw_put16(player_work, 0x418, 10); pw_put16(player_work, 0x41A, 200); pw_put16(player_work, 0x41C, 30);
    pw_put16(player_work, 0x400, 40); pw_put16(player_work, 0x402, 500); pw_put16(player_work, 0x404, 60);
    L3Class80CompatWork helper{};
    assert(l3_pl00_seed_class83_80042528(&player_work, 0, &helper));
    assert(helper.bytes[0] == 3 && helper.bytes[2] == 0x83 && helper.bytes[3] == 0);
    assert(static_cast<int16_t>(helper.bytes[0x1C] | (uint16_t(helper.bytes[0x1D]) << 8)) == -20);
    assert(static_cast<int16_t>(helper.bytes[0x1E] | (uint16_t(helper.bytes[0x1F]) << 8)) == 116);
    assert(static_cast<int16_t>(helper.bytes[0x20] | (uint16_t(helper.bytes[0x21]) << 8)) == 300);
    helper = {}; assert(l3_pl00_seed_class83_80042528(&player_work, 1, &helper));
    assert(static_cast<int16_t>(helper.bytes[0x1C] | (uint16_t(helper.bytes[0x1D]) << 8)) == 10);
    assert(static_cast<int16_t>(helper.bytes[0x1E] | (uint16_t(helper.bytes[0x1F]) << 8)) == 216);
    helper = {}; assert(l3_pl00_seed_class83_80042528(&player_work, 2, &helper));
    assert(static_cast<int16_t>(helper.bytes[0x1E] | (uint16_t(helper.bytes[0x1F]) << 8)) == 516);
    helper = {}; assert(!l3_pl00_seed_class83_80042528(&player_work, 3, &helper));

    // Synthetic MSG: two blocks with glyphs, framing, AUDIO/WAIT/NEXT and dynamic SEL/MULTI.
    // Pointer table is [0x0004, 0x0013]. Each block ends in 00 00.
    std::vector<uint8_t> msg(0x20, 0);
    put16(msg, 0, 0x0004); put16(msg, 2, 0x0012);
    size_t mo = 0x04;
    msg[mo++] = 0x15;                 // glyph A
    msg[mo++] = 0x86;                 // newline
    msg[mo++] = 0x8F;                 // AUDIO + four args
    msg[mo++] = 1; msg[mo++] = 2; msg[mo++] = 3; msg[mo++] = 4;
    msg[mo++] = 0xA4; msg[mo++] = 0x34; msg[mo++] = 0x12; // WAIT
    msg[mo++] = 0x9F;                 // NEXT
    msg[mo++] = 0xED;                 // unknown high byte preserved, not guessed
    msg[mo++] = 0; msg[mo++] = 0;     // terminator => 0x12
    mo = 0x12;
    msg[mo++] = 0x96; msg[mo++] = 0x10; msg[mo++] = 0x20;
    msg[mo++] = 0x96; msg[mo++] = 0x30; msg[mo++] = 0x40;
    msg[mo++] = 0x97; msg[mo++] = 0x01; msg[mo++] = 0x05; msg[mo++] = 0x06;
    msg[mo++] = 0xA9;
    msg[mo++] = 0; msg[mo++] = 0;
    msg.resize(mo);
    const auto parsed_msg = l3::parse_mml_message_payload(msg);
    const auto msg_stats = l3::inspect_mml_message(parsed_msg);
    assert(parsed_msg.pointer_table_bytes == 4 && parsed_msg.blocks.size() == 2);
    assert(msg_stats.blocks == 2 && msg_stats.glyph_bytes == 1 && msg_stats.newline_bytes == 1);
    assert(msg_stats.audio_commands == 1 && msg_stats.wait_commands == 1 && msg_stats.next_commands == 1);
    assert(msg_stats.selection_commands == 2 && msg_stats.multi_commands == 1 && msg_stats.unknown_control_bytes == 1);
    assert(parsed_msg.blocks[1].tokens[2].opcode == 0x97 && parsed_msg.blocks[1].tokens[2].args.size() == 3);
    const auto cooked_msg=l3::cook_l3message(parsed_msg); const auto msg_header=l3::inspect_l3message(cooked_msg,true);
    assert(msg_header.block_count==2 && msg_header.token_count==parsed_msg.blocks[0].tokens.size()+parsed_msg.blocks[1].tokens.size());
    assert(msg_header.argument_bytes==13 && msg_header.file_bytes==cooked_msg.size());
    const auto loaded_msg=l3::load_l3message(cooked_msg,true);
    assert(loaded_msg.blocks.size()==2 && loaded_msg.blocks[0].tokens.size()==parsed_msg.blocks[0].tokens.size());
    assert(loaded_msg.blocks[1].tokens[2].opcode==0x97 && loaded_msg.blocks[1].tokens[2].args.size()==3);
    l3::MmlMessageRuntimeCursor msg_cursor(loaded_msg);
    msg_cursor.start_block(0);
    l3::MmlRuntimeEvent me{};
    assert(msg_cursor.next(me) && me.kind==l3::MmlRuntimeEventKind::Glyph && me.opcode==0x15);
    assert(msg_cursor.next(me) && me.kind==l3::MmlRuntimeEventKind::Newline);
    assert(msg_cursor.next(me) && me.kind==l3::MmlRuntimeEventKind::Audio && me.args.size()==4 && me.args[0]==1);
    assert(msg_cursor.next(me) && me.kind==l3::MmlRuntimeEventKind::Wait && me.args.size()==2);
    assert(msg_cursor.next(me) && me.kind==l3::MmlRuntimeEventKind::Next);
    assert(msg_cursor.next(me) && me.kind==l3::MmlRuntimeEventKind::UnknownControl && me.opcode==0xED);
    assert(msg_cursor.next(me) && me.kind==l3::MmlRuntimeEventKind::BlockEnd);
    assert(!msg_cursor.next(me));
    msg_cursor.start_block(1);
    assert(msg_cursor.next(me) && me.kind==l3::MmlRuntimeEventKind::Selection);
    assert(msg_cursor.next(me) && me.kind==l3::MmlRuntimeEventKind::Selection);
    assert(msg_cursor.next(me) && me.kind==l3::MmlRuntimeEventKind::Multi);
    assert(msg_cursor.next(me) && me.kind==l3::MmlRuntimeEventKind::End);

    // Backend-neutral pose math: PS1 angle domain is 4096 units/turn and RotMatrix order is X*Y*Z.
    assert(std::abs(l3::pbd_angle12_to_radians(1024) - 1.57079632679f) < 0.0001f);
    const auto rz90 = l3::player_pose_rotation_xyz(0, 0, 1024);
    const auto rp = l3::player_pose_transform_point(rz90, {1.f,0.f,0.f});
    assert(std::abs(rp.x) < 0.0001f && std::abs(rp.y - 1.f) < 0.0001f);
    l3::PbdMainSkeleton pose_sk{};
    pose_sk.base_translation[1] = {10,0,0};
    l3::PbdMainPoseBank pose_bank{};
    pose_bank.root_translation = {5,6,7};
    const auto pose_mats = l3::build_player_pose_matrices(pose_sk, pose_bank);
    const auto root_origin = l3::player_pose_transform_point(pose_mats.world[0], {0,0,0});
    const auto child_origin = l3::player_pose_transform_point(pose_mats.world[1], {0,0,0});
    assert(root_origin.x == 5.f && root_origin.y == 6.f && root_origin.z == 7.f);
    assert(child_origin.x == 15.f && child_origin.y == 6.f && child_origin.z == 7.f);

    // Stage object runtime record parser preserves all 20 bytes and stops on the signed -1/0xFF sentinel.
    std::vector<uint8_t> obj(41, 0);
    obj[0]=0x12; obj[1]=0x40; put16(obj,2,0x3456); put16(obj,0x0C,static_cast<uint16_t>(int16_t(-100)));
    put16(obj,0x0E,200); put16(obj,0x10,300); put16(obj,0x12,0xA55A);
    obj[20]=0x34; obj[21]=0x20; put16(obj,20+0x0C,400); put16(obj,20+0x0E,500); put16(obj,20+0x10,600);
    obj[40]=0xFF;
    const auto object_list=l3::parse_stage_object_list(obj);
    assert(object_list.records.size()==2 && object_list.bytes_consumed==41);
    assert(object_list.records[0].id==0x12 && object_list.records[0].object_class==0x40);
    assert(object_list.records[0].words_02_to_0a[0]==0x3456 && object_list.records[0].x==-100 && object_list.records[0].tail_12==0xA55A);

    // Synthetic USA ST00 overlay tables: five area pointers, five 20-byte lists, setup seeds/blobs.
    std::vector<uint8_t> flowraw(0x1AE20,0); const uint32_t flowbase=0x80100000u;
    const size_t objtbl=0x1AB3C, auxtbl=0x1AC40, blobtbl=0x1AC54, restbl=0x1A6F0, seedtbl=0x1AD1C, seltbl=0x1ADE4;
    for(size_t ai=0;ai<5;++ai){
        const uint32_t loff=static_cast<uint32_t>(0x1000+ai*0x40), aoff=static_cast<uint32_t>(0x2000+ai*0x20), roff=static_cast<uint32_t>(0x3000+ai*0x20);
        put32(flowraw,objtbl+ai*4,flowbase+loff); put32(flowraw,auxtbl+ai*4,flowbase+aoff); put32(flowraw,restbl+ai*4,flowbase+roff);
        flowraw[loff+0]=static_cast<uint8_t>(ai+1); flowraw[loff+1]=0xE0; put16(flowraw,loff+0x0C,static_cast<uint16_t>(100+ai)); flowraw[loff+20]=0xFF;
        // Aux lists are 24-byte slots and terminate on signed halfword -1. Give area 1 one active record.
        if(ai==1){ put16(flowraw,aoff+0,0x8000); put16(flowraw,aoff+6,0x0200); put16(flowraw,aoff+24,0xFFFF); }
        else { put16(flowraw,aoff+0,0xFFFF); }
        put32(flowraw,seltbl+ai*4,static_cast<uint32_t>(ai));
        for(size_t j=0;j<8;++j) flowraw[seedtbl+ai*8+j]=static_cast<uint8_t>(0x20+ai*8+j);
        for(size_t j=0;j<40;++j) flowraw[blobtbl+ai*40+j]=static_cast<uint8_t>(ai*40+j);
    }
    const auto flow=l3::parse_st00_flow_us(flowraw,flowbase);
    assert(flow.areas.size()==5 && flow.areas[0].objects.records.size()==1 && flow.areas[4].objects.records[0].id==5);
    assert(flow.areas[2].source_object_offset==0x1080 && flow.areas[3].setup_selector==3 && flow.areas[1].setup_seed[0]==0x28);
    assert(flow.areas[0].aux.records.empty() && flow.areas[0].aux.bytes_consumed==24);
    assert(flow.areas[1].aux.records.size()==1 && flow.areas[1].aux.records[0].halfwords[0]==static_cast<int16_t>(0x8000) && flow.areas[1].aux.bytes_consumed==48);
    const auto cookedflow=l3::cook_l3st00_flow(flow); const auto flowh=l3::inspect_l3st00_flow(cookedflow,true);
    assert(flowh.area_count==5 && flowh.object_count==5 && flowh.aux_count==1 && flowh.file_bytes==cookedflow.size());
    const auto loadedflow=l3::load_l3st00_flow(cookedflow,true);
    assert(loadedflow.areas[0].objects.records.size()==1 && loadedflow.areas[4].objects.records[0].id==5);
    assert(loadedflow.areas[2].source_object_offset==flow.areas[2].source_object_offset);
    assert(loadedflow.areas[1].aux.records.size()==1 && loadedflow.areas[1].aux.records[0].raw==flow.areas[1].aux.records[0].raw);
    assert(loadedflow.areas[3].setup_blob==flow.areas[3].setup_blob && loadedflow.areas[3].setup_seed==flow.areas[3].setup_seed);

    const auto negative_tile = legends_tile_transform_xz(100, 200, -3, -7, 15, 1);
    assert(negative_tile.x == 100 - 7 * 32768 && negative_tile.z == 200 + 3 * 32768);
    const auto a = legends_tile_transform_xz(100, 200, 3, 7, 1, 0); assert(a.x == 106 && a.z == 214);
    const auto b = legends_tile_transform_xz(100, 200, 3, 7, 1, 1); assert(b.x == 114 && b.z == 194);
    const auto c = legends_tile_transform_xz(100, 200, 3, 7, 1, 2); assert(c.x == 94 && c.z == 186);
    const auto d = legends_tile_transform_xz(100, 200, 3, 7, 1, 3); assert(d.x == 86 && d.z == 206);

    MMLClassicMotion motion{};
    mml_jump_enter(&motion, 0); assert(motion.vertical_velocity == -0x1C0);
    mml_airborne_gravity_step(&motion); assert(motion.vertical_velocity == -0x190);
    mml_turn_ramp(&motion, 1, 4, 0x20); assert(motion.turn_accum == 4 && motion.yaw == 4);
    // 0x80042154 reversal semantics: the first opposite-direction frame zeroes the accumulator.
    mml_turn_ramp(&motion, -1, 4, 0x20); assert(motion.turn_accum == 0 && motion.yaw == 4);
    mml_turn_ramp(&motion, -1, 4, 0x20); assert(motion.turn_accum == -4 && motion.yaw == 0);
    motion.turn_accum = -0x20; mml_turn_ramp(&motion, 1, 4, 0x20); assert(motion.turn_accum == 0);
    assert((mml_circlepad_to_dpad(0.8f, 0.8f, 0.2f) & 0x30) == 0x30);

    // 0x8003E068 preserves axis44 on opposing/no input but zeros axis48 on opposing input.
    MMLControlProfile ip{};
    uint16_t raw_masks[16]{}; raw_masks[0]=0x0010; raw_masks[1]=0x0040; raw_masks[4]=0x0020; raw_masks[5]=0x0080;
    mml_build_rotated_masks(&ip, raw_masks);
    MMLMotionAccums ma{};
    mml_update_direction_accums(&ma, &ip, 0, 0x0010); assert(ma.axis44 == 0x28);
    mml_update_direction_accums(&ma, &ip, 0, 0x0010|0x0040); assert(ma.axis44 == 0x28);
    mml_update_direction_accums(&ma, &ip, 0, 0); assert(ma.axis44 == 0x28);
    ma.axis48 = 0x60; mml_update_direction_accums(&ma, &ip, 0, 0x0020|0x0080); assert(ma.axis48 == 0);
    for(int i=0;i<64;++i) { mml_update_direction_accums(&ma,&ip,0,0x0010); }
    assert(ma.axis44==0x280);

    std::vector<uint8_t> fake(84, 0); fake[0] = 'L'; fake[1] = '3'; fake[2] = 'M'; fake[3] = 'P'; fake[4] = 1; fake[6] = 84;
    bool rejected = false; try { (void)l3::inspect_l3map(fake, false); } catch (...) { rejected = true; } assert(rejected);

    std::vector<uint8_t> badp(64, 0); bool preject = false; try { (void)l3::parse_player_pbd_mesh(badp); } catch (...) { preject = true; } assert(preject);

    // Synthetic one-part player PBD. This tests the parser without bundling game data.
    std::vector<uint8_t> pbd(108, 0); const uint32_t base = 0x80010000u; pbd[0] = 1; pbd[2] = 3; pbd[3] = 7;
    put32(pbd, 4, base + 16); put32(pbd, 8, 0); put32(pbd, 12, base + 84);
    pbd[20] = 128; pbd[21] = 128; pbd[22] = 128; pbd[23] = 0x24;
    pbd[28] = 1; pbd[29] = 2; put16(pbd, 30, 0x1234); pbd[36] = 3; pbd[37] = 4; put16(pbd, 38, 0x0056); pbd[44] = 5; pbd[45] = 6;
    for (size_t i = 0; i < 32; ++i) pbd[48 + i] = pbd[16 + i];
    pbd[80] = 0; pbd[81] = 1; pbd[82] = 2; pbd[83] = 0;
    put16(pbd, 84, 10); put16(pbd, 86, 20); put16(pbd, 88, 30); put16(pbd, 92, 40); put16(pbd, 94, 50); put16(pbd, 96, 60); put16(pbd, 100, 70); put16(pbd, 102, 80); put16(pbd, 104, 90);
    const auto pm = l3::parse_player_pbd_mesh(pbd);
    assert(pm.inferred_load_base == base && pm.parts.size() == 1 && pm.parts[0].part_id == 7 && pm.parts[0].vertices.size() == 3 && pm.parts[0].triangles.size() == 1);
    assert(pm.parts[0].triangles[0].clut == 0x1234 && pm.parts[0].triangles[0].tpage == 0x0056);
    l3::L3ModelStats ms; const auto cookedm = l3::cook_l3model(pm, &ms); const auto mh = l3::inspect_l3model(cookedm, true);
    assert(ms.part_count == 1 && ms.vertex_count == 3 && ms.triangle_count == 1 && ms.quad_count == 0 && mh.file_bytes == cookedm.size());

    // Synthetic MML type-1 4bpp texture: one source 32-word x32-row
    // sector tile and one 16-color CLUT. Its first four pixels are distinct.
    std::vector<uint8_t> tex(0x1000, 0);
    put32(tex, 0x00, 1); put32(tex, 0x04, 0x20 + 32 + 0x800); put32(tex, 0x08, 0x12345678);
    put32(tex, 0x0C, 0); put32(tex, 0x10, 240); put32(tex, 0x14, 16); put32(tex, 0x18, 1);
    put32(tex, 0x1C, 320); put32(tex, 0x20, 0); put32(tex, 0x24, 32); put32(tex, 0x28, 32);
    for (uint16_t i = 0; i < 16; ++i) put16(tex, 0x100 + size_t(i) * 2, static_cast<uint16_t>(0x0100 + i));
    tex[0x800] = 0x21; tex[0x801] = 0x43;
    const auto tx = l3::parse_mml_type1_texture(tex, 0);
    assert(tx.width_pixels == 128 && tx.height == 32 && tx.bpp == 4 && tx.palette_entries == 16 && tx.palette_rows == 1);
    assert(tx.palette_row_banked);
    const auto indices = l3::unpack_texture_indices(tx);
    assert(indices.size() == 128u * 32u && indices[0] == 1 && indices[1] == 2 && indices[2] == 3 && indices[3] == 4);
    assert(std::all_of(indices.begin() + 4, indices.end(), [](uint8_t v) { return v == 0; }));
    assert(l3::texture_resolves_material(tx, 0x0005, 0x3C00));
    assert(l3::texture_palette_row(tx, 0x3C00) == 0);
    const auto cookedtx = l3::cook_l3texture(tx); const auto txh = l3::inspect_l3texture(cookedtx, true);
    assert(txh.width_pixels == 128 && txh.height == 32 && txh.index_bytes == 0x800 && txh.palette_bytes == 32 && txh.file_bytes == cookedtx.size());


    // Synthetic type-9 palette-only patch at a different CLUT location.
    std::vector<uint8_t> pal9(0x800, 0);
    put32(pal9, 0x00, 9); put32(pal9, 0x04, 0x8120); put32(pal9, 0x08, 0x12);
    put32(pal9, 0x0C, 128); put32(pal9, 0x10, 500); put32(pal9, 0x14, 16); put32(pal9, 0x18, 2);
    for (uint16_t i = 0; i < 32; ++i) put16(pal9, 0x100 + size_t(i) * 2, static_cast<uint16_t>(0x2000 + i));
    const auto pp = l3::parse_mml_type9_palette(pal9, 0);
    // Two stored rows are flattened into adjacent 16-color slots of bank Y500.
    const uint16_t patch_clut = static_cast<uint16_t>((500u << 6) | 9u);
    assert(pp.bpp == 4 && pp.palette_rows == 2 && l3::palette_patch_row(pp, patch_clut) == 1);
    assert(pp.palette_row_banked && l3::palette_patch_row(pp, uint16_t((501u << 6) | 8u)) == -1);
    assert(l3::palette_patch_resolves_clut(pp, patch_clut, 4));
    const auto cookedpp = l3::cook_l3palette(pp); const auto pph = l3::inspect_l3palette(cookedpp, true);
    assert(pph.palette_bytes == 64 && pph.file_bytes == cookedpp.size());

    // Synthetic type-4 CLT palette patch. Type 4 is a real tutorial palette source distinct from type 9.
    std::vector<uint8_t> pal4(0x800, 0);
    put32(pal4, 0x00, 4); put32(pal4, 0x04, 0x20); put32(pal4, 0x08, 2);
    put32(pal4, 0x0C, 16); put32(pal4, 0x10, 496); put32(pal4, 0x14, 16); put32(pal4, 0x18, 1);
    for (uint16_t i = 0; i < 16; ++i) put16(pal4, 0x100 + size_t(i) * 2, static_cast<uint16_t>(0x3000 + i));
    const auto cp = l3::parse_mml_type4_clut(pal4, 0);
    const uint16_t clut4 = static_cast<uint16_t>((496u << 6) | 1u);
    assert(cp.bpp == 4 && cp.clut_x == 16 && cp.clut_y == 496 && l3::palette_patch_row(cp, clut4) == 0);
    assert(cp.palette_row_banked);
    auto direct_header = pal9;
    put32(direct_header, 0x0c, 256);
    assert(!l3::parse_mml_type9_palette(direct_header, 0).palette_row_banked);
    const auto cookedcp = l3::cook_l3palette(cp); const auto cph = l3::inspect_l3palette(cookedcp, true);
    assert((cph.flags & 2u) != 0u && cph.palette_bytes == 32);


    // Load-order-aware material cache: a later upload to the same TPAGE/CLUT wins.
    l3::IndexedResource oldres; oldres.kind = l3::IndexedResourceKind::TextureType1; oldres.texture = tx;
    l3::IndexedResource newres = oldres;
    newres.texture.palette_ps1[1] = 0x001Fu; // red in PS1 BGR555 order
    newres.texture.packed_indices[0] = 0x11; newres.texture.packed_indices[1] = 0x11;
    std::vector<l3::IndexedResource> upload_order{oldres, newres};
    const uint32_t synthetic_material = (uint32_t(0x3C00u) << 16) | 0x0005u;
    const auto last = l3::resolve_material_word_last_writer(synthetic_material, upload_order);
    assert(last.complete() && last.image_resource == 1 && last.palette_resource == 1 && last.palette_row == 0);
    const auto mc = l3::build_material_cache({synthetic_material}, upload_order);
    assert(mc.entries.size() == 1 && mc.entries[0].resolved && mc.entries[0].indices.size() == 256u * 256u);
    assert(mc.entries[0].indices[0] == 1 && mc.entries[0].indices[3] == 1 && mc.entries[0].indices[4] == 0);
    const auto rgba = l3::material_page_rgba8(mc.entries[0]);
    assert(rgba.size() == 256u * 256u * 4u && rgba[0] == 255 && rgba[1] == 0 && rgba[2] == 0 && rgba[3] == 255);
    const auto cookedmc = l3::cook_l3material_cache(mc);
    const auto mch = l3::inspect_l3material_cache(cookedmc, true);
    const auto loadedmc = l3::load_l3material_cache(cookedmc, true);
    assert(mch.material_count == 1 && mch.resolved_count == 1 && mch.unresolved_count == 0);
    assert(loadedmc.entries.size() == 1 && loadedmc.entries[0].palette_ps1[1] == 0x001Fu);

    // Forty contiguous 12-bit animation channels exactly fill one 60-byte PBD frame.
    std::array<uint16_t, l3::kPbdPackedAngleChannels> channels{};
    for (size_t i = 0; i < channels.size(); ++i) channels[i] = static_cast<uint16_t>((i * 97u + 3u) & 0x0FFFu);
    const auto packed = l3::pack_pbd_12bit_frame(channels);
    const auto unpacked = l3::unpack_pbd_12bit_frame(packed.data());
    assert(unpacked == channels);

    // Main M-bank semantic mapping proven from the executable's scratch pose vectors.
    auto pose_channels = channels;
    pose_channels[0] = 0x0FFF; // -1 signed 12-bit
    pose_channels[1] = 0x0800; // -2048 signed 12-bit
    pose_channels[2] = 0x07FF; // +2047
    pose_channels[38] = 0; pose_channels[39] = 0;
    const auto pose = l3::decode_pbd_main_pose_bank(pose_channels);
    assert(pose.root_translation[0] == -1 && pose.root_translation[1] == -2048 && pose.root_translation[2] == 2047);
    assert(pose.part_rotation[0][0] == pose_channels[3] && pose.part_rotation[0][2] == pose_channels[5]);
    assert(pose.part_rotation[3][0] == pose_channels[12] && pose.part_rotation[3][1] == 0 && pose.part_rotation[3][2] == 0);
    assert(pose.part_rotation[4][0] == 0 && pose.part_rotation[4][1] == pose_channels[13] && pose.part_rotation[4][2] == pose_channels[14]);
    assert(pose.part_rotation[14][0] == pose_channels[35] && pose.part_rotation[14][2] == pose_channels[37]);
    assert(l3::pbd_main_pose_trailing_channels_zero(pose_channels));


    // Synthetic main action-0 anchor table: fifteen 8-byte base-translation entries.
    std::vector<uint8_t> skm(4 + 15 * 8, 0); const uint32_t skbase = 0x80030000u;
    put32(skm, 0, skbase + 4);
    for (size_t part = 0; part < 15; ++part) {
        const size_t q = 4 + part * 8;
        put16(skm, q + 0, static_cast<uint16_t>(int16_t(part * 10)));
        put16(skm, q + 2, static_cast<uint16_t>(int16_t(-int(part * 3))));
        put16(skm, q + 4, static_cast<uint16_t>(int16_t(part * 2)));
        put16(skm, q + 6, 0);
    }
    const auto sk = l3::parse_pbd_main_skeleton(skm, skbase);
    assert(sk.source_special_offset == 4 && sk.parent[0] == -1 && sk.parent[3] == 2 && sk.parent[11] == 10 && sk.parent[14] == 13);
    assert(sk.base_translation[7][0] == 70 && sk.base_translation[7][1] == -21 && sk.base_translation[7][2] == 14);
    const auto cookedsk = l3::cook_l3skeleton(sk); const auto skh = l3::inspect_l3skeleton(cookedsk, true);
    assert(skh.part_count == 15 && skh.file_bytes == cookedsk.size());

    // Baseline renderer composition: the four head meshes all attach to transformed matrix 1.
    std::array<l3::PbdModel, l3::kPlayerComponentSlots> comps{};
    const std::array<std::vector<uint8_t>, l3::kPlayerComponentSlots> ids{{
        {0,8,9,10,12,13}, {1,15,16,17}, {2,3,4}, {5,6,7}, {11,14}
    }};
    for (size_t slot = 0; slot < comps.size(); ++slot) for (uint8_t id : ids[slot]) { l3::PbdPart p; p.part_id = id; comps[slot].parts.push_back(p); }
    const auto pc = l3::build_baseline_player_composition(comps);
    assert(pc.attachments.size() == 18);
    for (size_t i = 6; i < 10; ++i) assert(pc.attachments[i].component_slot == 1 && pc.attachments[i].transform_id == 1);
    assert(pc.attachments[0].transform_id == 0 && pc.attachments[1].transform_id == 8 && pc.attachments[17].transform_id == 14);
    const auto cookedpc = l3::cook_l3player_composition(pc); const auto pch = l3::inspect_l3player_composition(cookedpc, true);
    assert(pch.attachment_count == 18 && pch.component_slots == 5 && pch.file_bytes == cookedpc.size());

    // Backend-neutral player visual bridge preserves transform attachment and primitive topology.
    l3::PbdVertex vv{}; vv.x = 7; vv.y = 8; vv.z = 9;
    comps[0].parts[0].vertices = {vv, vv, vv};
    l3::PbdTri vt{}; vt.index = {0,1,2}; vt.uv = {0,0,4,0,0,4}; vt.clut = 0x1234; vt.tpage = 0x0056; vt.code = 0x24;
    comps[0].parts[0].triangles.push_back(vt);
    l3::PlayerVisualStats pvs{}; const auto pv = l3::build_player_visuals(comps, pc, &pvs);
    assert(pv.size() == 1 && pv[0].transform_id == 0 && pv[0].vertex_count == 3 && pvs.triangles == 1 && pvs.vertex_references == 3);
    // Animated-pose bridge now produces real world-space player vertices, not only transform IDs.
    l3::PlayerPoseMatrices posed_mats{}; posed_mats.world[0].m[3]=100.f; posed_mats.world[0].m[7]=200.f; posed_mats.world[0].m[11]=300.f;
    const auto posed_vis=l3::pose_player_visuals(pv,posed_mats);
    assert(posed_vis.size()==1 && posed_vis[0].position[0].x==107.f && posed_vis[0].position[0].y==208.f && posed_vis[0].position[0].z==309.f);

    // Synthetic S/M pair with 2 action slots, one S region and one regular one-frame M pointer table.
    const uint32_t sbase = 0x80010000u, mbase = 0x80020000u;
    std::vector<uint8_t> spbd(24, 0), mpbd(72, 0);
    put32(spbd, 0, sbase + 8); put32(spbd, 4, 0);
    put16(spbd, 8, 0x1111); put16(spbd, 10, 0x2222); put32(spbd, 12, 0x33333333);
    put16(spbd, 16, 0x4444); put16(spbd, 18, 0x5555); put32(spbd, 20, 0x66666666);
    put32(mpbd, 0, mbase + 8); put32(mpbd, 4, 0); put32(mpbd, 8, mbase + 12);
    for (size_t i = 0; i < packed.size(); ++i) mpbd[12 + i] = packed[i];
    const auto anim = l3::parse_player_pbd_animation(spbd, sbase, mpbd, mbase);
    assert(anim.action_slots == 2 && anim.actions.size() == 1 && anim.actions[0].s_records.size() == 2);
    assert(anim.actions[0].m_is_frame_table && anim.actions[0].angle_frames.size() == 1 && anim.actions[0].angle_frames[0] == channels);
    l3::L3AnimationStats ast; const auto cookeda = l3::cook_l3animation(anim, &ast); const auto ah = l3::inspect_l3animation(cookeda, true);
    assert(ast.action_slots == 2 && ast.populated_actions == 1 && ast.frame_table_actions == 1 && ast.frames == 1 && ah.file_bytes == cookeda.size());

    // Synthetic pointer-free render graph using the executable-proven compressed X/Z gather order.
    l3::St00RenderSource rs;
    l3::SourceRenderPrimitive prim; prim.prefix = 0xAABBCCDDu; prim.uv = {0,0, 8,0, 8,8, 0,8}; prim.local_x = {-1,1,1,-1}; prim.local_z = {-2,-2,2,2}; prim.ignored_raw = {11,22,33,44};
    l3::SourceRenderStream stream; stream.code = 0x40; stream.base_shift = 4; stream.material_slot = 0; stream.primitives.push_back(prim); rs.streams.push_back(stream);
    l3::SourceRenderRecord rec; rec.flags_high2 = 1; rec.materials.push_back(0x3C000005u); rec.stream_indices.push_back(0); rs.records.push_back(rec);
    l3::SourceRenderDescriptor desc; desc.record_indices[0] = 0; rs.descriptors.push_back(desc);
    l3::L3RenderStats rst; const auto cookedr = l3::cook_l3render(rs, &rst); const auto rh = l3::inspect_l3render(cookedr, true);
    assert(rst.descriptor_count == 1 && rst.record_count == 1 && rst.stream_count == 1 && rst.stream_references == 1 && rst.material_words == 1 && rst.primitive_count == 1);
    assert(rh.file_bytes == cookedr.size());
    const auto lod0 = l3::select_st00_primary_lod(0x0000); assert(lod0.lod == l3::St00Lod::Near && lod0.near_replay_path && lod0.descriptor_slot == 0);
    const auto lod1 = l3::select_st00_primary_lod(0x0401); assert(lod1.lod == l3::St00Lod::Near && !lod1.near_replay_path);
    const auto lod2 = l3::select_st00_primary_lod(0x0AF1); assert(lod2.lod == l3::St00Lod::Mid && lod2.descriptor_slot == 1);
    const auto lod3 = l3::select_st00_primary_lod(0x1C21); assert(lod3.lod == l3::St00Lod::Far && lod3.descriptor_slot == 2);
    assert(l3::st00_descriptor_slot(l3::St00RenderBank::Secondary, l3::St00Lod::Far) == 5);

    const l3::St00CellOrigin ro{100, 50, 200};
    const auto expanded0 = l3::expand_st00_descriptor_side(rs, 0, 0, ro, 0, 1);
    assert(expanded0.size() == 1 && expanded0[0].world_y[0] == 50);
    assert(expanded0[0].world_x[0] == 68 && expanded0[0].world_z[0] == 136); // stream shift 4 + global shift 1
    const auto expanded1 = l3::expand_st00_descriptor_side(rs, 0, 0, ro, 1, 1);
    assert(expanded1[0].world_x[0] == 36 && expanded1[0].world_z[0] == 232); // (z,-x) after effective shift 5

    // Cell-origin formulas: positive cells take IDX Y; special cells encode Y in bits 8..12.
    l3::St00Source origins; origins.idx.resize(2); origins.idx[1].y = -123;
    const auto op = l3::st00_cell_origin(10, 20, 1, origins);
    assert(op.x == (10 << 9) - 0x7F00 && op.y == -123 && op.z == (20 << 9) - 0x7F00);
    const auto on = l3::st00_cell_origin(10, 20, static_cast<int16_t>(0xFF00u), origins);
    assert(on.y == -static_cast<int32_t>((0xFF00u & 0x1F00u) >> 1));

    // Area visual bridge resolves descriptor pointers through the HED descriptor table, not pointer arithmetic.
    l3::St00Source area_stage; area_stage.idx.resize(2); area_stage.idx[1].y = 5; area_stage.idx[1].descriptor_ptr = 0x80195550u;
    l3::SourceDescriptor area_desc; area_desc.source_ptr = 0x80195550u; area_stage.descriptors.push_back(area_desc);
    l3::SourceArea area; area.start_x = 1; area.start_z = 2; area.width = 1; area.height = 1; area.cells = {1}; area_stage.areas.push_back(area);
    const std::vector<uint32_t> area_depths{0x0AF1u}; l3::St00AreaVisualStats avs{};
    // Synthetic render descriptor uses mid slot 1.
    auto rs_area = rs; rs_area.descriptors[0].record_indices[1] = 0;
    const auto area_vis = l3::build_st00_area_primary_visuals(area_stage, rs_area, 0, area_depths, 0, &avs);
    assert(area_vis.size() == 1 && avs.positive_cells == 1 && avs.mid_cells == 1);


    // EBD directory: u32 count followed by exact 16-byte key/resource records.
    // Synthetic payload mirrors the original common lookup: class/subtype/optional variant.
    std::vector<uint8_t> ebd(0x80, 0);
    const uint32_t ebase = 0x8016C000u;
    put32(ebd, 0, 3);
    // E0 subtype 1 variants 0 and 1.
    ebd[4] = 0xE0; ebd[5] = 0x01; ebd[6] = 0; ebd[7] = 0;
    put32(ebd, 8, ebase + 0x40);
    ebd[20] = 0xE0; ebd[21] = 0x01; ebd[22] = 1; ebd[23] = 0;
    put32(ebd, 24, ebase + 0x44);
    // Class 20 subtype 35, one unambiguous entry.
    ebd[36] = 0x20; ebd[37] = 0x35; ebd[38] = 0; ebd[39] = 0;
    put32(ebd, 40, ebase + 0x48);
    const auto edir = l3::parse_ebd_directory(ebd, ebase);
    assert(edir.entries.size() == 3 && edir.entries[0].resource_offset[0] == 0x40);
    const auto cooked_ebd = l3::cook_l3ebd_directory(edir);
    const auto ebdh = l3::inspect_l3ebd_directory(cooked_ebd, true);
    assert(ebdh.entry_count == 3 && ebdh.source_load_base == ebase);
    const auto loaded_ebd = l3::load_l3ebd_directory(cooked_ebd, true);
    assert(loaded_ebd.load_base == ebase && loaded_ebd.entries.size() == edir.entries.size());
    assert(loaded_ebd.entries[1].object_class == 0xE0 && loaded_ebd.entries[1].variant == 1);
    assert(loaded_ebd.entries[2].resource_offset[0] == 0x48);
    const auto boot_area = l3::load_tutorial_runtime_area(cookedflow, cooked_ebd, 0, true);
    assert(boot_area.area_index == 0 && boot_area.stage.instances.size() == 1);
    assert(boot_area.flow.objects.records[0].id == 1);
    assert(boot_area.ebd.entries.size() == 3);
    assert(l3::normalize_ebd_object_class(0xE3) == 0xE0);
    assert(l3::normalize_ebd_object_class(0xA0) == 0x80);
    assert(l3::find_ebd_entries(edir, 0xE0, 0x01).size() == 2);
    assert(l3::find_ebd_entries(edir, 0xE0, 0x01, 1).size() == 1);

    l3::StageObjectRecord srec{};
    srec.id = 1; srec.object_class = 0x20; srec.raw[0] = 1; srec.raw[1] = 0x20;
    srec.raw[2] = 0x35; srec.raw[3] = 0; srec.x = -12; srec.y = 34; srec.z = -56;
    assert(l3::stage_object_ebd_candidates(srec, edir).size() == 1);
    assert(l3::stage_object_ebd_candidates_record_variant(srec, edir).size() == 1);
    l3::StageObjectRecord e0rec{};
    e0rec.object_class = 0xE0; e0rec.raw[1] = 0xE0; e0rec.raw[2] = 0x01; e0rec.raw[3] = 1;
    assert(l3::stage_object_ebd_candidates(e0rec, edir).size() == 2);
    assert(l3::stage_object_ebd_candidates_record_variant(e0rec, edir).size() == 1);
    const auto runtime_obj = l3::instantiate_stage_runtime_object(srec, 7);
    assert(runtime_obj.source_index == 7 && runtime_obj.family == l3::StageObjectFamily::Class20);
    assert(runtime_obj.subtype == 0x35 && runtime_obj.fixed_position_16_16[0] == -12 * 65536);
    // Original object-pool geometry and first-fit fresh-area allocation.
    const auto p20 = l3::stage_object_pool_spec(l3::StageObjectFamily::Class20);
    const auto p40 = l3::stage_object_pool_spec(l3::StageObjectFamily::Class40);
    const auto p60 = l3::stage_object_pool_spec(l3::StageObjectFamily::Class60);
    const auto p80 = l3::stage_object_pool_spec(l3::StageObjectFamily::Class80);
    const auto pc0 = l3::stage_object_pool_spec(l3::StageObjectFamily::ClassC0);
    const auto pe0 = l3::stage_object_pool_spec(l3::StageObjectFamily::ClassE0);
    assert(p20 && p20->capacity == 16 && p20->original_stride_bytes == 0x3C0);
    assert(p40 && p40->capacity == 4  && p40->original_stride_bytes == 0x5A4);
    assert(p60 && p60->capacity == 32 && p60->original_stride_bytes == 0x10C);
    assert(p80 && p80->capacity == 64 && p80->banks == 2 && p80->original_stride_bytes == 0x48);
    assert(pc0 && pc0->capacity == 16 && pc0->original_stride_bytes == 0x24);
    assert(pe0 && pe0->capacity == 32 && pe0->original_stride_bytes == 0x50);
    assert(!l3::stage_object_pool_spec(l3::StageObjectFamily::Class00));
    std::vector<l3::StageRuntimeObject> alloc_objs;
    auto a0 = runtime_obj; a0.source_index = 0; alloc_objs.push_back(a0);
    auto a1 = runtime_obj; a1.source_index = 1; alloc_objs.push_back(a1);
    auto ae = runtime_obj; ae.source_index = 2; ae.source_class = 0xE0;
    ae.family = l3::StageObjectFamily::ClassE0; alloc_objs.push_back(ae);
    const auto allocs = l3::assign_fresh_stage_pool_slots(alloc_objs);
    assert(allocs.size() == 3 && allocs[0].pool_slot && *allocs[0].pool_slot == 0);
    assert(allocs[1].pool_slot && *allocs[1].pool_slot == 1);
    assert(allocs[2].pool_slot && *allocs[2].pool_slot == 0);
    // Exact fresh-work seeding from the common stage parser.
    const auto seeded20 = l3::seed_fresh_stage_runtime_work(alloc_objs[0], allocs[0]);
    assert(seeded20 && seeded20->layout == l3::StageWorkSeedLayout::Common20_40_60);
    assert(seeded20->bytes.size() == 0x3C0 && seeded20->bytes[6] == 3);
    assert(l3::rd_u16(seeded20->bytes.data()+2) == 0x35);
    assert(static_cast<int32_t>(l3::rd_u32(seeded20->bytes.data()+0x1C)) == -12 * 65536);
    assert(static_cast<int32_t>(l3::rd_u32(seeded20->bytes.data()+0x20)) == 34 * 65536);
    assert(static_cast<int32_t>(l3::rd_u32(seeded20->bytes.data()+0x24)) == -56 * 65536);
    l3::StageRuntimeObject c0o = runtime_obj; c0o.source_index = 0; c0o.source_class = 0xC0;
    c0o.family = l3::StageObjectFamily::ClassC0; c0o.raw[1] = 0xC0;
    const l3::StageRuntimeAllocation c0a{0,l3::StageObjectFamily::ClassC0,size_t(0)};
    const auto seededc0 = l3::seed_fresh_stage_runtime_work(c0o,c0a);
    assert(seededc0 && seededc0->bytes.size()==0x24 && seededc0->layout==l3::StageWorkSeedLayout::CompactC0);
    l3::StageRuntimeObject e0o = c0o; e0o.source_class=0xE0; e0o.family=l3::StageObjectFamily::ClassE0; e0o.raw[1]=0xE0;
    const l3::StageRuntimeAllocation e0a{0,l3::StageObjectFamily::ClassE0,size_t(0)};
    const auto seedede0=l3::seed_fresh_stage_runtime_work(e0o,e0a);
    assert(seedede0 && seedede0->bytes.size()==0x50 && seedede0->layout==l3::StageWorkSeedLayout::Compact80_E0);
    assert(static_cast<int16_t>(l3::rd_u16(seedede0->bytes.data()+0x1C))==-12);
    l3::StageObjectList runtime_source;
    runtime_source.records = {srec, e0rec};
    const auto runtime_area = l3::build_fresh_stage_runtime_area(runtime_source, edir);
    assert(runtime_area.instances.size() == 2);
    assert(runtime_area.instances[0].ebd.state == l3::StageEbdBindingState::UniqueClassSubtype);
    assert(runtime_area.instances[1].ebd.state == l3::StageEbdBindingState::AmbiguousNarrowedBySourceByte3);
    assert(runtime_area.instances[1].ebd.class_subtype_candidates.size() == 2);
    assert(runtime_area.instances[1].ebd.source_byte3_candidates.size() == 1);

    // ST00 aux event selection: exact position for type 1/8, +/-0x200 facing window
    // for types 2/4/5/6, and the exact type-3 directional live-bound comparison.
    l3::St00AuxList aux_test;
    l3::St00AuxRecord ar{};
    ar.halfwords[4] = -256; ar.halfwords[5] = -384; ar.halfwords[6] = 15616;
    ar.halfwords[7] = 0x0800;
    aux_test.records.push_back(ar);
    l3::St00ContactSample cs{-256, -384, 15616, 0x0A00, 2, 0};
    assert(l3::st00_aux_position_equal(ar, cs));
    assert(l3::st00_aux_facing_within_window(ar, cs)); // +0x200 inclusive
    auto matches = l3::match_st00_aux_records(aux_test, cs);
    assert(matches.size() == 1 && matches[0].kind == l3::St00AuxMatchKind::ExactPositionFacingWindow);
    cs.facing = 0x0A01;
    assert(!l3::st00_aux_facing_within_window(ar, cs));
    assert(l3::match_st00_aux_records(aux_test, cs).empty());
    cs.type = 8; // type 8 ignores facing in the observed aux scan
    assert(l3::match_st00_aux_records(aux_test, cs).size() == 1);
    cs.type = 3;
    cs.facing = 0x0800;
    matches = l3::match_st00_aux_records(aux_test, cs);
    assert(matches.size() == 1 && matches[0].kind == l3::St00AuxMatchKind::Type3NeedsObjectBounds);
    // quadrant 2 at facing 0x800: original accepts when sample.z >= work+0x18.
    l3::St00Type3ObjectBounds t3b{}; t3b.work_14 = -300; t3b.work_18 = 15000;
    matches = l3::match_st00_aux_records(aux_test, cs, &t3b);
    assert(matches.size() == 1 && matches[0].kind == l3::St00AuxMatchKind::Type3DirectionalBounds);
    t3b.work_18 = 16000;
    assert(l3::match_st00_aux_records(aux_test, cs, &t3b).empty());
    // The distinct type-3 facing gate rejects when bit 0x400 is set after the +0x200 bias.
    t3b.work_18 = 15000; cs.facing = 0x0C00;
    assert(l3::match_st00_aux_records(aux_test, cs, &t3b).empty());


    // Player-facing QoL defaults preserve original behavior while enabling non-invasive 3DS helpers.
    L3QolSettings qol{};
    l3_qol_settings_defaults(&qol);
    assert(l3_qol_settings_valid(&qol));
    assert(qol.control_preset == L3_CONTROL_CLASSIC);
    assert(qol.camera_behavior == L3_CAMERA_CLASSIC);
    assert(qol.lock_on_mode == L3_LOCK_ON_HOLD);
    assert(qol.text_speed == L3_TEXT_SPEED_ORIGINAL);
    assert(qol.bottom_screen_mode == L3_BOTTOM_SCREEN_SMART);
    assert(qol.dialogue_history == 1 && qol.objective_panel == 1);
    qol.camera_sensitivity_percent = 500;
    qol.control_preset = 99;
    qol.invert_camera_x = 7;
    assert(!l3_qol_settings_valid(&qol));
    l3_qol_settings_sanitize(&qol);
    assert(l3_qol_settings_valid(&qol));
    assert(qol.camera_sensitivity_percent == 200);
    assert(qol.control_preset == L3_CONTROL_CLASSIC);
    assert(qol.invert_camera_x == 1);

    // Dialogue history retains runtime message references only, never source dialogue text.
    l3::DialogueHistory history;
    for (uint16_t i = 0; i < l3::DialogueHistory::kCapacity + 3; ++i) {
        history.push(l3::MessageRef{1, i});
    }
    assert(history.size() == l3::DialogueHistory::kCapacity);
    const auto history_ordered = history.ordered();
    assert(history_ordered.front().message == l3::MessageRef({1, 3}));
    assert(history_ordered.back().message == l3::MessageRef({1, 18}));
    assert(history_ordered.front().sequence < history_ordered.back().sequence);
    history.clear();
    assert(history.empty());

    // Retail-style bottom-screen command stream: useful status/objective/log data, no telemetry.
    l3_qol_settings_defaults(&qol);
    l3::BottomScreenModel bottom{};
    bottom.context = l3::BottomScreenContext::Gameplay;
    bottom.area_name_id = 1;
    bottom.hp = 64;
    bottom.hp_max = 100;
    bottom.buster_enabled = true;
    bottom.lock_on_active = true;
    bottom.objective_available = true;
    bottom.objective = l3::MessageRef{1, 2};
    bottom.dialogue_history_count = 4;
    auto bottom_layout = l3::build_bottom_screen_layout(bottom, qol);
    assert(bottom_layout.player_facing_only);
    assert(!bottom_layout.commands.empty());
    bool saw_objective = false, saw_health = false, saw_buster = false, saw_log = false, saw_lock = false;
    for (const auto& cmd : bottom_layout.commands) {
        assert(l3::bottom_ui_rect_in_bounds(cmd.rect));
        if (cmd.role == l3::BottomUiRole::ObjectiveBody) {
            saw_objective = true;
            assert(cmd.has_message && cmd.message == bottom.objective);
        }
        if (cmd.role == l3::BottomUiRole::HealthGauge) {
            saw_health = true;
            assert(cmd.value == 64 && cmd.maximum == 100);
        }
        if (cmd.role == l3::BottomUiRole::BusterReady) saw_buster = true;
        if (cmd.role == l3::BottomUiRole::DialogueHistory) saw_log = true;
        if (cmd.role == l3::BottomUiRole::LockOnActive) saw_lock = true;
    }
    assert(saw_objective && saw_health && saw_buster && saw_log && saw_lock);
    bool saw_area_name_value = false;
    for (const auto& cmd : bottom_layout.commands) {
        if (cmd.role == l3::BottomUiRole::AreaName && cmd.kind == l3::BottomUiCommandKind::Label) {
            saw_area_name_value = true;
            assert(cmd.value == bottom.area_name_id);
        }
    }
    assert(saw_area_name_value);

    // STATUS mode is deliberately less busy than SMART: no objective/history panels.
    qol.bottom_screen_mode = L3_BOTTOM_SCREEN_STATUS;
    bottom_layout = l3::build_bottom_screen_layout(bottom, qol);
    bool status_objective = false, status_log = false, status_health = false, status_buster = false;
    for (const auto& cmd : bottom_layout.commands) {
        assert(l3::bottom_ui_rect_in_bounds(cmd.rect));
        if (cmd.role == l3::BottomUiRole::ObjectiveBody || cmd.role == l3::BottomUiRole::ObjectiveHeader) status_objective = true;
        if (cmd.role == l3::BottomUiRole::DialogueHistory) status_log = true;
        if (cmd.role == l3::BottomUiRole::HealthGauge) status_health = true;
        if (cmd.role == l3::BottomUiRole::BusterReady) status_buster = true;
    }
    assert(!status_objective && !status_log && status_health && status_buster);

    // Cinematics intentionally quiet the lower display so cutscenes remain the focus.
    l3_qol_settings_defaults(&qol);
    bottom.context = l3::BottomScreenContext::Cinematic;
    bottom_layout = l3::build_bottom_screen_layout(bottom, qol);
    assert(bottom_layout.commands.size() == 2);
    assert(bottom_layout.commands[1].kind == l3::BottomUiCommandKind::CinematicShade);

    // Minimal mode still keeps health and lock-on status without a dashboard-like wall of data.
    bottom.context = l3::BottomScreenContext::Gameplay;
    qol.bottom_screen_mode = L3_BOTTOM_SCREEN_MINIMAL;
    bottom_layout = l3::build_bottom_screen_layout(bottom, qol);
    saw_health = false;
    for (const auto& cmd : bottom_layout.commands) {
        assert(l3::bottom_ui_rect_in_bounds(cmd.rect));
        if (cmd.role == l3::BottomUiRole::HealthGauge) saw_health = true;
    }
    assert(saw_health);

    // Bottom-screen minimap is derived from the cooked L3MP grid, not hand-authored rooms.
    l3::RuntimeMap minimap_source{};
    l3::RuntimeMapArea minimap_area{};
    minimap_area.start_x = 0;
    minimap_area.start_z = 0;
    minimap_area.width = 4;
    minimap_area.height = 3;
    minimap_area.cell_first = 0;
    minimap_area.cell_count = 12;
    minimap_source.areas.push_back(minimap_area);
    minimap_source.cells = {0, 1, 1, 0, -1, 1, 0, 0, 0, 1, 1, 1};
    const auto minimap = l3::build_bottom_map_model(minimap_source, 0,
        (1 << 9) - 0x7F00, (1 << 9) - 0x7F00);
    assert(minimap.width == 4 && minimap.height == 3);
    assert(minimap.cells.size() == 12);
    assert(minimap.cells[0] == l3::BottomMapCellKind::Empty);
    assert(minimap.cells[1] == l3::BottomMapCellKind::Geometry);
    assert(minimap.cells[4] == l3::BottomMapCellKind::Special);
    assert(minimap.player_in_area && minimap.player_x == 1 && minimap.player_z == 1);
    // A center + half a tile is the next containing cell boundary.
    const auto boundary_minimap = l3::build_bottom_map_model(minimap_source, 0,
        (1 << 9) - 0x7F00 + 0x100, (1 << 9) - 0x7F00 + 0x100);
    assert(boundary_minimap.player_in_area && boundary_minimap.player_x == 2 && boundary_minimap.player_z == 2);
    assert(l3::sample_bottom_map_cell(minimap, 1, 1, 4, 3) == l3::BottomMapCellKind::Geometry);
    const auto empty_minimap = l3::build_bottom_map_model(minimap_source, 9, 0, 0);
    assert(empty_minimap.width == 0 && empty_minimap.cells.empty() && !empty_minimap.player_in_area);


    // Exact USA control profile installer and derived masks.
    player_work = {};
    assert(l3_pl00_install_control_profile_800425fc(&player_work, 0));
    assert(pw_get16(player_work, 0x124) == 0x0010); // D-pad up in retail profile 0
    assert(pw_get16(player_work, 0x126) == 0x0040); // D-pad down
    assert(pw_get16(player_work, 0x128) == 0x0400 && pw_get16(player_work, 0x12A) == 0x0800);
    assert(pw_get16(player_work, 0x144) == 0x0050);
    assert(pw_get16(player_work, 0x148) == 0x00A0);
    assert(pw_get16(player_work, 0x14A) == 0x0020);
    assert(pw_get16(player_work, 0x150) == 0x0050);
    assert(pw_get16(player_work, 0x152) == 0x0010);
    assert(pw_get16(player_work, 0x154) == 0x0050);
    assert(pw_get16(player_work, 0x156) == 0x0040);
    assert(pw_get16(player_work, 0x158) == 0x00A0);
    assert(pw_get16(player_work, 0x15A) == 0x0080);
    assert(pw_get16(player_work, 0x15C) == 0x00A0);
    assert(pw_get16(player_work, 0x15E) == 0x0020);
    assert(pw_get16(player_work, 0x160) == 0x0050);
    assert(pw_get16(player_work, 0x162) == 0x0040);
    // Exact 0x8003D124 directional setup for the derived direction-0 mask pair.
    pw_put16(player_work, 0x11C, pw_get16(player_work, 0x12E));
    pw_put16(player_work, 0xB4, 0);
    assert(l3_pl00_directional_setup_8003d124(&player_work, 0));
    assert((int16_t)pw_get16(player_work, 0x48) == -0x140);
    assert((int16_t)pw_get16(player_work, 0xB4) == 0x40);
    pw_put16(player_work, 0x11C, pw_get16(player_work, 0x12C));
    pw_put16(player_work, 0xB4, 0);
    assert(l3_pl00_directional_setup_8003d124(&player_work, 0));
    assert((int16_t)pw_get16(player_work, 0x48) == 0x140);
    assert((int16_t)pw_get16(player_work, 0xB4) == -0x40);
    assert(!l3_pl00_install_control_profile_800425fc(&player_work, 2));

    // USA 0x8002FE74 + 0x8004B414: collision-group classification and the
    // exact post-solver snap/rollback mutation are now portable without naming
    // unproved floor/wall meanings.
    assert(l3_obj_group_bits_8002fe74(0x00) == 0x00);
    assert(l3_obj_group_bits_8002fe74(0x10) == 0x10);
    assert(l3_obj_group_bits_8002fe74(0x20) == 0x20);
    assert(l3_obj_group_bits_8002fe74(0xA3) == 0x80);
    assert(l3_obj_group_bits_8002fe74(0xE7) == 0xE0);

    player_work = {};
    pw_put16(player_work, 0x14, uint16_t(int16_t(10)));
    pw_put16(player_work, 0x16, uint16_t(int16_t(20)));
    pw_put16(player_work, 0x18, uint16_t(int16_t(30)));
    pw_put32(player_work, 0x1C, 10 * 65536);
    pw_put32(player_work, 0x20, 20 * 65536);
    pw_put32(player_work, 0x24, 30 * 65536);
    l3_pl00_snapshot_collision_pose(&player_work);
    pw_put16(player_work, 0x14, 40); pw_put16(player_work, 0x16, 50); pw_put16(player_work, 0x18, 60);
    pw_put32(player_work, 0x1C, 40 * 65536); pw_put32(player_work, 0x20, 50 * 65536); pw_put32(player_work, 0x24, 60 * 65536);
    pw_put16(player_work, 0x44, 0x50); pw_put16(player_work, 0x46, uint16_t(int16_t(-0x80))); pw_put16(player_work, 0x48, 0x20);
    player_work.bytes[0x74] = 1;
    L3RetailContactScratch retail_contact{101, 202, 303, 7};
    assert(l3_pl00_apply_contact_result_8004b414(&player_work, 0, &retail_contact) == 0);
    assert(pw_gets16(player_work, 0x14) == 40 && pw_gets16(player_work, 0x16) == 50 && pw_gets16(player_work, 0x18) == 60);
    assert(pw_gets32(player_work, 0x1C) == 40 * 65536 && pw_gets32(player_work, 0x20) == 50 * 65536 && pw_gets32(player_work, 0x24) == 60 * 65536);
    assert(pw_gets16(player_work, 0x44) == 0x50 && pw_gets16(player_work, 0x46) == -0x80 && pw_gets16(player_work, 0x48) == 0x20);
    assert(player_work.bytes[0x74] == 1);
    (void)l3_pl00_apply_contact_result_8004b414(&player_work, -1, &retail_contact);
    assert(pw_gets16(player_work, 0x14) == 10 && pw_gets16(player_work, 0x16) == 20 && pw_gets16(player_work, 0x18) == 30);
    assert(pw_gets16(player_work, 0x44) == 0 && pw_gets16(player_work, 0x46) == 0 && pw_gets16(player_work, 0x48) == 0);
    assert(player_work.bytes[0x74] == 0);

    // Nonzero 0x8002FE74 group takes the retail full-pose rollback path.
    player_work = {}; player_work.bytes[0x01] = 0x20;
    pw_put16(player_work, 0x14, 1); pw_put16(player_work, 0x16, 2); pw_put16(player_work, 0x18, 3);
    pw_put32(player_work, 0x1C, 1 * 65536); pw_put32(player_work, 0x20, 2 * 65536); pw_put32(player_work, 0x24, 3 * 65536); pw_put32(player_work, 0x28, 0x11223344);
    l3_pl00_snapshot_collision_pose(&player_work);
    pw_put16(player_work, 0x14, 9); pw_put16(player_work, 0x16, 9); pw_put16(player_work, 0x18, 9);
    pw_put32(player_work, 0x1C, 9 * 65536); pw_put32(player_work, 0x20, 9 * 65536); pw_put32(player_work, 0x24, 9 * 65536); pw_put32(player_work, 0x28, 0x55667788);
    pw_put16(player_work, 0x44, 0x55);
    (void)l3_pl00_apply_contact_result_8004b414(&player_work, -1, &retail_contact);
    assert(pw_gets16(player_work, 0x14) == 1 && pw_gets16(player_work, 0x16) == 2 && pw_gets16(player_work, 0x18) == 3);
    assert(pw_gets32(player_work, 0x28) == 0x11223344);
    assert(pw_gets16(player_work, 0x44) == 0x55); // this branch does not clear motion

    // Positive result axis masks snap the short and 16.16 mirrors to solver scratch.
    player_work = {};
    retail_contact = {-12, 34, 56, 7};
    const int32_t all_axis_masks = L3_CONTACT_RESULT_AXIS_X_MASK | L3_CONTACT_RESULT_AXIS_Z_MASK | L3_CONTACT_RESULT_AXIS_Y_MASK | 1;
    assert(l3_pl00_apply_contact_result_8004b414(&player_work, all_axis_masks, &retail_contact) == all_axis_masks);
    assert(pw_gets16(player_work, 0x14) == -12 && pw_gets16(player_work, 0x16) == 34 && pw_gets16(player_work, 0x18) == 56);
    assert(pw_gets32(player_work, 0x1C) == -12 * 65536 && pw_gets32(player_work, 0x20) == 34 * 65536 && pw_gets32(player_work, 0x24) == 56 * 65536);
    assert(player_work.bytes[0x74] == 7);
    player_work.bytes[0x74] = 2; retail_contact.contact_state = 9;
    (void)l3_pl00_apply_contact_result_8004b414(&player_work, L3_CONTACT_RESULT_AXIS_X_MASK, &retail_contact);
    assert(player_work.bytes[0x74] == 2); // retail preserves state 2

    // USA geometry-solver no-contact tail keeps exact dominant-axis tie order
    // and movement-sign mask bits (0x8004BC2C..0x8004BD90).
    assert(l3_collision_no_contact_result_8004bc2c(0,0,0,0,0,0) == 0);
    assert(l3_collision_no_contact_result_8004bc2c(10,2,3,0,0,0) == 0x1501);
    assert(l3_collision_no_contact_result_8004bc2c(-10,2,3,0,0,0) == 0x1602);
    assert(l3_collision_no_contact_result_8004bc2c(1,2,-10,0,0,0) == 0x1904);
    assert(l3_collision_no_contact_result_8004bc2c(1,10,2,0,0,0) == 0x1505);
    // Exact strict comparisons mean X/Z ties select Z; horizontal/Y ties select Y.
    assert((l3_collision_no_contact_result_8004bc2c(10,0,10,0,0,0) & 0xFF) == 3);
    assert((l3_collision_no_contact_result_8004bc2c(10,10,1,0,0,0) & 0xFF) == 5);

    // USA 0x8004B1E0 candidate gather: center, X-neighbor, Z-neighbor,
    // diagonal; positive packed cells de-duplicate while negative and optional
    // zero cells preserve retail behavior.
    l3::RuntimeMap candidate_map;
    l3::RuntimeMapArea candidate_area{};
    candidate_area.start_x=63; candidate_area.start_z=63; candidate_area.width=3; candidate_area.height=3;
    candidate_area.cell_first=0; candidate_area.cell_count=9;
    candidate_map.areas.push_back(candidate_area);
    candidate_map.cells = {0,-1,0,5,5,0,0,0,0};
    auto candidates = l3::runtime_map_collect_contact_candidates_8004b1e0(candidate_map,0,0,0,false);
    assert(candidates.size()==2);
    assert(candidates[0].cell_index==4 && candidates[0].packed_cell==5);
    assert(candidates[1].cell_index==1 && candidates[1].packed_cell==-1);
    candidates = l3::runtime_map_collect_contact_candidates_8004b1e0(candidate_map,0,0,0,true);
    assert(candidates.size()==3 && candidates[2].cell_index==0 && candidates[2].packed_cell==0);

    // USA jump launch/hold-gravity byte mutations are exact compatibility helpers.
    player_work = {}; player_work.bytes[0xA7] = 0xFF; player_work.bytes[0x0A] = 4; player_work.bytes[0xAC] = 0x5A;
    assert(l3_pl00_jump_launch_8003dac0(&player_work) == 1);
    assert(player_work.bytes[0x74] == 1 && player_work.bytes[0x0A] == 5 && player_work.bytes[0xAD] == 0x5A);
    assert(pw_gets16(player_work, 0x46) == -0x1C0 && l3_pl00_shot_is_enabled(&player_work));
    player_work = {}; player_work.bytes[0xA7] = 0xFF; player_work.bytes[0x16B] = 1;
    assert(l3_pl00_jump_launch_8003dac0(&player_work) == 1 && pw_gets16(player_work, 0x46) == -0x320);
    pw_put16(player_work, 0x138, 0x0040); pw_put16(player_work, 0x11C, 0x0040); pw_put16(player_work, 0x46, uint16_t(int16_t(-0x60)));
    assert(l3_pl00_jump_hold_gravity_8003db50(&player_work) == 1 && pw_gets16(player_work, 0x46) == -0x30);
    assert(l3_pl00_jump_hold_gravity_8003db50(&player_work) == 2 && pw_gets16(player_work, 0x46) == 0);
    pw_put16(player_work, 0x11C, 0); pw_put16(player_work, 0x46, uint16_t(int16_t(-0x60)));
    assert(l3_pl00_jump_hold_gravity_8003db50(&player_work) == 0 && pw_gets16(player_work, 0x46) == -0x60);

    // Fixed-point movement math uses the same Q12 convention as the original table.
    l3::ReferenceTables refs{};
    refs.trig[0] = {0, 4096};
    refs.trig[0x400] = {4096, 0};
    refs.trig[0x800] = {0, -4096};
    refs.trig[0xC00] = {-4096, 0};
    auto md = l3::reference_player_motion_delta(refs, 0, 0x50, 0, 0);
    assert(md.dx_fixed == 0 && md.dz_fixed == -0x50000);
    assert(l3::reference_fixed_to_short_80030150(-0x50000) == -5);

    // Portable integrated runtime: exact profile + exact fixed-point forward path,
    // targeting/Buster, safe map probe, save/checkpoint, and aux-event queue cooperate.
    l3::RuntimeMap session_map;
    l3::RuntimeMapArea session_area{};
    session_area.start_x = 63; session_area.start_z = 63; session_area.width = 2; session_area.height = 2;
    session_area.cell_first = 0; session_area.cell_count = 4;
    session_map.areas.push_back(session_area);
    // Packed negative floor cells (top -1): an empty grid has no floor and the
    // source 0x8003C4EC rule would correctly start a fall.
    session_map.cells = {-32768,-32768,-32768,-32768};
    l3::TutorialRuntimeArea ta{}; ta.area_index = 0;
    l3_qol_settings_defaults(&qol);
    l3::RuntimeSession integrated_session(std::move(session_map), std::move(ta), {}, qol);
    integrated_session.set_reference_tables(refs);
    integrated_session.reset_player({0,0,0}, 0, 100, 100);
    l3::RuntimeInputFrame rif{}; rif.held_psx_mask = 0x0010;
    integrated_session.tick(rif);
    assert(integrated_session.player_position().z == -32);
    assert(integrated_session.compat_player().bytes[0x09] == 2);
    // Fractional accumulator is retained instead of being rebuilt from the short mirror.
    integrated_session.tick(rif);
    assert(integrated_session.player_position().z == -64);
    auto mp = integrated_session.probe_player_map();
    assert(mp.in_area && mp.cell.has_value());
    // The integration session can now consume a retail solver result without
    // duplicating/approximating the 0x8004B414 response semantics.
    l3::RuntimeRetailContactResolution rr{}; rr.result=L3_CONTACT_RESULT_AXIS_X_MASK|L3_CONTACT_RESULT_AXIS_Y_MASK; rr.scratch={-4,6,0,3};
    assert(integrated_session.apply_retail_contact_resolution(rr)==rr.result);
    assert(integrated_session.player_position().x==-4 && integrated_session.player_position().y==6);
    // Contact dispatcher identity and the shared family-0/1 geometry classifier
    // are exact retail translations, without assigning guessed floor/wall semantics.
    assert(l3::runtime_contact_dispatch_family(0x0100).has_value());
    assert(l3::runtime_contact_dispatch_family(0x0170).has_value());
    assert(!l3::runtime_contact_dispatch_family(0x0180).has_value());
    l3::RuntimeContactGroupRef cgr{}; cgr.world_x0=0; cgr.world_x1=100; cgr.world_z0=0; cgr.world_z1=100;
    l3::RuntimeContactFamily01Query cq{}; cq.query_x=25; cq.query_z=50;
    assert(l3::runtime_contact_family01_classify_8004e304(cgr,cq)==4);
    cq.query_x=75; assert(l3::runtime_contact_family01_classify_8004e304(cgr,cq)==1);
    cq.query_x=101; assert(l3::runtime_contact_family01_classify_8004e304(cgr,cq)==0);
    // Combat smoke starts in idle: running completion gates are tested separately
    // with actual S records, which this map-only fixture deliberately lacks.
    integrated_session.reset_player(integrated_session.player_position(),0,100,100);
    const auto combat_origin=integrated_session.player_position();
    integrated_session.register_target({7,{combat_origin.x,combat_origin.y,combat_origin.z-207},1,2,true,true});
    rif = {}; rif.lock_held = true; rif.fire_pressed = true;
    integrated_session.tick(rif);
    assert(integrated_session.locked_target_id().has_value() && *integrated_session.locked_target_id() == 7);
    assert(!integrated_session.projectiles().empty());
    auto sv = integrated_session.make_save_state(42);
    auto sv_bytes = l3::cook_l3save(sv);
    auto sv2 = l3::load_l3save(sv_bytes, true);
    assert(sv2.checkpoint_id == 42 && sv2.player_z == integrated_session.player_position().z);
    l3::runtime_story_flag_set(sv2, 17, true);
    assert(l3::runtime_story_flag_get(sv2, 17));
    l3::runtime_story_flag_set(sv2, 0x604, true);
    l3::runtime_story_flag_set(sv2, 0x7E0, true);
    assert(l3::runtime_story_flag_get(sv2, 0x604) && l3::runtime_story_flag_get(sv2, 0x7E0));
    const auto retail_flag_roundtrip=l3::load_l3save(l3::cook_l3save(sv2),true);
    assert(l3::runtime_story_flag_get(retail_flag_roundtrip,0x604) && l3::runtime_story_flag_get(retail_flag_roundtrip,0x7E0));
    // Global location registration proves cross-stage save identity is independent of a map's local area index.
    l3::RuntimeMap second_map; second_map.areas.push_back(session_area); second_map.cells={0,0,0,0};
    l3::TutorialRuntimeArea second_area{}; second_area.area_index=0;
    integrated_session.register_location(77, std::move(second_map), 0, std::move(second_area));
    assert(integrated_session.has_registered_location(77));
    assert(integrated_session.transition_to_location(77,{0,0,0},0));
    assert(integrated_session.area_index()==77 && integrated_session.map_area_index()==0);
    integrated_session.set_story_flag(23,true);
    const auto world_save=integrated_session.make_save_state(77);
    assert(integrated_session.transition_to_location(0,{0,0,0},0));
    integrated_session.apply_save_state(world_save);
    assert(integrated_session.area_index()==77 && integrated_session.story_flag(23));

    // Gameplay input is owned by dialogue/cutscenes, so player motion cannot leak through scripted scenes.
    l3::MmlMessageFile runtime_msg; runtime_msg.blocks.resize(1);
    runtime_msg.blocks[0].tokens.push_back({l3::MmlMessageTokenKind::Control,0x8F,{0x34,0x12,0x00,0x00}});
    runtime_msg.blocks[0].tokens.push_back({l3::MmlMessageTokenKind::Glyph,0x22,{}});
    l3::RuntimeMap dialogue_map; dialogue_map.areas.push_back(session_area); dialogue_map.cells={0,0,0,0};
    l3::TutorialRuntimeArea dialogue_area{}; dialogue_area.area_index=0;
    l3::RuntimeSession dialogue_session(std::move(dialogue_map),std::move(dialogue_area),{runtime_msg},qol);
    dialogue_session.set_reference_tables(refs); dialogue_session.reset_player({0,0,0},0,100,100); dialogue_session.start_dialogue({0,0});
    l3::RuntimeInputFrame dialogue_input{}; dialogue_input.held_psx_mask=0x0010; dialogue_session.tick(dialogue_input);
    assert(dialogue_session.player_position().z==0 && dialogue_session.dialogue().waiting_for_audio);
    auto aq=dialogue_session.take_audio_requests(); assert(aq.size()==1 && aq[0].command_id==0x1234u);
    dialogue_session.complete_audio(0x9999u); dialogue_session.tick({}); assert(dialogue_session.dialogue().waiting_for_audio);
    dialogue_session.complete_audio(0x1234u); dialogue_session.tick({}); assert(!dialogue_session.dialogue().visible_glyphs.empty());

    // Safe skip only exists for a scene with a proven completion contract and requires a one-second hold.
    l3::RuntimeCutsceneCompletionContract cc{}; cc.id=9; cc.flags_on={0x604}; cc.flags_off={23}; cc.buster_enabled=false;
    integrated_session.register_cutscene_completion(cc); integrated_session.begin_cutscene(9); assert(integrated_session.cutscene_skippable());
    l3::RuntimeInputFrame skip{}; skip.skip_held=true; skip.held_psx_mask=0x0010;
    const auto before_skip_pos=integrated_session.player_position();
    for(int i=0;i<59;++i)integrated_session.tick(skip);
    assert(integrated_session.cutscene_active() && integrated_session.player_position().z==before_skip_pos.z);
    integrated_session.tick(skip);
    assert(!integrated_session.cutscene_active() && integrated_session.story_flag(0x604) && !integrated_session.story_flag(23));
    assert(!l3_pl00_shot_is_enabled(&integrated_session.compat_player()));

    auto damaged = sv_bytes; damaged.back() ^= 0x80;
    bool bad_save_rejected = false;
    try { (void)l3::load_l3save(damaged, true); } catch (...) { bad_save_rejected = true; }
    assert(bad_save_rejected);

    // Aux matches are queued as evidence-backed trigger records; no guessed side effects.
    l3::RuntimeMap aux_map; aux_map.areas.push_back(session_area); aux_map.cells={0,0,0,0};
    l3::TutorialRuntimeArea aux_area{}; aux_area.area_index=0; aux_area.flow.aux=aux_test;
    l3::RuntimeSession ars(std::move(aux_map), std::move(aux_area), {}, qol);
    l3::St00ContactSample qs{-256,-384,15616,0x0800,8,0};
    ars.submit_contact_sample(qs);
    auto queued = ars.take_aux_triggers();
    assert(queued.size()==1 && queued[0].record_index==0);
    assert(ars.take_aux_triggers().empty());

    // Original USA dynamic EBD selector evidence (initializer 0x80047354).
    // These tests encode the branch itself, not an unproved assignment of that handler
    // to every subtype-08 stage object.
    const std::array<std::pair<uint8_t, uint8_t>, 6> tutorial_dynamic_params{{
        {0x61u, 2u}, {0x81u, 0u}, {0xC1u, 0u},
        {0x60u, 2u}, {0xA0u, 0u}, {0xE0u, 0u},
    }};
    for (const auto& [param3, expected] : tutorial_dynamic_params) {
        const auto plan = l3::ebd_selector_plan_80047354(param3);
        assert(plan.selector_is_known && plan.selector == expected);
        if (expected == 2u) {
            assert(plan.spawn_class80_helper_pair);
            assert(plan.helper_param3_a == 5u && plan.helper_param3_b == 6u);
        } else {
            assert(!plan.spawn_class80_helper_pair);
        }
    }
    for (uint8_t fixed : {4u, 5u, 6u}) {
        const auto plan = l3::ebd_selector_plan_80047354(fixed);
        assert(plan.selector_is_known && plan.selector == fixed);
        assert(plan.source == l3::EbdSelectorSource::FixedLowNibble);
    }
    const auto runtime_plan = l3::ebd_selector_plan_80047354(0x03u);
    assert(!runtime_plan.selector_is_known);
    assert(runtime_plan.source == l3::EbdSelectorSource::RuntimeProbeZeroOrOne);
    assert(l3::ebd_selector_from_probe_80047354(129, 100) == 0u);
    assert(l3::ebd_selector_from_probe_80047354(130, 100) == 1u);
    assert(l3::ebd_selector_from_probe_80047354(50, 100) == 0u);


    // Cooked player assets now have full runtime readers, closing the old
    // host-cooker -> 3DS-runtime gap for L3MD/L3PC/L3AN/L3SK.
    l3::PbdModel cooked_model_source;
    l3::PbdPart cooked_part; cooked_part.part_id=3;
    cooked_part.vertices={{1,2,3,0},{4,5,6,0},{7,8,9,0}};
    l3::PbdTri cooked_tri; cooked_tri.index={0,1,2}; cooked_tri.uv={1,2,3,4,5,6};
    cooked_tri.clut=0x1234; cooked_tri.tpage=0x55; cooked_tri.r=10; cooked_tri.g=20; cooked_tri.b=30; cooked_tri.code=0x24;
    cooked_part.triangles.push_back(cooked_tri); cooked_model_source.parts.push_back(cooked_part);
    const auto cooked_model_bytes=l3::cook_l3model(cooked_model_source);
    const auto cooked_model_loaded=l3::load_l3model(cooked_model_bytes,true);
    assert(cooked_model_loaded.parts.size()==1 && cooked_model_loaded.parts[0].vertices.size()==3);
    assert(cooked_model_loaded.parts[0].triangles[0].clut==0x1234 && cooked_model_loaded.parts[0].triangles[0].index[2]==2);

    l3::PlayerComposition cooked_comp_source;
    cooked_comp_source.attachments.push_back({0,0,3,0});
    const auto cooked_comp_bytes=l3::cook_l3player_composition(cooked_comp_source);
    const auto cooked_comp_loaded=l3::load_l3player_composition(cooked_comp_bytes,true);
    assert(cooked_comp_loaded.attachments.size()==1 && cooked_comp_loaded.attachments[0].part_id==3);

    l3::PbdAnimation cooked_anim_source; cooked_anim_source.action_slots=2;
    l3::PbdAnimationAction cooked_action; cooked_action.action_index=1; cooked_action.has_m_pointer=true; cooked_action.m_is_frame_table=true;
    std::array<uint16_t,l3::kPbdPackedAngleChannels> cooked_frame{}; cooked_frame[0]=0x7FF; cooked_frame[3]=0x400; cooked_frame[37]=0xABC;
    cooked_action.angle_frames.push_back(cooked_frame); cooked_anim_source.actions.push_back(cooked_action);
    const auto cooked_anim_bytes=l3::cook_l3animation(cooked_anim_source);
    const auto cooked_anim_loaded=l3::load_l3animation(cooked_anim_bytes,true);
    assert(cooked_anim_loaded.action_slots==2 && cooked_anim_loaded.actions.size()==1);
    assert(cooked_anim_loaded.actions[0].angle_frames[0][37]==0xABC);

    l3::PbdMainSkeleton cooked_skel_source; cooked_skel_source.source_special_offset=0x44;
    cooked_skel_source.base_translation[1]={11,-12,13}; cooked_skel_source.source_reserved[1]=-7;
    const auto cooked_skel_bytes=l3::cook_l3skeleton(cooked_skel_source);
    const auto cooked_skel_loaded=l3::load_l3skeleton(cooked_skel_bytes,true);
    assert(cooked_skel_loaded.source_special_offset==0x44 && cooked_skel_loaded.parent[14]==13);
    assert(cooked_skel_loaded.base_translation[1][1]==-12 && cooked_skel_loaded.source_reserved[1]==-7);

    // Exact 0x80041DDC request-state transitions used by ordinary movement.
    L3PlayerCompatWork anim_req{};
    anim_req.bytes[0xAD] = 0x01u;
    anim_req.bytes[0xA4] = 0x0Fu;
    l3_pl00_anim_request_80041ddc(&anim_req, 0x0Cu, 0u, 0u);
    assert(anim_req.bytes[0xAC] == 0x0Cu);
    assert(anim_req.bytes[0xA4] == 0x02u); // 0x0F*2+4 -> 0x22, wrapped to 0x02
    anim_req.bytes[0xAD] = 0x0Cu;
    anim_req.bytes[0xA4] = 0x01u;
    l3_pl00_anim_request_80041ddc(&anim_req, 0x01u, 0u, 0u);
    assert(anim_req.bytes[0xAC] == 0x01u);
    assert(anim_req.bytes[0xA4] == 0x0Eu); // (1+0x1C)>>1
    anim_req.bytes[0xAD] = 0x05u;
    anim_req.bytes[0xAC] = 0x99u;
    l3_pl00_anim_request_80041ddc(&anim_req, 0x05u, 7u, 0u);
    assert(anim_req.bytes[0xAC] == 0x99u); // identical request early-return
    l3_pl00_anim_request_80041ddc(&anim_req, 0x07u, 3u, 1u);
    assert(anim_req.bytes[0xAD] == 0xFFu && anim_req.bytes[0xAC] == 0x07u && anim_req.bytes[0xA4] == 3u);

    // Exact pointer-free PL00M00 S->M sequencing recovered from
    // 0x800320B8 / 0x8003234C / 0x800329D4. Request 0x0C uses S action 12
    // and M action 13 (the M file has its skeleton anchor in slot 0).
    l3::PbdAnimation seq_anim; seq_anim.action_slots = 14;
    l3::PbdAnimationAction seq_s; seq_s.action_index = 12; seq_s.has_s_records = true;
    seq_s.s_records = {
        {0x0100u, 0x0000u, 0u}, // pose 0, duration 1, sequential
        {0x0201u, 0x0000u, 0u}, // pose 1, duration 2, sequential
        {0x0102u, 0x8000u, 0u}, // pose 2, duration 1, branch to record 0
    };
    l3::PbdAnimationAction seq_m; seq_m.action_index = 13; seq_m.has_m_pointer = true; seq_m.m_is_frame_table = true;
    seq_m.angle_frames.resize(3); seq_m.angle_frames[0][0]=0x100; seq_m.angle_frames[1][0]=0x200; seq_m.angle_frames[2][0]=0x300;
    seq_anim.actions.push_back(seq_s); seq_anim.actions.push_back(seq_m);
    auto seq = l3::pbd_main_body_track_begin(seq_anim, 0x0Cu, 0u);
    assert(seq.valid && seq.record_index==0 && seq.ticks_remaining==1 && seq.pose_frame==0);
    assert(l3::pbd_main_body_track_frame(seq_anim,seq)->at(0)==0x100);
    assert(l3::pbd_main_body_track_tick(seq_anim,seq));
    assert(seq.record_index==1 && seq.ticks_remaining==2 && seq.pose_frame==1);
    assert(l3::pbd_main_body_track_tick(seq_anim,seq)); // 2 -> 1, hold record 1
    assert(seq.record_index==1 && seq.ticks_remaining==1 && seq.pose_frame==1);
    assert(l3::pbd_main_body_track_tick(seq_anim,seq)); // 1 -> 0, advance
    assert(seq.record_index==2 && seq.ticks_remaining==1 && seq.pose_frame==2);
    assert(l3::pbd_main_body_track_tick(seq_anim,seq)); // signed 0x80 control -> branch record 0
    assert(seq.record_index==0 && seq.ticks_remaining==1 && seq.pose_frame==0);
    assert(l3::pbd_main_body_pose_action_for_request(seq_anim,0x0Cu)==&seq_anim.actions[1]);

    // -1 control repeats the current S record and reloads its duration byte.
    seq_anim.actions[0].s_records[0] = {0x0300u,0xFF00u,0u};
    seq = l3::pbd_main_body_track_begin(seq_anim,0x0Cu,0u);
    assert(seq.ticks_remaining==3);
    assert(l3::pbd_main_body_track_tick(seq_anim,seq) && seq.ticks_remaining==2);
    assert(l3::pbd_main_body_track_tick(seq_anim,seq) && seq.ticks_remaining==1);
    assert(l3::pbd_main_body_track_tick(seq_anim,seq) && seq.record_index==0 && seq.ticks_remaining==3);

    // PICA200 GPU_RGBA8 layout: 8x8 Morton order and A,B,G,R bytes.
    std::vector<uint8_t> linear_rgba(8u * 8u * 4u, 0);
    for (size_t px = 0; px < 64u; ++px) {
        linear_rgba[px * 4u + 0] = static_cast<uint8_t>(px);       // R
        linear_rgba[px * 4u + 1] = static_cast<uint8_t>(px + 1u);  // G
        linear_rgba[px * 4u + 2] = static_cast<uint8_t>(px + 2u);  // B
        linear_rgba[px * 4u + 3] = static_cast<uint8_t>(px + 3u);  // A
    }
    const auto tiled = l3::ctr_rgba8_tiled_abgr(linear_rgba, 8, 8);
    // Morton positions 0,1,2,3 correspond to source texels 0,1,8,9.
    const std::array<size_t,4> src_px{{0u,1u,8u,9u}};
    for (size_t morton = 0; morton < src_px.size(); ++morton) {
        const size_t sp = src_px[morton];
        const size_t d = morton * 4u;
        assert(tiled[d+0] == static_cast<uint8_t>(sp+3u));
        assert(tiled[d+1] == static_cast<uint8_t>(sp+2u));
        assert(tiled[d+2] == static_cast<uint8_t>(sp+1u));
        assert(tiled[d+3] == static_cast<uint8_t>(sp));
    }

    std::cout << "core tests: PASS\n";
    return 0;
}
