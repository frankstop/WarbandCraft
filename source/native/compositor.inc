/* Compatibility-profile compositor. Both depth images are ordinary float
 * luminance textures: Apple's legacy context cannot reliably sample GL_R32F. */
static unsigned char *frames,*snapshot;
static float *host_depth;
static size_t host_depth_capacity;
static GLuint textures[5],program,depth_read_fbo;
static int depth_valid,depth_width,depth_height,depth_samples;
static unsigned char *host_color;
static size_t host_color_capacity;
static GLenum depth_error;
static void clear_gl_errors(void){for(int i=0;i<32 && glGetError()!=GL_NO_ERROR;i++){} }
static GLuint shader(GLenum kind,const char *src){
 GLuint s=glCreateShader(kind);glShaderSource(s,1,&src,NULL);glCompileShader(s);
 GLint ok=0;glGetShaderiv(s,GL_COMPILE_STATUS,&ok);
 if(!ok){char log[2048];glGetShaderInfoLog(s,sizeof(log),NULL,log);fprintf(stderr,"[WBMC] Shader error: %s\n",log);glDeleteShader(s);return 0;}return s;
}
static const char *wb_composite_vs="#version 120\nvarying vec2 uv;void main(){gl_Position=gl_Vertex;uv=gl_MultiTexCoord0.xy;}";
static const char *wb_composite_fs=
 "#version 120\n"
 "uniform sampler2D worldColor,mcDepth,guiColor,hostDepth,hostColor;uniform vec4 clips;uniform int debugDepth,lightCount;varying vec2 uv;"
 "uniform vec3 eye,forwardDir,upDir,rightDir;uniform vec2 lens;uniform vec4 lights[8],lightColors[8];"
 "float linearZ(float d,float n,float f){return 2.0*n*f/(f+n-(d*2.0-1.0)*(f-n));}"
 "vec3 at(vec2 v,float z){vec2 q=v*2.0-1.0;return eye+z*(forwardDir+rightDir*q.x*lens.x*lens.y+upDir*q.y*lens.x);}"
 "float shadow(vec3 p,vec3 light){for(int j=1;j<=6;j++){vec3 q=mix(p,light,float(j)/7.0)-eye;float z=dot(q,forwardDir);if(z<=clips.z)continue;"
 "vec2 v=vec2(dot(q,rightDir)/(z*lens.x*lens.y),dot(q,upDir)/(z*lens.x))*0.5+0.5;"
 "if(v.x<0.0||v.x>1.0||v.y<0.0||v.y>1.0)continue;float d=texture2D(hostDepth,v).r;"
 "if(d<0.999999 && linearZ(d,clips.z,clips.w)<z-0.15)return 0.0;"
 "float m=texture2D(mcDepth,v).r;float coverage=texture2D(worldColor,v).a;"
 "if(m<0.999999 && coverage>0.95 && linearZ(m,clips.x,clips.y)<z-0.15)return 0.0;}return 1.0;}"
 "void main(){vec4 c=texture2D(worldColor,uv);float md=texture2D(mcDepth,uv).r;float hd=texture2D(hostDepth,uv).r;"
 "if(debugDepth==1){float v=clamp(linearZ(hd,clips.z,clips.w)/30.0,0.0,1.0);gl_FragColor=vec4(v,v,v,1.0);return;}"
 "if(hd<0.999999 && linearZ(md,clips.x,clips.y)>linearZ(hd,clips.z,clips.w)+0.03)c=vec4(0.0);"
 "vec4 host=texture2D(hostColor,uv);"
 "if(hd<0.999999 && lightCount>0){vec3 p=at(uv,linearZ(hd,clips.z,clips.w));vec3 n=normalize(cross(dFdx(p),dFdy(p)));"
 "if(dot(n,eye-p)<0.0)n=-n;vec3 energy=vec3(0.0);"
 "for(int i=0;i<8;i++){if(i>=lightCount)break;vec3 delta=lights[i].xyz-p;float dist=length(delta);float r=lights[i].w;"
 "if(dist<r && dist>0.01){float fall=1.0-dist/r;float diffuse=max(dot(n,delta/dist),0.0);"
 "energy+=lightColors[i].rgb*fall*fall*diffuse*shadow(p+n*0.1,lights[i].xyz);}}"
 "host.rgb=pow(clamp(pow(host.rgb,vec3(2.2))+energy*0.18,0.0,1.0),vec3(1.0/2.2));}"
 "vec4 g=texture2D(guiColor,uv);gl_FragColor=g+(c+host*(1.0-c.a))*(1.0-g.a);}";


