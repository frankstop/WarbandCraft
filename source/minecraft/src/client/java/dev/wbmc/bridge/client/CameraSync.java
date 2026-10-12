package dev.wbmc.bridge.client;

import dev.wbmc.bridge.CoordMap;
import dev.wbmc.bridge.link.ControlState;
import dev.wbmc.bridge.link.GameState;
import dev.wbmc.bridge.link.WarbandLink;
import dev.wbmc.bridge.link.Protocol;
import net.minecraft.client.Camera;
import net.minecraft.client.Minecraft;
import net.minecraft.world.phys.Vec3;
import org.joml.Vector3f;

/**
 * Keeps the two cameras identical so Minecraft's layer lines up with Warband's frame.
 *
 * <ul>
 *   <li>FOLLOW_Warband: Minecraft renders from Warband's camera (Warband stays in charge). Needs only
 *   read access to Warband's camera; used to verify alignment.</li>
 *   <li>DRIVE_Warband: Warband renders from Minecraft's camera, so the Minecraft player can walk
 *   around Warband's world. Needs the DLL's camera override.</li>
 * </ul>
 */
public final class CameraSync {
	private CameraSync() {
	}

	public enum Mode {
		FOLLOW_Warband,
		DRIVE_Warband
	}

	public interface CameraAccess {
		void warbandcraft$setPosition(Vec3 pos);

		void warbandcraft$setRotation(float yaw, float pitch);
	}

	private static Mode mode = Mode.DRIVE_Warband;
	/** The avatar follows the Minecraft player (hidden) and takes Warband's hits for it. */
	private static boolean standIn = true;
	/** Debug knobs for the in-Warband compositor (see dev commands "pt ..."). */
	public static int poseLag = 1;
	public static int depthIndex = 0;
	public static boolean noDepthTest;
	public static boolean debugDepth;
	private static final GameState STATE = new GameState();
	private static final ControlState CONTROL = new ControlState();
	private static float fov = 70.0F;
	private static boolean controlling;
	/**
	 * After control comes back from Warband, Warband's avatar is the truth: keep our hands off Warband's
	 * camera and avatar until the server has moved the Minecraft player to it and the client
	 * has received the new position.
	 */
	private static int holdUntilRecall = Integer.MIN_VALUE;
	private static long heldSinceRecallMs;

	public static Mode mode() {
		return mode;
	}

	public static void toggleMode() {
		mode = mode == Mode.FOLLOW_Warband ? Mode.DRIVE_Warband : Mode.FOLLOW_Warband;
	}

	public static void holdForRecall() {
		holdUntilRecall = dev.wbmc.bridge.TerrainManager.recallServed() + 1;
		heldSinceRecallMs = 0;
	}

	/** True when the Minecraft player may drive Warband (right zone, already placed at the avatar). */
	private static boolean ready(Minecraft mc, CoordMap.Mapping map) {
		if (map.provisional() || !STATE.has(Protocol.STATE_PLAYER_VALID) || map.zone() != STATE.stageId) {
			return false;  // the server hasn't switched to this Warband zone yet
		}
		if (mc.player == null || !map.contains(mc.player.getX())) {
			return false;  // teleport into this zone's region not received yet
		}
		if (holdUntilRecall != Integer.MIN_VALUE) {
			if (dev.wbmc.bridge.TerrainManager.recallServed() < holdUntilRecall) {
				return false;
			}
			long now = System.currentTimeMillis();
			if (heldSinceRecallMs == 0) {
				heldSinceRecallMs = now;
			}
			if (now - heldSinceRecallMs < 400) {
				return false;  // let the teleport reach the client
			}
			holdUntilRecall = Integer.MIN_VALUE;
		}
		return true;
	}

	public static boolean toggleStandIn() {
		standIn = !standIn;
		return standIn;
	}

	/** Field of view Minecraft must render with while the overlay is active. */
	public static float fov() {
		return fov;
	}

	/** True once Warband confirmed it rendered with our camera. */
	public static boolean warbandFollowing() {
		return STATE.has(Protocol.STATE_CAM_OVERRIDDEN);
	}

	private static boolean driving;

	/** True while Minecraft's camera is driving Warband's this frame. */
	public static boolean driving() {
		return driving;
	}

	/** True while Warband draws our frames into its own image. */
	public static boolean warbandCompositing() {
		return STATE.has(Protocol.STATE_COMPOSITING);
	}

