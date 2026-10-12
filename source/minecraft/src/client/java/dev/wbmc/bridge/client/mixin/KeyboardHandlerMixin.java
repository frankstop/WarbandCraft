package dev.wbmc.bridge.client.mixin;
import dev.wbmc.bridge.client.ArmyCommands;
import net.minecraft.client.KeyboardHandler;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
@Mixin(KeyboardHandler.class)
public abstract class KeyboardHandlerMixin {
    @Inject(method = "keyPress", at = @At("HEAD"), cancellable = true)
    private void warbandcraft$army(long window, int key, int scan, int action, int mods, CallbackInfo ci) {
        if (ArmyCommands.capture(key, action, mods)) ci.cancel();
    }
}
