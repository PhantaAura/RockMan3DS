#include "checkpoint_version.h"
#include "hardware_test.h"
#include "legends3ds/test_boot_policy.h"
#include <cstdio>
#include <climits>
#include <cstdarg>
#include <malloc.h>

namespace {
FILE* log_file=nullptr;
bool log_failed=false;
u64 previous_start=0;
u32 interval_us=0, previous_held=0;
size_t previous_area=0;
bool model_new=false;
Result model_result=-1;

void flush_log() {
    if(log_file && (std::ferror(log_file) || std::fflush(log_file)!=0)) {
        std::fclose(log_file);log_file=nullptr;log_failed=true;
    }
}
}

void hardware_test_line(const char* format, ...) {
    //320-pixel console =40columns. Keep each diagnostic on one row.
    char line[40];va_list args;va_start(args,format);
    std::vsnprintf(line,sizeof(line),format,args);va_end(args);
    std::printf("\x1b[2K%s\n",line);
}

void hardware_test_trace(const char* format, ...) {
    if(!log_file)return;
    va_list args;va_start(args,format);
    std::vfprintf(log_file,format,args);va_end(args);
    std::fputc('\n',log_file);
    // Existing sparse frame/lifecycle flushes publish these bounded records.
    // No per-record flush is added to the native update loop.
}

bool hardware_test_launch(MMLRendererGate& gate,bool& xyz_candidate) {
    if(FILE* policy=std::fopen("sdmc:/3ds/Legends3DS/test_boot.cfg","rb")) {
        char text[32]{};const auto size=std::fread(text,1,sizeof(text),policy);
        const bool failed=std::ferror(policy)!=0;const bool extra=std::fgetc(policy)!=EOF;
        std::fclose(policy);
        if(!failed&&!extra&&l3::parse_test_boot_policy(std::string_view(text,size))==
           l3::TestBootPolicy::SelectedRenderer)return true;
    }
    const auto show=[&]() {
        consoleClear();
        hardware_test_line("LEGENDS3DS " MML_CHECKPOINT_VERSION "");
        hardware_test_line("OLD3DS GAMEPLAY RESOURCE TEST");
        hardware_test_line("");
        hardware_test_line("A: Start selected renderer");
        hardware_test_line("X: Start safe0143 fallback");
        hardware_test_line("B: Start textures-off diagnosis");
        hardware_test_line("Y: Toggle XYZ / legacy");
        hardware_test_line("Geometry: %s",xyz_candidate?"IDX + GRID XYZ":"LEGACY FLAT");
        hardware_test_line("START: Exit");
        hardware_test_line("");
        hardware_test_line("Circle Pad move | L/R turn | Y fire");
        hardware_test_line("SELECT: next test area");
        hardware_test_line("Collision/events/audio incomplete.");
        hardware_test_line("Flags:%02lx",static_cast<unsigned long>(gate.feature_mask));
    };
    show();
    while(aptMainLoop()) {
        hidScanInput();const u32 down=hidKeysDown();
        if(down&KEY_START)return false;
        if(down&KEY_Y){xyz_candidate=!xyz_candidate;show();}
        if(down&KEY_X){mml_renderer_gate_set(&gate,MML_RENDER_0143_FALLBACK);consoleClear();return true;}
        if(down&KEY_B){mml_renderer_gate_set(&gate,MML_RENDER_BATCHING_TEXTURES_OFF);consoleClear();return true;}
        if(down&KEY_A){consoleClear();return true;}
        gfxFlushBuffers();gfxSwapBuffers();gspWaitForVBlank();
    }
    return false;
}

