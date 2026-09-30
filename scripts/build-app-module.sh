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
mkdir -p "$GEN/nnx"

cp "$ROOT/include/nnx.h" "$GEN/nnx.h"
cp "$ROOT/include/nnx/middleware.h" "$GEN/nnx/middleware.h"
cp "$ROOT/src/internal/nnx_internal.h" "$GEN/nnx_internal.h"
cp "$ROOT/src/core/app.c" "$GEN/app.c"
cp "$ROOT/src/core/context.c" "$GEN/context.c"
cp "$ROOT/src/core/response.c" "$GEN/response.c"
cp "$ROOT/src/router/router.c" "$GEN/router.c"
cp "$ROOT/src/middleware/builtin.c" "$GEN/builtin_middleware.c"
cp "$ROOT/src/adapter/nginx/module.c" "$GEN/nginx_module.c"
cp "$ROOT/src/adapter/nginx/request.c" "$GEN/nginx_request.c"
cp "$ROOT/src/adapter/nginx/response.c" "$GEN/nginx_response.c"
cp "$APP" "$GEN/user_app.c"

# Flattened generated sources include the copied internal header directly.
sed -i.bak 's#"../internal/nnx_internal.h"#"nnx_internal.h"#'     "$GEN/app.c" "$GEN/context.c" "$GEN/response.c" "$GEN/router.c" "$GEN/builtin_middleware.c"
sed -i.bak 's#"../../internal/nnx_internal.h"#"nnx_internal.h"#'     "$GEN/nginx_module.c" "$GEN/nginx_request.c"
rm -f "$GEN"/*.bak

cat > "$GEN/config" <<EOF
ngx_module_type=HTTP
ngx_module_name=ngx_http_nnx_module
ngx_module_incs="$GEN"
ngx_module_srcs="$GEN/nginx_module.c $GEN/nginx_request.c $GEN/nginx_response.c $GEN/app.c $GEN/context.c $GEN/router.c $GEN/response.c $GEN/builtin_middleware.c $GEN/user_app.c"
. auto/module
ngx_addon_name=\$ngx_module_name
EOF

cd "$NGINX_SRC"
./configure --with-compat --add-dynamic-module="$GEN" >&2
make >&2

MODULE="$NGINX_SRC/objs/ngx_http_nnx_module.so"
NGINX_BIN="$NGINX_SRC/objs/nginx"

if [ ! -f "$MODULE" ]; then
    echo "nnx: module build did not produce $MODULE" >&2
    exit 1
fi
if [ ! -x "$NGINX_BIN" ]; then
    echo "nnx: nginx build did not produce $NGINX_BIN" >&2
    exit 1
fi

# stdout is an API: callers capture exactly one module path from it.
printf '%s\n' "$MODULE"
