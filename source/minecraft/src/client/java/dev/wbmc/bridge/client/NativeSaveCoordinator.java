package dev.wbmc.bridge.client;

import dev.wbmc.bridge.CheckpointStore;
import dev.wbmc.bridge.link.WarbandLink;
import net.minecraft.client.Minecraft;
import net.minecraft.client.gui.screens.TitleScreen;
import net.minecraft.world.level.storage.LevelResource;
import com.google.gson.JsonParser;
import java.nio.file.*;
import org.slf4j.LoggerFactory;

/** Two-phase save handshake with the native loader. Native save I/O is paused
 * while the integrated server flushes and snapshots its matching world. */
public final class NativeSaveCoordinator {
    private static final Path REQUEST=Path.of("/tmp/warbandcraft/save-request.json");
    private static String handled;private static volatile Path pending;private static volatile boolean busy;
    private NativeSaveCoordinator() {}
    public static boolean busy(){return busy;}
    public static void tick(Minecraft mc){
        if(!Files.exists(REQUEST)||busy)return;
        try {
            var request=JsonParser.parseString(Files.readString(REQUEST)).getAsJsonObject();
            String id=request.get("id").getAsString();if(id.equals(handled))return;handled=id;
            String action=request.get("action").getAsString();Path nativeSave=Path.of(request.get("path").getAsString()).toAbsolutePath().normalize();
            Path allowed=Path.of(System.getProperty("user.home"),"Library/Application Support/MBWarband/Savegames", System.getProperty("warbandcraft.module","WarbandCraft")).toAbsolutePath().normalize();
            if(!nativeSave.startsWith(allowed)||!nativeSave.getFileName().toString().endsWith(".sav"))throw new IllegalStateException("Save path outside WarbandCraft");
            var store=new CheckpointStore(mc.gameDirectory.toPath().resolve("warbandcraft-checkpoints"));
            if(action.equals("prepare-save")){
                var context=WarbandLink.get().context();var server=mc.getSingleplayerServer();
                if(context==null||server==null)throw new IllegalStateException("No matching integrated world to checkpoint");
                busy=true;
                server.execute(()->{
                    try {
                        server.saveEverything(false,true,true);
                        pending=store.stageWorld(server.getWorldPath(LevelResource.ROOT),context.campaign());
                        respond(id,"ok");
                    }catch(Exception e){failure(id,e);}finally{busy=false;}
                });
            }else if(action.equals("commit-save")){
                if(pending==null)throw new IllegalStateException("Missing prepared world snapshot");
                store.commit(pending,nativeSave);pending=null;respond(id,"ok");
            }else if(action.equals("prepare-load")){
                // Close the integrated server before replacing its files. No active-world copies.
                var previous=WarbandLink.get().context();
                int previousEpoch=previous==null?-1:previous.epoch();
                busy=true;
                if(mc.level!=null)mc.level.disconnect();
                mc.disconnect();mc.setScreen(new TitleScreen());
                String campaign=store.restore(nativeSave,mc.gameDirectory.toPath().resolve("saves"));
                WorldBootstrap.reopen(previousEpoch);respond(id,campaign==null?"unpaired":"ok");busy=false;
            }
        }catch(Exception e){failure(handled,e);busy=false;}
    }
    private static void respond(String id,String status)throws java.io.IOException{
        Path tmp=Path.of("/tmp/warbandcraft/save-response.next"),out=Path.of("/tmp/warbandcraft/save-response.txt");
        Files.writeString(tmp,id+" "+status);Files.move(tmp,out,StandardCopyOption.ATOMIC_MOVE,StandardCopyOption.REPLACE_EXISTING);
    }
    private static void failure(String id,Exception error){
        LoggerFactory.getLogger("warbandcraft").error("Paired checkpoint failed; native save/load must not be treated as coordinated",error);
        try{respond(id,"error");}catch(Exception ignored){}
    }
}
