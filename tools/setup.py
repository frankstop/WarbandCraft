#!/usr/bin/env python3
"""Install only project-owned files; keep accounts, worlds and Native intact."""
import argparse
import configparser
import hashlib
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys
import urllib.request

ROOT = Path(__file__).resolve().parents[1]
API_NAME = 'fabric-api-0.116.17+1.21.1.jar'
API_URL = 'https://maven.fabricmc.net/net/fabricmc/fabric-api/fabric-api/0.116.17+1.21.1/' + API_NAME
API_SHA256 = '79ac44b40780acbd884b34c50be1e39af682847e5f5cb3b1fddeeaa768dce800'


def verified_api(destination):
    if destination.exists() and hashlib.sha256(destination.read_bytes()).hexdigest() == API_SHA256:
        return
    with urllib.request.urlopen(API_URL, timeout=60) as response:
        data = response.read()
    if hashlib.sha256(data).hexdigest() != API_SHA256:
        raise RuntimeError('Fabric API checksum mismatch; no instance files were replaced.')
    temporary = destination.with_suffix('.download')
    temporary.write_bytes(data)
    temporary.replace(destination)


def prepare_instance(prism_root, java_home, api_file):
    instance = prism_root / 'instances' / 'WarbandCraft'
    existing = instance.exists()
    if existing:
        pack = instance / 'mmc-pack.json'
        if not pack.exists():
            raise RuntimeError('An unrecognized WarbandCraft instance already exists; choose another PRISM_DIR.')
        versions = {c['uid']: c.get('version') for c in json.loads(pack.read_text())['components']}
        if versions.get('net.minecraft') != '1.21.1' or versions.get('net.fabricmc.fabric-loader') != '0.19.5':
            raise RuntimeError('The existing WarbandCraft instance has different game/loader versions; it was not changed.')
    else:
        instance.mkdir(parents=True)
        for name in ['instance.cfg', 'mmc-pack.json']:
            shutil.copy2(ROOT / 'prism' / name, instance / name)
    # Preserve Prism's directory convention and existing worlds.
    minecraft = instance / ('.minecraft' if (instance / '.minecraft').exists() else 'minecraft')
    mods = minecraft / 'mods'
    mods.mkdir(parents=True, exist_ok=True)
    if not existing:
        shutil.copy2(ROOT / 'prism/options.txt', minecraft / 'options.txt')
    config = configparser.ConfigParser(interpolation=None, strict=False)
    config.optionxform = str
    config.read(instance / 'instance.cfg')
    if not config.has_section('General'):
        config.add_section('General')
    config['General'].update({'OverrideJavaLocation': 'true', 'JavaPath': str(java_home / 'bin/java'),
                              'OverrideJavaArgs': 'true', 'JvmArgs': '-Dwarbandcraft.experimentalFrameCapture=true'})
    with (instance / 'instance.cfg').open('w') as file:
        config.write(file)
    shutil.copy2(ROOT / 'mods/warbandcraft-0.1.0.jar', mods / 'warbandcraft-0.1.0.jar')
    shutil.copy2(api_file, mods / API_NAME)
    return instance


def configure_window(config_file):
    if not config_file.exists():
        print('Start Warband once if needed; select Windowed / 1280x720 in Options.')
        return
    backup = config_file.with_name(config_file.name + '.warbandcraft-backup')
    if not backup.exists():
        shutil.copy2(config_file, backup)
    import re
    text = config_file.read_text()
    for key, value in [('start_windowed', 1), ('display_width', 1280), ('display_height', 720), ('max_framerate', 60)]:
        pattern = rf'(?m)^{key}\s*=.*$'
        replacement = f'{key} = {value}'
        text = re.sub(pattern, replacement, text) if re.search(pattern, text) else text + '\n' + replacement + '\n'
    config_file.write_text(text)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game-root', type=Path, default=Path(os.environ.get('WARBAND_DIR', str(Path.home() / 'Library/Application Support/Steam/steamapps/common/MountBlade Warband'))))
    parser.add_argument('--prism-root', type=Path, default=Path(os.environ.get('PRISM_DIR', str(Path.home() / 'Library/Application Support/PrismLauncher'))))
    args = parser.parse_args()
    if platform.system() != 'Darwin':
        raise RuntimeError('The native host supports macOS only.')
    game = args.game_root.expanduser().resolve()
    if not (game / 'Mount and Blade.app/Contents/MacOS/Mount and Blade').exists() or not (game / 'Modules/Native/mission_templates.txt').exists():
        raise RuntimeError('Native Steam Warband not found. Set WARBAND_DIR to the MountBlade Warband directory.')
    running = subprocess.run(['pgrep', '-f', '[M]ount and Blade.app/Contents/MacOS/Mount and Blade'], capture_output=True)
    if running.returncode == 0:
        raise RuntimeError('Close Warband before running Setup. Nothing was installed.')
    configured_java = os.environ.get('JAVA_HOME')
    java = Path(configured_java) if configured_java else Path(subprocess.check_output(['/usr/libexec/java_home', '-v', '21'], text=True).strip())
    java_version = subprocess.run([str(java / 'bin/java'), '-version'], capture_output=True, text=True, check=True)
    import re
    if not re.search(r'version "21(?:[.\"])', java_version.stderr + java_version.stdout):
        raise RuntimeError('Java 21 is required. Set JAVA_HOME to a Java 21 installation.')
    for artifact in ['bin/libwarbandcraft_host.dylib', 'bin/libwarbandcraft_core.dylib', 'mods/warbandcraft-0.1.0.jar']:
        if not (ROOT / artifact).exists():
            raise RuntimeError(f'{artifact} is missing; use a release or follow docs/development.md.')
    existing_module = game / 'Modules/WarbandCraft/troops.txt'
    if existing_module.exists() and 'trp_warbandcraft_mailbox' not in existing_module.read_text():
        raise RuntimeError('An unrelated module named WarbandCraft already exists; it was not changed.')
    cache = ROOT / 'backups/downloads'
    cache.mkdir(parents=True, exist_ok=True)
    api = cache / API_NAME
    verified_api(api)
    instance = prepare_instance(args.prism_root.expanduser().resolve(), java, api)
    subprocess.run([sys.executable, str(ROOT / 'source/install_module.py'), str(game / 'Modules')], check=True)
    configure_window(Path.home() / 'Library/Application Support/MBWarband/rgl_config.txt')
    print(f'Prism instance ready: {instance}')
    print('Setup complete. Open Play WarbandCraft.command, enter a 3D scene, then press F8.')


if __name__ == '__main__':
    try:
        main()
    except (RuntimeError, OSError, subprocess.SubprocessError, ValueError) as error:
        print(f'Setup failed: {error}', file=sys.stderr)
        sys.exit(1)