static int read_world_depth(void){
 // Warband reuses scene_fbo for smaller postprocessing targets before swap.
 // Read the attachment recorded during perspective drawing, not its later replacement.
 depth_valid=0;depth_width=depth_height=depth_samples=0;
 clear_gl_errors();
 if(scene_fbo){
  if(!scene_depth_name || scene_depth_type!=GL_RENDERBUFFER)return 0;
  GLint rb,rw,rh,rs;glGetIntegerv(GL_RENDERBUFFER_BINDING,&rb);
  glBindRenderbuffer(GL_RENDERBUFFER,scene_depth_name);
  glGetRenderbufferParameteriv(GL_RENDERBUFFER,GL_RENDERBUFFER_WIDTH,&rw);
  glGetRenderbufferParameteriv(GL_RENDERBUFFER,GL_RENDERBUFFER_HEIGHT,&rh);
  glGetRenderbufferParameteriv(GL_RENDERBUFFER,GL_RENDERBUFFER_SAMPLES,&rs);
  glBindRenderbuffer(GL_RENDERBUFFER,rb);
  depth_width=rw;depth_height=rh;depth_samples=rs;
  if(rw!=win_w || rh!=win_h || rs!=0)return 0;
  if(!depth_read_fbo)glGenFramebuffers(1,&depth_read_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER,depth_read_fbo);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,scene_depth_name);
  glReadBuffer(GL_NONE);glDrawBuffer(GL_NONE);

 }else glBindFramebuffer(GL_READ_FRAMEBUFFER,0);
 if(glCheckFramebufferStatus(GL_READ_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)return 0;
 for(size_t i=0;i<(size_t)win_w*win_h;i++)host_depth[i]=NAN;
 glReadPixels(0,0,win_w,win_h,GL_DEPTH_COMPONENT,GL_FLOAT,host_depth);
 depth_error=glGetError();
 if(depth_error){static GLenum last; if(last!=depth_error){fprintf(stderr,"[WBMC] Native depth unavailable (GL %#x); refusing world overlay\n",depth_error);last=depth_error;}return 0;}
 for(size_t i=0;i<(size_t)win_w*win_h;i++)if(!isfinite(host_depth[i]) || host_depth[i]<0 || host_depth[i]>1)return 0;
 depth_valid=1;return 1;
}

