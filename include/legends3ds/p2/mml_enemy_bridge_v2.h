
#ifndef MML_ENEMY_BRIDGE_V2_H
#define MML_ENEMY_BRIDGE_V2_H
#include <stdbool.h>
#include <stdint.h>
#include "mml_world_v2.h"
typedef struct {uint32_t gameplay_id;int32_t hp;bool alive,targetable;} MMLEnemyRuntime;
bool mml_enemy_from_world(const MMLWorldObject*o,int32_t hp,MMLEnemyRuntime*out);bool mml_enemy_apply_damage(MMLEnemyRuntime*e,int32_t d);bool mml_enemy_sync_world(const MMLEnemyRuntime*e,MMLWorld*w);
#endif
