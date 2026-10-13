package dev.wbmc.bridge.client;

import com.mojang.blaze3d.pipeline.RenderTarget;
import org.lwjgl.opengl.*;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import org.slf4j.LoggerFactory;

/** A bridge-native Minecraft shader: transparent world coverage survives grading.
 * It does not replace depth, camera, hand or HUD rendering. */
public final class BridgeShader {
    private static int program, fbo, color, vao, width, height;
    private static boolean failed;
    public static boolean enabled=!Boolean.getBoolean("warbandcraft.disableShader");
    private BridgeShader() {}
    private static String source(String name) throws IOException {
        try(var stream=BridgeShader.class.getResourceAsStream("/assets/warbandcraft/shaders/bridge."+name)){
            if(stream==null)throw new IOException("Missing bridge shader "+name);
            return new String(stream.readAllBytes(),StandardCharsets.UTF_8);
        }
    }
    private static int compile(int type,String text) {
        int s=GL20.glCreateShader(type);GL20.glShaderSource(s,text);GL20.glCompileShader(s);
        if(GL20.glGetShaderi(s,GL20.GL_COMPILE_STATUS)==0){String error=GL20.glGetShaderInfoLog(s);GL20.glDeleteShader(s);throw new IllegalStateException(error);}return s;
    }
    /** Returns a color-only FBO; callers must continue reading depth from main. */
    public static int render(RenderTarget main) {
        if(!enabled||failed)return main.frameBufferId;
        int oldProgram=GL11.glGetInteger(GL20.GL_CURRENT_PROGRAM),oldVao=GL11.glGetInteger(GL30.GL_VERTEX_ARRAY_BINDING);
        int oldFbo=GL11.glGetInteger(GL30.GL_DRAW_FRAMEBUFFER_BINDING),oldActive=GL11.glGetInteger(GL13.GL_ACTIVE_TEXTURE);
        boolean blend=GL11.glIsEnabled(GL11.GL_BLEND),depth=GL11.glIsEnabled(GL11.GL_DEPTH_TEST),scissor=GL11.glIsEnabled(GL11.GL_SCISSOR_TEST);
        int[] viewport=new int[4];GL11.glGetIntegerv(GL11.GL_VIEWPORT,viewport);
        var masks=org.lwjgl.BufferUtils.createByteBuffer(4);GL11.glGetBooleanv(GL11.GL_COLOR_WRITEMASK,masks);
        GL13.glActiveTexture(GL13.GL_TEXTURE0);int oldTexture=GL11.glGetInteger(GL11.GL_TEXTURE_BINDING_2D);
        try {
            if(program==0){
                int vs=compile(GL20.GL_VERTEX_SHADER,source("vert")),fs=compile(GL20.GL_FRAGMENT_SHADER,source("frag"));
                program=GL20.glCreateProgram();GL20.glAttachShader(program,vs);GL20.glAttachShader(program,fs);GL20.glLinkProgram(program);
                GL20.glDeleteShader(vs);GL20.glDeleteShader(fs);
                if(GL20.glGetProgrami(program,GL20.GL_LINK_STATUS)==0)throw new IllegalStateException(GL20.glGetProgramInfoLog(program));
                fbo=GL30.glGenFramebuffers();color=GL11.glGenTextures();vao=GL30.glGenVertexArrays();
            }
            GL11.glBindTexture(GL11.GL_TEXTURE_2D,color);
            if(width!=main.width||height!=main.height){
                width=main.width;height=main.height;
                GL11.glTexImage2D(GL11.GL_TEXTURE_2D,0,GL11.GL_RGBA8,width,height,0,GL11.GL_RGBA,GL11.GL_UNSIGNED_BYTE,(java.nio.ByteBuffer)null);
                GL11.glTexParameteri(GL11.GL_TEXTURE_2D,GL11.GL_TEXTURE_MIN_FILTER,GL11.GL_NEAREST);GL11.glTexParameteri(GL11.GL_TEXTURE_2D,GL11.GL_TEXTURE_MAG_FILTER,GL11.GL_NEAREST);
            }
            GL30.glBindFramebuffer(GL30.GL_DRAW_FRAMEBUFFER,fbo);
            GL30.glFramebufferTexture2D(GL30.GL_DRAW_FRAMEBUFFER,GL30.GL_COLOR_ATTACHMENT0,GL11.GL_TEXTURE_2D,color,0);
            if(GL30.glCheckFramebufferStatus(GL30.GL_DRAW_FRAMEBUFFER)!=GL30.GL_FRAMEBUFFER_COMPLETE)throw new IllegalStateException("Incomplete shader framebuffer");
            GL11.glBindTexture(GL11.GL_TEXTURE_2D,main.getColorTextureId());GL20.glUseProgram(program);GL30.glBindVertexArray(vao);
            GL20.glUniform1i(GL20.glGetUniformLocation(program,"world"),0);GL20.glUniform1f(GL20.glGetUniformLocation(program,"strength"),1);
            GL11.glDisable(GL11.GL_BLEND);GL11.glDisable(GL11.GL_DEPTH_TEST);GL11.glDisable(GL11.GL_SCISSOR_TEST);
            GL11.glViewport(0,0,width,height);GL11.glColorMask(true,true,true,true);
            GL11.glDrawArrays(GL11.GL_TRIANGLES,0,3);
            return fbo;
        } catch(IOException|RuntimeException e){
            failed=true;LoggerFactory.getLogger("warbandcraft").warn("Bridge shader unavailable; using vanilla color",e);return main.frameBufferId;
        } finally {
            GL20.glUseProgram(oldProgram);GL30.glBindVertexArray(oldVao);GL30.glBindFramebuffer(GL30.GL_DRAW_FRAMEBUFFER,oldFbo);
            GL11.glBindTexture(GL11.GL_TEXTURE_2D,oldTexture);GL13.glActiveTexture(oldActive);
            GL11.glViewport(viewport[0],viewport[1],viewport[2],viewport[3]);GL11.glColorMask(masks.get(0)!=0,masks.get(1)!=0,masks.get(2)!=0,masks.get(3)!=0);
            if(blend)GL11.glEnable(GL11.GL_BLEND);if(depth)GL11.glEnable(GL11.GL_DEPTH_TEST);if(scissor)GL11.glEnable(GL11.GL_SCISSOR_TEST);
        }
    }
}
