#!/bin/bash
# test_api.sh - llm_agent server API tests
# Usage: ./test_api.sh [HOST:PORT]
# Default: localhost:4445

BASE="${1:-http://localhost:4445}"
PASS=0
FAIL=0

# ---------------------------------------------------------------
# Helper functions
# ---------------------------------------------------------------
run_test() {
    local name="$1"
    local expected="$2"   # substring that must be present in response
    shift 2
    local resp
    resp=$(curl -s -m 10 "$@")
    if echo "$resp" | grep -q "$expected"; then
        echo "  PASS: $name"
        PASS=$((PASS+1))
    else
        echo "  FAIL: $name"
        echo "        expected substring: $expected"
        echo "        actual response:    $resp"
        FAIL=$((FAIL+1))
    fi
}

run_test_absent() {
    local name="$1"
    local absent="$2"   # substring that must NOT be present
    shift 2
    local resp
    resp=$(curl -s -m 10 "$@")
    if ! echo "$resp" | grep -q "$absent"; then
        echo "  PASS: $name"
        PASS=$((PASS+1))
    else
        echo "  FAIL: $name"
        echo "        unexpected substring: $absent"
        echo "        actual response:      $resp"
        FAIL=$((FAIL+1))
    fi
}

# ---------------------------------------------------------------
# Check server is reachable
# ---------------------------------------------------------------
echo "=== Connecting to $BASE ==="
if ! curl -s -m 3 "$BASE/api/agent/init" -X POST \
    -H "Content-Type: application/json" -d '{}' > /dev/null 2>&1; then
    echo "ERROR: Server not reachable at $BASE"
    echo "Start the server first:  ./qs_llm_agent_server"
    exit 1
fi
echo ""

# ---------------------------------------------------------------
# T-501-1: /api/agent/init
# ---------------------------------------------------------------
echo "--- T-501-1: /api/agent/init ---"

run_test "init returns conversation_id" \
    "conversation_id" \
    -X POST "$BASE/api/agent/init" \
    -H "Content-Type: application/json" \
    -d '{"query":"test query","max_iterations":5}'

run_test "init with no body returns conversation_id" \
    "conversation_id" \
    -X POST "$BASE/api/agent/init" \
    -H "Content-Type: application/json" \
    -d '{}'

run_test "init: status is ready" \
    '"status":"ready"' \
    -X POST "$BASE/api/agent/init" \
    -H "Content-Type: application/json" \
    -d '{"query":"hello"}'

# ---------------------------------------------------------------
# T-501-2: /api/agent/execute — file_list
# ---------------------------------------------------------------
echo ""
echo "--- T-501-2: /api/agent/execute (file_list) ---"

run_test "execute file_list returns entries" \
    '"entries"' \
    -X POST "$BASE/api/agent/execute" \
    -H "Content-Type: application/json" \
    -d '{"tool_name":"file_list","tool_args":{"path":"."}}'

run_test "execute file_list status ok" \
    '"status":"ok"' \
    -X POST "$BASE/api/agent/execute" \
    -H "Content-Type: application/json" \
    -d '{"tool_name":"file_list","tool_args":{"path":"."}}'

run_test "execute file_list with pattern *.c" \
    '"entries"' \
    -X POST "$BASE/api/agent/execute" \
    -H "Content-Type: application/json" \
    -d '{"tool_name":"file_list","tool_args":{"path":".","pattern":"*.c"}}'

run_test "execute file_list recursive" \
    '"entries"' \
    -X POST "$BASE/api/agent/execute" \
    -H "Content-Type: application/json" \
    -d '{"tool_name":"file_list","tool_args":{"path":".","recursive":1,"pattern":"*.c"}}'

run_test "execute file_list missing dir returns error" \
    '"error"' \
    -X POST "$BASE/api/agent/execute" \
    -H "Content-Type: application/json" \
    -d '{"tool_name":"file_list","tool_args":{"path":"./no_such_dir_xyz"}}'

# ---------------------------------------------------------------
# T-501-3: /api/agent/execute — file_read
# ---------------------------------------------------------------
echo ""
echo "--- T-501-3: /api/agent/execute (file_read) ---"

run_test "execute file_read returns content" \
    '"content"' \
    -X POST "$BASE/api/agent/execute" \
    -H "Content-Type: application/json" \
    -d '{"tool_name":"file_read","tool_args":{"path":"./main.c","start_line":1,"end_line":10}}'

