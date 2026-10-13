/* Save interception is gated until actual engine I/O paths pass acceptance.
 * Never infer a coordinated snapshot merely from the existence of a .sav. */
#include <pthread.h>
#include <limits.h>
#include <time.h>
typedef struct {FILE *file;char path[PATH_MAX];} WbSaveFile;
static WbSaveFile save_files[16];
static pthread_mutex_t save_mutex=PTHREAD_MUTEX_INITIALIZER;
static unsigned long save_serial;
static int is_bridge_save(const char *path){
 const char *enabled=getenv("WBMC_PAIRED_SAVES");size_t n=path?strlen(path):0;
 const char *module=getenv("WBMC_MODULE");if(!module)module="WarbandCraft";
 char component[256];snprintf(component,sizeof(component),"Savegames/%s/",module);
 return enabled&&!strcmp(enabled,"1")&&n>4&&strstr(path,component)&&!strcmp(path+n-4,".sav")&&!strstr(path,"last_savegame_backup");
}
static void json_string(FILE *f,const char *s){
 fputc('"',f);for(;*s;s++){if(*s=='"'||*s=='\\')fputc('\\',f);if((unsigned char)*s>=32)fputc(*s,f);}fputc('"',f);
}
static int save_handshake(const char *action,const char *path){
 mkdir("/tmp/warbandcraft",0700);char id[96],resolved[PATH_MAX];
 snprintf(id,sizeof(id),"%d-%lu",getpid(),__atomic_add_fetch(&save_serial,1,__ATOMIC_RELAXED));
 const char *absolute=realpath(path,resolved);if(!absolute){if(path[0]=='/')absolute=path;else{if(!getcwd(resolved,sizeof(resolved)))return 0;size_t n=strlen(resolved);snprintf(resolved+n,sizeof(resolved)-n,"/%s",path);absolute=resolved;}}
 FILE *f=fopen("/tmp/warbandcraft/save-request.next","w");if(!f)return 0;
 fputs("{\"id\":",f);json_string(f,id);fputs(",\"action\":",f);json_string(f,action);fputs(",\"path\":",f);json_string(f,absolute);fputs("}\n",f);fclose(f);
 if(rename("/tmp/warbandcraft/save-request.next","/tmp/warbandcraft/save-request.json"))return 0;
 for(int i=0;i<750;i++){
  char response[160]={0};FILE *r=fopen("/tmp/warbandcraft/save-response.txt","r");if(r){fgets(response,sizeof(response),r);fclose(r);}
  size_t n=strlen(id);if(!strncmp(response,id,n)&&response[n]==' '){
   int ok=!strcmp(response+n+1,"ok")||(!strcmp(action,"prepare-load")&&!strcmp(response+n+1,"unpaired"));
   if(!ok)fprintf(stderr,"[WBMC] %s failed: %s\n",action,response);return ok;
  }usleep(20000);
 }
 fprintf(stderr,"[WBMC] %s timed out: no paired checkpoint acknowledgement\n",action);return 0;
}
static FILE *wb_fopen(const char *path,const char *mode){
 int target=is_bridge_save(path),writing=target&&(strchr(mode,'w')||strchr(mode,'a'));
 if(target){
  if(writing&&prepare_save_core)prepare_save_core();
  if(!save_handshake(writing?"prepare-save":"prepare-load",path)){errno=EIO;return NULL;}
 }
 FILE *f=fopen(path,mode);
 if(f&&writing){pthread_mutex_lock(&save_mutex);for(int i=0;i<16;i++)if(!save_files[i].file){save_files[i].file=f;snprintf(save_files[i].path,sizeof(save_files[i].path),"%s",path);break;}pthread_mutex_unlock(&save_mutex);}
 return f;
}
static int wb_fclose(FILE *file){
 char path[PATH_MAX]={0};pthread_mutex_lock(&save_mutex);
 for(int i=0;i<16;i++)if(save_files[i].file==file){snprintf(path,sizeof(path),"%s",save_files[i].path);save_files[i].file=NULL;break;}
 pthread_mutex_unlock(&save_mutex);int result=fclose(file);
 if(!result&&path[0]&&!save_handshake("commit-save",path))fprintf(stderr,"[WBMC] Native save exists, but paired checkpoint failed; see checkpoint log\n");
 return result;
}
INTERPOSE(wb_fopen,fopen)
INTERPOSE(wb_fclose,fclose)
