#!/usr/bin/env python3
"""Validate module patching on a temporary fixture from an owned Native install."""
import hashlib
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def validate(path):
    parts = re.split(r'(?=^mst_)', path.read_text(), flags=re.M)[1:]
    assert len(parts) == 50, f'Expected 50 mission templates, found {len(parts)}'
    for part in parts:
        tokens = part.split()
        cursor = 6
        for _ in range(int(tokens[5])):
            cursor += 6 + int(tokens[cursor + 5])
        count = int(tokens[cursor]); cursor += 1
        for _ in range(count):
            cursor += 3
            for _ in range(2):
                operations = int(tokens[cursor]); cursor += 1; depth = 0
                for _ in range(operations):
                    opcode, argc = map(int, tokens[cursor:cursor+2])
                    basic = opcode & 0x3FFFFFFF
                    if basic in (4, 6, 7, 11, 12, 16, 17):
                        depth += 1
                    elif basic == 3:
                        depth -= 1
                        assert depth >= 0, f'Unmatched try_end in {tokens[0]}'
                    elif basic == 5:
                        assert depth > 0, f'Unmatched else_try in {tokens[0]}'
                    cursor += 2 + argc
                assert depth == 0, f'Unclosed operation block in {tokens[0]}'
        assert cursor == len(tokens), f'Trailing or truncated data in {tokens[0]}'


def main():
    native = Path(sys.argv[1]).expanduser().resolve()
    with tempfile.TemporaryDirectory(prefix='warbandcraft-module-') as temp:
        root = Path(temp); fixture = root / 'Native'; fixture.mkdir()
        for name in ['mission_templates.txt', 'troops.txt']:
            shutil.copy2(native / name, fixture / name)
        before = {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in fixture.iterdir()}
        subprocess.run([sys.executable, str(ROOT / 'source/install_module.py'), str(root)], check=True)
        patched = root / 'WarbandCraft'
        validate(patched / 'mission_templates.txt')
        assert int((patched / 'troops.txt').read_text().splitlines()[1]) == int((fixture / 'troops.txt').read_text().splitlines()[1]) + 1
        first = {p.name: p.read_bytes() for p in patched.iterdir()}
        subprocess.run([sys.executable, str(ROOT / 'source/install_module.py'), str(root)], check=True)
        assert first == {p.name: p.read_bytes() for p in patched.iterdir()}, 'Installer not idempotent'
        assert before == {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in fixture.iterdir()}, 'Native changed'
    print('PASS: 50 missions, balanced operations, troop count, idempotency and Native preservation')


if __name__ == '__main__':
    main()
