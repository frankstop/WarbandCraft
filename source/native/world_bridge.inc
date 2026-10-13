#define WB_CONTEXT_OFFSET 0x2b0000
#define WB_GEOMETRY_OFFSET 0x300000
#define WB_MAX_COLLIDERS 2048
static float mirror_boxes[WB_MAX_COLLIDERS][6];
static uint32_t mirror_revision,mirror_count,mirror_cursor,mirror_previous;
static int mirror_active;
static void publish_context(void){
 uint32_t *p=(void*)(t.mem+WB_CONTEXT_OFFSET);uint32_t n=__atomic_load_n(p,__ATOMIC_ACQUIRE);if(n&1)n++;
 __atomic_store_n(p,n+1,__ATOMIC_RELEASE);__atomic_thread_fence(__ATOMIC_SEQ_CST);
 p[1]=1;p[2]=get(33);p[3]=t.epoch;p[4]=get(31)+1;p[5]=get(32);
 for(int i=0;i<4;i++)p[6+i]=get(65520+i);
 __atomic_store_n(p,n+2,__ATOMIC_RELEASE);
}
static void mirror_geometry(void){
 if(get(33)!=2)return;uint32_t *p=(void*)(t.mem+WB_GEOMETRY_OFFSET);
 uint32_t seq=__atomic_load_n(p,__ATOMIC_ACQUIRE);
 if(seq&&!(seq&1)&&seq!=mirror_revision && !mirror_active){
  uint32_t count=p[3];if(p[1]!=1||p[2]!=t.epoch||count>WB_MAX_COLLIDERS)return;
  float scratch[WB_MAX_COLLIDERS][6];memcpy(scratch,p+16,count*sizeof(scratch[0]));__atomic_thread_fence(__ATOMIC_ACQUIRE);
  if(seq!=__atomic_load_n(p,__ATOMIC_ACQUIRE))return;
  for(uint32_t i=0;i<count;i++)for(int j=0;j<3;j++)if(!isfinite(scratch[i][j])||!isfinite(scratch[i][j+3])||scratch[i][j+3]<=scratch[i][j]||fabs(scratch[i][j])>1e6||scratch[i][j+3]-scratch[i][j]>256)return;
  memcpy(mirror_boxes,scratch,count*sizeof(scratch[0]));mirror_count=count;mirror_cursor=0;mirror_active=1;mirror_revision=seq;
 }
 if(!mirror_active||get(30000))return;
 uint32_t total=mirror_count>mirror_previous?mirror_count:mirror_previous;int n=0;
 while(mirror_cursor<total && n<32){
  uint32_t i=mirror_cursor++;int b=31000+n*7;put(b,i);
  if(i<mirror_count){
   int order[3]={0,2,1};
   for(int k=0;k<3;k++){int j=order[k];put(b+1+k,llround(mirror_boxes[i][j]*1000));put(b+4+k,llround((mirror_boxes[i][j+3]-mirror_boxes[i][j])*1000));}
  }else{put(b+1,0);put(b+2,0);put(b+3,-10000000);for(int k=0;k<3;k++)put(b+4+k,1);}
  n++;
 }
 put(30000,n);
 if(mirror_cursor==total){mirror_active=0;mirror_previous=mirror_count;put(30001,mirror_count);}
}
static void reset_world_bridge(void){mirror_revision=mirror_count=mirror_cursor=0;mirror_previous=get(33)==2?(uint32_t)get(30001):0;if(mirror_previous>WB_MAX_COLLIDERS)mirror_previous=0;mirror_active=0;}
