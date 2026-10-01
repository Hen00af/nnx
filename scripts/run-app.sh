#!/bin/sh
set -eu

if [ "$#" -lt 2 ] || [ "$#" -gt 3 ]; then
    echo "usage: $0 /path/to/nginx-source /path/to/app.c [port]" >&2
    exit 2
fi

ROOT=$(cd "$(dirname "$0")/.." && pwd)
NGINX_INPUT=$1
APP_INPUT=$2
PORT=${3:-8080}

if [ ! -d "$NGINX_INPUT" ] || [ ! -f "$NGINX_INPUT/configure" ]; then
    echo "nnx: invalid Nginx source directory: $NGINX_INPUT" >&2
    exit 2
fi
if [ ! -f "$APP_INPUT" ]; then
    echo "nnx: application source not found: $APP_INPUT" >&2
    exit 2
fi
case "$PORT" in
    ''|*[!0-9]*)
        echo "nnx: port must be a number between 1 and 65535" >&2
        exit 2
        ;;
esac
if [ "$PORT" -lt 1 ] || [ "$PORT" -gt 65535 ]; then
    echo "nnx: port must be a number between 1 and 65535" >&2
    exit 2
fi

NGINX_SRC=$(cd "$NGINX_INPUT" && pwd)
APP=$(cd "$(dirname "$APP_INPUT")" && pwd)/$(basename "$APP_INPUT")
MODULE=$("$ROOT/scripts/build-app-module.sh" "$NGINX_SRC" "$APP")
PREFIX=$(mktemp -d /tmp/nnx-run-XXXXXX)
NGINX_PID=""
mkdir -p "$PREFIX/logs"

cleanup() {
    status=$?
    trap - EXIT INT TERM
    if [ -n "$NGINX_PID" ] && kill -0 "$NGINX_PID" 2>/dev/null; then
        kill "$NGINX_PID" 2>/dev/null || true
        wait "$NGINX_PID" 2>/dev/null || true
    fi
    rm -rf "$PREFIX"
    exit "$status"
}
trap cleanup EXIT INT TERM

cat > "$PREFIX/nginx.conf" <<EOF
load_module $MODULE;
daemon off;
master_process off;
error_log stderr notice;
pid logs/nginx.pid;
events { worker_connections 1024; }
http {
    access_log /dev/stdout;
    server {
        listen 127.0.0.1:$PORT;
        location / { nnx; }
    }
}
EOF

echo "nnx: app=$APP" >&2
echo "nnx: listening on http://127.0.0.1:$PORT" >&2

"$NGINX_SRC/objs/nginx" -p "$PREFIX" -c "$PREFIX/nginx.conf" &
NGINX_PID=$!

set +e
wait "$NGINX_PID"
status=$?
set -e
NGINX_PID=""
exit "$status"
