#!/bin/sh
set -eu

if [ "$#" -ne 1 ]; then
    echo "usage: $0 /path/to/nginx-source" >&2
    exit 2
fi

ROOT=$(cd "$(dirname "$0")/.." && pwd)
NGINX_SRC=$(cd "$1" && pwd)
WORK=$(mktemp -d /tmp/nnx-runner-smoke-XXXXXX)
LOG="$WORK/run-app.log"
RUNNER_PID=""

cleanup() {
    if [ -n "$RUNNER_PID" ] && kill -0 "$RUNNER_PID" 2>/dev/null; then
        kill "$RUNNER_PID" 2>/dev/null || true
        wait "$RUNNER_PID" 2>/dev/null || true
    fi
    rm -rf "$WORK"
}
trap cleanup EXIT INT TERM

"$ROOT/scripts/run-app.sh" "$NGINX_SRC" "$ROOT/examples/hello.c" 18082 >"$LOG" 2>&1 &
RUNNER_PID=$!

body=""
for i in $(seq 1 30); do
    if ! kill -0 "$RUNNER_PID" 2>/dev/null; then
        echo "nnx: run-app exited before becoming healthy" >&2
        wait "$RUNNER_PID" 2>/dev/null || true
        RUNNER_PID=""
        cat "$LOG" >&2 || true
        exit 1
    fi
    body=$(curl -sS --max-time 2 http://127.0.0.1:18082/ 2>/dev/null || true)
    if [ "$body" = "Hello, nnx!" ]; then
        break
    fi
    sleep 1
done

if [ "$body" != "Hello, nnx!" ]; then
    echo "nnx: public run-app smoke test failed" >&2
    cat "$LOG" >&2 || true
    exit 1
fi

kill "$RUNNER_PID" 2>/dev/null || true
wait "$RUNNER_PID" 2>/dev/null || true
RUNNER_PID=""

echo "nnx public runner smoke test: PASS"
