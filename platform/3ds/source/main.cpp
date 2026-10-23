#include "checkpoint_version.h"
#include "legends3ds/gameplay_gates.h"
#include <3ds.h>
#include <citro3d.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "vshader0143_shbin.h"
#include "vshader_uv_shbin.h"
#include "legends3ds/l3map.h"
#include "legends3ds/area_package.h"
#include "legends3ds/map_initialization.h"
#include "legends3ds/st00_render.h"
#include "legends3ds/pbd_model.h"
#include "legends3ds/player_composition.h"
#include "legends3ds/pbd_animation.h"
#include "legends3ds/player_pose.h"
#include "legends3ds/visual_bridge.h"
#include "legends3ds/reference_tables.h"
#include "legends3ds/source_profile_class60_06.h"
#include "legends3ds/source_event_pipeline.h"
#include "legends3ds/source_event_package.h"
#include "legends3ds/source_event_command_package.h"
#include "legends3ds/scene_e0.h"
#include "legends3ds/scene_e0_material.h"
#include "legends3ds/source_scene_actor40.h"
#include "legends3ds/source_reaction_profile.h"
#include "legends3ds/source_scene_view.h"
#include "legends3ds/source_hit_proxy.h"
#include "legends3ds/child_runtime.h"
#include "legends3ds/source_rng_service.h"
#include "legends3ds/source_player_pose.h"
#include "legends3ds/source_buster_profile.h"
#include "legends3ds/runtime_session.h"
#include "legends3ds/runtime_animation.h"
#include "legends3ds/gameplay_clock.h"
#include "legends3ds/actor_runtime.h"
#include "legends3ds/actor_detection_preparation.h"
#include "legends3ds/actor_detection_profile.h"
#include "legends3ds/actor_material.h"
#include "legends3ds/tutorial_runtime.h"
#include "legends3ds/bottom_map.h"
#include "legends3ds/material_cache.h"
#include "legends3ds/ctr_texture_layout.h"
#include "mml_renderer_recovery.h"
#include "mml_runtime_diag_v2.h"
#include "hardware_test.h"
#include "native_xa_audio.h"
#include "legends3ds/source_camera.h"
#include "legends3ds/source_player_buster.h"
#include "mockup_hud.h"
#include "legends3ds/render/terrain_visibility.h"
#include "legends3ds/source_gate_e007.h"
#include "legends3ds/source_target_e008.h"
#include "legends3ds/render/frontend_view.h"
#include "legends3ds/render/ps1_geometry.h"
#include "legends3ds/render/terrain_geometry_candidate.h"
#include "legends3ds/render/positive_terrain.h"
#include "legends3ds/render/negative_terrain.h"
#include "legends3ds/render/area_inspection.h"
#include "legends3ds/render/gpu_frame_wait.h"
#include "legends3ds/render/area_placement.h"
#include "legends3ds/render/terrain_descriptor_patch.h"
#include "legends3ds/render/ps1_texture_modulation.h"

// Retain the 0.0.14.3 stack fix that is already proven on the user's Old 3DS XL.
u32 __stacksize__ = 256 * 1024;

namespace {
constexpr const char* kDataRoot = "sdmc:/3ds/Legends3DS/data/legends3ds/tutorial/";
constexpr const char* kBootLogPath = "sdmc:/3ds/Legends3DS/boot_" MML_CHECKPOINT_ID ".log";
constexpr const char* kAreaPackagePath = "sdmc:/3ds/Legends3DS/data/legends3ds/areas/boot.mmlpkg";
// Admission limits for this prototype's CPU area data, not total-device RAM claims.
constexpr size_t kRuntimeFileLimit = 4u * 1024u * 1024u;
constexpr uint64_t kTerrainBufferLimit = 4u * 1024u * 1024u;
// Candidate admission policies, not measured physical Old3DS RAM budgets.
constexpr uint64_t kAreaMaterialDecodedLimit = 8u * 1024u * 1024u;
constexpr uint32_t kAreaTextureLimit = 8u * 1024u * 1024u;
constexpr float kWorldScale = l3::kFrontendWorldScale;
static l3::FrontendViewSettings g_view_settings{};
constexpr u32 kClearColor = 0x101820FFu;
constexpr size_t kTutorialAreaCount = 5;
constexpr int kDrawBatchVertices = 60000;
constexpr uint32_t kTexturePageBytes = uint32_t(l3::kL3MaterialTextureWidth) *
                                        uint32_t(l3::kL3MaterialTextureHeight) * 4u;

#define DISPLAY_TRANSFER_FLAGS \
    (GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
     GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
     GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

// Exact 0.0.14.3 terrain vertex shape. The fallback path uses this type, the
// original one-loader AttrInfo/BufInfo mapping, the original shader, and no
// texture/material/alpha state from 0.0.14.4/5.
struct ProbeVertex0143 { float x, y, z; };

// Enhanced path is physically separate from fallback so a failure in the new
// UV/material layout cannot contaminate the 0.0.14.3 renderer gate.
struct ProbeVertexUV { float x, y, z, u, v; };
static_assert(sizeof(ProbeVertexUV) == 20, "enhanced vertex stride must match position3/UV2");
static_assert(offsetof(ProbeVertexUV, u) == 12 && offsetof(ProbeVertexUV, v) == 16,
    "enhanced UV offsets must remain unchanged");
// Original raw quad (vertex order as cooked) for the per-frame face test.
struct TerrainQuadInfo {
    std::array<int32_t,4> x{}, y{}, z{};
    uint32_t prefix = 0;
    bool negative = false;
};
struct TerrainBatch {
    uint32_t material_word = 0;
    uint64_t key = 0; // material word | unknown-binding bit 32 (patch span buffer)
    ProbeVertexUV* vbo = nullptr;
    size_t vertex_count = 0;
    C3D_Tex texture{};
    bool texture_ready = false;
    std::vector<TerrainQuadInfo> quads;   // quad k = vertices 6k..6k+5
    uint16_t* indices = nullptr;          // linear: [0,6q) drawn, [6q,12q) black
    size_t black_count = 0;
};
struct GpuMaterial {
    uint32_t material_word = 0;
    C3D_Tex texture{};
    bool texture_ready = false;
};

static DVLB_s* g_shader_dvlb = nullptr;
static shaderProgram_s g_shader_program{};
static bool g_shader_program_ready=false;
static int g_u_projection = -1;
static int g_u_model_view = -1;
static C3D_Mtx g_projection{};
static C3D_RenderTarget* g_top_target = nullptr;
enum class HardwareSceneDraw : uint8_t { All, TerrainOnly, PlayerOnly };
static HardwareSceneDraw g_hardware_scene_draw=HardwareSceneDraw::All;
static uint32_t g_render_submissions=0;
static bool g_gpu_frame_open=false;
static bool g_gpu_queue_pending=false;
static bool g_gpu_wait_failed=false;

static ProbeVertex0143* g_terrain_vbo_0143 = nullptr;
static size_t g_terrain_vertex_count_0143 = 0;
static ProbeVertexUV* g_terrain_vbo_uv = nullptr;
static size_t g_terrain_vertex_count_uv = 0;
static std::vector<TerrainBatch> g_terrain_batches;
static bool g_area_primitive_negative = false;
// 0.0.15.8: IDX descriptor swaps (door object 0x8003AC24) patch the baked
// terrain: the swapped identity's baked vertices collapse and its live
// descriptor is drawn from this overlay. Native cache; the original redraws
// from the live IDX every frame. Overlay textures are borrowed from the batch
// with the same key and never released here. Only identities whose baked
// descriptor has word0 bit 0x1000 are tracked: the door object (0x801124B0)
// swaps only those (CANDIDATE: no other IDX writer is ported).
struct TerrainOverlay {
    uint64_t key = 0;
    void* vbo = nullptr;
    size_t vertex_count = 0;
    size_t stride = 0;
    const C3D_Tex* texture = nullptr;
};
static l3::TerrainDescriptorPatch g_terrain_patch;
static std::vector<TerrainOverlay> g_terrain_overlay;
static uint64_t g_terrain_patch_generation = UINT64_MAX;
static bool g_terrain_patch_recording = false;
static uint16_t g_area_primitive_idx = 0xFFFFu;   // 0xFFFF: not tracked for swaps
static const std::vector<uint16_t>* g_area_idx_filter = nullptr;
static std::vector<GpuMaterial> g_player_materials;
static std::vector<GpuMaterial> g_actor_materials;
static std::vector<GpuMaterial> g_scene_materials;
static std::vector<GpuMaterial> g_scene_actor40_materials;
static size_t g_current_area = 0;

static MMLRendererGate g_renderer_gate{};
static MMLBootDiag g_boot_diag{};
static MMLRuntimeDiag g_runtime_diag{};
static uint32_t g_owned_linear_bytes = 0;
static uint32_t g_owned_texture_bytes = 0;
static uint32_t g_texture_count = 0;
static bool g_first_frame_logged = false;
static uint32_t g_submitted_vertices = 0;
// Independent enhanced-renderer candidate;0143 continues the original decode.
static bool g_terrain_xyz_candidate = true;
// Owner-only request0 diagnostic. Retail callback cadence/pause/lifecycle are
// not established, so this independent gate defaults off.
static bool g_retail_actor_animation = false;
static bool g_actor_state_fields = false;
static bool g_actor_rng_candidate = false;
static bool g_source_event_boot = false;
// Exact first E0 callback stores, with an explicitly diagnostic native sweep
// and frontend Near LOD. Main-phase scheduling and source camera depth are held.
static bool g_source_scene_e0 = false;
static bool g_source_scene_textures = false;
static bool g_source_scene_camera = false;
static bool g_source_scene_actor40 = false;
static bool g_source_scene_actor40_only = false;
static bool g_source_loop_replay = false;
static bool g_source_scene_rng_replay = false;
static bool g_source_actor_proxy = false;
static l3::SourceRngService g_actor_rng;
static l3::ChildRuntime g_children;
static l3::SourceGteRtpsState g_source_player_gte;
static uint64_t g_actor_update_ordinal = 0;
// Original owner materials, with diagnostic unlit white modulation. Shade and
// semitransparency parity remain UNKNOWN; independent default-OFF UV gate.
static bool g_actor_textures = false;
static l3::NegativeMaterialTables g_negative_materials;
static size_t g_area_primitive_count = 0;
static int32_t g_area_min_y = 0, g_area_max_y = 0;
static const char* g_content_source = "LOOSE";
static bool g_sidecars_bound = true;
static size_t g_package_bytes = 0;
static size_t g_package_decoded_estimate = 0;
static size_t g_package_peak_estimate = 0;

std::vector<uint8_t> read_binary(const std::string& path, size_t limit=kRuntimeFileLimit) {
    std::unique_ptr<FILE, decltype(&std::fclose)> f(std::fopen(path.c_str(), "rb"), &std::fclose);
    if (!f) throw std::runtime_error("cannot open " + path);
    if (std::fseek(f.get(), 0, SEEK_END) != 0) throw std::runtime_error("cannot seek " + path);
    const long n = std::ftell(f.get());
    if (n < 0 || static_cast<size_t>(n) > std::min(limit,kRuntimeFileLimit) || std::fseek(f.get(), 0, SEEK_SET) != 0)
        throw std::runtime_error("runtime file exceeds admission limit or cannot be sized: " + path);
    std::vector<uint8_t> bytes(static_cast<size_t>(n));
    if (!bytes.empty() && std::fread(bytes.data(), 1, bytes.size(), f.get()) != bytes.size())
        throw std::runtime_error("cannot read " + path);
    return bytes;
}

std::string data_path(const char* rel) { return std::string(kDataRoot) + rel; }

bool section_matches_file(const l3::LoadedAreaPackage& package, uint32_t kind, const std::string& path) {
    const auto section = std::find_if(package.directory.begin(), package.directory.end(),
        [kind](const auto& entry) { return entry.kind == kind; });
    if (section == package.directory.end()) return false;
    // A bounded byte comparison joins this exact pair to the supplied legacy
    // sidecars. Neither CRC nor source_profile_crc alone establishes identity.
    std::unique_ptr<FILE, decltype(&std::fclose)> local(std::fopen(path.c_str(), "rb"), &std::fclose);
    std::unique_ptr<FILE, decltype(&std::fclose)> packed(std::fopen(kAreaPackagePath, "rb"), &std::fclose);
    if (!local || !packed || section->size > kRuntimeFileLimit) return false;
    if (std::fseek(local.get(),0,SEEK_END)!=0 || std::ftell(local.get())!=static_cast<long>(section->size) ||
        std::fseek(local.get(),0,SEEK_SET)!=0 || std::fseek(packed.get(),static_cast<long>(section->offset),SEEK_SET)!=0)
        return false;
    std::array<uint8_t,4096> a{},b{};
    size_t remaining=section->size;
    while (remaining) {
        const size_t n=std::min(remaining,a.size());
        if (std::fread(a.data(),1,n,local.get())!=n || std::fread(b.data(),1,n,packed.get())!=n ||
            std::memcmp(a.data(),b.data(),n)!=0) return false;
        remaining-=n;
    }
    return true;
}

const char* renderer_stage_name(MMLRendererStage s) {
    switch (s) {
        case MML_RENDER_0143_FALLBACK: return "0143 FALLBACK";
        case MML_RENDER_LOGGING_ONLY: return "0143 + LOGGING";
        case MML_RENDER_UV_INPUT: return "UV INPUT";
        case MML_RENDER_BATCHING_TEXTURES_OFF: return "BATCHING / TEX OFF";
        case MML_RENDER_ONE_TERRAIN_TEXTURE: return "ONE TERRAIN TEX";
        case MML_RENDER_ALL_TERRAIN_TEXTURES: return "ALL TERRAIN TEX";
        case MML_RENDER_PLAYER_TEXTURES: return "PLAYER TEX";
        case MML_RENDER_ALPHA: return "ALPHA";
        case MML_RENDER_ANIMATION: return "ANIMATION";
        default: return "UNKNOWN";
    }
}

void reset_boot_log() {
    FILE* f = std::fopen(kBootLogPath, "w");
    if (!f) return;
    std::fprintf(f, "Legends3DS " MML_CHECKPOINT_VERSION " boot log\n");
    std::fclose(f);
}

void boot_log(MMLBootStage stage, const char* label) {
    mml_boot_diag_advance(&g_boot_diag, stage);
    FILE* f = std::fopen(kBootLogPath, "a");
    if (f) {
        std::fprintf(f, "stage=%u label=%s renderer=%u linear_owned=%lu tex_bytes=%lu tex_count=%lu\n",
                     unsigned(stage), label ? label : "",
                     unsigned(g_renderer_gate.active),
                     (unsigned long)g_owned_linear_bytes,
                     (unsigned long)g_owned_texture_bytes,
                     (unsigned long)g_texture_count);
        std::fclose(f);
    }
}

bool use_uv_renderer();

bool retail_actor_animation_requested() {
    return g_retail_actor_animation && use_uv_renderer();
}
// Own the GPU frame before any update may replace GPU-visible resources.
// The five-second budget is a diagnostic timeout, not a retail timing value.
u64 acquire_gpu_frame(const char* owner, bool pace_display=true) {
    if(g_gpu_frame_open)return 0;
    if(g_gpu_wait_failed)throw std::runtime_error("GPU wait already failed; restart required");
    const auto result=l3::wait_for_gpu_frame(u64(5000000)*CPU_TICKS_PER_USEC,pace_display,
        []{return u64(svcGetSystemTick());},
        []{return l3::GpuDisplayCounters{C3D_FrameCounter(0),C3D_FrameCounter(1)};},
        []{return C3D_FrameBegin(C3D_FRAME_NONBLOCK);},
        []{svcSleepThread(1000000);});
    if(result.status!=l3::GpuFrameWaitStatus::Acquired){
        g_gpu_wait_failed=true;
        char label[160];std::snprintf(label,sizeof(label),"%s timeout: %s; queued GPU resources retained",
            result.status==l3::GpuFrameWaitStatus::DisplayClockTimeout?"display clock":"GPU queue",owner);
        boot_log(MML_BOOT_C3D,label);
        throw std::runtime_error(label);
    }
    g_gpu_frame_open=true;
    g_gpu_queue_pending=false;
    return result.elapsed_ticks/CPU_TICKS_PER_USEC;
}
void submit_gpu_frame(bool has_gpu_work=true) {
    if(!g_gpu_frame_open)return;
    C3D_FrameEnd(0);
    g_gpu_frame_open=false;
    g_gpu_queue_pending=has_gpu_work;
}
void require_gpu_idle_for_release() {
    if(g_gpu_queue_pending||g_gpu_wait_failed)
        throw std::runtime_error("refusing to release GPU resources before queue completion");
}

// 0.0.15.0 gameplay profile. Missing gate files use the profile default
// (source-backed combat ON); explicit files still win; malformed files fail
// closed. gameplay_profile.ini SOURCE_COMBAT_DEFAULT=0 restores the historical
// all-OFF defaults for A/B comparison. See include/legends3ds/gameplay_gates.h.
static l3::GateFileState g_gameplay_profile=l3::GateFileState::Absent;
static l3::GameplayGateDefaults g_gameplay_defaults{};
l3::GateFileState read_gate_file(const char* path,const char* key) {
    FILE* file=std::fopen(path,"rb");
    if(!file)return l3::GateFileState::Absent;
    char data[65]{};const size_t n=std::fread(data,1,sizeof(data)-1,file);
    const bool invalid=std::ferror(file)!=0||!std::feof(file);
    std::fclose(file);
    if(invalid)return l3::GateFileState::Malformed;
    return l3::parse_gate_file(data,n,key);
}
bool load_default_gate(const char* path,const char* key,bool default_on) {
    const auto state=read_gate_file(path,key);
    if(state==l3::GateFileState::Malformed){
        char label[160];std::snprintf(label,sizeof(label),"%s file rejected; gate OFF",key);
        boot_log(MML_BOOT_DATA,label);
    }
    return l3::resolve_gate(state,default_on);
}
void load_gameplay_profile() {
    g_gameplay_profile=read_gate_file("sdmc:/3ds/Legends3DS/gameplay_profile.ini",l3::kGameplayProfileKey);
    g_gameplay_defaults=l3::gameplay_gate_defaults(g_gameplay_profile);
    boot_log(MML_BOOT_DATA,g_gameplay_defaults.source_actor_proxy
        ?"GAMEPLAY_PROFILE=SOURCE_COMBAT_DEFAULT actor_state/proxy/textures/rng default ON; candidate projectile retired when active"
        :"GAMEPLAY_PROFILE=HISTORICAL_DEFAULTS all source combat gates default OFF");
}
void load_actor_animation_gate() {
    FILE* file=std::fopen("sdmc:/3ds/Legends3DS/actor_animation_01415.cfg","rb");
    if(file){
        char data[65]{};const size_t n=std::fread(data,1,sizeof(data)-1,file);
        const bool invalid=n==0||std::ferror(file)!=0||!std::feof(file);
        std::fclose(file);
        unsigned enabled=0;char trailing=0;
        g_retail_actor_animation=!invalid&&
            std::sscanf(data,"RETAIL_ACTOR_ANIMATION=%u %c",&enabled,&trailing)==1&&enabled==1;
        if(!g_retail_actor_animation)boot_log(MML_BOOT_DATA,"actor animation diagnostic file rejected");
    }
    char label[240];
    std::snprintf(label,sizeof(label),
        "RETAIL_ACTOR_ANIMATION=%u owner_only=1 request=0 subtype=03 sequence=SOURCE_BACKED_PARTIAL cadence=CANDIDATE pause_ownership=UNKNOWN allocation_first_tick=UNKNOWN",
        unsigned(retail_actor_animation_requested()));
    boot_log(MML_BOOT_DATA,label);
}

bool actor_state_fields_requested() { return g_actor_state_fields && use_uv_renderer(); }
void load_actor_state_gate() {
    g_actor_state_fields=load_default_gate("sdmc:/3ds/Legends3DS/actor_state_01418.cfg",
        "ACTOR_STATE_FIELDS",g_gameplay_defaults.actor_state_fields);
    boot_log(MML_BOOT_DATA,actor_state_fields_requested()
        ? "ACTOR_STATE_FIELDS=1 STATE1_FIELDS_ONLY BIOS_RNG=UNKNOWN outer_consumers=PARTIAL cadence=CANDIDATE full_AI=false"
        : "ACTOR_STATE_FIELDS=0 full_AI=false");
}
void load_actor_rng_candidate_gate() {
    char data[64]{};unsigned enabled=0;char trailing=0;
    g_actor_rng_candidate=load_default_gate("sdmc:/3ds/Legends3DS/actor_rng.ini",
        "ACTOR_RNG_CANDIDATE",g_gameplay_defaults.actor_rng_candidate);
    if(FILE* file=std::fopen("sdmc:/3ds/Legends3DS/source_event.ini","rb")) {
        std::memset(data,0,sizeof(data));enabled=0;
        const auto n=std::fread(data,1,sizeof(data)-1,file);
        const bool invalid=n==0||std::ferror(file)!=0||!std::feof(file);std::fclose(file);
        g_source_event_boot=!invalid&&std::sscanf(data,"SOURCE_EVENT_BOOT=%u %c",&enabled,&trailing)==1&&enabled==1;
    }
    g_source_actor_proxy=load_default_gate("sdmc:/3ds/Legends3DS/source_actor_proxy.ini",
        "SOURCE_ACTOR_PROXY",g_gameplay_defaults.source_actor_proxy);
    if(FILE* file=std::fopen("sdmc:/3ds/Legends3DS/source_scene_e0.ini","rb")) {
        std::memset(data,0,sizeof(data));enabled=0;
        const auto n=std::fread(data,1,sizeof(data)-1,file);
        const bool invalid=n==0||std::ferror(file)!=0||!std::feof(file);std::fclose(file);
        g_source_scene_e0=!invalid&&std::sscanf(data,"SOURCE_SCENE_E0=%u %c",&enabled,&trailing)==1&&enabled==1;
    }
    const auto scene_gate=[&](const char* path,const char* format) {
        char bytes[64]{};unsigned value=0;char extra=0;
        FILE* f=std::fopen(path,"rb");if(!f)return false;
        const auto n=std::fread(bytes,1,sizeof(bytes)-1,f);
        const bool valid=n>0&&!std::ferror(f)&&std::feof(f)&&!std::memchr(bytes,0,n);std::fclose(f);
        return valid&&std::sscanf(bytes,format,&value,&extra)==1&&value==1;
    };
    g_source_scene_textures=scene_gate("sdmc:/3ds/Legends3DS/source_scene_textures.ini","SOURCE_SCENE_TEXTURES=%u %c");
    g_source_scene_camera=scene_gate("sdmc:/3ds/Legends3DS/source_scene_camera.ini","SOURCE_SCENE_CAMERA=%u %c");
    g_source_scene_actor40=scene_gate("sdmc:/3ds/Legends3DS/source_scene_actor40.ini","SOURCE_SCENE_ACTOR40=%u %c");
    g_source_scene_actor40_only=scene_gate("sdmc:/3ds/Legends3DS/source_scene_actor40_only.ini","SOURCE_SCENE_ACTOR40_ONLY=%u %c");
    g_source_loop_replay=scene_gate("sdmc:/3ds/Legends3DS/source_loop_replay.ini","SOURCE_LOOP_REPLAY=%u %c");
    g_source_scene_rng_replay=scene_gate("sdmc:/3ds/Legends3DS/source_scene_rng.ini","SOURCE_SCENE_RNG_REPLAY=%u %c");
    if(g_source_scene_rng_replay)g_source_loop_replay=false;
    boot_log(MML_BOOT_DATA,g_source_scene_rng_replay?"SOURCE_SCENE_RNG_REPLAY=1 recorded_OpenBIOS_context_required":"SOURCE_SCENE_RNG_REPLAY=0");
    boot_log(MML_BOOT_DATA,g_source_loop_replay?"SOURCE_LOOP_REPLAY=1 recorded_OpenBIOS_boundary_only retail_chronology=false":"SOURCE_LOOP_REPLAY=0 scratch_owner_unknown");
    boot_log(MML_BOOT_DATA,g_source_scene_textures?"SOURCE_SCENE_TEXTURES=1 initial_batch_only live_palette=UNKNOWN":"SOURCE_SCENE_TEXTURES=0");
    boot_log(MML_BOOT_DATA,g_source_scene_camera?"SOURCE_SCENE_CAMERA=1 radial_subset clipping=CANDIDATE":"SOURCE_SCENE_CAMERA=0");
    boot_log(MML_BOOT_DATA,g_source_scene_e0
        ?"SOURCE_SCENE_E0=1 bounded_original_updates chronology=CANDIDATE Near_LOD=FRONTEND_DIAGNOSTIC texture=UNKNOWN"
        :"SOURCE_SCENE_E0=0 scripted E0 native diagnostic disabled");
    boot_log(MML_BOOT_DATA,g_source_actor_proxy
        ?"SOURCE_ACTOR_PROXY=1 explicit_fresh_pool_shift_subset Buster_player_GTE=UNKNOWN"
        :"SOURCE_ACTOR_PROXY=0 source pose/proxy owner admission held");
    boot_log(MML_BOOT_DATA,g_source_event_boot
        ?"SOURCE_EVENT_BOOT=1 fresh_ST00_subset_only full_NewGame_fade_cutscene=UNKNOWN"
        :"SOURCE_EVENT_BOOT=0 source owner inputs remain held");
    boot_log(MML_BOOT_DATA,g_actor_rng_candidate
        ?"ACTOR_RNG_CANDIDATE=1 SHARED_STREAM=class03_effect234_prop_scene source_algorithm=SELECTED_OPENBIOS full_chronology=UNKNOWN held_callback=ROLLBACK retail_parity=false"
        :"ACTOR_RNG_CANDIDATE=0 original RNG ownership remains held");
}
void prepare_actor_animation_banks(l3::ActorAreaRuntime& actors,const l3::EbdDirectory& ebd) {
    if(actor_state_fields_requested()) {
        constexpr std::array<uint8_t,8> requests{{0,1,2,3,4,6,21,22}};
        try {
            l3::bind_imported_actor_animation_banks(actors,ebd,requests.data(),requests.size());
            char label[192];std::snprintf(label,sizeof(label),
                "actor_request_banks unique=%lu decoded_cache_elements=%lu decoded_total_elements=%lu cap=524288 allocator_overhead=EXCLUDED cpu_gpu_total=UNKNOWN",
                static_cast<unsigned long>(actors.unique_pose_cache_count),
                static_cast<unsigned long>(actors.decoded_pose_cache_bytes),
                static_cast<unsigned long>(actors.decoded_resource_bytes));
            boot_log(MML_BOOT_DATA,label);
        } catch(const std::exception& error) {boot_log(MML_BOOT_DATA,error.what());}
    } else if(retail_actor_animation_requested()) {
        l3::bind_imported_actor_request0_caches(actors,ebd);
    }
}

bool actor_textures_requested() { return g_actor_textures&&use_uv_renderer(); }
void load_actor_texture_gate() {
    g_actor_textures=load_default_gate("sdmc:/3ds/Legends3DS/actor_render_01416.cfg",
        "ACTOR_TEXTURES",g_gameplay_defaults.actor_textures);
    char label[200];std::snprintf(label,sizeof(label),
        "ACTOR_TEXTURES=%u owner_only=1 material=SOURCE_BACKED_PARTIAL shade=UNKNOWN modulation=WHITE_UNLIT_DIAGNOSTIC alpha=UNKNOWN",
        unsigned(actor_textures_requested()));
    boot_log(MML_BOOT_DATA,label);
}

void load_view_calibration() {
    bool candidate_override=false;
    const char* path="sdmc:/3ds/Legends3DS/view_01412.cfg";
    FILE* f=std::fopen(path,"rb");
    if(f) {
        char data[1025];const size_t n=std::fread(data,1,sizeof(data),f);
        const bool failed=std::ferror(f)!=0;std::fclose(f);
        try {
            if(failed||n>1024)throw std::runtime_error("view calibration file invalid");
            g_view_settings=l3::parse_frontend_view_config(std::string(data,n));
            candidate_override=true;
            boot_log(MML_BOOT_GFX,"owner-local candidate view calibration accepted");
        } catch(const std::exception& error) {
            g_view_settings={};boot_log(MML_BOOT_GFX,error.what());
        }
    }
    if(!use_uv_renderer())g_view_settings=l3::legacy_frontend_view();
    FILE* log=std::fopen(kBootLogPath,"a");
    if(log) {
        const bool retail_profile=use_uv_renderer()&&!candidate_override;
        std::fprintf(log,"view_evidence=%s view_profile=%s fov_vertical_deg=%.3f follow=%.3f eye_height=%.3f target_height=%.3f world_scale=%.9f aspect=%.9f viewport=400x240 near=%.3f far=%.3f player_scale_q12=%u projection_reference_h=%u projectile_presentation=CANDIDATE_PROXY_TRIANGLE top_ps1_hud_binding=UNKNOWN\n",
            retail_profile?"SOURCE_BACKED_PARTIAL":"CANDIDATE",
            use_uv_renderer()?(candidate_override?"owner_local_override":"retail_baseline"):"legacy_0143",
            g_view_settings.vertical_fov_degrees,g_view_settings.follow_distance,
            g_view_settings.eye_height,g_view_settings.target_height,kWorldScale,
            l3::kFrontendAspect,l3::kFrontendNear,l3::kFrontendFar,
            use_uv_renderer()?512u:4096u,retail_profile?384u:0u);
        std::fclose(log);
    }
}

void boot_fail(uint32_t code, const char* label) {
    mml_boot_diag_fail(&g_boot_diag, code);
    FILE* f = std::fopen(kBootLogPath, "a");
    if (f) {
        std::fprintf(f, "FAILED code=0x%08lx label=%s last_stage=%u renderer=%u\n",
                     (unsigned long)code, label ? label : "",
                     unsigned(g_boot_diag.stage), unsigned(g_renderer_gate.active));
        std::fclose(f);
    }
}

MMLRendererStage choose_boot_stage(u32 held) {
    // One-shot boot bisect controls. The default is the complete enhanced path;
    // SELECT always forces the true 0.0.14.3 renderer path.
    if (held & KEY_SELECT) return MML_RENDER_0143_FALLBACK;
    if (held & KEY_L) return MML_RENDER_LOGGING_ONLY;
    if (held & KEY_R) return MML_RENDER_UV_INPUT;
    if (held & KEY_DLEFT) return MML_RENDER_BATCHING_TEXTURES_OFF;
    if (held & KEY_DUP) return MML_RENDER_ONE_TERRAIN_TEXTURE;
    if (held & KEY_DRIGHT) return MML_RENDER_ALL_TERRAIN_TEXTURES;
    if (held & KEY_X) return MML_RENDER_PLAYER_TEXTURES;
    if (held & KEY_Y) return MML_RENDER_ALPHA;
    return MML_RENDER_ANIMATION;
}

bool use_uv_renderer() {
    return g_renderer_gate.uv_enabled;
}

bool terrain_xyz_active() {
    return use_uv_renderer() && g_terrain_xyz_candidate;
}

bool use_material_batches() {
    return g_renderer_gate.batching_enabled;
}

bool terrain_textures_requested() {
    return g_renderer_gate.textures_enabled;
}

bool all_terrain_textures_requested() {
    return (g_renderer_gate.feature_mask & MML_RF_ALL_TERRAIN) != 0;
}

bool player_textures_requested() {
    return (g_renderer_gate.feature_mask & MML_RF_PLAYER) != 0;
}

bool alpha_requested() {
    return g_renderer_gate.alpha_enabled;
}

bool retail_animation_requested() {
    return g_renderer_gate.animation_enabled;
}

ProbeVertex0143 world_vertex_0143(float x, float y, float z) {
    return {x * kWorldScale, -y * kWorldScale, z * kWorldScale};
}

ProbeVertexUV world_vertex_uv(float x, float y, float z, float u=0.0f, float v=0.0f) {
    return {x * kWorldScale, -y * kWorldScale, z * kWorldScale, u, v};
}

l3::RuntimeVec3i first_positive_cell_center(const l3::RuntimeMap& map, size_t area_index) {
    if (area_index >= map.areas.size()) throw std::runtime_error("area index outside map");
    const auto& a = map.areas[area_index];
    for (uint32_t local = 0; local < a.cell_count; ++local) {
        const uint32_t ci = a.cell_first + local;
        if (ci >= map.cells.size()) break;
        const int16_t packed = map.cells[ci];
        if (packed <= 0) continue;
        const size_t ii = static_cast<uint16_t>(packed);
        if (ii >= map.idx.size()) continue;
        const uint32_t lx = local % a.width, lz = local / a.width;
        const int32_t gx = int32_t(a.start_x) + int32_t(lx);
        const int32_t gz = int32_t(a.start_z) + int32_t(lz);
        // Render/contact origins are tile centers; adding 0x100 spawned on the seam.
        return {(gx * 0x200) - 0x7F00, map.idx[ii].y, (gz * 0x200) - 0x7F00};
    }
    return {a.world_origin_x, 0, a.world_origin_z};
}

void release_gpu_texture(C3D_Tex& tex, bool& ready) {
    if (ready) {
        C3D_TexDelete(&tex);
        if (g_owned_texture_bytes >= kTexturePageBytes) g_owned_texture_bytes -= kTexturePageBytes;
        if (g_texture_count) --g_texture_count;
    }
    ready = false;
}

void release_terrain_0143() {
    if (g_terrain_vbo_0143) {
        const uint32_t bytes = uint32_t(g_terrain_vertex_count_0143 * sizeof(ProbeVertex0143));
        linearFree(g_terrain_vbo_0143);
        if (g_owned_linear_bytes >= bytes) g_owned_linear_bytes -= bytes;
    }
    g_terrain_vbo_0143 = nullptr;
    g_terrain_vertex_count_0143 = 0;
}

void release_terrain_uv_flat() {
    if (g_terrain_vbo_uv) {
        const uint32_t bytes = uint32_t(g_terrain_vertex_count_uv * sizeof(ProbeVertexUV));
        linearFree(g_terrain_vbo_uv);
        if (g_owned_linear_bytes >= bytes) g_owned_linear_bytes -= bytes;
    }
    g_terrain_vbo_uv = nullptr;
    g_terrain_vertex_count_uv = 0;
}

void release_terrain_batches() {
    for (auto& b : g_terrain_batches) {
        if (b.vbo) {
            const uint32_t bytes = uint32_t(b.vertex_count * sizeof(ProbeVertexUV));
            linearFree(b.vbo);
            if (g_owned_linear_bytes >= bytes) g_owned_linear_bytes -= bytes;
        }
        b.vbo = nullptr;
        if (b.indices) {
            const uint32_t bytes = uint32_t(b.quads.size() * 12u * sizeof(uint16_t));
            linearFree(b.indices);
            if (g_owned_linear_bytes >= bytes) g_owned_linear_bytes -= bytes;
            b.indices = nullptr;
        }
        release_gpu_texture(b.texture, b.texture_ready);
    }
    std::vector<TerrainBatch>().swap(g_terrain_batches);
}

void release_terrain_overlay() {
    require_gpu_idle_for_release();
    for (auto& o : g_terrain_overlay) {
        if (!o.vbo) continue;
        const uint32_t bytes = uint32_t(o.vertex_count * o.stride);
        linearFree(o.vbo);
        if (g_owned_linear_bytes >= bytes) g_owned_linear_bytes -= bytes;
    }
    std::vector<TerrainOverlay>().swap(g_terrain_overlay);
}

void release_player_materials() {
    require_gpu_idle_for_release();
    for (auto& m : g_player_materials) release_gpu_texture(m.texture, m.texture_ready);
    std::vector<GpuMaterial>().swap(g_player_materials);
}

void release_actor_materials() {
    require_gpu_idle_for_release();
    for(auto& m:g_actor_materials)release_gpu_texture(m.texture,m.texture_ready);
    std::vector<GpuMaterial>().swap(g_actor_materials);
}

void release_scene_materials() {
    require_gpu_idle_for_release();
    for(auto& m:g_scene_materials)release_gpu_texture(m.texture,m.texture_ready);
    std::vector<GpuMaterial>().swap(g_scene_materials);
}
void release_scene_actor40_materials() {
    require_gpu_idle_for_release();
    for(auto& m:g_scene_actor40_materials)release_gpu_texture(m.texture,m.texture_ready);
    std::vector<GpuMaterial>().swap(g_scene_actor40_materials);
}

void release_all_render_resources() {
    require_gpu_idle_for_release();
    release_terrain_overlay();
    g_terrain_patch.reset();
    g_terrain_patch_generation = UINT64_MAX;
    release_terrain_0143();
    release_terrain_uv_flat();
    release_terrain_batches();
    release_player_materials();
    release_actor_materials();
    release_scene_materials();
    release_scene_actor40_materials();
}

void push_quad_0143(std::vector<ProbeVertex0143>& dst, const l3::ExpandedRenderPrimitive& p) {
    const unsigned order[6] = {0,1,2,0,2,3};
    for (unsigned oi : order)
        dst.push_back(world_vertex_0143(float(p.world_x[oi]), float(p.world_y[oi]), float(p.world_z[oi])));
}

void push_quad_uv(std::vector<ProbeVertexUV>& dst, const l3::ExpandedRenderPrimitive& p) {
    // Full Positive IDX XYZ/strip decoding is separately selectable. It keeps
    // each UV associated with its vertex; legacy UV and0143 retain their fan.
    const std::array<uint8_t,6> legacy{{0,1,2,0,2,3}};
    const auto& order = terrain_xyz_active() ? l3::kTerrainCandidateStripIndices : legacy;
    for (unsigned oi : order) {
        const float u = (float(p.uv[oi * 2u]) + 0.5f) / 256.0f;
        // tex3ds uses bottom-origin V for top-to-bottom source image rows.
        const float v = 1.0f - (float(p.uv[oi * 2u + 1u]) + 0.5f) / 256.0f;
        dst.push_back(world_vertex_uv(float(p.world_x[oi]), float(p.world_y[oi]), float(p.world_z[oi]), u, v));
    }
}

uint64_t validate_area_vertex_budget(const l3::RuntimeMap& map,
                                const l3::St00RenderSource& render,size_t area_index) {
    const uint64_t max_primitives=kTerrainBufferLimit/sizeof(ProbeVertexUV)/6u;
    return terrain_xyz_active()
        ? l3::count_area_combined_mid_primitives(map,render,area_index,max_primitives)
        : l3::count_area_render_primitives(map,render,area_index,max_primitives);
}

size_t next_diagnostic_area(const l3::RuntimeMap& map,const l3::St00RenderSource& render,size_t start) {
    const uint64_t cap=kTerrainBufferLimit/sizeof(ProbeVertexUV)/6u;
    if (terrain_xyz_active()) {
        const auto next=l3::next_area_with_mid_terrain(map,render,start,cap);
        if (next) return *next;
    } else if (!map.areas.empty() && start<map.areas.size()) {
        for (size_t offset=0;offset<map.areas.size();++offset) {
            const size_t area=(start+offset)%map.areas.size();
            if (validate_area_vertex_budget(map,render,area)) return area;
        }
    }
    throw std::runtime_error("package has no renderable diagnostic area");
}

bool swappable_descriptor(const l3::RuntimeMap& map, uint16_t descriptor) {
    return descriptor < map.descriptors.size() && (map.descriptors[descriptor].word0 & 0x1000u);
}
// IDX cells an E0/07 gate rewrites (0x80115B64/0x80115CA4); tracked by the
// terrain patch like door cells even though the closed word0 0x40500 has no
// 0x1000 pair.
static std::vector<uint16_t> g_extra_swappable_idx;
void set_e007_gate_cells(const l3::TutorialRuntimeArea& area, const l3::RuntimeMap& map, size_t area_index) {
    g_extra_swappable_idx.clear();
    for (const auto& instance : area.stage.instances) {
        const auto& o = instance.object;
        if (o.source_class != 0xE0 || o.subtype != 7 || o.source_param3 != 0) continue;
        if (const auto gate = l3::source_e007_init_80115a88(map, area_index, o.source_position, 0, false))
            g_extra_swappable_idx.push_back(gate->idx_cell);
    }
    // E0/08 doors with parameter & 0x4F own a cell too (0x80116750).
    for (const auto& instance : area.stage.instances) {
        const auto& o = instance.object;
        if (o.source_class != 0xE0 || o.subtype != 8 || o.source_param3 < 0x80) continue;
        if (const auto door = l3::source_e008_init_80115eac(map, area_index, o.source_position, o.source_param3, false, false))
            if (door->idx_cell) g_extra_swappable_idx.push_back(*door->idx_cell);
    }
}

template <typename Fn>
void for_each_area_primitive(const l3::RuntimeMap& map,
                             const l3::St00RenderSource& render,
                             size_t area_index,
                             Fn&& fn) {
    if (area_index >= map.areas.size()) throw std::runtime_error("terrain area outside map");
    const auto& area = map.areas[area_index];
    g_area_primitive_count=0;g_area_min_y=g_area_max_y=0;
    g_area_primitive_negative=false;
    if (terrain_xyz_active()) {
        // Positive grid entries are visibility aliases, not packed rotations.
        // ROCK_NEO 0x80021834 marks IDX bits; 0x80024bac draws each once using
        // authored IDX XYZ and raw6 bank selection. The legacy branch below
        // remains the independently selectable 0143/UV fallback.
        const auto placement = l3::collect_area_positive_placement(map, area_index);
        for (const auto& instance : placement.instances) {
            if (g_area_idx_filter && std::find(g_area_idx_filter->begin(), g_area_idx_filter->end(),
                                               instance.idx_index) == g_area_idx_filter->end()) continue;
            const bool tracked = swappable_descriptor(map, instance.descriptor_index) ||
                std::find(g_extra_swappable_idx.begin(), g_extra_swappable_idx.end(), instance.idx_index) != g_extra_swappable_idx.end();
            if (g_terrain_patch_recording && tracked) g_terrain_patch.bake(instance.idx_index, instance.descriptor_index);
            g_area_primitive_idx = tracked ? instance.idx_index : uint16_t(0xFFFFu);
            if (instance.descriptor_index == 0xFFFFu) continue;
            const auto prims = l3::expand_positive_terrain_descriptor_lod(render,
                instance.descriptor_index, instance.bank, l3::St00Lod::Mid,
                instance.origin, 0);
            for (const auto& primitive : prims) {
                if (!g_area_primitive_count) g_area_min_y=g_area_max_y=primitive.world_y[0];
                for (int32_t y : primitive.world_y) {
                    g_area_min_y=std::min(g_area_min_y,y);
                    g_area_max_y=std::max(g_area_max_y,y);
                }
                fn(primitive);
                ++g_area_primitive_count;
            }
        }
        g_area_primitive_idx = 0xFFFFu;
        // Negative cells carry no IDX descriptor; a swap overlay skips them.
        if (g_area_idx_filter) return;
        g_area_primitive_negative=true;
        const auto negative = l3::collect_area_negative_placement(map, render, area_index);
        uint32_t unknown_materials = 0;
        for (const auto& instance : negative.instances) {
            const auto* binding = l3::find_negative_material_binding(g_negative_materials, area_index, instance.packed_cell);
            if (!binding) ++unknown_materials;
            for (const auto& primitive : l3::expand_negative_terrain_mid(render, instance, binding)) {
                if (!g_area_primitive_count) g_area_min_y=g_area_max_y=primitive.world_y[0];
                for (int32_t y : primitive.world_y) { g_area_min_y=std::min(g_area_min_y,y); g_area_max_y=std::max(g_area_max_y,y); }
                fn(primitive); ++g_area_primitive_count;
            }
        }
        hardware_test_line("NEG bound:%lu omitted:%lu mat?:%lu", (unsigned long)negative.instances.size(),
            (unsigned long)(negative.default_cells_unbound+negative.special_cells_unbound), (unsigned long)unknown_materials);
        hardware_test_line("IDX unique:%lu aliases:%lu special:%lu",
            static_cast<unsigned long>(placement.instances.size()),
            static_cast<unsigned long>(placement.duplicate_occurrences),
            static_cast<unsigned long>(placement.negative_cells));
        return;
    }
    for (uint32_t local = 0; local < area.cell_count; ++local) {
        const uint32_t ci = area.cell_first + local;
        if (ci >= map.cells.size()) throw std::runtime_error("terrain cell range escaped map");
        const int16_t packed = map.cells[ci];
        if (packed <= 0) continue;
        const size_t ii = static_cast<uint16_t>(packed);
        if (ii >= map.idx.size()) throw std::runtime_error("terrain IDX escaped map");
        const auto& idx = map.idx[ii];
        if (g_area_idx_filter && std::find(g_area_idx_filter->begin(), g_area_idx_filter->end(),
                                           uint16_t(ii)) == g_area_idx_filter->end()) continue;
        const bool tracked = swappable_descriptor(map, idx.descriptor_index);
        if (g_terrain_patch_recording && tracked) g_terrain_patch.bake(uint16_t(ii), idx.descriptor_index);
        g_area_primitive_idx = tracked ? uint16_t(ii) : uint16_t(0xFFFFu);
        if (idx.descriptor_index == 0xFFFFu || idx.descriptor_index >= render.descriptors.size()) continue;
        const uint32_t lx = local % area.width, lz = local / area.width;
        const int32_t gx = int32_t(area.start_x) + int32_t(lx);
        const int32_t gz = int32_t(area.start_z) + int32_t(lz);
        const l3::St00CellOrigin origin{(gx * 0x200) - 0x7F00, idx.y, (gz * 0x200) - 0x7F00};
        const auto prims = l3::expand_st00_descriptor_lod(render, idx.descriptor_index,
            l3::St00RenderBank::Primary, l3::St00Lod::Mid, origin,
            static_cast<uint16_t>(packed), 0);
        for (const auto& p : prims) {
            if (!g_area_primitive_count) g_area_min_y=g_area_max_y=p.world_y[0];
            for (int32_t y : p.world_y) {
                g_area_min_y=std::min(g_area_min_y,y);
                g_area_max_y=std::max(g_area_max_y,y);
            }
            fn(p);
            ++g_area_primitive_count;
        }
    }
    g_area_primitive_idx = 0xFFFFu;
}

l3::AreaInspectionEntry inspection_entry(const l3::RuntimeMap& map, const l3::St00RenderSource& render, size_t area) {
    validate_area_vertex_budget(map,render,area);
    l3::AreaInspectionGeometry geometry(kTerrainBufferLimit/sizeof(ProbeVertexUV)/6u);
    for_each_area_primitive(map,render,area,[&](const l3::ExpandedRenderPrimitive& p){geometry.add(p);});
    const auto result=l3::choose_area_inspection_entry(map,area,geometry,g_view_settings);
    hardware_test_line("VIEW CANDIDATE seen:%lu/%lu temp:%lu", (unsigned long)result.primitives_with_visible_vertex,
        (unsigned long)result.primitive_count,(unsigned long)geometry.storage_bytes());
    const auto negative=l3::collect_area_negative_placement(map,render,area);
    size_t unknown_materials=0;
    for (const auto& instance:negative.instances)
        if (!l3::find_negative_material_binding(g_negative_materials,area,instance.packed_cell)) ++unknown_materials;
    hardware_test_visibility_event(area,negative.instances.size(),negative.default_cells_unbound,
        negative.special_cells_unbound,unknown_materials,result.primitives_with_visible_vertex,
        geometry.storage_bytes(),l3::area_inspection_candidate_name(result.candidate_source));
    geometry.release();
    if (!result.valid || !result.primitives_with_visible_vertex) throw std::runtime_error("no diagnostic entry admits terrain");
    return result;
}

void rebuild_terrain_0143(const l3::RuntimeMap& map, const l3::St00RenderSource& render, size_t area_index) {
    release_all_render_resources();
    std::vector<ProbeVertex0143> cpu;
    g_terrain_patch_recording = true;
    for_each_area_primitive(map, render, area_index, [&](const l3::ExpandedRenderPrimitive& p) {
        const size_t first = cpu.size();
        push_quad_0143(cpu, p);
        if (g_area_primitive_idx != 0xFFFFu)
            g_terrain_patch.add_span(g_area_primitive_idx, 0, uint32_t(first), uint32_t(cpu.size() - first));
    });
    g_terrain_patch_recording = false;
    if (cpu.empty()) throw std::runtime_error("selected area produced no terrain vertices");
    const size_t bytes = cpu.size() * sizeof(ProbeVertex0143);
    g_terrain_vbo_0143 = static_cast<ProbeVertex0143*>(linearAlloc(bytes));
    if (!g_terrain_vbo_0143) throw std::runtime_error("linearAlloc failed for 0143 terrain VBO");
    std::memcpy(g_terrain_vbo_0143, cpu.data(), bytes);
    g_terrain_vertex_count_0143 = cpu.size();
    g_owned_linear_bytes += uint32_t(bytes);
}

void rebuild_terrain_uv_flat(const l3::RuntimeMap& map, const l3::St00RenderSource& render, size_t area_index) {
    release_all_render_resources();
    std::vector<ProbeVertexUV> cpu;
    g_terrain_patch_recording = true;
    for_each_area_primitive(map, render, area_index, [&](const l3::ExpandedRenderPrimitive& p) {
        const size_t first = cpu.size();
        push_quad_uv(cpu, p);
        if (g_area_primitive_idx != 0xFFFFu)
            g_terrain_patch.add_span(g_area_primitive_idx, 0, uint32_t(first), uint32_t(cpu.size() - first));
    });
    g_terrain_patch_recording = false;
    if (cpu.empty()) throw std::runtime_error("selected area produced no UV terrain vertices");
    const size_t bytes = cpu.size() * sizeof(ProbeVertexUV);
    g_terrain_vbo_uv = static_cast<ProbeVertexUV*>(linearAlloc(bytes));
    if (!g_terrain_vbo_uv) throw std::runtime_error("linearAlloc failed for UV terrain VBO");
    std::memcpy(g_terrain_vbo_uv, cpu.data(), bytes);
    g_terrain_vertex_count_uv = cpu.size();
    g_owned_linear_bytes += uint32_t(bytes);
}

bool upload_material_texture(const l3::MaterialCacheEntry* e, C3D_Tex& tex) {
    if (!e || !e->resolved) return false;
    if (g_owned_texture_bytes > kAreaTextureLimit - kTexturePageBytes)
        throw std::runtime_error("area combined terrain/player GPU texture policy exceeded");
    const auto rgba = l3::material_page_rgba8(*e);
    if (rgba.size() != size_t(kTexturePageBytes))
        throw std::runtime_error("material RGBA page size mismatch");
    const auto gpu = l3::ctr_rgba8_tiled_abgr(rgba,
        l3::kL3MaterialTextureWidth, l3::kL3MaterialTextureHeight);
    if (!C3D_TexInit(&tex, l3::kL3MaterialTextureWidth, l3::kL3MaterialTextureHeight, GPU_RGBA8))
        throw std::runtime_error("C3D_TexInit failed for material page");
    C3D_TexSetFilter(&tex, GPU_NEAREST, GPU_NEAREST);
    C3D_TexUpload(&tex, gpu.data());
    C3D_TexFlush(&tex);
    g_owned_texture_bytes += kTexturePageBytes;
    ++g_texture_count;
    return true;
}

void rebuild_terrain_batches(const l3::RuntimeMap& map,
                             const l3::St00RenderSource& render,
                             const l3::MaterialCache* cache,
                             size_t area_index) {
    release_all_render_resources();
    std::map<uint64_t, std::vector<ProbeVertexUV>> cpu;
    std::map<uint64_t, std::vector<TerrainQuadInfo>> quads;
    g_terrain_patch_recording = true;
    for_each_area_primitive(map, render, area_index, [&](const l3::ExpandedRenderPrimitive& p) {
        const uint64_t key = uint64_t(p.material_word) | (p.texture_binding_known ? 0u : (uint64_t{1} << 32));
        auto& dst = cpu[key];
        const size_t first = dst.size();
        push_quad_uv(dst, p);
        if (terrain_xyz_active()) quads[key].push_back({p.world_x, p.world_y, p.world_z, p.prefix, g_area_primitive_negative});
        if (g_area_primitive_idx != 0xFFFFu)
            g_terrain_patch.add_span(g_area_primitive_idx, key, uint32_t(first), uint32_t(dst.size() - first));
    });
    g_terrain_patch_recording = false;
    if (cpu.empty()) throw std::runtime_error("selected area produced no material batches");

    bool uploaded_one = false;
    g_terrain_batches.reserve(cpu.size());
    for (auto& kv : cpu) {
        TerrainBatch batch;
        batch.material_word = uint32_t(kv.first);
        batch.key = kv.first;
        batch.vertex_count = kv.second.size();
        const size_t bytes = batch.vertex_count * sizeof(ProbeVertexUV);
        batch.vbo = static_cast<ProbeVertexUV*>(linearAlloc(bytes));
        if (!batch.vbo) throw std::runtime_error("linearAlloc failed for terrain batch VBO");
        std::memcpy(batch.vbo, kv.second.data(), bytes);
        g_owned_linear_bytes += uint32_t(bytes);

        // Register ownership before a fallible texture upload so error cleanup
        // also releases the just-created vertex buffer.
        g_terrain_batches.push_back(batch);
        auto& owned_batch = g_terrain_batches.back();
        // Face-test index lists need the original quad order (6 vertices each)
        // and 16-bit indices.
        auto q = quads.find(kv.first);
        if (q != quads.end() && q->second.size() * 6u == owned_batch.vertex_count && owned_batch.vertex_count <= 65535u) {
            const size_t index_bytes = q->second.size() * 12u * sizeof(uint16_t);
            owned_batch.indices = static_cast<uint16_t*>(linearAlloc(index_bytes));
            if (owned_batch.indices) {
                owned_batch.quads = std::move(q->second);
                g_owned_linear_bytes += uint32_t(index_bytes);
            }
        }
        if (terrain_textures_requested() && cache && !(kv.first >> 32)) {
            const auto* e = l3::find_material_cache_entry(*cache, batch.material_word);
            const bool may_upload = all_terrain_textures_requested() || !uploaded_one;
            if (may_upload && e && e->resolved) {
                owned_batch.texture_ready = upload_material_texture(e, owned_batch.texture);
                uploaded_one = uploaded_one || owned_batch.texture_ready;
            }
        }
    }
}

template <typename Vertex>
void add_terrain_overlay(uint64_t key, const std::vector<Vertex>& cpu, const C3D_Tex* texture) {
    if (cpu.empty()) return;
    const size_t bytes = cpu.size() * sizeof(Vertex);
    TerrainOverlay o;
    o.key = key; o.vertex_count = cpu.size(); o.stride = sizeof(Vertex); o.texture = texture;
    o.vbo = linearAlloc(bytes);
    if (!o.vbo) throw std::runtime_error("linearAlloc failed for terrain swap overlay");
    std::memcpy(o.vbo, cpu.data(), bytes);
    GSPGPU_FlushDataCache(o.vbo, bytes);
    g_owned_linear_bytes += uint32_t(bytes);
    g_terrain_overlay.push_back(o);
}

// Runs with the GPU frame owned (the previous frame has completed), so the
// baked buffers and the outgoing overlay are no longer being read.
void patch_terrain_descriptors(const l3::RuntimeMap& live, const l3::St00RenderSource& render, size_t area_index) {
    const auto plan = g_terrain_patch.plan(live);
    if (plan.empty() && plan.overlay.empty() && g_terrain_overlay.empty()) return;
    const size_t stride = use_uv_renderer() ? sizeof(ProbeVertexUV) : sizeof(ProbeVertex0143);
    const size_t touched = g_terrain_patch.apply(plan, stride,
        [](uint64_t key) -> uint8_t* {
            if (!use_uv_renderer()) return reinterpret_cast<uint8_t*>(g_terrain_vbo_0143);
            if (!use_material_batches()) return reinterpret_cast<uint8_t*>(g_terrain_vbo_uv);
            for (auto& b : g_terrain_batches) if (b.key == key) return reinterpret_cast<uint8_t*>(b.vbo);
            return nullptr;
        },
        [](uint8_t* p, size_t n) { GSPGPU_FlushDataCache(p, n); });
    release_terrain_overlay();
    // The overlay pass must not disturb the area's diagnostic geometry totals.
    const uint32_t saved_count = g_area_primitive_count;
    const int32_t saved_min_y = g_area_min_y, saved_max_y = g_area_max_y;
    g_area_idx_filter = &plan.overlay;
    try {
        if (plan.overlay.empty()) {
        } else if (!use_uv_renderer()) {
            std::vector<ProbeVertex0143> cpu;
            for_each_area_primitive(live, render, area_index, [&](const l3::ExpandedRenderPrimitive& p) { push_quad_0143(cpu, p); });
            add_terrain_overlay(0, cpu, nullptr);
        } else if (!use_material_batches()) {
            std::vector<ProbeVertexUV> cpu;
            for_each_area_primitive(live, render, area_index, [&](const l3::ExpandedRenderPrimitive& p) { push_quad_uv(cpu, p); });
            add_terrain_overlay(0, cpu, nullptr);
        } else {
            std::map<uint64_t, std::vector<ProbeVertexUV>> cpu;
            for_each_area_primitive(live, render, area_index, [&](const l3::ExpandedRenderPrimitive& p) {
                push_quad_uv(cpu[uint64_t(p.material_word) | (p.texture_binding_known ? 0u : (uint64_t{1} << 32))], p);
            });
            for (const auto& kv : cpu) {
                const C3D_Tex* texture = nullptr;
                for (const auto& b : g_terrain_batches)
                    if (b.key == kv.first && b.texture_ready) { texture = &b.texture; break; }
                add_terrain_overlay(kv.first, kv.second, texture);
            }
        }
    } catch (...) {
        g_area_idx_filter = nullptr;
        throw;
    }
    g_area_idx_filter = nullptr;
    g_area_primitive_count = saved_count; g_area_min_y = saved_min_y; g_area_max_y = saved_max_y;
    size_t overlay_vertices = 0;
    for (const auto& o : g_terrain_overlay) overlay_vertices += o.vertex_count;
    hardware_test_line("IDX swap patch hide:%lu restore:%lu spans:%lu overlay_idx:%lu overlay_verts:%lu saved:%lu",
        (unsigned long)plan.hide.size(), (unsigned long)plan.restore.size(), (unsigned long)touched,
        (unsigned long)plan.overlay.size(), (unsigned long)overlay_vertices,
        (unsigned long)g_terrain_patch.saved_bytes());
}

void rebuild_player_materials(const l3::MaterialCache* cache) {
    release_player_materials();
    if (!player_textures_requested() || !cache) return;
    g_player_materials.reserve(cache->entries.size());
    for (const auto& e : cache->entries) {
        GpuMaterial m;
        m.material_word = e.material_word;
        g_player_materials.push_back(m);
        auto& owned_material=g_player_materials.back();
        owned_material.texture_ready=upload_material_texture(&e,owned_material.texture);
    }
}

const GpuMaterial* find_player_gpu_material(uint32_t word) {
    for (const auto& m : g_player_materials)
        if (m.material_word == word) return &m;
    return nullptr;
}

const l3::PbdAnimationAction& choose_probe_action(const l3::PbdAnimation& animation) {
    for (const auto& a : animation.actions)
        if (a.action_index == 1 && a.m_is_frame_table && !a.angle_frames.empty()) return a;
    for (const auto& a : animation.actions)
        if (a.m_is_frame_table && !a.angle_frames.empty()) return a;
    throw std::runtime_error("main animation contains no frame-table action");
}

const std::array<uint16_t, l3::kPbdPackedAngleChannels>& fallback_probe_frame(
    const l3::PbdAnimationAction& action, const l3::RuntimeSession& session) {
    const size_t fi = (session.frame_counter() / 4u) % action.angle_frames.size();
    return action.angle_frames[fi];
}

void configure_terrain_attributes_0143(const ProbeVertex0143* vbo = g_terrain_vbo_0143) {
    C3D_AttrInfo* ai = C3D_GetAttrInfo();
    AttrInfo_Init(ai);
    AttrInfo_AddLoader(ai, 0, GPU_FLOAT, 3);
    AttrInfo_AddFixed(ai, 1);
    C3D_FixedAttribSet(1, 0.46f, 0.53f, 0.58f, 1.0f);
    C3D_BufInfo* bi = C3D_GetBufInfo();
    BufInfo_Init(bi);
    if (vbo)
        BufInfo_Add(bi, vbo, sizeof(ProbeVertex0143), 1, 0x0);
}

void configure_terrain_attributes_uv(const ProbeVertexUV* vbo) {
    C3D_AttrInfo* ai = C3D_GetAttrInfo();
    AttrInfo_Init(ai);
    // Keep fetched slots contiguous: slot0 -> shader v0 (position),
    // slot1 -> v2 (UV), slot2 -> v1 (fixed colour). FixedAttribSet uses
    // the attribute slot, not the destination shader-register index.
    AttrInfo_AddLoader(ai, 0, GPU_FLOAT, 3);
    AttrInfo_AddLoader(ai, 2, GPU_FLOAT, 2);
    const int colour_slot = AttrInfo_AddFixed(ai, 1);
    if (colour_slot != 2) throw std::runtime_error("enhanced colour attribute admission failed");
    C3D_FixedAttribSet(colour_slot, 1.0f, 1.0f, 1.0f, 1.0f);
    C3D_BufInfo* bi = C3D_GetBufInfo();
    BufInfo_Init(bi);
    if (!vbo || BufInfo_Add(bi, vbo, sizeof(ProbeVertexUV), 2, 0x10) < 0)
        throw std::runtime_error("enhanced terrain vertex buffer admission failed");
}

void configure_immediate_attributes_0143() {
    C3D_AttrInfo* ai = C3D_GetAttrInfo();
    AttrInfo_Init(ai);
    AttrInfo_AddLoader(ai, 0, GPU_FLOAT, 3);
    AttrInfo_AddLoader(ai, 1, GPU_FLOAT, 4);
}

void configure_immediate_attributes_uv() {
    C3D_AttrInfo* ai = C3D_GetAttrInfo();
    AttrInfo_Init(ai);
    AttrInfo_AddLoader(ai, 0, GPU_FLOAT, 3);
    AttrInfo_AddLoader(ai, 1, GPU_FLOAT, 4);
    AttrInfo_AddLoader(ai, 2, GPU_FLOAT, 2);
}

void configure_texenv_0143() {
    C3D_TexEnv* env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
    C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
}

void configure_texenv_uv(bool textured) {
    C3D_TexEnv* env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    if (textured) {
        C3D_TexEnvSrc(env, C3D_Both, GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
        C3D_TexEnvFunc(env, C3D_Both, GPU_MODULATE);
    } else {
        C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
        C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
    }
}

bool draw_source_subtract_uv(uint8_t r,uint8_t g,uint8_t b,uint16_t width,uint16_t height) {
    if(!use_uv_renderer()||width!=320||height!=240)return false;
    // Original semitransparent TILE62 + E1000140 subtracts its grayscale
    // from the scene. Extend its full-screen coverage to the400x240 viewport.
    // PICA quantization and native scheduling still require device/reference
    // comparison; this adapter makes no PS1 packet-address claim.
    C3D_Mtx identity;Mtx_Identity(&identity);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER,g_u_projection,&identity);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER,g_u_model_view,&identity);
    configure_immediate_attributes_uv();configure_texenv_uv(false);
    C3D_DepthTest(false,GPU_ALWAYS,GPU_WRITE_COLOR);
    C3D_AlphaTest(false,GPU_ALWAYS,0);
    C3D_AlphaBlend(GPU_BLEND_REVERSE_SUBTRACT,GPU_BLEND_ADD,
        GPU_ONE,GPU_ONE,GPU_ONE,GPU_ZERO);
    constexpr float points[6][2]={{-1,-1},{1,-1},{1,1},{-1,-1},{1,1},{-1,1}};
    // Original undithered TILE converts RGB8 to RGB5 before subtraction.
    // Expand with the same convention as cooked materials; low bits must
    // not darken the scene before they reach the source's next RGB5 step.
    const auto channel=[](uint8_t v) {
        const unsigned five=v>>3;
        return float((five<<3)|(five>>2))/255.0f;
    };
    ++g_runtime_diag.draw_calls;
    C3D_ImmDrawBegin(GPU_TRIANGLES);
    for(const auto& point:points) {
        C3D_ImmSendAttrib(point[0],point[1],0.0f,1.0f);
        C3D_ImmSendAttrib(channel(r),channel(g),channel(b),1.0f);
        C3D_ImmSendAttrib(0.0f,0.0f,0.0f,0.0f);
        ++g_submitted_vertices;
    }
    C3D_ImmDrawEnd();
    C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_ONE,GPU_ZERO,GPU_ONE,GPU_ZERO);
    C3D_DepthTest(true,GPU_GEQUAL,GPU_WRITE_ALL);
    if(alpha_requested())C3D_AlphaTest(true,GPU_GREATER,0);
    return true;
}

