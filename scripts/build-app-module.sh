#!/bin/sh
set -eu

if [ "$#" -ne 2 ]; then
    echo "usage: $0 /path/to/nginx-source /path/to/app.c" >&2
    exit 2
fi

NGINX_SRC=$(cd "$1" && pwd)
APP=$(cd "$(dirname "$2")" && pwd)/$(basename "$2")
ROOT=$(cd "$(dirname "$0")/.." && pwd)
GEN="$ROOT/.nnx-build"

rm -rf "$GEN"
mkdir -p "$GEN"
cp "$ROOT/nginx/config" "$GEN/config"
cp "$ROOT/src/nginx_adapter.c" "$GEN/nginx_adapter.c"
cp "$ROOT/src/app.c" "$GEN/app.c"
cp "$ROOT/src/router.c" "$GEN/router.c"
cp "$ROOT/src/response.c" "$GEN/response.c"
cp "$ROOT/src/nnx_internal.h" "$GEN/nnx_internal.h"
cp "$ROOT/include/nnx.h" "$GEN/nnx.h"
cp "$APP" "$GEN/user_app.c"

cat > "$GEN/config" <<EOF
ngx_module_type=HTTP
ngx_module_name=ngx_http_nnx_module
ngx_module_incs="$GEN"
ngx_module_srcs="$GEN/nginx_adapter.c $GEN/app.c $GEN/router.c $GEN/response.c $GEN/user_app.c"
. auto/module
ngx_addon_name=\$ngx_module_name
EOF

cd "$NGINX_SRC"
./configure --with-compat --add-dynamic-module="$GEN"
make modules

echo "$NGINX_SRC/objs/ngx_http_nnx_module.so"
