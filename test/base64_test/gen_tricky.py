#!/usr/bin/env python3
"""
問題が起きやすいパターンをbase64エンコードして stdout に出力する。
フォーマット: base64文字列\thex表現
C側の verify_decode で読み込んでデコード検証する。
"""
import base64, struct

cases = []

# '+' (index=62) が出やすいバイト列を狙う
# 6ビットグループで 0b111110 (62) になるパターン
cases.append((b'\xfb\xe7', "+/==に近いパターン1"))
cases.append((b'\xfb\xef\xbe', "全1近傍3byte"))
cases.append((b'\xff\xff\xff', "0xFF x3"))
cases.append((b'\xff\xff\xff\xff', "0xFF x4"))

# '/' (index=63) が出やすい
cases.append((b'\xff', "0xFF single"))
cases.append((b'\xff\x00', "0xFF 0x00"))
cases.append((b'\x00\xff', "0x00 0xFF"))

# パディング 0, 1, 2 の境界
for n in range(1, 16):
    data = bytes(range(n))
    cases.append((data, f"sequential {n} bytes"))

# 全256バイト値を一通り含む
cases.append((bytes(range(256)), "all 256 byte values"))

# 繰り返しパターン
cases.append((b'\xfb\xef\xbe\xfb\xef\xbe', "+/+/ repeat"))
cases.append((b'\x00' * 6, "zeros x6"))
cases.append((b'\xaa\xbb\xcc' * 4, "aabbcc x4"))

for data, label in cases:
    b64 = base64.b64encode(data).decode()
    hex_str = data.hex()
    print(f"{b64}\t{hex_str}\t{label}")
