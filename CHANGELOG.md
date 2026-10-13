# Changelog

## 0.2.1 — 2026-10-13

Experimental patch for an existing WarbandCraft installation, launched through the isolated WarbandCraft-Next module and Prism instance.

- Fix native framebuffer depth reuse that cut diagonal holes into Steve and blocks.
- Correct native collider coordinates and scale; confirm collider presence and removal with native rays.
- Close the Minecraft level connection before world teardown to allow scene/world transitions.
- Retain campaign identity across mission generations and add a macOS Minecraft application identity.
- Include the 0.2.0 construction mirroring, campaign checkpoint store, terrain sampling, color grading and experimental native lighting.
- Publish reproducible native/GPU/Java/module/Fabric checks and live evidence.

Troop collision/navigation, exact shared combat, full native geometry import and paired native save/load remain unaccepted. Native paired save hooks stay disabled by default. See [RELEASE_STATUS.md](RELEASE_STATUS.md) for the feature and acceptance matrix.

## 0.1.0 — 2026-10-12

Initial experimental native Mac release.

- Native Intel Warband loader and OpenGL compositor; real Minecraft 1.21.1 Fabric simulation.
- Dedicated mission-VM mailbox, agent position/health publication, collision rays, damage and army-order transport.
- F8 control switching, matching camera poses, Retina render-size correction, Minecraft inventory/hand/HUD and Steve skin.
- Dedicated Creative bridge world, starter kit, redstone, TNT and Elytra simulation.
- Portable setup template, working manual launcher, source, transport checks and validation record.

Warband menus remain manual. No automatic battle entry or tutorial bypass is included. Native troops do not collide with Minecraft blocks. See the validation document for remaining gameplay acceptance work.
