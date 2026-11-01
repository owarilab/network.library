#!/bin/bash
# test/ 配下の全テストを一括ビルド・実行するスクリプト
# 使い方: bash run_all.sh
#   各テストを make して実行し、pass/fail 集計行を表示する。
#   全テストが exit 0 なら最後に "ALL TESTS PASSED" を表示し exit 0、
#   1つでも失敗すると "SOME TESTS FAILED" を表示し exit 1。

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

TIMEOUT=60
total=0
failed=0
failed_dirs=""

for dir in */; do
    dir="${dir%/}"
    [ -f "$dir/Makefile" ] || continue

    program=$(grep -m1 '^PROGRAM' "$dir/Makefile" | sed 's/^[^=]*=[[:space:]]*//')
    [ -n "$program" ] || program="$dir"

    total=$((total + 1))
    echo "========================================"
    echo "=== $dir ($program) ==="

    if ! (cd "$dir" && make >/dev/null 2>&1); then
        echo "  [FAIL] build failed"
        failed=$((failed + 1))
        failed_dirs="$failed_dirs $dir"
        continue
    fi

    if (cd "$dir" && timeout "$TIMEOUT" ./"$program"); then
        echo "  [OK] $dir"
    else
        echo "  [FAIL] $dir (exit code $?)"
        failed=$((failed + 1))
        failed_dirs="$failed_dirs $dir"
    fi
done

echo "========================================"
echo "=== run_all summary ==="
echo "total: $total, failed: $failed"
if [ "$failed" -gt 0 ]; then
    echo "failed dirs:$failed_dirs"
    echo "SOME TESTS FAILED"
    exit 1
else
    echo "ALL TESTS PASSED"
    exit 0
fi
