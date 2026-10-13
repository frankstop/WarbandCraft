package dev.wbmc.bridge.mixin;
import dev.wbmc.bridge.WorldMirror;
import net.minecraft.core.BlockPos;
import net.minecraft.world.level.block.state.BlockState;
import net.minecraft.world.level.chunk.LevelChunk;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

@Mixin(LevelChunk.class)
public abstract class ChunkChangeMixin {
    @Inject(method="setBlockState",at=@At("RETURN"))
    private void warbandcraft$changed(BlockPos pos,BlockState state,boolean moving,CallbackInfoReturnable<BlockState> ci){
        if(ci.getReturnValue()!=null && !state.is(dev.wbmc.bridge.WarbandBridgeMod.TERRAIN) && !(ci.getReturnValue().is(dev.wbmc.bridge.WarbandBridgeMod.TERRAIN) && state.isAir()))WorldMirror.changed((LevelChunk)(Object)this);
    }
}
