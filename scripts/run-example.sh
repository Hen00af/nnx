#!/bin/sh
set -eu

if [ "$#" -ne 1 ]; then
    echo "usage: $0 /path/to/nginx-source" >&2
    exit 2
fi

ROOT=$(cd "$(dirname "$0")/.." && pwd)
NGINX_SRC=$(cd "$1" && pwd)
MODULE=$("$ROOT/scripts/build-app-module.sh" "$NGINX_SRC" "$ROOT/examples/hello.c")
PREFIX=$(mktemp -d /tmp/nnx-example-XXXXXX)
mkdir -p "$PREFIX/logs"

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
        listen 127.0.0.1:8080;
        location / { nnx; }
    }
}
EOF

exec "$NGINX_SRC/objs/nginx" -p "$PREFIX" -c "$PREFIX/nginx.conf"
