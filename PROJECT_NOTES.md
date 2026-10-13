# WarbandCraft: what is missing

Reviewed 12 October 2026 from the Desktop project's source, README, and validation records. This is a static review; no new gameplay session was run. Existing validation results are reported by the project, not independently reproduced here.

The project has a working integration prototype according to its validation records: Minecraft frame composition, camera correspondence, player control, agent position/health transfer, army orders, terrain probes, and damage transport. The largest remaining gap is making Minecraft construction, Survival, and Warband's campaign operate as one consistent game.

## 1. Buildings that affect Warband troops — highest gameplay priority

The README explicitly states that native troops do not collide with Minecraft blocks. There is also no block-to-native navigation update in the reviewed adapter or mission patcher.

A wall can therefore stop Steve without being a useful fortification against native troops. Bridges, stairs, gates, and towers need native collision and navigation support before they become reliable army tactics. Native projectile blocking by player builds also needs implementation or explicit testing; visual depth occlusion alone does not establish projectile collision.

Acceptance: build a wall, doorway, and bridge; infantry and cavalry must respect them, navigate openings, and respond correctly when blocks are removed. Check arrows against the same structures.

## 2. Campaign-specific saves and stable battlefield identity

WorldBootstrap always opens the same `warband-bridge` Minecraft world. TerrainManager stores anchors by scene ID, with separate Minecraft regions for those IDs. This preserves construction, but does not associate it with a Warband campaign save or generated battlefield layout. The README acknowledges that reused battlefield IDs can share builds.

Missing: campaign identifiers, stable location/terrain identifiers, coordinated save/load behavior, and a recovery policy when only one game's save advances. Minecraft inventory also needs a deliberate relationship to the selected campaign.

Acceptance: build in campaign A, open campaign B, and verify separation. Reload A and revisit both permanent locations and generated battlefields without misplaced construction.

## 3. A renewable Survival resource and progression loop

WorldBootstrap creates a void Creative world, disables natural mob spawning, and freezes daylight/weather. Native terrain is intentionally unbreakable. StarterKit supplies powerful gear and a finite collection of materials.

Crafting exists because this uses actual Minecraft, but the bridge does not supply a designed way to obtain renewable wood, ore, food, or other resources from Warband locations. No custom merchant, loot, quest, denar, or crafting progression integration was found in the reviewed source. Native Warband's existing campaign systems remain separate.

Missing: choose resource sources such as harvestable props, designated mining areas, merchants, or battle loot, then connect them to costs and rewards. Define whether the intended default is a Creative sandbox or a Survival campaign.

## 4. More faithful troop and combat integration

The mission patcher reads team and troop IDs, but the native adapter currently publishes every agent as the same generic kind, name, and human-sized hitbox. Team, troop identity, equipment, and mounted state are not carried into the published entity representation.

Missing: friendly/enemy identification and intentional friendly-fire rules; differentiated horses/riders and troop hitboxes; an explicit mapping for equipment, armor, shields, and combat balance. Java sends hit position and damage flags, but the native adapter forwards only target ID and damage amount into the mission mailbox.

Minecraft soldier proxies deliberately ignore environmental damage unless it is an attack/projectile/explosion. Lava, drowning, and other environmental effects therefore need separate bridge behavior if they should affect native troops. FireBridge contains a hazard consumer, but the reviewed native adapter does not publish fire hazards.

Acceptance: verify melee, arrows, TNT, shields, allies, horses, environmental hazards, and troop kills in actual battles. Transport checks alone do not validate those outcomes.

## 5. Unified player health, death, and campaign consequences

LifeBridge enables keep-inventory and immediate respawn, reports Minecraft death to Warband, and recalls the player. The mission hit trigger suppresses native hit damage while Steve owns control and reports it to Minecraft instead. These are prototype rules rather than a complete shared character model.

Missing: decide how healing, food, native campaign health, wounds, defeat, capture, inventory loss, and respawning interact. Define what should happen when changing between Steve and the Warband character while injured.

Acceptance: test repeated control switches, injury/healing, Minecraft death, native defeat, and loading a saved campaign. The resulting health and inventory must follow a documented rule.

## 6. Reliable collision across all scene geometry

TerrainManager approximates native terrain with invisible blocks using downward and horizontal rays near the player. The source includes fixed probe heights and fill limits, plus a flat-floor fallback when ray support fails.

This needs acceptance testing on stairs, slopes, bridges, overhangs, interiors, doors, water, and siege structures. Fast Elytra movement also needs testing against the rate at which terrain samples arrive. The current representation does not establish accurate support for overlapping floors or moving native props.

Acceptance: a repeatable route through each geometry type, with checks for falling through surfaces, invisible barriers, and incorrect landing heights.

## 7. Complete installation and useful startup recovery

The launcher assumes an existing Prism instance called WarbandCraft and built native libraries. The packaged installer patches the Warband module; it does not provision the Minecraft instance, install Fabric/Java, or deploy the mod into a new instance.

Missing: dependency checks, a reproducible Prism setup/import, version compatibility checks, and clear errors when either process or the bridge fails. The launcher also accepts any already-running Warband process, without verifying that it loaded the bridge and correct module.

Acceptance: install on a clean supported Mac using the documented process, then exercise missing Prism, wrong instance, wrong module, and disconnected bridge cases.

## 8. Repeatable regression checks and broader gameplay evidence

VALIDATION.md records camera measurements, isolated Minecraft redstone/TNT tests, transport checks, builds, and a 25-vs-25 battle. It explicitly leaves soldier combat, Survival damage, Elytra feel, subjective composition, and all scene types for human acceptance. The Desktop source does not include a reproducible test suite or the scripts behind the recorded transport checks.

Missing: retain those test tools with the source, record repeatable gameplay scenarios, and measure frame time, input latency, memory, and terrain sampling in long sessions and larger battles. The published entity table has a 1,024-agent cap.

Windowed native macOS Warband is the documented target. Fullscreen, other modules, multiplayer, and other platforms are expansion work, not established supported modes.

## Suggested order

1. Reproduce the current core gameplay checks and retain the test tools.
2. Make player builds affect native troops and projectiles.
3. Isolate saves by campaign and stable location.
4. Define and implement the Survival resource loop and shared character rules.
5. Improve combat identity, scene collision, installation, and performance based on the resulting tests.

Optional later expansion: multiplayer, other modules/platforms, richer formations, construction quests, settlement upgrades, and a more complete in-game settings/tutorial interface.
