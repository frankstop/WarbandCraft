# WarbandCraft 0.2.1 — 13 October 2026

0.2.1 is installed as the isolated **WarbandCraft-Next** build. Start it with **Play WarbandCraft 0.2.1.command**. The original **Play WarbandCraft.command** still starts the older prototype. Close both bridge games before launching either build.

The diagonal holes shown in the two supplied screenshots are fixed in the tested scenes. Codex can now use both games directly through Computer Use. This patch does not complete the shared physical world upgrade: the limits and acceptance gaps below are part of the release, not implied successes.

## What 0.2.1 fixes

- **Corrupted world occlusion:** Warband reused its scene framebuffer for a 160 × 90 post-processing depth target before presentation. Reading it as 1280 × 720 left most depth values invalid or stale, cutting diagonal holes into Steve and blocks. The bridge now retains the full-size depth attachment seen during perspective drawing, checks its dimensions and every returned value, and rejects unavailable depth.
- **Native collider scale:** the mission script interpreted millimetre coordinates with a multiplier of 100 instead of 1000. Placement and scale now use 1000 and restore the previous bridge convention afterward. A live native ray hits the correct one-block surface, then misses after removal.
- **World-switch hang:** Minecraft's client teardown waited for an integrated server whose connection had never been closed. Transitions now close the level connection before teardown. A custom-battle world saved and shut down, and the isolated campaign world reopened successfully.
- **Mission identity:** native generation changes now include the saved campaign identity and visit counter. A fresh troop allocation cannot reuse an earlier generation merely because its local mission counter starts at one again.
- **Computer Use launch:** Minecraft now runs with a local macOS application identity using the already-installed Temurin runtime. Prism retains account handling; its launch input travels through private pipes. The Next launcher refreshes the installed module on every launch and removes superseded bridge JARs from the active mod folder into backups.
- **Unknown ray normals:** native cast-ray responses report zero for an unavailable normal instead of fabricating an upward normal. Horizontal terrain probes already avoid depending on that missing information.

## Everything 0.2.0 introduced, and its current test result

| Feature area | What it brings | Validation in 0.2.1 | Limits / remaining acceptance |
|---|---|---|---|
| Depth composition | Matched frame/camera/projection data; float depth textures compatible with the native Mac renderer; separate world and hand/HUD layers; diagnostics | Real town wall hides the lower part of a far magenta column while a near lime cube remains visible. Solid Steve, first-person hand, inventory, placement and breaking observed through Computer Use. Castle and tavern transitions render correctly. GPU regression reproduces framebuffer reuse. | Windowed 1280 × 720 with native antialiasing off is the verified configuration. Multisampled, reduced and missing depth are rejected; multisample resolve is unsupported. Glass/water, resizing and the complete scene matrix have not passed acceptance. |
| Native collision resource | Invisible collision-only cube; bounded snapshots; native-thread placement/scaling and removed-prop retirement | Engine accepted the resource. For a Minecraft cube at (40960,120,0), a horizontal native ray hit its expected surface at native (-0.488,19.556,-14.252), marked as a mirrored collider. The same ray returned no hit after removal. Mailbox and all 50 mission-template structural checks pass. | A collision ray establishes presence and coordinates, not infantry/cavalry/projectile behavior. Those tests and navigation remain outstanding. |
| Campaign/location persistence | Saved campaign identity; separate Minecraft worlds; permanent-scene regions; distinct generated encounter regions; immutable paired-checkpoint store with recoverable rollback | Actual isolated campaign world and inventory reopened after a custom-battle world saved and shut down. Java fixtures verify immutable snapshots, corruption rejection, inventory/build restoration, displaced-world backups and unknown saves. | Native save interception remains **disabled by default** (`WBMC_PAIRED_SAVES=0`). Actual coordinated Warband save/load, Save As, older-checkpoint rollback and interruption recovery are not accepted. Legacy worlds are not automatically migrated. |
| Native-scene sampling | Horizontal obstacle probes; upper/lower sampled surfaces; thin prop floors; stale-ray cancellation; mission guards; periodic refresh and speed lookahead | Actual native rays work in town, custom battlefield, castle and tavern. Steve stopped at a sampled town wall. The interior interaction test found a full invisible voxel at the player's eye despite a visibly clear target farther away. | **Known failure:** coarse samples can create phantom barriers and block placement/targeting. This is not imported native geometry, decorative-object classification, or arbitrary overlapping-floor support. Stairs, ceilings, flowers, siege props and fast Elytra acceptance remain open. |
| Construction mirroring | Actual server collision shapes; compatible-box merging; dirty-chunk updates for placement, removal and world-driven changes, including doors/pistons/TNT/fluids; exclusion from native terrain rays | Real mouse placement consumed dirt; mouse breaking removed a block and returned an item. Cube insertion/removal reached native collision queries. Fixtures preserve gaps/slabs and reject stale or oversized snapshots. Earlier server test confirmed real redstone and TNT edits. | No claim of a functional 50-troop fort, native obstacle-aware routing, constructed bridge traversal or soldier pressure-plate → piston-door interaction. The budget is 2048 merged colliders; larger snapshots retain the previous complete snapshot. |
| Minecraft shader | Custom transparency-preserving color grading; independent depth and hand/HUD capture; disable option | Live lime block pixel changed from RGB (52,93,14) with grading off to (44,94,0) with it on. Native scene and HUD remain visible; no shader fallback error occurred. GPU tests preserve translucent alpha and zero coverage. | Iris/Sodium/Complementary integration remains future work. This is color grading, not a full lighting/shadow/reflection pack. Use `-Dwarbandcraft.disableShader=true` to disable it. |
| Native lighting | Emitting server blocks publish colored sources; nearest 32 published and up to 8 shaded; approximate illumination and screen-space occlusion | Powered redstone lamp brightens the actual native town wall and tavern surfaces. Removing power removes illumination. Saved on/off images and the GPU fixture demonstrate both states. | **Experimental visual quality:** visible patches, coarse shadow edges and inaccurate normals/materials. Off-screen shadows, moving troops, all source types, large-light budgets and frame-time targets are not accepted. |

