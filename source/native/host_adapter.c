#define GL_SILENCE_DEPRECATION
#include "transport.h"
#include <SDL.h>
#include <glew.h>
#include <dlfcn.h>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <malloc/malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <objc/message.h>
#include <objc/runtime.h>
#define INTERPOSE(repl,orig) __attribute__((used)) static struct {const void *a,*b;} ip_##orig __attribute__((section("__DATA,__interpose")))={(void*)&repl,(void*)&orig};
static WbTransport t;
static int initialized;
static uintptr_t mailbox, candidate;
static int stride, candidate_stride;
static int64_t candidate_seq, epoch=-1, damage_total;
static double scan_time, last_script_time;
static int64_t last_seq;
static uint64_t last_composite;
static uint32_t death_ack;
static GLuint scene_fbo;
static int win_w,win_h;
static int inside_compositor;
static float host_near=.1f,host_far=1000,host_fov=70;
static GLuint native_program;
static float render_pos[3],render_dir[3],render_up[3];
static unsigned render_samples;
static void (*original_uniform)(GLint,GLsizei,GLboolean,const GLfloat*);
static void (*original_use)(GLuint);
typedef struct {GLuint program;GLint view,vp;} Locations;
static Locations locations[256];static int location_count;
static int inverse(const float *m,float *out){
 double a[4][8];for(int r=0;r<4;r++)for(int c=0;c<8;c++)a[r][c]=c<4?m[c*4+r]:(c-4==r);
 for(int c=0;c<4;c++){int pivot=c;for(int r=c+1;r<4;r++)if(fabs(a[r][c])>fabs(a[pivot][c]))pivot=r;if(fabs(a[pivot][c])<1e-9)return 0;
  for(int k=0;k<8;k++){double x=a[c][k];a[c][k]=a[pivot][k];a[pivot][k]=x;}double d=a[c][c];for(int k=0;k<8;k++)a[c][k]/=d;
  for(int r=0;r<4;r++)if(r!=c){d=a[r][c];for(int k=0;k<8;k++)a[r][k]-=d*a[c][k];}}
 for(int r=0;r<4;r++)for(int c=0;c<4;c++)out[c*4+r]=a[r][c+4];return 1;
}
static void bridge_use(GLuint p){if(!inside_compositor)native_program=p;original_use(p);}
static void bridge_uniform(GLint loc,GLsizei count,GLboolean transpose,const GLfloat *value){
 original_uniform(loc,count,transpose,value);if(inside_compositor||count!=1||!native_program)return;
 Locations *l=NULL;for(int i=0;i<location_count;i++)if(locations[i].program==native_program){l=locations+i;break;}
 if(!l && location_count<256){l=locations+location_count++;l->program=native_program;l->view=glGetUniformLocation(native_program,"matView");l->vp=glGetUniformLocation(native_program,"matViewProj");}
 if(!l||l->vp<0||loc!=l->vp)return;
 GLint vpport[4];glGetIntegerv(GL_VIEWPORT,vpport);if(vpport[2]!=win_w||vpport[3]!=win_h)return;
 float vp[16];glGetUniformfv(native_program,l->vp,vp);
 double r3=vp[3]*vp[3]+vp[7]*vp[7]+vp[11]*vp[11];if(r3<.9||r3>1.1)return;
 double a=-(vp[2]*vp[3]+vp[6]*vp[7]+vp[10]*vp[11])/r3,b=vp[14]+a*vp[15];
 float n=fabs(b/(a-1)),f=fabs(b/(a+1)),fov=2*atan(1/sqrt(vp[1]*vp[1]+vp[5]*vp[5]+vp[9]*vp[9]))*180/M_PI;
 if(n>.001 && n<10 && f>n && f<100000 && fov>10 && fov<160){host_near=n;host_far=f;host_fov=fov;
  float basis[16]={0},inv[16];basis[15]=1;
  for(int c=0;c<4;c++){basis[c*4]=vp[c*4];basis[c*4+1]=vp[c*4+1];basis[c*4+2]=vp[c*4+3];}
  if(inverse(basis,inv)){for(int i=0;i<3;i++){render_pos[i]=inv[12+i];render_dir[i]=vp[i*4+3]/sqrt(r3);render_up[i]=vp[i*4+1]*tan(fov*M_PI/360);}render_samples++;}
 }
}
static void (*original_bind)(GLenum,GLuint);
static void bridge_bind(GLenum target,GLuint fbo){
 if(!inside_compositor && fbo && (target==GL_FRAMEBUFFER || target==GL_DRAW_FRAMEBUFFER)){
  original_bind(target,fbo); GLint depth=0,vp[4];glGetIntegerv(GL_VIEWPORT,vp);
  glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,&depth);
  if(depth!=GL_NONE && vp[2]==win_w && vp[3]==win_h)scene_fbo=fbo;
  return;
 }
 original_bind(target,fbo);
}
static void install_hooks(void){
 void **ptr=dlsym(RTLD_DEFAULT,"__glewBindFramebuffer");
 if(ptr && *ptr && !original_bind){original_bind=*ptr;*ptr=bridge_bind;fprintf(stderr,"[WBMC] Scene framebuffer hook installed\n");}
 ptr=dlsym(RTLD_DEFAULT,"__glewUseProgram");if(ptr&&*ptr&&!original_use){original_use=*ptr;*ptr=bridge_use;}
 ptr=dlsym(RTLD_DEFAULT,"__glewUniformMatrix4fv");if(ptr&&*ptr&&!original_uniform){original_uniform=*ptr;*ptr=bridge_uniform;}
}
static WbmcControl applied;
static WbmcControl pose_history[64];
static double now(void){ struct timespec ts;clock_gettime(CLOCK_MONOTONIC,&ts);return ts.tv_sec+ts.tv_nsec*1e-9; }
static int64_t peek(uintptr_t p,int s,int i){ return s==8?((int64_t*)p)[i]:((int32_t*)p)[i]; }
static int64_t get(int i){return peek(mailbox,stride,i);}
static void put(int i,int64_t x){if(stride==8)((int64_t*)mailbox)[i]=x;else((int32_t*)mailbox)[i]=(int32_t)x;}
static int readable(uintptr_t p,size_t n){
 mach_vm_address_t a=p;mach_vm_size_t size;vm_region_basic_info_data_64_t info;mach_msg_type_number_t count=VM_REGION_BASIC_INFO_COUNT_64;mach_port_t obj;
 return mach_vm_region(mach_task_self(),&a,&size,VM_REGION_BASIC_INFO_64,(vm_region_info_t)&info,&count,&obj)==KERN_SUCCESS && a<=p && p+n<=a+size && (info.protection&(VM_PROT_READ|VM_PROT_WRITE))==(VM_PROT_READ|VM_PROT_WRITE);
}
static int signature(uintptr_t p,int s){return peek(p,s,0)==1463962947 && peek(p,s,1)==12648430 && peek(p,s,2)==305419896 && peek(p,s,3)==1985229328 && peek(p,s,4)==1 && peek(p,s,5)==65536;}
static void find_mailbox(void){
 if(mailbox && (!readable(mailbox,65536*stride)||!signature(mailbox,stride))){mailbox=0;fprintf(stderr,"[WBMC] Mailbox moved; reacquiring\n");}
 if(mailbox)return;
 if(candidate && readable(candidate,65536*candidate_stride) && signature(candidate,candidate_stride)){
  int64_t seq=peek(candidate,candidate_stride,7);
  if(seq>candidate_seq && seq-candidate_seq<1000){mailbox=candidate;stride=candidate_stride;fprintf(stderr,"[WBMC] Validated mission mailbox %p, %d-bit slots, frame %lld\n",(void*)mailbox,stride*8,seq);return;}
 }
 if(now()-scan_time<1)return;scan_time=now();
 mach_vm_address_t a=0;mach_vm_size_t size;static unsigned char *buf; if(!buf)buf=malloc(1048576+64);
 while(1){
  vm_region_basic_info_data_64_t info;mach_msg_type_number_t count=VM_REGION_BASIC_INFO_COUNT_64;mach_port_t obj;
  if(mach_vm_region(mach_task_self(),&a,&size,VM_REGION_BASIC_INFO_64,(vm_region_info_t)&info,&count,&obj)!=KERN_SUCCESS)break;
  if((info.protection&3)==3 && !(info.protection&VM_PROT_EXECUTE) && size<512*1024*1024){
   for(size_t off=0;off<size;off+=1048576){size_t n=size-off;if(n>1048576+64)n=1048576+64;mach_vm_size_t got=0;
    if(mach_vm_read_overwrite(mach_task_self(),a+off,n,(mach_vm_address_t)buf,&got)!=KERN_SUCCESS)continue;
    for(size_t j=0;j+64<=got;j+=4){
     if(*(uint32_t*)(buf+j)!=1463962947)continue;
     for(int s=4;s<=8;s+=4)if(signature((uintptr_t)(buf+j),s) && peek((uintptr_t)(buf+j),s,7)>1){
      uintptr_t p=a+off+j; if(p==(uintptr_t)buf+j)continue;
      size_t allocation=malloc_size((void*)p);
      if(allocation>=65536*s && allocation<=131072*s && readable(p,65536*s)){
       candidate=p;candidate_stride=s;candidate_seq=peek(p,s,7);
       mailbox=p;stride=s;fprintf(stderr,"[WBMC] Validated reserved troop allocation %p (%zu bytes), frame %lld\n",(void*)p,allocation,candidate_seq);return;
      }
     }
    }
   }
  }
  if(a+size<=a)break;a+=size;
 }
}
static void coords(float *out,int base){out[0]=get(base)*.001f;out[1]=get(base+2)*.001f;out[2]=get(base+1)*.001f;}
static void write_coords(int base,const float *p){put(base,llround(p[0]*1000));put(base+1,llround(p[2]*1000));put(base+2,llround(p[1]*1000));}
static void rotation(const float *p,const float *target,int base){
 double dx=target[0]-p[0],dy=target[2]-p[2],dz=target[1]-p[1];
 put(base,llround(atan2(-dx,dy)*180000/M_PI));put(base+1,llround(atan2(dz,hypot(dx,dy))*180000/M_PI));
}
static uint32_t ray_count,ray_cursor,ray_chunk,ray_serial;
static WbmcRay *rays;static WbmcRayHit *hits;
static void handle_rays(void){
 if(ray_count && get(62)==ray_serial){
  for(uint32_t i=0;i<ray_chunk;i++){
   int b=16384+i*12;WbmcRayHit *h=hits+ray_cursor+i;memset(h,0,sizeof(*h));
   h->hit=get(b+6);if(h->hit){coords(h->pos,b+7);h->normal[1]=1;}
  }
  ray_cursor+=ray_chunk;
  if(ray_cursor==ray_count){wb_finish_rays(&t);ray_count=0;}
 }
 if(!ray_count){ray_count=wb_rays(&t,&rays,&hits);ray_cursor=0;}
 if(ray_count && get(60)==get(62)){
  ray_chunk=ray_count-ray_cursor;if(ray_chunk>256)ray_chunk=256;
  for(uint32_t i=0;i<ray_chunk;i++){int b=16384+i*12;WbmcRay *r=rays+ray_cursor+i;
   write_coords(b,r->start);rotation(r->start,r->end,b+3);
   float dx=r->end[0]-r->start[0],dy=r->end[1]-r->start[1],dz=r->end[2]-r->start[2];put(b+5,llround(sqrt(dx*dx+dy*dy+dz*dz)*1000));
  }
  put(61,ray_chunk);ray_serial++;if(!ray_serial)ray_serial++;put(60,ray_serial);
 }
}
static void focus(SDL_Window *w){SDL_RaiseWindow(w);id app=((id(*)(id,SEL))objc_msgSend)((id)objc_getClass("NSApplication"),sel_registerName("sharedApplication"));((void(*)(id,SEL,BOOL))objc_msgSend)(app,sel_registerName("activateIgnoringOtherApps:"),YES);}
static int composite(uint64_t frame); /* below */
void bridge_swap(SDL_Window *w){
 if(!initialized){mkdir("/tmp/warbandcraft",0700);initialized=wb_open(&t,"/tmp/warbandcraft/bridge.shm");fprintf(stderr,"[WBMC] Native host loaded; transport %s\n",initialized?"ready":"failed");}
 if(initialized){
  install_hooks();find_mailbox();double time=now();WbmcGameState state={0};state.unitsPerMeter=1;state.nearZ=host_near;state.farZ=host_far;state.fovYDeg=host_fov;
  SDL_GetWindowPosition(w,&state.winX,&state.winY);SDL_GetWindowSize(w,&state.winW,&state.winH);state.aspect=(float)state.winW/state.winH;state.flags=WBMC_STATE_WINDOW_VALID;
  SDL_GL_GetDrawableSize(w,&win_w,&win_h);state.bbW=win_w;state.bbH=win_h;
  static int dimensions_logged; if(!dimensions_logged){GLint vp[4];glGetIntegerv(GL_VIEWPORT,vp);fprintf(stderr,"[WBMC] Window %dx%d, drawable %dx%d, viewport %d,%d %dx%d\n",state.winW,state.winH,win_w,win_h,vp[0],vp[1],vp[2],vp[3]);dimensions_logged=1;}
  if(mailbox){
   int64_t seq=get(7);if(seq!=last_seq){last_seq=seq;last_script_time=time;}
   if(time-last_script_time<.3){
    if(get(6)!=epoch){epoch=get(6);wb_new_mission(&t);ray_count=0;damage_total=get(13);fprintf(stderr,"[WBMC] Mission %lld, scene %lld\n",epoch,get(31));}
    state.stageId=(uint32_t)get(31)+1;coords(state.playerPos,16);coords(state.camPos,20);coords(state.camTarget,23);float up[3];coords(up,26);for(int i=0;i<3;i++)state.camUp[i]=up[i]-state.camPos[i];
    state.flags|=WBMC_STATE_CAMERA_VALID;if(get(8))state.flags|=WBMC_STATE_PLAYER_VALID;
    if(get(9)){state.flags|=WBMC_STATE_CAM_OVERRIDDEN;last_composite=get(55);}
    static double diagnostic_time;if(time-diagnostic_time>1 && render_samples){diagnostic_time=time;FILE *fp=fopen("/tmp/warbandcraft/camera-diagnostic.json","w");if(fp){fprintf(fp,"{\"actual_engine_pos\":[%f,%f,%f],\"actual_engine_forward\":[%f,%f,%f],\"actual_engine_up\":[%f,%f,%f],\"script_pos\":[%f,%f,%f],\"script_target\":[%f,%f,%f],\"ack\":%llu,\"fov\":%f}",render_pos[0],render_pos[1],render_pos[2],render_dir[0],render_dir[1],render_dir[2],render_up[0],render_up[1],render_up[2],state.camPos[0],state.camPos[1],state.camPos[2],state.camTarget[0],state.camTarget[1],state.camTarget[2],(unsigned long long)last_composite,host_fov);fclose(fp);}}
    uint32_t count=(uint32_t)get(11);if(count>WBMC_MAX_ENTITIES)count=WBMC_MAX_ENTITIES;WbmcEntity entities[WBMC_MAX_ENTITIES]={0};
    for(uint32_t i=0;i<count;i++){int b=256+i*8;WbmcEntity *e=entities+i;e->id=wb_entity_id(&t,(uint32_t)get(b+3));e->kind=WBMC_ENT_SMALL_MONSTER;coords(e->pos,b);e->hp=get(b+4);e->maxHp=get(b+5)>0?e->hp*100/get(b+5):e->hp;e->quat[3]=1;e->flags=2;memcpy(e->boxCenter,e->pos,12);e->boxCenter[1]+=.9f;e->boxHalf[0]=e->boxHalf[2]=.35f;e->boxHalf[1]=.9f;snprintf(e->name,48,"Warband agent %lld",get(b+3));}
    wb_entities(&t,entities,count);
    if(get(13)>damage_total){float from[3];coords(from,70);wb_player_hit_from(&t,get(13)-damage_total,get(15)>0?get(15):100,from);}damage_total=get(13);
    WbmcControl ctrl;int drive=wb_control(&t,&ctrl,time) && get(8);
    if(drive){applied=ctrl;pose_history[ctrl.mcFrame%64]=ctrl;put(40,ctrl.flags);put(42,ctrl.mcFrame);write_coords(44,ctrl.camPos);rotation(ctrl.camPos,ctrl.camTarget,47);put(49,get(30));write_coords(50,ctrl.hunterPos);put(53,llround((180-ctrl.hunterYawDeg)*1000));put(54,5);}else{put(40,0);put(54,0);}
    handle_rays();WbmcDamage d;int n=0;if(!get(10000)){while(n<128 && wb_damage(&t,&d)){put(10001+n*2,(uint32_t)d.id);put(10002+n*2,llround(d.amount));n++;}put(10000,n);}
    uint32_t *army=(void*)(t.mem+0x2a0000);uint32_t r=army[1],wr=__atomic_load_n(army,__ATOMIC_ACQUIRE);
    if(wr-r<=64 && r!=wr && !get(64)){int *entry=(void*)(army+4+((r%64)*2));put(65,entry[0]);put(64,entry[1]+1);__atomic_store_n(army+1,r+1,__ATOMIC_RELEASE);}
    uint32_t *death=(void*)(t.mem+0x2a1000);uint32_t ds=__atomic_load_n(death,__ATOMIC_ACQUIRE);if(ds!=death_ack){death_ack=ds;if(death[1]==t.epoch)put(66,1);}
    if(drive && get(9) && (ctrl.flags&WBMC_CTRL_COMPOSITE) && composite(last_composite))state.flags|=WBMC_STATE_COMPOSITING;
   }
  }
  wb_publish(&t,&state);
  WbmcHeader *h=(void*)t.mem;if(h->mhwFocusReq!=t.focus_ack){t.focus_ack=h->mhwFocusReq;focus(w);}
 }
}
int bridge_event(SDL_Event *e){
  if(initialized && e->type==SDL_WINDOWEVENT && e->window.event==SDL_WINDOWEVENT_FOCUS_LOST && ((mailbox && get(9)) || now()-t.last_mc_time<.75))return 0;
  if(initialized && e->type==SDL_KEYDOWN && !e->key.repeat && e->key.keysym.sym==SDLK_F8)wb_request_mc(&t);
 return 1;
}
void bridge_shutdown(void){
 if(mailbox && readable(mailbox,65536*stride)){put(40,0);put(54,0);}
 if(original_bind){void **p=dlsym(RTLD_DEFAULT,"__glewBindFramebuffer");if(p)*p=original_bind;}
 if(original_use){void **p=dlsym(RTLD_DEFAULT,"__glewUseProgram");if(p)*p=original_use;}
 if(original_uniform){void **p=dlsym(RTLD_DEFAULT,"__glewUniformMatrix4fv");if(p)*p=original_uniform;}
 if(initialized)wb_close(&t);
}

