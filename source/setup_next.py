"""Prepare an isolated Prism instance using the existing installed dependencies.
No legacy worlds or account files are copied. The normal launcher stays intact.
"""
from pathlib import Path
import configparser, shutil, uuid, json, os
from setup_minecraft_app import prepare

root=Path(__file__).resolve().parent.parent
instances=Path.home()/'Library/Application Support/PrismLauncher/instances'
base=instances/'WarbandCraft';dest=instances/'WarbandCraft-Next'
if not base.exists():raise SystemExit('The existing WarbandCraft Prism instance is required.')
dest.mkdir(parents=True,exist_ok=True)
if not (dest/'instance.cfg').exists():
    cfg=configparser.ConfigParser(interpolation=None);cfg.optionxform=str;cfg.read(base/'instance.cfg')
    cfg['General']['name']='WarbandCraft-Next';cfg['General']['uuid']=uuid.uuid4().hex
    cfg['General']['JvmArgs']='"-Dwarbandcraft.experimentalFrameCapture=true -Dwarbandcraft.module=WarbandCraft-Next"'
    for key in ['lastLaunchTime','lastTimePlayed','totalTimePlayed']:cfg['General'][key]='0'
    with (dest/'instance.cfg').open('w') as file:cfg.write(file)
    shutil.copy2(base/'mmc-pack.json',dest/'mmc-pack.json')
    for name in ['config','options.txt']:
        src=base/'minecraft'/name;target=dest/'minecraft'/name
        if src.is_dir():shutil.copytree(src,target,dirs_exist_ok=True)
        elif src.is_file():target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(src,target)
mods=dest/'minecraft/mods';mods.mkdir(parents=True,exist_ok=True)
for p in (base/'minecraft/mods').glob('*.jar'):
    if not p.name.startswith('warbandcraft'):shutil.copy2(p,mods/p.name)
candidate=root/'source/minecraft/build/libs/warbandcraft-0.2.1.jar'
if not candidate.exists():raise SystemExit('Build 0.2.1 first.')
backup=root/'backups/0.2.0-before-0.2.1/next-mods';backup.mkdir(parents=True,exist_ok=True)
for old in mods.glob('warbandcraft-*.jar'):
    if old.name!=candidate.name:shutil.move(str(old),str(backup/old.name))
shutil.copy2(candidate,mods/candidate.name)
prism=Path(os.environ.get('PRISM_BIN',str(Path.home()/'Downloads/Prism Launcher.app/Contents/MacOS/prismlauncher')))
if not prism.exists():prism=Path('/Applications/Prism Launcher.app/Contents/MacOS/prismlauncher')
launcher=prepare(root,prism)
cfg=configparser.ConfigParser(interpolation=None);cfg.optionxform=str;cfg.read(dest/'instance.cfg')
cfg['General']['AutomaticJava']='false';cfg['General']['OverrideJavaLocation']='true';cfg['General']['JavaPath']=str(launcher)
with (dest/'instance.cfg').open('w') as file:cfg.write(file)
print('Prepared isolated instance:',dest)