void hardware_test_begin(bool xyz_candidate) {
    model_result=APT_CheckNew3DS(&model_new);
    char path[160];
    std::snprintf(path,sizeof(path),"sdmc:/3ds/Legends3DS/hardware_" MML_CHECKPOINT_ID "_%llu.csv",static_cast<unsigned long long>(osGetTime()));
    log_file=std::fopen(path,"w");
    if(!log_file){log_failed=true;return;}
    std::fprintf(log_file,"# build=" MML_CHECKPOINT_VERSION ",model_query_rc=%ld,new3ds=%d,memory_layout=%lu,app_region_bytes=%lu,heap_reserved=%lu,linear_reserved=%lu,stack_reserved=262144\n",
        static_cast<long>(model_result),model_new?1:0,static_cast<unsigned long>(osGetApplicationMemType()),
        static_cast<unsigned long>(osGetMemRegionSize(MEMREGION_APPLICATION)),
        static_cast<unsigned long>(envGetHeapSize()),static_cast<unsigned long>(envGetLinearHeapSize()));
    std::fprintf(log_file,"# terrain_decode=%s,evidence=%s,PHYSICS_1TO1=false\n",
        xyz_candidate?"IDX_AND_GRID_XYZ":"LEGACY_FLAT",xyz_candidate?"SOURCE_BACKED_PARTIAL":"UNKNOWN");
    std::fprintf(log_file,"# CPU timing: frame_before_log_us excludes this write/flush; interval_us includes the previous frame's logging. Sparse samples cannot establish worst-case pacing. Render includes synchronization; region reservation is not live malloc usage.\n");
    std::fprintf(log_file,"frame,area,held,pressed,x,y,z,heading,projectiles,frame_before_log_us,interval_us,update_us,render_us,bottom_ui_us,draw_calls,triangles,owned_linear,owned_texture,malloc_live,malloc_arena_free,linear_free,vram_free,action,phase,animation,animation_pose,vertical_velocity,grounded,contact_status,psx_held,psx_pressed,floor_known,floor_y,floor_cell,floor_primitive,contact_result,candidate_cells,groups_tested,primitives_tested,unsupported_primitives,contact_iterations,wall_contact,ceiling_contact,landed\n");
    flush_log();
}

void hardware_test_area_event(const char* phase, size_t area, u64 elapsed_us,
                             size_t package_bytes, size_t decoded_estimate, size_t peak_estimate) {
    if (!log_file) return;
    const struct mallinfo memory=mallinfo();
    std::fprintf(log_file,
        "# area_event=%s,area=%lu,elapsed_us=%llu,package_bytes=%lu,decoded_estimate=%lu,load_peak_estimate=%lu,malloc_live=%lu,linear_free=%lu,vram_free=%lu\n",
        phase,static_cast<unsigned long>(area),static_cast<unsigned long long>(elapsed_us),
        static_cast<unsigned long>(package_bytes),static_cast<unsigned long>(decoded_estimate),
        static_cast<unsigned long>(peak_estimate),static_cast<unsigned long>(memory.uordblks),
        static_cast<unsigned long>(linearSpaceFree()),static_cast<unsigned long>(vramSpaceFree()));
    flush_log();
}

void hardware_test_geometry_event(bool xyz_candidate,size_t area,size_t primitives,int32_t min_y,int32_t max_y) {
    if(!log_file)return;
    std::fprintf(log_file,"# geometry_event,area=%lu,decode=%s,evidence=%s,primitives=%lu,min_y=%ld,max_y=%ld\n",
        (unsigned long)area,xyz_candidate?"IDX_AND_GRID_XYZ":"LEGACY_FLAT",xyz_candidate?"SOURCE_BACKED_PARTIAL":"UNKNOWN",
        (unsigned long)primitives,(long)min_y,(long)max_y);
    flush_log();
}

