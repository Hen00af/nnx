#!/bin/sh
set -eu

if [ "$#" -ne 1 ]; then
    echo "usage: $0 /path/to/nginx-source" >&2
    exit 2
fi

ROOT=$(cd "$(dirname "$0")/.." && pwd)
NGINX_SRC=$(cd "$1" && pwd)
MODULE=$("$ROOT/scripts/build-app-module.sh" "$NGINX_SRC" "$ROOT/examples/crash_test.c")
PREFIX=$(mktemp -d /tmp/nnx-crash-XXXXXX)
LOG="$PREFIX/nginx.log"
MASTER_PID=""

cleanup() {
    if [ -n "$MASTER_PID" ] && kill -0 "$MASTER_PID" 2>/dev/null; then
        "$NGINX_SRC/objs/nginx" -p "$PREFIX" -c "$PREFIX/nginx.conf" -s quit             >/dev/null 2>&1 || kill "$MASTER_PID" 2>/dev/null || true
        wait "$MASTER_PID" 2>/dev/null || true
    fi
    rm -rf "$PREFIX"
}
trap cleanup EXIT INT TERM

mkdir -p "$PREFIX/logs"

cat > "$PREFIX/nginx.conf" <<EOF
load_module $MODULE;
daemon off;
master_process on;
worker_processes 1;
error_log stderr notice;
pid logs/nginx.pid;
events { worker_connections 1024; }
http {
    access_log /dev/stdout;
    server {
        listen 127.0.0.1:18081;
        location / { nnx; }
    }
}
EOF

"$NGINX_SRC/objs/nginx" -p "$PREFIX" -c "$PREFIX/nginx.conf" >"$LOG" 2>&1 &
MASTER_PID=$!

healthy=0
for i in $(seq 1 30); do
    if [ "$(curl -sS --max-time 2 http://127.0.0.1:18081/health 2>/dev/null || true)" = "ok" ]; then
        healthy=1
        break
    fi
    sleep 1
done
if [ "$healthy" -ne 1 ]; then
    echo "nnx: worker did not become healthy before crash test" >&2
    cat "$LOG" >&2 || true
    exit 1
fi

kill -0 "$MASTER_PID"

# The connection is expected to fail because the worker terminates mid-request.
curl -sS --max-time 5 http://127.0.0.1:18081/crash >/dev/null 2>&1 || true

# The master must survive and spawn a replacement worker.
kill -0 "$MASTER_PID"

recovered=0
for i in $(seq 1 30); do
    if [ "$(curl -sS --max-time 2 http://127.0.0.1:18081/health 2>/dev/null || true)" = "ok" ]; then
        recovered=1
        break
    fi
    sleep 1
done

if [ "$recovered" -ne 1 ]; then
    echo "nnx: worker did not recover after SIGSEGV" >&2
    cat "$LOG" >&2 || true
    exit 1
fi

echo "nnx worker crash isolation test: PASS"
