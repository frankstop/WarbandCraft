#pragma once
#include "protocol.h"
#include <stddef.h>
#include <stdint.h>
/* One host/game-thread writer. Minecraft owns control/rays/damage writes. */
typedef struct WbTransport {
    unsigned char *mem;
    void *file, *mapping;
    uint64_t frame, last_mc;
    uint32_t epoch, focus_ack;
    double last_mc_time;
} WbTransport;
int wb_open(WbTransport *t, const char *path);
void wb_close(WbTransport *t);
void wb_new_mission(WbTransport *t);
void wb_publish(WbTransport *t, WbmcGameState *state);
int wb_control(WbTransport *t, WbmcControl *out, double now);
void wb_entities(WbTransport *t, const WbmcEntity *entries, uint32_t count);
int wb_damage(WbTransport *t, WbmcDamage *out);
void wb_player_hit(WbTransport *t, float amount, float max_hp);
void wb_player_hit_from(WbTransport *t, float amount, float max_hp, const float from[3]);
void wb_request_mc(WbTransport *t);
uint64_t wb_entity_id(const WbTransport *t, uint32_t agent);
uint32_t wb_rays(WbTransport *t, WbmcRay **rays, WbmcRayHit **hits);
void wb_finish_rays(WbTransport *t);
