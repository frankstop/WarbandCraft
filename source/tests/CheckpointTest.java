import dev.wbmc.bridge.CheckpointStore;
import dev.wbmc.bridge.BoxMerge;
import java.nio.file.*;
import java.util.*;

public class CheckpointTest {
    public static void main(String[] args)throws Exception{
        Path root=Files.createTempDirectory("warbandcraft-checkpoint-test"),world=root.resolve("world"),nativeSave=root.resolve("sg00.sav"),saves=root.resolve("saves");
        Files.createDirectories(world.resolve("region"));Files.writeString(world.resolve("level.dat"),"inventory-before");Files.writeString(world.resolve("region/r.0.0.mca"),"built-wall");Files.writeString(world.resolve("session.lock"),"locked");
        Files.writeString(nativeSave,"campaign-native-checkpoint-1");String campaign="0123456789abcdef0123456789abcdef";
        var store=new CheckpointStore(root.resolve("checkpoints"));Path staged=store.stageWorld(world,campaign);assert !Files.exists(staged.resolve("world/session.lock"));
        Path checkpoint=store.commit(staged,nativeSave);assert store.restore(nativeSave,saves).equals(campaign);
        Path restored=saves.resolve("warbandcraft-"+campaign);assert Files.readString(restored.resolve("level.dat")).equals("inventory-before");
        Files.writeString(restored.resolve("level.dat"),"inventory-future");store.restore(nativeSave,saves);assert Files.readString(restored.resolve("level.dat")).equals("inventory-before");
        try(var dirs=Files.list(saves)){assert dirs.anyMatch(p->p.getFileName().toString().startsWith("rollback-"));}
        boolean refused=false;try{store.commit(store.stageWorld(world,campaign),nativeSave);}catch(java.io.IOException e){refused=true;}assert refused;
        Files.writeString(checkpoint.resolve("world/level.dat"),"corrupt");refused=false;try{store.restore(nativeSave,saves);}catch(java.io.IOException e){refused=true;}assert refused;
        Files.writeString(nativeSave,"unknown-native-save");assert store.restore(nativeSave,saves)==null;
        var joined=BoxMerge.merge(List.of(new float[]{0,0,0,1,1,1},new float[]{1,0,0,2,1,1}));assert joined.size()==1 && joined.getFirst()[3]==2;
        var gap=BoxMerge.merge(List.of(new float[]{0,0,0,1,1,1},new float[]{2,0,0,3,1,1}));assert gap.size()==2;
        var slab=BoxMerge.merge(List.of(new float[]{0,0,0,1,.5f,1},new float[]{1,0,0,2,1,1}));assert slab.size()==2;
        System.out.println("PASS: checkpoint rollback, inventory/build recovery, displaced-world backups, immutable snapshots, corruption rejection, unknown saves, collider merging preserves gaps and slabs.");
        System.out.println("Fixture retained at "+root);
    }
}
