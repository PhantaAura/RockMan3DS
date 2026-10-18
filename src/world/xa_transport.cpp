#include "legends3ds/xa_transport.h"
#include <algorithm>
namespace l3 {
XaTransportResult service_native_xa_cd_request(XaNativeCdState& state,
    const XaStreamReader& stream,const SourceEventCdRequest& request,
    const XaPlaybackObservation& playback) noexcept {
    XaTransportResult out;
    if(!stream.ready()||!playback.ready||playback.failed)return out;
    const bool pause_request=request.source_function==0x8001d254u&&request.command==9&&request.parameter_bytes==0;
    if((request.source_function!=0x8001d2bc&&!pause_request)||request.parameter_bytes>4){out.status=XaTransportStatus::UnsupportedRequest;return out;}
    for(size_t i=request.parameter_bytes;i<4;++i)if(request.parameters[i]){out.status=XaTransportStatus::UnsupportedRequest;return out;}
    auto next=state;const auto& info=stream.info();
    const auto status=uint8_t(2|((playback.playing&&!playback.paused)?32:0)); // native motor-ready/reading protocol translation
    switch(request.command){
    case 14:
        if(request.parameter_bytes!=1||request.parameters[0]!=200){out.status=XaTransportStatus::UnsupportedRequest;return out;}
        next.mode=200;break;
    case 13:
        if(request.parameter_bytes!=2||request.parameters[0]!=info.filter_file||request.parameters[1]!=info.filter_channel){out.status=XaTransportStatus::UnsupportedRequest;return out;}
        next.filter_bound=true;break;
    case 2: {
        if(request.parameter_bytes!=3){out.status=XaTransportStatus::UnsupportedRequest;return out;}
        const auto location=source_event_sector_to_bcd_80077e88(int32_t(info.start_lba));
        if(!std::equal(location.begin(),location.end(),request.parameters.begin())){out.status=XaTransportStatus::NeedsLocation;return out;}
        // Only the admitted cold stream beginning. Arbitrary seeks need proved
        // predictor state and cannot restart this predecoded stream by guess.
        if(playback.ever_started){out.status=XaTransportStatus::NeedsLocation;return out;}
        next.location_bound=true;break;
    }
    case 27:
        if(request.parameter_bytes){out.status=XaTransportStatus::UnsupportedRequest;return out;}
        if(next.mode!=200||!next.filter_bound||!next.location_bound){out.status=XaTransportStatus::NeedsConfiguration;return out;}
        if(!playback.playing||playback.paused){
            if(!next.start_requested&&!playback.ever_started){next.start_requested=true;out.request_start=true;state=next;}
            out.status=XaTransportStatus::NeedsPlayback;return out;
        }
        break;
    case 1:
        if(request.parameter_bytes){out.status=XaTransportStatus::UnsupportedRequest;return out;}
        break;
    case 9:
        if(!pause_request){out.status=XaTransportStatus::UnsupportedRequest;return out;}
        if(!playback.paused){out.status=XaTransportStatus::NeedsPause;out.request_pause=true;return out;}
        break;
    case 17: {
        if(request.parameter_bytes){out.status=XaTransportStatus::UnsupportedRequest;return out;}
        if(!playback.playing||playback.paused||playback.played_frames>=info.total_frames){out.status=XaTransportStatus::NeedsPlayback;return out;}
        const auto lba=stream.sector_for_frame(playback.played_frames);
        const auto location=source_event_sector_to_bcd_80077e88(int32_t(lba));
        // Track/index/relative subchannel fields are not in this package.
        // Leave them UNKNOWN; the proved outer owner consumes only5..7.
        next.retained_known.fill(0);
        for(size_t i=0;i<3;++i){next.retained_response[5+i]=location[i];next.retained_known[5+i]=1;}
        break;
    }
    default:out.status=XaTransportStatus::UnsupportedRequest;return out;
    }
    if(request.command!=17){next.retained_response[0]=status;next.retained_known[0]=1;}
    SourceEventCdReceipt receipt;receipt.request=request;receipt.poll_result=receipt.control_result=1;
    receipt.response=next.retained_response;receipt.response_known=next.retained_known;
    // This is a native transport mapping, not a PS1 drive-timing claim. Only
    // observed native channel pause supplies the original completion code2.
    if(pause_request)receipt.synchronous_callback_code=2;
    state=next;out.status=XaTransportStatus::Ready;out.receipt=receipt;return out;
}
} // namespace l3
