#!/usr/bin/env python3
import os,sys,subprocess,tempfile,threading,signal
from pathlib import Path
runtime='/Library/Java/JavaVirtualMachines/temurin-21.jdk/Contents/Home/bin/java'
if 'org.prismlauncher.EntryPoint' not in sys.argv:
    os.execv(runtime,[runtime,*sys.argv[1:]])
app=Path.home()/'Library/Application Support/WarbandCraft/runtime/WarbandCraft Minecraft.app'
with tempfile.TemporaryDirectory(prefix='wbmc-java-') as folder:
    pipes=[Path(folder)/name for name in ['stdin','stdout','stderr']]
    for p in pipes:os.mkfifo(p,0o600)
    def pump(source,dest):
        try:
            with source() as src,dest() as dst:
                while True:
                    chunk=os.read(src.fileno(),4096)
                    if not chunk:break
                    os.write(dst.fileno(),chunk)
        except (BrokenPipeError,OSError):pass
    for p,stream,mode in [(pipes[0],sys.stdin.buffer,'wb'),(pipes[1],sys.stdout.buffer,'rb'),(pipes[2],sys.stderr.buffer,'rb')]:
        if mode=='wb':fn=lambda p=p,stream=stream:pump(lambda:stream,lambda:p.open('wb',buffering=0))
        else:fn=lambda p=p,stream=stream:pump(lambda:p.open('rb'),lambda:stream)
        threading.Thread(target=fn,daemon=True).start()
    args=sys.argv[1:]
    cp=args.index('-cp')+1;parts=args[cp].split(':');parts=[str(app.parent/'NewLaunch.jar') if p.endswith('/jars/NewLaunch.jar') else p for p in parts];args[cp]=str(app.parent/'launcher-classes')+':'+':'.join(parts)
    args[args.index('org.prismlauncher.EntryPoint')]='dev.wbmc.launcher.Launcher'
    def stop(signum,frame):
        try:os.kill(int((Path(folder)/'pid').read_text()),signal.SIGTERM)
        except (FileNotFoundError,ValueError,ProcessLookupError):pass
    signal.signal(signal.SIGTERM,stop);signal.signal(signal.SIGINT,stop)
    result=subprocess.run(['open','-n','-W','-g','-a',str(app),'--stdin','/dev/null','--stdout',str(pipes[1]),'--stderr',str(pipes[2]),'--args','-Duser.dir='+os.getcwd(),'-Dwarbandcraft.launchInput='+str(pipes[0]),'-Dwarbandcraft.launchPid='+str(Path(folder)/'pid'),*args])
    # Streams use OS descriptors so a blocked stdin reader cannot deadlock Python shutdown.
    status=result.returncode
    for pipe in [*pipes,Path(folder)/'pid']:
        try:pipe.unlink()
        except FileNotFoundError:pass
    try:os.rmdir(folder)
    except OSError:pass
    os._exit(status)
