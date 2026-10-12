package dev.wbmc.bridge.client;
import dev.wbmc.bridge.link.WarbandLink;
import net.minecraft.client.Minecraft;
import net.minecraft.network.chat.Component;
import org.lwjgl.glfw.GLFW;
/** Alt + 1/2/3/0 selects a division; Alt + F1..F5 orders it. */
public final class ArmyCommands {
    private static int group = 9;
    private ArmyCommands() {}
    public static boolean capture(int key, int action, int mods) {
        Minecraft mc = Minecraft.getInstance();
        if (!Overlay.active() || mc.player == null || mc.screen != null || (mods & GLFW.GLFW_MOD_ALT) == 0) return false;
        int next = switch (key) { case GLFW.GLFW_KEY_1 -> 0; case GLFW.GLFW_KEY_2 -> 1; case GLFW.GLFW_KEY_3 -> 2; case GLFW.GLFW_KEY_0 -> 9; default -> -1; };
        int order = switch (key) { case GLFW.GLFW_KEY_F1 -> 0; case GLFW.GLFW_KEY_F2 -> 1; case GLFW.GLFW_KEY_F3 -> 2; case GLFW.GLFW_KEY_F4 -> 12; case GLFW.GLFW_KEY_F5 -> 5; default -> -1; };
        if (next < 0 && order < 0) return false;
        if (action == GLFW.GLFW_PRESS) {
            if (next >= 0) { group = next; mc.player.displayClientMessage(Component.literal("Warband division: " + switch (group) { case 0 -> "infantry"; case 1 -> "archers"; case 2 -> "cavalry"; default -> "everyone"; }), true); }
            else if (!WarbandLink.get().pushOrder(group, order)) mc.player.displayClientMessage(Component.literal("Army order unavailable: bridge disconnected or queue full"), true);
        }
        return true;
    }
}
