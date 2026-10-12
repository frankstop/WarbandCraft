package dev.wbmc.bridge;

import net.minecraft.server.MinecraftServer;
import net.minecraft.server.level.ServerPlayer;
import net.minecraft.world.level.GameRules;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/** Reports Steve's death to the matching Warband mission generation, then recalls
 * the respawned Minecraft player. Warband's mission owns defeat and campaign outcomes.
 * Keep-inventory is an explicit experimental bridge-world rule. */
public final class LifeBridge {
	private LifeBridge() {
	}

	private static final Logger LOG = LoggerFactory.getLogger("warbandcraft");

	public static void onServerStarted(MinecraftServer server) {
		if (!TerrainManager.isBridgeWorld()) {
			return;
		}
		server.getGameRules().getRule(GameRules.RULE_DO_IMMEDIATE_RESPAWN).set(true, server);
		server.getGameRules().getRule(GameRules.RULE_KEEPINVENTORY).set(true, server);
	}

	public static void onRespawn(ServerPlayer oldPlayer, ServerPlayer newPlayer, boolean alive) {
		if (!TerrainManager.isBridgeWorld() || alive) {
			return;
		}
		LOG.info("{} died: notify Warband mission and recall", newPlayer.getName().getString());
		dev.wbmc.bridge.link.WarbandLink.get().reportPlayerDeath();
		TerrainManager.requestRecall();
	}
}
