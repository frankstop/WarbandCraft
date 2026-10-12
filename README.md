# WarbandCraft

**Real Minecraft inside native macOS Mount & Blade: Warband.**

[![Experimental](https://img.shields.io/badge/status-experimental-orange)](docs/validation.md)
[![Minecraft 1.21.1](https://img.shields.io/badge/Minecraft-1.21.1-green)](docs/installation.md)
[![MIT](https://img.shields.io/badge/license-MIT-blue)](LICENSE)

Warband runs its world, soldiers and battles. Minecraft Java Edition runs alongside it and supplies the player, inventory, crafting, blocks, redstone, TNT and Elytra. A native OpenGL bridge draws Minecraft's captured world, hand and HUD inside Warband's frame.

This is an early, locally tested prototype. Camera alignment, native composition, live soldiers, terrain queries, redstone and TNT have been checked. Combat, Elytra, terrain transitions and scene compatibility need more gameplay testing. **Warband soldiers do not yet collide with Minecraft blocks.** See [validation and known limits](docs/validation.md).

## Start playing

You need your own copies of **Warband** and **Minecraft: Java Edition**. The tested setup is **Apple Silicon macOS**, Steam's native **Warband 1.174** under Rosetta, **Prism Launcher**, **Java 21**, Minecraft **1.21.1**, Fabric Loader **0.19.5** and Fabric API **0.116.17+1.21.1**.

1. Download and extract the [latest prerelease](https://github.com/frankstop/WarbandCraft/releases/tag/v0.1.0), or clone this repository.
2. Install Prism Launcher and Java 21, and sign into Prism with an account that owns Minecraft Java Edition.
3. With Warband closed, run **Setup WarbandCraft.command** once. It creates a separate Prism instance and a separate Warband module from your installed Native module. It downloads Fabric API; Prism downloads Minecraft and Fabric Loader.
4. Double-click **Play WarbandCraft.command**. Leave its Terminal window open while playing.
5. In Warband, choose **Custom Battle → Start**, or load a campaign and enter a playable 3D scene.
6. Press **F8** (or **Fn + F8**) to control Steve. Press it again to return to Warband.

**The launcher opens both games. You still select a battle or load a campaign yourself.** Minecraft opens its bridge world automatically. No tutorial skip or automatic Warband menu bypass is installed.

[Detailed installation and troubleshooting](docs/installation.md) · [Development](docs/development.md) · [Architecture](docs/architecture.md) · [Report a bug](https://github.com/frankstop/WarbandCraft/issues/new/choose)

## Controls

| Input | Action |
|---|---|
| F8 / Fn + F8 | Switch Steve / Warband control |
| WASD / mouse / Space / Shift | Move / look / jump / sneak |
| E | Minecraft inventory |
| Left / right click | Break or attack / place or use |
| 1–9 / wheel | Select hotbar slot |
| F5 | Minecraft first-person / third-person camera |
| T | Minecraft chat and commands |
| F9 | Bridge diagnostics |
| Alt + 1 / 2 / 3 / 0 | Infantry / archers / cavalry / everyone |
| Alt + F1 / F2 / F3 / F4 / F5 | Hold / follow / charge / hold fire / advance |

The dedicated bridge world starts in Creative. Enter `/gamemode survival` for normal crafting and damage, or `/gamemode creative` for free building. An empty inventory receives a starter kit: blocks, crafting table, redstone, TNT, weapons, an equipped Elytra and rockets. Elytra uses Minecraft's normal flight controls: press Space while falling, then use a rocket.

Switching to another application hides Minecraft's input window and releases camera control. Resume in Warband with F8.

## How it works

A Fabric mod and an Intel macOS loader communicate through local shared-memory files. A dedicated troop's slots form a mailbox for Warband's mission VM. The native bridge validates the mailbox allocation; mission operations run on Warband's game thread. They read agent positions and health, set the camera/avatar, perform collision rays and deliver damage.

Minecraft simulates the player and blocks, captures color/depth/HUD and publishes the corresponding camera pose. Warband applies that pose and combines the matching frame with its own OpenGL depth buffer. The bridge checks the rendered camera against the captured pose to avoid composing mismatched frames. A watchdog releases camera control if Minecraft stops responding.

All 50 installed Native mission templates are instrumented. That is implementation coverage, **not proof that every scene works**.

## Known limits

- Native soldiers pass through Minecraft blocks; host hitboxes are approximate.
- Host terrain is sampled into invisible Minecraft collision blocks. Narrow passages, slopes and small props can be coarse. Warband terrain cannot be mined.
- Builds persist by scene ID. Generated battlefields that reuse an ID can share builds.
- Combat and death forwarding are implemented but need broader end-to-end gameplay validation. Armor, shields, projectiles and mounted play may expose mismatches.
- Elytra runs Minecraft's simulation; flight feel and fast terrain sampling still need acceptance testing.
- Windowed play at **1280 × 720** is the supported default. Frame slots support up to 1920 × 1200. Fullscreen, other Warband modules, multiplayer and Intel Macs are unverified.
- Minecraft keep-inventory and immediate respawn are enabled in the test world. Warband owns battle defeat and campaign consequences.

## Source layout

```text
source/native/       Native loader, transport, camera and OpenGL compositor
source/minecraft/    Minecraft 1.21.1 Fabric mod and Gradle wrapper
source/              Native build and Warband module installer
bin/                 Intel macOS bridge binaries
mods/                Built WarbandCraft Fabric mod
prism/               Account-free Prism instance template
tools/               Setup and verification helpers
tests/               Native ABI/transport regression checks
docs/                Installation, architecture, validation and development
```

## Credits

- [minecraft-crossover-bridge](https://github.com/justbustin/minecraft-crossover-bridge) by **justbustin** is the Minecraft-side code foundation and shared-memory/frame-transport reference.
- [SkyCraft](https://github.com/chasmlol/SkyCraft) by **chasmlol** inspired keeping the host game authoritative while drawing real Minecraft inside its frame.
- [Minecraft-Ring](https://github.com/siddoff/Minecraft-Ring) by **siddoff** inspired the host/Fabric separation, setup documentation and candid feature/limitation presentation.
- WarbandCraft's native Mac Warband loader, mission mailbox, collision queries and compositor integration were developed for this project.
- [Fabric](https://fabricmc.net/), [Prism Launcher](https://prismlauncher.org/) and the Warband Module System make the integration possible.

[MIT License](LICENSE) · [Third-party notices](THIRD-PARTY-NOTICES.md)

This is an unofficial fan project, unaffiliated with Mojang, Microsoft or TaleWorlds. Minecraft, Warband, their assets, saved games and account credentials are not distributed.
