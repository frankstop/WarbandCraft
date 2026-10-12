#!/usr/bin/env python3
"""Append bridge-only troop and mission triggers to a COPY of installed Native.

The troop's slots are a validated, versioned mailbox owned by this module.
All gameplay operations run in Warband's mission VM on its game thread.
"""
from pathlib import Path
import re, shutil, sys

root = Path(sys.argv[1])
native = root / 'Native'
dest = root / 'WarbandCraft'
if not dest.exists():
    shutil.copytree(native, dest)
troops = (native / 'troops.txt').read_text().splitlines()
tid = int(troops[1])
troops[1] = str(tid + 1)
last = max(i for i, line in enumerate(troops) if line.startswith('trp_'))
new = troops[last:].copy()
new[0] = 'trp_warbandcraft_mailbox WarbandCraft_Mailbox WarbandCraft_Mailbox 0 0 0 0 19 0 0'
(dest / 'troops.txt').write_text('\n'.join(troops + [''] + new) + '\n')
T = (5 << 56) | tid
OP = dict(try_begin=4, else_try=5, try_end=3, try_for_range=6, try_for_agents=12,
    eq=31, ge=30, gt=32, lt=2147483678, neq=2147483679,
    assign=2133, val_add=2105, val_sub=2106, val_mul=2107, val_div=2108,
    store_add=2120, store_sub=2121, store_mul=2122, store_div=2123,
    troop_set_slot=500, troop_get_slot=520, troop_slot_eq=540,
    set_fixed_point_multiplier=2124, get_player_agent_no=1700,
    agent_is_active=1712, agent_is_alive=1702, agent_is_human=1704,
    agent_get_position=1710, agent_set_position=1711, agent_get_team=1770,
    agent_get_troop_id=1718, store_agent_hit_points=1720,
    agent_set_visibility=2096, agent_set_no_dynamics=1762,
    agent_deliver_damage_to_agent=1722, team_give_order=1790,
    mission_cam_set_mode=2001, mission_cam_get_position=2010,
    mission_cam_set_position=2011, mission_cam_get_aperture=2013,
    mission_cam_set_aperture=2014, init_position=701,
    position_set_x=729, position_set_y=730, position_set_z=731,
    position_get_x=726, position_get_y=727, position_get_z=728,
    position_rotate_z_floating=734, position_rotate_x_floating=738,
    position_transform_position_to_parent=716, cast_ray=1900,
    store_trigger_param_1=2071, store_trigger_param_2=2072, store_trigger_param_3=2073,
    set_trigger_result=2075,
    store_current_scene=2211)
locals_ = {}
def v(x):
    if isinstance(x, str):
        if x not in locals_: locals_[x] = (17 << 56) | len(locals_)
        return locals_[x]
    return x
def o(name, *args): return (OP[name], *(v(a) for a in args))
def put(slot, val): return o('troop_set_slot', T, slot, val)
def get(var, slot): return o('troop_get_slot', var, T, slot)
def read_pos(pos, slot):
    out=[]
    for i, op in enumerate(['position_get_x','position_get_y','position_get_z']):
        out += [o(op, 'q', pos), put(slot+i, 'q')]
    return out
def set_pos(pos, slot):
    out=[o('init_position', pos)]
    for i, op in enumerate(['position_set_x','position_set_y','position_set_z']):
        out += [get('q', slot+i), o(op,pos,'q')]
    return out
def encode(ops):
    return str(len(ops)) + ' ' + ' '.join(str(x) for op in ops for x in (op[0], len(op)-1, *op[1:]))
def trigger(interval, ops): return f'{interval:.6f} 0.000000 0.000000 0 {encode(ops)}'

init = [put(65535,0), get('epoch',6),o('val_add','epoch',1),
    o('try_for_range','i',0,65536),put('i',0),o('try_end')]
init += [put(i,n) for i,n in enumerate([1463962947,12648430,305419896,1985229328,1,65536])]
init += [put(6,'epoch'),put(7,1),o('store_current_scene','q'),put(31,'q')]

# Native writes inputs only after a completed frame. A five-frame watchdog releases
# the camera/avatar even when the Minecraft process disappears.
frame = [o('set_fixed_point_multiplier',1000),get('seq',7),o('val_add','seq',1),put(7,'seq'),
    put(8,0),put(9,0),o('get_player_agent_no','player'),put(10,'player'),
    o('try_begin'),o('ge','player',0),o('agent_is_active','player'),o('agent_is_alive','player'),put(8,1),
    get('ttl',54),get('flags',40),o('try_begin'),o('gt','ttl',0),o('gt','flags',0),
    o('val_sub','ttl',1),put(54,'ttl'),o('mission_cam_set_mode',1,0,0)]
frame += set_pos(60,44)+[get('q',47),o('position_rotate_z_floating',60,'q'),get('q',48),
    o('position_rotate_x_floating',60,'q'),o('mission_cam_set_position',60),get('q',49),
    o('mission_cam_set_aperture','q'),put(9,1),get('q',42),put(55,'q')]
frame += set_pos(60,50)+[get('q',53),o('position_rotate_z_floating',60,'q'),
    o('agent_set_position','player',60),o('agent_set_visibility','player',0),
    o('agent_set_no_dynamics','player',0),
    o('else_try'),get('previous',57),o('eq','previous',1),o('mission_cam_set_mode',0,0,0),
    o('agent_set_visibility','player',1),o('agent_set_no_dynamics','player',1),o('try_end'),
    get('q',9),put(57,'q'),o('agent_get_position',60,'player')]
