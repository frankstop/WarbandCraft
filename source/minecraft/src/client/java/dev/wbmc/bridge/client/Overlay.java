package dev.wbmc.bridge.client;

import dev.wbmc.bridge.link.GameState;
import dev.wbmc.bridge.link.WarbandLink;
import dev.wbmc.bridge.link.Protocol;
import dev.wbmc.bridge.TerrainManager;
import net.minecraft.client.Minecraft;
import net.minecraft.client.gui.screens.PauseScreen;
import org.lwjgl.glfw.GLFW;
import org.lwjgl.glfw.GLFWNativeCocoa;
import org.lwjgl.system.JNI;
import org.lwjgl.system.Platform;
import org.lwjgl.system.macosx.ObjCRuntime;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Overlay mode: Minecraft's window becomes a borderless, always-on-top, transparent layer
 * glued to Warband's window. Only Minecraft's own content (blocks, entities, hand, HUD) is
 * opaque; everywhere else Warband shows through, composited by macOS.
 *
 * <p>The window is always created with a transparent framebuffer so overlay mode can turn
 * on/off at runtime (e.g. when Warband starts after Minecraft). While off, the final blit forces
 * alpha to 1 so the window looks completely normal.
 */
public final class Overlay {
	private Overlay() {
	}

	private static final Logger LOG = LoggerFactory.getLogger("warbandcraft");

	private static boolean transparentWindow;
	private static boolean active;
	/** The player handed control to Warband (F8): our window is hidden and Minecraft is paused. */
	private static boolean warbandMode = true;
	private static int lastSwitchReq = Integer.MIN_VALUE;
	private static long window;
	private static int savedX, savedY, savedW, savedH;
	private static int appliedX = Integer.MIN_VALUE, appliedY, appliedW, appliedH;
	private static long lastInWorldMs;
	private static long lastWantMs;
	private static long focusGraceUntil;
	private static final GameState STATE = new GameState();

	public static boolean active() {
		return active;
	}

	public static boolean transparentWindow() {
		return transparentWindow;
	}

	public static boolean warbandMode() {
		return warbandMode;
	}

	/** F8 in Minecraft: hide and pause Minecraft, give Warband its camera and the keyboard/mouse. */
	public static void switchToWarband(Minecraft mc) {
		switchToWarband(mc, true);
	}
	private static void switchToWarband(Minecraft mc, boolean refocus) {
		if (warbandMode || window == 0L) {
			return;
		}
		warbandMode = true;
		CameraSync.releaseWarbandCamera();
		mc.options.pauseOnLostFocus = false; // Redstone and TNT continue while Warband owns input.
		mc.mouseHandler.releaseMouse();
		GLFW.glfwHideWindow(window);
		if (refocus) WarbandLink.get().requestWarbandFocus();
		LOG.info("Control -> Warband");
	}

	/** F8 in Warband (reported by the DLL): bring Minecraft back on top. */
	public static void switchToMc(Minecraft mc) {
		if (!warbandMode) {
			return;
		}
		warbandMode = false;
		focusGraceUntil = System.currentTimeMillis() + 1500;
		GLFW.glfwShowWindow(window);
		GLFW.glfwFocusWindow(window);
		if (Platform.get() == Platform.MACOSX) {
			long msgSend = ObjCRuntime.getLibrary().getFunctionAddress("objc_msgSend");
			long app = JNI.invokePPP(ObjCRuntime.objc_getClass("NSApplication"), ObjCRuntime.sel_getUid("sharedApplication"), msgSend);
			if (app != 0) JNI.invokePPV(app, ObjCRuntime.sel_getUid("activateIgnoringOtherApps:"), true, msgSend);
		}
		if (mc.screen instanceof PauseScreen) {
			mc.setScreen(null);
		}
		LOG.info("Control -> Minecraft");
		if (mc.screen == null) mc.mouseHandler.grabMouse();
	}

	/** Called right before GLFW creates Minecraft's window. */
	public static void applyWindowHints() {
		if (Boolean.getBoolean("warbandcraft.disableOverlay")) {
			return;
		}
		GLFW.glfwWindowHint(GLFW.GLFW_TRANSPARENT_FRAMEBUFFER, GLFW.GLFW_TRUE);
		transparentWindow = true;
		if (Platform.get() == Platform.MACOSX) {
			// Match Warband's pixel density (CrossOver renders it at 1x) and save 4x fill rate.
			// Always, not only when Warband is already up: a Retina framebuffer is twice Warband's
			// size, too big to be drawn inside Warband's frame.
			GLFW.glfwWindowHint(GLFW.GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW.GLFW_FALSE);
		}
	}

	public static void onWindowCreated(long handle) {
		window = handle;
		if (handle == 0L) {
			transparentWindow = false;
		}
	}

