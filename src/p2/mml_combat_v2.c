
#include "mml_combat_v2.h"
#include <string.h>
bool mml_buster_config_classic_ready(const MMLBusterConfig*c){return c&&c->cadence_e>=MML_EVIDENCE_SOURCE_BACKED&&c->speed_e>=MML_EVIDENCE_SOURCE_BACKED&&c->lifetime_e>=MML_EVIDENCE_SOURCE_BACKED&&c->damage_e>=MML_EVIDENCE_SOURCE_BACKED;}
void mml_combat_init(MMLCombatRuntime*r){if(r)memset(r,0,sizeof(*r));}
bool mml_combat_fire(MMLCombatRuntime*r,const MMLBusterConfig*c,bool classic,uint32_t owner,uint32_t target,int32_t x,int32_t y,int32_t z,int32_t vx,int32_t vy,int32_t vz){if(!r||!c||r->cooldown>0||(classic&&!mml_buster_config_classic_ready(c)))return false;for(size_t i=0;i<MML_COMBAT_MAX_PROJECTILES;i++)if(!r->p[i].active){r->p[i]=(MMLProjectile){true,owner,target,x,y,z,vx,vy,vz,c->projectile_lifetime,c->damage};r->cooldown=c->fire_cooldown;return true;}return false;}
void mml_combat_tick(MMLCombatRuntime*r){if(!r)return;if(r->cooldown>0)r->cooldown--;for(size_t i=0;i<MML_COMBAT_MAX_PROJECTILES;i++)if(r->p[i].active){r->p[i].x+=r->p[i].vx;r->p[i].y+=r->p[i].vy;r->p[i].z+=r->p[i].vz;if(--r->p[i].lifetime<=0)r->p[i].active=false;}for(size_t i=0;i<r->actor_count;i++)if(r->a[i].invuln>0)r->a[i].invuln--;}
bool mml_combat_damage(MMLCombatActor*a,int32_t d,int32_t inv){if(!a||!a->alive||a->invuln>0||d<=0)return false;a->hp-=d;a->invuln=inv;if(a->hp<=0){a->hp=0;a->alive=false;a->targetable=false;}return true;}
