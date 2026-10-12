package dev.wbmc.bridge.client.mixin;

import dev.wbmc.bridge.client.Overlay;
import net.minecraft.client.gui.screens.Screen;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/** Menu blur is a post-process over Minecraft's own frame only; skip it (Warband stays sharp behind the dimmed menu). */
@Mixin(Screen.class)
public abstract class ScreenMixin {
	@Inject(method = "renderBlurredBackground", at = @At("HEAD"), cancellable = true)
	private void warbandcraft$noBlur(float partialTick, CallbackInfo ci) {
		if (Overlay.active()) {
			ci.cancel();
		}
	}
}
