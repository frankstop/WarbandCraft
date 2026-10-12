package dev.wbmc.bridge.entity;

import dev.wbmc.bridge.WarbandBridgeMod;
import net.minecraft.core.Registry;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.resources.ResourceLocation;
import net.minecraft.world.entity.EntityType;
import net.minecraft.world.entity.MobCategory;

public final class WarbandBridgeEntities {
	private WarbandBridgeEntities() {
	}

	public static final EntityType<WarbandEntity> Warband_ENTITY = Registry.register(
		BuiltInRegistries.ENTITY_TYPE,
		ResourceLocation.fromNamespaceAndPath(WarbandBridgeMod.MOD_ID, "warband_entity"),
		EntityType.Builder.<WarbandEntity>of(WarbandEntity::new, MobCategory.MISC)
			.sized(1.0F, 1.0F)
			.noSave()
			.noSummon()
			.clientTrackingRange(16)
			.updateInterval(1)
			.build("warband_entity"));

	public static void init() {
		// A living entity (Minecraft's melee rules, crits, shields) needs its attributes.
		net.fabricmc.fabric.api.object.builder.v1.entity.FabricDefaultAttributeRegistry.register(Warband_ENTITY, WarbandEntity.createAttributes());
	}
}