void hardware_test_visibility_event(size_t area, size_t negative_bound, size_t default_unbound,
    size_t special_unbound, size_t material_unknown, size_t visible_primitives,
    size_t inspection_storage, const char* candidate_source) {
    if (!log_file) return;
    std::fprintf(log_file,"# visibility_event,area=%lu,negative_bound=%lu,default_unbound=%lu,special_unbound=%lu,negative_material_unknown=%lu,diagnostic_visible=%lu,inspection_storage=%lu,entry_source=%s,entry_status=CANDIDATE,retail_entry=UNKNOWN,ground_contact_applied=false\n",
        (unsigned long)area,(unsigned long)negative_bound,(unsigned long)default_unbound,
        (unsigned long)special_unbound,(unsigned long)material_unknown,(unsigned long)visible_primitives,
        (unsigned long)inspection_storage,candidate_source);
    flush_log();
}

void hardware_test_sample(const l3::RuntimeSession& session,const MMLRuntimeDiag& d,
                          u64 start,u32 held,u32 pressed,size_t area) {
    interval_us=previous_start?static_cast<u32>((start-previous_start)/CPU_TICKS_PER_USEC):0;
    previous_start=start;
    const auto& work=session.compat_player();
    static unsigned previous_state=UINT_MAX;
    const unsigned state=unsigned(work.bytes[9])|(unsigned(work.bytes[10])<<8)|(unsigned(work.bytes[0xAC])<<16);
    const auto& contact=session.player_contact();
    static unsigned previous_contact_state=UINT_MAX;
    const unsigned contact_state=unsigned(contact.status)|
        ((contact.grounded?(*contact.grounded?2u:1u):0u)<<3)|
        (unsigned(contact.wall_contact)<<5)|(unsigned(contact.ceiling_contact)<<6);
    const bool changed=held!=previous_held||area!=previous_area||state!=previous_state||
        contact_state!=previous_contact_state;
    previous_contact_state=contact_state;
    previous_state=state;
    previous_held=held;previous_area=area;
    if(!log_file||(!changed&&!pressed&&session.frame_counter()>5&&session.frame_counter()%120u))return;
    const auto p=session.player_position();const struct mallinfo memory=mallinfo();
    const auto read16=[&](size_t offset){return uint16_t(work.bytes[offset])|uint16_t(uint16_t(work.bytes[offset+1])<<8);};
    std::fprintf(log_file,"%lu,%lu,%lu,%lu,%ld,%ld,%ld,%u,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%u,%u,%u,%u,%d,%d,%u,%u,%u,%u,%d,%ld,%ld,%ld,%lu,%lu,%lu,%lu,%u,%u,%u,%u\n",
        static_cast<unsigned long>(session.frame_counter()),static_cast<unsigned long>(area),
        static_cast<unsigned long>(held),static_cast<unsigned long>(pressed),
        static_cast<long>(p.x),static_cast<long>(p.y),static_cast<long>(p.z),session.player_yaw(),
        static_cast<unsigned long>(session.projectiles().size()),static_cast<unsigned long>(d.frame_us),
        static_cast<unsigned long>(interval_us),static_cast<unsigned long>(d.phase_us[MML_PHASE_UPDATE]),
        static_cast<unsigned long>(d.phase_us[MML_PHASE_RENDER]),static_cast<unsigned long>(d.phase_us[MML_PHASE_BOTTOM_UI]),static_cast<unsigned long>(d.draw_calls),
        static_cast<unsigned long>(d.triangles),static_cast<unsigned long>(d.linear_bytes),
        static_cast<unsigned long>(d.texture_bytes),static_cast<unsigned long>(memory.uordblks),
        static_cast<unsigned long>(memory.fordblks),static_cast<unsigned long>(linearSpaceFree()),
        static_cast<unsigned long>(vramSpaceFree()),unsigned(work.bytes[9]),unsigned(work.bytes[10]),
        unsigned(work.bytes[0xAC]),unsigned(work.bytes[0xA4]),int(static_cast<int16_t>(read16(0x46))),
        contact.grounded?int(*contact.grounded):-1,unsigned(contact.status),unsigned(read16(0x11C)),unsigned(read16(0x11E)),
        unsigned(contact.floor_y.has_value()),int(contact.floor_y.value_or(0)),
        contact.floor_cell_index?static_cast<long>(*contact.floor_cell_index):-1L,
        contact.floor_primitive_index?static_cast<long>(*contact.floor_primitive_index):-1L,
        static_cast<long>(contact.result),static_cast<unsigned long>(contact.candidate_cells),
        static_cast<unsigned long>(contact.groups_tested),static_cast<unsigned long>(contact.primitives_tested),
        static_cast<unsigned long>(contact.unsupported_primitives),unsigned(contact.iterations),
        unsigned(contact.wall_contact),unsigned(contact.ceiling_contact),unsigned(contact.landed));
    flush_log();
}

