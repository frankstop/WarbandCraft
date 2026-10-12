# Validation — 12 October 2026

Real Minecraft 1.21.1 / Fabric was running alongside native Steam macOS Warband 1.174. The native compositor received actual Minecraft pixels and depth at 1280 × 720; bridge flags were 79 (camera/player/window valid, camera overridden, native composition active).

546 samples during movement covered translation ranges of 12.60 m horizontally, 4.06 m vertically and 19.17 m on the other horizontal axis. The maximum difference between Warband’s rendered camera and the camera belonging to the displayed Minecraft frame was **0.000818 m**. Direction-vector error stayed below 0.000017. Both projections were 75° vertical. These measurements verify camera correspondence; subjective smoothness and block occlusion still require human acceptance.

An isolated Minecraft server-command test powered a real redstone lamp (lit=true) after toggling an adjacent redstone block. Real primed TNT reduced a test structure from 75 to 57 oak plank blocks. The temporary test structure was cleaned up. The player’s inventory and equipment were read from the actual Minecraft server, including blocks, tools, TNT, rockets and an equipped Elytra.

The native transport checks passed publication, heartbeat expiry, torn reads, invalid poses, damage queues, stale entity IDs, overflow/wrap, player hits, collision rays and incompatible mappings. Java and Intel native builds passed. All 50 patched Native mission templates were parsed and their operation nesting checked.

Native soldiers continued moving and fighting while Minecraft owned input; a 25-vs-25 custom battle completed. Native collision-ray responses and live agent health/positions were observed. End-to-end soldier combat, Survival damage, Elytra feel, and every scene type remain human acceptance items. Native troops do not yet collide with Minecraft blocks.
