"""Write a unit-box collision body in the documented BRF manifold format.

Format reference: https://github.com/Swyter/openbrf-redux/blob/redux/brfBody.cpp
This asset has no visible mesh; Minecraft remains responsible for rendering it.
"""
import struct
from pathlib import Path

def box_resource(path):
    data = bytearray()
    def integer(n): data.extend(struct.pack('<i', n))
    def string(s):
        b = s.encode('ascii'); integer(len(b)); data.extend(b)
    string('rfver '); integer(1)
    string('body'); integer(1); string('bo_warbandcraft_cube'); string('manifold')
    vertices = [(0,0,0),(1,0,0),(1,1,0),(0,1,0),(0,0,1),(1,0,1),(1,1,1),(0,1,1)]
    integer(len(vertices))
    for p in vertices: data.extend(struct.pack('<3f', *p))
    faces = [(0,3,2,1),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)]
    integer(len(faces))
    for face in faces:
        integer(-1); integer(0); integer(len(face))
        for v in face: integer(v)
    string('end')
    path = Path(path); path.parent.mkdir(parents=True, exist_ok=True); path.write_bytes(data)