void hardware_test_hud(const MMLRuntimeDiag& d,const l3::RuntimeSession& session) {
    const auto p=session.player_position();const struct mallinfo memory=mallinfo();
    hardware_test_line("Frame %lu %s %luus",static_cast<unsigned long>(session.frame_counter()),
        R_FAILED(model_result)?"MODEL?":model_new?"NEW3DS":"OLD3DS",static_cast<unsigned long>(interval_us));
    hardware_test_line("Draw %lu Tri %lu Heap %luK",static_cast<unsigned long>(d.draw_calls),
        static_cast<unsigned long>(d.triangles),static_cast<unsigned long>(memory.uordblks/1024u));
    hardware_test_line("Free linear %luK VRAM %luK",static_cast<unsigned long>(linearSpaceFree()/1024u),static_cast<unsigned long>(vramSpaceFree()/1024u));
    hardware_test_line("XYZ %ld %ld %ld",static_cast<long>(p.x),static_cast<long>(p.y),static_cast<long>(p.z));
    hardware_test_line("Yaw %u Shots %lu Log %s",session.player_yaw(),static_cast<unsigned long>(session.projectiles().size()),log_failed?"FAIL":log_file?"ON":"OFF");
}

void hardware_test_end() {if(log_file){std::fclose(log_file);log_file=nullptr;}}

void hardware_test_actor_event(size_t area,const l3::ActorAreaRuntime& actors) {
    if (!log_file) return;
    size_t floors=0,poses=0,selected=0;
    for (const auto& actor:actors.actors) {floors+=actor.floor_sample_available;poses+=actor.pose.has_value();selected+=actor.pose_selected_by_retail_state;}
    std::fprintf(log_file,"# actor_event,area=%lu,instances=%lu,unique_resources=%lu,decoded_element_bytes=%lu,source_poses=%lu,floor_bound=%lu,initializer_selected_poses=%lu,texture=UNKNOWN,progression_pose_selection=UNKNOWN,placement=CANDIDATE\n",
        (unsigned long)area,(unsigned long)actors.actors.size(),(unsigned long)actors.unique_resource_count,
        (unsigned long)actors.decoded_resource_bytes,(unsigned long)poses,(unsigned long)floors,(unsigned long)selected);
    flush_log();
}
void hardware_test_clock_event(const l3::GameplayClock& clock) {
    static uint64_t previous=UINT64_MAX;
    if (!log_file || clock.updates()/120u==previous) return;
    previous=clock.updates()/120u;
    std::fprintf(log_file,"# gameplay_clock,updates=%llu,pending=%llu,discarded_us=%llu,nominal_hz=30,hardware_rate=HARDWARE_PENDING\n",
        (unsigned long long)clock.updates(),(unsigned long long)clock.pending_updates(),(unsigned long long)clock.discarded_us());
}

