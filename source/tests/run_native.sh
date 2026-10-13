#!/bin/bash
set -euo pipefail
TASK_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
GAME="${WARBAND_DIR:-$HOME/Library/Application Support/Steam/steamapps/common/MountBlade Warband}"
RES="$GAME/Mount and Blade.app/Contents/Resources"
mkdir -p "$TASK_ROOT/../validation"
clang -arch x86_64 -O1 -Wno-deprecated-declarations \
 -I"$RES/SDL2.framework/Versions/A/Headers" -I"$RES/GLEW.framework/Versions/2.1.0/Headers" \
 "$TASK_ROOT/tests/compositor_gpu.c" "$TASK_ROOT/native/transport.c" \
 "$RES/SDL2.framework/Versions/A/SDL2" "$RES/GLEW.framework/Versions/2.1.0/GLEW" \
 -framework OpenGL -lobjc -Wl,-rpath,"$RES" -o "$TASK_ROOT/../validation/compositor_gpu"
"$TASK_ROOT/../validation/compositor_gpu"

clang -arch x86_64 -O1 -I"$RES/GLEW.framework/Versions/2.1.0/Headers" \
 "$TASK_ROOT/tests/shader_gpu.c" "$RES/GLEW.framework/Versions/2.1.0/GLEW" \
 -framework OpenGL -Wl,-rpath,"$RES" -o "$TASK_ROOT/../validation/shader_gpu"
cd "$TASK_ROOT/.."
"$TASK_ROOT/../validation/shader_gpu"