frame += read_pos(60,16)+[o('store_agent_hit_points','q','player',1),put(14,'q'),
    o('store_agent_hit_points','pct','player',0),o('val_mul','q',100),
    o('try_begin'),o('gt','pct',0),o('val_div','q','pct'),o('try_end'),put(15,'q'),o('try_end')]
frame += [o('mission_cam_get_position',60)]+read_pos(60,20)
for destslot, axis in [(23,'position_set_y'),(26,'position_set_z')]:
    frame += [o('init_position',61),o(axis,61,1000),o('position_transform_position_to_parent',62,60,61)]+read_pos(62,destslot)
frame += [o('mission_cam_get_aperture','q'),put(30,'q'),o('assign','count',0),
    o('try_for_agents','agent'),o('neq','agent','player'),o('agent_is_active','agent'),
    o('agent_is_alive','agent'),o('lt','count',1024),o('store_mul','base','count',8),o('val_add','base',256),
    o('agent_get_position',60,'agent')]
for i, op in enumerate(['position_get_x','position_get_y','position_get_z']):
    frame += [o(op,'q',60),o('store_add','slot','base',i),put('slot','q')]
for i, op, args in [(3,None,[]),(4,'store_agent_hit_points',['agent',1]),
    (5,'store_agent_hit_points',['agent',0]),(6,'agent_get_team',['agent']),(7,'agent_get_troop_id',['agent'])]:
    frame += ([o(op,'q',*args)] if op else [o('assign','q','agent')])+[o('store_add','slot','base',i),put('slot','q')]
frame += [o('val_add','count',1),o('try_end'),put(11,'count')]

# Actual scene collision, up to 256 rays per rendered frame. Native splits larger batches.
frame += [get('req',60),get('done',62),o('try_begin'),o('neq','req','done'),get('count',61),
    o('try_for_range','i',0,'count'),o('store_mul','base','i',12),o('val_add','base',16384),o('init_position',60)]
for i, op in enumerate(['position_set_x','position_set_y','position_set_z']):
    frame += [o('store_add','slot','base',i),get('q','slot'),o(op,60,'q')]
for i,op in [(3,'position_rotate_z_floating'),(4,'position_rotate_x_floating')]:
    frame += [o('store_add','slot','base',i),get('q','slot'),o(op,60,'q')]
frame += [o('store_add','slot','base',5),get('length','slot'),o('store_add','slot','base',6),put('slot',0),
    o('try_begin'),o('cast_ray','prop',61,60,'length'),put('slot',1)]
for i,op in enumerate(['position_get_x','position_get_y','position_get_z']):
    frame += [o(op,'q',61),o('store_add','slot','base',7+i),put('slot','q')]
frame += [o('try_end'),o('try_end'),put(62,'req'),o('try_end')]

frame += [get('count',10000),o('try_begin'),o('ge','player',0),o('agent_is_active','player'),
    o('try_for_range','i',0,'count'),o('store_mul','base','i',2),o('val_add','base',10001),
    get('victim','base'),o('val_add','base',1),get('damage','base'),o('agent_is_active','victim'),
    o('agent_is_alive','victim'),o('agent_deliver_damage_to_agent','player','victim','damage'),o('try_end'),o('try_end'),put(10000,0)]
frame += [get('death',66),o('try_begin'),o('eq','death',1),o('ge','player',0),o('agent_is_active','player'),
    put(9,0),put(40,0),put(54,0),o('agent_deliver_damage_to_agent','player','player',10000),o('try_end'),put(66,0)]
frame += [get('order',64),o('try_begin'),o('gt','order',0),get('division',65),o('ge','player',0),
    o('agent_get_team','team','player'),o('val_sub','order',1),o('team_give_order','team','division','order'),
    o('try_end'),put(64,0),o('set_fixed_point_multiplier',100)]

hit = [o('store_trigger_param_1','victim'),o('get_player_agent_no','player'),o('eq','victim','player'),
    o('troop_slot_eq',T,9,1),
    o('store_trigger_param_3','damage'),get('q',12),o('val_add','q',1),put(12,'q'),
    get('q',13),o('val_add','q','damage'),put(13,'q'),
    o('store_trigger_param_2','attacker'),o('try_begin'),o('ge','attacker',0),
    o('agent_is_active','attacker'),o('set_fixed_point_multiplier',1000),o('agent_get_position',60,'attacker')]
hit += read_pos(60,70)+[o('set_fixed_point_multiplier',100),o('try_end'),o('set_trigger_result',0)]

text=(native/'mission_templates.txt').read_text()
parts=re.split(r'(?=^mst_)',text,flags=re.M)
assert len(parts)==51
for i in range(1,len(parts)):
    tok=parts[i].split()
    spawns=int(tok[5]); cursor=6
    for _ in range(spawns):
        items=int(tok[cursor+5]); cursor+=6+items
    n=int(tok[cursor]); tok[cursor]=str(n+3); cursor+=1
    for _ in range(n):
        cursor+=3
        for block in range(2):
            count=int(tok[cursor]); cursor+=1
            for __ in range(count):
                argc=int(tok[cursor+1]); cursor+=2+argc
    assert cursor==len(tok), (i, cursor, len(tok))
    parts[i]=' '.join(tok[:4])+'\n'+tok[4]+'\n'+' '.join(tok[5:])+'\n'
    parts[i]+='\n'.join([trigger(-19,init),trigger(0,frame),trigger(-28,hit)])+'\n'
(dest/'mission_templates.txt').write_text(''.join(parts))
print(f'Installed WarbandCraft: {len(parts)-1} mission templates, reserved troop {tid}; Native untouched')
