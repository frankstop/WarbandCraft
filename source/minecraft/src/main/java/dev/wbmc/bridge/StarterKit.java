package dev.wbmc.bridge;

import net.minecraft.server.level.ServerPlayer;
import net.minecraft.world.item.Item;
import net.minecraft.world.item.ItemStack;
import net.minecraft.world.item.Items;

/** Seeds only an empty inventory in the dedicated bridge world. */
public final class StarterKit {
    public static void giveIfEmpty(ServerPlayer p) {
        if (!TerrainManager.isBridgeWorld() || !p.getInventory().isEmpty()) return;
        Item[] hotbar = {Items.OAK_PLANKS, Items.CRAFTING_TABLE, Items.REDSTONE,
            Items.REDSTONE_TORCH, Items.TNT, Items.FLINT_AND_STEEL, Items.FIREWORK_ROCKET,
            Items.DIAMOND_SWORD, Items.DIAMOND_PICKAXE};
        for (int i=0; i<hotbar.length; i++) p.getInventory().setItem(i,
            new ItemStack(hotbar[i], i==1 || i==5 || i==7 || i==8 ? 1 : 64));
        p.getInventory().setItem(38, new ItemStack(Items.ELYTRA));
        p.getInventory().add(new ItemStack(Items.OAK_LOG,64));
        p.getInventory().add(new ItemStack(Items.IRON_INGOT,32));
        p.getInventory().add(new ItemStack(Items.STONE,64));
        p.getInventory().add(new ItemStack(Items.LEVER,64));
        p.getInventory().add(new ItemStack(Items.REDSTONE_LAMP,64));
        p.getInventory().add(new ItemStack(Items.COOKED_BEEF,64));
        p.getInventory().add(new ItemStack(Items.SHIELD));
        p.getInventory().add(new ItemStack(Items.BOW));
        p.getInventory().add(new ItemStack(Items.ARROW,64));
    }
}