	/** Once per frame on the render thread, before anything is drawn. */
	public static void onFrame(Minecraft mc) {
		WarbandLink link = WarbandLink.get();
		boolean alive = link.poll();
		link.bumpMcHeartbeat();
		int req = link.mcSwitchRequests();
		if (lastSwitchReq == Integer.MIN_VALUE) {
			lastSwitchReq = req;
		} else if (req != lastSwitchReq) {
			lastSwitchReq = req;
			switchToMc(mc);
		}
		boolean haveState = alive && link.snapshot(STATE);
		// Only take over once a avatar is actually in the world, so Warband's title screen and
		// menus stay usable. Short gaps (area loads) don't toggle the window.
		long now = System.currentTimeMillis();
		if (active && !warbandMode && now > focusGraceUntil && !mc.isWindowActive()) {
			switchToWarband(mc, false);
		}
		if (haveState && STATE.has(Protocol.STATE_PLAYER_VALID)) {
			lastInWorldMs = now;
		}
		boolean inWorld = haveState && now - lastInWorldMs < 3000;
		boolean want = transparentWindow && window != 0L && inWorld && !warbandMode && !mc.getWindow().isFullscreen();
		if (want) {
			lastWantMs = now;
		}
		// Turn on immediately, but only turn off after a sustained reason (or a user toggle).
		if (want && !active) {
			setActive(mc, true);
		} else if (!want && active && (warbandMode || now - lastWantMs > 1500)) {
			setActive(mc, false);
		}
		if (active && STATE.has(Protocol.STATE_WINDOW_VALID)) {
			follow(STATE.winX, STATE.winY, STATE.winW, STATE.winH);
			// Older GLFW builds on current macOS can ignore the Retina hint. Warband
			// renders at one pixel per point, so use its actual render dimensions.
			int w = STATE.bbW > 0 ? STATE.bbW : STATE.winW;
			int h = STATE.bbH > 0 ? STATE.bbH : STATE.winH;
			if (w > 0 && h > 0 && (mc.getWindow().getWidth() != w || mc.getWindow().getHeight() != h)) {
				mc.getWindow().setWidth(w);
				mc.getWindow().setHeight(h);
				mc.resizeDisplay();
				LOG.info("Bridge render resolution {}x{}",w,h);
			}
		}
	}

	private static void setActive(Minecraft mc, boolean on) {
		active = on;
		LOG.info("Overlay mode {}", on ? "ON" : "OFF");
		if (on) {
			// Taking over (startup, back from Warband with F8, after a loading screen): Warband's avatar
			// is where the player really is, so Steve starts there, and nothing drives Warband's
			// camera or avatar until he has been moved.
			CameraSync.holdForRecall();
			TerrainManager.requestRecall();
			int[] x = new int[1], y = new int[1], w = new int[1], h = new int[1];
			GLFW.glfwGetWindowPos(window, x, y);
			GLFW.glfwGetWindowSize(window, w, h);
			savedX = x[0];
			savedY = y[0];
			savedW = w[0];
			savedH = h[0];
			GLFW.glfwSetWindowAttrib(window, GLFW.GLFW_DECORATED, GLFW.GLFW_FALSE);
			GLFW.glfwSetWindowAttrib(window, GLFW.GLFW_FLOATING, GLFW.GLFW_TRUE);
			setShadow(false);
			// When Warband draws our frames, this window is fully transparent; macOS would then
			// let clicks fall through to Warband unless told otherwise.
			objcBool("setIgnoresMouseEvents:", false);
			appliedX = Integer.MIN_VALUE;
		} else if (!warbandMode) {
			GLFW.glfwSetWindowAttrib(window, GLFW.GLFW_FLOATING, GLFW.GLFW_FALSE);
			GLFW.glfwSetWindowAttrib(window, GLFW.GLFW_DECORATED, GLFW.GLFW_TRUE);
			setShadow(true);
			if (savedW > 0 && savedH > 0) {
				GLFW.glfwSetWindowSize(window, savedW, savedH);
				GLFW.glfwSetWindowPos(window, savedX, savedY);
			}
		}
	}

	/**
	 * Warband reports its client area in Windows screen coordinates. CrossOver (Retina mode off)
	 * maps one Windows pixel to one macOS point with the same top-left origin as GLFW.
	 */
	private static void follow(int x, int y, int w, int h) {
		if (w < 64 || h < 64) {
			return;
		}
		if (x == appliedX && y == appliedY && w == appliedW && h == appliedH) {
			return;
		}
		GLFW.glfwSetWindowSize(window, w, h);
		GLFW.glfwSetWindowPos(window, x, y);
		appliedX = x;
		appliedY = y;
		appliedW = w;
		appliedH = h;
		LOG.info("Overlay glued to Warband client area {}x{} at {},{}", w, h, x, y);
	}

	/** Borderless transparent windows would otherwise cast a shadow around every block. */
	private static void setShadow(boolean shadow) {
		objcBool("setHasShadow:", shadow);
	}

	/** Calls an NSWindow setter taking a BOOL. */
	private static void objcBool(String selector, boolean value) {
		if (Platform.get() != Platform.MACOSX) {
			return;
		}
		try {
			long nsWindow = GLFWNativeCocoa.glfwGetCocoaWindow(window);
			long msgSend = ObjCRuntime.getLibrary().getFunctionAddress("objc_msgSend");
			if (nsWindow != 0L && msgSend != 0L) {
				JNI.invokePPV(nsWindow, ObjCRuntime.sel_getUid(selector), value, msgSend);
			}
		} catch (Throwable t) {
			LOG.warn("NSWindow {} failed: {}", selector, t.toString());
		}
	}
}
