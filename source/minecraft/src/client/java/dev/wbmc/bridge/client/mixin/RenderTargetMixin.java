package dev.wbmc.bridge.client.mixin;

import com.mojang.blaze3d.pipeline.RenderTarget;
import com.mojang.blaze3d.platform.GlStateManager;
import dev.wbmc.bridge.client.FramePassthrough;
import dev.wbmc.bridge.client.Overlay;
import net.minecraft.client.Minecraft;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.Redirect;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/**
 * The final blit to the window normally leaves the window's alpha untouched. With a
 * transparent window that alpha decides what shows through, so: clear it first (to 0 in
 * overlay mode, 1 otherwise) and, in overlay mode, copy Minecraft's alpha along with color.
 *
 * <p>Only the main render target's blit targets the window. The same method also
 * composites other targets (e.g. entity outlines, every frame) onto the main target, and
 * those must be left alone or they would wipe the rendered world.
 */
@Mixin(RenderTarget.class)
public abstract class RenderTargetMixin {
	private static final int GL_COLOR_BUFFER_BIT = 0x4000;

	private boolean warbandcraft$isWindowBlit() {
		return (Object) this == Minecraft.getInstance().getMainRenderTarget();
	}

	@Inject(method = "_blitToScreen", at = @At("HEAD"), cancellable = true)
	private void warbandcraft$clearWindow(int width, int height, boolean disableBlend, CallbackInfo ci) {
		if (!warbandcraft$isWindowBlit()) {
			return;
		}
		FramePassthrough.endFrame(Minecraft.getInstance());
		if (!Overlay.transparentWindow()) {
			return;
		}
		GlStateManager._glBindFramebuffer(org.lwjgl.opengl.GL30.GL_FRAMEBUFFER, 0);
		GlStateManager._colorMask(true, true, true, true);
		GlStateManager._clearColor(0.0F, 0.0F, 0.0F, Overlay.active() ? 0.0F : 1.0F);
		GlStateManager._clear(GL_COLOR_BUFFER_BIT, Minecraft.ON_OSX);
		if (FramePassthrough.activeInWarband()) {
			// Warband is showing this frame inside its own; our window only keeps the input focus.
			ci.cancel();
		}
	}

	@Redirect(method = "_blitToScreen", at = @At(value = "INVOKE",
		target = "Lcom/mojang/blaze3d/platform/GlStateManager;_colorMask(ZZZZ)V", remap = false, ordinal = 0))
	private void warbandcraft$writeAlpha(boolean r, boolean g, boolean b, boolean a) {
		GlStateManager._colorMask(r, g, b, a || (Overlay.active() && warbandcraft$isWindowBlit()));
	}
}
