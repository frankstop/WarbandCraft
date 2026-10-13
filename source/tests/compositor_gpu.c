#include <assert.h>
#include "../native/host_adapter.c"
#include <OpenGL/OpenGL.h>
static float depth_at(float z,float n,float f){return .5f*((f+n-2*n*f/z)/(f-n)+1);}
int main(void){
 CGLPixelFormatAttribute attrs[]={kCGLPFAAccelerated,(CGLPixelFormatAttribute)0};
 CGLPixelFormatObj format;GLint count;CGLContextObj context;
 assert(CGLChoosePixelFormat(attrs,&format,&count)==kCGLNoError);
 assert(CGLCreateContext(format,NULL,&context)==kCGLNoError);CGLDestroyPixelFormat(format);CGLSetCurrentContext(context);
 assert(glewInit()==GLEW_OK);clear_gl_errors();
 printf("GPU: %s; OpenGL %s\n",glGetString(GL_RENDERER),glGetString(GL_VERSION));
 GLuint a=shader(GL_VERTEX_SHADER,wb_composite_vs),b=shader(GL_FRAGMENT_SHADER,wb_composite_fs);assert(a&&b);
 GLuint p=glCreateProgram();glAttachShader(p,a);glAttachShader(p,b);glLinkProgram(p);GLint linked;glGetProgramiv(p,GL_LINK_STATUS,&linked);assert(linked);
 GLuint fbo,color;glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);glGenTextures(1,&color);glBindTexture(GL_TEXTURE_2D,color);
 glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,3,1,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,color,0);
 assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);
 unsigned char green[]={0,255,0,255,0,255,0,255,0,255,0,255};unsigned char gui[]={0,0,0,0,0,0,0,0,0,0,255,255};
 float mc[]={depth_at(12,.05,1000),depth_at(1,.05,1000),depth_at(12,.05,1000)};
 float native[]={depth_at(10,.1,1000),depth_at(2,.1,1000),depth_at(10,.1,1000)};
 unsigned char red[]={255,0,0,255,255,0,0,255,255,0,0,255};
 GLuint tex[5];glGenTextures(5,tex);
 for(int i=0;i<5;i++){
  glActiveTexture(GL_TEXTURE0+i);glBindTexture(GL_TEXTURE_2D,tex[i]);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  glPixelStorei(GL_UNPACK_ALIGNMENT,1);
  if(i==1||i==3)glTexImage2D(GL_TEXTURE_2D,0,GL_LUMINANCE32F_ARB,3,1,0,GL_LUMINANCE,GL_FLOAT,i==1?mc:native);
  else glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,3,1,0,GL_RGBA,GL_UNSIGNED_BYTE,i==0?green:i==4?red:gui);
 }
 assert(glGetError()==GL_NO_ERROR);glUseProgram(p);
 const char *names[]={"worldColor","mcDepth","guiColor","hostDepth","hostColor"};for(int i=0;i<5;i++)glUniform1i(glGetUniformLocation(p,names[i]),i);
 glUniform4f(glGetUniformLocation(p,"clips"),.05,1000,.1,1000);glUniform1i(glGetUniformLocation(p,"debugDepth"),0);
 glViewport(0,0,3,1);glDisable(GL_DEPTH_TEST);glEnable(GL_BLEND);glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);glClearColor(1,0,0,1);glClear(GL_COLOR_BUFFER_BIT);
 glBegin(GL_QUADS);glTexCoord2f(0,0);glVertex2f(-1,-1);glTexCoord2f(1,0);glVertex2f(1,-1);glTexCoord2f(1,1);glVertex2f(1,1);glTexCoord2f(0,1);glVertex2f(-1,1);glEnd();
 unsigned char result[12];glReadPixels(0,0,3,1,GL_RGBA,GL_UNSIGNED_BYTE,result);assert(glGetError()==GL_NO_ERROR);
 assert(result[0]>250&&result[1]<5);assert(result[4]<5&&result[5]>250);assert(result[8]<5&&result[10]>250);
 assert(!wb_depth_visible(mc[0],native[0],.05,1000,.1,1000));assert(wb_depth_visible(mc[1],native[1],.05,1000,.1,1000));
 assert(!wb_depth_visible(NAN,native[0],.05,1000,.1,1000));

 glUniform1i(glGetUniformLocation(p,"lightCount"),1);
 glUniform3f(glGetUniformLocation(p,"eye"),0,0,0);glUniform3f(glGetUniformLocation(p,"forwardDir"),0,0,-1);
 glUniform3f(glGetUniformLocation(p,"rightDir"),1,0,0);glUniform3f(glGetUniformLocation(p,"upDir"),0,1,0);
 glUniform2f(glGetUniformLocation(p,"lens"),.1,3);
 glUniform4f(glGetUniformLocation(p,"lights[0]"),-2,0,-8,8);glUniform4f(glGetUniformLocation(p,"lightColors[0]"),0,1,0,1);
 glBegin(GL_QUADS);glTexCoord2f(0,0);glVertex2f(-1,-1);glTexCoord2f(1,0);glVertex2f(1,-1);glTexCoord2f(1,1);glVertex2f(1,1);glTexCoord2f(0,1);glVertex2f(-1,1);glEnd();
 glReadPixels(0,0,3,1,GL_RGBA,GL_UNSIGNED_BYTE,result);assert(glGetError()==GL_NO_ERROR);assert(result[1]>20);
 glUniform1i(glGetUniformLocation(p,"lightCount"),0);
 glBegin(GL_QUADS);glTexCoord2f(0,0);glVertex2f(-1,-1);glTexCoord2f(1,0);glVertex2f(1,-1);glTexCoord2f(1,1);glVertex2f(1,1);glTexCoord2f(0,1);glVertex2f(-1,1);glEnd();
 glReadPixels(0,0,3,1,GL_RGBA,GL_UNSIGNED_BYTE,result);assert(result[1]<5);
 puts("PASS: point light illuminates native-color surface and switching it off removes illumination.");
 // Reproduce Warband reattaching a reduced depth buffer to the same world FBO.
 GLuint worldDepth,postDepth;glGenRenderbuffers(1,&worldDepth);glBindRenderbuffer(GL_RENDERBUFFER,worldDepth);
 glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT24,3,1);
 glBindFramebuffer(GL_FRAMEBUFFER,fbo);glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,worldDepth);
 glClearDepth(.75);glDepthMask(GL_TRUE);glClear(GL_DEPTH_BUFFER_BIT);
 glGenRenderbuffers(1,&postDepth);glBindRenderbuffer(GL_RENDERBUFFER,postDepth);glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT24,1,1);
 glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,postDepth);
 scene_fbo=fbo;scene_depth_type=GL_RENDERBUFFER;scene_depth_name=worldDepth;win_w=3;win_h=1;
 host_depth=malloc(3*sizeof(float));assert(read_world_depth());
 for(int i=0;i<3;i++)assert(fabs(host_depth[i]-.75)<.00001);
 scene_depth_name=postDepth;assert(!read_world_depth());scene_depth_name=0;assert(!read_world_depth());
 GLuint multiDepth;glGenRenderbuffers(1,&multiDepth);glBindRenderbuffer(GL_RENDERBUFFER,multiDepth);
 glRenderbufferStorageMultisample(GL_RENDERBUFFER,4,GL_DEPTH_COMPONENT24,3,1);
 glBindFramebuffer(GL_FRAMEBUFFER,depth_read_fbo);glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,multiDepth);
 glClearDepth(.4);glClear(GL_DEPTH_BUFFER_BIT);scene_depth_name=multiDepth;
 assert(!read_world_depth());assert(!depth_valid&&depth_samples>=4);
 puts("PASS: unsupported multisampled depth is rejected rather than drawing corrupt pixels.");
 glBindFramebuffer(GL_FRAMEBUFFER,fbo);glDeleteFramebuffers(1,&depth_read_fbo);depth_read_fbo=0;free(host_depth);host_depth=NULL;
 puts("PASS: retained full-size world depth survives framebuffer reuse; reduced and missing attachments rejected.");
 // Exercise the real native mailbox queue, including stale and invalid snapshots.
 t.mem=calloc(1,WBMC_SHM_SIZE);t.epoch=7;mailbox=(uintptr_t)calloc(65536,sizeof(int64_t));stride=8;put(33,2);
 uint32_t *geometry=(void*)(t.mem+WB_GEOMETRY_OFFSET);geometry[0]=2;geometry[1]=1;geometry[2]=6;geometry[3]=1;
 float box[]={1,2,3,2,4,6};memcpy(geometry+16,box,sizeof(box));mirror_geometry();assert(get(30000)==0);
 geometry[2]=7;mirror_geometry();assert(get(30000)==1);assert(get(31001)==1000&&get(31002)==3000&&get(31003)==2000);
 assert(get(31004)==1000&&get(31005)==3000&&get(31006)==2000);put(30000,0);
 geometry[0]=4;geometry[3]=0;mirror_geometry();assert(get(30000)==1&&get(31003)==-10000000);put(30000,0);
 geometry[0]=6;geometry[3]=2049;mirror_geometry();assert(get(30000)==0);
 free((void*)mailbox);mailbox=0;free(t.mem);t.mem=NULL;
 puts("PASS: collider axis mapping, removed-collider retirement, stale epochs and oversized snapshots.");
 puts("PASS: native wall occludes farther block; nearer block visible; GUI stays visible; invalid depth rejected; float textures sample correctly.");
 CGLSetCurrentContext(NULL);CGLDestroyContext(context);return 0;
}
