#!/bin/sh
set -eu

if [ "$#" -ne 1 ]; then
    echo "usage: $0 /path/to/nginx-source" >&2
    exit 2
fi

ROOT=$(cd "$(dirname "$0")/.." && pwd)
exec "$ROOT/scripts/run-app.sh" "$1" "$ROOT/examples/hello.c" 8080
