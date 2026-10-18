#include "legends3ds/actor_animation_bank.h"
#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>
#include <utility>

namespace l3 {
namespace {
bool valid_shape(const ActorAnimationRequestCache& cache) noexcept {
    return !cache.sequence.poses.empty() &&
        cache.sequence.poses.size() == cache.sequence.records.size() &&
        cache.sequence.records.size() <= kActorSequenceRecordLimit;
}
ActorAnimationStatus pose_ready(const ActorAnimationRequestCache& cache,
                                size_t index, const ActorPoseResource& live) noexcept {
    if (!valid_shape(cache) || index >= cache.sequence.poses.size())
        return ActorAnimationStatus::InvalidCache;
    const auto& pose = cache.sequence.poses[index];
    if (pose.request != cache.request || pose.record_index != index ||
        pose.sequencing_record != cache.sequence.records[index] ||
        pose.frame_index != pose.sequencing_record[0] ||
        pose.base_translation.empty() ||
        pose.base_translation.size() > kActorClass20PartLimit)
        return ActorAnimationStatus::InvalidCache;
    if (pose.base_translation.size() != live.base_translation.size())
        return ActorAnimationStatus::PartCountMismatch;
    return ActorAnimationStatus::SourceBackedPartial;
}
void copy_pose(const ActorPoseResource& source, ActorPoseResource& target) noexcept {
    target.request = source.request;
    target.record_index = source.record_index;
    target.frame_index = source.frame_index;
    target.countdown = source.countdown;
    target.next_control = source.next_control;
    target.sequencing_record = source.sequencing_record;
    target.root_translation = source.root_translation;
    target.rotations = source.rotations;
    std::copy(source.base_translation.begin(), source.base_translation.end(),
              target.base_translation.begin());
    target.sequence_source_offset = source.sequence_source_offset;
    target.pose_source_offset = source.pose_source_offset;
    target.translations_source_offset = source.translations_source_offset;
}
} // namespace

static ActorAnimationBank decode_ebd_animation_bank_family(
    const EbdDirectory& ebd, size_t row, const ActorMeshResource& mesh,
    const uint8_t* requests, size_t count, size_t cap, bool class40) {
    if (!requests || !count || count > kActorAnimationRequestLimit)
        throw std::runtime_error("actor animation bank request set invalid");
    if (row > std::numeric_limits<uint32_t>::max() || mesh.source_row != row)
        throw std::runtime_error("actor animation bank mesh/row mismatch");
    std::array<bool, kActorAnimationRequestLimit> seen{};
    for (size_t i = 0; i < count; ++i) {
        if (seen[requests[i]])
            throw std::runtime_error("actor animation bank duplicate request");
        seen[requests[i]] = true;
    }
    const size_t minimum = sizeof(ActorAnimationBank) + count * sizeof(ActorAnimationRequestCache);
    if (minimum > cap)
        throw std::runtime_error("actor animation bank decoded-element budget exceeded");
    ActorAnimationBank out;
    out.source_row = static_cast<uint32_t>(row);
    out.requests.reserve(count);
    out.decoded_bytes = sizeof(ActorAnimationBank) +
        out.requests.capacity() * sizeof(ActorAnimationRequestCache);
    if (out.decoded_bytes > cap)
        throw std::runtime_error("actor animation bank decoded-element budget exceeded");
    for (size_t i = 0; i < count; ++i) {
        // Embedded sequence metadata is already included in the request slot.
        // Pass its size back to the existing decoder's full-cache cap, then
        // count only the separately owned elements when adding the result.
        const size_t remaining = cap - out.decoded_bytes;
        const size_t sequence_cap = remaining > std::numeric_limits<size_t>::max() -
                sizeof(ActorPoseSequenceCache)
            ? std::numeric_limits<size_t>::max()
            : remaining + sizeof(ActorPoseSequenceCache);
        auto sequence = class40
            ?decode_ebd_scene_actor40_pose_sequence(ebd,row,mesh,requests[i],sequence_cap)
            :decode_ebd_actor_pose_sequence(ebd,row,mesh,requests[i],sequence_cap);
        if (sequence.decoded_bytes < sizeof(ActorPoseSequenceCache) ||
            sequence.decoded_bytes - sizeof(ActorPoseSequenceCache) > remaining)
            throw std::runtime_error("actor animation bank decoded-element budget exceeded");
        out.decoded_bytes += sequence.decoded_bytes - sizeof(ActorPoseSequenceCache);
        out.requests.push_back({requests[i], std::move(sequence)});
    }
    return out;
}
ActorAnimationBank decode_ebd_actor_animation_bank(
    const EbdDirectory& ebd,size_t row,const ActorMeshResource& mesh,
    const uint8_t* requests,size_t count,size_t cap) {
    return decode_ebd_animation_bank_family(ebd,row,mesh,requests,count,cap,false);
}
ActorAnimationBank decode_ebd_scene_actor40_animation_bank(
    const EbdDirectory& ebd,size_t row,const ActorMeshResource& mesh,
    const uint8_t* requests,size_t count,size_t cap) {
    return decode_ebd_animation_bank_family(ebd,row,mesh,requests,count,cap,true);
}

const ActorAnimationRequestCache* find_actor_animation_request(
    const ActorAnimationBank& bank, uint8_t request) noexcept {
    if (bank.requests.size() > kActorAnimationRequestLimit) return nullptr;
    for (const auto& cache : bank.requests)
        if (cache.request == request) return &cache;
    return nullptr;
}

ActorAnimationStatus actor_animation_request_ready(
    const ActorAnimationBank& bank, uint8_t request,
    const ActorPoseResource& live) noexcept {
    const auto* cache = find_actor_animation_request(bank, request);
    return cache ? pose_ready(*cache, 0, live) : ActorAnimationStatus::MissingRequest;
}

ActorAnimationPreview actor_animation_preview(const ActorAnimationBank& bank,
    uint8_t request,const ActorPoseResource& live,const ActorSequenceCursor& cursor) noexcept {
    ActorAnimationPreview out;out.cursor=cursor;
    const auto* cache=find_actor_animation_request(bank,request);
    if(!cache){out.result.status=ActorAnimationStatus::MissingRequest;return out;}
    const bool restart=live.request!=request;
    const auto current=pose_ready(*cache,restart?0:cursor.record_index,live);
    if(current!=ActorAnimationStatus::SourceBackedPartial){out.result.status=current;return out;}
    if(!restart&&live.record_index!=cursor.record_index)return out;
    bool changed=restart;
    if(restart){
        if(actor_sequence_reset(out.cursor,cache->sequence.records.data(),cache->sequence.records.size())!=
           ActorSequenceStatus::SourceBackedPartial)return out;
    }else{
        const auto step=actor_sequence_step(out.cursor,cache->sequence.records.data(),cache->sequence.records.size());
        if(step.status!=ActorSequenceStatus::SourceBackedPartial)return out;
        changed=step.record_changed;
    }
    const auto status=pose_ready(*cache,out.cursor.record_index,live);
    if(status!=ActorAnimationStatus::SourceBackedPartial){out.result.status=status;return out;}
    out.pose=(!restart&&!changed)?&live:&cache->sequence.poses[out.cursor.record_index];
    out.result={ActorAnimationStatus::SourceBackedPartial,restart,changed};return out;
}

ActorAnimationResult actor_animation_reset(
    const ActorAnimationBank& bank, uint8_t request,
    ActorPoseResource& live, ActorSequenceCursor& cursor) noexcept {
    const auto* cache = find_actor_animation_request(bank, request);
    if (!cache) return {ActorAnimationStatus::MissingRequest, false, false};
    const auto status = pose_ready(*cache, 0, live);
    if (status != ActorAnimationStatus::SourceBackedPartial) return {status, false, false};
    auto next = cursor;
    if (actor_sequence_reset(next, cache->sequence.records.data(), cache->sequence.records.size()) !=
        ActorSequenceStatus::SourceBackedPartial)
        return {ActorAnimationStatus::InvalidCache, false, false};
    const bool changed_request = live.request != request;
    const bool changed_record = changed_request || live.record_index != 0;
    copy_pose(cache->sequence.poses[0], live);
    live.countdown = next.countdown_a5;
    cursor = next;
    return {ActorAnimationStatus::SourceBackedPartial, changed_request, changed_record};
}

ActorAnimationResult actor_animation_step(
    const ActorAnimationBank& bank, ActorPoseResource& live,
    ActorSequenceCursor& cursor) noexcept {
    const auto* cache = find_actor_animation_request(bank, live.request);
    if (!cache) return {ActorAnimationStatus::MissingRequest, false, false};
    const auto current = pose_ready(*cache, cursor.record_index, live);
    if (current != ActorAnimationStatus::SourceBackedPartial) return {current, false, false};
    if (live.record_index != cursor.record_index)
        return {ActorAnimationStatus::InvalidCache, false, false};
    auto next = cursor;
    const auto step = actor_sequence_step(next, cache->sequence.records.data(),
                                          cache->sequence.records.size());
    if (step.status != ActorSequenceStatus::SourceBackedPartial)
        return {ActorAnimationStatus::InvalidCache, false, false};
    const auto status = pose_ready(*cache, next.record_index, live);
    if (status != ActorAnimationStatus::SourceBackedPartial) return {status, false, false};
    if (step.record_changed) copy_pose(cache->sequence.poses[next.record_index], live);
    live.countdown = next.countdown_a5;
    cursor = next;
    return {ActorAnimationStatus::SourceBackedPartial, false, step.record_changed};
}
} // namespace l3
