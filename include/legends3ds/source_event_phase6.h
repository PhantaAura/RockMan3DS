#pragma once
#include <cstdint>
#include <optional>
#include "legends3ds/source_event_phase5.h"
#include "legends3ds/source_event_random_effect.h"
#include "legends3ds/source_camera.h"
#include "legends3ds/source_fade.h"
#include "legends3ds/source_event_phase2.h"
#include "legends3ds/visual_bridge.h"

namespace l3 {
struct SourceEventWork;
struct SourceEventFlagSnapshot;
// The source stores only the marked lanes of800C3560..800C3571. Consumers
// must apply that mask rather than zeroing retained/unknown transition lanes.
struct SourceEventHandoffTransitionIntent {
    uint32_t source_function=0x80101274u;
    uint64_t source_generation=0;
    std::array<uint8_t,18> raw{},known{};
    std::array<int16_t,3> spawn{};
    uint8_t source_stage=0,destination_area=1,argument=0;
};
struct SourceEventSceneOverlayLease {
    const void* native_owner=nullptr;
    uint64_t serial=0,generation=0;
};
struct SourceEventSceneOverlayPresentation {
    SourceEventSceneOverlayLease lease;
    uint8_t r=0,g=0,b=0;
    uint16_t width=320,height=240;
    bool subtract_from_scene=true;
    uint32_t source_function=0x80100614u;
};
// Original byte owners keep their addresses until semantics are proved.
struct SourceEventPhase6Context {
    std::optional<uint8_t> byte_80122424,byte_8012242c,byte_80122430,byte_80122434,byte_80122438;
    // Main-loop iteration halfword, not controller input. Its initial owner
    // must be admitted separately; no native-frame/button fallback is used.
    std::optional<uint16_t> scratch_half_1f800002;
    uint32_t consumed_callback=0;
    // A completed original callee may wait for the next original callee.
    // This continuation prevents executing the callback repeatedly while its
    // camera consumer is unavailable. It is not a completed retail update.
    bool awaiting_camera_8001ec0c=false;
    bool awaiting_timeline_8001f070=false;
    bool awaiting_caller_801011e8=false;
    std::optional<uint8_t> byte_800bc733,byte_1f800074;
    std::optional<uint16_t> half_1f800006,half_1f800076;
    SourceProjectionControls projection;
    std::optional<int8_t> byte_800c1b6a;
    std::optional<uint16_t> input_800c0c2a;
    // One bounded native presentation owner, not a fabricated PS1 packet/OT.
    std::optional<SourceEventSceneOverlayPresentation> scene_overlay_pending;
    std::optional<SourceEventHandoffTransitionIntent> transition_pending;
};
struct SourceEventPhase6TimelineRecord {uint32_t header=0,callback=0;};
struct SourceEventPhase6Profile {
    bool initialized=false;
    uint16_t field_800bc81c=0;
    uint32_t camera_program_token=0,callback_table_token=0;
    std::array<uint32_t,231> camera_words{};
    std::array<SourceEventPhase6TimelineRecord,18> timeline{};
};
// Owner-local neutral source command/timeline data; bounded fixed arrays are
// allocated only by the package owner. No executable code is loaded or run.
SourceEventPhase6Profile load_source_event_phase6_profile(const std::vector<uint8_t>& bytes);
// Raw08 enables a callback; the data selector is the stored A4 table pointer.
// Unknown, disabled, malformed or unowned table identities provide no callback.
std::optional<uint32_t> source_event_phase6_selected_callback_8010102c(
    const SourceEventWork&,const SourceEventPhase6Profile&) noexcept;
// Exact cold executable BSS clear80068000 owns these retained bytes. This is
// an explicit fresh-executable-boot subset operation, never a warm area/save
// fallback and never an implicit store inside the scene constructor.
void initialize_source_event_scene_from_executable_boot_80068000(SourceEventPhase5Context& scene) noexcept;
// The supported ST00 overlay LOAD byte80122424 is zero. Admit this only at an
// explicit fresh overlay lifecycle with the hash-guarded phase5 profile.
bool initialize_source_event_phase6_overlay_load(SourceEventPhase6Context& context,
    const SourceEventPhase5Profile& profile) noexcept;
// Explicit cold executable BSS premise. Controller state is deliberately not
// retained as a later live input owner; the false flag branch needs no input.
void initialize_source_event_phase6_executable_boot_80068000(SourceEventPhase6Context& context) noexcept;
// Exact main-loop tail80011ECC..80011EE4. Missing initial scratch ownership
// holds; scheduling this tail is separately the frontend chronology premise.
bool consume_source_main_loop_tail_80011ecc(std::optional<uint16_t>& scratch_half) noexcept;
enum class SourceEventPhase6Status : uint8_t {
    NeedsEventWork,WrongEventPhase,NeedsOverlayControl,NeedsInputWord,
    NeedsCameraField,UnsupportedCallbackTable,UnsupportedCallback,
    NeedsActorBinding,NeedsResourceOwnership,CallbackConsumed,AwaitingCameraConsumer
    ,NeedsProfile,UnsupportedProfile,NeedsCameraWord,NeedsEventCounter,
    UnsupportedCameraCommand,CameraConsumed,AwaitingTimelineConsumer,
    NeedsTimelineWord,TimelineConsumed,AwaitingCallerConsumer,
    NeedsCallerBusy,NeedsPredicateStage,NeedsStoryFlag,NeedsPredicateInput,
    NeedsSceneOverlayControl,UnsupportedSceneOverlayMode,NeedsSceneOverlayLease,
    AwaitingSceneOverlayConsumer,CallerConsumed,NeedsType10FadeConsumer,
    NeedsActorField,NeedsPoolBitmap,NeedsPoolHeader,PoolCapacityExceeded,NeedsRandomOwner,
    Type10FadeConsumed,HandoffConditionFalse,NeedsHandoffConsumer,NeedsHandoffPlayer,
    NeedsHandoffFlags,NeedsHandoffCommands,NeedsHandoffDisplayOwner,HandoffDisplayMismatch,
    HandoffCommandMalformed,HandoffCommandFull,HandoffConsumed,AwaitingTransitionLoader
};
struct SourceEventPhase6Result {
    SourceEventPhase6Status status=SourceEventPhase6Status::NeedsEventWork;
    bool committed=false,callback_executed=false;
    uint32_t callback=0,next_consumer=0x8010102cu;
    std::optional<uint16_t> missing_offset;
    uint8_t random_callback_consumed_count=0;
    std::optional<SourceEventRandomEffectStatus> random_callback_status;
    std::optional<SourceFadeStartupStatus> fade_startup_status;
    bool transition_produced=false,player_resumed=false;
};
// Complete8010102C, including its optional camera adjustment and data-selected
// callbacks whose scalar stores/allocators have original-instruction proofs. All pointer identities resolve to bounded owned
// pools. The following8001EC0C/8001F070 remain explicit unconsumed owners.
// No scene counter, event phase, completion predicate or transition is forged.
SourceEventPhase6Result consume_source_event1_phase6_prefix_8010102c(
    const SourceEventWork& work,SourceEventPhase5Context& scene,
    SourceEventPhase6Context& context,const SourceEventPhase5ResourceBinding* resources,
    const SourceEventPhase6Profile* profile=nullptr,
    const SourceEventRandomEffectReceipts* random_receipts=nullptr,
    const SourceEventRandomEffectReceiptOwner* random_owner=nullptr,
    const SourceEventCandidateRng* candidate_rng=nullptr) noexcept;
// Complete8001EC0C data interpreter and retained-vector integration. Missing
// owners/unsupported data hold atomically; original projection controls are
// produced through the shared proven setter, without guessing a viewport.
SourceEventPhase6Result consume_source_event1_camera_8001ec0c(
    const SourceEventWork& work,SourceEventPhase5Context& scene,
    SourceEventPhase6Context& context,const SourceEventPhase6Profile* profile) noexcept;
SourceEventPhase6Result consume_source_event1_timeline_8001f070(
    SourceEventWork& work,SourceEventPhase6Context& context,
    const SourceEventPhase6Profile* profile) noexcept;
struct SourceEventCompletionPredicateResult {
    SourceEventPhase6Status status=SourceEventPhase6Status::NeedsPredicateStage;
    std::optional<bool> value;
};
// Exact short circuit of8001DE84. Unknown controller ownership is required
// only if the raw stage byte or source flag896+event is nonzero.
SourceEventCompletionPredicateResult evaluate_source_event_completion_8001de84(
    const SourceEventWork& work,const SourceEventFlagSnapshot* flags,
    std::optional<int8_t> byte_800c1b6a,std::optional<uint16_t> input_800c0c2a) noexcept;
SourceEventPhase6Result consume_source_event1_caller_801011e8(
    const SourceEventWork& work,const SourceEventPhase5Context& scene,
    SourceEventPhase6Context& context,const SourceEventFlagSnapshot* flags,
    std::optional<int16_t> busy_80098a72,uint64_t presentation_serial) noexcept;
// Exact8010122C: rechecks the caller's source condition against the actual
// fade owner, starts type10 and stores phase7. No elapsed-time completion.
SourceEventPhase6Result consume_source_event1_type10_fade_8010122c(
    SourceEventWork&,SourceEventPhase6Context&,SourceFadeContext&,
    const SourceFadeStartupProfile*,SourceFadeSchedulerIntentQueue*,
    const SourceEventFlagSnapshot*) noexcept;
// Explicit ownership of80042314's8002D004(ignoredA0,0,0) resource helper.
// Native code must furnish this only after admitting its two actual retained
// primitive/template buffer owners and executing their baseline packet copy.
// Zero offsets still copy from distinct template addresses; this is not a no-op
// or permission to fabricate missing resource ownership.
struct SourceEventHandoffDisplayReceipt {
    const void* native_owner=nullptr;
    uint64_t source_generation=0;
    uint32_t completed_function=0,first_header=0,second_header=0;
    uint32_t argument5=0,argument6=0;
    bool admitted=false;
};
// Native representation of the two HEAD part headers at 800F7820/800F7830.
// Retain only baseline render fields for head parts 16/17, not a second mesh.
// Capture once from the admitted fresh head and its actual visual bridge.
class SourceEventHeadUvBaseline {
public:
    size_t record_count() const noexcept { return records_.size(); }
private:
    struct Record {
        size_t visual_index=0;
        uint8_t part_id=0,vertex_count=0,primitive_code=0;
        uint16_t clut=0,tpage=0;
        std::array<std::array<uint8_t,2>,4> uv{};
    };
    size_t visual_count_=0;
    bool admitted_=false;
    std::vector<Record> records_;
    friend std::optional<SourceEventHeadUvBaseline> capture_source_event_head_uv_baseline(
        const PbdModel&,const std::vector<PlayerVisualPrimitive>&);
    friend std::optional<SourceEventHandoffDisplayReceipt> restore_source_event_head_uv_8002d004(
        const SourceEventHeadUvBaseline&,std::vector<PlayerVisualPrimitive>&,
        const void*,uint64_t) noexcept;
};
std::optional<SourceEventHeadUvBaseline> capture_source_event_head_uv_baseline(
    const PbdModel& admitted_fresh_head,const std::vector<PlayerVisualPrimitive>& live);
// Exact rendered UV/CLUT/tpage effect of 8002D004(ignored,0,0), after a complete
// structural preflight. PS1 packet padding/OT bytes are not fabricated. The
// native renderer derives its submitted pose from this same live visual owner.
// Missing/stale owners preserve every visual and return no completion receipt.
std::optional<SourceEventHandoffDisplayReceipt> restore_source_event_head_uv_8002d004(
    const SourceEventHeadUvBaseline&,std::vector<PlayerVisualPrimitive>& live,
    const void* native_owner,uint64_t source_generation) noexcept;
// Complete source 80015EE8 with an explicit original caller argument6.
// Reads only XYZ lanes0..5 and heading10..11 from 800C3560, preserving unknown
// retained lanes6..9. State3 extra mirrors copy both values and known markers.
// This applies player stores only, not a CD acknowledgement or area lifecycle.
bool apply_source_handoff_spawn_80015ee8(const SourceEventHandoffTransitionIntent&,
    uint64_t current_source_generation,L3PlayerCompatWork&,SourceEventPlayerKnown&,
    bool source_argument6) noexcept;
// Original new-area callback 80015840 clears transition byte 800C356C only
// after its state-1 gameplay camera initializer 80038648 and event continuation
// 8001F3C8. Its prior state-0 path owns transport/spawn through 80016D64.
// Native code may publish zero only after all equivalent receipts belong to
// this exact intent.
struct SourceAreaLoadCompletionReceipt {
    const void* native_owner=nullptr;
    uint64_t source_generation=0;
    uint8_t destination_area=0;
    uint32_t source_callback=0x80015840u;
    uint32_t camera_initializer=0x80038648u;
    bool area_replaced=false,spawn_applied=false,gameplay_camera_initialized=false;
};
bool acknowledge_source_area_load_80015840(
    const SourceEventHandoffTransitionIntent&,
    const SourceAreaLoadCompletionReceipt&,
    const void* current_native_owner,
    std::optional<int8_t>& transition_status_800c356c) noexcept;
struct SourceEventHandoffInputs {
    L3PlayerCompatWork* player=nullptr;
    SourceEventPlayerKnown* player_known=nullptr;
    SourceEventFlagSnapshot* flags=nullptr;
    SourceEventCommandQueue* commands=nullptr;
    const SourceEventHandoffDisplayReceipt* display=nullptr;
    uint64_t source_generation=0;
};
// Complete bounded80101260..8010130C after the phase7 callback/camera/timeline.
// Transition fields, flag1540, player reset, queue producer and event00 clear
// commit together. The area loader and CD type2 dispatcher are separate owners.
SourceEventPhase6Result consume_source_event1_handoff_80101260(
    SourceEventWork&,SourceEventPhase6Context&,const SourceFadeContext&,
    const SourceEventHandoffInputs&) noexcept;
// Shared original80101310 color tail after either caller branch. In particular
// it can run after event00 was cleared without repeating a completed handoff.
SourceEventPhase6Result consume_source_event1_color_tail_80101310(
    const SourceEventPhase5Context&,SourceEventPhase6Context&,uint64_t serial) noexcept;
// Call only after the renderer has consumed the exact pending presentation.
// RuntimeSession additionally verifies active resource ownership and session
// identity. No type10 fade or scene completion is acknowledged by this receipt.
bool acknowledge_source_event_scene_overlay_80100614(
    SourceEventPhase6Context& context,uint64_t serial) noexcept;
} // namespace l3
