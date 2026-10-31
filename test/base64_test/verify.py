import sys, base64

ok = 0
ng = 0
for line in sys.stdin:
    line = line.strip()
    if not line:
        continue
    b64, hex_orig = line.split('\t')
    decoded = base64.b64decode(b64)
    expected = bytes.fromhex(hex_orig)
    if decoded == expected:
        print(f'[PASS] {b64:<32s} -> {decoded}')
        ok += 1
    else:
        print(f'[FAIL] {b64:<32s} -> {decoded} (expected {expected})')
        ng += 1

print(f'\nPython verification: {ok} passed, {ng} failed')
