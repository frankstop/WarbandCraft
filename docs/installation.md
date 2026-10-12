# Installation

## Tested requirements

- Apple Silicon Mac running macOS 26.6.2, with Rosetta and Steam's native macOS Mount & Blade: Warband **1.174** installed. Other macOS releases and Intel Macs have not been tested. No CrossOver or WSE2 is used.
- [Prism Launcher](https://prismlauncher.org/download/) installed separately; a signed-in Microsoft account that owns Minecraft Java Edition.
- [Java 21](https://adoptium.net/temurin/releases/?version=21) and Python 3. Native source builds additionally need Xcode Command Line Tools.
- Enough memory and graphics capacity to run both games. The profile sets Minecraft's maximum heap to 4 GB.

## First installation

Extract the prerelease archive into a permanent folder. Do not launch it from inside the ZIP. Close Warband and open **Setup WarbandCraft.command**. It:

1. Checks the native Steam installation and Java 21.
2. Downloads the pinned Fabric API JAR from Fabric's Maven repository and verifies its SHA-256.
3. Creates a separate **WarbandCraft** Prism profile, or updates the project's existing profile without replacing its worlds or account data.
4. Copies installed Native to **Modules/WarbandCraft** and adds the mailbox troop and mission triggers. Installed Native is preserved.
5. Sets Warband to windowed 1280 × 720 / 60 FPS, preserving the original `rgl_config.txt` as `rgl_config.txt.warbandcraft-backup`.

Prism downloads Minecraft and Fabric Loader on first launch. You may need to finish Prism's account selection/sign-in and Java setup. No credentials come with this project.

## Opening the game — every time

1. Double-click **Play WarbandCraft.command**. It starts Steam, launches the dedicated Prism profile and opens native Warband with the host library injected.
2. Wait for loading to finish. Choose **Custom Battle → Start**, or **Load Game**, select a character and enter a 3D scene.
3. Press **F8**, or **Fn + F8**, to switch to Steve.

Keep the launcher's Terminal window open. A normal Steam launch does not inject the bridge. This release deliberately leaves Warband's menus and campaign loading manual.

## Custom installation locations

Run setup and play from Terminal with these environment variables if needed:

```bash
export WARBAND_DIR="/Volumes/Games/steamapps/common/MountBlade Warband"
export PRISM_DIR="$HOME/Library/Application Support/PrismLauncher"
export PRISM_BIN="/Applications/Prism Launcher.app/Contents/MacOS/prismlauncher"
export JAVA_HOME="$(/usr/libexec/java_home -v 21)"
bash "Setup WarbandCraft.command"
bash "Play WarbandCraft.command"
```

`WARBAND_DIR` contains `Mount and Blade.app` and `Modules`, not the application bundle itself. Use the same `PRISM_DIR` during setup and launch. The Prism template contains no game client, accounts, saves or launcher executable.

## Troubleshooting

| Symptom | Check |
|---|---|
| Warband is at a menu | This is expected. Start a battle or load a save and enter a 3D scene. |
| F8 does nothing | Try Fn + F8; start through the command; verify Minecraft's WarbandCraft instance finished loading. |
| Minecraft doesn't launch | Inspect `logs/launcher.log`; complete Prism's account selection; check Java 21 and instance name. |
| Missing bridge / wrong module | Select WarbandCraft, not another module; restart through the command after closing a normally launched game. |
| Blocks drift or cover another app | Use 1280 × 720 windowed play; F9 should show native composition. Switch to Warband with F8 and report the scene, dimensions and logs. |
| Game is slow or appears frozen on startup | Texture loading may take a minute or longer on the tested installation; wait for the normal main menu. |
| Setup refuses an existing instance/module | Its name collides with another installation or its versions differ. It is intentionally not overwritten. |
| macOS refuses a downloaded command | Review the source and use Terminal to run the setup/play scripts; managed-machine execution policies may require local IT support. |

## Data and uninstall

Warband campaign saves stay in macOS's MBWarband save directory. Minecraft saves stay in Prism's `instances/WarbandCraft/minecraft/saves` (or `.minecraft/saves`, depending on Prism's convention). Keep copies before experimenting with new builds.

To stop using the integration, close both games and start Steam Warband with the Native module. Remove only the separate WarbandCraft module/profile if you no longer want them, after preserving their saves. Restore window preferences from the configuration backup if desired. The Steam executable is not patched.
