
#ifndef MML_H4_RUNTIME_H
#define MML_H4_RUNTIME_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef struct { int32_t x,y,z; } MMLVec3i;
typedef struct {MMLVec3i a,b,c;int16_t nx_q12,ny_q12,nz_q12;uint32_t flags;} MMLFloorTri;
typedef struct {bool hit;uint32_t tri_index;int32_t floor_y;int16_t nx_q12,ny_q12,nz_q12;uint32_t flags;} MMLFloorHit;
typedef struct {int32_t max_drop,max_step_up;int16_t min_floor_ny_q12;} MMLCollisionPolicy;
typedef enum {MML_CAM_GAMEPLAY=0,MML_CAM_LOCKON=1,MML_CAM_SCRIPTED=2} MMLCameraOwner;
typedef struct {MMLCameraOwner owner;uint16_t yaw12;int16_t pitch12;uint32_t target_id;bool target_valid;} MMLCameraState;
typedef struct {uint32_t id;uint16_t yaw_to_target12;uint32_t distance_sq;bool alive,targetable;} MMLTargetCandidate;
typedef struct {bool active;uint32_t target_id;} MMLLockOnState;
uint16_t mml_angle12_wrap(int32_t a);int16_t mml_angle12_delta(uint16_t from,uint16_t to);
uint16_t mml_projection_heading12(uint16_t heading12,uint16_t frame_offset12);
bool mml_floor_query(const MMLFloorTri *tris,size_t count,int32_t x,int32_t y,int32_t z,const MMLCollisionPolicy *policy,MMLFloorHit *out);
bool mml_lockon_acquire(MMLLockOnState *st,uint16_t facing12,uint16_t max_angle_delta12,uint32_t max_distance_sq,const MMLTargetCandidate *cands,size_t count);
bool mml_lockon_retain(MMLLockOnState *st,uint16_t facing12,uint16_t max_angle_delta12,uint32_t max_distance_sq,const MMLTargetCandidate *cands,size_t count);
void mml_lockon_release(MMLLockOnState *st);void mml_camera_set_owner(MMLCameraState *cam,MMLCameraOwner owner);void mml_camera_apply_lockon(MMLCameraState *cam,const MMLLockOnState *lock);
#endif
