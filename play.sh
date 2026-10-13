#!/bin/bash
set -euo pipefail
DIR="$(cd "$(dirname "$0")" && pwd)"
GAME="${WARBAND_DIR:-$HOME/Library/Application Support/Steam/steamapps/common/MountBlade Warband}"
PRISM="${PRISM_BIN:-$HOME/Downloads/Prism Launcher.app/Contents/MacOS/prismlauncher}"
if [ ! -x "$PRISM" ]; then
  PRISM="/Applications/Prism Launcher.app/Contents/MacOS/prismlauncher"
fi
if [ ! -x "$PRISM" ]; then
  echo 'Prism executable not found. Set PRISM_BIN to its executable path.'; exit 1
fi
if [ ! -f "$GAME/Modules/WarbandCraft/mission_templates.txt" ]; then
  python3 "$DIR/source/install_module.py" "$GAME/Modules"
fi
mkdir -p "$DIR/logs" /tmp/warbandcraft
open -g -a Steam
printf 'WarbandCraft' > "$HOME/Library/Application Support/MBWarband/last_module_warband"
echo 'WarbandCraft: native Warband + real Minecraft 1.21.1'
echo 'Opening Warband and Minecraft. Please wait for both games to load.'
echo 'E: inventory. WASD: move. Mouse: look. F8: Steve / Warband.'
if ! pgrep -f '[j]ava .*warbandcraft.experimentalFrameCapture' >/dev/null; then
  "$PRISM" --launch WarbandCraft >> "$DIR/logs/launcher.log" 2>&1 &
fi
if pgrep -f '/Contents/MacOS/Mount and Blade' >/dev/null; then
  echo 'Warband is already open. Enter a 3D scene and press F8 for Steve.'
  exit 0
fi
cd "$GAME"
export WBMC_CORE="$DIR/bin/libwarbandcraft_core.dylib"
export DYLD_INSERT_LIBRARIES="$DIR/bin/libwarbandcraft_host.dylib"
exec "$GAME/Mount and Blade.app/Contents/MacOS/Mount and Blade" "$@" 2>> "$DIR/logs/native.log"
