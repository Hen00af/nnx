#!/bin/sh
set -eu

if [ "$#" -ne 1 ]; then
    echo "usage: $0 /path/to/nginx-source" >&2
    exit 2
fi

ROOT=$(cd "$(dirname "$0")/.." && pwd)
NGINX_SRC=$(cd "$1" && pwd)
MODULE=$("$ROOT/scripts/build-app-module.sh" "$NGINX_SRC" "$ROOT/examples/merge_test.c")
PREFIX=$(mktemp -d /tmp/nnx-integration-XXXXXX)
LOG="$PREFIX/nginx.log"
COOKIE="$PREFIX/cookies.txt"

cleanup() {
    if [ -f "$PREFIX/logs/nginx.pid" ]; then
        kill "$(cat "$PREFIX/logs/nginx.pid")" 2>/dev/null || true
    fi
    rm -rf "$PREFIX"
}
trap cleanup EXIT INT TERM

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

"$NGINX_SRC/objs/nginx" -p "$PREFIX" -c "$PREFIX/nginx.conf" >"$LOG" 2>&1 &
NGINX_PID=$!
printf '%s\n' "$NGINX_PID" > "$PREFIX/logs/nginx.pid"

for i in $(seq 1 30); do
    if curl -fsS http://127.0.0.1:8080/ >/dev/null 2>&1; then
        break
    fi
    sleep 1
done

assert_status() {
    expected=$1
    shift
    actual=$(curl -sS -o /dev/null -w '%{http_code}' "$@")
    [ "$actual" = "$expected" ] || {
        echo "expected HTTP $expected, got $actual: curl $*" >&2
        exit 1
    }
}

assert_body() {
    expected=$1
    shift
    actual=$(curl -fsS "$@")
    [ "$actual" = "$expected" ] || {
        echo "unexpected body for curl $*" >&2
        printf 'expected: %s\nactual:   %s\n' "$expected" "$actual" >&2
        exit 1
    }
}

body=$(curl -fsS http://127.0.0.1:8080/)
printf '%s' "$body" | grep -q 'nnx works'

user_headers="$PREFIX/user.headers"
user_body="$PREFIX/user.body"
curl -fsS -D "$user_headers" -o "$user_body" http://127.0.0.1:8080/users/42
grep -q '"id":"42"' "$user_body"
grep -q '"method":"GET"' "$user_body"
grep -qi '^X-Request-ID:' "$user_headers"
grep -qi '^X-Content-Type-Options: nosniff' "$user_headers"
grep -qi '^X-Frame-Options: SAMEORIGIN' "$user_headers"

assert_body "nginx" "http://127.0.0.1:8080/search?q=nginx"
assert_body "hello nnx" -X POST -H "Content-Type: text/plain" --data-binary "hello nnx" http://127.0.0.1:8080/echo
assert_body "Seiya Hattori" -X POST -H "Content-Type: application/x-www-form-urlencoded" --data "name=Seiya+Hattori" http://127.0.0.1:8080/form

cookie_headers="$PREFIX/cookie.headers"
curl -fsS -D "$cookie_headers" -c "$COOKIE" http://127.0.0.1:8080/cookie >/dev/null
grep -qi '^Set-Cookie: nnx_session=hello' "$cookie_headers"
assert_body "hello" -b "$COOKIE" http://127.0.0.1:8080/cookie/read

metadata_body="$PREFIX/metadata.body"
curl -fsS http://127.0.0.1:8080/metadata > "$metadata_body"
grep -q '"host":"127.0.0.1:8080"' "$metadata_body"
grep -q '"scheme":"http"' "$metadata_body"
grep -q '"request_id":"nnx-' "$metadata_body"

assert_body "request scoped memory" http://127.0.0.1:8080/ctx

group_headers="$PREFIX/group.headers"
group_body="$PREFIX/group.body"
curl -fsS -D "$group_headers" -o "$group_body" http://127.0.0.1:8080/api/v1/users/123
grep -qx '123' "$group_body"
grep -qi '^X-NNX-Group: api' "$group_headers"

not_found_headers="$PREFIX/404.headers"
not_found_body="$PREFIX/404.body"
curl -sS -D "$not_found_headers" -o "$not_found_body" http://127.0.0.1:8080/not-found
grep -q 'HTTP/1.1 404' "$not_found_headers"
grep -q '"error":404' "$not_found_body"
grep -qi '^X-Request-ID:' "$not_found_headers"

method_headers="$PREFIX/405.headers"
method_body="$PREFIX/405.body"
curl -sS -X DELETE -D "$method_headers" -o "$method_body" http://127.0.0.1:8080/users/42
grep -q 'HTTP/1.1 405' "$method_headers"
grep -q '"error":405' "$method_body"


payload_headers="$PREFIX/413.headers"
payload_body="$PREFIX/413.body"
curl -sS -X POST -H "Content-Type: text/plain" \
  --data-binary "0123456789012345678901234567890123456789" \
  -D "$payload_headers" -o "$payload_body" \
  http://127.0.0.1:8080/echo
grep -q 'HTTP/1.1 413' "$payload_headers"
grep -q '"error":413' "$payload_body"
grep -qi '^X-Request-ID:' "$payload_headers"

internal_headers="$PREFIX/500.headers"
internal_body="$PREFIX/500.body"
curl -sS -D "$internal_headers" -o "$internal_body" \
  http://127.0.0.1:8080/implicit-500
grep -q 'HTTP/1.1 500' "$internal_headers"
grep -q '"error":500' "$internal_body"
grep -qi '^X-Request-ID:' "$internal_headers"

assert_status 200 http://127.0.0.1:8080/
assert_status 200 http://127.0.0.1:8080/api/v1/users/123
assert_status 404 http://127.0.0.1:8080/not-found
assert_status 405 -X DELETE http://127.0.0.1:8080/users/42
assert_status 413 -X POST --data-binary "0123456789012345678901234567890123456789" http://127.0.0.1:8080/echo
assert_status 500 http://127.0.0.1:8080/implicit-500

echo "nnx integration test: PASS"