void draw_source_fade_uv(const l3::SourceFadePresentation& fade) {
    if(fade.visible&&fade.subtract_from_scene)
        (void)draw_source_subtract_uv(fade.grayscale,fade.grayscale,fade.grayscale,fade.width,fade.height);
}

void send_immediate_0143(float x, float y, float z, float r, float g, float b, float a=1.0f) {
    const auto v = world_vertex_0143(x,y,z);
    ++g_submitted_vertices;
    C3D_ImmSendAttrib(v.x,v.y,v.z,1.0f);
    C3D_ImmSendAttrib(r,g,b,a);
}

void send_immediate_uv(float x, float y, float z, float r, float g, float b,
                       float u=0.0f, float v=0.0f, float a=1.0f) {
    const auto q = world_vertex_uv(x,y,z);
    ++g_submitted_vertices;
    C3D_ImmSendAttrib(q.x,q.y,q.z,1.0f);
    C3D_ImmSendAttrib(r,g,b,a);
    C3D_ImmSendAttrib(u,v,0.0f,0.0f);
}

void draw_player_0143(const std::vector<l3::PlayerVisualPrimitive>& local_visuals,
                      const l3::PbdMainSkeleton& skeleton,
                      const l3::PbdAnimationAction& action,
                      const l3::ReferenceTables& refs,
                      const l3::RuntimeSession& session) {
    // Keep this function behavior-identical to 0.0.14.3 for the renderer fallback.
    const size_t fi = (session.frame_counter() / 4u) % action.angle_frames.size();
    const auto bank = l3::decode_pbd_main_pose_bank(action.angle_frames[fi]);
    const auto matrices = l3::build_player_pose_matrices(skeleton, bank);
    const auto posed = l3::pose_player_visuals(local_visuals, matrices);
    if(posed.empty())return;
    const auto pos = session.player_position();
    const auto& trig = refs.trig[session.player_yaw() & 0x0FFFu];
    const float sn = float(trig.sin_q12) / 4096.0f;
    const float cs = float(trig.cos_q12) / 4096.0f;

    configure_texenv_0143();
    configure_immediate_attributes_0143();
    ++g_runtime_diag.draw_calls;
    C3D_ImmDrawBegin(GPU_TRIANGLES);
    for (const auto& p : posed) {
        const unsigned tri_order[6] = {0,1,2,0,2,3};
        const unsigned count = p.vertex_count == 3 ? 3u : 6u;
        for (unsigned oi=0; oi<count; ++oi) {
            const unsigned vi = p.vertex_count == 3 ? oi : tri_order[oi];
            const auto& q = p.position[vi];
            const float rx = q.x * cs - q.z * sn;
            const float rz = q.x * sn + q.z * cs;
            send_immediate_0143(float(pos.x)+rx, float(pos.y)+q.y, float(pos.z)+rz,
                                float(p.r)/255.0f, float(p.g)/255.0f, float(p.b)/255.0f);
        }
    }
    C3D_ImmDrawEnd();
}

// Owner-local inspection setup only. Explicit source-area identity and raw
// coordinates are validated; this is never a retail spawn/save/progression rule.
// FRONTEND_DIAGNOSTIC unattended input script (diagnostic_autoplay.txt).
// Lines: "<frames> <3DS key mask hex> <circle x> <circle y>". When present the
// display probes are skipped and the gameplay loop reads these keys instead
// of HID; the app exits when the script ends. Never part of normal play.
struct DiagnosticAutoplayStep { uint32_t frames=0; u32 keys=0; int16_t cx=0, cy=0; };
// Key mask 0xFFFFF: steer to the first live reward pickup with the tank
// controls (turn until facing it, then walk); the turn sign is learned.
constexpr u32 kAutoplaySteerToPickup=0xfffffu;
// Key mask 0xFFFFE: steer to world point (circle x, circle y) = (x, z).
constexpr u32 kAutoplaySteerToPoint=0xffffeu;
struct DiagnosticAutoplay {
    bool active=false; std::vector<DiagnosticAutoplayStep> steps;
    size_t index=0; uint32_t used=0; u32 previous=0;
    int turn_sign=1; int last_gap=-1; bool last_turned=false;
};
static DiagnosticAutoplay g_autoplay;
static void load_diagnostic_autoplay() {
    FILE* file=std::fopen("sdmc:/3ds/Legends3DS/diagnostic_autoplay.txt","r");
    if(!file)return;
    char line[128];
    while(std::fgets(line,sizeof line,file)) {
        if(line[0]=='#')continue;
        unsigned frames=0,keys=0;int cx=0,cy=0;
        if(std::sscanf(line,"%u %x %d %d",&frames,&keys,&cx,&cy)>=2&&frames)
            g_autoplay.steps.push_back({frames,u32(keys),int16_t(cx),int16_t(cy)});
    }
    std::fclose(file);
    g_autoplay.active=!g_autoplay.steps.empty();
}
// Returns false once the script is exhausted.
static bool diagnostic_autoplay_frame(u32& down, u32& held, u32& up, circlePosition& cp) {
    while(g_autoplay.index<g_autoplay.steps.size()&&g_autoplay.used>=g_autoplay.steps[g_autoplay.index].frames) {
        ++g_autoplay.index;g_autoplay.used=0;
    }
    if(g_autoplay.index>=g_autoplay.steps.size())return false;
    const auto& step=g_autoplay.steps[g_autoplay.index];
    ++g_autoplay.used;
    held=step.keys;cp.dx=step.cx;cp.dy=step.cy;
    // Steering steps are resolved by the caller; their circle fields are a goal.
    if(held==kAutoplaySteerToPickup||held==kAutoplaySteerToPoint){held=0;cp={};}
    down=held&~g_autoplay.previous;up=g_autoplay.previous&~held;g_autoplay.previous=held;
    return true;
}

void load_diagnostic_spawn(const l3::RuntimeMap& map, const l3::St00RenderSource& render,
                           size_t& area, l3::RuntimeVec3i& spawn, uint16_t& yaw) {
    if (!use_uv_renderer() || !g_sidecars_bound) return;
    FILE* file=std::fopen("sdmc:/3ds/Legends3DS/diagnostic_spawn_01414.txt","r");
    if (!file) return;
    unsigned long ai=0, heading=0;long x=0,y=0,z=0;char extra=0;
    const int fields=std::fscanf(file,"%lu %ld %ld %ld %lu %c",&ai,&x,&y,&z,&heading,&extra);
    std::fclose(file);
    if (fields!=5 || ai>=map.areas.size() || heading>4095 ||
        x<-32768 || x>32767 || y<-32768 || y>32767 || z<-32768 || z>32767 ||
        !l3::runtime_map_find_cell(map,ai,int32_t(x),int32_t(z)) ||
        validate_area_vertex_budget(map,render,ai)==0) {
        boot_log(MML_BOOT_DATA,"diagnostic spawn rejected; default retained");return;
    }
    area=ai;spawn={int32_t(x),int32_t(y),int32_t(z)};yaw=uint16_t(heading);
    boot_log(MML_BOOT_DATA,"explicit diagnostic spawn accepted; retail area/entry CANDIDATE");
}

uint32_t player_material_word(const l3::PosedPlayerVisualPrimitive& p) {
    return (uint32_t(p.clut) << 16u) | uint32_t(p.tpage);
}

