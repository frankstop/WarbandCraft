#define GL_SILENCE_DEPRECATION
#include <glew.h>
#include <OpenGL/OpenGL.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
static char *read_shader(const char *ext){
 char path[512];snprintf(path,sizeof(path),"source/minecraft/src/client/resources/assets/warbandcraft/shaders/bridge.%s",ext);
 FILE *f=fopen(path,"rb");assert(f);fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);char *s=calloc(n+1,1);assert(fread(s,1,n,f)==n);fclose(f);return s;
}
static GLuint compile(GLenum kind,const char *source){
 GLuint s=glCreateShader(kind);glShaderSource(s,1,&source,NULL);glCompileShader(s);GLint ok;glGetShaderiv(s,GL_COMPILE_STATUS,&ok);
 if(!ok){char b[4096];glGetShaderInfoLog(s,sizeof(b),NULL,b);puts(b);}assert(ok);return s;
}
int main(void){
 CGLPixelFormatAttribute attrs[]={kCGLPFAAccelerated,kCGLPFAOpenGLProfile,(CGLPixelFormatAttribute)kCGLOGLPVersion_3_2_Core,(CGLPixelFormatAttribute)0};
 CGLPixelFormatObj format;GLint count;CGLContextObj context;
 assert(CGLChoosePixelFormat(attrs,&format,&count)==kCGLNoError);assert(CGLCreateContext(format,NULL,&context)==kCGLNoError);
 CGLDestroyPixelFormat(format);CGLSetCurrentContext(context);glewExperimental=GL_TRUE;glewInit();while(glGetError()!=GL_NO_ERROR){}
 printf("Shader GPU: %s; OpenGL %s\n",glGetString(GL_RENDERER),glGetString(GL_VERSION));
 char *vs=read_shader("vert"),*fs=read_shader("frag");GLuint a=compile(GL_VERTEX_SHADER,vs),b=compile(GL_FRAGMENT_SHADER,fs),p=glCreateProgram();
 glAttachShader(p,a);glAttachShader(p,b);glLinkProgram(p);GLint ok;glGetProgramiv(p,GL_LINK_STATUS,&ok);assert(ok);
 GLuint textures[2],fbo,vao;glGenTextures(2,textures);glGenFramebuffers(1,&fbo);glGenVertexArrays(1,&vao);glBindVertexArray(vao);
 unsigned char input[]={64,32,16,128,0,0,0,0};glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,textures[0]);
 glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,2,1,0,GL_RGBA,GL_UNSIGNED_BYTE,input);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
 glBindTexture(GL_TEXTURE_2D,textures[1]);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,2,1,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);
 glBindFramebuffer(GL_FRAMEBUFFER,fbo);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,textures[1],0);assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);
 glBindTexture(GL_TEXTURE_2D,textures[0]);glUseProgram(p);glUniform1i(glGetUniformLocation(p,"world"),0);glUniform1f(glGetUniformLocation(p,"strength"),1);
 glViewport(0,0,2,1);glDisable(GL_BLEND);glDisable(GL_DEPTH_TEST);glDrawArrays(GL_TRIANGLES,0,3);
 unsigned char result[8];glReadPixels(0,0,2,1,GL_RGBA,GL_UNSIGNED_BYTE,result);assert(glGetError()==GL_NO_ERROR);
 assert(result[3]==128&&result[7]==0&&result[4]==0&&result[5]==0&&result[6]==0);
 assert(result[0]!=input[0]||result[1]!=input[1]||result[2]!=input[2]);
 puts("PASS: Minecraft bridge shader grades world color while preserving translucent alpha and transparent background.");
 free(vs);free(fs);CGLSetCurrentContext(NULL);CGLDestroyContext(context);return 0;
}
