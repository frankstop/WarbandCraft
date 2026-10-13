# WarbandCraft — A Failed Mac integration

This build runs real Minecraft Java 1.21.1 with Fabric alongside Steam's native Intel macOS Warband 1.174 under Rosetta. Warband draws Minecraft's world, hand, Steve model, and HUD in its own OpenGL frame. Warband's mission scripts supply real player and agent positions, health, camera control, and scene collision rays.

For the current isolated 0.2.1 patch, double-click **Play WarbandCraft 0.2.1.command** and read [RELEASE_STATUS.md](RELEASE_STATUS.md) for its complete feature/test matrix. The instructions below describe the older prototype started by **Play WarbandCraft.command** inside the Desktop's **WarbandCraft** folder. Steam must be running. The launcher selects the separate **WarbandCraft** module and starts the separate **WarbandCraft** Prism instance. Enter a playable scene, then press **F8** (or Fn+F8 on a Mac keyboard).

## Controls

| Key | Action |
|---|---|
| F8 | Switch Steve / Warband input and camera |
| WASD, mouse, Space, Shift | Minecraft movement, jump, sneak |
| E | Actual Minecraft inventory |
| Right / left click | Use or place / attack or break |
| 1–9 / wheel | Minecraft hotbar |
| F5 | First-person / Steve third-person cameras |
| F9 | Bridge status and entity diagnostics |
| Alt+1 / 2 / 3 / 0 | Infantry / archers / cavalry / everyone |
| Alt+F1 / F2 / F3 / F4 / F5 | Hold / follow / charge / hold fire / advance |
| T | Minecraft chat and commands |

The bridge world begins in Creative for testing. For actual survival inventory, crafting and damage, type `/gamemode survival`. Use `/gamemode creative` to return to free building. The starter kit is granted only to an empty inventory. It includes materials, a crafting table, redstone, TNT, flint and steel, weapons, an equipped Elytra, and rockets. For Elytra: jump from a height, press Space again while falling, then use a rocket.

Switching to another application hides Minecraft's input window and releases Warband's camera. It must not cover ChatGPT or the desktop. Press F8 in Warband to resume. Closing Minecraft also releases camera control automatically.

## Human test

1. Start a custom battle. Switch with F8; check that Warband soldiers keep moving.
2. Open E, select blocks, place and break several blocks. Walk and jump on them.
3. Place a crafting table. In Survival, craft planks/sticks from the supplied logs.
4. Wire a lever to redstone and a lamp. Toggle it. Ignite TNT near placed blocks.
5. Hit an enemy using the diamond sword or a bow; observe Warband health/defeat. In Survival, verify incoming soldier damage, armor and shields.
6. Test Elytra and rockets; use F5 to see Steve.
7. Switch to ChatGPT, then back to Warband. Confirm the overlay hides and F8 resumes.
8. Test town streets, taverns, villages, arenas, field battles and sieges. All 50 Native mission templates contain the bridge triggers; each scene type still needs acceptance testing.

## Current limits

- This is a prototype ready for gameplay testing, not a claim of complete scene compatibility.
- Warband troops currently do **not** collide with Minecraft blocks. Minecraft players do. Warband terrain is represented by sampled invisible Minecraft collision blocks, so small geometry can be coarse.
- Soldier proxy hitboxes are approximate capsules/boxes, not Warband's skeletal hitboxes. The table holds up to 1,024 live agents.
- Minecraft builds persist by Warband scene ID. Reused generated battlefield scene IDs can share builds. Campaign-specific and generated-terrain persistence needs further work.
- Warband's original terrain cannot be mined. Host fire/projectiles do not light Minecraft blocks automatically. Minecraft TNT, crafting, redstone and Elytra use Minecraft's actual simulation.
- Windowed Warband is the supported mode. Fullscreen, other Warband modules and multiplayer have not been validated.
- Minecraft keep-inventory and immediate respawn are enabled in this test world; Warband still owns battle defeat and campaign consequences.

## Files and diagnosis

Native log: `WarbandCraft/logs/native.log`. Minecraft log/world: Prism's `instances/WarbandCraft/minecraft/` directory. Source and build scripts are included in `source/`. `source/install_module.py` patches a copy of installed Native, preserving Native and existing other modules. `source/build_native.sh` builds the Intel renderer/loader. The loader permits native core updates without restarting the game; Minecraft JAR updates require restarting Minecraft.

The failed Desktop installation was preserved separately. No Steam game executable was patched, no Minecraft account credentials were copied, and existing Prism worlds were not overwritten.

Upstream inspiration and Minecraft-side source: [minecraft-crossover-bridge](https://github.com/justbustin/minecraft-crossover-bridge), [SkyCraft](https://github.com/chasmlol/SkyCraft), and [Minecraft-Ring](https://github.com/siddoff/Minecraft-Ring). The native Warband mission mailbox, engine operations, OpenGL compositor and loader were implemented here. See the included upstream license.

## Current patch (0.2.1)

**Play WarbandCraft 0.2.1.command** (or **Play WarbandCraft Next.command**) starts the isolated Next instance/module. It fixes the screenshot corruption, native collider multiplier, and campaign-world teardown, and provides a Minecraft app identity for direct Computer Use. See [RELEASE_STATUS.md](RELEASE_STATUS.md) for everything inherited from 0.2.0, actual automated/live test results, evidence, and unresolved shared-world milestones. Paired native saves remain opt-in; navigation and arbitrary native geometry import are not complete. The older normal launcher remains available.
