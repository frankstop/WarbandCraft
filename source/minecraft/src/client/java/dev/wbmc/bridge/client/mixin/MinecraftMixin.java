package dev.wbmc.bridge.client.mixin;

import dev.wbmc.bridge.client.FramePassthrough;
import dev.wbmc.bridge.client.Overlay;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;
import net.minecraft.client.Minecraft;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

@Mixin(Minecraft.class)
public abstract class MinecraftMixin {
	@Inject(method = "runTick", at = @At("HEAD"))
	private void warbandcraft$frame(boolean tick, CallbackInfo ci) {
		Overlay.onFrame((Minecraft) (Object) this);
	}

	/** While frames go to Warband, rendering faster than Warband (~60 fps) only wastes readbacks. */
	@Inject(method = "getFramerateLimit", at = @At("RETURN"), cancellable = true)
	private void warbandcraft$capFps(CallbackInfoReturnable<Integer> cir) {
		if (FramePassthrough.wanted() && cir.getReturnValue() > 60) {
			cir.setReturnValue(60);
		}
	}
}
