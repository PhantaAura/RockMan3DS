#pragma once
#include "legends3ds/pbd_animation.h"
#include "legends3ds/runtime_session.h"
namespace l3 {
// One call on each side of a gameplay update, independent of render frequency.
// Only proven PL00M00 records feed the work flags consumed by player actions.
inline bool runtime_animation_submit(const PbdAnimation& animation,
                                     const PbdMainBodyTrack& track, RuntimeSession& session) {
    if (!track.valid) return false;
    const auto* action = find_pbd_animation_action(animation, track.request);
    if (!action || track.record_index >= action->s_records.size()) return false;
    const auto& record = action->s_records[track.record_index];
    return session.submit_player_animation_record(track.request, track.pose_frame,
        track.ticks_remaining, static_cast<uint8_t>(record.word1),
        static_cast<int8_t>(record.word1 >> 8));
}
inline bool runtime_animation_prepare(const PbdAnimation& animation,
                                     PbdMainBodyTrack& track, RuntimeSession& session) {
    const auto request = session.compat_player().bytes[0xAC];
    if (!track.valid || track.request != request)
        track = pbd_main_body_track_begin(animation, request, session.compat_player().bytes[0xA4]);
    return runtime_animation_submit(animation, track, session);
}
inline bool runtime_animation_finish(const PbdAnimation& animation,
                                    PbdMainBodyTrack& track, RuntimeSession& session) {
    const auto request = session.compat_player().bytes[0xAC];
    // +AD=0xFF is the source setter's forced same-request restart.
    if (!track.valid || track.request != request || session.compat_player().bytes[0xAD]==0xFFu)
        track = pbd_main_body_track_begin(animation, request, session.compat_player().bytes[0xA4]);
    else
        (void)pbd_main_body_track_tick(animation, track);
    return runtime_animation_submit(animation, track, session);
}
} // namespace l3
