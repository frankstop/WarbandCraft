package dev.wbmc.launcher;
public class Launcher {
    public static void main(String[] args) throws Exception {
        java.nio.file.Files.writeString(java.nio.file.Path.of(System.getProperty("warbandcraft.launchPid")),Long.toString(ProcessHandle.current().pid()));
        Class<?> library=Class.forName("com.sun.jna.NativeLibrary");
        Object libc=library.getMethod("getInstance",String.class).invoke(null,"c");
        Object chdir=library.getMethod("getFunction",String.class).invoke(libc,"chdir");
        chdir.getClass().getMethod("invokeInt",Object[].class).invoke(chdir,(Object)new Object[]{System.getProperty("user.dir")});
        System.setIn(java.nio.file.Files.newInputStream(java.nio.file.Path.of(System.getProperty("warbandcraft.launchInput"))));
        Class.forName("org.prismlauncher.EntryPoint").getMethod("main",String[].class).invoke(null,(Object)args);
    }
}
