#include "transport.h"
#include <string.h>
#include <math.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif
#define AT(t,off,type) ((type *)((t)->mem+(off)))
static uint32_t load32(volatile uint32_t *p) { return __atomic_load_n(p,__ATOMIC_ACQUIRE); }
static void store32(volatile uint32_t *p,uint32_t v) { __atomic_store_n(p,v,__ATOMIC_RELEASE); }
static uint32_t begin(volatile uint32_t *seq) {
 uint32_t n=load32(seq); if(n&1)n++; store32(seq,n+1);
 __atomic_thread_fence(__ATOMIC_SEQ_CST); return n+2;
}
static void end(volatile uint32_t *seq,uint32_t n) { store32(seq,n); }
uint64_t wb_entity_id(const WbTransport *t,uint32_t agent) { return ((uint64_t)t->epoch<<32)|agent; }
int wb_open(WbTransport *t,const char *path) {
 memset(t,0,sizeof(*t));
#ifdef _WIN32
 HANDLE f=CreateFileA(path,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
 if(f==INVALID_HANDLE_VALUE)return 0;
 LARGE_INTEGER current,size;size.QuadPart=WBMC_SHM_SIZE;
 if(!GetFileSizeEx(f,&current)||(current.QuadPart!=0&&current.QuadPart!=WBMC_SHM_SIZE)){CloseHandle(f);return 0;}
 if(current.QuadPart==0&&(!SetFilePointerEx(f,size,NULL,FILE_BEGIN)||!SetEndOfFile(f))){CloseHandle(f);return 0;}
 HANDLE m=CreateFileMappingA(f,NULL,PAGE_READWRITE,0,WBMC_SHM_SIZE,NULL);
 if(!m){CloseHandle(f);return 0;}
 t->mem=MapViewOfFile(m,FILE_MAP_ALL_ACCESS,0,0,WBMC_SHM_SIZE);t->file=f;t->mapping=m;
#else
 int f=open(path,O_RDWR|O_CREAT,0600); if(f<0)return 0;
 struct stat info;
 if(fstat(f,&info)||(info.st_size!=0&&info.st_size!=WBMC_SHM_SIZE)){close(f);return 0;}
 if(info.st_size==0&&ftruncate(f,WBMC_SHM_SIZE)){close(f);return 0;}
 t->mem=mmap(NULL,WBMC_SHM_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,f,0);t->file=(void *)(intptr_t)(f+1);
 if(t->mem==MAP_FAILED)t->mem=NULL;
#endif
 if(!t->mem){wb_close(t);return 0;}
 WbmcHeader *h=AT(t,0,WbmcHeader);
 if(h->magic && (h->magic!=WBMC_MAGIC||h->version!=WBMC_VERSION||h->size!=WBMC_SHM_SIZE)){wb_close(t);return 0;}
 if(!h->magic){memset(t->mem,0,WBMC_SHM_SIZE);h->version=WBMC_VERSION;h->size=WBMC_SHM_SIZE;store32(&h->magic,WBMC_MAGIC);}
#ifdef _WIN32
 h->mhwPid=GetCurrentProcessId();
#else
 h->mhwPid=getpid();
#endif
 h->mhwStartMs=(uint64_t)time(NULL)*1000;
 t->focus_ack=h->mhwFocusReq;
 /* Epoch persists across host reconnects; never credit stale queued damage. */
 t->epoch=h->coreGeneration; wb_new_mission(t);return 1;
}
void wb_close(WbTransport *t){
#ifdef _WIN32
 if(t->mem)UnmapViewOfFile(t->mem);if(t->mapping)CloseHandle(t->mapping);if(t->file)CloseHandle(t->file);
#else
 if(t->mem)munmap(t->mem,WBMC_SHM_SIZE);if(t->file)close((int)(intptr_t)t->file-1);
#endif
 memset(t,0,sizeof(*t));
}
void wb_new_mission(WbTransport *t){
 if(!t->mem)return;
 WbmcHeader *h=AT(t,0,WbmcHeader);t->epoch++;if(!t->epoch)t->epoch=1;
 h->coreGeneration=t->epoch;
 uint32_t *army=(void *)(t->mem+0x2a0000);
 __atomic_store_n(army+1,__atomic_load_n(army,__ATOMIC_ACQUIRE),__ATOMIC_RELEASE);
 WbmcDamageQueue *q=AT(t,WBMC_OFF_DAMAGE,WbmcDamageQueue);store32(&q->read,load32(&q->write));
 WbmcHunterEvents *e=AT(t,WBMC_OFF_HUNTER,WbmcHunterEvents);uint32_t n=begin(&e->seq);
 memset((char *)e+4,0,sizeof(*e)-4);end(&e->seq,n);
 wb_entities(t,NULL,0);
}
void wb_publish(WbTransport *t,WbmcGameState *s){
 WbmcGameState *d=AT(t,WBMC_OFF_STATE,WbmcGameState);uint32_t n=begin(&d->seq);
 s->frame=++t->frame;memcpy((char *)d+4,(char *)s+4,sizeof(*s)-4);end(&d->seq,n);
 __atomic_store_n((uint64_t *)(void *)(t->mem+0x10),t->frame,__ATOMIC_RELEASE);
}
int wb_control(WbTransport *t,WbmcControl *out,double now){
 uint64_t mc=__atomic_load_n((uint64_t *)(void *)(t->mem+0x18),__ATOMIC_ACQUIRE);
 if(mc!=t->last_mc){t->last_mc=mc;t->last_mc_time=now;}
 if(!mc||now-t->last_mc_time>0.75)return 0;
 WbmcControl *p=AT(t,WBMC_OFF_CONTROL,WbmcControl);
 for(int tries=0;tries<8;tries++){
  uint32_t a=load32(&p->seq);if(!a||(a&1))continue;
  memcpy(out,p,sizeof(*out));__atomic_thread_fence(__ATOMIC_ACQUIRE);
  if(a!=load32(&p->seq))continue;
  if(!(out->flags&WBMC_CTRL_OVERRIDE_CAMERA))return 0;
  for(int i=0;i<3;i++)if(!isfinite(out->camPos[i])||!isfinite(out->camTarget[i])||!isfinite(out->hunterPos[i]))return 0;
  if(!isfinite(out->fovYDeg)||out->fovYDeg<10||out->fovYDeg>160)return 0;
  return 1;
 }return 0;
}
void wb_entities(WbTransport *t,const WbmcEntity *entries,uint32_t count){
 if(count>WBMC_MAX_ENTITIES)count=WBMC_MAX_ENTITIES;
 WbmcEntityTable *d=AT(t,WBMC_OFF_ENTITIES,WbmcEntityTable);uint32_t n=begin(&d->seq);
 d->count=count;d->frame=t->frame;if(count)memcpy(d->entities,entries,count*sizeof(*entries));end(&d->seq,n);
}
int wb_damage(WbTransport *t,WbmcDamage *out){
 WbmcDamageQueue *q=AT(t,WBMC_OFF_DAMAGE,WbmcDamageQueue);uint32_t r=load32(&q->read),w=load32(&q->write);
 if(w-r>WBMC_DAMAGE_RING){store32(&q->read,w);return 0;}
 while(r!=w){memcpy(out,&q->ring[r%WBMC_DAMAGE_RING],sizeof(*out));store32(&q->read,++r);
  if((uint32_t)(out->id>>32)==t->epoch && isfinite(out->amount)&&out->amount>0&&out->amount<=10000)return 1;
 }return 0;
}
void wb_player_hit(WbTransport *t,float amount,float max_hp){
 wb_player_hit_from(t,amount,max_hp,NULL);
}
void wb_player_hit_from(WbTransport *t,float amount,float max_hp,const float from[3]){
 if(!isfinite(amount)||amount<=0||!isfinite(max_hp)||max_hp<=0)return;
 WbmcHunterEvents *e=AT(t,WBMC_OFF_HUNTER,WbmcHunterEvents);uint32_t n=begin(&e->seq);
 e->hitCount++;e->totalDamage+=amount;e->lastDamage=amount;e->hunterMaxHp=max_hp;e->lastHitFrame=t->frame;
 if(from)memcpy(e->lastHitFrom,from,3*sizeof(float));end(&e->seq,n);
}
void wb_request_mc(WbTransport *t){WbmcHeader *h=AT(t,0,WbmcHeader);store32(&h->mcSwitchReq,load32(&h->mcSwitchReq)+1);}
uint32_t wb_rays(WbTransport *t,WbmcRay **rays,WbmcRayHit **hits){
 WbmcRayHeader *q=AT(t,WBMC_OFF_RAYS,WbmcRayHeader);
 if(load32(&q->reqSeq)==load32(&q->respSeq))return 0;
 if(q->count>WBMC_MAX_RAYS){q->count=0;wb_finish_rays(t);return 0;}
 *rays=AT(t,WBMC_OFF_RAYS+WBMC_RAYS_OFF_RAYS,WbmcRay);
 *hits=AT(t,WBMC_OFF_RAYS+WBMC_RAYS_OFF_HITS,WbmcRayHit);return q->count;
}
void wb_finish_rays(WbTransport *t){WbmcRayHeader *q=AT(t,WBMC_OFF_RAYS,WbmcRayHeader);q->processed=q->count;store32(&q->respSeq,load32(&q->reqSeq));}
