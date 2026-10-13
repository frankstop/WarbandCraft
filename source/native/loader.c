#include <SDL.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#define INTERPOSE(repl,orig) __attribute__((used)) static struct {const void *a,*b;} ip_##orig __attribute__((section("__DATA,__interpose")))={(void*)&repl,(void*)&orig};
static void *core;
static void (*tick)(SDL_Window*),(*shutdown_core)(void),(*prepare_save_core)(void);
static int (*event_core)(SDL_Event*);
static struct timespec modified;
static unsigned int checks,generation;
static void reload(void){
 const char *path=getenv("WBMC_CORE");if(!path)return;
 struct stat st;if(stat(path,&st))return;
 if(core && st.st_mtimespec.tv_sec==modified.tv_sec && st.st_mtimespec.tv_nsec==modified.tv_nsec)return;
 mkdir("/tmp/warbandcraft",0700);char tmp[256];snprintf(tmp,sizeof(tmp),"/tmp/warbandcraft/core-%d-%u.dylib",getpid(),++generation);
 FILE *in=fopen(path,"rb"),*out=fopen(tmp,"wb");if(!in||!out){if(in)fclose(in);if(out)fclose(out);return;}
 char buf[16384];size_t n;while((n=fread(buf,1,sizeof(buf),in)))fwrite(buf,1,n,out);fclose(in);fclose(out);
 void *next=dlopen(tmp,RTLD_NOW|RTLD_LOCAL);if(!next){fprintf(stderr,"[WBMC] Reload failed: %s\n",dlerror());return;}
 void(*next_tick)(SDL_Window*)=dlsym(next,"bridge_swap");int(*next_event)(SDL_Event*)=dlsym(next,"bridge_event");
 if(!next_tick||!next_event){dlclose(next);return;}
 if(shutdown_core)shutdown_core();if(core)dlclose(core);
 core=next;tick=next_tick;event_core=next_event;shutdown_core=dlsym(core,"bridge_shutdown");prepare_save_core=dlsym(core,"bridge_prepare_save");modified=st.st_mtimespec;
 fprintf(stderr,"[WBMC] Loaded native core generation %u\n",generation);
}
static void swap(SDL_Window*w){if(!core||++checks%60==0)reload();if(tick)tick(w);SDL_GL_SwapWindow(w);}
static int poll(SDL_Event*e){int r;while((r=SDL_PollEvent(e)) && event_core && !event_core(e)){}return r;}
INTERPOSE(swap,SDL_GL_SwapWindow)
INTERPOSE(poll,SDL_PollEvent)

#include "save_hooks.inc"
