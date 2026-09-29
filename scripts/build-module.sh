#!/bin/sh
set -eu

if [ "$#" -ne 1 ]; then
    echo "usage: $0 /path/to/nginx-source" >&2
    exit 2
fi

NGINX_SRC=$(cd "$1" && pwd)
ROOT=$(cd "$(dirname "$0")/.." && pwd)

cd "$NGINX_SRC"
./configure --with-compat --add-dynamic-module="$ROOT/nginx"
make modules

echo "nnx: module built under $NGINX_SRC/objs/"
echo "nnx: load it with: load_module /absolute/path/to/ngx_http_nnx_module.so;"