void draw_player_material_group(const std::vector<l3::PosedPlayerVisualPrimitive>& posed,
                                uint32_t material_word,
                                bool semi_transparent,
                                const GpuMaterial* material,
                                const l3::ReferenceTables& refs,
                                const l3::RuntimeSession& session) {
    const bool textured = player_textures_requested() && material && material->texture_ready;
    if (textured) C3D_TexBind(0, const_cast<C3D_Tex*>(&material->texture));
    configure_texenv_uv(textured);
    // RGB and STP have independent semantics. Opaque opcodes ignore the STP
    // flag while still discarding zero texels. Shared pages retain STP for the
    // other opcode group; blending modes for that group remain unverified.
    if (textured) {
        C3D_TexEnvScale(C3D_GetTexEnv(0), C3D_RGB, GPU_TEVSCALE_2);
        C3D_TexEnvScale(C3D_GetTexEnv(0), C3D_Alpha,
            semi_transparent ? GPU_TEVSCALE_1 : GPU_TEVSCALE_2);
    }
    configure_immediate_attributes_uv();
    const auto pos = session.player_position();
    (void)refs;
    const auto& work=session.compat_player().bytes;
    const uint16_t source_0116=uint16_t(work[0x116])|(uint16_t(work[0x117])<<8u);
    const auto actor_matrix=l3::build_player_actor_render_matrix(
        {float(pos.x),float(pos.y),float(pos.z)},session.player_yaw(),source_0116);

    bool open=false;
    for (const auto& p : posed) {
        if (player_material_word(p) != material_word ||
            l3::ps1_textured_semitransparency_enabled(p.primitive_code) != semi_transparent) continue;
        if(!open){++g_runtime_diag.draw_calls;C3D_ImmDrawBegin(GPU_TRIANGLES);open=true;}
        const auto& tri_order = l3::kPs1QuadTriangleIndices;
        const unsigned count = p.vertex_count == 3 ? 3u : 6u;
        for (unsigned oi=0; oi<count; ++oi) {
            const unsigned vi = p.vertex_count == 3 ? oi : tri_order[oi];
            const auto& q = p.position[vi];
            const auto world=l3::player_pose_transform_point(actor_matrix,q);
            const float u = (float(p.uv[vi][0]) + 0.5f) / 256.0f;
            const float v = 1.0f - (float(p.uv[vi][1]) + 0.5f) / 256.0f;
            send_immediate_uv(world.x,world.y,world.z,
                              textured ? l3::ps1_texture_color_input(p.primitive_code, p.r) : float(p.r)/255.0f,
                              textured ? l3::ps1_texture_color_input(p.primitive_code, p.g) : float(p.g)/255.0f,
                              textured ? l3::ps1_texture_color_input(p.primitive_code, p.b) : float(p.b)/255.0f,
                              u, v);
        }
    }
    if(open)C3D_ImmDrawEnd();
}

void draw_player_uv(const std::vector<l3::PlayerVisualPrimitive>& local_visuals,
                    const std::vector<uint32_t>& material_words,
                    std::vector<l3::PosedPlayerVisualPrimitive>& posed,
                    const l3::PbdMainSkeleton& skeleton,
                    const l3::PbdAnimationAction& fallback_action,
                    const l3::PbdAnimation& animation,
                    const l3::PbdMainBodyTrack& track,
                    const l3::ReferenceTables& refs,
                    const l3::RuntimeSession& session) {
    const std::array<uint16_t, l3::kPbdPackedAngleChannels>* frame = nullptr;
    if (retail_animation_requested()) frame = l3::pbd_main_body_track_frame(animation, track);
    const auto& channels = frame ? *frame : fallback_probe_frame(fallback_action, session);
    const auto bank = l3::decode_pbd_main_pose_bank(channels);
    const auto matrices = l3::build_player_pose_matrices(skeleton, bank);
    l3::pose_player_visuals_into(local_visuals, matrices, posed);

    for (uint32_t word : material_words) {
        // Two bounded opcode groups, no frame allocation and no empty draws.
        for (bool semi : {false, true}) {
            bool present = false;
            for (const auto& p : posed) {
                if (player_material_word(p) == word &&
                    l3::ps1_textured_semitransparency_enabled(p.primitive_code) == semi) {
                    present = true; break;
                }
            }
            if (present) draw_player_material_group(posed, word, semi,
                find_player_gpu_material(word), refs, session);
        }
    }
}

// The real imported class20 mesh uses the existing enhanced immediate path.
// Owner page/CLUT and original byte UVs are optionally bound. White unlit
// modulation is a diagnostic, not recovered original shade. Missing cache or
// CLUT retains the existing flat gray path;0143 never enters this function.
void draw_imported_actors_uv(const l3::ActorAreaRuntime& area) {
    configure_immediate_attributes_uv();
    for (const auto& actor : area.actors) {
        if (actor.source_released || actor.source_hidden || !actor.resource || !actor.pose) continue;
        const auto pose=l3::build_imported_actor_pose_matrices(actor);
        bool open=false,textured=false;uint32_t active_word=0;
        for (size_t i=0;i<pose.part_count;++i) {
            if (!pose.draw_enabled[i]) continue;
            const auto& part=actor.resource->model.parts[i];
            if(part.triangles.empty()&&part.quads.empty())continue;
            const auto raw=actor.resource->bindings[i].material_word;
            const uint32_t word=(raw&0xffff0000u)|uint16_t(raw|0x20u);
            if(!open||word!=active_word){
                if(open)C3D_ImmDrawEnd();
                const GpuMaterial* material=nullptr;
                if(actor_textures_requested())for(const auto& m:g_actor_materials)
                    if(m.material_word==word&&m.texture_ready){material=&m;break;}
                textured=material!=nullptr;
                if(material)C3D_TexBind(0,const_cast<C3D_Tex*>(&material->texture));
                configure_texenv_uv(textured);
                ++g_runtime_diag.draw_calls;C3D_ImmDrawBegin(GPU_TRIANGLES);
                active_word=word;open=true;
            }
            const auto vertex=[&](uint8_t index,uint8_t u,uint8_t source_v) {
                const auto& v=part.vertices[index];
                const auto world=l3::player_pose_transform_point(pose.world[i],
                    {float(v.x),float(v.y),float(v.z)});
                const auto uv=l3::actor_render_uv(u,source_v);
                const float color=textured?1.f:128.f/255.f;
                send_immediate_uv(world.x,world.y,world.z,color,color,color,uv[0],uv[1]);
            };
            for (const auto& tri:part.triangles)for(size_t k=0;k<tri.index.size();++k)
                vertex(tri.index[k],tri.uv[k*2],tri.uv[k*2+1]);
            for (const auto& quad:part.quads)
                for (const auto index:l3::kPs1QuadTriangleIndices)
                    vertex(quad.index[index],quad.uv[index*2],quad.uv[index*2+1]);
        }
        if(open)C3D_ImmDrawEnd();
    }
}
// Real retained E0 meshes, not stand-in actors or the class20 pose hierarchy.
// This diagnostic renderer deliberately leaves texture/CLUT, source camera
// depth and GTE shade unacknowledged. ABR/STP are gated below; original
// ordering and PICA quantization are still not accepted as PS1 parity.
// ST00 E0/08 targets/doors: one opaque, untextured pass tinted by +0x40
// (FRONTEND_DIAGNOSTIC). Variant 0 is the first (E0,8) row, 1 the second (CANDIDATE).
static const l3::EbdDirectory* g_e008_mesh_owner=nullptr;
static std::vector<std::pair<size_t,l3::SceneE0MeshResource>> g_e008_meshes;
size_t draw_source_e008_uv(const l3::RuntimeSession& session) {
    if(!use_uv_renderer()||session.e008_objects().empty())return 0;
    const auto& ebd=session.active_area().ebd;
    if(g_e008_mesh_owner!=&ebd){g_e008_meshes.clear();g_e008_mesh_owner=&ebd;
        for(const auto row:l3::find_ebd_entries(ebd,0xE0,8)){
            try{g_e008_meshes.emplace_back(row,l3::decode_scene_e0_mesh(ebd,row,l3::SceneE0MeshTier::Near));}
            catch(const std::exception& e){boot_log(MML_BOOT_DATA,e.what());}
        }}
    if(g_e008_meshes.empty())return 0;
    configure_immediate_attributes_uv();configure_texenv_uv(false);
    size_t drawn=0;
    for(const auto& o:session.e008_objects()){
        if(o.released||(o.target&&o.state>3))continue;
        const auto& mesh=g_e008_meshes[o.target?0:std::min<size_t>(1,g_e008_meshes.size()-1)].second;
        const float a=float(o.heading_30&0xfff)*6.2831853f/4096.f,cs=std::cos(a),sn=std::sin(a);
        const float r=float(o.color_40&255u)/255.f,g=float((o.color_40>>8)&255u)/255.f,b=float((o.color_40>>16)&255u)/255.f;
        const auto vertex=[&](uint8_t index,uint8_t u,uint8_t v){
            const auto& p=mesh.part.vertices[index];
            const float x=float(o.position[0])+cs*float(p.x)+sn*float(p.z);
            const float z=float(o.position[2])-sn*float(p.x)+cs*float(p.z);
            const auto uv=l3::actor_render_uv(u,v);
            send_immediate_uv(x,float(o.position[1])+float(p.y),z,r,g,b,uv[0],uv[1]);
        };
        ++g_runtime_diag.draw_calls;C3D_ImmDrawBegin(GPU_TRIANGLES);
        for(const auto& tri:mesh.part.triangles)for(size_t k=0;k<3;++k)vertex(tri.index[k],tri.uv[k*2],tri.uv[k*2+1]);
        for(const auto& quad:mesh.part.quads)for(auto k:l3::kPs1QuadTriangleIndices)vertex(quad.index[k],quad.uv[k*2],quad.uv[k*2+1]);
        C3D_ImmDrawEnd();++drawn;
    }
    return drawn;
}
// ST00 E0/05 boxes: the first (E0,5) row, untextured, tinted by +0x40
// (FRONTEND_DIAGNOSTIC); hidden once the break action starts.
static const l3::EbdDirectory* g_e005_mesh_owner=nullptr;
static std::optional<l3::SceneE0MeshResource> g_e005_mesh;
size_t draw_source_e005_uv(const l3::RuntimeSession& session) {
    if(!use_uv_renderer()||session.e005_boxes().empty())return 0;
    const auto& ebd=session.active_area().ebd;
    if(g_e005_mesh_owner!=&ebd){g_e005_mesh.reset();g_e005_mesh_owner=&ebd;
        for(const auto row:l3::find_ebd_entries(ebd,0xE0,5)){
            try{g_e005_mesh=l3::decode_scene_e0_mesh(ebd,row,l3::SceneE0MeshTier::Near);break;}
            catch(const std::exception& e){boot_log(MML_BOOT_DATA,e.what());}
        }}
    if(!g_e005_mesh)return 0;
    configure_immediate_attributes_uv();configure_texenv_uv(false);
    size_t drawn=0;
    for(const auto& box:session.e005_boxes()){
        const auto& o=box.object;
        if(o.released||o.action_09==1)continue;
        const float r=float(o.color_40&255u)/255.f,g=float((o.color_40>>8)&255u)/255.f,b=float((o.color_40>>16)&255u)/255.f;
        const auto vertex=[&](uint8_t index,uint8_t u,uint8_t v){
            const auto& p=g_e005_mesh->part.vertices[index];
            const auto uv=l3::actor_render_uv(u,v);
            send_immediate_uv(float(o.position[0])+float(p.x),float(o.position[1])+float(p.y),float(o.position[2])+float(p.z),r,g,b,uv[0],uv[1]);
        };
        ++g_runtime_diag.draw_calls;C3D_ImmDrawBegin(GPU_TRIANGLES);
        for(const auto& tri:g_e005_mesh->part.triangles)for(size_t k=0;k<3;++k)vertex(tri.index[k],tri.uv[k*2],tri.uv[k*2+1]);
        for(const auto& quad:g_e005_mesh->part.quads)for(auto k:l3::kPs1QuadTriangleIndices)vertex(quad.index[k],quad.uv[k*2],quad.uv[k*2+1]);
        C3D_ImmDrawEnd();++drawn;
    }
    return drawn;
}
size_t draw_source_scene_e0_uv(const l3::RuntimeSession& session,const l3::SceneE0Resources& resources) {
    if(!use_uv_renderer()||!resources.resident_in(session.active_area().ebd))return 0;
    configure_immediate_attributes_uv();configure_texenv_uv(false);
    const auto& pool=session.source_event_phase5_context().pool_e0;
    size_t instances=0,semi_instances=0,held_materials=0,submitted_passes=0;
    for(size_t slot=0;slot<pool.slots.size();++slot) {
        const auto draw=l3::scene_e0_draw_view(pool.slots[slot],resources,
            session.active_area().ebd,l3::SceneE0MeshTier::Near);
        if(draw.status!=l3::SceneE0Status::Ready)continue;
        const auto& v=draw.view;
        if(v.mesh->part.triangles.empty()&&v.mesh->part.quads.empty())continue;
        const GpuMaterial* material=nullptr;
        if(g_source_scene_textures)for(const auto& m:g_scene_materials)
            if(m.material_word==v.mesh->material_word&&m.texture_ready){material=&m;break;}
        const auto plan=l3::plan_scene_e0_material(v.modulation_word,v.mesh->material_word,
            alpha_requested(),material!=nullptr,material!=nullptr);
        if(plan.status!=l3::SceneE0MaterialPlanStatus::Ready){++held_materials;continue;}
        if(plan.semi_transparent)++semi_instances;
        if(material)C3D_TexBind(0,const_cast<C3D_Tex*>(&material->texture));
        configure_texenv_uv(material!=nullptr);
        if(material){C3D_TexEnvScale(C3D_GetTexEnv(0),C3D_RGB,GPU_TEVSCALE_2);
            C3D_TexEnvScale(C3D_GetTexEnv(0),C3D_Alpha,GPU_TEVSCALE_2);}
        const float scale=material?256.f:255.f;
        const float r=float(v.modulation_word&255u)/scale;
        const float g=float((v.modulation_word>>8)&255u)/scale;
        const float b=float((v.modulation_word>>16)&255u)/scale;
        const auto vertex=[&](uint8_t index,uint8_t u,uint8_t source_v) {
            const auto& p=v.mesh->part.vertices[index];
            const float x=float(v.position[0])+(float(v.rotation[0])*p.x+float(v.rotation[1])*p.y+float(v.rotation[2])*p.z)/4096.f;
            const float y=float(v.position[1])+(float(v.rotation[3])*p.x+float(v.rotation[4])*p.y+float(v.rotation[5])*p.z)/4096.f;
            const float z=float(v.position[2])+(float(v.rotation[6])*p.x+float(v.rotation[7])*p.y+float(v.rotation[8])*p.z)/4096.f;
            const auto uv=l3::actor_render_uv(u,source_v);
            send_immediate_uv(x,y,z,r,g,b,uv[0],uv[1]);
        };
        ++instances;
        for(size_t pass_index=0;pass_index<plan.pass_count;++pass_index){
            const auto& pass=plan.passes[pass_index];
            if(material)C3D_TexEnvScale(C3D_GetTexEnv(0),C3D_Alpha,
                pass.alpha_tev_scale==1?GPU_TEVSCALE_1:GPU_TEVSCALE_2);
            switch(pass.texels){
            case l3::SceneE0TexelPass::OpaqueOnly:C3D_AlphaTest(true,GPU_GREATER,128);break;
            case l3::SceneE0TexelPass::StpOnly:C3D_AlphaTest(true,GPU_EQUAL,128);break;
            default:if(material||alpha_requested())C3D_AlphaTest(true,GPU_GREATER,0);break;
            }
            // Source ABR fractions are mapped to the nearest representable
            // PICA constant. RGB5 truncation and original OT ordering remain
            // separate CANDIDATE raster boundaries, not 1:1 acceptance.
            const bool quarter=pass.source==l3::SceneE0BlendFactor::Quarter;
            C3D_BlendingColor(quarter?0x40404040u:0x80808080u);
            const auto factor=[](l3::SceneE0BlendFactor value)->GPU_BLENDFACTOR {
                switch(value){case l3::SceneE0BlendFactor::Zero:return GPU_ZERO;
                case l3::SceneE0BlendFactor::One:return GPU_ONE;
                default:return GPU_CONSTANT_ALPHA;}
            };
            C3D_AlphaBlend(pass.equation==l3::SceneE0BlendEquation::DestinationMinusSource?
                GPU_BLEND_REVERSE_SUBTRACT:GPU_BLEND_ADD,GPU_BLEND_ADD,
                factor(pass.source),factor(pass.destination),GPU_ONE,GPU_ZERO);
            const bool stp=pass.texels==l3::SceneE0TexelPass::StpOnly;
            C3D_DepthTest(true,GPU_GEQUAL,stp?GPU_WRITE_COLOR:GPU_WRITE_ALL);
            ++submitted_passes;++g_runtime_diag.draw_calls;C3D_ImmDrawBegin(GPU_TRIANGLES);
            for(const auto& tri:v.mesh->part.triangles)for(size_t k=0;k<3;++k)
                vertex(tri.index[k],tri.uv[k*2],tri.uv[k*2+1]);
            for(const auto& quad:v.mesh->part.quads)for(auto k:l3::kPs1QuadTriangleIndices)
                vertex(quad.index[k],quad.uv[k*2],quad.uv[k*2+1]);
            C3D_ImmDrawEnd();
        }
        C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_ONE,GPU_ZERO,GPU_ONE,GPU_ZERO);
        C3D_BlendingColor(0);C3D_DepthTest(true,GPU_GEQUAL,GPU_WRITE_ALL);
        C3D_AlphaTest(alpha_requested(),GPU_GREATER,0);
    }
    if(session.frame_counter()%60u==0u)
        hardware_test_trace("# source_E0_material,frame=%lu,instances=%lu,semi_instances=%lu,passes=%lu,held=%lu,alpha_gate=%u,STP_POLICY=REQUIRE_RETAINED_NEAREST,blend_quantization=CANDIDATE,OT_order=UNKNOWN",
            (unsigned long)session.frame_counter(),(unsigned long)instances,(unsigned long)semi_instances,
            (unsigned long)submitted_passes,(unsigned long)held_materials,unsigned(alpha_requested()));
    return instances;
}

size_t draw_source_scene_actor40_uv(const l3::RuntimeSession& session,const l3::SourceSceneActor40Resources& resource,const l3::ReferenceTables& refs) {
    const auto& work=session.source_event_phase5_context().pool40.slots[0];
    const auto* selected=l3::select_source_scene_actor40_pose(work,resource);
    const auto* mesh=l3::select_source_scene_actor40_mesh(work,resource);
    if(!selected||!mesh)return 0;
    const auto integer_pose=l3::produce_source_scene_actor40_integer_pose_8003346c(work,resource,
        &refs,session.source_event_phase6_context().byte_1f800074,*selected);
    if(integer_pose.status!=l3::SourceSceneActor40PoseStatus::SourceBackedPartial)return 0;
    const auto pose=l3::present_source_scene_actor40_integer_pose(integer_pose);
    configure_immediate_attributes_uv();configure_texenv_uv(false);size_t drawn=0;
    for(size_t i=0;i<pose.part_count;++i){if(!pose.draw_enabled[i])continue;
        const auto& part=mesh->model.parts[i];
        if(part.triangles.empty()&&part.quads.empty())continue;
        const GpuMaterial* material=nullptr;
        if(g_source_scene_textures)for(const auto& m:g_scene_actor40_materials)
            if(m.material_word==mesh->bindings[i].material_word&&m.texture_ready){material=&m;break;}
        if(material)C3D_TexBind(0,const_cast<C3D_Tex*>(&material->texture));
        configure_texenv_uv(material!=nullptr);
        if(material){C3D_TexEnvScale(C3D_GetTexEnv(0),C3D_RGB,GPU_TEVSCALE_2);
            C3D_TexEnvScale(C3D_GetTexEnv(0),C3D_Alpha,GPU_TEVSCALE_2);}
        // Neutral0x80 diagnostic; source shade350 and GTE lighting remain UNKNOWN.
        const float color=128.f/(material?256.f:255.f);
        const auto vertex=[&](uint8_t index,uint8_t u,uint8_t v){const auto& p=part.vertices[index];
            const auto point=l3::player_pose_transform_point(pose.world[i],{float(p.x),float(p.y),float(p.z)});
            const auto uv=l3::actor_render_uv(u,v);send_immediate_uv(point.x,point.y,point.z,color,color,color,uv[0],uv[1]);};
        ++drawn;++g_runtime_diag.draw_calls;C3D_ImmDrawBegin(GPU_TRIANGLES);
        for(const auto& tri:part.triangles)for(size_t j=0;j<3;++j)vertex(tri.index[j],tri.uv[j*2],tri.uv[j*2+1]);
        for(const auto& quad:part.quads)for(auto j:l3::kPs1QuadTriangleIndices)vertex(quad.index[j],quad.uv[j*2],quad.uv[j*2+1]);
        C3D_ImmDrawEnd();
    }
    return drawn;
}

struct ActorSourceMetadata {
    std::optional<l3::ActorDetectionProfiles> detection;
    std::optional<l3::SourceNormalizationLookup> normalization;
    std::optional<l3::RetailPlayerBounds> bounds;
    std::optional<l3::SourceCombatProfiles> combat;
    std::optional<l3::Class60_06Profile> child6;
    std::optional<l3::SourceEventPollProfile> event_poll;
    std::optional<l3::SourceEventPhase2Package> event_phase2;
    std::optional<l3::SourceEventCommandPackage> event_command;
    std::optional<l3::SourceFadeStartupProfile> fade_startup;
    std::optional<l3::SourceEventPhase5Profile> event_phase5;
    std::optional<l3::SourceEventPhase6Profile> event_phase6;
    std::optional<l3::SourceEffectDispatchProfile> effect_dispatch;
    std::optional<l3::SourcePropLiftProfile> prop_lift;
    std::optional<l3::SourceSceneViewProfile> scene_view;
    std::optional<l3::SourceReactionProfile> reaction;
    std::optional<l3::SourceHitProxyRules> hit_proxy_rules;
    std::optional<l3::SourceRngProfile> rng;
    std::optional<l3::SourcePlayerTargetProfile> target;
};
ActorSourceMetadata load_actor_source_metadata() {
    ActorSourceMetadata out;
    if(!g_sidecars_bound)return out;
    const auto load=[&](const char* name,size_t limit,const auto& decode){
        const auto path=data_path(name);
        if(FILE* present=std::fopen(path.c_str(),"rb")){
            std::fclose(present);
            try {decode(read_binary(path,limit));boot_log(MML_BOOT_DATA,name);}
            catch(const std::exception& error){boot_log(MML_BOOT_DATA,error.what());}
        }else boot_log(MML_BOOT_DATA,"optional actor metadata absent; dependent behavior held");
    };
    load("actor_detection.l3dp",32+256*4,[&](const auto& bytes){
        out.detection=l3::load_l3actor_detection_profiles(bytes.data(),bytes.size());});
    load("actor_normalization.l3nr",l3::kSourceNormalizationPackageBytes,[&](const auto& bytes){
        out.normalization=l3::load_l3normalization_lookup(bytes.data(),bytes.size());});
    load("actor_detection_bounds.l3db",44,[&](const auto& bytes){
        out.bounds=l3::load_actor_detection_segment_bounds(bytes);});
    load("actor_combat.l3cb",32+l3::kSourceCombatProfileCapacity*l3::kSourceCombatProfileV5EntryBytes,
        [&](const auto& bytes){out.combat=l3::load_l3source_combat_profiles(bytes.data(),bytes.size());});
    load("class60_06.l3c6",112,[&](const auto& bytes){out.child6=l3::load_source_profile_class60_06(bytes.data(),bytes.size());});
    load("source_event.l3ep",64+256*4,[&](const auto& bytes){out.event_poll=l3::load_source_event_poll_profile(bytes);});
    load("source_event_command.l3ec",l3::kSourceEventCommandPackageBytes,
        [&](const auto& bytes){out.event_command=l3::load_source_event_command_package(bytes);});
    load("source_fade.l3fd",72,[&](const auto& bytes){out.fade_startup=l3::load_source_fade_startup_profile(bytes);});
    load("source_event_phase5.l3p5",1024,[&](const auto& bytes){out.event_phase5=l3::load_source_event_phase5_profile(bytes);});
    load("source_event_phase6.l3p6",2048,[&](const auto& bytes){out.event_phase6=l3::load_source_event_phase6_profile(bytes);});
    load("source_effect_dispatch.l3f2",l3::kSourceEffectDispatchPackageBytes,
        [&](const auto& bytes){out.effect_dispatch=l3::load_source_effect_dispatch_profile(bytes);});
    load("source_prop_lift.l3pl",l3::kSourcePropLiftProfileBytes,
        [&](const auto& bytes){out.prop_lift=l3::load_source_prop_lift_profile(bytes);});
    load("player/target.l3tg",48,[&](const auto& bytes){l3::SourcePlayerTargetProfile profile;
        if(!l3::decode_source_player_target_profile(bytes,profile))throw std::runtime_error("L3TG rejected");
        out.target=profile;});
    load("source_scene_view.l3sv",l3::kSourceSceneViewProfileBytes,[&](const auto& bytes){out.scene_view=l3::load_source_scene_view_profile(bytes);});
    load("actor_reactions.l3r7",100,[&](const auto& bytes){out.reaction=l3::load_source_reaction_profile(bytes.data(),bytes.size());});
    load("source_event_phase2.l3eq",l3::kSourceEventPhase2PackageBytes,
        [&](const auto& bytes){out.event_phase2=l3::load_source_event_phase2_package(bytes);});
    load("hit_proxy_rules.l3pr",32,[&](const auto& bytes){l3::SourceHitProxyRules rules;
        if(!l3::decode_source_hit_proxy_rules(bytes,rules))throw std::runtime_error("invalid hit proxy rules");
        out.hit_proxy_rules=rules;});
    load("source_rng.l3rg",l3::kSourceRngProfileBytes,[&](const auto& bytes){l3::SourceRngProfile profile;
        if(l3::load_l3source_rng_profile(bytes.data(),bytes.size(),profile)!=l3::SourceRngStatus::Ready)
            throw std::runtime_error("invalid observed RNG profile");
        out.rng=profile;});
    return out;
}
l3::ActorAreaRuntime load_area_actors(const l3::TutorialRuntimeArea& area,
                                    const l3::RuntimeMap& map, size_t area_index,
                                    const ActorSourceMetadata& metadata,
                                    std::optional<uint8_t> selector) {
    auto actors=l3::spawn_imported_actors(area.stage,area.ebd);
    for (auto& actor:actors.actors) {
        try {
            if (!actor.pose) l3::bind_imported_actor_pose(actor,area.ebd,0,0,false);
            const auto floor=l3::sample_imported_actor_floor(actor,map,area_index);
            if (floor.floor_y) l3::bind_imported_actor_floor(actor,*floor.floor_y);
            if(metadata.combat)(void)l3::bind_imported_actor_combat_profile(actor,*metadata.combat,selector);
            if(metadata.combat)(void)l3::bind_imported_actor_reaverbot19_record(actor,*metadata.combat,selector);
        } catch (const std::exception& error) {boot_log(MML_BOOT_DATA,error.what());}
    }
    prepare_actor_animation_banks(actors,area.ebd);
    if(g_source_actor_proxy)for(auto& actor:actors.actors)
        (void)l3::initialize_imported_actor_fresh_pose_proxy_subset(actor);
    return actors;
}
l3::ActorAreaRuntime load_active_actors(const l3::RuntimeSession& session,
                                      const ActorSourceMetadata& metadata) {
    const auto& area=session.active_area();
    auto records=area.flow.objects;
    const auto& scripted=session.conditional_spawn_objects().records;
    records.records.insert(records.records.end(),scripted.begin(),scripted.end());
    // Rebuild only when a source gate changes its group. Shared class pools are
    // bounded across static and conditional records, not separately per list.
    auto stage=l3::build_fresh_stage_runtime_area(records,area.ebd);
    auto actors=l3::spawn_imported_actors(stage,area.ebd);
    for(auto& actor:actors.actors){
        try {
            if(!actor.pose)l3::bind_imported_actor_pose(actor,area.ebd,0,0,false);
            const auto floor=l3::sample_imported_actor_floor(actor,session.active_map(),session.map_area_index());
            if(floor.floor_y)l3::bind_imported_actor_floor(actor,*floor.floor_y);
            if(metadata.combat)(void)l3::bind_imported_actor_combat_profile(actor,*metadata.combat,session.source_stats_selector());
            if(metadata.combat)(void)l3::bind_imported_actor_reaverbot19_record(actor,*metadata.combat,session.source_stats_selector());
        } catch(const std::exception& error){boot_log(MML_BOOT_DATA,error.what());}
    }
    prepare_actor_animation_banks(actors,area.ebd);
    // This loader creates a new diagnostic logical pool through
    // build_fresh_stage_runtime_area, including conditional group changes.
    // Apply the explicit source fresh-init subset to the prepared candidate;
    // never infer retained fields from an existing presentation reload.
    if(g_source_actor_proxy)for(auto& actor:actors.actors)
        (void)l3::initialize_imported_actor_fresh_pose_proxy_subset(actor);
    hardware_test_actor_event(session.map_area_index(),actors);
    return actors;
}

void draw_projectiles_0143(const l3::RuntimeSession& session) {
    if(std::none_of(session.projectiles().begin(),session.projectiles().end(),
        [](const auto& p){return p.active;}))return;
    configure_texenv_0143();
    configure_immediate_attributes_0143();
    ++g_runtime_diag.draw_calls;
    C3D_ImmDrawBegin(GPU_TRIANGLES);
    for (const auto& p : session.projectiles()) if (p.active) {
        const float s = 18.0f;
        send_immediate_0143(float(p.position.x)-s,float(p.position.y),float(p.position.z),1.0f,0.9f,0.2f);
        send_immediate_0143(float(p.position.x)+s,float(p.position.y),float(p.position.z),1.0f,0.9f,0.2f);
        send_immediate_0143(float(p.position.x),float(p.position.y)-s,float(p.position.z),1.0f,1.0f,0.4f);
    }
    C3D_ImmDrawEnd();
}

void draw_projectiles_uv(const l3::RuntimeSession& session) {
    if(std::none_of(session.projectiles().begin(),session.projectiles().end(),
        [](const auto& p){return p.active;}))return;
    configure_texenv_uv(false);
    configure_immediate_attributes_uv();
    ++g_runtime_diag.draw_calls;
    C3D_ImmDrawBegin(GPU_TRIANGLES);
    for (const auto& p : session.projectiles()) if (p.active) {
        const float s = 18.0f;
        send_immediate_uv(float(p.position.x)-s,float(p.position.y),float(p.position.z),1.0f,0.9f,0.2f);
        send_immediate_uv(float(p.position.x)+s,float(p.position.y),float(p.position.z),1.0f,0.9f,0.2f);
        send_immediate_uv(float(p.position.x),float(p.position.y)-s,float(p.position.z),1.0f,1.0f,0.4f);
    }
    C3D_ImmDrawEnd();
}

// Native presentation of the live source Buster collision segment during
// callback state 1: the owner point (+2C/+30 as written by 0x800434F0) to the
// +14/+18 endpoint, as registered through 0x8003430C. It reads the recorded
// registration because 0x80043708..24 copies +14 over +2C right after it; the
// work fields alone always give a zero-length segment (0.0.15.0..0.0.15.10).
// This is a FRONTEND_DIAGNOSTIC ribbon: the original shot sprite/effect
// renderer is not ported. 0.0.15.14: the Square shot is subtype 0x81, whose
// segment is its previous point (+2C) to its current point (+14), 100 units.
size_t draw_source_busters_uv(const l3::ChildRuntime& children) {
    const auto& pool=children.buster_pool();
    const auto& segments=children.buster_registered_segments();
    size_t drawn=0;
    for(size_t slot=0;slot<l3::source_buster_capacity;++slot){
        if(!(pool.allocated_800989f0&(0x80000000u>>slot)))continue;
        const auto& w=pool.slots[slot];
        if(!(int8_t(w[0])&w[6]&1)||(w[2]!=0x80&&w[2]!=0x81)||w[8]!=1)continue;
        if(!drawn){configure_texenv_uv(false);configure_immediate_attributes_uv();++g_runtime_diag.draw_calls;C3D_ImmDrawBegin(GPU_TRIANGLES);}
        const auto& segment=segments[slot];
        const float ax=segment.second_2c[0],ay=segment.second_2c[1],az=segment.second_2c[2];
        const float bx=segment.first_14[0],by=segment.first_14[1],bz=segment.first_14[2];
        constexpr float h=6.0f;
        send_immediate_uv(ax,ay-h,az,1.0f,0.95f,0.3f);send_immediate_uv(bx,by-h,bz,1.0f,0.95f,0.3f);send_immediate_uv(bx,by+h,bz,1.0f,1.0f,0.6f);
        send_immediate_uv(ax,ay-h,az,1.0f,0.95f,0.3f);send_immediate_uv(bx,by+h,bz,1.0f,1.0f,0.6f);send_immediate_uv(ax,ay+h,az,1.0f,1.0f,0.6f);
        ++drawn;
    }
    if(drawn)C3D_ImmDrawEnd();
    return drawn;
}

// Live subtype-0x87 reward pickups (0x80046B34). FRONTEND_DIAGNOSTIC marker:
// the original model (0x8002F9C4 resource at +0x14, rotation matrix +0x18)
// is not ported. A small diamond at the source position, sized by +0x3E,
// hidden while the original visible bit (+0 bit 1) is clear, so the blink
// state flashes like the original.
size_t draw_source_reward_pickups_uv(const l3::ChildRuntime& children) {
    const auto& pool=children.effect_pool();
    size_t drawn=0;
    for(size_t slot=0;slot<l3::source_effect_capacity;++slot){
        if(!(pool.allocated_80098838[slot/32]&(0x80000000u>>(slot%32))))continue;
        const auto& w=pool.slots[slot];
        if(w[2]!=l3::kSourceRewardPickupSubtype||!(w[0]&w[6]&1u)||!(w[0]&2u)||w[8]>=7)continue;
        if(!drawn){configure_texenv_uv(false);configure_immediate_attributes_uv();++g_runtime_diag.draw_calls;C3D_ImmDrawBegin(GPU_TRIANGLES);}
        const auto h16=[&](size_t o){return float(int16_t(uint16_t(w[o]|(w[o+1]<<8))));};
        const float x=h16(0x38),y=h16(0x3a),z=h16(0x3c),s=std::max(4.0f,h16(0x3e)/8.0f);
        const float r=w[3]?1.0f:0.3f,g=w[3]?0.35f:1.0f,b=w[3]?0.35f:0.8f;
        send_immediate_uv(x-s,y,z,r,g,b);send_immediate_uv(x+s,y,z,r,g,b);send_immediate_uv(x,y-s,z,1,1,1);
        send_immediate_uv(x-s,y,z,r,g,b);send_immediate_uv(x+s,y,z,r,g,b);send_immediate_uv(x,y+s,z,r*0.6f,g*0.6f,b*0.6f);
        send_immediate_uv(x,y,z-s,r,g,b);send_immediate_uv(x,y,z+s,r,g,b);send_immediate_uv(x,y-s,z,1,1,1);
        send_immediate_uv(x,y,z-s,r,g,b);send_immediate_uv(x,y,z+s,r,g,b);send_immediate_uv(x,y+s,z,r*0.6f,g*0.6f,b*0.6f);
        ++drawn;
    }
    if(drawn)C3D_ImmDrawEnd();
    return drawn;
}

void draw_terrain_range(size_t count) {
    for (size_t first=0; first<count;) {
        size_t n=std::min<size_t>(kDrawBatchVertices,count-first);
        n-=n%3u; if(n==0)break;
        ++g_runtime_diag.draw_calls;
        g_submitted_vertices+=static_cast<uint32_t>(n);
        C3D_DrawArrays(GPU_TRIANGLES,(int)first,(int)n); first+=n;
    }
}

void draw_terrain_overlay() {
    for (const auto& o : g_terrain_overlay) {
        if (!use_uv_renderer()) {
            configure_terrain_attributes_0143(static_cast<const ProbeVertex0143*>(o.vbo));
        } else {
            configure_terrain_attributes_uv(static_cast<const ProbeVertexUV*>(o.vbo));
            if (o.texture) C3D_TexBind(0,const_cast<C3D_Tex*>(o.texture));
            configure_texenv_uv(o.texture != nullptr);
        }
        draw_terrain_range(o.vertex_count);
    }
}