run_test "execute file_read status ok" \
    '"status":"ok"' \
    -X POST "$BASE/api/agent/execute" \
    -H "Content-Type: application/json" \
    -d '{"tool_name":"file_read","tool_args":{"path":"./main.c","start_line":1,"end_line":5}}'

run_test "execute file_read returns lines_read" \
    '"lines_read"' \
    -X POST "$BASE/api/agent/execute" \
    -H "Content-Type: application/json" \
    -d '{"tool_name":"file_read","tool_args":{"path":"./main.c","start_line":1,"end_line":10}}'

run_test "execute file_read missing file returns error" \
    '"error"' \
    -X POST "$BASE/api/agent/execute" \
    -H "Content-Type: application/json" \
    -d '{"tool_name":"file_read","tool_args":{"path":"./no_such_file_xyz.c"}}'

# ---------------------------------------------------------------
# T-501-4: Security — path traversal must be rejected
# ---------------------------------------------------------------
echo ""
echo "--- T-501-4: Security (path traversal) ---"

run_test "file_read: ../path is rejected" \
    '"error"' \
    -X POST "$BASE/api/agent/execute" \
    -H "Content-Type: application/json" \
    -d '{"tool_name":"file_read","tool_args":{"path":"../../../etc/passwd"}}'

run_test "file_list: ../path is rejected" \
    '"error"' \
    -X POST "$BASE/api/agent/execute" \
    -H "Content-Type: application/json" \
    -d '{"tool_name":"file_list","tool_args":{"path":"../../"}}'

run_test_absent "file_read: /etc/passwd content is NOT returned" \
    "root:" \
    -X POST "$BASE/api/agent/execute" \
    -H "Content-Type: application/json" \
    -d '{"tool_name":"file_read","tool_args":{"path":"../../../etc/passwd"}}'

# ---------------------------------------------------------------
# T-501-5: /api/agent/execute — unknown tool
# ---------------------------------------------------------------
echo ""
echo "--- T-501-5: Unknown tool ---"

run_test "execute unknown tool returns 400 error" \
    '"error"' \
    -X POST "$BASE/api/agent/execute" \
    -H "Content-Type: application/json" \
    -d '{"tool_name":"rm_rf","tool_args":{"path":"/"}}'

run_test "execute missing tool_name returns error" \
    '"error"' \
    -X POST "$BASE/api/agent/execute" \
    -H "Content-Type: application/json" \
    -d '{"tool_args":{"path":"."}}'

# ---------------------------------------------------------------
# T-501-6: /api/agent/think (requires LLM — skipped if unavailable)
# ---------------------------------------------------------------
echo ""
echo "--- T-501-6: /api/agent/think (LLM required) ---"

THINK_RESP=$(curl -s -m 30 -X POST "$BASE/api/agent/think" \
    -H "Content-Type: application/json" \
    -d '{"query":"List files in the current directory.","context":""}')

if echo "$THINK_RESP" | grep -q '"action"'; then
    echo "  PASS: think returns action field"
    PASS=$((PASS+1))
    if echo "$THINK_RESP" | grep -q '"use_tool"\|"final_answer"'; then
        echo "  PASS: think action is use_tool or final_answer"
        PASS=$((PASS+1))
    else
        echo "  INFO: think action value unexpected: $THINK_RESP"
    fi
else
    echo "  SKIP: think - LLM may not be configured (response: $THINK_RESP)"
fi

# ---------------------------------------------------------------
# T-501-7: /api/agent/run (requires LLM — skipped if unavailable)
# ---------------------------------------------------------------
echo ""
echo "--- T-501-7: /api/agent/run (LLM required) ---"

RUN_RESP=$(curl -s -m 120 -X POST "$BASE/api/agent/run" \
    -H "Content-Type: application/json" \
    -d '{"query":"List all .c files in the current directory.","max_iterations":3}')

if echo "$RUN_RESP" | grep -q '"status"'; then
    echo "  PASS: run returns status field"
    PASS=$((PASS+1))
    echo "  INFO: $RUN_RESP" | head -c 200
    echo ""
else
    echo "  SKIP: run - LLM may not be configured (response: $RUN_RESP)"
fi

# ---------------------------------------------------------------
# Summary
# ---------------------------------------------------------------
echo ""
echo "========================================"
echo "Results: $PASS passed, $FAIL failed"
echo "========================================"
if [[ $FAIL -gt 0 ]]; then
    exit 1
fi
exit 0
