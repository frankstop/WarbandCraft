# Validation — 12 October 2026

Real Minecraft 1.21.1 / Fabric was running alongside native Steam macOS Warband 1.174. The native compositor received actual Minecraft pixels and depth at 1280 × 720; bridge flags were 79 (camera/player/window valid, camera overridden, native composition active).

546 samples during movement covered translation ranges of 12.60 m horizontally, 4.06 m vertically and 19.17 m on the other horizontal axis. The maximum difference between Warband’s rendered camera and the camera belonging to the displayed Minecraft frame was **0.000818 m**. Direction-vector error stayed below 0.000017. Both projections were 75° vertical. These measurements verify camera correspondence; subjective smoothness and block occlusion still require human acceptance.

An isolated Minecraft server-command test powered a real redstone lamp (lit=true) after toggling an adjacent redstone block. Real primed TNT reduced a test structure from 75 to 57 oak plank blocks. The temporary test structure was cleaned up. The player’s inventory and equipment were read from the actual Minecraft server, including blocks, tools, TNT, rockets and an equipped Elytra.

The native transport checks passed publication, heartbeat expiry, torn reads, invalid poses, damage queues, stale entity IDs, overflow/wrap, player hits, collision rays and incompatible mappings. Java and Intel native builds passed. All 50 patched Native mission templates were parsed and their operation nesting checked.

Native soldiers continued moving and fighting while Minecraft owned input; a 25-vs-25 custom battle completed. Native collision-ray responses and live agent health/positions were observed. End-to-end soldier combat, Survival damage, Elytra feel, and every scene type remain human acceptance items. Native troops do not yet collide with Minecraft blocks.

## 0.2.1 patch validation — 13 October 2026

Direct Computer Use is now available for both Warband and the Next Minecraft runtime. The full feature/test matrix and evidence index are in RELEASE_STATUS.md. The automated run is `validation/automated-checks-0.2.1.log`.

The supplied diagonal-hole screenshots were reproduced as a depth-readback defect: the native perspective framebuffer's full-size depth attachment was replaced by a reduced post-processing attachment before presentation. Retaining and validating the perspective attachment fixes solid Steve and block rendering in the tested scenes. A live native stone wall hides the lower part of a far column while the near block is visible; the hand/HUD remain intact. Fresh 0.2.1 castle and tavern images and a third-person image confirm rendering after scene changes. Captured castle diagnostics report valid 1280 × 720 depth, no GL error, no multisampling, and a reported camera disagreement of 0.000001 m. Multisampled depth remains unsupported and is explicitly rejected.

A corrected multiplier of 1000 gives the native collision resource its expected size and position. For the cube at Minecraft (40960,120,0), the native engine returned hit type 2 at (-0.488,19.556,-14.252). Removing the cube made the same ray return hit type 0. This proves collision-query presence and removal, not troop blocking or AI navigation.

Mouse placement and breaking passed on a temporary clear-space platform: dirt count changed 64 → 61 with repeated placement and then 61 → 62 when a block was broken and collected. Test construction was removed and the modified inventory slots restored. An earlier interior placement attempt failed because the target was an invisible terrain voxel at the player's eye. Coarse interior geometry remains a known failure.

World teardown now closes the level connection before waiting for the integrated server. Live logs show a custom-battle world stopping/saving, then the existing isolated campaign world opening. Campaign save/load rollback remains unaccepted; native paired save hooks stay disabled.

The live custom shader changes a reference lime block pixel from RGB (52,93,14) to (44,94,0) while preserving native rendering and HUD. A powered lamp brightens native tavern surfaces, and removing power removes the effect. The lighting images also expose patchy illumination and rough screen-space shadows; it remains experimental.