static unsigned char *frames,*snapshot;
static GLuint textures[4],program;
static GLuint shader(GLenum kind,const char *src){GLuint s=glCreateShader(kind);glShaderSource(s,1,&src,NULL);glCompileShader(s);GLint ok;glGetShaderiv(s,GL_COMPILE_STATUS,&ok);if(!ok){char log[1024];glGetShaderInfoLog(s,1024,NULL,log);fprintf(stderr,"[WBMC] Shader error: %s\n",log);return 0;}return s;}
static int composite(uint64_t frame){
 if(!frame)return 0;
 if(!frames){int fd=open("/tmp/warbandcraft/frames.shm",O_RDONLY);if(fd<0)return 0;struct stat s;fstat(fd,&s);if(s.st_size!=WBMC_FRAMES_FILE_SIZE){close(fd);return 0;}void*p=mmap(NULL,s.st_size,PROT_READ,MAP_SHARED,fd,0);close(fd);if(p==MAP_FAILED)return 0;frames=p;snapshot=malloc(WBMC_FRAME_SLOT_SIZE);}
 WbmcFramesHeader *header=(void*)frames;if(header->magic!=WBMC_FRAMES_MAGIC || header->version!=1)return 0;
 unsigned char *slot=frames+0x1000+(frame%3)*WBMC_FRAME_SLOT_SIZE;
 WbmcFrameHeader *source=(void*)slot;uint32_t seq=__atomic_load_n(&source->seq,__ATOMIC_ACQUIRE);if(!seq || seq&1 || source->frameId!=frame)return 0;
 uint32_t w=source->width,h=source->height;if(!w || !h || w>1920 || h>1200)return 0;size_t layer=(size_t)w*h*4;
 memcpy(snapshot,slot,WBMC_FRAME_HDR+layer*3);__atomic_thread_fence(__ATOMIC_ACQUIRE);if(seq!=source->seq)return 0;
 WbmcFrameHeader *f=(void*)snapshot;inside_compositor=1;
 WbmcControl *pose=pose_history+frame%64;
 if(pose->mcFrame==frame){double pe=0,de=0,ue=0;int order[3]={0,2,1};for(int i=0;i<3;i++){int j=order[i];double x=render_pos[i]-pose->camPos[j];pe+=x*x;x=render_dir[i]-(pose->camTarget[j]-pose->camPos[j]);de+=x*x;x=render_up[i]-pose->camUp[j];ue+=x*x;}
  static double alignment_time;if(now()-alignment_time>.25){alignment_time=now();FILE *fp=fopen("/tmp/warbandcraft/alignment.json","w");if(fp){fprintf(fp,"{\"frame\":%llu,\"position_error_m\":%f,\"direction_error\":%f,\"up_error\":%f,\"native_fov\":%f,\"mc_fov\":%f,\"native_dimensions\":[%d,%d],\"mc_dimensions\":[%u,%u]}",(unsigned long long)frame,sqrt(pe),sqrt(de),sqrt(ue),host_fov,f->fovYDeg,win_w,win_h,w,h);fclose(fp);}}
  // Never draw pixels belonging to a camera the native renderer did not use.
  if(pe>.0004 || de>.000004 || ue>.000004 || fabs(host_fov-f->fovYDeg)>.05){inside_compositor=0;return 0;}}
 GLint oldprogram,oldfbo,active,pack,unpack,oldvao=0;glGetIntegerv(GL_CURRENT_PROGRAM,&oldprogram);glGetIntegerv(GL_FRAMEBUFFER_BINDING,&oldfbo);glGetIntegerv(GL_ACTIVE_TEXTURE,&active);glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING,&pack);glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING,&unpack);
 glPushAttrib(GL_ALL_ATTRIB_BITS);glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);glBindBuffer(GL_PIXEL_PACK_BUFFER,0);
 if(!program){
  const char *vs="#version 120\nvarying vec2 uv;void main(){gl_Position=gl_Vertex;uv=gl_MultiTexCoord0.xy;}";
  const char *fs="#version 120\nuniform sampler2D worldColor,mcDepth,guiColor,hostDepth;uniform vec4 clips;uniform int haveDepth;varying vec2 uv;float linearZ(float d,float n,float f){return 2.0*n*f/(f+n-(d*2.0-1.0)*(f-n));}void main(){vec4 c=texture2D(worldColor,uv);float md=texture2D(mcDepth,uv).r;float hd=texture2D(hostDepth,uv).r;if(haveDepth==1 && hd<0.999999 && linearZ(md,clips.x,clips.y)>linearZ(hd,clips.z,clips.w)+0.03)c=vec4(0.0);vec4 g=texture2D(guiColor,uv);gl_FragColor=g+c*(1.0-g.a);}";
  GLuint a=shader(GL_VERTEX_SHADER,vs),b=shader(GL_FRAGMENT_SHADER,fs);program=glCreateProgram();glAttachShader(program,a);glAttachShader(program,b);glLinkProgram(program);glDeleteShader(a);glDeleteShader(b);glGenTextures(4,textures);
  fprintf(stderr,"[WBMC] Native compositor initialized (%dx%d, scene FBO %u)\n",w,h,scene_fbo);
 }
 glUseProgram(program);
 for(int i=0;i<4;i++){glActiveTexture(GL_TEXTURE0+i);glBindTexture(GL_TEXTURE_2D,textures[i]);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  if(i<3)glTexImage2D(GL_TEXTURE_2D,0,i==1?GL_R32F:GL_RGBA8,w,h,0,i==1?GL_RED:GL_BGRA,i==1?GL_FLOAT:GL_UNSIGNED_BYTE,snapshot+WBMC_FRAME_HDR+layer*i);
 }
 glBindFramebuffer(GL_FRAMEBUFFER,scene_fbo?scene_fbo:0);glCopyTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT,0,0,win_w,win_h,0);
 GLenum error=glGetError();glBindFramebuffer(GL_FRAMEBUFFER,0);glViewport(0,0,win_w,win_h);
 glUniform1i(glGetUniformLocation(program,"worldColor"),0);glUniform1i(glGetUniformLocation(program,"mcDepth"),1);glUniform1i(glGetUniformLocation(program,"guiColor"),2);glUniform1i(glGetUniformLocation(program,"hostDepth"),3);
 glUniform4f(glGetUniformLocation(program,"clips"),f->mcNear,f->mcFar,host_near,host_far);glUniform1i(glGetUniformLocation(program,"haveDepth"),error==GL_NO_ERROR?1:0);
 glDisable(GL_DEPTH_TEST);glDepthMask(GL_FALSE);glDisable(GL_CULL_FACE);glDisable(GL_ALPHA_TEST);glDisable(GL_SCISSOR_TEST);glDisable(GL_STENCIL_TEST);glEnable(GL_BLEND);glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);glColorMask(1,1,1,1);
 glBegin(GL_QUADS);glTexCoord2f(0,0);glVertex2f(-1,-1);glTexCoord2f(1,0);glVertex2f(1,-1);glTexCoord2f(1,1);glVertex2f(1,1);glTexCoord2f(0,1);glVertex2f(-1,1);glEnd();
 glUseProgram(oldprogram);glBindFramebuffer(GL_FRAMEBUFFER,oldfbo);glBindBuffer(GL_PIXEL_PACK_BUFFER,pack);glBindBuffer(GL_PIXEL_UNPACK_BUFFER,unpack);glPopAttrib();glActiveTexture(active);inside_compositor=0;return 1;
}
