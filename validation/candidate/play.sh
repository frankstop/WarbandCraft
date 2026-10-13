#!/bin/bash
set -euo pipefail
PROJECT="$(cd "$(dirname "$0")/../.." && pwd)"
GAME="${WARBAND_DIR:-$HOME/Library/Application Support/Steam/steamapps/common/MountBlade Warband}"
PRISM="${PRISM_BIN:-$HOME/Downloads/Prism Launcher.app/Contents/MacOS/prismlauncher}"
if [ ! -x "$PRISM" ]; then PRISM='/Applications/Prism Launcher.app/Contents/MacOS/prismlauncher'; fi
if [ ! -x "$PRISM" ]; then echo 'Prism Launcher is required.'; exit 1; fi
if pgrep -f '/Contents/MacOS/Mount and Blade|[j]ava .*warbandcraft.experimentalFrameCapture' >/dev/null; then
  echo 'Close the two bridge games before starting the candidate; they share one transport.'; exit 1
fi
python3 "$PROJECT/source/setup_next.py"
python3 "$PROJECT/source/install_module.py" "$GAME/Modules" --output "$GAME/Modules/WarbandCraft-Next"
mkdir -p "$PROJECT/validation/candidate/logs"
open -g -a Steam
printf 'WarbandCraft-Next' > "$HOME/Library/Application Support/MBWarband/last_module_warband"
"$PRISM" --launch WarbandCraft-Next >> "$PROJECT/validation/candidate/logs/minecraft.log" 2>&1 &
cd "$GAME"
export WBMC_MODULE=WarbandCraft-Next
export WBMC_CORE="$PROJECT/validation/candidate/bin/libwarbandcraft_core.dylib"
export DYLD_INSERT_LIBRARIES="$PROJECT/validation/candidate/bin/libwarbandcraft_host.dylib"
# Paired save hooks remain opt-in until the real game's save I/O is verified.
export WBMC_PAIRED_SAVES="${WBMC_PAIRED_SAVES:-0}"
exec "$GAME/Mount and Blade.app/Contents/MacOS/Mount and Blade" 2>> "$PROJECT/validation/candidate/logs/native.log"
