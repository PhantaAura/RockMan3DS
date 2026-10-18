#pragma once
#include "legends3ds/xa_stream.h"
#include "legends3ds/source_event_command.h"
namespace l3 {
// Native playback observations, never an elapsed-frame clock. This is a small
// port transport adapter, not a PS1 CD emulator or proof of physical CD timing.
struct XaPlaybackObservation {
    bool ready=false,playing=false,ever_started=false,failed=false;
    uint64_t played_frames=0;
    bool paused=false;
};
struct XaNativeCdState {
    std::optional<uint8_t> mode;
    bool filter_bound=false,location_bound=false,start_requested=false;
    std::array<uint8_t,8> retained_response{},retained_known{};
};
enum class XaTransportStatus:uint8_t {Ready,NeedsBackend,UnsupportedRequest,NeedsConfiguration,NeedsPlayback,NeedsLocation,NeedsPause};
struct XaTransportResult {
    XaTransportStatus status=XaTransportStatus::NeedsBackend;
    std::optional<SourceEventCdReceipt> receipt;
    bool request_start=false;
    bool request_pause=false;
};
XaTransportResult service_native_xa_cd_request(XaNativeCdState& state,
    const XaStreamReader& stream,const SourceEventCdRequest& request,
    const XaPlaybackObservation& playback) noexcept;
} // namespace l3