	public static void copyCurrentPose(ControlState out) {
		out.flags = CONTROL.flags;
		out.fovYDeg = CONTROL.fovYDeg;
		out.poseLag = CONTROL.poseLag;
		out.depthIndex = CONTROL.depthIndex;
		System.arraycopy(CONTROL.camPos, 0, out.camPos, 0, 3);
		System.arraycopy(CONTROL.camTarget, 0, out.camTarget, 0, 3);
		System.arraycopy(CONTROL.camUp, 0, out.camUp, 0, 3);
		System.arraycopy(CONTROL.avatarPos, 0, out.avatarPos, 0, 3);
		out.avatarYawDeg = CONTROL.avatarYawDeg;
	}

	public static void afterCameraSetup(Camera camera) {
		Minecraft mc = Minecraft.getInstance();
		CoordMap.Mapping map = CoordMap.get();
		boolean haveState = WarbandLink.get().snapshot(STATE);
		if (!Overlay.active() || map == null || !haveState || (mode == Mode.DRIVE_Warband && !ready(mc, map))) {
			releaseWarbandCamera();
			fov = mc.options.fov().get();
			driving = false;
			return;
		}
		driving = mode == Mode.DRIVE_Warband;

		if (mode == Mode.FOLLOW_Warband) {
			releaseWarbandCamera();
			if (!STATE.has(Protocol.STATE_CAMERA_VALID)) {
				fov = mc.options.fov().get();
				return;
			}
			Vec3 pos = map.toMc(STATE.camPos[0], STATE.camPos[1], STATE.camPos[2]);
			Vec3 dir = map.dirToMc(STATE.camTarget[0] - STATE.camPos[0], STATE.camTarget[1] - STATE.camPos[1],
				STATE.camTarget[2] - STATE.camPos[2]).normalize();
			float yaw = (float) Math.toDegrees(Math.atan2(-dir.x, dir.z));
			float pitch = (float) Math.toDegrees(-Math.asin(Math.max(-1.0, Math.min(1.0, dir.y))));
			CameraAccess access = (CameraAccess) camera;
			access.warbandcraft$setRotation(yaw, pitch);
			access.warbandcraft$setPosition(pos);
			fov = STATE.fovYDeg > 1.0F ? STATE.fovYDeg : mc.options.fov().get();
			return;
		}

		// DRIVE_Warband: send our camera to Warband.
		fov = STATE.fovYDeg > 1.0F ? STATE.fovYDeg : mc.options.fov().get();
		Vec3 p = camera.getPosition();
		Vector3f f = camera.getLookVector();
		Vector3f u = camera.getUpVector();
		double[] pos = map.toWarband(p.x, p.y, p.z);
		double[] target = map.toWarband(p.x + f.x(), p.y + f.y(), p.z + f.z());
		double[] up = map.dirToWarband(u.x(), u.y(), u.z());
		boolean passthrough = FramePassthrough.wanted();
		CONTROL.flags = Protocol.CTRL_OVERRIDE_CAMERA
			| (standIn && mc.player != null ? Protocol.CTRL_MOVE_AVATAR | Protocol.CTRL_HIDE_AVATAR : 0)
			| (passthrough ? Protocol.CTRL_COMPOSITE : 0) | (noDepthTest ? Protocol.CTRL_NO_DEPTH_TEST : 0)
			| (debugDepth ? Protocol.CTRL_DEBUG_DEPTH : 0);
		CONTROL.poseLag = poseLag;
		CONTROL.depthIndex = depthIndex;
		CONTROL.mcFrame++;
		for (int i = 0; i < 3; i++) {
			CONTROL.camPos[i] = (float) pos[i];
			CONTROL.camTarget[i] = (float) target[i];
			CONTROL.camUp[i] = (float) up[i];
		}
		CONTROL.fovYDeg = fov;
		if (mc.player != null) {
			float pt = camera.getPartialTickTime();
			double[] feet = map.toWarband(net.minecraft.util.Mth.lerp(pt, mc.player.xo, mc.player.getX()),
				net.minecraft.util.Mth.lerp(pt, mc.player.yo, mc.player.getY()),
				net.minecraft.util.Mth.lerp(pt, mc.player.zo, mc.player.getZ()));
			for (int i = 0; i < 3; i++) {
				CONTROL.avatarPos[i] = (float) feet[i];
			}
			CONTROL.avatarYawDeg = mc.player.getViewYRot(pt);
		}
		controlling = true;
		if (!passthrough) {
			WarbandLink.get().writeControl(CONTROL);
		}
		// With passthrough, FramePassthrough sends this pose once the frame's pixels are ready.
	}

	public static void releaseWarbandCamera() {
			CONTROL.flags = 0;
			CONTROL.mcFrame++;
			WarbandLink.get().writeControl(CONTROL);
			controlling = false;
	}
}
