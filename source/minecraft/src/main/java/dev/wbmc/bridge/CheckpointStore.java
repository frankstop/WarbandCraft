package dev.wbmc.bridge;

import java.io.*;
import java.nio.file.*;
import java.security.*;
import java.util.*;

/** Immutable paired snapshots. Restoration preserves the displaced world as a backup. */
public final class CheckpointStore {
    private final Path root;
    public CheckpointStore(Path root){this.root=root;}
    public static String digest(Path file) throws IOException {
        try {
            var hash=MessageDigest.getInstance("SHA-256");
            try(var in=Files.newInputStream(file)){byte[] b=new byte[65536];int n;while((n=in.read(b))>0)hash.update(b,0,n);}
            return HexFormat.of().formatHex(hash.digest());
        }catch(NoSuchAlgorithmException e){throw new AssertionError(e);}
    }
    public static String digestTree(Path directory) throws IOException {
        try {
            var hash=MessageDigest.getInstance("SHA-256");
            try(var files=Files.walk(directory)){
                for(Path file:files.filter(Files::isRegularFile).sorted().toList()) {
                    if(file.getFileName().toString().equals("session.lock"))continue;
                    hash.update(directory.relativize(file).toString().getBytes(java.nio.charset.StandardCharsets.UTF_8));hash.update((byte)0);
                    hash.update(digest(file).getBytes(java.nio.charset.StandardCharsets.US_ASCII));
                }
            }
            return HexFormat.of().formatHex(hash.digest());
        }catch(NoSuchAlgorithmException e){throw new AssertionError(e);}
    }
    public Path stageWorld(Path world,String campaign) throws IOException {
        if(!campaign.matches("[0-9a-f]{32}"))throw new IOException("Invalid campaign identity");
        Files.createDirectories(root);Path staging=root.resolve("pending-"+UUID.randomUUID());
        copyTree(world,staging.resolve("world"));
        Properties meta=new Properties();meta.setProperty("campaign",campaign);meta.setProperty("format","1");
        writeProperties(staging.resolve("manifest.properties"),meta);return staging;
    }
    public Path commit(Path staging,Path nativeSave) throws IOException {
        String hash=digest(nativeSave);Path destination=root.resolve(hash);
        if(Files.exists(destination))throw new IOException("Native save content already has an immutable checkpoint; refusing ambiguous overwrite");
        Files.copy(nativeSave,staging.resolve("native.sav"));
        if(!digest(staging.resolve("native.sav")).equals(hash)||!digest(nativeSave).equals(hash))throw new IOException("Native save changed during checkpoint");
        Properties meta=readProperties(staging.resolve("manifest.properties"));meta.setProperty("native_sha256",hash);
        meta.setProperty("native_name",nativeSave.getFileName().toString());meta.setProperty("world_sha256",digestTree(staging.resolve("world")));writeProperties(staging.resolve("manifest.properties"),meta);
        Files.move(staging,destination,StandardCopyOption.ATOMIC_MOVE);return destination;
    }
    public String restore(Path nativeSave,Path saves) throws IOException {
        String hash=digest(nativeSave);Path checkpoint=root.resolve(hash);
        if(!Files.isDirectory(checkpoint))return null;
        Properties meta=readProperties(checkpoint.resolve("manifest.properties"));String campaign=meta.getProperty("campaign","");
        if(!campaign.matches("[0-9a-f]{32}")||!hash.equals(meta.getProperty("native_sha256"))||!hash.equals(digest(checkpoint.resolve("native.sav"))))throw new IOException("Invalid checkpoint manifest/native snapshot");
        if(!digestTree(checkpoint.resolve("world")).equals(meta.getProperty("world_sha256")))throw new IOException("World snapshot checksum mismatch");
        Files.createDirectories(saves);Path world=saves.resolve("warbandcraft-"+campaign),staging=saves.resolve("restore-"+UUID.randomUUID());
        copyTree(checkpoint.resolve("world"),staging);
        Path backup=saves.resolve("rollback-"+UUID.randomUUID());
        boolean displaced=Files.exists(world);
        if(displaced)Files.move(world,backup,StandardCopyOption.ATOMIC_MOVE);
        try{Files.move(staging,world,StandardCopyOption.ATOMIC_MOVE);}
        catch(IOException e){if(displaced)Files.move(backup,world,StandardCopyOption.ATOMIC_MOVE);throw e;}
        return campaign;
    }
    public static void copyTree(Path source,Path dest) throws IOException {
        if(!Files.isDirectory(source))throw new IOException("Missing snapshot source "+source);
        try(var files=Files.walk(source)){
            for(Path from:files.toList()){
                if(Files.isSymbolicLink(from))throw new IOException("Symlinks are not checkpoint inputs");
                if(from.getFileName().toString().equals("session.lock"))continue;
                Path to=dest.resolve(source.relativize(from));
                if(Files.isDirectory(from))Files.createDirectories(to);else Files.copy(from,to,StandardCopyOption.COPY_ATTRIBUTES);
            }
        }
    }
    private static Properties readProperties(Path file)throws IOException {
        Properties p=new Properties();try(var in=Files.newInputStream(file)){p.load(in);}return p;
    }
    private static void writeProperties(Path file,Properties p)throws IOException {
        try(var out=Files.newOutputStream(file)){p.store(out,"WarbandCraft paired checkpoint");}
    }
}
