package dev.wbmc.bridge;

import dev.wbmc.bridge.link.WarbandLink;
import net.fabricmc.fabric.api.event.lifecycle.v1.ServerChunkEvents;
import net.minecraft.core.BlockPos;
import net.minecraft.server.MinecraftServer;
import net.minecraft.server.level.ServerLevel;
import net.minecraft.world.level.ChunkPos;
import net.minecraft.world.level.block.state.BlockState;
import net.minecraft.world.level.chunk.LevelChunk;
import org.slf4j.LoggerFactory;
import java.util.*;

/** Native colliders and light sources derived from actual server block states.
 * Dirty chunks include server-driven changes: explosions, fluids and pistons. */
public final class WorldMirror {
    private static final Map<Long,LevelChunk> chunks=new HashMap<>();
    private static final Set<Long> dirty=new LinkedHashSet<>();
    private static final Map<Long,Map<BlockPos,BlockState>> cells=new HashMap<>();
    private static long ticks; private static int epoch=-1; private static boolean publishNeeded=true;
    private WorldMirror() {}
    public static void init() {
        ServerChunkEvents.CHUNK_LOAD.register((level,chunk)->{
            if(level.dimension()!=net.minecraft.world.level.Level.OVERWORLD)return;
            long key=chunk.getPos().toLong();chunks.put(key,chunk);dirty.add(key);
        });
        ServerChunkEvents.CHUNK_UNLOAD.register((level,chunk)->{
            if(level.dimension()!=net.minecraft.world.level.Level.OVERWORLD)return;
            long key=chunk.getPos().toLong();chunks.remove(key);cells.remove(key);dirty.remove(key);publishNeeded=true;
        });
    }
    public static void changed(LevelChunk chunk) {
        if(chunk.getLevel() instanceof ServerLevel && TerrainManager.isBridgeWorld()) {
            long key=chunk.getPos().toLong();chunks.put(key,chunk);dirty.add(key);
        }
    }
    public static void reset(){chunks.clear();dirty.clear();cells.clear();epoch=-1;publishNeeded=true;ticks=0;}
    public static void tick(MinecraftServer server) {
        if(!TerrainManager.isBridgeWorld())return;
        var context=WarbandLink.get().context();var map=CoordMap.get();
        if(context==null||map==null||map.provisional())return;
        if(epoch!=context.epoch()){epoch=context.epoch();publishNeeded=true;}
        // Process one dirty chunk per tick rather than rescanning the world per frame.
        if(!dirty.isEmpty()) {
            long key=dirty.iterator().next();dirty.remove(key);var chunk=chunks.get(key);
            if(chunk!=null){cells.put(key,scan(chunk));publishNeeded=true;}
        }
        if(++ticks%10!=0)return;
        List<float[]> boxes=new ArrayList<>(),lights=new ArrayList<>();
        var level=server.overworld();
        for(var chunk:cells.values())for(var entry:chunk.entrySet()) {
            var pos=entry.getKey();var state=entry.getValue();
            if(!map.contains(pos.getX()+.5))continue;
            for(var box:state.getCollisionShape(level,pos).toAabbs()) {
                double[] lo=map.toWarband(pos.getX()+box.minX,pos.getY()+box.minY,pos.getZ()+box.minZ);
                double[] hi=map.toWarband(pos.getX()+box.maxX,pos.getY()+box.maxY,pos.getZ()+box.maxZ);
                float[] b=new float[6];for(int k=0;k<3;k++){b[k]=(float)Math.min(lo[k],hi[k]);b[k+3]=(float)Math.max(lo[k],hi[k]);}
                if(b[3]>b[0]&&b[4]>b[1]&&b[5]>b[2])boxes.add(b);
            }
            if(state.getLightEmission()>0) {
                double[] p=map.toWarband(pos.getX()+.5,pos.getY()+.5,pos.getZ()+.5);
                String name=net.minecraft.core.registries.BuiltInRegistries.BLOCK.getKey(state.getBlock()).getPath();
                boolean soul=name.contains("soul_");
                lights.add(new float[]{(float)p[0],(float)p[1],(float)p[2],soul?.25f:1,soul?.7f:.65f,soul?1:.32f,state.getLightEmission(),0});
            }
        }
        boxes=BoxMerge.merge(boxes);
        if(boxes.size()>2048) {
            LoggerFactory.getLogger("warbandcraft").warn("Construction exceeds 2048 merged colliders ({}); retaining last complete snapshot",boxes.size());
            return;
        }
        if(publishNeeded && WarbandLink.get().publishGeometry(epoch,boxes))publishNeeded=false;
        if(!server.getPlayerList().getPlayers().isEmpty()){
            var p=server.getPlayerList().getPlayers().getFirst();double[] at=map.toWarband(p.getX(),p.getY(),p.getZ());
            lights.sort(Comparator.comparingDouble(l->Math.pow(l[0]-at[0],2)+Math.pow(l[1]-at[1],2)+Math.pow(l[2]-at[2],2)));
        }
        WarbandLink.get().publishLights(epoch,lights.subList(0,Math.min(32,lights.size())));
    }
    private static Map<BlockPos,BlockState> scan(LevelChunk chunk) {
        Map<BlockPos,BlockState> result=new HashMap<>();int sectionIndex=0;
        for(var section:chunk.getSections()) {
            int y0=chunk.getMinBuildHeight()+sectionIndex++*16;
            if(section.hasOnlyAir())continue;
            for(int y=0;y<16;y++)for(int z=0;z<16;z++)for(int x=0;x<16;x++){
                var state=section.getBlockState(x,y,z);
                if(!state.isAir()&&!state.is(WarbandBridgeMod.TERRAIN))result.put(new BlockPos(chunk.getPos().getMinBlockX()+x,y0+y,chunk.getPos().getMinBlockZ()+z),state);
            }
        }
        return result;
    }
}