// 0x2C010101: the original back-face fill modulates the texel by 1/128.
void configure_texenv_black_uv(bool textured) {
    C3D_TexEnv* env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    if (textured) {
        C3D_TexEnvSrc(env, C3D_RGB, GPU_TEXTURE0, GPU_CONSTANT, GPU_PRIMARY_COLOR);
        C3D_TexEnvFunc(env, C3D_RGB, GPU_MODULATE);
        C3D_TexEnvSrc(env, C3D_Alpha, GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
        C3D_TexEnvFunc(env, C3D_Alpha, GPU_MODULATE);
    } else {
        C3D_TexEnvSrc(env, C3D_RGB, GPU_CONSTANT, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
        C3D_TexEnvFunc(env, C3D_RGB, GPU_REPLACE);
        C3D_TexEnvSrc(env, C3D_Alpha, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
        C3D_TexEnvFunc(env, C3D_Alpha, GPU_REPLACE);
    }
    C3D_TexEnvColor(env, 0xFF020202u);
}

struct TerrainFaceStats { uint32_t drawn = 0, black = 0, skipped = 0, unsupported = 0, batches_unclassified = 0; };
static TerrainFaceStats g_terrain_face_stats;

// faces == nullptr keeps the previous draw-everything behaviour.
void draw_terrain(const l3::TerrainFaceCamera* faces = nullptr, uint8_t positive_mode = 0) {
    g_terrain_face_stats = {};
    if (!use_uv_renderer()) {
        configure_texenv_0143();
        configure_terrain_attributes_0143();
        for (size_t first=0; first<g_terrain_vertex_count_0143;) {
            size_t n=std::min<size_t>(kDrawBatchVertices,g_terrain_vertex_count_0143-first);
            n-=n%3u; if(n==0)break;
            ++g_runtime_diag.draw_calls;
            g_submitted_vertices+=static_cast<uint32_t>(n);
            C3D_DrawArrays(GPU_TRIANGLES,(int)first,(int)n); first+=n;
        }
        draw_terrain_overlay();
        return;
    }

    if (!use_material_batches()) {
        configure_texenv_uv(false);
        configure_terrain_attributes_uv(g_terrain_vbo_uv);
        for (size_t first=0; first<g_terrain_vertex_count_uv;) {
            size_t n=std::min<size_t>(kDrawBatchVertices,g_terrain_vertex_count_uv-first);
            n-=n%3u; if(n==0)break;
            ++g_runtime_diag.draw_calls;
            g_submitted_vertices+=static_cast<uint32_t>(n);
            C3D_DrawArrays(GPU_TRIANGLES,(int)first,(int)n); first+=n;
        }
        draw_terrain_overlay();
        return;
    }

    for (auto& batch : g_terrain_batches) {
        configure_terrain_attributes_uv(batch.vbo);
        if (batch.texture_ready) C3D_TexBind(0,const_cast<C3D_Tex*>(&batch.texture));
        configure_texenv_uv(batch.texture_ready);
        batch.black_count = 0;
        if (faces && batch.indices) {
            // Original per-quad face test (0x80025900 / 0x80024F30 family
            // rules) with the 0x1F800008 camera; see terrain_visibility.h.
            const size_t q = batch.quads.size();
            size_t drawn = 0, black = 0;
            for (size_t k = 0; k < q; ++k) {
                const auto& quad = batch.quads[k];
                const auto r = l3::classify_terrain_face(*faces, quad.x, quad.y, quad.z, quad.prefix,
                    quad.negative, positive_mode, l3::kObservedArea1BackFaceThreshold);
                uint16_t* out = nullptr;
                switch (r.face) {
                case l3::TerrainFace::Draw: out = batch.indices + 6u * drawn++; ++g_terrain_face_stats.drawn; break;
                case l3::TerrainFace::Unsupported: out = batch.indices + 6u * drawn++; ++g_terrain_face_stats.unsupported; break;
                case l3::TerrainFace::Black: out = batch.indices + 6u * (q + black++); ++g_terrain_face_stats.black; break;
                case l3::TerrainFace::Skip: ++g_terrain_face_stats.skipped; break;
                }
                if (out) for (unsigned i = 0; i < 6u; ++i) out[i] = uint16_t(6u * k + i);
            }
            GSPGPU_FlushDataCache(batch.indices, q * 12u * sizeof(uint16_t));
            batch.black_count = black;
            if (drawn) {
                ++g_runtime_diag.draw_calls;
                g_submitted_vertices += static_cast<uint32_t>(6u * drawn);
                C3D_DrawElements(GPU_TRIANGLES, int(6u * drawn), C3D_UNSIGNED_SHORT, batch.indices);
            }
            continue;
        }
        if (faces) ++g_terrain_face_stats.batches_unclassified;
        for (size_t first=0; first<batch.vertex_count;) {
            size_t n=std::min<size_t>(kDrawBatchVertices,batch.vertex_count-first);
            n-=n%3u; if(n==0)break;
            ++g_runtime_diag.draw_calls;
            g_submitted_vertices+=static_cast<uint32_t>(n);
            C3D_DrawArrays(GPU_TRIANGLES,(int)first,(int)n); first+=n;
        }
    }
    // Back faces the original fills with 0x2C010101.
    for (const auto& batch : g_terrain_batches) {
        if (!batch.black_count) continue;
        configure_terrain_attributes_uv(batch.vbo);
        if (batch.texture_ready) C3D_TexBind(0,const_cast<C3D_Tex*>(&batch.texture));
        configure_texenv_black_uv(batch.texture_ready);
        ++g_runtime_diag.draw_calls;
        g_submitted_vertices += static_cast<uint32_t>(6u * batch.black_count);
        C3D_DrawElements(GPU_TRIANGLES, int(6u * batch.black_count), C3D_UNSIGNED_SHORT,
                         batch.indices + 6u * batch.quads.size());
    }
    draw_terrain_overlay();
}

uint16_t make_psx_held(u32 keys, const circlePosition& cp) {
    uint16_t m = 0;
    if (keys & KEY_DUP) m |= 0x0010u;
    if (keys & KEY_DDOWN) m |= 0x0040u;
    if (keys & KEY_DLEFT) m |= 0x0400u; // Installed classic profile turns with shoulder masks.
    if (keys & KEY_DRIGHT) m |= 0x0800u;
    if (keys & KEY_L) m |= 0x0400u;
    if (keys & KEY_R) m |= 0x0800u;
    if (keys & KEY_B) m |= 0x4000u;
    if (keys & KEY_A) m |= 0x2000u; // Circle: profile-0 +0x134/+0x136 (door/check action).
    if (cp.dy > 28) m |= 0x0010u;
    if (cp.dy < -28) m |= 0x0040u;
    if (cp.dx < -28) m |= 0x0400u;
    if (cp.dx > 28) m |= 0x0800u;
    return m;
}


// Hold L+R at launch for a physical-device top-screen bisect. These are
// diagnostic colors, not a gameplay screenshot or a replacement renderer.
void top_screen_cpu_probe() {
    consoleClear();std::printf("LEGENDS3DS " MML_CHECKPOINT_VERSION "\nTOP SCREEN TEST 1/2\n\nAutomatic display diagnostic.\nTop should show red/green/blue.\nA: continue to GPU test\nSTART: exit\n");
    gfxFlushBuffers();
    while(aptMainLoop()){
        hidScanInput();const auto keys=hidKeysDown();
        if(keys&KEY_START)throw std::runtime_error("top screen CPU test stopped");
        if(keys&KEY_A)break;
        u16 width=0,height=0;auto* fb=gfxGetFramebuffer(GFX_TOP,GFX_LEFT,&width,&height);
        if(!fb||!width||!height)throw std::runtime_error("top screen CPU framebuffer unavailable");
        const size_t pixels=size_t(width)*height;
        for(size_t i=0;i<pixels;++i){const size_t band=(i/width)*3u/height;
            fb[3*i+0]=band==2?255:0;fb[3*i+1]=band==1?255:0;fb[3*i+2]=band==0?255:0;}
        if(R_FAILED(GSPGPU_FlushDataCache(fb,pixels*3u)))throw std::runtime_error("top CPU cache flush failed");
        gfxScreenSwapBuffers(GFX_TOP,false);gspWaitForVBlank();
    }
}
void top_screen_gpu_probe() {
    consoleClear();std::printf("LEGENDS3DS " MML_CHECKPOINT_VERSION "\nTOP SCREEN TEST 2/2\n\nTop should be bright cyan.\nNo geometry/shader/fade involved.\nA: selected renderer\nX: safe0143 | B: textures off\nLEFT: UV terrain | RIGHT: UV player\nSTART: exit\n");
    gfxFlushBuffers();
    while(aptMainLoop()){
        hidScanInput();const auto keys=hidKeysDown();
        if(keys&KEY_START)throw std::runtime_error("top screen GPU test stopped");
        if(keys&KEY_DLEFT){mml_renderer_gate_set(&g_renderer_gate,MML_RENDER_BATCHING_TEXTURES_OFF);g_hardware_scene_draw=HardwareSceneDraw::TerrainOnly;break;}
        if(keys&KEY_DRIGHT){mml_renderer_gate_set(&g_renderer_gate,MML_RENDER_BATCHING_TEXTURES_OFF);g_hardware_scene_draw=HardwareSceneDraw::PlayerOnly;break;}
        if(keys&KEY_X){mml_renderer_gate_set(&g_renderer_gate,MML_RENDER_0143_FALLBACK);break;}
        if(keys&KEY_B){mml_renderer_gate_set(&g_renderer_gate,MML_RENDER_BATCHING_TEXTURES_OFF);break;}
        if(keys&KEY_A)break;
        (void)acquire_gpu_frame("cyan clear");
        C3D_RenderTargetClear(g_top_target,C3D_CLEAR_ALL,0x00FFFFFFu,0);
        // Clear/transfer only: do not bind unused 3D framebuffer state.
        // Citro3D1.7.1 exposes used on its public render-target structure.
        g_top_target->used=true;
        submit_gpu_frame();
    }
    consoleClear();
}
void init_gpu_target(bool& gpu_ready) {
    if (!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE)) throw std::runtime_error("C3D_Init failed");
    gpu_ready=true;
    boot_log(MML_BOOT_C3D, "citro3d");

    g_top_target = C3D_RenderTargetCreate(240,400,GPU_RB_RGBA8,GPU_RB_DEPTH24_STENCIL8);
    if (!g_top_target) throw std::runtime_error("top render target allocation failed");
    C3D_RenderTargetSetOutput(g_top_target,GFX_TOP,GFX_LEFT,DISPLAY_TRANSFER_FLAGS);
}
void init_gpu(bool& gpu_ready) {
    if(!gpu_ready)init_gpu_target(gpu_ready);
    (void)acquire_gpu_frame("shader and resource initialization",false);
    const bool uv = use_uv_renderer();
    const u8* shader_bytes = uv ? vshader_uv_shbin : vshader0143_shbin;
    const u32 shader_size = uv ? vshader_uv_shbin_size : vshader0143_shbin_size;
    g_shader_dvlb = DVLB_ParseFile(const_cast<u32*>(reinterpret_cast<const u32*>(shader_bytes)), shader_size);
    if (!g_shader_dvlb) throw std::runtime_error("shader parse failed");
    if (R_FAILED(shaderProgramInit(&g_shader_program))) throw std::runtime_error("shader program initialization failed");
    g_shader_program_ready=true;
    if (R_FAILED(shaderProgramSetVsh(&g_shader_program,&g_shader_dvlb->DVLE[0]))) throw std::runtime_error("vertex shader initialization failed");
    C3D_BindProgram(&g_shader_program);
    boot_log(MML_BOOT_SHADER, uv ? "uv shader" : "0143 shader");

    g_u_projection = shaderInstanceGetUniformLocation(g_shader_program.vertexShader,"projection");
    g_u_model_view = shaderInstanceGetUniformLocation(g_shader_program.vertexShader,"modelView");
    if(g_u_projection<0||g_u_model_view<0)throw std::runtime_error("required shader matrix uniform missing");
    hardware_test_trace("# native_shader_uniforms,projection=%d,model_view=%d,renderer=%u",
        g_u_projection,g_u_model_view,unsigned(g_renderer_gate.active));
    Mtx_PerspTilt(&g_projection,C3D_AngleFromDegrees(g_view_settings.vertical_fov_degrees),l3::kFrontendAspect,l3::kFrontendNear,l3::kFrontendFar,false);
    C3D_CullFace(GPU_CULL_NONE);
    C3D_DepthTest(true,GPU_GEQUAL,GPU_WRITE_ALL);
    if (alpha_requested()) C3D_AlphaTest(true, GPU_GREATER, 0);
    if (uv) configure_texenv_uv(false); else configure_texenv_0143();
}

void shutdown_gpu() {
    // Submit any partial frame before draining it. The following acquisition
    // waits for previous draw/transfer completion before a single free occurs.
    if(g_gpu_frame_open)submit_gpu_frame();
    (void)acquire_gpu_frame("GPU resource shutdown",false);
    // This acquired frame has no target, state commands or draw. Close the
    // CPU frame without claiming a new GPU resource lease.
    submit_gpu_frame(false);
    release_all_render_resources();
    if (g_shader_program_ready) shaderProgramFree(&g_shader_program);
    g_shader_program_ready=false;
    if (g_shader_dvlb) DVLB_Free(g_shader_dvlb);
    g_shader_dvlb=nullptr;
    if (g_top_target) C3D_RenderTargetDelete(g_top_target);
    g_top_target=nullptr;
    C3D_Fini();
}

void wait_for_exit_on_error(const std::string& message) {
    std::printf("\x1b[0;0HLegends3DS " MML_CHECKPOINT_VERSION " ERROR\n\n%s\n\nRenderer: %s\nBoot stage: %u\nError code: 0x%08lX\n\nExpected cooked data at:\n%s\n\nPress START to exit.\n",
                message.c_str(), renderer_stage_name(g_renderer_gate.active),
                unsigned(g_boot_diag.stage), (unsigned long)g_boot_diag.error_code, kDataRoot);
    // Error reporting must remain responsive even if a display clock stopped.
    while (aptMainLoop()) { hidScanInput(); if (hidKeysDown() & KEY_START) break; gfxFlushBuffers(); svcSleepThread(1000000); }
}

std::string area_materials_path(size_t area, bool player) {
    char rel[96];
    std::snprintf(rel, sizeof(rel), player ? "materials/player_area_%02lu.l3mc" : "materials/area_%02lu.l3mc",
                  (unsigned long)area);
    return data_path(rel);
}

l3::MaterialCache load_area_materials(size_t area, bool player, uint64_t remaining,
                                     uint64_t& resident) {
    const auto bytes=read_binary(area_materials_path(area,player));
    resident=l3::preflight_area_material_cache(bytes,remaining);
    return l3::load_l3material_cache(bytes,false);
}

void rebuild_actor_materials(size_t area,uint64_t remaining) {
    release_actor_materials();
    if(!actor_textures_requested()||!g_sidecars_bound)return;
    try{
        char rel[96];std::snprintf(rel,sizeof(rel),"materials/actor_area_%02lu.l3mc",static_cast<unsigned long>(area));
        const auto bytes=read_binary(data_path(rel));
        const auto cache=l3::load_actor_material_cache(bytes,std::min(remaining,l3::kActorMaterialDecodedLimit));
        g_actor_materials.reserve(cache.entries.size());
        for(const auto& e:cache.entries){
            if(!e.resolved)continue;
            GpuMaterial m;m.material_word=e.material_word;g_actor_materials.push_back(m);
            auto& owned=g_actor_materials.back();owned.texture_ready=upload_material_texture(&e,owned.texture);
        }
        char label[200];std::snprintf(label,sizeof(label),
            "actor_materials area=%lu pages=%lu gpu_bytes=%lu cache_decoded_policy=1048576 allocator_overhead=EXCLUDED shade=UNKNOWN alpha=UNKNOWN",
            static_cast<unsigned long>(area),static_cast<unsigned long>(g_actor_materials.size()),
            static_cast<unsigned long>(g_actor_materials.size()*kTexturePageBytes));
        boot_log(MML_BOOT_DATA,label);
    }catch(const std::exception& error){
        release_actor_materials();boot_log(MML_BOOT_DATA,error.what());
        boot_log(MML_BOOT_DATA,"actor texture optional cache rejected; flat gray fallback retained");
    }
}

void preflight_current_terrain(const l3::RuntimeMap& map,
                               const l3::St00RenderSource& render, size_t area) {
    validate_area_vertex_budget(map,render,area);
    if (!use_uv_renderer() || !use_material_batches()) return;
    uint64_t resident=0, pages=0;
    for (const bool player : {false,true}) {
        if (player ? !player_textures_requested() : !terrain_textures_requested()) continue;
        // Keep only one bounded sidecar input buffer during admission. The
        // outgoing decoded caches stay intact until all sidecars are admitted.
        const auto bytes=read_binary(area_materials_path(area,player));
        resident+=l3::preflight_area_material_cache(bytes,kAreaMaterialDecodedLimit-resident);
        const auto h=l3::inspect_l3material_cache(bytes,false);
        pages+=(player || all_terrain_textures_requested()) ? h.resolved_count : std::min<uint32_t>(h.resolved_count,1u);
        if (pages>kAreaTextureLimit/kTexturePageBytes)
            throw std::runtime_error("area combined terrain/player texture policy exceeded");
    }
}

void rebuild_current_terrain(const l3::RuntimeMap& map,
                             const l3::St00RenderSource& render,
                             l3::MaterialCache* terrain_cache,
                             l3::MaterialCache* player_cache,
                             size_t area) {
    preflight_current_terrain(map,render,area);
    // Drop the outgoing GPU and CPU material residency before decoding the next
    // area's caches. The map/render package remains shared until package unload.
    release_all_render_resources();
    *terrain_cache={};
    *player_cache={};
    if (!use_uv_renderer()) {
        rebuild_terrain_0143(map, render, area);
        return;
    }
    if (!use_material_batches()) {
        rebuild_terrain_uv_flat(map, render, area);
        rebuild_actor_materials(area,kAreaMaterialDecodedLimit);
        return;
    }

    uint64_t terrain_resident=0, player_resident=0;
    if (terrain_textures_requested()) {
        *terrain_cache = load_area_materials(area,false,kAreaMaterialDecodedLimit,terrain_resident);
        rebuild_terrain_batches(map, render, terrain_cache, area);
        if (g_texture_count) boot_log(MML_BOOT_FIRST_TEXTURE, "terrain texture upload");
    } else {
        rebuild_terrain_batches(map, render, nullptr, area);
    }

    if (player_textures_requested()) {
        *player_cache = load_area_materials(area,true,kAreaMaterialDecodedLimit-terrain_resident,player_resident);
        rebuild_player_materials(player_cache);
    }
    rebuild_actor_materials(area,kAreaMaterialDecodedLimit-terrain_resident-player_resident);
}

} // namespace

int main() {
    gfxInitDefault();
    gfxSet3D(false); // Only the left eye is rendered; explicitly own 2D output.
    gfxSetScreenFormat(GFX_TOP,GSP_BGR8_OES);
    gfxSetDoubleBuffering(GFX_TOP,true);
    consoleInit(GFX_BOTTOM,nullptr);
    mml_renderer_gate_init(&g_renderer_gate);
    mml_boot_diag_init(&g_boot_diag, __stacksize__);
    mml_diag_init(&g_runtime_diag, 0u, __stacksize__); // no invented frame-time pass threshold
    reset_boot_log();
    boot_log(MML_BOOT_GFX, "gfx + bottom console");

    hidScanInput();
    load_diagnostic_autoplay();
    if(g_autoplay.active)boot_log(MML_BOOT_GFX,"DIAGNOSTIC_AUTOPLAY=1 scripted input; display probes skipped");
    const bool top_screen_probe=!g_autoplay.active&&(MML_CHECKPOINT_DISPLAY_TEST ||
        (hidKeysHeld()&(KEY_L|KEY_R))==(KEY_L|KEY_R));
    mml_renderer_gate_set(&g_renderer_gate, choose_boot_stage(hidKeysHeld()));
    // Optional independent diagnostic gates. SELECT always overrides this file.
    if (!(hidKeysHeld() & KEY_SELECT)) {
        if (FILE* flags = std::fopen("sdmc:/3ds/Legends3DS/renderer_flags.txt", "r")) {
            unsigned mask = 0; char trailing = 0;
            const int fields = std::fscanf(flags, "%x %c", &mask, &trailing);
            std::fclose(flags);
            if (fields != 1 || !mml_renderer_gate_mask(&g_renderer_gate, mask))
                mml_renderer_gate_set(&g_renderer_gate, MML_RENDER_0143_FALLBACK);
        }
    }
    if(!MML_CHECKPOINT_DISPLAY_TEST &&
       !hardware_test_launch(g_renderer_gate,g_terrain_xyz_candidate)){gfxExit();return 0;}
    load_gameplay_profile();
    load_actor_animation_gate();
    load_actor_state_gate();
    load_actor_rng_candidate_gate();
    load_actor_texture_gate();
    hardware_test_begin(terrain_xyz_active());
    std::printf("\x1b[0;0HLEGENDS3DS " MML_CHECKPOINT_VERSION "\n\nRenderer: %s\n\nBoot bisect keys:\nSELECT 0143 fallback | L logging\nR UV | Left batching | Up one tex\nRight all terrain | X player tex\nY alpha | no key full animation\n\nLoading...\n",
                renderer_stage_name(g_renderer_gate.active));
    boot_log(MML_BOOT_GFX, "renderer gate selected");

    bool gpu_ready=false;
    try {
        // Run before any game package or shader admission: the display tests
        // remain usable even with no cooked assets installed.
        if(top_screen_probe){
            boot_log(MML_BOOT_GFX,"automatic CPU top-screen test");
            top_screen_cpu_probe();
            init_gpu_target(gpu_ready);
            top_screen_gpu_probe();
            boot_log(MML_BOOT_C3D,"GPU clear test completed by user");
        }
        // The probe may select X/B after startup flags were sampled. Bind
        // the matching camera only once that final renderer choice is owned.
        load_view_calibration();
        hardware_test_trace("# top_draw_plan,mode=%u,renderer=%u,0=all,1=terrain_only,2=player_only,empty_immediate_batches=SKIPPED",
            unsigned(g_hardware_scene_draw),unsigned(g_renderer_gate.active));
        l3::RuntimeMap boot_map;
        l3::St00RenderSource render;
        bool package_loaded = false;
        std::vector<l3::TutorialRuntimeArea> packaged_areas;
        std::optional<l3::St00SpawnCatalog> spawn_catalog;
        if (FILE* present = std::fopen(kAreaPackagePath, "rb")) {
            std::fclose(present);
            const u64 load_start = svcGetSystemTick();
            hardware_test_area_event("package_before", 0, 0, 0, 0, 0);
            try {
                l3::AreaPackageLimits limits;
                limits.max_file_bytes = 8u * 1024u * 1024u;
                limits.max_section_bytes = kRuntimeFileLimit;
                limits.max_decoded_bytes = 4u * 1024u * 1024u;
                limits.max_load_peak_bytes = 8u * 1024u * 1024u;
                limits.max_map_areas = 256;
                auto package = l3::load_area_package(kAreaPackagePath, limits);
                // Admit every expansion before committing. Empty structural
                // areas are skipped by diagnostic selection, not a bad package.
                bool has_terrain=false;
                for (size_t i=0;i<package.map.areas.size();++i)
                    has_terrain=validate_area_vertex_budget(package.map,package.render,i)!=0 || has_terrain;
                if (!has_terrain) throw std::runtime_error("package has no bound terrain");
                g_package_bytes = package.diagnostics.file_bytes;
                g_package_decoded_estimate = package.diagnostics.decoded_estimated_bytes;
                g_package_peak_estimate = package.diagnostics.load_peak_estimated_bytes;
                // Isolated legacy adapter: only the exact already-cooked map/render
                // pair may use the tutorial's independently supplied flow/materials.
                g_sidecars_bound = section_matches_file(package, 2, data_path("st00.l3map")) &&
                                   section_matches_file(package, 3, data_path("st00.l3render"));
                // 0.0.15.5: the generalized boot package has no section35. With
                // byte-identical sidecar map/render, apply the sidecar's original
                // startup initialization, exactly as the loose path below does.
                // Through 0.0.15.4 the package ran Area 1 without its 994 writes
                // (host tests always applied them): the floor before the first
                // door was absent and Mega Man fell out of the world.
                if (g_sidecars_bound && !package.diagnostics.map_initialization_writes) {
                    const auto initialization_path=data_path("st00.l3init");
                    if(FILE* input=std::fopen(initialization_path.c_str(),"rb")) {
                        std::fclose(input);
                        try {
                            const auto initialization=l3::load_map_initialization(
                                read_binary(initialization_path,l3::kMapInitializationMaxFileBytes),true);
                            if(!l3::apply_sidecar_map_initialization(package,initialization))
                                boot_log(MML_BOOT_DATA,"sidecar map initialization refused; package geometry retained");
                        } catch(const std::exception& error) {
                            boot_log(MML_BOOT_DATA,error.what());
                            boot_log(MML_BOOT_DATA,"sidecar map initialization UNKNOWN; package geometry retained");
                        }
                    } else boot_log(MML_BOOT_DATA,"sidecar map initialization absent UNKNOWN");
                }
                char initialization_message[112];
                std::snprintf(initialization_message,sizeof(initialization_message),
                    "map initialization profile=%lu writes=%lu temporary=%llu; absent=UNKNOWN",
                    static_cast<unsigned long>(package.diagnostics.map_initialization_profile),
                    static_cast<unsigned long>(package.diagnostics.map_initialization_writes),
                    static_cast<unsigned long long>(package.diagnostics.map_initialization_temporary_bytes));
                boot_log(MML_BOOT_DATA,initialization_message);
                boot_map = std::move(package.map);
                render = std::move(package.render);
                g_negative_materials = std::move(package.negative_materials);
                packaged_areas = std::move(package.runtime_areas);
                spawn_catalog=std::move(package.spawns);
                package.unload();
                package_loaded = true;
                g_content_source = "PACKAGE";
                hardware_test_area_event("package_loaded", 0,
                    (svcGetSystemTick() - load_start) / CPU_TICKS_PER_USEC,
                    g_package_bytes, g_package_decoded_estimate, g_package_peak_estimate);
                boot_log(MML_BOOT_DATA, g_sidecars_bound ? "package map/render; exact local sidecars" :
                         "package map/render; other bindings UNKNOWN");
            } catch (const std::exception& error) {
                // An invalid optional package never blocks the trusted loose boot.
                boot_log(MML_BOOT_DATA, error.what());
                hardware_test_area_event("package_rejected", 0,
                    (svcGetSystemTick() - load_start) / CPU_TICKS_PER_USEC, 0, 0, 0);
                g_sidecars_bound = true;
                g_package_bytes = g_package_decoded_estimate = g_package_peak_estimate = 0;
            }
        }
        if (!package_loaded) {
            const auto map_bytes=read_binary(data_path("st00.l3map"));
            const auto render_bytes=read_binary(data_path("st00.l3render"));
            boot_map = l3::load_l3map(map_bytes, true);
            render = l3::load_l3render(render_bytes, true);
            const auto initialization_path=data_path("st00.l3init");
            if(FILE* input=std::fopen(initialization_path.c_str(),"rb")) {
                const bool seek_ok=std::fseek(input,0,SEEK_END)==0;
                const long length=seek_ok?std::ftell(input):-1;
                std::fclose(input);
                try {
                    if(length<0||static_cast<uint64_t>(length)>l3::kMapInitializationMaxFileBytes)
                        throw std::runtime_error("loose map initialization exceeds finite cap");
                    const auto initialization=l3::load_map_initialization(read_binary(initialization_path,l3::kMapInitializationMaxFileBytes),true);
                    const auto applied=l3::apply_map_initialization(boot_map,render,initialization,
                        l3::crc32(map_bytes.data(),map_bytes.size()),l3::crc32(render_bytes.data(),render_bytes.size()));
                    char message[80];
                    std::snprintf(message,sizeof(message),"loose map initialization writes=%lu",
                        static_cast<unsigned long>(applied.write_count));
                    boot_log(MML_BOOT_DATA,message);
                } catch(const std::exception& error) {
                    boot_log(MML_BOOT_DATA,error.what());
                    boot_log(MML_BOOT_DATA,"loose map initialization UNKNOWN; safe geometry retained");
                }
            } else boot_log(MML_BOOT_DATA,"loose map initialization absent UNKNOWN");
        }
        if (terrain_xyz_active() && g_sidecars_bound && g_negative_materials.empty()) {
            const auto path = data_path("st00.l3nm");
            if (FILE* f = std::fopen(path.c_str(), "rb")) {
                std::fclose(f);
                try {
                    auto bindings=l3::load_l3negative_materials(read_binary(path),true);
                    if (bindings.size()!=boot_map.areas.size()) throw std::runtime_error("negative material/map area count mismatch");
                    g_negative_materials=std::move(bindings);
                } catch (const std::exception& error) {
                    boot_log(MML_BOOT_DATA,error.what());
                    // Optional unknown textures never prevent geometry or safe boot.
                }
            }
        }
        if (!g_sidecars_bound) {
            // A different geometry package does not inherit tutorial palettes or
            // event bindings merely because it reuses the same material words.
            const uint32_t unbound_mask = g_renderer_gate.feature_mask &
                ~(MML_RF_TERRAIN | MML_RF_ALL_TERRAIN | MML_RF_PLAYER | MML_RF_ALPHA);
            if (!mml_renderer_gate_mask(&g_renderer_gate, unbound_mask))
                mml_renderer_gate_set(&g_renderer_gate, MML_RENDER_0143_FALLBACK);
        }
        init_gpu(gpu_ready);

        const auto refs=l3::load_l3reference(read_binary(data_path("reference.l3ref")),true);
        const auto actor_metadata=load_actor_source_metadata();
        // GTE projection for the player lease/lock-on path: InitGeom 0x8007A4F0
        // then 0x80011F48 (OFX/OFY 160/120, H 384). Those are the only EXE
        // writers of OFX/OFY/H (none in ST00). DQA/DQB keep the InitGeom
        // values; later fog writers only change IR0, which 0x80034EB8 ignores.
        if(actor_metadata.scene_view) {
            l3::SourceProjectionControls player_projection;
            l3::source_projection_init_geom_8007a4f0(player_projection);
            (void)l3::initialize_source_scene_projection_80011f48(player_projection,*actor_metadata.scene_view);
            hardware_test_trace("# source_player_projection,bound=%u,owner=8007A4F0+80011F48,dq=INITGEOM",
                unsigned(l3::bind_source_projection_controls(g_source_player_gte,player_projection)));
        }
        std::optional<l3::SourceAngleSteps> actor_angle_steps;
        if (g_sidecars_bound) {
            const auto path = data_path("actor_angle.l3ref");
            if (FILE* file = std::fopen(path.c_str(), "rb")) {
                // A fixed 81-byte read bounds both malformed input and load
                // memory. This optional data never blocks the trusted boot.
                std::array<uint8_t, l3::kActorAnglePackageBytes + 1> bytes{};
                const size_t count = std::fread(bytes.data(), 1, bytes.size(), file);
                const bool read_failed = std::ferror(file) != 0;
                std::fclose(file);
                try {
                    if (read_failed) throw std::runtime_error("actor angle read failed");
                    actor_angle_steps = l3::load_l3actor_angle(bytes.data(), count);
                    boot_log(MML_BOOT_DATA, "actor angle query data CRC valid; full AI/lock-on unbound");
                } catch (const std::exception& error) {
                    boot_log(MML_BOOT_DATA, error.what());
                }
            } else boot_log(MML_BOOT_DATA, "actor angle data absent; non-axis heading UNKNOWN");
        }
        std::vector<l3::MmlMessageFile> messages;
        std::vector<l3::TutorialRuntimeArea> areas;
        if (!packaged_areas.empty()) {
            areas.resize(boot_map.areas.size());
            for (size_t i=0;i<areas.size();++i) areas[i].area_index=i;
            for (auto& area:packaged_areas) areas.at(area.area_index)=std::move(area);
            std::vector<l3::TutorialRuntimeArea>().swap(packaged_areas);
            boot_log(MML_BOOT_DATA,"package runtime table/list/EBD bound");
        } else if (g_sidecars_bound) {
            const auto flow=read_binary(data_path("st00.l3flow"));
            for(size_t i=0;i<kTutorialAreaCount;++i){
                char rel[64];
                std::snprintf(rel,sizeof(rel),"ebd/area_%02lu.l3ebd",(unsigned long)i);
                areas.push_back(l3::load_tutorial_runtime_area(flow,read_binary(data_path(rel)),i,true));
            }
        } else {
            // No guessed table/list/EBD selection: these areas bind map data only.
            areas.resize(boot_map.areas.size());
            for (size_t i=0;i<areas.size();++i) areas[i].area_index=i;
        }
        if(g_sidecars_bound && !spawn_catalog){
            const auto path=data_path("st00.l3spawn");
            if(FILE* present=std::fopen(path.c_str(),"rb")){
                std::fclose(present);
                try {spawn_catalog=l3::load_l3st00_spawn(read_binary(path),true);}
                catch(const std::exception& error){boot_log(MML_BOOT_DATA,error.what());}
            }
        }
        if(spawn_catalog && areas.size()==l3::kSt00SpawnAreaCount){
            for(size_t i=0;i<areas.size();++i)areas[i].conditional_spawns=std::move(spawn_catalog->areas[i]);
            spawn_catalog.reset();
            boot_log(MML_BOOT_DATA,"conditional source spawn catalog bound");
        }
        if (g_sidecars_bound) {
            messages.push_back(l3::load_l3message(read_binary(data_path("messages/st00_01.l3msg")),true));
            messages.push_back(l3::load_l3message(read_binary(data_path("messages/st00_02.l3msg")),true));
        }
        if (areas.empty() || areas.size()!=boot_map.areas.size())
            throw std::runtime_error("area binding count differs from loaded map");
        boot_log(MML_BOOT_DATA, g_sidecars_bound ? "map/render/ref/flow/messages/ebd bound" : "map/render bound; gameplay records unbound");
        size_t ebd_source_bytes=0, ebd_payload_areas=0;
        for(const auto& area:areas) {
            ebd_source_bytes+=area.ebd.source_payload.size();
            if(!area.ebd.source_payload.empty())++ebd_payload_areas;
        }
        char ebd_label[128];
        std::snprintf(ebd_label,sizeof(ebd_label),
            "EBD retained source bytes=%lu payload areas=%lu class20 actor resource subset bound",
            (unsigned long)ebd_source_bytes,(unsigned long)ebd_payload_areas);
        boot_log(MML_BOOT_DATA,ebd_label);

        const std::array<const char*,l3::kPlayerComponentSlots> model_names{{
            "body.l3model","head.l3model","right.l3model","left.l3model","feet.l3model"}};
        std::array<l3::PbdModel,l3::kPlayerComponentSlots> models{};
        for(size_t i=0;i<models.size();++i)
            models[i]=l3::load_l3model(read_binary(data_path((std::string("player/")+model_names[i]).c_str())),true);
        const auto composition=l3::load_l3player_composition(read_binary(data_path("player/player.l3pc")),true);
        const auto animation=l3::load_l3animation(read_binary(data_path("player/main.l3anim")),true);
        const auto skeleton=l3::load_l3skeleton(read_binary(data_path("player/main.l3skel")),true);
        const auto source_buster_profile=l3::load_source_buster_profile(
            read_binary(data_path("player/buster.l3bp")),true);
        l3::SourceBusterPlayer source_buster_player;
        // Tutorial loadout via the 0x8003BE6C restore branch: +170 = 0,
        // +171 = 1 (MEASURED in PCSX-Redux at first tutorial control).
        l3::initialize_source_buster_lanes_8003be6c(
            source_buster_profile,source_buster_player,0,1);
        // 0.0.15.12 enemy-reward pickups (0x80046B34 / subtype 0x87). Optional
        // owner-local L3RW tables; without them defeat rewards stay queued.
        l3::SourceRewardProfile source_reward_profile;
        {
            const auto path=data_path("reward.l3rw");
            if(FILE* present=std::fopen(path.c_str(),"rb")){
                std::fclose(present);
                if(!l3::load_source_reward_profile(read_binary(path),source_reward_profile))
                    boot_log(MML_BOOT_DATA,"reward.l3rw rejected; rewards held");
                else boot_log(MML_BOOT_DATA,"reward pickups 0x80046B34/0x8004602C bound (L3RW)");
            } else boot_log(MML_BOOT_DATA,"reward.l3rw absent; rewards held");
        }
        // Original global80098798 owns the shared24-slot projection leases.
        // The inherited native frontend has already reserved its low8 bits;
        // retain this owner across frames and reset it with the child pools.
        std::optional<uint32_t> source_buster_proxy_mask=0xffu;
        auto local_visuals=l3::build_player_visuals(models,composition,nullptr);
        // Retain only the source head-part UV templates. Actual native draw
        // primitives are reset when the original phase-7 caller requests it.
        const auto head_uv_baseline=g_source_event_boot?l3::capture_source_event_head_uv_baseline(
            models[static_cast<size_t>(l3::PlayerComponentSlot::Head)],local_visuals):
            std::optional<l3::SourceEventHeadUvBaseline>{};
        if(g_source_event_boot)hardware_test_trace("# source_head_uv_owner,admitted=%u,records=%lu,full_face_animation=false",
            unsigned(head_uv_baseline.has_value()),(unsigned long)(head_uv_baseline?head_uv_baseline->record_count():0));
        const auto player_material_words=l3::collect_player_visual_material_words(local_visuals);
        std::vector<l3::PosedPlayerVisualPrimitive> player_pose_storage(local_visuals.size());
        const auto& fallback_action=choose_probe_action(animation);
        boot_log(MML_BOOT_PLAYER, "player model/composition/animation/skeleton");

        L3QolSettings qol{}; l3_qol_settings_defaults(&qol);
        l3::RuntimeSession session(std::move(boot_map),std::move(areas[0]),std::move(messages),qol);
        // This native entry is a fresh diagnostic session, not a restored retail
        // save. The original fresh initializer proves selector1 only here.
        session.initialize_fresh_source_statistics_8001326c();
        session.set_reference_tables(refs);
        // 0.0.15.15: ST00 E0/06 tutorial message triggers (owner-local L3TR).
        if(g_sidecars_bound){
            const auto path=data_path("st00.l3tr");
            if(FILE* present=std::fopen(path.c_str(),"rb")){
                std::fclose(present);
                try{
                    session.bind_tutorial_triggers(std::make_shared<const l3::TutorialTriggerCatalog>(
                        l3::load_l3tutorial_triggers(read_binary(path),true)));
                    boot_log(MML_BOOT_DATA,"tutorial triggers E0/06 0x80114424 bound (L3TR)");
                }catch(const std::exception& error){boot_log(MML_BOOT_DATA,error.what());}
            }
        }
        // 0.0.15.0: one combat owner. When the source actor/Buster path is active
        // (enhanced renderer + ACTOR_STATE_FIELDS + SOURCE_ACTOR_PROXY) the
        // historical candidate projectile and candidate lock-on are retired. The
        // 0143 safe path keeps the historical candidate behavior unchanged.
        const bool source_combat_active=g_source_actor_proxy&&actor_state_fields_requested();
        session.set_candidate_combat_enabled(!source_combat_active);
        boot_log(MML_BOOT_DATA,source_combat_active
            ?"COMBAT_OWNER=SOURCE Buster80035724/800433C0 hit/defeat; candidate projectile+lock-on retired; lock-on80034EB8 camera owner UNKNOWN"
            :"COMBAT_OWNER=CANDIDATE historical projectile path (source combat inactive)");
        for(size_t i=1;i<areas.size();++i) session.register_area(i,std::move(areas[i]));
        std::vector<l3::TutorialRuntimeArea>().swap(areas);
        const auto& map = session.active_map();
        g_current_area=next_diagnostic_area(map,render,0);
        auto spawn=first_positive_cell_center(map,g_current_area);
        uint16_t inspection_yaw=0;
        // 0.0.15.0 M2: without the source opening scene, begin where the
        // original first gives Mega Man to the player (0x80101274 handoff:
        // Area 1). The earlier first-positive-cell diagnostic spawn placed
        // him inside a solid Area 0 block, which the contact solver could
        // not resolve and which led to the reported map-edge escape.
        // 0.0.15.4: the handoff must precede the terrain-XYZ inspection entry.
        // Through 0.0.15.3 it was an else-branch of that default-on renderer
        // path, so the enhanced build never reached it and booted into the
        // Area 0 inspection camera instead.
        bool handoff_spawn=false;
        if(!g_source_event_boot) {
            constexpr auto& handoff=l3::kSourceSt00OpeningHandoff;
            if(handoff.destination_area<map.areas.size()&&
               validate_area_vertex_budget(map,render,handoff.destination_area)) {
                g_current_area=handoff.destination_area;
                spawn={handoff.x,handoff.y,handoff.z};inspection_yaw=handoff.heading;handoff_spawn=true;
                boot_log(MML_BOOT_DATA,"BOOT_SPAWN=SOURCE_HANDOFF_80101274 area1 opening_scene=SKIPPED");
            } else boot_log(MML_BOOT_DATA,"BOOT_SPAWN=DIAGNOSTIC handoff area unavailable");
        }
        if (!handoff_spawn && terrain_xyz_active()) { const auto entry=inspection_entry(map,render,g_current_area); spawn={entry.origin.x,entry.origin.y,entry.origin.z}; inspection_yaw=entry.yaw; }
        load_diagnostic_spawn(map,render,g_current_area,spawn,inspection_yaw);
        if (g_current_area) session.transition_to_area(g_current_area,spawn,inspection_yaw);
        session.reset_player(spawn,inspection_yaw,100,100);
        auto player_track=l3::pbd_main_body_track_begin(animation,
            session.compat_player().bytes[0xAC], session.compat_player().bytes[0xA4]);

        l3::MaterialCache terrain_materials{};
        l3::MaterialCache player_materials{};
        set_e007_gate_cells(session.active_area(), map, g_current_area);
        rebuild_current_terrain(map, render, &terrain_materials, &player_materials, g_current_area);
        if(actor_metadata.event_poll)session.bind_source_event_profile(*actor_metadata.event_poll,0);
        if(actor_metadata.event_phase2)session.bind_source_event_phase2_package(*actor_metadata.event_phase2);
        if(actor_metadata.event_command)session.bind_source_event_command_package(*actor_metadata.event_command);
        if(actor_metadata.fade_startup)session.bind_source_fade_profile(*actor_metadata.fade_startup);
        if(actor_metadata.event_phase5) {
            session.bind_source_event_phase5_profile(*actor_metadata.event_phase5);
            const auto admitted=session.bind_source_event_phase5_resources();
            hardware_test_trace("# source_scene_resources,status=%u,admitted=%u,binding_count=%lu,resident_EBD_SHA256_verified=%u",
                unsigned(admitted),unsigned(session.source_event_phase5_resources().admitted()),
                (unsigned long)session.source_event_phase5_resources().binding_count(),
                unsigned(session.source_event_phase5_resources().admitted()));
        }
        if(actor_metadata.event_phase6) {
            const bool admitted=session.bind_source_event_phase6_profile(*actor_metadata.event_phase6);
            hardware_test_trace("# source_scene_profile,format=L3P6,admitted=%u,bytes=1180,full_scene_complete=false",unsigned(admitted));
        }
        if(g_source_event_boot&&actor_metadata.prop_lift) {
            const bool admitted=session.bind_source_prop_lift_profile(*actor_metadata.prop_lift);
            hardware_test_trace("# source_prop_lift_profile,format=L3PL,bytes=384,admitted=%u,rng_owner=UNKNOWN",unsigned(admitted));
        }
        if(g_source_event_boot&&actor_metadata.effect_dispatch) {
            const bool admitted=session.bind_source_effect_dispatch_profile(*actor_metadata.effect_dispatch);
            hardware_test_trace("# source_lower80_profile,format=L3E2,bytes=384,admitted=%u,table_owner=EXPLICIT_OVERLAY_LEASE,rng_owner=UNKNOWN",
                unsigned(admitted));
        }
        std::optional<l3::SceneE0Resources> scene_e0;
        if(g_source_event_boot&&g_source_scene_e0&&use_uv_renderer()&&actor_metadata.event_phase5) {
            try {
                scene_e0=l3::load_scene_e0_resources(session.active_area().ebd,*actor_metadata.event_phase5,
                    session.source_event_phase5_resources(),128u*1024u);
                hardware_test_trace("# source_scene_E0_cache,resources=%lu,decoded_bytes=%lu,budget_bytes=131072,texture=UNKNOWN,render_parity=false",
                    (unsigned long)scene_e0->resource_count(),(unsigned long)scene_e0->decoded_bytes());
            } catch(const std::exception& error) {boot_log(MML_BOOT_DATA,error.what());}
        }
        if(scene_e0&&g_source_scene_textures&&actor_metadata.event_phase5) {
            try {
                const auto words=l3::collect_scene_e0_material_words(*actor_metadata.event_phase5,*scene_e0);
                const auto cache=l3::load_scene_e0_material_snapshot(read_binary(data_path("materials/scene_e0_initial.l3sm"),
                    l3::kSceneE0MaterialDecodedLimit+4096),actor_metadata.event_phase5->ebd_source_digest,words);
                g_scene_materials.reserve(cache.entries.size());
                size_t held=0;
                for(const auto& e:cache.entries) {
                    if(!e.resolved){++held;continue;}
                    GpuMaterial m;m.material_word=e.material_word;g_scene_materials.push_back(m);
                    auto& owned=g_scene_materials.back();owned.texture_ready=upload_material_texture(&e,owned.texture);
                    if(!owned.texture_ready)++held;
                }
                hardware_test_trace("# source_scene_materials,resolved_pages=%lu,held=%lu,gpu_bytes=%lu,initial_upload_only=true,live_palette=UNKNOWN",
                    (unsigned long)g_scene_materials.size(),(unsigned long)held,(unsigned long)(g_scene_materials.size()*kTexturePageBytes));
            } catch(const std::exception& error){release_scene_materials();boot_log(MML_BOOT_DATA,error.what());}
        }
        unsigned last_source_view_status=UINT32_MAX;
        std::optional<l3::SourceSceneActor40Resources> scene_actor40;
        bool scene_actor40_prefix_ready=false,scene_actor40_pose_ready=false;
        unsigned last_actor40_pose_status=UINT32_MAX;
        unsigned last_actor40_update_status=UINT32_MAX;
        if(g_source_event_boot&&g_source_scene_actor40&&use_uv_renderer()&&actor_metadata.event_phase5) {
            try{scene_actor40=l3::decode_source_scene_actor40_phase5_resources(*actor_metadata.event_phase5,
                session.source_event_phase5_resources(),session.active_area().ebd,512u*1024u);
                // IDs are the unchanged ST00 state0..12 call-site arguments.
                // They name original requests, not inferred animation labels.
                const uint8_t requests[]={16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,34,48};
                try{
                    l3::prepare_source_scene_actor40_request_bank(*scene_actor40,session.active_area().ebd,
                        requests,sizeof(requests),512u*1024u);
                    auto descriptor=scene_actor40->descriptor_copy;
                    descriptor[16]=20;descriptor[28]=21; // original state4 draw-slot stores
                    l3::prepare_source_scene_actor40_mesh_variant(*scene_actor40,session.active_area().ebd,descriptor,512u*1024u);
                }catch(const std::exception& error){boot_log(MML_BOOT_DATA,error.what());}
                hardware_test_trace("# source_actor40_cache,parts=%lu,mesh_bytes=%lu,pose_records=%lu,pose_bytes=%lu,requests=%lu,total_decoded_bytes=%lu,cap=524288,dispatch=8010D824,initializer=8010C930,later_states=SOURCE_BACKED_PARTIAL",
                    (unsigned long)scene_actor40->mesh.model.parts.size(),(unsigned long)scene_actor40->mesh.footprint.decoded_bytes,
                    (unsigned long)scene_actor40->pose_sequence.poses.size(),(unsigned long)scene_actor40->pose_sequence.decoded_bytes,
                    (unsigned long)(scene_actor40->additional_requests.requests.size()+1),
                    (unsigned long)l3::source_scene_actor40_decoded_bytes(*scene_actor40));
            }catch(const std::exception& error){boot_log(MML_BOOT_DATA,error.what());}
        }
        if(scene_actor40&&g_source_scene_textures&&actor_metadata.event_phase5) {
            try {
                std::vector<uint32_t> words;
                for(const auto& binding:scene_actor40->mesh.bindings)words.push_back(binding.material_word);
                std::sort(words.begin(),words.end());words.erase(std::unique(words.begin(),words.end()),words.end());
                const auto cache=l3::load_scene_e0_material_snapshot(read_binary(data_path("materials/scene_actor40_initial.l3sm"),
                    l3::kSceneE0MaterialDecodedLimit+4096),actor_metadata.event_phase5->ebd_source_digest,words);
                g_scene_actor40_materials.reserve(cache.entries.size());size_t held=0;
                for(const auto& entry:cache.entries){if(!entry.resolved){++held;continue;}
                    GpuMaterial material;material.material_word=entry.material_word;g_scene_actor40_materials.push_back(material);
                    auto& owned=g_scene_actor40_materials.back();owned.texture_ready=upload_material_texture(&entry,owned.texture);
                    if(!owned.texture_ready)++held;}
                hardware_test_trace("# source_actor40_materials,pages=%lu,held=%lu,gpu_bytes=%lu,initial_upload_only=true,live_palette=UNKNOWN,shade=UNKNOWN",
                    (unsigned long)g_scene_actor40_materials.size(),(unsigned long)held,(unsigned long)(g_scene_actor40_materials.size()*kTexturePageBytes));
            }catch(const std::exception& error){release_scene_actor40_materials();boot_log(MML_BOOT_DATA,error.what());}
        }
        NativeXaAudio xa_audio;l3::XaNativeCdState xa_transport;
        uint32_t scene_overlay_receipts=0;
        uint32_t last_audio_logged_frame=UINT32_MAX;
        bool source_xa_requested=false;
        if(auto* cfg=std::fopen("sdmc:/3ds/Legends3DS/source_audio.cfg","rb")){
            char data[64]{};const auto n=std::fread(data,1,63,cfg);const bool extra=std::fgetc(cfg)!=EOF;std::fclose(cfg);
            unsigned enabled=0;char trailing=0;
            source_xa_requested=!extra&&!std::memchr(data,0,n)&&std::sscanf(data,"SOURCE_XA_AUDIO=%u %c",&enabled,&trailing)==1&&enabled==1;
        }
        if(source_xa_requested&&actor_metadata.event_command){
            char path[256];std::snprintf(path,sizeof(path),"%saudio/stream_%lu.l3xa",kDataRoot,
                (unsigned long)actor_metadata.event_command->profile.id);
            const auto start=svcGetSystemTick();const bool ready=xa_audio.open(path);
            xa_transport.retained_response=actor_metadata.event_command->cold_context.response_80098a98;
            xa_transport.retained_known=actor_metadata.event_command->cold_context.response_known;
            hardware_test_trace("# source_xa_open,ready=%u,result=%08lx,linear=%lu,index=%lu,load_us=%llu,reason=%s,native_protocol=CANDIDATE,hardware=HARDWARE_PENDING",
                unsigned(ready),(unsigned long)xa_audio.dsp_result(),(unsigned long)xa_audio.linear_bytes(),
                (unsigned long)xa_audio.stream().resident_index_bytes(),(unsigned long long)((svcGetSystemTick()-start)/CPU_TICKS_PER_USEC),xa_audio.error());
        }

        if(g_source_event_boot) {
            const bool fresh=session.initialize_fresh_source_event_subset_st00();
            if(fresh)session.initialize_source_main_loop_pads_from_executable_boot_800177cc();
            hardware_test_trace("# source_event_boot_subset,committed=%u,full_original_boot=false",unsigned(fresh));
            if(fresh&&actor_metadata.scene_view)hardware_test_trace("# source_projection_boot,committed=%u,caller=80011F48,fresh_only=true",
                unsigned(session.initialize_source_scene_projection_from_executable_boot(*actor_metadata.scene_view)));
        }
        if(g_actor_rng_candidate && actor_metadata.rng) {
            const auto status=l3::install_source_rng_service(g_actor_rng,*actor_metadata.rng,l3::SourceRngMode::CandidateOpenBios);
            hardware_test_trace("# source_rng_install,status=%u,scope=%u,warnings=%lu,observed_returns=%lu,other_consumers=%lu,retail_parity=false",
                unsigned(status),unsigned(actor_metadata.rng->scope),(unsigned long)actor_metadata.rng->warnings,
                (unsigned long)actor_metadata.rng->actual_rand_returns,(unsigned long)actor_metadata.rng->observed_consumer_count);
        }
        auto actors=load_active_actors(session,actor_metadata);
        if(g_source_actor_proxy)for(auto& actor:actors.actors)
            (void)l3::initialize_imported_actor_fresh_pose_proxy_subset(actor);
        if(actor_state_fields_requested()){hardware_test_actor_action2_probe(actors,session.active_map(),session.map_area_index(),refs);hardware_test_actor_action4_probe(actors,session.active_map(),session.map_area_index(),refs);}
        l3::ActorReloadRevisionState actor_revisions{session.conditional_spawn_revision(),
            session.conditional_spawn_revision(),std::nullopt};
        mockup_hud_invalidate();
        boot_log(MML_BOOT_TERRAIN_VBO, terrain_xyz_active()?"IDX + GRID XYZ terrain resources built":"legacy terrain resources built");
        hardware_test_geometry_event(terrain_xyz_active(),g_current_area,g_area_primitive_count,g_area_min_y,g_area_max_y);
        l3::RuntimeVec3i last_valid=spawn;
        l3::MockupUiState ui_state{};
        bool select_ui_used=false;
        uint32_t ui_hold_frames=0;
        if(g_source_event_boot&&g_source_loop_replay) {
            try {
                const auto bytes=read_binary(data_path("source_loop_snapshot.l3ls"),l3::kSourceLoopSnapshotBytes);
                l3::SourceLoopSnapshot snapshot;
                const auto status=l3::load_l3source_loop_snapshot(bytes.data(),bytes.size(),snapshot);
                const bool bound=status==l3::SourceLoopSnapshotStatus::Ready&&session.bind_source_loop_snapshot_replay(snapshot);
                hardware_test_trace("# source_loop_replay_profile,status=%u,bound=%u,counter_seeded=false,scope=RECORDED_OPENBIOS_CONSTRUCTOR,retail_chronology=false",unsigned(status),unsigned(bound));
            }catch(const std::exception& error){boot_log(MML_BOOT_DATA,error.what());}
        }
        if(g_source_event_boot&&g_source_scene_rng_replay){
            try{
                const auto bytes=read_binary(data_path("source_scene_rng.l3sr"),l3::kSourceEventRandomTapeHeaderBytes+
                    l3::kSourceEventRandomTapeMaximumRecords*l3::kSourceEventRandomTapeRecordBytes);
                l3::SourceEventRandomTapeProfile profile;
                const auto status=l3::load_l3source_event_random_tape(bytes.data(),bytes.size(),profile);
                const bool bound=status==l3::SourceEventRandomTapeStatus::Ready&&session.bind_source_scene_rng_replay(profile);
                hardware_test_trace("# source_scene_rng_profile,status=%u,bound=%u,records=%lu,scope=RECORDED_OPENBIOS_SCENE,retail_chronology=false",
                    unsigned(status),unsigned(bound),(unsigned long)profile.record_count);
            }catch(const std::exception& error){boot_log(MML_BOOT_DATA,error.what());}
        }
        l3::GameplayClock gameplay_clock;
        u64 previous_update_clock=svcGetSystemTick();
        bool pending_fire=false,pending_lock=false,pending_dialogue=false,pending_left=false,pending_right=false;
        uint16_t pending_psx_pressed=0;
        uint64_t last_rng_mismatch_generation=0;
        unsigned last_scene_status=~0u,last_scene_waits=~0u;
        uint32_t last_scene_callback=~0u;
        uint64_t actor_packet_commits=0;
        std::optional<uint64_t> failed_source_handoff_generation;
        std::optional<uint64_t> failed_door_generation;
        std::optional<l3::SourceEventCdRequest> applied_mix_request;
        uint64_t applied_mix_digest=0;
        size_t applied_mix_count=0,applied_mix_index=0;
        const auto clear_applied_mix=[&] {
            applied_mix_request.reset();applied_mix_digest=0;
            applied_mix_count=applied_mix_index=0;
        };

        while(aptMainLoop()) {
            const u64 frame_start=svcGetSystemTick();
            g_runtime_diag.draw_calls=0;
            g_submitted_vertices=0;
            hidScanInput(); u32 down=hidKeysDown(), held=hidKeysHeld(), up_keys=hidKeysUp();
            circlePosition autoplay_circle{};
            if(g_autoplay.active) {
                const size_t step_before=g_autoplay.index;const uint32_t used_before=g_autoplay.used;
                if(!diagnostic_autoplay_frame(down,held,up_keys,autoplay_circle)) {
                    hardware_test_trace("# diagnostic_autoplay_end,frame=%lu,evidence=FRONTEND_DIAGNOSTIC",(unsigned long)session.frame_counter());
                    break;
                }
                const auto& autoplay_step=g_autoplay.steps[g_autoplay.index];
                if(autoplay_step.keys==kAutoplaySteerToPickup||autoplay_step.keys==kAutoplaySteerToPoint) {
                    const auto pp=session.player_position();
                    const auto& pw=session.compat_player().bytes;
                    const int heading=int(uint16_t(pw[0x56]|(uint16_t(pw[0x57])<<8))&0xfff);
                    u32 steer=0;
                    std::optional<std::array<int,2>> goal;
                    if(autoplay_step.keys==kAutoplaySteerToPoint)goal={{int(autoplay_step.cx),int(autoplay_step.cy)}};
                    else for(const auto& e:g_children.effect_pool().slots)if(e[0]&&e[2]==0x87){
                        goal={{int(int16_t(uint16_t(e[0x38]|(e[0x39]<<8)))),int(int16_t(uint16_t(e[0x3c]|(e[0x3d]<<8))))}};break;}
                    if(goal){
                        const int tx=(*goal)[0]-int(pp.x);
                        const int tz=(*goal)[1]-int(pp.z);
                        // Forward motion is (-sin h, -cos h): face atan2(-x, -z).
                        const int want=int(std::lround(std::atan2(double(-tx),double(-tz))*4096.0/6.283185307179586))&0xfff;
                        int gap=(want-heading)&0xfff;if(gap>2048)gap-=4096;
                        const int mag=gap<0?-gap:gap;
                        if(g_autoplay.last_turned&&g_autoplay.last_gap>=0&&mag>g_autoplay.last_gap+2)g_autoplay.turn_sign=-g_autoplay.turn_sign;
                        g_autoplay.last_gap=mag;g_autoplay.last_turned=mag>48;
                        const bool arrived=tx*tx+tz*tz<24*24;
                        if(!arrived&&mag>48)steer|=((gap>0)==(g_autoplay.turn_sign>0))?KEY_DLEFT:KEY_DRIGHT;
                        if(!arrived&&mag<128)steer|=KEY_DUP;
                    }
                    held=steer;down=held&~g_autoplay.previous;up_keys=g_autoplay.previous&~held;g_autoplay.previous=held;
                }
                if(g_autoplay.index!=step_before||used_before==0)
                    hardware_test_trace("# diagnostic_autoplay,frame=%lu,step=%lu,keys=%03lx,circle=%d:%d,evidence=FRONTEND_DIAGNOSTIC",
                        (unsigned long)session.frame_counter(),(unsigned long)g_autoplay.index,(unsigned long)held,
                        int(autoplay_circle.dx),int(autoplay_circle.dy));
            }
            if(down&KEY_START) break;
            if(g_render_submissions==1)boot_log(MML_BOOT_FIRST_FRAME,"waiting for first 3D GPU submission");
            const u64 gpu_wait_us=acquire_gpu_frame("gameplay frame");
            if(g_render_submissions==1)boot_log(MML_BOOT_FIRST_FRAME,"first 3D GPU submission synchronized");
            const u32 released_keys=up_keys;
            const bool ui_focus=(held&KEY_SELECT)!=0;
            if(down&KEY_SELECT) {select_ui_used=false;ui_hold_frames=0;}
            if(ui_focus) {
                const auto pp=session.player_position();
                const auto ui_view=l3::make_mockup_map_view(session.active_map(),session.map_area_index(),pp.x,pp.z,session.player_yaw());
                const auto command=[&](l3::MockupUiCommand c) {(void)l3::mockup_ui_command(ui_state,ui_view,c);select_ui_used=true;};
                if(down&KEY_L) command(l3::MockupUiCommand::PreviousTab);
                if(down&KEY_R) command(l3::MockupUiCommand::NextTab);
                if(down&KEY_X) command(l3::MockupUiCommand::ZoomMap);
                if(down&KEY_B) command(l3::MockupUiCommand::CenterPlayer);
                if(down&KEY_A) command(l3::MockupUiCommand::ToggleVisible);
                const u32 pan=(ui_hold_frames++%8u==0u)?held:down;
                if(pan&KEY_DLEFT) command(l3::MockupUiCommand::PanLeft);
                if(pan&KEY_DRIGHT) command(l3::MockupUiCommand::PanRight);
                if(pan&KEY_DUP) command(l3::MockupUiCommand::PanUp);
                if(pan&KEY_DDOWN) command(l3::MockupUiCommand::PanDown);
            }
            const u32 touch_up=hidKeysUp()&KEY_TOUCH;
            if((down|held|touch_up)&KEY_TOUCH) {
                touchPosition touch{};
                if((down|held)&KEY_TOUCH) hidTouchRead(&touch);
                const auto touch_player=session.player_position();
                const auto touch_map=l3::make_mockup_map_view(session.active_map(),
                    session.map_area_index(),touch_player.x,touch_player.z);
                const auto phase=(down&KEY_TOUCH)?l3::MockupTouchPhase::Begin:
                    touch_up?l3::MockupTouchPhase::End:l3::MockupTouchPhase::Move;
                (void)l3::mockup_ui_touch_event(ui_state,touch_map,phase,touch.px,touch.py);
            }
            const auto source_handoff = g_source_event_boot ? session.source_area_handoff()
                : std::optional<l3::SourceEventHandoffTransitionIntent>{};
            if(!source_handoff)failed_source_handoff_generation.reset();
            const bool source_area_transition = source_handoff &&
                failed_source_handoff_generation != source_handoff->source_generation &&
                session.can_commit_source_area_handoff();
            if (source_handoff && !source_area_transition && session.frame_counter()%60u == 0u)
                hardware_test_trace("# source_area_handoff_held,frame=%lu,destination=%u,command_status=%u,source_request_retained=true",
                    (unsigned long)session.frame_counter(),unsigned(source_handoff->destination_area),
                    unsigned(session.source_event_command_result().status));
            // Player door event 0x80050C2C case 3: event 0 runs in tick() and
            // offers the load at substate 6 (356C = 1); this commit is the loader.
            const auto door_handoff=source_area_transition?std::optional<l3::SourceEventHandoffTransitionIntent>{}
                : session.source_door_transition();
            if(!door_handoff)failed_door_generation.reset();
            const bool door_transition=door_handoff&&failed_door_generation!=door_handoff->source_generation&&
                session.can_commit_source_door_transition();
            if(source_area_transition || door_transition || ((released_keys&KEY_SELECT) && !select_ui_used)) {
                const u64 transition_start=svcGetSystemTick();
                hardware_test_area_event(source_area_transition?"source_transition_before":
                    door_transition?"door_transition_before":"transition_before",g_current_area,0,
                    g_package_bytes,g_package_decoded_estimate,g_package_peak_estimate);
                const size_t outgoing_area=g_current_area;
                const size_t outgoing_primitives=g_area_primitive_count;
                const int32_t outgoing_min_y=g_area_min_y,outgoing_max_y=g_area_max_y;
                size_t next_area=outgoing_area;
                l3::RuntimeVec3i next_spawn{};
                uint16_t next_yaw=0;
                l3::ActorAreaRuntime next_actors;
                bool resources_changed=false,restoration_failed=false;
                try {
                    l3::transition_area_resources([&] {
                        next_area=source_area_transition?source_handoff->destination_area:
                            door_transition?door_handoff->destination_area:
                            next_diagnostic_area(map,render,(outgoing_area+1u)%map.areas.size());
                        const auto* binding=session.registered_area(next_area);
                        if (!binding) throw std::runtime_error("selected area has no runtime registration");
                        next_spawn=source_area_transition?l3::RuntimeVec3i{source_handoff->spawn[0],source_handoff->spawn[1],source_handoff->spawn[2]}:
                            door_transition?l3::RuntimeVec3i{door_handoff->spawn[0],door_handoff->spawn[1],door_handoff->spawn[2]}:
                            first_positive_cell_center(map,next_area);
                        if (source_area_transition) {
                            next_yaw=uint16_t(source_handoff->raw[10])|(uint16_t(source_handoff->raw[11])<<8);
                        } else if (door_transition) {
                            next_yaw=uint16_t(door_handoff->raw[10])|(uint16_t(door_handoff->raw[11])<<8);
                        } else if (terrain_xyz_active()) {
                            const auto entry=inspection_entry(map,render,next_area);
                            next_spawn={entry.origin.x,entry.origin.y,entry.origin.z};next_yaw=entry.yaw;
                        }
                        preflight_current_terrain(map,render,next_area);
                        // Actor mesh admission is capped at 512KiB per area;
                        // only these bounded CPU actors coexist during staging.
                        next_actors=load_area_actors(*binding,map,next_area,actor_metadata,session.source_stats_selector());
                        resources_changed=true;
                        set_e007_gate_cells(*binding,map,next_area);
                        rebuild_current_terrain(map,render,&terrain_materials,&player_materials,next_area);
                    }, [&] {
                        // RuntimeSession copies fallible work before changing
                        // collision, actor-table identity or pending events.
                        if (!(source_area_transition?session.commit_source_area_handoff():
                              door_transition?session.commit_source_door_transition():
                              session.transition_to_area(next_area,next_spawn,next_yaw)))
                            throw std::runtime_error("selected area transition rejected");
                        // The door arrival is event 0's offset from the request.
                        if (door_transition) {next_spawn=session.player_position();next_yaw=session.player_yaw();}
                        // The committed replacement invalidates the command
                        // owner. A rejected staging attempt retains its audio.
                        xa_audio.close();xa_transport={};clear_applied_mix();
                        hardware_test_trace("# source_xa_unload,linear=%lu,index=%lu,source_handoff=%u,reason=committed_area_replacement,retail_overlap_verified=false",
                            (unsigned long)xa_audio.linear_bytes(),
                            (unsigned long)xa_audio.stream().resident_index_bytes(),unsigned(source_area_transition));
                        g_children.initialize();
                        source_buster_proxy_mask=0xffu;
                    }, [&] {
                        l3::unload_imported_actors(next_actors);
                        if (resources_changed) {
                            // Old3DS avoids two GPU/cache sets at once. Rebuild
                            // the old view from owner-local data on rejection.
                            try {
                                set_e007_gate_cells(session.active_area(),map,outgoing_area);
                                rebuild_current_terrain(map,render,&terrain_materials,&player_materials,outgoing_area);
                            } catch (const std::exception& restore_error) {
                                restoration_failed=true;
                                release_all_render_resources();terrain_materials={};player_materials={};
                                boot_log(MML_BOOT_DATA,restore_error.what());
                                throw std::runtime_error("area transition restoration failed; partial resources released");
                            }
                        }
                        g_area_primitive_count=outgoing_primitives;
                        g_area_min_y=outgoing_min_y;g_area_max_y=outgoing_max_y;
                    });
                    l3::unload_imported_actors(actors);
                    actors=std::move(next_actors);
                    // Presentation resources belong to the outgoing area,
                    // independent of the recorded simulation phase gate.
                    // That gate becomes inactive on replacement and may never
                    // call service_scene_actors again. Retire only after the
                    // fallible transition has committed; failed staging keeps
                    // the old scene and its textures available for restoration.
                    if(!session.source_event_phase5_resources().resident_in(session.active_area().ebd)) {
                        const bool retired=scene_actor40.has_value()||scene_e0.has_value()||
                            !g_scene_actor40_materials.empty()||!g_scene_materials.empty();
                        scene_actor40.reset();scene_e0.reset();
                        scene_actor40_prefix_ready=scene_actor40_pose_ready=false;
                        release_scene_actor40_materials();release_scene_materials();
                        if(retired)hardware_test_area_event("outgoing_scene_released",next_area,0,
                            g_package_bytes,g_package_decoded_estimate,g_package_peak_estimate);
                    }
                    // This native area adapter creates source work through
                    // seed_fresh_stage_runtime_work, rather than reusing live
                    // pool slots. The opt-in diagnostic explicitly admits the
                    // original fresh pool/shift stores for those new owners.
                    // It does not establish natural retail transition order.
                    if(g_source_actor_proxy)for(auto& actor:actors.actors)
                        (void)l3::initialize_imported_actor_fresh_pose_proxy_subset(actor);
                    actor_revisions={session.conditional_spawn_revision(),
                        session.conditional_spawn_revision(),std::nullopt};
                    if(actor_metadata.event_poll)session.bind_source_event_profile(*actor_metadata.event_poll,0);
                    g_current_area=next_area;spawn=next_spawn;inspection_yaw=next_yaw;
                    hardware_test_actor_event(g_current_area,actors);
                    mockup_hud_invalidate();
                    pending_fire=pending_lock=pending_dialogue=false;
                    pending_psx_pressed=0;
                    hardware_test_geometry_event(terrain_xyz_active(),g_current_area,g_area_primitive_count,g_area_min_y,g_area_max_y);
                    hardware_test_area_event(source_area_transition?"source_transition_after":
                        door_transition?"door_transition_after":"transition_after",g_current_area,
                        (svcGetSystemTick()-transition_start)/CPU_TICKS_PER_USEC,
                        g_package_bytes,g_package_decoded_estimate,g_package_peak_estimate);
                    last_valid=spawn;
                    gameplay_clock.reset();
                    previous_update_clock=svcGetSystemTick();
                } catch (const std::exception& error) {
                    boot_log(MML_BOOT_DATA,error.what());
                    // A retained source request must not retry a failed disk/GPU
                    // replacement every frame. Keep the old area and intent;
                    // a fresh source lease or relaunch can retry preparation.
                    if(source_area_transition)failed_source_handoff_generation=source_handoff->source_generation;
                    if(door_transition)failed_door_generation=door_handoff->source_generation;
                    hardware_test_area_event(restoration_failed?"transition_restore_failed":"transition_rejected",
                        outgoing_area,(svcGetSystemTick()-transition_start)/CPU_TICKS_PER_USEC,
                        g_package_bytes,g_package_decoded_estimate,g_package_peak_estimate);
                    if (restoration_failed) throw;
                    // Rejected loading is still a loading pause. Restoration
                    // time must not become movement catch-up in the old area.
                    pending_fire=pending_lock=pending_dialogue=false;
                    pending_psx_pressed=0;gameplay_clock.reset();
                    previous_update_clock=svcGetSystemTick();
                }
            }

            circlePosition cp{}; hidCircleRead(&cp);
            if(g_autoplay.active)cp=autoplay_circle;
            l3::RuntimeInputFrame in{};
            in.held_psx_mask=make_psx_held(held,cp);
            in.pressed_psx_mask=make_psx_held(down,circlePosition{});
            in.fire_pressed=(down&KEY_Y)!=0;
            in.lock_pressed=(down&KEY_X)!=0;
            in.lock_held=(held&KEY_X)!=0;
            in.advance_dialogue=(down&KEY_A)!=0;
            in.dialogue_left=(down&(KEY_DLEFT|KEY_CPAD_LEFT))!=0;
            in.dialogue_right=(down&(KEY_DRIGHT|KEY_CPAD_RIGHT))!=0;
            const u64 update_start=svcGetSystemTick();
            const u64 update_clock=svcGetSystemTick();
            gameplay_clock.advance((update_clock-previous_update_clock)/CPU_TICKS_PER_USEC,ui_focus);
            previous_update_clock=update_clock;
            if (ui_focus) {
                pending_fire=pending_lock=pending_dialogue=false;pending_psx_pressed=0;
            }
            else {
                pending_psx_pressed|=in.pressed_psx_mask;
                pending_fire=pending_fire||in.fire_pressed;
                pending_lock=pending_lock||in.lock_pressed;
                pending_dialogue=pending_dialogue||in.advance_dialogue;
                pending_left=pending_left||in.dialogue_left;pending_right=pending_right||in.dialogue_right;
            }
            // Bounded catch-up retains debt; no variable-delta physics or render-rate animation.
            for (unsigned step=0;step<4 && gameplay_clock.consume();++step) {
                in.fire_pressed=pending_fire;in.lock_pressed=pending_lock;in.advance_dialogue=pending_dialogue;
                in.dialogue_left=pending_left;in.dialogue_right=pending_right;pending_left=pending_right=false;
                in.pressed_psx_mask=pending_psx_pressed;pending_psx_pressed=0;
                pending_fire=pending_lock=pending_dialogue=false;
                const auto actor_gameplay_owned=[&]{
                    // Tutorial messages keep gameplay; their handlers pause the
                    // object classes through 0x8002EE90 (mode 1). CANDIDATE consumer:
                    // the native class-0x20 loop holds while paused.
                    return !(session.dialogue().active&&session.dialogue().gates_gameplay)&&
                        session.tutorial_pause_mode()!=1&&!session.cutscene_active()&&
                        session.camera().owner==l3::RuntimeCameraOwner::Gameplay;
                };
                const bool actor_tick_admitted=(retail_actor_animation_requested()||actor_state_fields_requested())&&actor_gameplay_owned();
                if (retail_animation_requested()) (void)l3::runtime_animation_prepare(animation,player_track,session);
                if(source_xa_requested&&session.source_event_command_result().pending_request){
                    const auto& command=session.source_event_command_result();
                    const auto& request=*command.pending_request;
                    const bool pause_request=request.source_function==0x8001d254u&&request.command==9;
                    bool prefix_ready=!pause_request;
                    if(pause_request&&command.pending_mix_program){
                        const auto& program=*command.pending_mix_program;
                        const auto digest=l3::source_event_cd_mix_program_digest(program);
                        const bool same_request=applied_mix_request&&
                            applied_mix_request->source_function==request.source_function&&
                            applied_mix_request->command==request.command&&
                            applied_mix_request->parameter_bytes==request.parameter_bytes&&
                            applied_mix_request->parameters==request.parameters;
                        const bool new_prefix=!same_request||applied_mix_digest!=digest||applied_mix_count!=program.count;
                        if(new_prefix){
                            clear_applied_mix();applied_mix_request=request;
                            applied_mix_digest=digest;applied_mix_count=program.count;
                        }
                        // Preserve exact source ordering: any older queued mix,
                        // then this admitted 127/254-call prefix, then pause.
                        // No timers or assumed DSP completion supply a receipt.
                        const size_t previous_applied=applied_mix_index;
                        if(program.count<=program.records.size()&&session.source_event_cd_mix_intents().count==0){
                            while(applied_mix_index<program.count&&
                                  xa_audio.apply_diagonal_mix(program.records[applied_mix_index].values))
                                ++applied_mix_index;
                            prefix_ready=applied_mix_index==program.count;
                        }
                        if(new_prefix||previous_applied!=applied_mix_index)
                            hardware_test_trace("# source_native_mix_prefix,frame=%lu,applied=%lu,count=%lu,digest=%016llx,pause_allowed=%u",
                                (unsigned long)session.frame_counter(),(unsigned long)applied_mix_index,
                                (unsigned long)program.count,(unsigned long long)digest,unsigned(prefix_ready));
                    }else if(!pause_request)clear_applied_mix();
                    if(prefix_ready){
                        const auto r=l3::service_native_xa_cd_request(xa_transport,xa_audio.stream(),request,xa_audio.observation());
                        if(r.request_start)(void)xa_audio.start();
                        if(r.request_pause){
                            const bool paused=xa_audio.pause();
                            hardware_test_trace("# source_native_pause,frame=%lu,requested=true,backend_paused=%u,prefix_count=%lu,receipt_deferred_until_observation=true",
                                (unsigned long)session.frame_counter(),unsigned(paused),(unsigned long)applied_mix_index);
                        }
                        if(r.receipt){
                            l3::SourceEventCommandInputs receipts;receipts.receipt_count=1;receipts.receipts[0]=*r.receipt;
                            if(pause_request){
                                receipts.receipts[0].applied_mix_prefix_count=static_cast<uint16_t>(applied_mix_index);
                                receipts.receipts[0].applied_mix_prefix_digest=applied_mix_digest;
                            }
                            (void)session.deliver_source_event_cd_receipts(receipts);
                        }
                    }
                }else clear_applied_mix();
                const auto service_scene_actors=[&] {
                if(scene_actor40) {
                    if(!session.source_event_phase5_resources().resident_in(session.active_area().ebd)){scene_actor40.reset();scene_actor40_prefix_ready=scene_actor40_pose_ready=false;release_scene_actor40_materials();}
                    else if(!scene_actor40_pose_ready){
                        const auto result=session.service_diagnostic_scene_actor40_initializer(*scene_actor40);
                        scene_actor40_prefix_ready=scene_actor40_pose_ready=result.committed;
                        if(unsigned(result.status)!=last_actor40_pose_status||result.committed){
                            const auto& globals=session.source_scene_actor40_initializer_globals();
                            hardware_test_trace("# source_actor40_initializer,frame=%lu,status=%u,committed=%u,parts=%u,work08_ack=%u,mirror_bytes=12,fixed_position_written=false,global70=%08lx,global74=%08lx,chronology=CANDIDATE",
                                (unsigned long)session.frame_counter(),unsigned(result.status),unsigned(result.committed),unsigned(result.pose_parts),
                                unsigned(result.committed),(unsigned long)globals.word_80122470.value_or(0),(unsigned long)globals.word_80122474.value_or(0));
                            last_actor40_pose_status=unsigned(result.status);
                        }
                    }
                    else {
                        const auto result=session.service_diagnostic_scene_actor40_update(*scene_actor40);
                        if(result.request_reset||unsigned(result.status)!=last_actor40_update_status||session.frame_counter()%60u==0u){
                            hardware_test_trace("# source_actor40_update,frame=%lu,status=%u,committed=%u,state=%u,substate=%u,request=%u,shadow_pending=%u,timed_cues_pending=%u,tail_status=%u,player_mirrors=%u,chronology=CANDIDATE",
                                (unsigned long)session.frame_counter(),unsigned(result.status),unsigned(result.committed),
                                unsigned(result.state),unsigned(result.substate),unsigned(result.request),
                                unsigned(result.shadow_pending),unsigned(result.timed_cues_pending),
                                result.tail_status?unsigned(*result.tail_status):255u,unsigned(result.final_player_mirrors_applied));
                            const auto& work=session.source_event_phase5_context().pool40.slots[0];
                            const auto raw_short=[&](size_t at){const uint16_t value=uint16_t(work.raw[at])|(uint16_t(work.raw[at+1])<<8);
                                return value<32768u?int(value):int(value)-65536;};
                            hardware_test_trace("# source_actor40_position,frame=%lu,state=%u,substate=%u,x=%d,y=%d,z=%d,request=%u,halfword_owner=80030150,physics_1to1=false",
                                (unsigned long)session.frame_counter(),unsigned(work.raw[9]),unsigned(work.raw[10]),
                                raw_short(20),raw_short(22),raw_short(24),unsigned(work.raw[172]));
                            last_actor40_update_status=unsigned(result.status);
                        }
                    }
                }
                // Original80015E04 lower-family80 sweep follows class40
                // and precedes the80015E0C E0 sweep. Unknown RNG stays held.
                if(g_source_event_boot) {
                    l3::SourceEffectDispatchInputProvider provider;
                    // Preserve strict reference replay: never fill a missing
                    // recorded result with a generated candidate sample.
                    if(g_actor_rng.admitted()&&!g_source_scene_rng_replay){
                        provider.effect2_candidate_rng=&g_actor_rng;
                        provider.candidate_update={session.frame_counter(),g_actor_update_ordinal,l3::SourceRngUpdatePhase::Effect2Callback};
                        provider.effect34_candidate_rng=&g_actor_rng;
                        provider.candidate34_update={session.frame_counter(),g_actor_update_ordinal,l3::SourceRngUpdatePhase::Effect34Callback};
                    }
                    const auto pass=session.service_diagnostic_scene_lower_effects(provider);
                    if(pass.children||pass.released||session.frame_counter()%60u==0u) {
                        hardware_test_trace("# source_lower80_update,frame=%lu,status=%u,visited=%u,callbacks=%u,committed=%u,held=%u,children=%u,released=%u,rng_consumed=%u,chronology=CANDIDATE",
                            (unsigned long)session.frame_counter(),unsigned(pass.status),unsigned(pass.visited),
                            unsigned(pass.callbacks),unsigned(pass.committed),unsigned(pass.held),
                            unsigned(pass.children),unsigned(pass.released),unsigned(pass.rng_consumed));
                        for(size_t i=0;i<pass.held;++i) {
                            const auto& h=pass.holds[i];
                            hardware_test_trace("# source_lower80_hold,frame=%lu,slot=%u,subtype=%u,callback=%08lx,kind=%u,status=%u,missing_offset=%u,missing_offset_known=%u,needs_rng_receipts=%u,unknowns_preserved=true",
                                (unsigned long)session.frame_counter(),unsigned(h.slot),unsigned(h.subtype),
                                (unsigned long)h.callback,unsigned(h.kind),unsigned(h.consumer_status),
                                h.missing_offset?unsigned(*h.missing_offset):255u,unsigned(h.missing_offset.has_value()),
                                unsigned(h.kind==l3::SourceEffectDispatchHoldKind::Effect2&&h.consumer_status==unsigned(l3::SourceEffect2Status::NeedsRngReceipts)));
                        }
                    }
                }
                if(scene_e0) {
                    if(!scene_e0->resident_in(session.active_area().ebd)||!session.source_event_phase5_resources().resident_in(session.active_area().ebd)){scene_e0.reset();release_scene_materials();}
                    else {
                        const auto prop_rng_before=g_actor_rng.native_global_ordinal();
                        const auto pass=session.service_diagnostic_scene_e0_updates(*scene_e0,g_actor_rng.admitted()&&!g_source_scene_rng_replay?&g_actor_rng:nullptr);
                        if(g_actor_rng.native_global_ordinal()!=prop_rng_before&&session.frame_counter()%60u==0u)
                            hardware_test_trace("# source_prop_rng_candidate,frame=%lu,ordinal_before=%llu,ordinal_after=%llu,scope=SHARED_OPENBIOS_CANDIDATE,retail_chronology=false",
                                (unsigned long)session.frame_counter(),(unsigned long long)prop_rng_before,
                                (unsigned long long)g_actor_rng.native_global_ordinal());
                        if(session.frame_counter()%60u==0u){
                            for(const size_t slot:{size_t(0),size_t(2),size_t(14)}){
                                const auto& prop=session.source_event_phase5_context().pool_e0.slots[slot];
                                const auto half=[&](size_t offset){const uint16_t raw=uint16_t(prop.raw[offset])|(uint16_t(prop.raw[offset+1])<<8);
                                    return raw<32768u?int(raw):int(raw)-65536;};
                                const auto& result=pass.slots[slot];
                                hardware_test_trace("# source_prop,frame=%lu,slot=%u,status=%u,committed=%u,handler=%08lx,state=%u,substate=%u,xyz=%d/%d/%d,position_known=%u%u%u%u%u%u",
                                    (unsigned long)session.frame_counter(),unsigned(slot),unsigned(result.status),unsigned(result.committed),
                                    (unsigned long)result.source_function,unsigned(prop.raw[9]),unsigned(prop.raw[10]),half(56),half(58),half(60),
                                    unsigned(prop.known[56]),unsigned(prop.known[57]),unsigned(prop.known[58]),unsigned(prop.known[59]),unsigned(prop.known[60]),unsigned(prop.known[61]));
                            }
                        }
                        if(pass.initialized_count||session.frame_counter()%60u==0u)
                            hardware_test_trace("# source_scene_E0_update,frame=%lu,status=%u,initialized=%lu,updated=%lu,held=%lu,chronology=CANDIDATE,later_behavior=SOURCE_BACKED_PARTIAL",
                                (unsigned long)session.frame_counter(),unsigned(pass.status),
                                (unsigned long)pass.initialized_count,(unsigned long)pass.updated_count,(unsigned long)pass.held_count);
                    }
                }
                if(g_source_event_boot){const auto pass=session.service_diagnostic_scene_effects();
                    if(pass.updated||pass.released||session.frame_counter()%60u==0u)
                        hardware_test_trace("# source_scene_effect80_update,frame=%lu,status=%u,committed=%u,updated=%u,released=%u,chronology=CANDIDATE",
                            (unsigned long)session.frame_counter(),unsigned(pass.status),unsigned(pass.committed),
                            unsigned(pass.updated),unsigned(pass.released));}
                };
                // Original family sweeps precede the event constructor. In
                // recorded replay mode no actor update is run on its constructor
                // tick; the first one sees the following counter after the tail.
                if((g_source_loop_replay||g_source_scene_rng_replay)&&session.source_recorded_scene_admitted()&&
                   session.source_main_loop_recorded_phase_gate().status==l3::SourceMainLoopPhaseStatus::ReadyForUpdate)
                    service_scene_actors();
                if(!g_source_loop_replay&&!g_source_scene_rng_replay)service_scene_actors();
                const auto old_event_status=session.source_event_result().status;
                const auto old_event_phase=session.source_event_work()?session.source_event_work()->phase04():0;
                const auto old_event_id=session.source_event_work()?session.source_event_work()->event01():0;
                const auto old_command_status=session.source_event_command_result().status;
                const auto old_command_phase=session.source_event_command_queue().phase_800989c4;
                // Y is only the native frontend edge. The original animation
                // callback which later asserts player+174 bits10/20 remains
                // untraced, so this admission is CANDIDATE and is logged below.
                const bool source_buster_fire_pressed=l3::source_buster_fire_admitted(
                    g_source_actor_proxy,in.fire_pressed,session.player_shot_enabled(),session.player_busy_8004324c());
                {
                    // E0/06 type 6 reads live class-0x20 +14/+18.
                    std::vector<std::array<int16_t,2>> live;
                    for(const auto& actor:actors.actors)
                        if(actor.source_behavior&&!actor.source_released&&actor.source_class==0x20)
                            live.push_back({{actor.source_behavior->raw_14[0],actor.source_behavior->raw_14[2]}});
                    session.set_tutorial_class20_positions(std::move(live));
                }
                const bool dialogue_was_active=session.dialogue().active;
                session.tick(in,g_actor_rng.admitted()&&!g_source_scene_rng_replay?&g_actor_rng:nullptr);
                if(session.tutorial_triggers_active()&&(session.dialogue().active!=dialogue_was_active||
                   session.frame_counter()%60u==0u))
                    hardware_test_trace("# source_tutorial,frame=%lu,status=%u,cell=%d:%d,dialogue=%u,block=%u,choice=%u,pause=%lu,bd=%02x,flag1552=%u,flag1570=%u,unsupported=%d,wait=%u%u%u%u%u",
                        (unsigned long)session.frame_counter(),unsigned(session.tutorial_trigger_tick().status),
                        int(session.tutorial_triggers().cell_x()),int(session.tutorial_triggers().cell_z()),
                        unsigned(session.dialogue().active),unsigned(session.dialogue().ref.block),unsigned(session.dialogue().choice),
                        (unsigned long)session.tutorial_pause_mode(),unsigned(session.compat_player().bytes[0xBD]),
                        unsigned(session.story_flag(0x610)),unsigned(session.story_flag(0x622)),
                        session.tutorial_trigger_tick().unsupported_type?int(*session.tutorial_trigger_tick().unsupported_type):-1,
                        unsigned(session.dialogue().waiting_for_audio),unsigned(session.dialogue().waiting_for_advance),
                        unsigned(session.dialogue().waiting_for_script),unsigned(session.dialogue().waiting_for_choice),unsigned(session.dialogue().wait_frames!=0));
                // 0x80047228(this, +0x38, record): the box's L3CB E0/05 row word00. The
                // box calls it inside its callback, before trigger 16 pauses objects.
                for(const auto& reward:session.take_e005_reward_requests()){
                    const auto* profile=actor_metadata.combat&&session.source_stats_selector()
                        ? l3::find_source_combat_profile(*actor_metadata.combat,*session.source_stats_selector(),0xE0,5,reward.record,0)
                        : nullptr;
                    const bool queued=profile&&profile->reward_word00&&g_children.enqueue_reward_80047228(reward.point,*profile->reward_word00);
                    hardware_test_trace("# e005_reward,frame=%lu,record=%u,word00=%08lx,queued=%u,flag620=%u",
                        (unsigned long)session.frame_counter(),unsigned(reward.record),
                        profile&&profile->reward_word00?(unsigned long)*profile->reward_word00:0ul,unsigned(queued),unsigned(session.story_flag(0x620)));
                }
                for(const auto clip:session.take_tutorial_voice_requests())
                    if(session.frame_counter()%30u==0u)hardware_test_trace("# source_tutorial_voice,frame=%lu,clip=%lu,playback=UNPORTED",
                        (unsigned long)session.frame_counter(),(unsigned long)clip);
                if(session.source_event_result().random_callback_consumed_count&&g_actor_rng.admitted()&&!g_source_scene_rng_replay)
                    hardware_test_trace("# source_event_rng_candidate,frame=%lu,calls=%u,ordinal_after=%llu,scope=SHARED_OPENBIOS_CANDIDATE,retail_chronology=false",
                        (unsigned long)session.frame_counter(),unsigned(session.source_event_result().random_callback_consumed_count),
                        (unsigned long long)g_actor_rng.native_global_ordinal());
                if (g_source_event_boot && head_uv_baseline) {
                    if (const auto generation = session.source_handoff_display_generation()) {
                        const auto receipt = l3::restore_source_event_head_uv_8002d004(
                            *head_uv_baseline, local_visuals, &session, *generation);
                        const bool admitted = receipt && session.bind_source_handoff_display_receipt(*receipt);
                        hardware_test_trace("# source_head_uv_handoff,frame=%lu,reset=%u,admitted=%u,records=%lu,generation=%llu,next_original_tick=true,scene_complete=false",
                            (unsigned long)session.frame_counter(),unsigned(receipt.has_value()),unsigned(admitted),
                            (unsigned long)head_uv_baseline->record_count(),(unsigned long long)*generation);
                    }
                }
                if(session.source_event_command_result().transport_receipts_consumed||
                   !session.source_event_command_result().pending_request)clear_applied_mix();
                // Snapshot the first held context before cleanup/tail drift.
                // A new tape generation rearms this diagnostic; no owners change.
                if(g_source_scene_rng_replay&&
                   session.source_scene_rng_replay_status()==l3::SourceEventRandomTapeStatus::ContextMismatch) {
                    const auto& tape=session.source_scene_rng_replay();
                    if(tape.generation()!=last_rng_mismatch_generation&&tape.cursor()<tape.profile().record_count) {
                        last_rng_mismatch_generation=tape.generation();
                        const auto& expected=tape.profile().records[tape.cursor()];
                        const auto& scene=session.source_event_phase5_context();
                        const auto scratch=session.source_event_phase6_context().scratch_half_1f800002;
                        hardware_test_trace("# source_scene_rng_first_mismatch,frame=%lu,generation=%llu,cursor=%lu,scratch_known=%u,scratch=%u,expected_scratch=%u,boundary=AFTER_EVENT_BEFORE_CLEANUP_AND_TAIL,stores=0,cursor_advanced=false",
                            (unsigned long)session.frame_counter(),(unsigned long long)tape.generation(),
                            (unsigned long)tape.cursor(),unsigned(scratch.has_value()),unsigned(scratch.value_or(0)),unsigned(expected.scratch_half));
                        uint32_t actor_token=0;unsigned actor_token_known=0;
                        if(const auto& work=session.source_event_work()) {
                            for(size_t i=0;i<4;++i) {
                                actor_token|=uint32_t(work->raw[36+i])<<(8*i);
                                if(work->known[36+i])actor_token_known|=1u<<i;
                            }
                        }
                        hardware_test_trace("# source_scene_rng_first_identity,frame=%lu,live=%08lx,live_known=%x,expected=%08lx",
                            (unsigned long)session.frame_counter(),(unsigned long)actor_token,actor_token_known,
                            (unsigned long)expected.source_actor);
                        const uint32_t base=0x800c3580u,stride=1444u;
                        if(actor_token_known==15u&&actor_token>=base&&(actor_token-base)%stride==0&&
                           (actor_token-base)/stride<scene.pool40.slots.size()) {
                            const auto& actor=scene.pool40.slots[(actor_token-base)/stride];
                            const auto word_at=[&](size_t at){return uint32_t(actor.raw[at])|(uint32_t(actor.raw[at+1])<<8)|
                                (uint32_t(actor.raw[at+2])<<16)|(uint32_t(actor.raw[at+3])<<24);};
                            const auto half_at=[&](size_t at){const uint16_t value=uint16_t(actor.raw[at])|(uint16_t(actor.raw[at+1])<<8);
                                return value<32768u?int(value):int(value)-65536;};
                            const auto known_mask=[&](size_t at,size_t count){unsigned value=0;
                                for(size_t i=0;i<count;++i){if(actor.known[at+i])value|=1u<<i;}
                                return value;};
                            hardware_test_trace("# source_scene_rng_first_actor,frame=%lu,identity=%08lx,half_xyz=%d/%d/%d,half_known=%02x,fixed_xyz=%08lx/%08lx/%08lx,fixed_known=%03x,state08=%u,state09=%u,sub10=%u,requestAC=%u,sequenceA4=%u,countdownA5=%u,field_known=%x/%x/%x/%x/%x/%x",
                                (unsigned long)session.frame_counter(),(unsigned long)actor_token,
                                half_at(20),half_at(22),half_at(24),known_mask(20,6),
                                (unsigned long)word_at(28),(unsigned long)word_at(32),(unsigned long)word_at(36),known_mask(28,12),
                                unsigned(actor.raw[8]),unsigned(actor.raw[9]),unsigned(actor.raw[10]),unsigned(actor.raw[172]),
                                unsigned(actor.raw[164]),unsigned(actor.raw[165]),known_mask(8,1),known_mask(9,1),known_mask(10,1),
                                known_mask(172,1),known_mask(164,1),known_mask(165,1));
                        }
                        hardware_test_trace("# source_scene_rng_first_pool,frame=%lu,allocation=%08lx/%08lx,allocation_known=%u/%u,pending=%08lx/%08lx,pending_known=%u/%u,expected=%08lx/%08lx",
                            (unsigned long)session.frame_counter(),(unsigned long)scene.pool80.used[0].value_or(0),
                            (unsigned long)scene.pool80.used[1].value_or(0),unsigned(scene.pool80.used[0].has_value()),unsigned(scene.pool80.used[1].has_value()),
                            (unsigned long)scene.pool80_pending_release_800988c8[0].value_or(0),
                            (unsigned long)scene.pool80_pending_release_800988c8[1].value_or(0),
                            unsigned(scene.pool80_pending_release_800988c8[0].has_value()),unsigned(scene.pool80_pending_release_800988c8[1].has_value()),
                            (unsigned long)expected.pool80_before[0],(unsigned long)expected.pool80_before[1]);
                        for(size_t i=0;i<scene.pool80.slots.size();++i) {
                            const auto& used=scene.pool80.used[i/32];
                            if(!used||!(*used&(0x80000000u>>(i&31u))))continue;
                            const auto& slot=scene.pool80.slots[i];
                            const unsigned known=(slot.known[0]?1u:0u)|(slot.known[2]?2u:0u)|
                                (slot.known[8]?4u:0u)|(slot.known[13]?8u:0u);
                            hardware_test_trace("# source_scene_rng_first_slot,frame=%lu,slot=%lu,raw00=%u,raw02=%u,raw08=%u,raw0D=%u,known_mask=%x",
                                (unsigned long)session.frame_counter(),(unsigned long)i,unsigned(slot.raw[0]),unsigned(slot.raw[2]),
                                unsigned(slot.raw[8]),unsigned(slot.raw[13]),known);
                        }
                    }
                }
                if(g_source_event_boot&&session.source_event_phase5_resources().resident_in(session.active_area().ebd)){
                    const auto pass=session.service_diagnostic_scene_effect_releases();
                    if(!pass.committed||session.frame_counter()%60u==0u)
                        hardware_test_trace("# source_scene_effect80_cleanup,frame=%lu,status=%u,committed=%u,allocation_known=%u/%u,chronology=CANDIDATE",
                            (unsigned long)session.frame_counter(),unsigned(pass.status),unsigned(pass.committed),
                            unsigned(session.source_event_phase5_context().pool80.used[0].has_value()),
                            unsigned(session.source_event_phase5_context().pool80.used[1].has_value()));}
                if(g_source_event_boot&&session.frame_counter()%60u==0u)
                    hardware_test_trace("# source_pad_decoder,frame=%lu,status=%u,committed=%u,predicate_pressed=%u,decoder=80012298,native_digital_adapter=CANDIDATE,scratch_seed=false",
                        (unsigned long)session.frame_counter(),unsigned(session.source_main_loop_pad_result().status),
                        unsigned(session.source_main_loop_pad_result().committed),unsigned(session.source_event_phase6_context().input_800c0c2a.value_or(0)));

                if(g_source_scene_rng_replay&&session.frame_counter()%60u==0u){
                    const auto& tape=session.source_scene_rng_replay();
                    const auto position=session.player_position();
                    hardware_test_trace("# source_scene_rng_replay,frame=%lu,status=%u,constructor_admitted=%u,cursor=%lu,records=%lu,actual_calls=%u,player_x=%ld,player_y=%ld,player_z=%ld,chronology=CANDIDATE",
                        (unsigned long)session.frame_counter(),unsigned(session.source_scene_rng_replay_status()),unsigned(session.source_recorded_scene_admitted()),
                        (unsigned long)tape.cursor(),(unsigned long)tape.profile().record_count,unsigned(session.source_event_result().random_callback_consumed_count),
                        long(position.x),long(position.y),long(position.z));
                }
                if(g_source_scene_rng_replay&&session.source_scene_rng_replay_status()==l3::SourceEventRandomTapeStatus::ContextMismatch&&
                   session.frame_counter()%60u==0u){
                    const auto& tape=session.source_scene_rng_replay();
                    if(tape.cursor()<tape.profile().record_count){
                        const auto& expected=tape.profile().records[tape.cursor()];
                        const auto& scene=session.source_event_phase5_context();const auto& actor=scene.pool40.slots[0];
                        const auto short_at=[&](size_t offset){const unsigned bits=unsigned(actor.raw[offset])|(unsigned(actor.raw[offset+1])<<8);
                            return bits<=32767u?int(bits):int(bits)-65536;};
                        const auto signed_bits=[](uint16_t bits){return bits<=32767u?int(bits):int(bits)-65536;};
                        hardware_test_trace("# source_scene_rng_mismatch,frame=%lu,actual_actor=%d/%d/%d,recorded_actor=%d/%d/%d,actual_pool=%08lx/%08lx,recorded_pool=%08lx/%08lx,actual_half=%u,recorded_half=%u,stores=0,cursor_advanced=false",
                            (unsigned long)session.frame_counter(),short_at(20),short_at(22),short_at(24),
                            signed_bits(expected.position[0]),signed_bits(expected.position[1]),signed_bits(expected.position[2]),
                            (unsigned long)scene.pool80.used[0].value_or(0),(unsigned long)scene.pool80.used[1].value_or(0),
                            (unsigned long)expected.pool80_before[0],(unsigned long)expected.pool80_before[1],
                            unsigned(session.source_event_phase6_context().scratch_half_1f800002.value_or(0)),unsigned(expected.scratch_half));
                    }
                }
                if(g_source_loop_replay&&(session.source_loop_snapshot_admitted()||session.frame_counter()%60u==0u)) {
                    if(session.frame_counter()%60u==0u||session.source_event_result().phase5_consumer_status)
                        hardware_test_trace("# source_loop_replay,frame=%lu,status=%u,admitted=%u,counter_known=%u,counter=%u,recorded_boundary=true,retail_chronology=false",
                            (unsigned long)session.frame_counter(),unsigned(session.source_loop_snapshot_status()),unsigned(session.source_loop_snapshot_admitted()),
                            unsigned(session.source_event_phase6_context().scratch_half_1f800002.has_value()),unsigned(session.source_event_phase6_context().scratch_half_1f800002.value_or(0)));
                }
                const auto& event_result=session.source_event_result();
                if(session.frame_counter()==1 || event_result.status!=old_event_status ||
                   (session.source_event_work()&&(session.source_event_work()->phase04()!=old_event_phase||
                    session.source_event_work()->event01()!=old_event_id)) ||
                   (g_source_event_boot&&session.frame_counter()%60==0))
                    hardware_test_trace("# source_event,frame=%lu,area=%lu,status=%u,committed=%u,event=%u,phase=%u,consumer=%08lx",
                        (unsigned long)session.frame_counter(),(unsigned long)session.map_area_index(),
                        unsigned(event_result.status),unsigned(event_result.committed),
                        session.source_event_work()?unsigned(session.source_event_work()->event01()):0u,
                        session.source_event_work()?unsigned(session.source_event_work()->phase04()):0u,
                        (unsigned long)(event_result.request?event_result.request->source_function:0));
                if(event_result.fade_consumer_status) {
                    hardware_test_trace("# source_fade,frame=%lu,status=%u,committed=%u,phase=%u,pending_scheduler=%lu,callback=80017e94,presentation_complete=false",
                        (unsigned long)session.frame_counter(),unsigned(*event_result.fade_consumer_status),
                        unsigned(event_result.committed),session.source_event_work()?unsigned(session.source_event_work()->phase04()):0u,
                        (unsigned long)session.source_fade_intents().count);
                }
                if(event_result.phase5_consumer_status) {
                    const auto& scene=session.source_event_phase5_context();
                    hardware_test_trace("# source_scene_constructor,frame=%lu,status=%u,committed=%u,phase=%u,pool40=%08lx,pool80=%08lx,poolE0=%08lx,record_resources=OWNER_EBD,script_callback_prefix=SOURCE_BACKED_PARTIAL,scene_render_complete=false",
                        (unsigned long)session.frame_counter(),unsigned(*event_result.phase5_consumer_status),
                        unsigned(event_result.committed),session.source_event_work()?unsigned(session.source_event_work()->phase04()):0u,
                        (unsigned long)scene.pool40.used[0].value_or(0),
                        (unsigned long)scene.pool80.used[0].value_or(0),
                        (unsigned long)scene.pool_e0.used[0].value_or(0));
                }
                if(event_result.phase6_consumer_status) {
                    const auto& script=session.source_event_phase6_context();
                    const unsigned waits=unsigned(script.awaiting_camera_8001ec0c)|
                        (unsigned(script.awaiting_timeline_8001f070)<<1)|(unsigned(script.awaiting_caller_801011e8)<<2);
                    const bool report=last_scene_status!=unsigned(*event_result.phase6_consumer_status)||
                        last_scene_callback!=event_result.script_callback||last_scene_waits!=waits||session.frame_counter()%60==0;
                    if(report){
                    last_scene_status=unsigned(*event_result.phase6_consumer_status);
                    last_scene_callback=event_result.script_callback;last_scene_waits=waits;
                    hardware_test_trace("# source_scene_callback,frame=%lu,status=%u,committed=%u,callback=%08lx,awaiting_camera=%u,awaiting_timeline=%u,awaiting_caller=%u,next_consumer=%08lx,scene_complete=false",
                        (unsigned long)session.frame_counter(),unsigned(*event_result.phase6_consumer_status),
                        unsigned(event_result.committed),(unsigned long)event_result.script_callback,
                        unsigned(script.awaiting_camera_8001ec0c),
                        unsigned(script.awaiting_timeline_8001f070),unsigned(script.awaiting_caller_801011e8),
                        (unsigned long)(event_result.request?event_result.request->source_function:0));
                    hardware_test_trace("# source_scene_camera,frame=%lu,fog_controls_known=%u,dqa=%08lx,dqb=%08lx,source_shift_known=%u,source_shift=%u,renderer_binding_record=source_scene_view",
                        (unsigned long)session.frame_counter(),unsigned(script.projection.dqa.has_value()&&script.projection.dqb.has_value()),
                        (unsigned long)script.projection.dqa.value_or(0),(unsigned long)script.projection.dqb.value_or(0),
                        unsigned(script.byte_1f800074.has_value()),unsigned(script.byte_1f800074.value_or(0)));
                    if(session.source_event_work()){
                        const auto& work=*session.source_event_work();
                        const auto word=[&](size_t at)noexcept->std::optional<uint32_t>{
                            uint32_t value=0;for(size_t i=0;i<4;++i){if(!work.known[at+i])return {};value|=uint32_t(work.raw[at+i])<<(8*i);}return value;};
                        const auto clock=word(16),timeline=word(164);
                        hardware_test_trace("# source_scene_progress,frame=%lu,event_clock_known=%u,event_clock=%lu,timeline_known=%u,timeline=%08lx,counter_owner_known=%u,counter=%u,needs_counter=%u,sparse_sample=true",
                            (unsigned long)session.frame_counter(),unsigned(clock.has_value()),(unsigned long)clock.value_or(0),
                            unsigned(timeline.has_value()),(unsigned long)timeline.value_or(0),unsigned(script.scratch_half_1f800002.has_value()),
                            unsigned(script.scratch_half_1f800002.value_or(0)),unsigned(*event_result.phase6_consumer_status==l3::SourceEventPhase6Status::NeedsInputWord));
                    }
                    }
                }
                if(event_result.command_receipt_count) {
                    const auto command=session.source_event_command_request();
                    hardware_test_trace("# source_command,frame=%lu,receipts=%u,queued=%lu,function=%08lx,type=%08lx,word04=%08lx,word08=%08lx,word0c=%08lx,playback=false,CD_status_is_not_scene_completion=true",
                        (unsigned long)session.frame_counter(),unsigned(event_result.command_receipt_count),
                        (unsigned long)session.source_event_command_queue().count,
                        (unsigned long)(command?command->source_function:0),
                        (unsigned long)(command?command->words[0]:0),(unsigned long)(command?command->words[1]:0),
                        (unsigned long)(command?command->words[2]:0),(unsigned long)(command?command->words[3]:0));
                }
                if(source_xa_requested){
                    const auto& mix=session.source_event_cd_mix_intents();size_t applied=0;
                    while(applied<mix.count&&xa_audio.apply_diagonal_mix(mix.records[applied].values))++applied;
                    if(applied)(void)session.acknowledge_source_event_cd_mix(applied);
                }
                const auto& command_result=session.source_event_command_result();
                if(g_source_event_boot&&(session.frame_counter()==1||command_result.popped||
                   command_result.status!=old_command_status||
                   session.source_event_command_queue().phase_800989c4!=old_command_phase||session.frame_counter()%60==0)) {
                    const auto& context=session.source_event_command_context();
                    hardware_test_trace("# source_command_service,frame=%lu,status=%u,committed=%u,popped=%u,phase=%ld,pending_CD=%u,param_count=%u,params=%u:%u:%u:%u,CD_status_raw=%ld,transport_receipts=%u,source_request=%08lx,missing=%08lx,native_transport=%s",
                        (unsigned long)session.frame_counter(),unsigned(command_result.status),unsigned(command_result.committed),unsigned(command_result.popped),
                        session.source_event_command_queue().phase_800989c4?long(*session.source_event_command_queue().phase_800989c4):-1L,
                        command_result.pending_request?unsigned(command_result.pending_request->command):0u,
                        command_result.pending_request?unsigned(command_result.pending_request->parameter_bytes):0u,
                        command_result.pending_request?unsigned(command_result.pending_request->parameters[0]):0u,
                        command_result.pending_request?unsigned(command_result.pending_request->parameters[1]):0u,
                        command_result.pending_request?unsigned(command_result.pending_request->parameters[2]):0u,
                        command_result.pending_request?unsigned(command_result.pending_request->parameters[3]):0u,
                        context.byte_80098ab8?long(*context.byte_80098ab8):-1L,unsigned(command_result.transport_receipts_consumed),
                        (unsigned long)(command_result.pending_source_request?*command_result.pending_source_request:0),
                        (unsigned long)(command_result.missing_source_address?*command_result.missing_source_address:0),
                        xa_audio.observation().ready?"NATIVE_XA_PLAYBACK_CANDIDATE":"UNBOUND");
                }
                if(actor_revisions.observed_source_revision!=session.conditional_spawn_revision()){
                    const u64 spawn_start=svcGetSystemTick();
                    // Group changes are loading work; steady-state tick/render
                    // does not decode or allocate actor resources.
                    bool committed=false;
                    const auto source_revision=session.conditional_spawn_revision();
                    const auto failed=[&](const char* reason){
                        char label[320];
                        std::snprintf(label,sizeof(label),
                            "LOAD_FAILURE source_revision=%lu committed_actor_revision=%lu presentation=STALE hardware=HARDWARE_PENDING reason=%s",
                            static_cast<unsigned long>(source_revision),
                            static_cast<unsigned long>(actor_revisions.committed_actor_revision),reason);
                        boot_log(MML_BOOT_DATA,label);
                    };
                    try {
                        committed=l3::reload_imported_actors_for_revision(actors,actor_revisions,
                            source_revision,[&]{return load_active_actors(session,actor_metadata);});
                    } catch(const std::exception& error){failed(error.what());}
                    catch(...){failed("non-standard actor loader exception");}
                    // A failed source revision is consumed once. Its logical
                    // group may already differ; only old presentation survives.
                    if(committed&&actor_state_fields_requested()){hardware_test_actor_action2_probe(actors,session.active_map(),session.map_area_index(),refs);hardware_test_actor_action4_probe(actors,session.active_map(),session.map_area_index(),refs);}
                    hardware_test_area_event(committed?"conditional_spawn":"conditional_spawn_LOAD_FAILURE_STALE_PRESENTATION",
                        g_current_area,
                        (svcGetSystemTick()-spawn_start)/CPU_TICKS_PER_USEC,
                        actors.decoded_resource_bytes,actors.actors.size(),source_revision);
                }
                // This diagnostic lifecycle advances initial pose0 on the first
                // admitted native tick. Original allocation-frame timing and
                // pause ownership remain unknown; stale presentation is held.
                if(actor_tick_admitted&&actor_gameplay_owned()&&!actor_revisions.failed_source_revision) {
                    if(actor_state_fields_requested()) {
                        const auto pp=session.player_position();
                        ++g_actor_update_ordinal;
                        g_children.reset_frame_queues();
                        // ST00 E0/08 targets register their 0x800341BC proxies
                        // (radius 0x50, 0x04000000) on the shared hit records.
                        for(const auto& request:session.e008_proxy_requests()){
                            l3::SourceHitProxyRegistration registration;
                            const auto& p=request.point;
                            registration.pointed_words={{uint32_t(uint16_t(p[0]))|(uint32_t(uint16_t(p[1]))<<16),uint32_t(uint16_t(p[2]))}};
                            registration.a1=0x50;registration.a2=0;registration.a3=0x8000;
                            registration.stack10=uint8_t(0xE0u|request.slot);registration.stack18=0x04000000u;
                            registration.stack1c=0x00800000u;registration.stack20=0;
                            registration.target=&session.e008_hit_targets()[request.record];registration.mask_80098798=0xffu;
                            registration.owners_800be478=g_children.proxy_owner_pointers();
                            (void)l3::source_hit_proxy_register_800341bc(g_children.hit_proxy_bank(),registration);
                            g_children.publish_proxy_owner_points();
                        }
                        // ST00 E0/05 boxes: radius 0xA0 at y-0x80, mode/word from
                        // 0x8011CB00/0x8011CAF8, stack1c 0, stack20 0x80 (0x801140BC).
                        for(const auto& request:session.e005_proxy_requests()){
                            auto& box=session.e005_boxes_mutable()[request.box];
                            l3::SourceHitProxyRegistration registration;
                            const auto& p=request.point;
                            registration.pointed_words={{uint32_t(uint16_t(p[0]))|(uint32_t(uint16_t(p[1]))<<16),uint32_t(uint16_t(p[2]))}};
                            registration.a1=0xA0;registration.a2=0;registration.a3=request.mode;
                            registration.stack10=uint8_t(0xE0u|box.slot);registration.stack18=request.word;
                            registration.stack1c=0;registration.stack20=0x80;
                            registration.target=&box.target;registration.mask_80098798=0xffu;
                            registration.owners_800be478=g_children.proxy_owner_pointers();
                            (void)l3::source_hit_proxy_register_800341bc(g_children.hit_proxy_bank(),registration);
                            g_children.publish_proxy_owner_points();
                        }
                        // E0/00 sees a cleared pool slot on its next update and
                        // raises that spawn record's flag (0x80111AD4).
                        const size_t static_objects=session.active_area().flow.objects.records.size();
                        for(auto& actor:actors.actors)
                            if(actor.source_released&&!actor.source_release_reported){
                                actor.source_release_reported=true;
                                if(actor.source_index>=static_objects){
                                    session.note_conditional_object_released(actor.source_index-static_objects);
                                    hardware_test_trace("# conditional_defeat_flag,frame=%lu,source_index=%lu,record=%lu",
                                        (unsigned long)session.frame_counter(),(unsigned long)actor.source_index,
                                        (unsigned long)(actor.source_index-static_objects));
                                }
                            }
                        for(auto& actor:actors.actors) {
                            const auto target=l3::sample_imported_actor_target(actor,pp.x,pp.z,
                                actor_angle_steps ? &*actor_angle_steps : nullptr);
                            if(!target)continue;
                            if(actor.source_reaverbot19) {
                                // ST00 subtype 0x19 (0x80104864): rand() at RA 0x801048F0,
                                // 0x8003648C while flags 0x20, then the callback.
                                const auto& st=*actor.source_reaverbot19;
                                std::optional<uint16_t> sample;
                                l3::SourceRngUpdateTransaction rb_rng;bool rb_begun=false;
                                if(g_actor_rng.admitted()&&actor.original_pool_slot<16u&&
                                   (actor.source_dispatch_flag0&actor.source_dispatch_flag6&1u)&&
                                   l3::begin_source_rng_update(g_actor_rng,rb_rng,
                                    {session.frame_counter(),g_actor_update_ordinal,l3::SourceRngUpdatePhase::Class03Callbacks})==l3::SourceRngStatus::Ready) {
                                    rb_begun=true;
                                    const auto staged=l3::stage_source_rng_class03(rb_rng,
                                        {l3::kSourceClass19RngCallerRa,0x8009f108u+uint32_t(actor.original_pool_slot)*0x3c0u,
                                         uint8_t(actor.original_pool_slot),st.state_08,st.action_09,st.phase_0a});
                                    if(staged.status==l3::SourceRngStatus::Ready)sample=staged.sample32a;
                                }
                                const auto detection_before=actor.source_detection19;
                                std::optional<bool> detected;
                                if((st.flags_348&0x20u)&&actor_metadata.detection&&
                                   pp.x>=INT16_MIN&&pp.x<=INT16_MAX&&pp.y>=INT16_MIN&&pp.y<=INT16_MAX&&pp.z>=INT16_MIN&&pp.z<=INT16_MAX) {
                                    l3::ActorDetectionPreparationInputs prepare;
                                    prepare.source.distance_work80=target->source_work_80;
                                    prepare.source.actor_raw={st.raw_14[0],st.raw_14[1],st.raw_14[2]};
                                    prepare.source.player_raw={static_cast<int16_t>(pp.x),static_cast<int16_t>(pp.y),static_cast<int16_t>(pp.z)};
                                    prepare.source.heading56=st.heading_56;
                                    prepare.source.reference=&refs;
                                    prepare.source.profile=(*actor_metadata.detection)[actor.source_detection19.profile_352];
                                    prepare.source.global_800bbd96=session.actor_detection_globals().halfword_800bbd96;
                                    prepare.source.scratch_1f800075=session.active_area().source_render_scratch_75;
                                    prepare.map=&session.active_map();prepare.area_index=session.map_area_index();
                                    prepare.normalization_lookup=actor_metadata.normalization?&*actor_metadata.normalization:nullptr;
                                    prepare.segment_bounds=actor_metadata.bounds;
                                    const auto prepared=l3::prepare_actor_detection_800364b0(actor.source_detection19,prepare);
                                    if(prepared.status==l3::ActorDetectionPreparationStatus::Ready){
                                        detected=prepared.detector.detected;actor.source_detection19=prepared.next;
                                    }
                                }
                                l3::Class20_03MotionWorld world;
                                world.map=&session.active_map();world.area_index=session.map_area_index();
                                world.references=&refs;world.bounds={-32,32,-32,32,-32,0};world.object_class=0x20;
                                l3::Class20_19Inputs in;
                                in.world=&world;in.random_sample=sample;in.work_80=target->source_work_80;
                                const auto& pw=session.compat_player().bytes;
                                const auto rd16=[&](size_t at){return int16_t(uint16_t(pw[at])|(uint16_t(pw[at+1])<<8));};
                                const auto rd32=[&](size_t at){return int32_t(uint32_t(pw[at])|(uint32_t(pw[at+1])<<8)|(uint32_t(pw[at+2])<<16)|(uint32_t(pw[at+3])<<24));};
                                in.player_raw={rd16(0x14),rd16(0x16),rd16(0x18)};
                                in.player_fixed={rd32(0x1c),rd32(0x20),rd32(0x24)};
                                in.player_heading=uint16_t(rd16(0x56))&0xfffu;
                                in.angle_steps=actor_angle_steps?&*actor_angle_steps:nullptr;
                                in.detected_8003648c=detected;
                                in.wake_flag_set=st.wake_flag_37c?session.story_flag(st.wake_flag_37c):
                                    (st.state_08==0&&st.raw_14[3]?session.story_flag(uint16_t(st.raw_14[3])):false);
                                in.sound_consumer_ready=true;
                                const uint8_t action_before=st.action_09;
                                const auto rb=l3::tick_imported_actor_reaverbot19(actor,in,refs,
                                    g_source_actor_proxy?&g_children.hit_proxy_bank():nullptr,0xffu,g_children.proxy_owner_pointers());
                                g_children.publish_proxy_owner_points();
                                if(rb.committed&&sample)(void)l3::commit_source_rng_update(g_actor_rng,rb_rng);
                                else if(rb_begun)l3::rollback_source_rng_update(rb_rng);
                                if(!rb.committed)actor.source_detection19=detection_before;
                                if(rb.committed&&rb.result.defeat_reward_80047148&&actor.source_record19&&actor.source_point_180_known){
                                    l3::EnemyDefeatDelivery delivery;
                                    auto& audio=session.source_audio_queue();
                                    if(g_children.prepare_enemy_defeat_delivery(actor.source_point_180,actor.source_record19->word00_reward,audio,delivery)==
                                       l3::ChildRuntimeStatus::SourceBackedPartial&&g_children.validate_enemy_defeat_delivery(delivery,audio)){
                                        audio=delivery.staged_audio();(void)g_children.commit_enemy_defeat_delivery(delivery);
                                    }
                                }
                                if(rb.committed&&(action_before!=actor.source_reaverbot19->action_09||rb.result.released_80031824||
                                    rb.result.sound_count||session.frame_counter()%120u==0u))
                                    hardware_test_trace("# reaverbot19,frame=%lu,source_index=%lu,state=%u,action=%u,phase=%u,hp=%d,flags=%04x,hidden=%u,detected=%d,sound=%x,proxy=%u,released=%u,pos=%d:%d:%d",
                                        (unsigned long)session.frame_counter(),(unsigned long)actor.source_index,
                                        unsigned(actor.source_reaverbot19->state_08),unsigned(actor.source_reaverbot19->action_09),
                                        unsigned(actor.source_reaverbot19->phase_0a),
                                        actor.source_reaverbot19->health_6e?int(*actor.source_reaverbot19->health_6e):-1,
                                        unsigned(actor.source_reaverbot19->flags_348),unsigned(actor.source_hidden),
                                        detected?int(*detected):-1,rb.result.sound_count?unsigned(rb.result.sounds[0]):0u,
                                        unsigned(rb.proxy_registered),unsigned(rb.result.released_80031824),
                                        int(actor.source_origin[0]),int(actor.source_origin[1]),int(actor.source_origin[2]));
                                else if(!rb.committed&&session.frame_counter()%120u==0u)
                                    hardware_test_trace("# reaverbot19_hold,frame=%lu,source_index=%lu,status=%u",
                                        (unsigned long)session.frame_counter(),(unsigned long)actor.source_index,unsigned(actor.last_reaverbot19_status));
                                continue;
                            }
                            if(actor.source_popout) {
                                // ST00 subtype 0x1F (0x801062EC): one shared rand() per
                                // frame at RA 0x80106378, then the callback. The frame
                                // after it rewrites itself, subtype-3 state 0 runs.
                                const size_t index=size_t(&actor-actors.actors.data());
                                const bool morph_frame=actor.source_popout->morph_to_class20_03;
                                std::optional<uint16_t> sample;
                                l3::SourceRngUpdateTransaction popout_rng;
                                bool rng_begun=false;
                                if(g_actor_rng.admitted()&&actor.original_pool_slot<16u&&
                                   (actor.source_dispatch_flag0&actor.source_dispatch_flag6&1u)&&
                                   l3::begin_source_rng_update(g_actor_rng,popout_rng,
                                    {session.frame_counter(),g_actor_update_ordinal,l3::SourceRngUpdatePhase::Class03Callbacks})==l3::SourceRngStatus::Ready) {
                                    rng_begun=true;
                                    const auto& p=*actor.source_popout;
                                    // CANDIDATE: the subtype-3 prologue draw (0x801022F8) on its state-0 frame.
                                    const auto staged=l3::stage_source_rng_class03(popout_rng,
                                        {morph_frame?l3::kSourceClass03RngCallerRa:l3::kSourceClass1FRngCallerRa,
                                         0x8009f108u+uint32_t(actor.original_pool_slot)*0x3c0u,
                                         uint8_t(actor.original_pool_slot),morph_frame?uint8_t(0):p.state_08,
                                         morph_frame?uint8_t(0):p.action_09,morph_frame?uint8_t(0):p.phase_0a});
                                    if(staged.status==l3::SourceRngStatus::Ready)sample=staged.sample32a;
                                }
                                if(morph_frame) {
                                    bool morphed=false;
                                    try {
                                        morphed=l3::morph_imported_actor_popout_to_class20_03(actors,index,session.active_area().ebd);
                                        if(morphed) {
                                            auto& m=actors.actors[index];
                                            if(actor_metadata.combat)(void)l3::bind_imported_actor_combat_profile(m,*actor_metadata.combat,session.source_stats_selector());
                                            prepare_actor_animation_banks(actors,session.active_area().ebd);
                                            if(g_source_actor_proxy)(void)l3::initialize_imported_actor_fresh_pose_proxy_subset(m);
                                        }
                                    } catch(const std::exception& error){boot_log(MML_BOOT_DATA,error.what());}
                                    if(sample)(void)l3::commit_source_rng_update(g_actor_rng,popout_rng);
                                    else if(rng_begun)l3::rollback_source_rng_update(popout_rng);
                                    const auto& m=actors.actors[index];
                                    hardware_test_trace("# popout_1f,frame=%lu,source_index=%lu,event=morph,ok=%d,subtype=%u,hp=%d,pos=%d:%d:%d",
                                        (unsigned long)session.frame_counter(),(unsigned long)m.source_index,int(morphed),unsigned(m.subtype),
                                        m.source_behavior&&m.source_behavior->health_6e?int(*m.source_behavior->health_6e):-1,
                                        int(m.source_origin[0]),int(m.source_origin[1]),int(m.source_origin[2]));
                                    continue;
                                }
                                const uint8_t before=actor.source_popout->action_09;
                                const auto popout=l3::tick_imported_actor_popout_1f(actor,refs,sample,target->source_work_80);
                                if(popout.committed&&sample)(void)l3::commit_source_rng_update(g_actor_rng,popout_rng);
                                else if(rng_begun)l3::rollback_source_rng_update(popout_rng);
                                if(popout.committed&&(before!=actor.source_popout->action_09||popout.morph_pending))
                                    hardware_test_trace("# popout_1f,frame=%lu,source_index=%lu,event=action,action=%u,work80=%lu,scale_z=%ld,pos=%d:%d:%d",
                                        (unsigned long)session.frame_counter(),(unsigned long)actor.source_index,
                                        unsigned(actor.source_popout->action_09),(unsigned long)target->source_work_80,
                                        long(actor.source_popout->scale_z_384),
                                        int(actor.source_origin[0]),int(actor.source_origin[1]),int(actor.source_origin[2]));
                                continue;
                            }
                            l3::Class20_03CallbackInputs source;
                            source.action.work_80=target->source_work_80;
                            if(target->source_heading_80031198 && *target->source_heading_80031198<=4095u)
                                source.action.target_heading_80031198=static_cast<uint16_t>(*target->source_heading_80031198);
                            if(target->source_player_to_actor_angle_80031198)
                                source.angle_player_to_actor_80031198=static_cast<uint16_t>(*target->source_player_to_actor_angle_80031198);
                            const auto& player_work=session.compat_player().bytes;
                            source.player_heading_800b5206=static_cast<uint16_t>(player_work[0x56]|(uint16_t(player_work[0x57])<<8));
                            source.action.packet_33c=0; // Declared diagnostic premise; no hit delivery.
                            if(actor.source_behavior) {
                                const uint16_t id=static_cast<uint16_t>(actor.source_behavior->work_362);
                                if(id<2048u)source.action.predicate_8001da58=session.story_flag(id);
                            }
                            source.combat_record_byte_a=actor.source_combat_byte_a;
                            std::optional<l3::ImportedActorDetectionPreparation> detection;
                            if(actor.source_behavior && actor.source_behavior->detection_profile_352 &&
                               pp.x>=INT16_MIN&&pp.x<=INT16_MAX&&pp.y>=INT16_MIN&&pp.y<=INT16_MAX&&
                               pp.z>=INT16_MIN&&pp.z<=INT16_MAX) {
                                const auto& state=*actor.source_behavior;
                                l3::ActorDetectionWork work{state.detection_origin_34c,
                                    *state.detection_profile_352,state.detection_phase_353,
                                    state.detection_vector_354,state.detection_countdown_35a,state.detection_offset_35b};
                                l3::ActorDetectionPreparationInputs prepare;
                                prepare.source.distance_work80=target->source_work_80;
                                prepare.source.actor_raw={state.raw_14[0],state.raw_14[1],state.raw_14[2]};
                                prepare.source.player_raw={static_cast<int16_t>(pp.x),static_cast<int16_t>(pp.y),static_cast<int16_t>(pp.z)};
                                prepare.source.heading56=state.behavior.heading_56;
                                prepare.source.reference=&refs;
                                if(actor_metadata.detection)prepare.source.profile=(*actor_metadata.detection)[work.profile_352];
                                prepare.source.global_800bbd96=session.actor_detection_globals().halfword_800bbd96;
                                prepare.source.scratch_1f800075=session.active_area().source_render_scratch_75;
                                prepare.map=&session.active_map();prepare.area_index=session.map_area_index();
                                prepare.normalization_lookup=actor_metadata.normalization?&*actor_metadata.normalization:nullptr;
                                prepare.segment_bounds=actor_metadata.bounds;
                                const auto prepared=l3::prepare_actor_detection_800364b0(work,prepare);
                                if(actor.last_detection_preparation_status!=prepared.status &&
                                   prepared.status!=l3::ActorDetectionPreparationStatus::Ready) {
                                    const auto& segment=prepared.segment0.status!=l3::RuntimeContactStatus::SourceBackedPartial
                                        ?prepared.segment0:prepared.segment1;
                                    hardware_test_line("# detector_hold,frame=%lu,area=%lu,source_index=%lu,prep=%u,phase=%u,countdown=%u,actor=%d:%d:%d,player=%d:%d:%d,origin=%d:%d:%d,vector=%d:%d:%d,work80=%lu,heading=%u,updated=%d:%d:%d,midpoint=%d:%d:%d,cell=%lu,group=%lu,primitive=%lu,code=%lu",
                                        (unsigned long)session.frame_counter(),(unsigned long)session.map_area_index(),
                                        (unsigned long)actor.source_index,unsigned(prepared.status),unsigned(work.phase_353),unsigned(work.countdown_35a),
                                        int(state.raw_14[0]),int(state.raw_14[1]),int(state.raw_14[2]),int(pp.x),int(pp.y),int(pp.z),
                                        int(work.origin_34c[0]),int(work.origin_34c[1]),int(work.origin_34c[2]),
                                        int(work.vector_354[0]),int(work.vector_354[1]),int(work.vector_354[2]),
                                        (unsigned long)target->source_work_80,unsigned(state.behavior.heading_56),
                                        int(prepared.updated_point[0]),int(prepared.updated_point[1]),int(prepared.updated_point[2]),
                                        int(prepared.midpoint[0]),int(prepared.midpoint[1]),int(prepared.midpoint[2]),
                                        (unsigned long)segment.unsupported_cell_index,(unsigned long)segment.unsupported_group_index,
                                        (unsigned long)segment.unsupported_primitive_index,(unsigned long)segment.unsupported_code);
                                }
                                actor.last_detection_preparation_status=prepared.status;
                                actor.last_detection_status=prepared.detector.status;
                                if(prepared.status==l3::ActorDetectionPreparationStatus::Ready)
                                    detection=l3::prepare_imported_actor_detection(actor,prepared.source);
                            }
                            l3::SourceRngUpdateTransaction rng_transaction;
                            if(g_actor_rng.admitted() && actor.source_behavior && actor.original_pool_slot<16u &&
                               actor.source_class==0x20 && actor.subtype==3 &&
                               (actor.source_dispatch_flag0&actor.source_dispatch_flag6&1u)) {
                                if(l3::begin_source_rng_update(g_actor_rng,rng_transaction,
                                    {session.frame_counter(),g_actor_update_ordinal,l3::SourceRngUpdatePhase::Class03Callbacks})==l3::SourceRngStatus::Ready) {
                                    const auto& state=actor.source_behavior->behavior;
                                    const auto sample=l3::stage_source_rng_class03(rng_transaction,
                                        {l3::kSourceClass03RngCallerRa,0x8009f108u+uint32_t(actor.original_pool_slot)*0x3c0u,
                                         uint8_t(actor.original_pool_slot),actor.source_behavior->state_08,state.action_09,state.phase_0a});
                                    if(sample.status==l3::SourceRngStatus::Ready)source.source_random_sample=sample.sample32a;
                                }
                            }
                            l3::ImportedActorProxyTailContext proxy_tail;
                            proxy_tail.references=&refs;
                            proxy_tail.bank=&g_children.hit_proxy_bank();
                            proxy_tail.shadows=&g_children.shadow_queue();
                            proxy_tail.map=&session.active_map();
                            proxy_tail.area_index=session.map_area_index();
                            // Preserve the existing diagnostic proxy mask. This
                            // is not a newly recovered retail mask owner.
                            proxy_tail.mask_80098798=0xffu;
                            // Leased targets need the 0x800BE478 owner table to re-register.
                            proxy_tail.owners_800be478=g_children.proxy_owner_pointers();
                            const auto* proxy_tail_ptr=g_source_actor_proxy?&proxy_tail:nullptr;
                            const uint32_t pending_actor_packet=actor.source_proxy_target.packet_00;
                            l3::ImportedActorBehaviorTick actor_tick;
                            if(actor.source_behavior&&actor.source_behavior->behavior.action_09==2) {
                                const auto movement=l3::prepare_imported_actor_action2_contact(actor,
                                    session.active_map(),session.map_area_index(),refs);
                                actor_tick=l3::tick_imported_actor_state1(actor,source,&movement,
                                    detection?&*detection:nullptr,&session.source_audio_queue(),nullptr,nullptr,nullptr,&g_children,&refs,nullptr,proxy_tail_ptr);
                            } else if(actor.source_behavior&&actor.source_behavior->behavior.action_09==3) {
                                const auto movement=l3::prepare_imported_actor_action3_contact(actor,
                                    session.active_map(),session.map_area_index(),refs,source.action.target_heading_80031198);
                                actor_tick=l3::tick_imported_actor_state1(actor,source,nullptr,
                                    detection?&*detection:nullptr,&session.source_audio_queue(),&movement,nullptr,nullptr,&g_children,&refs,nullptr,proxy_tail_ptr);
                            } else if(actor.source_behavior&&actor.source_behavior->behavior.action_09==4) {
                                const auto movement=l3::prepare_imported_actor_action4_contact(actor,
                                    session.active_map(),session.map_area_index(),refs);
                                actor_tick=l3::tick_imported_actor_state1(actor,source,nullptr,
                                    detection?&*detection:nullptr,&session.source_audio_queue(),nullptr,nullptr,&movement,&g_children,&refs,nullptr,proxy_tail_ptr);
                            } else if(actor.source_behavior&&actor.source_behavior->behavior.action_09==9) {
                                const auto movement=l3::prepare_imported_actor_action9_contact(actor,
                                    session.active_map(),session.map_area_index(),refs,source.action.work_80,
                                    source.action.target_heading_80031198,detection?std::optional<bool>(detection->result.detected):std::nullopt);
                                actor_tick=l3::tick_imported_actor_state1(actor,source,nullptr,
                                    detection?&*detection:nullptr,&session.source_audio_queue(),nullptr,&movement,nullptr,&g_children,&refs,nullptr,proxy_tail_ptr);
                            } else if(actor.source_behavior&&(actor.source_behavior->behavior.action_09==7||actor.source_behavior->behavior.action_09==8)) {
                                source.reaction_profile=actor_metadata.reaction?&*actor_metadata.reaction:nullptr;
                                const auto reaction=l3::prepare_imported_actor_reaction_contact(actor,session.active_map(),session.map_area_index(),refs,
                                    source.reaction_profile,actor_angle_steps?&*actor_angle_steps:nullptr,source.action.target_heading_80031198,&session.source_audio_queue());
                                actor_tick=l3::tick_imported_actor_state1(actor,source,nullptr,detection?&*detection:nullptr,
                                    &session.source_audio_queue(),nullptr,nullptr,nullptr,&g_children,&refs,&reaction,proxy_tail_ptr);
                            } else actor_tick=l3::tick_imported_actor_state1(actor,source,nullptr,
                                detection?&*detection:nullptr,&session.source_audio_queue(),nullptr,nullptr,nullptr,&g_children,&refs,nullptr,proxy_tail_ptr);
                            if(proxy_tail_ptr)g_children.publish_proxy_owner_points();
                            const bool packet_consumed=pending_actor_packet&&actor_tick.committed&&actor_tick.proxy_finalized;
                            if(packet_consumed)++actor_packet_commits;
                            if(g_source_actor_proxy&&((packet_consumed&&actor_packet_commits<=3u)||session.frame_counter()%60u==0u))
                                hardware_test_trace("# source_actor_contact_commit,frame=%lu,slot=%lu,work00=%lu,packet_before=%08lx,packet_after=%08lx,body=%u,committed=%u,proxy_finalized=%u,proxy_registered=%u,clear_receipts=%llu,mask=FRONTEND_DIAGNOSTIC,retail_encounter=false",
                                    (unsigned long)session.frame_counter(),(unsigned long)actor.original_pool_slot,
                                    (unsigned long)actor.source_behavior->work_00,(unsigned long)pending_actor_packet,
                                    (unsigned long)actor.source_proxy_target.packet_00,unsigned(actor_tick.status),
                                    unsigned(actor_tick.committed),unsigned(actor_tick.proxy_finalized),unsigned(actor_tick.proxy_registered),
                                    (unsigned long long)actor_packet_commits);
                            if(rng_transaction.active()) {
                                if(rng_transaction.call_count()) {
                                    const auto call=rng_transaction.calls()[0];
                                    if(actor_tick.committed) {
                                        const auto status=l3::commit_source_rng_update(g_actor_rng,rng_transaction);
                                        if(status!=l3::SourceRngStatus::Ready)throw std::runtime_error("source RNG transaction publication failed");
                                    } else l3::rollback_source_rng_update(rng_transaction);
                                    if(session.frame_counter()%60u==0u)
                                        hardware_test_trace("# actor_rng_candidate,frame=%lu,update=%llu,slot=%u,ordinal=%llu,seed_before=%lu,seed_after=%lu,sample=%u,committed=%u,body=%u,retail_parity=false",
                                            (unsigned long)session.frame_counter(),(unsigned long long)g_actor_update_ordinal,unsigned(call.actor.slot),
                                            (unsigned long long)call.native_global_ordinal,(unsigned long)call.seed_before32,(unsigned long)call.seed_after32,
                                            unsigned(call.sample32a),unsigned(actor_tick.committed),unsigned(actor_tick.status));
                                } else l3::rollback_source_rng_update(rng_transaction);
                            }
                            if(actor_tick.committed&&actor_tick.notify_detection_global)session.notify_actor_detection();
                            if(actor_tick.defeated)
                                hardware_test_trace("# source_enemy_defeat,frame=%lu,slot=%lu,effect128_65=%u,effect_exhausted=%u,reward80047228=%u,reward_queue=%lu,released80031824=true,sound161=%u,reward_effect135_rng_consumer=UNKNOWN,hardware_verified=false",
                                    (unsigned long)session.frame_counter(),(unsigned long)actor.original_pool_slot,
                                    unsigned(actor_tick.defeat_effect_allocated),unsigned(actor_tick.defeat_effect_exhausted),
                                    unsigned(actor_tick.reward_enqueued),(unsigned long)g_children.enemy_reward_queue().count,
                                    unsigned(actor_tick.source_audio_queued||actor_tick.source_audio_dropped));
                            if(actor_tick.committed&&g_source_actor_proxy&&!actor_tick.proxy_finalized) {
                                const auto proxy=l3::tick_imported_actor_pose_proxy(actor,refs,g_children.hit_proxy_bank(),0xffu,
                                    g_children.proxy_owner_pointers());
                                g_children.publish_proxy_owner_points();
                                if(session.frame_counter()%60u==0u)
                                    hardware_test_trace("# source_actor_proxy,frame=%lu,slot=%lu,pose=%u,tail=%u,committed=%u,registered=%u,full=%u,point=%d:%d:%d,player_Buster_GTE=UNKNOWN",
                                        (unsigned long)session.frame_counter(),(unsigned long)actor.original_pool_slot,
                                        unsigned(proxy.pose_status),unsigned(proxy.proxy.status),unsigned(proxy.committed),
                                        unsigned(proxy.proxy.registered),unsigned(proxy.proxy.bank_full),
                                        actor.source_behavior->proxy_position_180[0],actor.source_behavior->proxy_position_180[1],
                                        actor.source_behavior->proxy_position_180[2]);
                            }
                        }
                        const auto allocated=g_children.pool().allocated_bitmap;
                        for(size_t child_slot=0;child_slot<l3::class60_pool_capacity;++child_slot) {
                            if(!(allocated&(0x80000000u>>child_slot)))continue;
                            l3::ChildFrameContext context;
                            context.map=&session.active_map();context.area_index=session.map_area_index();
                            context.references=&refs;context.profile=actor_metadata.child6?&*actor_metadata.child6:nullptr;
                            const auto& pw=session.compat_player().bytes;
                            context.player_heading_800b5206=uint16_t(pw[0x56]|(uint16_t(pw[0x57])<<8));
                            // Native fresh logical80033EBC owner. No unported
                            // mask leases/owner pointers are acknowledged here.
                            context.proxy_mask_80098798=0xffu;
                            context.audio=&session.source_audio_queue();
                            const auto identity=0x8009cf88u+uint32_t(child_slot)*268u;
                            const auto tick=g_children.tick(identity,context);
                            if(session.frame_counter()%60u==0u || tick.released)
                                hardware_test_trace("# source_child,frame=%lu,slot=%lu,status=%u,lifecycle=%u,committed=%u,proxy=%u,registry=%u,shadow=%u,mesh=false,effect128_later_consumers=UNKNOWN",
                                    (unsigned long)session.frame_counter(),(unsigned long)child_slot,unsigned(tick.status),unsigned(tick.lifecycle_status),
                                    unsigned(tick.committed),unsigned(tick.proxy_status),unsigned(tick.registry_status),unsigned(tick.shadow_status));
                        }
                        if(g_source_actor_proxy) {
                            l3::SourcePlayerPoseInputs pose_input;
                            pose_input.skeleton=&skeleton;pose_input.reference=&refs;
                            pose_input.scratch_shift_1f800074=0;
                            const auto& player_work=session.compat_player().bytes;
                            pose_input.limb_pose_selector_106=player_work[0x106];
                            const auto* pose_frame=l3::pbd_main_body_track_frame(animation,player_track);
                            std::optional<l3::PbdMainPoseBank> pose_bank;
                            if(pose_frame){pose_bank=l3::decode_pbd_main_pose_bank(*pose_frame);pose_input.primary_pose=&*pose_bank;}
                            const auto player_position=session.player_position();
                            if(player_position.x>=INT16_MIN&&player_position.x<=INT16_MAX&&
                               player_position.y>=INT16_MIN&&player_position.y<=INT16_MAX&&
                               player_position.z>=INT16_MIN&&player_position.z<=INT16_MAX)
                                pose_input.player_position_14={{static_cast<int16_t>(player_position.x),
                                    static_cast<int16_t>(player_position.y),static_cast<int16_t>(player_position.z)}};
                            pose_input.heading_56=uint16_t(player_work[0x56]|(uint16_t(player_work[0x57])<<8));
                            pose_input.heading_offset_116=uint16_t(player_work[0x116]|(uint16_t(player_work[0x117])<<8));
                            const auto integer_pose=l3::produce_source_player_integer_pose_800325d4(pose_input);
                            const auto muzzle=l3::source_player_muzzles_800331a0(integer_pose);
                            const bool muzzle_bound=l3::bind_source_player_muzzles_80035724(muzzle,source_buster_player);
                            // RuntimeSession owns these current native work
                            // fields even when the event-side known mask does
                            // not. They are the exact scalar owners consumed by
                            // original80035724, not inferred render state.
                            source_buster_player.class_01=player_work[1];
                            source_buster_player.heading_56=pose_input.heading_56;

                            const auto pack_xyz=[](int32_t x,int32_t y,int32_t z) {
                                const auto short_value=[](int32_t value) {
                                    return static_cast<uint16_t>(static_cast<int16_t>(
                                        std::clamp(value,int32_t(INT16_MIN),int32_t(INT16_MAX))));
                                };
                                return std::array<uint32_t,2>{{uint32_t(short_value(x))|
                                    (uint32_t(short_value(y))<<16),uint32_t(short_value(z))}};
                            };
                            l3::SourceBusterFrameContext buster_frame;
                            if(pose_input.player_position_14)
                                buster_frame.owner_position_14=pack_xyz((*pose_input.player_position_14)[0],
                                    (*pose_input.player_position_14)[1],(*pose_input.player_position_14)[2]);
                            if(muzzle_bound&&pose_input.player_position_14) {
                                // +400/+418 are written by an untraced player
                                // aiming owner after the proved pose pass. Keep
                                // this native forward-ray bridge explicit and
                                // replace it when that source writer is found.
                                constexpr int32_t candidate_ray_length=0x800;
                                const auto& direction=refs.trig[pose_input.heading_56&0x0fffu];
                                const int32_t dx=-int32_t((int64_t(direction.sin_q12)*candidate_ray_length)/4096);
                                const int32_t dz=-int32_t((int64_t(direction.cos_q12)*candidate_ray_length)/4096);
                                buster_frame.owner_endpoint_400=pack_xyz(
                                    int16_t(muzzle.muzzle_3c8_part4[0])+dx,
                                    int16_t(muzzle.muzzle_3c8_part4[1]),
                                    int16_t(muzzle.muzzle_3c8_part4[2])+dz);
                                buster_frame.owner_endpoint_418=pack_xyz(
                                    int16_t(muzzle.muzzle_3e0_part7[0])+dx,
                                    int16_t(muzzle.muzzle_3e0_part7[1]),
                                    int16_t(muzzle.muzzle_3e0_part7[2])+dz);
                            }
                            std::optional<uint32_t> source_buster_busy_198=session.source_buster_busy_198();
                            buster_frame.owner_busy_198=&source_buster_busy_198;
                            buster_frame.proxy_mask_80098798=&source_buster_proxy_mask;
                            // The original +78 bounds owner is still unknown.
                            // Admit only the actor-proxy chain in this pass;
                            // never label the clear map receipt as retail map collision.
                            buster_frame.map_collision_80049c24=false;
                            buster_frame.audio=&session.source_audio_queue();
                            // Subtype 0x81 (Square/Y lane +171): weapon row 1 at the
                            // player's +4FC/+4FE levels; its own wall test reads the map.
                            buster_frame.map=&session.active_map();buster_frame.area_index=session.map_area_index();
                            buster_frame.references=&refs;
                            buster_frame.shot81=l3::source_buster_shot81_stats(source_buster_profile,
                                int8_t(player_work[0x4fc]),int8_t(player_work[0x4fe]));
                            const auto buster_tick=g_children.tick_source_busters_80017154(buster_frame);
                            if(source_buster_busy_198)
                                session.set_source_buster_busy_198(*source_buster_busy_198);
                            // 0.0.15.10: 0x1F800008 comes from the matrix half of the
                            // original gameplay producer 0x80037FB4 (session camera).
                            const auto& follow_camera=session.source_gameplay_camera();
                            const auto source_camera=follow_camera.initialized&&
                                session.camera().owner==l3::RuntimeCameraOwner::Gameplay?
                                std::optional<std::array<uint32_t,8>>(follow_camera.camera_1f800008):std::nullopt;
                            l3::SourceBusterSpawn buster_spawn;
                            std::optional<l3::SourcePlayerBusterResult> lock_on;
                            if(source_camera&&muzzle_bound&&pose_input.player_position_14&&actor_metadata.target&&
                               actor_metadata.target->image_target_mode_80097d02) {
                                // Original order: 0x80034EB8 consumes the leases
                                // (0x80035508), selects, then spawns via 0x80035724.
                                l3::SourcePlayerBusterInputs select;
                                // CANDIDATE: the writer of player work+00 bit0 is
                                // untraced; a gameplay-owned player stands in.
                                select.active=true;
                                select.target_mode_49e=*actor_metadata.target->image_target_mode_80097d02;
                                // CANDIDATE, as before: +0x174 bit 0x10 comes from an
                                // untraced animation callback; the Y edge stands in.
                                // 0x80034EB8: +174 bit 0x20 fires lane +171 (stack10 0),
                                // the measured Square shot; bit 0x10 is the +170 lane.
                                select.fire_flags_174=uint8_t(source_buster_fire_pressed?0x20u:0x00u);
                                select.player_point_14=*pose_input.player_position_14;
                                select.target_profile_8008994c=*actor_metadata.target;
                                select.camera_1f800008=source_camera;
                                lock_on=l3::source_player_select_and_spawn_80034eb8(g_children,
                                    g_source_player_gte,source_buster_player,source_buster_proxy_mask,select);
                                // Lane 0 (+174 bit 0x20) is the Square shot; lane 1 is +170.
                                if(lock_on->spawn_attempted[0])buster_spawn=lock_on->spawns[0];
                                else if(lock_on->spawn_attempted[1])buster_spawn=lock_on->spawns[1];
                            }
                            using LockStatus=l3::SourcePlayerBusterStatus;
                            const bool lock_on_ran=lock_on&&(lock_on->status==LockStatus::SourceBackedPartial||
                                lock_on->status==LockStatus::ProjectionHeld||lock_on->status==LockStatus::BusterHeld);
                            if(!lock_on_ran&&source_buster_fire_pressed&&muzzle_bound)
                                buster_spawn=g_children.spawn_source_buster_80035724(
                                    source_buster_player,0,-1,source_buster_proxy_mask);
                            if(source_buster_fire_pressed||buster_tick.committed||buster_tick.held)
                                hardware_test_trace("# source_buster_runtime,frame=%lu,fire_edge=%u,spawn_status=%u,identity=%08lx,callback_status=%u,committed=%u,held=%u,registered=%u,impacts=%u,map_impacts=%u,released=%u,live=%u,producer=80035724,lane=171,callback=800437A8,damage=%d,range=%d,homing=UNPORTED,retail_cadence=false",
                                    (unsigned long)session.frame_counter(),unsigned(source_buster_fire_pressed),
                                    unsigned(buster_spawn.status),(unsigned long)buster_spawn.source_identity,
                                    unsigned(buster_tick.status),unsigned(buster_tick.committed),unsigned(buster_tick.held),
                                    unsigned(buster_tick.registered),unsigned(buster_tick.impacts),unsigned(buster_tick.map_impacts),unsigned(buster_tick.released),
                                    unsigned(__builtin_popcount(g_children.buster_pool().allocated_800989f0)),
                                    buster_frame.shot81?int(buster_frame.shot81->damage_800375d4):-1,buster_frame.shot81?int(buster_frame.shot81->range_80037634):-1);
                            if(lock_on&&(source_buster_fire_pressed||session.frame_counter()%60u==0u))
                                hardware_test_trace("# source_lock_on,frame=%lu,status=%u,proxy_status=%u,projected=%lu,broad=%ld,window=%ld,player_depth_plus_50=%u,camera=80037FB4,pad_1f8000f2=RETAINED_UNKNOWN,shift=%u,packed_trig_substitutions=%lu,active=CANDIDATE,fire_174=CANDIDATE_Y,target_mode=IMAGE_80097D02_CANDIDATE",
                                    (unsigned long)session.frame_counter(),unsigned(lock_on->status),unsigned(lock_on->proxy_status),
                                    (unsigned long)lock_on->projected_proxies,(long)lock_on->selected_proxy[0],(long)lock_on->selected_proxy[1],
                                    unsigned(lock_on->player_depth_plus_50),unsigned(follow_camera.scratch_shift_1f800074),
                                    (unsigned long)follow_camera.packed_trig_substitutions);
                            // Without the lock-on inputs the leases are consumed
                            // alone, still with the source camera words.
                            const auto leases=lock_on_ran?l3::SourcePlayerLeaseResult{lock_on->proxy_status}:
                                g_children.consume_player_leases(g_source_player_gte,source_camera,source_buster_proxy_mask);
                            if(session.frame_counter()%60u==0u)
                                hardware_test_trace("# source_player_leases,frame=%lu,status=%u,projected=%lu,pose=%u,muzzle=%u,profile=L3BP1,camera_owner=80037FB4,projection_owner=8007A4F0+80011F48,Buster_selection=%s",
                                    (unsigned long)session.frame_counter(),unsigned(leases.status),(unsigned long)leases.projected,
                                    unsigned(integer_pose.status),unsigned(muzzle_bound),lock_on_ran?"80034EB8":"NONE");
                        }
                        if(actor_metadata.hit_proxy_rules) {
                            const auto& bank=g_children.hit_proxy_bank();
                            const size_t bank_rows=bank.count;
                            const bool buster_live=g_children.buster_pool().allocated_800989f0!=0;
                            char rows[256];size_t at=0;rows[0]=0;
                            for(size_t r=0;buster_live&&r<bank_rows&&r<8&&at+32<sizeof rows;++r)
                                at+=size_t(std::snprintf(rows+at,sizeof rows-at,"%08lx@%d:%d:%d/%u;",
                                    (unsigned long)bank.records[r].encoded_identity,int(bank.records[r].point[0]),
                                    int(bank.records[r].point[1]),int(bank.records[r].point[2]),unsigned(bank.records[r].encoded_extent)));
                            const auto pairs=g_children.consume_proxy_pairs(actor_metadata.hit_proxy_rules);
                            if(buster_live||pairs.status!=l3::SourceHitProxyStatus::SourceBackedPartial)
                                hardware_test_trace("# source_proxy_pairs,frame=%lu,status=%u,rows=%lu,admitted=%lu,overlapping=%lu,records=%s",
                                    (unsigned long)session.frame_counter(),unsigned(pairs.status),(unsigned long)bank_rows,
                                    (unsigned long)pairs.admitted_pairs,(unsigned long)pairs.overlapping_pairs,rows);
                        }
                        // Pair packets belong to the next original callback.
                        // Retain them; the body deliberately holds until all
                        // hit/reaction/defeat consumers are available.
                        if(g_source_actor_proxy)for(auto& actor:actors.actors)
                            if(actor.source_proxy_bound&&actor.source_behavior)
                                actor.source_behavior->packet_33c=actor.source_proxy_target.packet_00;
                        l3::SourceEffect128Inputs effect_inputs;
                        effect_inputs.audio=&session.source_audio_queue();
                        if(g_actor_rng.admitted()&&!g_source_scene_rng_replay){
                            effect_inputs.rng=&g_actor_rng;effect_inputs.rng_frame=session.frame_counter();
                            effect_inputs.source_generation=session.frame_counter()+1;
                        }
                        effect_inputs.impact82_timers_8008a240=source_buster_profile.impact82_timers_8008a240;
                        const auto effects=g_children.tick_effects(effect_inputs);
                        // 0x80047228 -> 0x80046B34, then subtype-0x87 pickups.
                        if(source_reward_profile.loaded&&g_actor_rng.admitted()&&!g_source_scene_rng_replay) {
                            for(int n=0;n<8&&g_children.enemy_reward_queue().count;++n) {
                                const auto ex=g_children.expand_next_enemy_reward(source_reward_profile,refs,g_actor_rng,session.frame_counter());
                                if(ex.status!=l3::SourceRewardStatus::SourceBackedPartial)break;
                                hardware_test_trace("# source_reward_expand,frame=%lu,spawned=%u,discarded=%u,pool_full=%u,source=0x80046B34,rng=CANDIDATE_SHARED",
                                    (unsigned long)session.frame_counter(),unsigned(ex.spawned),unsigned(ex.discarded),unsigned(ex.pool_full));
                            }
                            const auto pp=session.player_position();
                            l3::SourceRewardWorld reward_world;reward_world.map=&session.active_map();reward_world.area_index=session.map_area_index();
                            reward_world.player={int16_t(pp.x),int16_t(pp.y),int16_t(pp.z)};
                            reward_world.stats_selector_800c1b6b=int8_t(session.source_stats_selector().value_or(1));
                            reward_world.audio=&session.source_audio_queue();
                            int32_t money=session.source_money_800c1b2c();int16_t hp=int16_t(session.hp());
                            const auto pickups=g_children.tick_reward_pickups(source_reward_profile,refs,g_actor_rng,
                                session.frame_counter(),reward_world,money,hp,int16_t(session.hp_max()));
                            if(pickups.collected)session.apply_source_reward_collection(money,hp);
                            if(pickups.collected||pickups.released||(pickups.updated&&session.frame_counter()%15u==0u))
                            {
                                // First live pickup's +38/3A/3C and state, for placement checks.
                                int px=0,py=0,pz=0,pst=-1;
                                for(const auto& e:g_children.effect_pool().slots)if(e[0]&&e[2]==0x87){
                                    px=int16_t(uint16_t(e[0x38]|(e[0x39]<<8)));py=int16_t(uint16_t(e[0x3a]|(e[0x3b]<<8)));
                                    pz=int16_t(uint16_t(e[0x3c]|(e[0x3d]<<8)));pst=e[8];break;}
                                hardware_test_trace("# source_reward_pickups,frame=%lu,updated=%u,held=%u,collected=%u,released=%u,money=%ld,hp=%u,money_awarded=%ld,energy_awarded=%ld,first=%d:%d:%d,state=%d,player=%d:%d:%d",
                                    (unsigned long)session.frame_counter(),unsigned(pickups.updated),unsigned(pickups.held),unsigned(pickups.collected),
                                    unsigned(pickups.released),(long)session.source_money_800c1b2c(),unsigned(session.hp()),
                                    (long)pickups.money_awarded,(long)pickups.energy_awarded,px,py,pz,pst,
                                    int(reward_world.player[0]),int(reward_world.player[1]),int(reward_world.player[2]));
                            }
                        }
                        if((effects.committed||effects.held) && session.frame_counter()%60u==0u)
                            hardware_test_trace("# source_effects,frame=%lu,considered=%lu,committed=%lu,held=%lu,retail_playback=false",
                                (unsigned long)session.frame_counter(),(unsigned long)(effects.committed+effects.held),
                                (unsigned long)effects.committed,(unsigned long)effects.held);
                        g_children.consume_pending_releases();
                    } else (void)l3::tick_imported_actor_request0(actors);
                }
                if (retail_animation_requested()) (void)l3::runtime_animation_finish(animation,player_track,session);
                if((g_source_loop_replay||g_source_scene_rng_replay)&&session.source_recorded_scene_admitted())
                    (void)session.service_source_main_loop_tail_80011ecc();
            }
            hardware_test_clock_event(gameplay_clock);
            // An explicit native presentation scheduler pass, independent of
            // catch-up movement ticks. Original-vs-native cadence is unproven.
            if(g_source_event_boot&&use_uv_renderer()&&!ui_focus) {
                const auto fade=session.service_source_fade_scheduler();
                if(fade.committed||session.source_fade_callback_state().running) {
                    const auto image=session.source_fade_presentation();
                    hardware_test_trace("# source_fade_callback,frame=%lu,status=%u,committed=%u,ack=%u,running=%u,iterations=%lu,terminated=%u,presentation=%u,grayscale=%u,native_scheduler_pass=true,cadence=SOURCE_BACKED_PARTIAL",
                        (unsigned long)session.frame_counter(),unsigned(fade.status),unsigned(fade.committed),
                        unsigned(fade.native_scheduler_acknowledged),unsigned(session.source_fade_callback_state().running),
                        (unsigned long)session.source_fade_callback_state().committed_iterations,
                        unsigned(fade.source_callback_terminated),unsigned(image.status),unsigned(image.grayscale));
                }
            }
            mml_diag_phase(&g_runtime_diag,MML_PHASE_UPDATE,static_cast<uint32_t>((svcGetSystemTick()-update_start)/CPU_TICKS_PER_USEC));

            const auto cell_position=session.player_position();
            if(l3::runtime_map_find_cell(session.active_map(),session.map_area_index(),cell_position.x,cell_position.z)) last_valid=cell_position;
            else (void)session.recover_player_position_diagnostic(last_valid);

            const auto pp=session.player_position();
            auto inspection_position=pp;auto inspection_yaw=session.player_yaw();
            bool inspect_scene_actor=false;
            // Explicit diagnostic proof view only. Keep source camera mode
            // untouched; this does not publish player wrapper stores, move the
            // actor or establish retail camera/lifecycle parity.
            if(g_hardware_scene_draw==HardwareSceneDraw::All&&g_source_scene_actor40_only&&!g_source_scene_camera&&scene_actor40&&scene_actor40_pose_ready&&
               session.source_event_phase5_resources().resident_in(session.active_area().ebd)) {
                const auto& work=session.source_event_phase5_context().pool40.slots[0];
                bool known=true;for(size_t offset=20;offset<26;++offset)known&=work.known[offset]!=0;
                known&=work.known[86]!=0&&work.known[87]!=0;
                if(known) {
                    const auto half=[&](size_t offset){const auto bits=uint16_t(work.raw[offset])|(uint16_t(work.raw[offset+1])<<8);return int32_t(bits<32768?int32_t(bits):int32_t(bits)-65536);};
                    inspection_position={half(20),half(22),half(24)};inspection_yaw=uint16_t(half(86))&0xfffu;inspect_scene_actor=true;
                }
            }
            auto camera=l3::frontend_follow_camera(
                l3::frontend_world_point(float(inspection_position.x),float(inspection_position.y),float(inspection_position.z)),
                inspection_yaw,g_view_settings);
            // 0.0.15.9: the original follow camera (0x80038894 eye and look-at)
            // replaces the rigid native follow whenever gameplay owns the view.
            const auto& source_follow=session.source_gameplay_camera();
            const bool source_follow_view=!inspect_scene_actor&&source_follow.initialized&&
                source_follow.follow_frames_80098668>0&&session.camera().owner==l3::RuntimeCameraOwner::Gameplay;
            if(source_follow_view) {
                camera.eye=l3::frontend_world_point(float(source_follow.eye_800a39f0[0]),
                    float(source_follow.eye_800a39f0[1]),float(source_follow.eye_800a39f0[2]));
                camera.target=l3::frontend_world_point(float(source_follow.target_800a39a0[0]),
                    float(source_follow.target_800a39a0[1]),float(source_follow.target_800a39a0[2]));
                if(session.frame_counter()%120u==0u)
                    hardware_test_trace("# source_follow_camera,frame=%lu,eye=%ld,%ld,%ld,target=%ld,%ld,%ld,yaw=%ld,distance=%ld,unsupported_eye_tests=%lu,source=0x80038894",
                        (unsigned long)session.frame_counter(),(long)source_follow.eye_800a39f0[0],(long)source_follow.eye_800a39f0[1],
                        (long)source_follow.eye_800a39f0[2],(long)source_follow.target_800a39a0[0],(long)source_follow.target_800a39a0[1],
                        (long)source_follow.target_800a39a0[2],(long)(source_follow.yaw_800a3968&0xfff),(long)source_follow.distance_800a3964,
                        (unsigned long)source_follow.unsupported_eye_tests);
            }
            if(inspect_scene_actor&&session.frame_counter()%60u==0u)
                hardware_test_trace("# source_actor40_inspection_camera,frame=%lu,x=%ld,y=%ld,z=%ld,camera=FRONTEND_DIAGNOSTIC,retail_camera=false,source_actor_moved=false",
                    (unsigned long)session.frame_counter(),(long)inspection_position.x,(long)inspection_position.y,(long)inspection_position.z);
            const C3D_FVec eye=FVec3_New(camera.eye.x,camera.eye.y,camera.eye.z);
            const C3D_FVec at=FVec3_New(camera.target.x,camera.target.y,camera.target.z);
            const C3D_FVec up=FVec3_New(0.0f,1.0f,0.0f);
            C3D_Mtx view{};Mtx_LookAt(&view,eye,at,up,false);
            C3D_Mtx frame_projection=g_projection;
            bool source_camera_bound=false;
            if(g_source_event_boot&&g_source_scene_camera&&use_uv_renderer()&&actor_metadata.scene_view&&
               session.source_event_phase5_resources().resident_in(session.active_area().ebd)) {
                auto source_view=l3::produce_source_scene_view_8001e550({&*actor_metadata.scene_view,&refs,
                    &session.source_event_phase5_context(),&session.source_event_phase6_context(),
                    session.source_event_phase5_resources().resident_in(session.active_area().ebd)});
                if(source_view.frame&&(source_view.frame->center_x_pixels!=200.f||source_view.frame->center_y_pixels!=120.f)) {
                    source_view.status=l3::SourceSceneViewStatus::UnsupportedOptics;source_view.frame.reset();
                }
                if(source_view.frame) {
                    const auto& f=*source_view.frame;
                    for(size_t row=0;row<4;++row)view.r[row]=FVec4_New(f.native_view_row_major[row*4],
                        f.native_view_row_major[row*4+1],f.native_view_row_major[row*4+2],f.native_view_row_major[row*4+3]);
                    Mtx_PerspTilt(&frame_projection,C3D_AngleFromDegrees(f.vertical_fov_degrees),
                        l3::kFrontendAspect,l3::kFrontendNear,l3::kFrontendFar,false);
                    source_camera_bound=true;
                }
                if(unsigned(source_view.status)!=last_source_view_status||session.frame_counter()%60u==0u) {
                    last_source_view_status=unsigned(source_view.status);
                    hardware_test_trace("# source_scene_view,frame=%lu,status=%u,bound=%u,missing_camera_offset=%u,focal_pixels=%.1f,source_window=320x240,frontend=400x240,clip=CANDIDATE,unsupported=FRONTEND_DIAGNOSTIC,parity=false",
                        (unsigned long)session.frame_counter(),unsigned(source_view.status),unsigned(source_camera_bound),
                        unsigned(source_view.missing_camera_offset.value_or(0xffffu)),source_view.frame?double(source_view.frame->focal_pixels):0.);
                }
            }


            // 0.0.15.8: door objects swap IDX descriptors (0x8003AC24); show
            // the live descriptor without rebuilding the area. The door-leaf
            // rotation of state 2 (0x801126DC) is UNKNOWN/unported, so the
            // slab switches at the IDX write.
            if(session.idx_descriptor_generation()!=g_terrain_patch_generation){
                patch_terrain_descriptors(session.active_map(),render,g_current_area);
                g_terrain_patch_generation=session.idx_descriptor_generation();
            }
            const u64 render_start=svcGetSystemTick();
            // Frame ownership was acquired before resource-changing updates.
            C3D_RenderTargetClear(g_top_target,C3D_CLEAR_ALL,kClearColor,0);
            if(!C3D_FrameDrawOn(g_top_target)){
                submit_gpu_frame();
                throw std::runtime_error("top screen: C3D_FrameDrawOn rejected");
            }
            C3D_BindProgram(&g_shader_program);
            // World vertices are (x, -y, z) and Mtx_LookAt is a rotation, so the
            // LookAt path showed the scene mirrored left-right (facing -z the PS1
            // GTE has screen-right = -x). Negate camera-space x to restore the
            // GTE's handedness. The source scene view already carries the exact
            // diag(1,-1,-1)*R*diag(1,-1,1) conversion and is left as is.
            if(!source_camera_bound)
                for(auto& row:frame_projection.r)row.x=-row.x;
            C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER,g_u_projection,&frame_projection);
            C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER,g_u_model_view,&view);
            if(g_hardware_scene_draw!=HardwareSceneDraw::PlayerOnly){
                // The original drops back faces at NCLIP; only the gameplay
                // camera has the 0x1F800008 words the face test needs.
                std::optional<l3::TerrainFaceCamera> faces;
                const auto mode=session.active_area().source_render_scratch_75;
                // The camera words use scratch shift 0x1F800074; cooked terrain is unshifted.
                if(source_follow_view&&!source_camera_bound&&mode&&source_follow.scratch_shift_1f800074==0)
                    faces=l3::make_terrain_face_camera(source_follow.camera_1f800008);
                draw_terrain(faces?&*faces:nullptr,mode.value_or(0));
                if(session.frame_counter()%120u==0u)
                    hardware_test_trace("# terrain_faces,frame=%lu,classified=%u,mode75=%d,drawn=%lu,black=%lu,skipped=%lu,unsupported=%lu,unclassified_batches=%lu,threshold=%d,source=NCLIP_80025900_OBSERVED",
                        (unsigned long)session.frame_counter(),unsigned(faces.has_value()),mode?int(*mode):-1,
                        (unsigned long)g_terrain_face_stats.drawn,(unsigned long)g_terrain_face_stats.black,
                        (unsigned long)g_terrain_face_stats.skipped,(unsigned long)g_terrain_face_stats.unsupported,
                        (unsigned long)g_terrain_face_stats.batches_unclassified,int(l3::kObservedArea1BackFaceThreshold));
            }
            // Independent renderer proof filter. A mixed gameplay/scene image
            // cannot prove which imported owner is visible. It defaults OFF and
            // never acknowledges an original player/actor lifecycle operation.
            const bool isolate_scene_actor=g_source_scene_actor40_only&&g_source_scene_actor40&&
                use_uv_renderer()&&scene_actor40&&scene_actor40_prefix_ready&&scene_actor40_pose_ready&&
                session.source_event_phase5_resources().resident_in(session.active_area().ebd);
            const bool draw_legacy_player=!isolate_scene_actor&&g_hardware_scene_draw!=HardwareSceneDraw::TerrainOnly;
            if(draw_legacy_player) {
                if (use_uv_renderer())
                    draw_player_uv(local_visuals,player_material_words,player_pose_storage,skeleton,fallback_action,animation,player_track,refs,session);
                else draw_player_0143(local_visuals,skeleton,fallback_action,refs,session);
            }
            if(session.frame_counter()%60u==0u&&scene_actor40)
                hardware_test_trace("# source_actor40_visibility_filter,frame=%lu,isolate_selected=%u,legacy_player_drawn=%u,retail_actor_identity=UNKNOWN",
                    (unsigned long)session.frame_counter(),unsigned(g_source_scene_actor40_only),unsigned(draw_legacy_player));
            if (use_uv_renderer()&&g_hardware_scene_draw==HardwareSceneDraw::All) draw_imported_actors_uv(actors);
            if(scene_e0&&(!scene_e0->resident_in(session.active_area().ebd)||!session.source_event_phase5_resources().resident_in(session.active_area().ebd))){scene_e0.reset();release_scene_materials();}
            if(g_hardware_scene_draw==HardwareSceneDraw::All&&scene_actor40&&scene_actor40_prefix_ready&&scene_actor40_pose_ready&&session.source_event_phase5_resources().resident_in(session.active_area().ebd)) {
                const auto count=draw_source_scene_actor40_uv(session,*scene_actor40,refs);
                if(session.frame_counter()%60u==0u){const auto* pose=l3::select_source_scene_actor40_pose(
                        session.source_event_phase5_context().pool40.slots[0],*scene_actor40);
                    hardware_test_trace("# source_actor40_draw,frame=%lu,parts=%lu,request=%u,record=%u,shade=UNKNOWN,integer_pose=MATCHES_MEASURED_FIELDS,raster=CANDIDATE,retail_parity=false",
                        (unsigned long)session.frame_counter(),(unsigned long)count,
                        pose?unsigned(pose->request):255u,pose?unsigned(pose->record_index):255u);}
            }
            if(scene_e0&&g_hardware_scene_draw==HardwareSceneDraw::All) {
                const auto submitted=draw_source_scene_e0_uv(session,*scene_e0);
                if(session.frame_counter()%60u==0u)
                    hardware_test_trace("# source_scene_E0_draw,frame=%lu,instances=%lu,LOD=FRONTEND_NEAR_DIAGNOSTIC,texture=INITIAL_BATCH_DIAGNOSTIC,source_camera_bound=%u,retail_parity=false",
                        (unsigned long)session.frame_counter(),(unsigned long)submitted,unsigned(source_camera_bound));
            }
            if(g_hardware_scene_draw==HardwareSceneDraw::All){
                const auto targets=draw_source_e008_uv(session);
                if(targets&&session.frame_counter()%30u==0u){
                    char hp[96];size_t at=0;hp[0]=0;
                    for(const auto& o:session.e008_objects())if(o.target&&at<80)
                        at+=size_t(std::snprintf(hp+at,sizeof(hp)-at,"%02x:s%u:hp%u;",unsigned(o.parameter),unsigned(o.state),unsigned(o.hp_0a)));
                    auto& rec=session.e008_hit_targets();
                    hardware_test_trace("# e008_targets,frame=%lu,%s,record0=%08lx,record1=%08lx",
                        (unsigned long)session.frame_counter(),hp,(unsigned long)rec[0].packet_00,(unsigned long)rec[1].packet_00);
                }
                if(targets&&session.frame_counter()%120u==0u)
                    hardware_test_trace("# e008_draw,frame=%lu,drawn=%lu,check_target=%ld,flag608=%u,flag609=%u,flag60e=%u,presentation=FRONTEND_DIAGNOSTIC",
                        (unsigned long)session.frame_counter(),(unsigned long)targets,
                        session.player_check_target()?long(*session.player_check_target()):-1L,
                        unsigned(session.story_flag(0x608)),unsigned(session.story_flag(0x609)),unsigned(session.story_flag(0x60E)));
            }
            if(g_hardware_scene_draw==HardwareSceneDraw::All){
                const auto boxes=draw_source_e005_uv(session);
                if(!session.e005_boxes().empty()&&session.frame_counter()%30u==0u){
                    const auto& box=session.e005_boxes().front();
                    hardware_test_trace("# e005_box,frame=%lu,drawn=%lu,slot=%u,hp=%d,action=%u,released=%u,flag620=%u,flag621=%u,rng=%u,no_random=%u,presentation=FRONTEND_DIAGNOSTIC",
                        (unsigned long)session.frame_counter(),(unsigned long)boxes,unsigned(box.slot),int(box.object.hp_44),
                        unsigned(box.object.action_09),unsigned(box.object.released),unsigned(session.story_flag(0x620)),unsigned(session.story_flag(0x621)),
                        unsigned(box.rng_status),unsigned(box.reward_without_random));
                }
            }
            if(g_hardware_scene_draw==HardwareSceneDraw::All){if(use_uv_renderer())draw_projectiles_uv(session);else draw_projectiles_0143(session);}
            if(source_combat_active&&use_uv_renderer()&&g_hardware_scene_draw==HardwareSceneDraw::All) {
                // The CSV "projectiles" column counts the retired candidate
                // projectiles; source Buster shots are reported here instead.
                if(const auto drawn=draw_source_busters_uv(g_children))
                    hardware_test_trace("# source_buster_draw,frame=%lu,drawn=%lu,segment=8003430C_REGISTRATION,presentation=FRONTEND_DIAGNOSTIC",
                        (unsigned long)session.frame_counter(),(unsigned long)drawn);
                (void)draw_source_reward_pickups_uv(g_children);
            }
            const auto scene_overlay=g_source_event_boot&&g_hardware_scene_draw==HardwareSceneDraw::All?session.source_event_scene_overlay():std::nullopt;
            const bool scene_overlay_submitted=scene_overlay&&scene_overlay->subtract_from_scene&&
                scene_overlay->source_function==0x80100614u&&
                draw_source_subtract_uv(scene_overlay->r,scene_overlay->g,scene_overlay->b,
                    scene_overlay->width,scene_overlay->height);
            if(g_source_event_boot&&use_uv_renderer()&&g_hardware_scene_draw==HardwareSceneDraw::All)draw_source_fade_uv(session.source_fade_presentation());
            submit_gpu_frame();
            if(g_render_submissions==0)boot_log(MML_BOOT_FIRST_FRAME,"first 3D GPU submission queued; completion pending");
            ++g_render_submissions;
            if(!g_first_frame_logged || session.frame_counter()%60u==0u)
                hardware_test_trace("# top_gpu_submission,frame=%lu,command_fraction=%.5f,command_capacity_bytes=%lu,renderer=%u,source=completed_submission,physical_parity=false",
                    (unsigned long)session.frame_counter(),double(C3D_GetCmdBufUsage()),
                    (unsigned long)C3D_DEFAULT_CMDBUF_SIZE,unsigned(g_renderer_gate.active));
            if(scene_overlay_submitted) {
                const bool accepted=session.acknowledge_source_event_scene_overlay(scene_overlay->lease);
                if(accepted)++scene_overlay_receipts;
                if(scene_overlay_receipts==1u||session.frame_counter()%60u==0u||!accepted)
                    hardware_test_trace("# source_scene_overlay,frame=%lu,serial=%llu,r=%u,g=%u,b=%u,submitted=1,accepted=%u,source_function=80100614,device_parity=false",
                        (unsigned long)session.frame_counter(),(unsigned long long)scene_overlay->lease.serial,
                        unsigned(scene_overlay->r),unsigned(scene_overlay->g),unsigned(scene_overlay->b),unsigned(accepted));
            }
            // Render timing includes the synchronization wait, not GPU execution time alone.
            mml_diag_phase(&g_runtime_diag,MML_PHASE_RENDER,static_cast<uint32_t>(gpu_wait_us+(svcGetSystemTick()-render_start)/CPU_TICKS_PER_USEC));

            if (!g_first_frame_logged) {
                g_first_frame_logged = true;
                boot_log(MML_BOOT_FIRST_FRAME, "first frame submitted; GPU completion pending");
                boot_log(MML_BOOT_READY, "runtime initialized; first GPU sync pending");
                mockup_hud_init();
            }
            g_runtime_diag.triangles = g_submitted_vertices/3u;
            g_runtime_diag.linear_bytes = g_owned_linear_bytes;
            g_runtime_diag.texture_bytes = g_owned_texture_bytes;
            g_runtime_diag.texture_count = g_texture_count;
            const u64 ui_start=svcGetSystemTick();
            const auto ui_position=session.player_position();
            const auto ui_map=l3::make_mockup_map_view(session.active_map(),session.map_area_index(),ui_position.x,ui_position.z,session.player_yaw());
            l3::mockup_map_update_follow(ui_state,ui_map);
            {
                const auto& d=session.dialogue();
                char text[480];size_t n=0;
                if(d.active&&d.source_controls_inline){
                    for(size_t i=0;i<d.visible_glyphs.size()&&n+8<sizeof(text);++i){
                        const uint8_t g=d.visible_glyphs[i];
                        if(g==0x86){text[n++]='\n';continue;}
                        if(const char* icon=l3::mml_glyph_icon(g)){for(const char* p=icon;*p&&n+1<sizeof(text);++p)text[n++]=*p;continue;}
                        const char c=l3::mml_glyph_ascii(g);if(c)text[n++]=c;
                    }
                }
                text[n]=0;
                mockup_hud_set_dialogue(d.active&&d.source_controls_inline,text,
                    d.waiting_for_choice?int(d.choice_targets.size()):0,int(d.choice),d.waiting_for_advance);
            }
            mockup_hud_draw(ui_state,session.bottom_screen_model(),ui_map,&g_runtime_diag);
            gfxScreenSwapBuffers(GFX_BOTTOM,false);
            mml_diag_phase(&g_runtime_diag,MML_PHASE_BOTTOM_UI,static_cast<uint32_t>((svcGetSystemTick()-ui_start)/CPU_TICKS_PER_USEC));
            g_runtime_diag.frame_us=static_cast<uint32_t>((svcGetSystemTick()-frame_start)/CPU_TICKS_PER_USEC);
            if(g_runtime_diag.frame_us>g_runtime_diag.loading_spike_us)g_runtime_diag.loading_spike_us=g_runtime_diag.frame_us;
            if(source_xa_requested){
                xa_audio.pump();
                if(session.frame_counter()%60==0&&session.frame_counter()!=last_audio_logged_frame){
                    last_audio_logged_frame=session.frame_counter();const auto observation=xa_audio.observation();
                    hardware_test_trace("# source_xa_playback,frame=%lu,ready=%u,playing=%u,started=%u,failed=%u,played_frames=%llu,sector=%lu,linear=%lu,index=%lu,underruns=%lu,max_read_us=%lu,dropped_dsp_frames=%lu,position_basis=COMPLETED_BUFFERS,retail_parity=false,reason=%s",
                        (unsigned long)session.frame_counter(),unsigned(observation.ready),unsigned(observation.playing),unsigned(observation.ever_started),unsigned(observation.failed),
                        (unsigned long long)observation.played_frames,(unsigned long)xa_audio.stream().sector_for_frame(observation.played_frames),
                        (unsigned long)xa_audio.linear_bytes(),(unsigned long)xa_audio.stream().resident_index_bytes(),(unsigned long)xa_audio.underruns(),
                        (unsigned long)xa_audio.max_read_us(),observation.ready?(unsigned long)ndspGetDroppedFrames():0ul,xa_audio.error());
                }
            }
            hardware_test_sample(session,g_runtime_diag,frame_start,held,down,g_current_area);
            hardware_test_actor_target_event(session, actors,
                actor_angle_steps ? &*actor_angle_steps : nullptr);
            if(actor_state_fields_requested())hardware_test_actor_state_event(session,actors);
        }
        xa_audio.close();
        hardware_test_area_event("unload_before",g_current_area,0,
            g_package_bytes,g_package_decoded_estimate,g_package_peak_estimate);
        l3::unload_imported_actors(actors);
        scene_e0.reset();
        shutdown_gpu(); gpu_ready=false;
        terrain_materials={}; player_materials={}; render={};
        session.release_inactive_locations();
        hardware_test_area_event("gpu_unloaded",g_current_area,0,
            g_package_bytes,g_package_decoded_estimate,g_package_peak_estimate);
    } catch(const std::exception& e) {
        consoleInit(GFX_BOTTOM,nullptr);
        boot_fail(0x014B0001u, e.what());
        // Report first: a stuck GPU queue must not hide the error by hanging
        // in resource destruction. Timed-out resources remain owned until exit.
        if(gpu_ready&&!g_gpu_wait_failed){
            try{shutdown_gpu();gpu_ready=false;}
            catch(const std::exception& cleanup_error){boot_log(MML_BOOT_C3D,cleanup_error.what());}
        }
        wait_for_exit_on_error(e.what());
    }
    l3::NegativeMaterialTables().swap(g_negative_materials);
    hardware_test_area_event("cpu_scope_unloaded",g_current_area,0,0,0,0);
    hardware_test_end();
    gfxExit();
    return 0;
}