static int composite(uint64_t frame){
 if(!frame||!render_samples||win_w<=0||win_h<=0)return 0;
 if(!frames){
  int fd=open("/tmp/warbandcraft/frames.shm",O_RDONLY);if(fd<0)return 0;
  struct stat s;if(fstat(fd,&s)||s.st_size!=WBMC_FRAMES_FILE_SIZE){close(fd);return 0;}
  void*p=mmap(NULL,s.st_size,PROT_READ,MAP_SHARED,fd,0);close(fd);if(p==MAP_FAILED)return 0;
  frames=p;snapshot=malloc(WBMC_FRAME_SLOT_SIZE);if(!snapshot){munmap(frames,WBMC_FRAMES_FILE_SIZE);frames=NULL;return 0;}
 }
 WbmcFramesHeader *header=(void*)frames;if(header->magic!=WBMC_FRAMES_MAGIC||header->version!=1)return 0;
 unsigned char *slot=frames+0x1000+(frame%3)*WBMC_FRAME_SLOT_SIZE;
 WbmcFrameHeader *source=(void*)slot;uint32_t seq=__atomic_load_n(&source->seq,__ATOMIC_ACQUIRE);
 if(!seq||(seq&1)||source->frameId!=frame)return 0;
 uint32_t w=source->width,h=source->height;if(!w||!h||w>WBMC_FRAME_MAX_W||h>WBMC_FRAME_MAX_H)return 0;
 size_t layer=(size_t)w*h*4;memcpy(snapshot,slot,WBMC_FRAME_HDR+layer*3);
 __atomic_thread_fence(__ATOMIC_ACQUIRE);if(seq!=__atomic_load_n(&source->seq,__ATOMIC_ACQUIRE))return 0;
 WbmcFrameHeader *f=(void*)snapshot;WbmcControl *pose=pose_history+frame%64;
 if(pose->mcFrame!=frame||f->poseId!=frame||f->flags!=3||!wb_valid_clips(f->mcNear,f->mcFar)||!wb_valid_clips(host_near,host_far))return 0;
 double pe=0,de=0,ue=0;int order[3]={0,2,1};
 for(int i=0;i<3;i++){int j=order[i];double x=render_pos[i]-pose->camPos[j];pe+=x*x;x=render_dir[i]-(pose->camTarget[j]-pose->camPos[j]);de+=x*x;x=render_up[i]-pose->camUp[j];ue+=x*x;}
 if(pe>.0004||de>.000004||ue>.000004||fabs(host_fov-f->fovYDeg)>.05)return 0;
 inside_compositor=1;
 GLint oldprogram,oldread,olddraw,active,pack,unpack;
 glGetIntegerv(GL_CURRENT_PROGRAM,&oldprogram);glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING,&oldread);glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&olddraw);
 glGetIntegerv(GL_ACTIVE_TEXTURE,&active);glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING,&pack);glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING,&unpack);
 glPushAttrib(GL_ALL_ATTRIB_BITS);glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
 glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);glBindBuffer(GL_PIXEL_PACK_BUFFER,0);
 glPixelStorei(GL_PACK_SWAP_BYTES,GL_FALSE);glPixelStorei(GL_UNPACK_SWAP_BYTES,GL_FALSE);glPixelTransferf(GL_DEPTH_SCALE,1);glPixelTransferf(GL_DEPTH_BIAS,0);
 glPixelStorei(GL_PACK_ALIGNMENT,4);glPixelStorei(GL_PACK_ROW_LENGTH,0);glPixelStorei(GL_PACK_SKIP_ROWS,0);glPixelStorei(GL_PACK_SKIP_PIXELS,0);
 glPixelStorei(GL_UNPACK_ALIGNMENT,4);glPixelStorei(GL_UNPACK_ROW_LENGTH,0);glPixelStorei(GL_UNPACK_SKIP_ROWS,0);glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
 int ok=0;size_t pixels=(size_t)win_w*win_h;
 if(pixels>host_depth_capacity){float *p=realloc(host_depth,pixels*sizeof(float));if(!p)goto restore;host_depth=p;host_depth_capacity=pixels;}
 if(!read_world_depth())goto restore;
 if(pixels>host_color_capacity){unsigned char *p=realloc(host_color,pixels*4);if(!p)goto restore;host_color=p;host_color_capacity=pixels;}
 glBindFramebuffer(GL_READ_FRAMEBUFFER,0);glReadBuffer(GL_BACK);clear_gl_errors();
 glReadPixels(0,0,win_w,win_h,GL_BGRA,GL_UNSIGNED_BYTE,host_color);depth_error=glGetError();if(depth_error)goto restore;
 if(!program){
  GLuint a=shader(GL_VERTEX_SHADER,wb_composite_vs),b=shader(GL_FRAGMENT_SHADER,wb_composite_fs);
  if(!a||!b){if(a)glDeleteShader(a);if(b)glDeleteShader(b);goto restore;}
  program=glCreateProgram();glAttachShader(program,a);glAttachShader(program,b);glLinkProgram(program);glDeleteShader(a);glDeleteShader(b);
  GLint linked=0;glGetProgramiv(program,GL_LINK_STATUS,&linked);if(!linked){char log[2048];glGetProgramInfoLog(program,sizeof(log),NULL,log);fprintf(stderr,"[WBMC] Compositor link failed: %s\n",log);glDeleteProgram(program);program=0;goto restore;}
  glGenTextures(5,textures);fprintf(stderr,"[WBMC] Depth compositor ready (%ux%u, world FBO %u)\n",w,h,scene_fbo);
 }
 glUseProgram(program);clear_gl_errors();
 for(int i=0;i<5;i++){
  glActiveTexture(GL_TEXTURE0+i);glBindTexture(GL_TEXTURE_2D,textures[i]);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_BASE_LEVEL,0);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,0);
  if(i==1)glTexImage2D(GL_TEXTURE_2D,0,GL_LUMINANCE32F_ARB,w,h,0,GL_LUMINANCE,GL_FLOAT,snapshot+WBMC_FRAME_HDR+layer);
  else if(i==3)glTexImage2D(GL_TEXTURE_2D,0,GL_LUMINANCE32F_ARB,win_w,win_h,0,GL_LUMINANCE,GL_FLOAT,host_depth);
  else if(i==4)glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,win_w,win_h,0,GL_BGRA,GL_UNSIGNED_BYTE,host_color);
  else glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,w,h,0,GL_BGRA,GL_UNSIGNED_BYTE,snapshot+WBMC_FRAME_HDR+layer*i);
 }
 depth_error=glGetError();if(depth_error)goto restore;
 glBindFramebuffer(GL_DRAW_FRAMEBUFFER,0);glDrawBuffer(GL_BACK);glViewport(0,0,win_w,win_h);
 glUniform1i(glGetUniformLocation(program,"worldColor"),0);glUniform1i(glGetUniformLocation(program,"mcDepth"),1);
 glUniform1i(glGetUniformLocation(program,"guiColor"),2);glUniform1i(glGetUniformLocation(program,"hostDepth"),3);
 glUniform1i(glGetUniformLocation(program,"hostColor"),4);
 upload_lights(program,pose);
 glUniform4f(glGetUniformLocation(program,"clips"),f->mcNear,f->mcFar,host_near,host_far);
 glUniform1i(glGetUniformLocation(program,"debugDepth"),(pose->flags&WBMC_CTRL_DEBUG_DEPTH)?1:0);
 glDisable(GL_DEPTH_TEST);glDepthMask(GL_FALSE);glDisable(GL_CULL_FACE);glDisable(GL_ALPHA_TEST);glDisable(GL_SCISSOR_TEST);glDisable(GL_STENCIL_TEST);
 glDisable(GL_BLEND);glColorMask(1,1,1,1);
 glBegin(GL_QUADS);glTexCoord2f(0,0);glVertex2f(-1,-1);glTexCoord2f(1,0);glVertex2f(1,-1);glTexCoord2f(1,1);glVertex2f(1,1);glTexCoord2f(0,1);glVertex2f(-1,1);glEnd();
 ok=glGetError()==GL_NO_ERROR;
