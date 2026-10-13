package dev.wbmc.bridge.client;

import com.mojang.blaze3d.platform.InputConstants;
import dev.wbmc.bridge.CoordMap;
import dev.wbmc.bridge.link.GameState;
import dev.wbmc.bridge.link.WarbandLink;
import dev.wbmc.bridge.link.Protocol;
import net.fabricmc.api.ClientModInitializer;
import net.fabricmc.fabric.api.client.event.lifecycle.v1.ClientTickEvents;
import net.fabricmc.fabric.api.client.keybinding.v1.KeyBindingHelper;
import net.fabricmc.fabric.api.client.rendering.v1.EntityRendererRegistry;
import net.fabricmc.fabric.api.client.rendering.v1.HudRenderCallback;
import dev.wbmc.bridge.entity.WarbandBridgeEntities;
import net.minecraft.client.KeyMapping;
import net.minecraft.client.Minecraft;
import net.minecraft.client.gui.GuiGraphics;
import net.minecraft.network.chat.Component;
import org.lwjgl.glfw.GLFW;

public class WarbandBridgeClient implements ClientModInitializer {
	private static boolean heldForDeath;
	private static final GameState HUD_STATE = new GameState();
	private static KeyMapping cameraModeKey;
	private static KeyMapping overlayKey;
	private static KeyMapping hudKey;
	private static boolean showStatus = false;

	@Override
	public void onInitializeClient() {
		cameraModeKey = KeyBindingHelper.registerKeyBinding(new KeyMapping("key.warbandcraft.camera_mode",
			InputConstants.Type.KEYSYM, GLFW.GLFW_KEY_F7, "category.warbandcraft"));
		overlayKey = KeyBindingHelper.registerKeyBinding(new KeyMapping("key.warbandcraft.overlay",
			InputConstants.Type.KEYSYM, GLFW.GLFW_KEY_F8, "category.warbandcraft"));
		hudKey = KeyBindingHelper.registerKeyBinding(new KeyMapping("key.warbandcraft.status",
			InputConstants.Type.KEYSYM, GLFW.GLFW_KEY_F9, "category.warbandcraft"));

		ClientTickEvents.END_CLIENT_TICK.register(mc -> {
			// Steve died: keep Warband's camera and avatar where he fell until the server has put the
			// respawned player back at the avatar (LifeBridge), not at the world's spawn point.
			boolean dead = mc.player != null && mc.player.isDeadOrDying();
			if (dead && !heldForDeath) {
				CameraSync.holdForRecall();
				Overlay.switchToWarband(mc);
			}
			heldForDeath = dead;
			while (cameraModeKey.consumeClick()) {
				CameraSync.toggleMode();
				toast(mc, "Camera: " + CameraSync.mode());
			}
			while (overlayKey.consumeClick()) {
				// Hand control to Warband (menus, quest board, travelling...). F8 in Warband comes back.
				Overlay.switchToWarband(mc);
			}
			while (hudKey.consumeClick()) {
				// Debug view: bridge status line, hitbox outlines, every HP tag.
				showStatus = !showStatus;
				WarbandEntityRenderer.showHitboxes = showStatus;
				toast(mc, "Debug view " + (showStatus ? "on" : "off"));
			}


			NativeSaveCoordinator.tick(mc);
            WorldBootstrap.tick(mc);
			DevCommands.tick(mc);
		});

		HudRenderCallback.EVENT.register((graphics, tickCounter) -> renderStatus(graphics));
		EntityRendererRegistry.register(WarbandBridgeEntities.Warband_ENTITY, WarbandEntityRenderer::new);
	}

	private static void toast(Minecraft mc, String msg) {
		if (mc.player != null) {
			mc.player.displayClientMessage(Component.literal("[Warband Bridge] " + msg), true);
		}
	}

	private static void renderStatus(GuiGraphics g) {
		Minecraft mc = Minecraft.getInstance();
		if (!showStatus || mc.options.hideGui || mc.getDebugOverlay().showDebugScreen()) {
			return;
		}
		WarbandLink link = WarbandLink.get();
		String line1;
		String line2 = null;
		if (!link.alive()) {
			line1 = "Warband: not connected";
		} else {
			boolean have = link.snapshot(HUD_STATE);
			line1 = String.format("Warband: live  frame %d  %s  camera %s%s", have ? HUD_STATE.frame : 0,
				!Overlay.active() ? "overlay off" : FramePassthrough.activeInWarband() ? "drawn inside Warband" : FramePassthrough.suppressWindowWorld() ? "waiting for native rendering" : "overlay window", CameraSync.mode(),
				CameraSync.mode() == CameraSync.Mode.DRIVE_Warband && !CameraSync.warbandFollowing() ? " (Warband not following yet)" : "");
			if (have && HUD_STATE.has(Protocol.STATE_PLAYER_VALID)) {
				line2 = String.format("avatar %.0f %.0f %.0f", HUD_STATE.playerPos[0], HUD_STATE.playerPos[1], HUD_STATE.playerPos[2]);
			} else if (CoordMap.get() != null && CoordMap.get().provisional()) {
				line2 = "avatar position unknown (provisional anchor)";
			}
		}
		g.drawString(mc.font, line1, 4, 4, 0xFFFFFF, true);
		if (line2 != null) {
			g.drawString(mc.font, line2, 4, 14, 0xFFFFFF, true);
		}
	}
}