void hardware_test_actor_target_event(const l3::RuntimeSession& session,
    const l3::ActorAreaRuntime& actors, const l3::SourceAngleSteps* steps) {
    // Sparse evidence query only. No per-frame allocation and no target/AI
    // mutation. The full original detection/attack state remains unbound.
    static uint32_t last_frame = UINT32_MAX;
    const uint32_t frame = session.frame_counter();
    if (!log_file || frame == last_frame || (frame > 5 && frame % 120u)) return;
    last_frame = frame;
    const auto player = session.player_position();
    for (const auto& actor : actors.actors) {
        const auto sample = l3::sample_imported_actor_target(actor, player.x, player.z, steps);
        if (!sample) continue;
        const int32_t heading = sample->source_heading_80031198
            ? static_cast<int32_t>(*sample->source_heading_80031198) : -1;
        std::fprintf(log_file,
            "# actor_target,frame=%lu,source_index=%lu,work80=%lu,source_heading=%ld,angle_table=%s,evidence=SOURCE_BACKED_PARTIAL,ai_bound=false,lock_bound=false\n",
            static_cast<unsigned long>(frame), static_cast<unsigned long>(actor.source_index),
            static_cast<unsigned long>(sample->source_work_80), static_cast<long>(heading),
            steps ? "CRC_VALID" : "UNKNOWN");
    }
    flush_log();
}

void hardware_test_actor_state_event(const l3::RuntimeSession& session,
    const l3::ActorAreaRuntime& actors) {
    static uint32_t last_frame=UINT32_MAX;
    const uint32_t frame=session.frame_counter();
    if(!log_file||frame==last_frame||(frame>40&&frame%120u))return;
    last_frame=frame;
    for(const auto& actor:actors.actors) {
        if(!actor.source_behavior)continue;
        const auto& state=*actor.source_behavior;
        const auto& behavior=state.behavior;
        std::fprintf(log_file,
            "# actor_state1,frame=%lu,source_index=%lu,initialized=%u,status=%u,action=%u,phase=%u,timer32c=%d,flags328=%u,request=%u,pose_request=%u,pose_record=%u,hp_known=%u,bank_requests=%lu,full_callback=false,retail_parity=false\n",
            static_cast<unsigned long>(frame),static_cast<unsigned long>(actor.source_index),
            unsigned(actor.source_behavior_initialized),unsigned(actor.last_behavior_status),
            unsigned(behavior.action_09),unsigned(behavior.phase_0a),int(behavior.timer_32c),
            unsigned(behavior.flags_328),unsigned(behavior.animation_ac),
            actor.pose?unsigned(actor.pose->request):255u,
            actor.pose?unsigned(actor.pose->record_index):255u,unsigned(state.health_6e.has_value()),
            static_cast<unsigned long>(actor.animation_bank?actor.animation_bank->requests.size():0));
        std::fprintf(log_file,
            "# actor_source_context,frame=%lu,source_index=%lu,detector_status=%u,preparation_status=%u,detector_phase=%u,detector_countdown=%u,hp=%d,combat_A_known=%u,stats_selector=%d,scratch75=%d,audio_count=%u,last_audio_queued=%u,last_audio_drops=%u,audio_playback=false\n",
            static_cast<unsigned long>(frame),static_cast<unsigned long>(actor.source_index),
            unsigned(actor.last_detection_status),actor.last_detection_preparation_status?unsigned(*actor.last_detection_preparation_status):255u,
            unsigned(state.detection_phase_353),unsigned(state.detection_countdown_35a),int(state.health_6e.value_or(-1)),
            unsigned(actor.source_combat_byte_a.has_value()),
            session.source_stats_selector()?int(*session.source_stats_selector()):-1,
            session.active_area().source_render_scratch_75?int(*session.active_area().source_render_scratch_75):-1,
            unsigned(session.source_audio_queue().count),unsigned(actor.last_source_audio_queued),unsigned(actor.last_source_audio_dropped));
    }
    flush_log();
}