Minecraft inventory, crafting, redstone, TNT and Elytra remain its own simulation. Warband retains its native armies, orders, AI, combat and campaign systems. Existing proxy hitboxes and damage transfer do not establish a shared, geometrically validated combat system. No end-to-end sword/arrow/TNT/shield combat or high-speed Elytra pass is claimed for this patch.

## Evidence and verification

Run `bash source/tests/run_all.sh` from the Desktop project. The completed run is recorded in `validation/automated-checks-0.2.1.log`. It covers native builds, real offscreen compatibility/core-profile GPU rendering, collider mailbox processing, Java checkpoint/shape fixtures, all generated mission templates, and the offline Fabric build.

Gameplay evidence is in `validation/0.2.1/`:

- `wall-occlusion.png`, `inventory.png`, `native-wall-collision.png`: live town checks using the repaired native renderer before the final Java world-transition patch.
- `castle-first-person.png`, `solid-steve-0.2.1.png`, `cua-block-placement.png`, `cua-block-break.png`, `tavern-transition.png`: actual 0.2.1 runtime and Computer Use input.
- `native-collider-present.json`, `native-collider-removed.json`: native engine queries, not synthetic collider fixtures.
- `castle-alignment.json`: both images 1280 × 720, depth valid, GL error zero, nonmultisampled attachment, approximately 0.000001 m camera discrepancy at capture.
- `shader-off.png`, `shader-on.png`, `shader-comparison.json`: live grading comparison.
- `lamp-off.png`, `lamp-on.png`, `lamp-off-again.png` and `tavern-lamp-*.png`: live native illumination and its quality limitations.

`block-placement.png` is an earlier failed attempt against coarse invisible terrain; it is not placement-success evidence. Use the `cua-block-*` pair for the successful clear-space test. GUI actions were performed through Computer Use. Backend probes read actual world state and issue native collision queries; they are distinguished from direct mouse/keyboard tests.

## Upgrade continuation

The shared-world acceptance standard is in `IMPLEMENTATION_PLAN.md`. Next establish native infantry, cavalry and projectile blocking against the proven collider; separately choose obstacle-aware navigation. Then implement reliable proxy block contacts and the complete soldier/pressure-plate/piston-door scenario. Native geometry import and an attacker/weapon/reach/obstruction combat contract remain substantive milestones. Paired save hooks must pass actual engine I/O checks before enabling them.

## Local files and recovery

- Launcher: `Play WarbandCraft 0.2.1.command` (also available through `Play WarbandCraft Next.command`).
- Mod: `source/minecraft/build/libs/warbandcraft-0.2.1.jar`.
- Native binaries: `validation/candidate/bin/`.
- Module: installed `WarbandCraft-Next`, reconstructed from the installed Native module by `source/install_module.py`.
- Prism instance: `~/Library/Application Support/PrismLauncher/instances/WarbandCraft-Next`.
- Computer Use runtime identity: `~/Library/Application Support/WarbandCraft/runtime/WarbandCraft Minecraft.app`.
- Recoverable original backups: `backups/20261012-192339/`; pre-patch Next backups: `backups/0.2.0-before-0.2.1/`.

The original launcher, original installed bridge module/mod/world and Steam executable remain intact. Test construction added for the castle and tavern checks was removed. This release is a local patch for this existing installation, not a portable bundle of either game's assets or accounts.
