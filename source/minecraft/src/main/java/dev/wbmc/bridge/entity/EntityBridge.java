package dev.wbmc.bridge.entity;

import dev.wbmc.bridge.CoordMap;
import dev.wbmc.bridge.TerrainManager;
import dev.wbmc.bridge.link.EntityInfo;
import dev.wbmc.bridge.link.WarbandLink;
import dev.wbmc.bridge.link.Protocol;
import it.unimi.dsi.fastutil.longs.Long2ObjectOpenHashMap;
import it.unimi.dsi.fastutil.longs.LongOpenHashSet;
import net.minecraft.core.particles.ParticleTypes;
import net.minecraft.network.chat.Component;
import net.minecraft.server.MinecraftServer;
import net.minecraft.server.level.ServerLevel;
import net.minecraft.world.damagesource.DamageSource;
import net.minecraft.tags.DamageTypeTags;
import net.minecraft.util.Mth;
import net.minecraft.world.entity.Entity;
import net.minecraft.world.entity.projectile.Projectile;
import net.minecraft.world.phys.AABB;
import net.minecraft.world.phys.Vec3;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.List;

/**
 * Keeps one {@link WarbandEntity} per hittable Warband entity, matching Warband's positions and
 * hitboxes every tick, and forwards Minecraft hits on them to Warband as damage.
 */
public final class EntityBridge {
	private EntityBridge() {
	}

	private static final Logger LOG = LoggerFactory.getLogger("warbandcraft");
	/** Minecraft damage -> Warband HP. A netherite sword swing (8) removes ~120 HP. */
	public static final float DAMAGE_SCALE = 3.0F;
	/**
	 * Explosions: point-blank TNT deals up to ~57 Minecraft damage, i.e. ~340 Warband HP, a bit
	 * more than Warband's Large Barrel Bomb+ (180 base).
	 */
	public static final float EXPLOSION_SCALE = 2.0F;

	private static final Long2ObjectOpenHashMap<WarbandEntity> PROXIES = new Long2ObjectOpenHashMap<>();
	private static final List<EntityInfo> ENTITIES = new ArrayList<>();
	private static final LongOpenHashSet SEEN = new LongOpenHashSet();

	public static void reset() {
		PROXIES.clear();
	}

	public static void onServerTick(MinecraftServer server) {
		if (!TerrainManager.isBridgeWorld()) {
			return;
		}
		CoordMap.Mapping map = CoordMap.get();
		ServerLevel level = server.overworld();
		if (map == null || map.provisional() || !WarbandLink.get().readEntities(ENTITIES)) {
			removeAll();
			return;
		}
		SEEN.clear();
		for (EntityInfo e : ENTITIES) {
			SEEN.add(e.id);
			WarbandEntity proxy = PROXIES.get(e.id);
			if (proxy == null || proxy.isRemoved()) {
				proxy = WarbandBridgeEntities.Warband_ENTITY.create(level);
				if (proxy == null) {
					continue;
				}
				proxy.setWarbandId(e.id, e.kind);
				apply(proxy, e, map);
				level.addFreshEntity(proxy);
				PROXIES.put(e.id, proxy);
				LOG.info("Warband entity appeared: {} (kind {}, hp {}/{})", e.name, e.kind, e.hp, e.maxHp);
			} else {
				apply(proxy, e, map);
			}
		}
		PROXIES.long2ObjectEntrySet().removeIf(en -> {
			if (!SEEN.contains(en.getLongKey())) {
				en.getValue().discard();
				return true;
			}
			return false;
		});
	}

	private static void removeAll() {
		for (WarbandEntity e : PROXIES.values()) {
			e.discard();
		}
		PROXIES.clear();
	}

	/** Oriented Warband hitbox -> axis-aligned Minecraft box (blocks). */
	private static void apply(WarbandEntity proxy, EntityInfo e, CoordMap.Mapping map) {
		float[] h = e.boxHalf;
		float[] c = e.boxCenter;
		if (h[0] <= 0 || h[1] <= 0 || h[2] <= 0) {
			// No hitbox from Warband: rough defaults around the entity origin.
			boolean large = e.kind == Protocol.ENT_LARGE_MONSTER;
			h = large ? new float[] {0.6F, 1.2F, 0.6F} : new float[] {0.35F, 0.9F, 0.35F};
			c = new float[] {e.pos[0], e.pos[1] + h[1], e.pos[2]};
		}
		double[] w = new double[3];
		if ((e.flags & 2) != 0) {
			// Already a world-aligned box (Warband's model bounds).
			for (int i = 0; i < 3; i++) {
				w[i] = h[i];
			}
		} else {
			float[] r = rotation(e.quat);
			for (int i = 0; i < 3; i++) {
				w[i] = Math.abs(r[i * 3]) * h[0] + Math.abs(r[i * 3 + 1]) * h[1] + Math.abs(r[i * 3 + 2]) * h[2];
			}
		}
		double s = 1.0 / map.unitsPerMeter();
		Vec3 center = map.toMc(c[0], c[1], c[2]);
		float hx = (float) (w[0] * s);
		float hy = (float) (w[1] * s);
		float hz = (float) (w[2] * s);
		proxy.setBox(hx, hy, hz);
		proxy.setPos(center.x, center.y - hy, center.z);
		Component name = proxy.getCustomName();
		if (name == null || !name.getString().equals(e.name)) {
			proxy.setCustomName(Component.literal(e.name));
		}
		if (e.maxHp > 0) {
			proxy.setHp(e.hp, e.maxHp);
		}
	}

