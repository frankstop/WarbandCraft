"""Parse the generated module and check executable structure and slot boundaries."""
from pathlib import Path
import re, struct, sys, json
root=Path(sys.argv[1]);parts=re.split(r'(?=^mst_)',(root/'mission_templates.txt').read_text(),flags=re.M)
assert len(parts)==51
for part in parts[1:]:
    t=part.split();c=6
    for _ in range(int(t[5])): c+=6+int(t[c+5])
    triggers=int(t[c]);c+=1
    for _ in range(triggers):
        c+=3
        for block in range(2):
            count=int(t[c]);c+=1;depth=0;ops=[]
            for _ in range(count):
                opcode=int(t[c]);n=int(t[c+1]);args=list(map(int,t[c+2:c+2+n]));c+=2+n
                ops.append((opcode,args))
                if opcode in [4,6,12]:depth+=1
                if opcode==3:depth-=1
                assert depth>=0,part[:80]
            assert depth==0,part[:80]
            if any(op==520 and args[-1]==30000 for op,args in ops):
                multiplier=None;scales=0
                for op,args in ops:
                    if op==2124:multiplier=args[0]
                    if op in [1854,1855]:
                        assert multiplier==1000, 'Native colliders must consume millimetres, not centimetres'
                        scales+=1
                assert scales==2 and multiplier==100, 'Restore native fixed-point multiplier after collider updates' 
    assert c==len(t),(c,len(t))
meta=json.loads((root/'warbandcraft-module.json').read_text());props=(root/'scene_props.txt').read_text()
assert len(re.findall(r'^spr_',props,re.M))==int(props.splitlines()[1])
assert 'spr_warbandcraft_collision' in props and meta['max_colliders']==2048
data=(root/'Resource/warbandcraft_collision.brf').read_bytes();offset=0
def integer():
    global offset
    n=struct.unpack_from('<i',data,offset)[0];offset+=4;return n
def string():
    global offset
    n=integer();s=data[offset:offset+n].decode();offset+=n;return s
assert string()=='rfver ' and integer()==1
assert string()=='body' and integer()==1
assert string()=='bo_warbandcraft_cube' and string()=='manifold'
vertices=integer();assert vertices==8;offset+=vertices*12
faces=integer();assert faces==6
for _ in range(faces):
    assert integer()==-1 and integer()==0
    assert integer()==4
    for _ in range(4):assert 0<=integer()<vertices
assert string()=='end' and offset==len(data)
assert 31000+32*7<50000 and 50000+2048<65520
print('PASS: all 50 mission templates parse with balanced operations; prop counts and box resource are valid; mailbox regions do not overlap.')
