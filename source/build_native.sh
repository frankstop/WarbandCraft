#!/bin/bash
set -euo pipefail
DIR="$(cd "$(dirname "$0")" && pwd)"
GAME="${WARBAND_DIR:-$HOME/Library/Application Support/Steam/steamapps/common/MountBlade Warband}"
RES="$GAME/Mount and Blade.app/Contents/Resources"
OUT="${WBMC_BUILD_OUT:-$DIR/../validation/candidate/bin}"
mkdir -p "$OUT"
clang -arch x86_64 -dynamiclib -O2 -I"$RES/SDL2.framework/Versions/A/Headers" -I"$RES/GLEW.framework/Versions/2.1.0/Headers" \
 "$RES/SDL2.framework/Versions/A/SDL2" "$RES/GLEW.framework/Versions/2.1.0/GLEW" -framework OpenGL -lobjc \
 "$DIR/native/host_adapter.c" "$DIR/native/transport.c" -o "$OUT/libwarbandcraft_core.next.dylib"
mv "$OUT/libwarbandcraft_core.next.dylib" "$OUT/libwarbandcraft_core.dylib"
clang -arch x86_64 -dynamiclib -O2 -I"$RES/SDL2.framework/Versions/A/Headers" "$RES/SDL2.framework/Versions/A/SDL2" \
 "$DIR/native/loader.c" -o "$OUT/libwarbandcraft_host.dylib"
