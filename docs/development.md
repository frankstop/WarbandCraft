# Development

The current source targets 0.2.1. The default native output is `validation/candidate/bin/`; the isolated launcher is **Play WarbandCraft 0.2.1.command**. Historical binaries under `bin/` and `mods/` support the older launcher.

## Build the Fabric mod

Use Java 21:

```bash
export JAVA_HOME="$(/usr/libexec/java_home -v 21)"
cd source/minecraft
./gradlew build
```

The result is `source/minecraft/build/libs/warbandcraft-0.2.1.jar`. Copy it into `mods/` for packaging or into the dedicated Prism instance's `minecraft/mods/` for testing. Stop Minecraft before replacing a JAR. Do not copy a developer's worlds, account database or Prism caches into a release.

## Build the native host

The Steam macOS application provides the SDL2 and GLEW frameworks/headers used by the adapter. Xcode Command Line Tools are required.

```bash
bash source/build_native.sh
# Custom Steam library:
WARBAND_DIR="/Volumes/Games/steamapps/common/MountBlade Warband" bash source/build_native.sh
```

Both libraries are Intel x86_64 because native macOS Warband is Intel, even when Java runs as ARM64. The frameworks are linked from their concrete version directories; their bundle-root symlink representations may not work with `-framework SDL2`. Dylib IDs use `@rpath` and contain no developer-specific installation path.

## Verification

```bash
bash tools/verify.sh
bash source/tests/run_all.sh
python3 tools/verify_module.py \
  "$HOME/Library/Application Support/Steam/steamapps/common/MountBlade Warband/Modules/Native"
```

The first command runs the C ABI/transport tests, setup preservation/version-collision tests, Python syntax checks, shell parsing and JSON checks. The second copies only the user's Native troop/mission text into a temporary fixture, runs the installer twice and validates all operation counts/nesting, the appended troop count, idempotency and preservation of input files. It does not install into the live game. Native data remains local and is not included in fixtures or CI artifacts.

GitHub Actions runs portable checks and the Fabric build. The complete native host cannot be compiled on a hosted runner without the user's Warband framework files, which are not redistributed. Passing CI does not validate live camera, collision or combat behavior.

## In-game acceptance

Use a disposable custom battle and the project's dedicated world. Verify stationary blocks during walking/looking/jumping, soldier simulation while Minecraft owns input, foreground application switching, crafting, redstone, TNT, combat and Elytra. Try campaign scene transitions separately. Record observations and limits; implementation alone is not a passing gameplay test.

Native log: `logs/native.log`. Prism log: `instances/WarbandCraft/minecraft/logs/latest.log`. F9 reports bridge status. Composition uses flag bit 6; the observed active-state flags were 79.

The development helper can read commands from `/tmp/warbandcraft/mc_cmd.txt` and exports a read-only server probe to `/tmp/warbandcraft/probe.json`. `probe`, `status`, `save`, `blocks x y z r` and `run <Minecraft server command>` support diagnostics. It is a development facility for the dedicated local test world, not a network API.

## Packaging

The current downloadable patch is `releases/WarbandCraft-0.2.1-patch.zip`, with the file manifest at `releases/0.2.1-manifest.json`. Keep its layout intact: the Next launcher needs the candidate play script, candidate native libraries and built 0.2.1 mod. The historical archive example below describes the 0.1.0 portable package.

Keep release files limited to repository-owned source, scripts, docs, the WarbandCraft JAR and its two native dylibs. Generate an archive from tracked files:

```bash
git archive --format=zip --prefix=WarbandCraft/ -o WarbandCraft-v0.1.0-macos.zip v0.1.0
shasum -a 256 WarbandCraft-v0.1.0-macos.zip
```

Do not distribute patched Native modules, Steam binaries/frameworks, Minecraft client JARs, accounts, savegames, recordings or local logs. Setup reconstructs the module from the user's installation.
