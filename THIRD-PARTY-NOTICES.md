# Third-party notices

## Included source and binaries

The Minecraft-side bridge derives from **justbustin's minecraft-crossover-bridge**, MIT licensed: https://github.com/justbustin/minecraft-crossover-bridge. Its full original license remains at `source/minecraft/UPSTREAM-LICENSE`; the root MIT license preserves its copyright. The native Warband adaptation and additional integration work are distributed under MIT.

The Gradle wrapper (`gradlew`, `gradlew.bat`, `gradle-wrapper.jar`) is from Gradle, licensed under Apache License 2.0: https://github.com/gradle/gradle. See `licenses/Apache-2.0.txt`. Gradle itself is downloaded separately by the wrapper.

The bundled dylibs are WarbandCraft's own builds. They dynamically link to the user's installed SDL2 and GLEW frameworks and system OpenGL/Objective-C libraries. Those frameworks are not redistributed.

## Downloaded or supplied separately

- Fabric API: Apache License 2.0, https://github.com/FabricMC/fabric. Setup downloads the pinned artifact from Fabric's Maven repository. A pinned SHA-256 verifies its bytes.
- Fabric Loader: Apache License 2.0, https://github.com/FabricMC/fabric-loader. Downloaded by Prism.
- Fabric Loom: MIT, https://github.com/FabricMC/fabric-loom. Used by Gradle during builds.
- Prism Launcher: GPL-3.0, https://github.com/PrismLauncher/PrismLauncher. Installed separately; no launcher executable or account database is bundled.
- Minecraft Java Edition and official mappings: obtained separately from Mojang through Prism/build tooling, under their applicable terms.
- Mount & Blade: Warband and its Native module: supplied by the user's Steam installation. The installer modifies a local copy; no TaleWorlds assets or module data files are distributed.
- SDL2: zlib license, https://github.com/libsdl-org/SDL. GLEW: its upstream licenses, https://github.com/nigels-com/glew. Resolved from Warband's installed application.

## Inspiration

SkyCraft (https://github.com/chasmlol/SkyCraft) and Minecraft-Ring (https://github.com/siddoff/Minecraft-Ring) are credited architectural and presentation references. Their host-game plugins and game assets are not bundled in WarbandCraft. Attribution does not imply identical feature support.