	/** The live proxy of Warband entity {@code id}, or null. */
	public static WarbandEntity proxy(long id) {
		WarbandEntity e = PROXIES.get(id);
		return e != null && !e.isRemoved() ? e : null;
	}

	/** The live proxy whose hitbox is nearest to {@code pos} (within {@code maxDist} blocks), or null. */
	static WarbandEntity nearestProxy(Vec3 pos, double maxDist) {
		WarbandEntity best = null;
		double bestSq = maxDist * maxDist;
		for (WarbandEntity e : PROXIES.values()) {
			if (e.isRemoved()) {
				continue;
			}
			double d = e.distanceToSqr(pos);
			if (d < bestSq) {
				best = e;
				bestSq = d;
			}
		}
		return best;
	}

	/** Point of {@code box} nearest to {@code p}. */
	private static Vec3 closestPoint(AABB box, Vec3 p) {
		return new Vec3(Mth.clamp(p.x, box.minX, box.maxX), Mth.clamp(p.y, box.minY, box.maxY), Mth.clamp(p.z, box.minZ, box.maxZ));
	}

	/** Row-major 3x3 rotation matrix of quaternion (x, y, z, w). */
	private static float[] rotation(float[] q) {
		float x = q[0], y = q[1], z = q[2], w = q[3];
		float n = x * x + y * y + z * z + w * w;
		if (n < 1e-6F) {
			return new float[] {1, 0, 0, 0, 1, 0, 0, 0, 1};
		}
		float s = 2.0F / n;
		return new float[] {
			1 - s * (y * y + z * z), s * (x * y - z * w), s * (x * z + y * w),
			s * (x * y + z * w), 1 - s * (x * x + z * z), s * (y * z - x * w),
			s * (x * z - y * w), s * (y * z + x * w), 1 - s * (x * x + y * y)
		};
	}

	static void onHit(WarbandEntity proxy, DamageSource source, float amount, boolean critical) {
		CoordMap.Mapping map = CoordMap.get();
		if (map == null) {
			return;
		}
		// Where the blow landed: nearest the blast or projectile, else the attacker's line of
		// sight through the hitbox, else its center.
		AABB box = proxy.getBoundingBox();
		Vec3 hit = box.getCenter();
		int flags = critical ? Protocol.DAMAGE_CRITICAL : 0;
		float scale = DAMAGE_SCALE;
		String kind = critical ? "melee, critical" : "melee";
		Vec3 blast = source.getSourcePosition();
		if (source.is(DamageTypeTags.IS_EXPLOSION) && blast != null) {
			hit = closestPoint(box, blast);
			flags |= Protocol.DAMAGE_OUTWARD;
			scale = EXPLOSION_SCALE;
			kind = "explosion";
		} else if (source.getDirectEntity() instanceof Projectile projectile) {
			hit = closestPoint(box, projectile.position());
			flags |= Protocol.DAMAGE_OUTWARD;
			kind = "projectile";
		} else if (source.getEntity() != null) {
			Entity attacker = source.getEntity();
			Vec3 eye = attacker.getEyePosition();
			Vec3 end = eye.add(attacker.getLookAngle().scale(8.0));
			hit = box.clip(eye, end).orElse(hit);
		}
		double[] m = map.toWarband(hit.x, hit.y, hit.z);
		float dmg = amount * scale;
		boolean ok = WarbandLink.get().pushDamage(proxy.warbandId(), dmg, (float) m[0], (float) m[1], (float) m[2], flags);
		if (proxy.level() instanceof ServerLevel level) {
			level.sendParticles(ParticleTypes.DAMAGE_INDICATOR, hit.x, hit.y, hit.z, Math.max(1, (int) (amount / 2)), 0.2, 0.2, 0.2, 0.2);
		}
		LOG.info("Hit {} ({}) for {} MC -> {} Warband damage ({})", proxy.getName().getString(), kind, amount, dmg, ok ? "sent" : "queue full");
	}
}