void hardware_test_actor_action2_probe(const l3::ActorAreaRuntime& actors,
    const l3::RuntimeMap& map,size_t area,const l3::ReferenceTables& refs) {
    if(!log_file)return;
    // Area-load diagnostic only. Copies share immutable resources; temporary
    // live-pose storage is released before ordinary ticks. Source AI admission
    // is authored here; no active actor position or action is modified.
    for(const auto& active:actors.actors) {
        if(!active.source_behavior_initialized)continue;
        auto candidate=active;
        candidate.source_behavior->behavior.action_09=2;
        candidate.source_behavior->behavior.phase_0a=0;
        candidate.source_behavior->behavior.turn_66=0;
        const auto motion=l3::prepare_imported_actor_action2_contact(candidate,map,area,refs);
        std::fprintf(log_file,
            "# actor_action2_query,source_index=%lu,status=%u,wall_status=%u,wall_result=%ld,floor_status=%u,floor_word=%ld,input_ready=%u,after_x=%d,after_y=%d,after_z=%d,active_actor_modified=false,admission=AUTHORED_ACTION2,retail_parity=false\n",
            static_cast<unsigned long>(active.source_index),unsigned(motion.status),unsigned(motion.wall.status),
            static_cast<long>(motion.wall.result),unsigned(motion.floor.status),static_cast<long>(motion.floor.result),
            unsigned(motion.inputs.has_value()),int(motion.after_wall.raw[0]),int(motion.after_wall.raw[1]),int(motion.after_wall.raw[2]));
    }
    flush_log();
}

void hardware_test_actor_action4_probe(const l3::ActorAreaRuntime& actors,
    const l3::RuntimeMap& map,size_t area,const l3::ReferenceTables& refs) {
    if(!log_file)return;
    for(const auto& active:actors.actors) {
        if(!active.source_behavior_initialized)continue;
        // Area-load copy test. The active actor, original RNG chronology and
        // gameplay queue are untouched. This is not an observed retail action.
        auto candidate=active;
        auto& state=*candidate.source_behavior;
        state.behavior.parameter_03|=0x20u;
        state.behavior.flags_328=0;state.behavior.action_09=4;state.behavior.phase_0a=0;
        l3::SourceAudioQueue queue;l3::initialize_source_audio_queue_80019e14(queue);
        l3::Class20_03CallbackInputs input;input.action.packet_33c=0;
        const auto motion=l3::prepare_imported_actor_action4_contact(candidate,map,area,refs);
        const auto result=l3::tick_imported_actor_state1(candidate,input,nullptr,nullptr,&queue,
            nullptr,nullptr,&motion);
        std::fprintf(log_file,"# actor_action4_runtime_probe,area=%lu,source_index=%lu,motion=%u,body=%u,committed=%u,phase=%u,timer=%d,request=%u,audio=%u,byte_c_available=%u,active_actor_modified=false,admission=AUTHORED_ACTION4,retail_parity=false\n",
            static_cast<unsigned long>(area),static_cast<unsigned long>(active.source_index),
            unsigned(motion.status),unsigned(result.status),unsigned(result.committed),
            unsigned(state.behavior.phase_0a),int(state.behavior.timer_32c),
            unsigned(candidate.pose?candidate.pose->request:255u),unsigned(queue.count),
            unsigned(candidate.source_combat_byte_c.has_value()));
        // Prove that a caller cannot fabricate the child consumer or pretend
        // the allocator was exhausted to push the live bridge past this gate.
        candidate=active;
        candidate.source_behavior->behavior.parameter_03|=0x20u;
        candidate.source_behavior->behavior.flags_328=0;
        candidate.source_behavior->behavior.action_09=4;
        candidate.source_behavior->behavior.phase_0a=2;
        candidate.source_behavior->behavior.timer_32c=1;
        input.produced_child_374=0;
        const auto held=l3::tick_imported_actor_state1(candidate,input,nullptr,nullptr,&queue);
        std::fprintf(log_file,"# actor_action4_child_boundary,area=%lu,source_index=%lu,body=%u,committed=%u,active_actor_modified=false,admission=AUTHORED_CHILD_HOLD,retail_parity=false\n",
            static_cast<unsigned long>(area),static_cast<unsigned long>(active.source_index),
            unsigned(held.status),unsigned(held.committed));
    }
    flush_log();
}
