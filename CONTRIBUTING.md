# Contributing

Please reproduce issues with the supported versions and the WarbandCraft module before proposing a fix. Include the scene type, control mode, window dimensions and whether the native composition flag is active. Remove account details and local personal paths from logs.

Keep host operations on Warband's mission thread, preserve shared-memory layouts unless both sides are updated, and retain camera watchdog/release behavior. Do not add an automatic Warband startup/menu bypass to the default launcher. Keep user saves and unrelated Prism instances intact.

Run `bash tools/verify.sh` and build the Fabric mod with Java 21. Changes to rendering, collision, combat or focus need a short in-game reproduction and observed result in the pull request. The native library requires the installed Steam macOS game frameworks to compile.

Useful next areas: native troop collision/pathfinding around Minecraft builds, generated-battlefield identity, scene transitions, mounted stand-ins, combat acceptance and terrain sampling during Elytra flight.
