#!/bin/bash
set -euo pipefail
PROJECT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$PROJECT"
bash source/build_native.sh
bash source/tests/run_native.sh
JAVA_ROOT="/Library/Java/JavaVirtualMachines/temurin-21.jdk/Contents/Home"
mkdir -p validation/java-tests
"$JAVA_ROOT/bin/javac" -d validation/java-tests \
 source/minecraft/src/main/java/dev/wbmc/bridge/CheckpointStore.java \
 source/minecraft/src/main/java/dev/wbmc/bridge/BoxMerge.java source/tests/CheckpointTest.java
"$JAVA_ROOT/bin/java" -ea -cp validation/java-tests CheckpointTest
GAME_MODULES="${WARBAND_DIR:-$HOME/Library/Application Support/Steam/steamapps/common/MountBlade Warband}/Modules"
python3 source/install_module.py "$GAME_MODULES" --output validation/module
python3 source/tests/validate_module.py validation/module
source/minecraft/gradlew -p source/minecraft build --offline
