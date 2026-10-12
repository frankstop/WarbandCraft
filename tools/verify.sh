#!/bin/bash
set -euo pipefail
DIR="$(cd "$(dirname "$0")/.." && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
clang -O2 "$DIR/tests/transport_test.c" "$DIR/source/native/transport.c" -I"$DIR/source/native" -lm -o "$TMP/transport_test"
"$TMP/transport_test"
python3 -m unittest discover -s "$DIR/tests" -p 'test_*.py'
python3 - "$DIR" <<'PY'
import ast,json,sys
from pathlib import Path
root=Path(sys.argv[1])
for folder in ['source','tools','tests']:
    for p in (root/folder).rglob('*.py'): ast.parse(p.read_text(),filename=str(p))
for p in root.rglob('*.json'):
    if '.git' not in p.parts and 'build' not in p.parts: json.loads(p.read_text())
print('PASS: Python syntax and JSON')
PY
for script in "$DIR"/*.sh "$DIR"/*.command "$DIR"/source/*.sh "$DIR"/tools/*.sh; do bash -n "$script"; done
echo 'PASS: shell syntax'
