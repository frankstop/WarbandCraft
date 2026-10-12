package dev.wbmc.bridge.entity;

import dev.wbmc.bridge.CoordMap;
import dev.wbmc.bridge.TerrainManager;
import dev.wbmc.bridge.link.WarbandLink;
import dev.wbmc.bridge.link.Protocol;
import net.minecraft.core.Holder;
import net.minecraft.core.registries.Registries;
import net.minecraft.resources.ResourceKey;
import net.minecraft.resources.ResourceLocation;
import net.minecraft.server.MinecraftServer;
import net.minecraft.server.level.ServerLevel;
import net.minecraft.server.level.ServerPlayer;
import net.minecraft.world.damagesource.DamageSource;
import net.minecraft.world.damagesource.DamageType;
import net.minecraft.world.phys.Vec3;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Warband -> Minecraft damage. While the avatar stands in for the Minecraft player, Warband reports
 * the hits it takes and the HP it loses to status ailments.
 * <ul>
 *   <li>Each hit is a Minecraft attack by the monster that dealt it (the proxy of the monster
 *   nearest the avatar): a shield raised towards it blocks it, armor reduces it, and it
 *   knocks back. Its size is a base by attacker size plus the share of the avatar's HP lost.</li>
 *   <li>Status damage (poison, fire...) drains health once a second, proportional to the
 *   avatar's HP lost; shields and armor don't help (damage type {@code warbandcraft:status}).</li>
 * </ul>
 * Creative mode is invulnerable as usual.
 */
public final class CombatBridge {
	private CombatBridge() {
	}

	private static final Logger LOG = LoggerFactory.getLogger("warbandcraft");
	/** Attack by a monster with a Minecraft proxy ("slain by a Warband soldier"). */
	public static final ResourceKey<DamageType> MONSTER = key("monster");
	/** Attack by a monster without a proxy nearby ("slain by a monster"). */
	public static final ResourceKey<DamageType> MONSTER_HIT = key("monster_hit");
	/** Poison, fire and other status damage: bypasses shields and armor, no knockback. */
	public static final ResourceKey<DamageType> STATUS = key("status");

	private static final float[] EV = new float[9];
	/** Minecraft damage per Warband hit, before Minecraft armor: a base by attacker size plus a share of the Warband hit. */
	private static final float PROPORTIONAL = 30.0F; // x (Warband damage / avatar max HP)
	private static int lastHits = -1;
	private static float lastTotal;
	private static float lastStatus;
	private static float pendingStatus;
	private static int statusTicks;

	private static ResourceKey<DamageType> key(String name) {
		return ResourceKey.create(Registries.DAMAGE_TYPE, ResourceLocation.fromNamespaceAndPath("warbandcraft", name));
	}

	private static Holder<DamageType> type(ServerLevel level, ResourceKey<DamageType> key) {
		return level.registryAccess().registryOrThrow(Registries.DAMAGE_TYPE).getHolderOrThrow(key);
	}

	public static void reset() {
		lastHits = -1;
		pendingStatus = 0.0F;
		statusTicks = 0;
	}

	public static void onServerTick(MinecraftServer server) {
		if (!TerrainManager.isBridgeWorld() || !WarbandLink.get().readAvatarEvents(EV)) {
			return;
		}
		int hits = (int) EV[0];
		float total = EV[1];
		float status = EV[8];
		if (lastHits < 0 || hits < lastHits || total < lastTotal) {  // first read: only count damage from now on
			lastHits = hits;
			lastTotal = total;
			lastStatus = status;
			return;
		}
		float maxHp = EV[6] > 0 ? EV[6] : 150.0F;
		ServerLevel level = server.overworld();
		if (hits != lastHits) {
			float warbandDamage = total - lastTotal;
			lastHits = hits;
			lastTotal = total;
			CoordMap.Mapping map = CoordMap.get();
			Vec3 from = map != null ? map.toMc(EV[3], EV[4], EV[5]) : null;
			// Warband damage after the avatar's armor is tiny for well-geared avatars, so scale to
			// Minecraft terms instead: small monsters hit like a zombie, large ones much harder.
			float amount = Math.min(20.0F, Math.max(0.0F, warbandDamage / maxHp * 20.0F));
			for (ServerPlayer player : server.getPlayerList().getPlayers()) {
				hit(level, player, from, amount, warbandDamage, maxHp);
			}
		}

		pendingStatus += Math.max(0.0F, status - lastStatus);
		lastStatus = status;
		if (++statusTicks >= 20) {
			statusTicks = 0;
			float amount = pendingStatus / maxHp * PROPORTIONAL;
			if (amount >= 0.05F) {
				for (ServerPlayer player : server.getPlayerList().getPlayers()) {
					player.hurt(new DamageSource(type(level, STATUS)), amount);
				}
				LOG.info("Warband status damage {} HP -> {} Minecraft damage", pendingStatus, amount);
			}
			pendingStatus = 0.0F;
		}
	}

	private static void hit(ServerLevel level, ServerPlayer player, Vec3 from, float amount, float warbandDamage, float maxHp) {
		WarbandEntity attacker = from != null ? EntityBridge.nearestProxy(from, 32.0) : null;
		DamageSource source;
		if (attacker != null) {
			source = new DamageSource(type(level, MONSTER), attacker);
		} else if (from != null) {
			source = new DamageSource(type(level, MONSTER_HIT), from);
		} else {
			source = new DamageSource(type(level, MONSTER_HIT));
		}
		// Blocking needs to know where the blow came from: the attacker's position.
		boolean blocked = player.isDamageSourceBlocked(source);
		boolean hurt = player.hurt(source, amount);
		// Minecraft already knocks back by 0.4 away from the attacker; big Warband blows push
		// harder, unless the shield took them.
		if (hurt && !blocked && from != null) {
			double dx = from.x - player.getX();
			double dz = from.z - player.getZ();
			double extra = Math.min(1.1, warbandDamage / maxHp * 3.0);
			if (extra > 0.05 && dx * dx + dz * dz > 1e-4) {
				player.knockback(extra, dx, dz);
				player.hurtMarked = true;
			}
		}
		LOG.info("Warband hit {} for {} Warband HP -> {} Minecraft damage from {}{}", player.getName().getString(), warbandDamage, amount,
			attacker != null ? attacker.getName().getString() : "?",
			blocked ? " (blocked by shield)" : hurt ? "" : " (ignored: creative/invulnerable)");
	}
}
