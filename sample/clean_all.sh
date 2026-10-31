#!/bin/bash
# sample/ 配下の全サンプルを make clean するスクリプト

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

for dir in */; do
    dir="${dir%/}"
    if [ -f "$dir/Makefile" ]; then
        echo "=== make clean: $dir ==="
        (cd "$dir" && make clean) || echo "  [WARN] make clean failed in $dir"
    fi
done

echo "Done."