restore:;
 static double diagnostic_time;
 if(now()-diagnostic_time>.5){
  diagnostic_time=now();FILE *fp=fopen("/tmp/warbandcraft/alignment.json","w");
  if(fp){fprintf(fp,"{\"frame\":%llu,\"position_error_m\":%f,\"direction_error\":%f,\"up_error\":%f,\"native_fov\":%f,\"mc_fov\":%f,\"native_dimensions\":[%d,%d],\"mc_dimensions\":[%u,%u],\"world_fbo\":%u,\"depth_error\":%u,\"depth_valid\":%d,\"depth_attachment\":%u,\"depth_dimensions\":[%d,%d],\"depth_samples\":%d}",(unsigned long long)frame,sqrt(pe),sqrt(de),sqrt(ue),host_fov,f->fovYDeg,win_w,win_h,w,h,scene_fbo,depth_error,depth_valid,scene_depth_name,depth_width,depth_height,depth_samples);fclose(fp);}
 }
 glUseProgram(oldprogram);glBindFramebuffer(GL_READ_FRAMEBUFFER,oldread);glBindFramebuffer(GL_DRAW_FRAMEBUFFER,olddraw);
 glBindBuffer(GL_PIXEL_PACK_BUFFER,pack);glBindBuffer(GL_PIXEL_UNPACK_BUFFER,unpack);glPopClientAttrib();glPopAttrib();glActiveTexture(active);inside_compositor=0;return ok;
}

static void compositor_shutdown(void){
 if(depth_read_fbo){glDeleteFramebuffers(1,&depth_read_fbo);depth_read_fbo=0;}
 if(program){glDeleteProgram(program);program=0;}
 if(textures[0])glDeleteTextures(5,textures);memset(textures,0,sizeof(textures));
 if(frames)munmap(frames,WBMC_FRAMES_FILE_SIZE);frames=NULL;
 free(snapshot);snapshot=NULL;free(host_depth);host_depth=NULL;host_depth_capacity=0;
 free(host_color);host_color=NULL;host_color_capacity=0;
}
