"""Give the installed Minecraft runtime a native app identity for Computer Use.
Uses the existing Temurin runtime and Prism launcher; account data stays in Prism.
"""
from pathlib import Path
import plistlib,shutil,subprocess

def prepare(root, prism):
    java_home=Path('/Library/Java/JavaVirtualMachines/temurin-21.jdk/Contents/Home')
    runtime=Path.home()/'Library/Application Support/WarbandCraft/runtime'
    contents=runtime/'WarbandCraft Minecraft.app/Contents'
    (contents/'MacOS').mkdir(parents=True,exist_ok=True)
    shutil.copy2(java_home/'bin/java',contents/'MacOS/java')
    for name in ('lib','conf','legal','release'):
        link=contents/name
        if not link.exists():link.symlink_to(java_home/name)
    with (contents/'Info.plist').open('wb') as f:
        plistlib.dump(dict(CFBundleIdentifier='dev.wbmc.minecraft',CFBundleName='WarbandCraft Minecraft',
            CFBundleDisplayName='WarbandCraft Minecraft',CFBundleExecutable='java',CFBundlePackageType='APPL',
            CFBundleVersion='0.2.1',CFBundleShortVersionString='0.2.1',NSHighResolutionCapable=True),f)
    # The vendor signature seals the old embedded Info.plist; sign this local wrapper identity.
    subprocess.run(['codesign','--force','--sign','-',str(contents/'MacOS/java')],check=True)
    classes=runtime/'launcher-classes';classes.mkdir(exist_ok=True)
    subprocess.run([str(java_home/'bin/javac'),'-d',str(classes),str(root/'source/Launcher.java')],check=True)
    shutil.copy2(prism.parent/'jars/NewLaunch.jar',runtime/'NewLaunch.jar')
    subprocess.run(['/System/Library/Frameworks/CoreServices.framework/Frameworks/LaunchServices.framework/Support/lsregister','-f',str(contents.parent)],check=True)
    return root/'source/java_launcher.py'
