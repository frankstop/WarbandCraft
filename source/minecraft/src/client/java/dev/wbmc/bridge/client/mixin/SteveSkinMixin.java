package dev.wbmc.bridge.client.mixin;

import dev.wbmc.bridge.client.Overlay;
import net.minecraft.client.Minecraft;
import net.minecraft.client.player.AbstractClientPlayer;
import net.minecraft.client.resources.PlayerSkin;
import net.minecraft.resources.ResourceLocation;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

@Mixin(AbstractClientPlayer.class)
public abstract class SteveSkinMixin {
    private static final PlayerSkin STEVE = new PlayerSkin(
        ResourceLocation.withDefaultNamespace("textures/entity/player/wide/steve.png"),
        null, null, null, PlayerSkin.Model.WIDE, false);

    @Inject(method = "getSkin", at = @At("HEAD"), cancellable = true)
    private void warbandcraft$steve(CallbackInfoReturnable<PlayerSkin> ci) {
        if ((Object) this == Minecraft.getInstance().player && Overlay.active()) ci.setReturnValue(STEVE);
    }
}
