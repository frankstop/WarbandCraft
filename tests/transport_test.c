#include "transport.h"
#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
_Static_assert(sizeof(WbmcGameState)==0x100,"state ABI");
_Static_assert(sizeof(WbmcEntity)==0x80,"entity ABI");
_Static_assert(sizeof(WbmcDamage)==0x20,"damage ABI");
_Static_assert(offsetof(WbmcGameState,stageId)==0x7c,"scene ABI");
_Static_assert(offsetof(WbmcHeader,mcHeartbeat)==0x18,"heartbeat ABI");
int main(void){
 char path[]="/tmp/warbandcraft-test-XXXXXX";int f=mkstemp(path);assert(f>=0);close(f);
 WbTransport t;assert(wb_open(&t,path));WbmcHeader *h=(void *)t.mem;
 assert(h->magic==WBMC_MAGIC&&h->version==WBMC_VERSION);
 WbmcGameState state={0};state.flags=3;state.unitsPerMeter=1;state.stageId=33;wb_publish(&t,&state);
 WbmcGameState *pub=(void *)(t.mem+WBMC_OFF_STATE);assert(pub->seq==2&&pub->stageId==33&&pub->frame==1);
 WbmcControl *c=(void *)(t.mem+WBMC_OFF_CONTROL);WbmcControl got;
 c->seq=2;c->flags=WBMC_CTRL_OVERRIDE_CAMERA;c->fovYDeg=70;h->mcHeartbeat=1;
 assert(wb_control(&t,&got,1));assert(!wb_control(&t,&got,2));
 h->mcHeartbeat=2;c->seq=3;assert(!wb_control(&t,&got,3));c->seq=4;c->camPos[0]=NAN;
 assert(!wb_control(&t,&got,3));c->camPos[0]=0;assert(wb_control(&t,&got,3));
 WbmcEntity e={0};e.id=wb_entity_id(&t,42);wb_entities(&t,&e,1);
 WbmcEntityTable *es=(void *)(t.mem+WBMC_OFF_ENTITIES);assert(es->count==1&&es->entities[0].id==e.id);
 WbmcDamageQueue *q=(void *)(t.mem+WBMC_OFF_DAMAGE);q->ring[0].id=e.id;q->ring[0].amount=7;q->write=1;
 WbmcDamage damage;assert(wb_damage(&t,&damage)&&damage.amount==7&&!wb_damage(&t,&damage));
 q->ring[1].id=e.id;q->ring[1].amount=NAN;q->write=2;assert(!wb_damage(&t,&damage));
 wb_new_mission(&t);q->ring[2].id=e.id;q->ring[2].amount=10;q->write=3;assert(!wb_damage(&t,&damage));
 q->write=q->read+WBMC_DAMAGE_RING+1;assert(!wb_damage(&t,&damage)&&q->read==q->write);
 q->write=q->read=UINT32_MAX;q->ring[UINT32_MAX%WBMC_DAMAGE_RING].id=wb_entity_id(&t,42);q->ring[UINT32_MAX%WBMC_DAMAGE_RING].amount=1;q->write=0;
 assert(wb_damage(&t,&damage)&&q->read==0);
 wb_player_hit(&t,12,100);WbmcHunterEvents *hits=(void *)(t.mem+WBMC_OFF_HUNTER);assert(hits->hitCount==1&&hits->totalDamage==12);
 wb_player_hit(&t,NAN,100);assert(hits->hitCount==1);
 uint32_t *army=(void *)(t.mem+0x2a0000);army[0]=4;army[1]=1;
 wb_new_mission(&t);assert(hits->hitCount==0 && army[0]==army[1]);
 wb_request_mc(&t);assert(h->mcSwitchReq==1);
 WbmcRayHeader *r=(void *)(t.mem+WBMC_OFF_RAYS);r->reqSeq=1;r->count=WBMC_MAX_RAYS+1;
 WbmcRay *rays;WbmcRayHit *rh;assert(wb_rays(&t,&rays,&rh)==0&&r->respSeq==1);
 wb_close(&t);assert(wb_open(&t,path));h=(void *)t.mem;h->version=99;wb_close(&t);
 assert(!wb_open(&t,path));unlink(path);
 FILE *wrong=fopen(path,"wb");fputs("wrong mapping",wrong);fclose(wrong);
 assert(!wb_open(&t,path));wrong=fopen(path,"rb");fseek(wrong,0,SEEK_END);assert(ftell(wrong)==13);fclose(wrong);unlink(path);
 puts("PASS: native ABI, publication, heartbeat timeout, torn reads, invalid poses, damage, stale IDs, overflow/wrap, player hits, rays, incompatible mapping");return 0;
}
