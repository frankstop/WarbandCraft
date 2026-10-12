#!/bin/bash
set -euo pipefail
DIR="$(cd "$(dirname "$0")" && pwd)"
GAME="${WARBAND_DIR:-$HOME/Library/Application Support/Steam/steamapps/common/MountBlade Warband}"
RES="$GAME/Mount and Blade.app/Contents/Resources"
mkdir -p "$DIR/../bin"
clang -arch x86_64 -dynamiclib -O2 -install_name @rpath/libwarbandcraft_core.dylib -I"$RES/SDL2.framework/Versions/A/Headers" -I"$RES/GLEW.framework/Versions/2.1.0/Headers" \
 "$RES/SDL2.framework/Versions/A/SDL2" "$RES/GLEW.framework/Versions/2.1.0/GLEW" -framework OpenGL -lobjc \
 "$DIR/native/host_adapter.c" "$DIR/native/transport.c" -o "$DIR/../bin/libwarbandcraft_core.next.dylib"
mv "$DIR/../bin/libwarbandcraft_core.next.dylib" "$DIR/../bin/libwarbandcraft_core.dylib"
clang -arch x86_64 -dynamiclib -O2 -install_name @rpath/libwarbandcraft_host.dylib -I"$RES/SDL2.framework/Versions/A/Headers" "$RES/SDL2.framework/Versions/A/SDL2" \
 "$DIR/native/loader.c" -o "$DIR/../bin/libwarbandcraft_host.dylib"
