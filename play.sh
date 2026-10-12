#!/bin/bash
set -euo pipefail
DIR="$(cd "$(dirname "$0")" && pwd)"
GAME="${WARBAND_DIR:-$HOME/Library/Application Support/Steam/steamapps/common/MountBlade Warband}"
PRISM_DIR="${PRISM_DIR:-$HOME/Library/Application Support/PrismLauncher}"
PRISM="${PRISM_BIN:-}"
if [ -z "$PRISM" ]; then
  for candidate in '/Applications/Prism Launcher.app/Contents/MacOS/prismlauncher' "$HOME/Applications/Prism Launcher.app/Contents/MacOS/prismlauncher" "$HOME/Downloads/Prism Launcher.app/Contents/MacOS/prismlauncher"; do
    if [ -x "$candidate" ]; then PRISM="$candidate"; break; fi
  done
fi
if [ ! -x "$PRISM" ]; then echo 'Prism not found. Set PRISM_BIN to its executable path.'; exit 1; fi
if [ ! -x "$GAME/Mount and Blade.app/Contents/MacOS/Mount and Blade" ]; then echo 'Warband not found. Set WARBAND_DIR to its installation directory.'; exit 1; fi
if [ ! -f "$GAME/Modules/WarbandCraft/mission_templates.txt" ] || [ ! -f "$PRISM_DIR/instances/WarbandCraft/mmc-pack.json" ]; then
  echo 'Run Setup WarbandCraft.command once before playing.'; exit 1
fi
mkdir -p "$DIR/logs" /tmp/warbandcraft
open -g -a Steam
printf 'WarbandCraft' > "$HOME/Library/Application Support/MBWarband/last_module_warband"
echo 'Opening Warband + Minecraft. Wait for both games to load.'
echo 'Choose a battle or load a campaign. In a 3D scene, F8 / Fn+F8 switches to Steve.'
if ! pgrep -f '[j]ava .*warbandcraft.experimentalFrameCapture' >/dev/null; then
  "$PRISM" --dir "$PRISM_DIR" --launch WarbandCraft >> "$DIR/logs/launcher.log" 2>&1 &
fi
if pgrep -f '[M]ount and Blade.app/Contents/MacOS/Mount and Blade' >/dev/null; then
  echo 'Warband is already running. Return to it and press F8 in a playable scene.'
  exit 0
fi
cd "$GAME"
export WBMC_CORE="$DIR/bin/libwarbandcraft_core.dylib"
export DYLD_INSERT_LIBRARIES="$DIR/bin/libwarbandcraft_host.dylib"
exec "$GAME/Mount and Blade.app/Contents/MacOS/Mount and Blade" "$@" 2>> "$DIR/logs/native.log"
