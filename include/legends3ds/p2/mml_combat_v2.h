
#ifndef MML_COMBAT_V2_H
#define MML_COMBAT_V2_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define MML_COMBAT_MAX_PROJECTILES 16
#define MML_COMBAT_MAX_ACTORS 32
typedef enum {MML_EVIDENCE_UNKNOWN=0,MML_EVIDENCE_SOURCE_BACKED,MML_EVIDENCE_MEASURED} MMLEvidence;
typedef struct {int32_t fire_cooldown,projectile_speed,projectile_lifetime,damage;MMLEvidence cadence_e,speed_e,lifetime_e,damage_e;} MMLBusterConfig;
typedef struct {bool active;uint32_t owner_id,target_id;int32_t x,y,z,vx,vy,vz,lifetime,damage;} MMLProjectile;
typedef struct {uint32_t id;int32_t hp,invuln;bool alive,targetable;} MMLCombatActor;
typedef struct {MMLProjectile p[MML_COMBAT_MAX_PROJECTILES];MMLCombatActor a[MML_COMBAT_MAX_ACTORS];size_t actor_count;int32_t cooldown;} MMLCombatRuntime;
bool mml_buster_config_classic_ready(const MMLBusterConfig*c);
void mml_combat_init(MMLCombatRuntime*r);
bool mml_combat_fire(MMLCombatRuntime*r,const MMLBusterConfig*c,bool classic,uint32_t owner,uint32_t target,int32_t x,int32_t y,int32_t z,int32_t vx,int32_t vy,int32_t vz);
void mml_combat_tick(MMLCombatRuntime*r);
bool mml_combat_damage(MMLCombatActor*a,int32_t damage,int32_t invuln_frames);
#endif
