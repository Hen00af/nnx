CC ?= cc
CFLAGS ?= -Wall -Wextra -Werror -std=c11
CPPFLAGS += -Iinclude -Isrc/internal

CORE = src/core/app.c src/core/context.c src/core/response.c
ROUTER = src/router/router.c
MIDDLEWARE = src/middleware/builtin.c

.PHONY: all test test-sanitize integration clean

all: test

tests/router/router_test: tests/router/router_test.c $(CORE) $(ROUTER) include/nnx.h src/internal/nnx_internal.h
	$(CC) $(CFLAGS) $(CPPFLAGS) tests/router/router_test.c $(CORE) $(ROUTER) -o $@

tests/core/response_test: tests/core/response_test.c src/core/context.c src/core/response.c include/nnx.h src/internal/nnx_internal.h
	$(CC) $(CFLAGS) $(CPPFLAGS) tests/core/response_test.c src/core/context.c src/core/response.c -o $@

tests/core/middleware_test: tests/core/middleware_test.c $(CORE) $(ROUTER) include/nnx.h src/internal/nnx_internal.h
	$(CC) $(CFLAGS) $(CPPFLAGS) tests/core/middleware_test.c $(CORE) $(ROUTER) -o $@

tests/core/error_test: tests/core/error_test.c $(CORE) $(ROUTER) include/nnx.h src/internal/nnx_internal.h
	$(CC) $(CFLAGS) $(CPPFLAGS) tests/core/error_test.c $(CORE) $(ROUTER) -o $@

tests/core/lifecycle_test: tests/core/lifecycle_test.c $(CORE) $(ROUTER) include/nnx.h src/internal/nnx_internal.h
	$(CC) $(CFLAGS) $(CPPFLAGS) tests/core/lifecycle_test.c $(CORE) $(ROUTER) -o $@

tests/middleware/builtin_test: tests/middleware/builtin_test.c $(CORE) $(ROUTER) $(MIDDLEWARE) include/nnx.h include/nnx/middleware.h src/internal/nnx_internal.h
	$(CC) $(CFLAGS) $(CPPFLAGS) tests/middleware/builtin_test.c $(CORE) $(ROUTER) $(MIDDLEWARE) -o $@

tests/adapter/nginx_response_test: tests/adapter/nginx_response_test.c tests/adapter/ngx_config.h tests/adapter/ngx_core.h tests/adapter/ngx_http.h src/adapter/nginx/response.c
	$(CC) $(CFLAGS) -Itests/adapter tests/adapter/nginx_response_test.c src/adapter/nginx/response.c -o $@

test: tests/router/router_test tests/core/response_test tests/core/middleware_test tests/core/error_test tests/core/lifecycle_test tests/middleware/builtin_test tests/adapter/nginx_response_test
	./tests/router/router_test
	./tests/core/response_test
	./tests/core/middleware_test
	./tests/core/error_test
	./tests/core/lifecycle_test
	./tests/middleware/builtin_test
	./tests/adapter/nginx_response_test

test-sanitize: clean
	$(MAKE) CFLAGS="$(CFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer" test

clean:
	rm -f tests/router/router_test tests/core/response_test tests/core/middleware_test tests/core/error_test tests/core/lifecycle_test tests/middleware/builtin_test tests/adapter/nginx_response_test


integration:
	@test -n "$(NGINX_SRC)" || (echo "usage: make integration NGINX_SRC=/path/to/nginx-source" >&2; exit 2)
	./scripts/integration-test.sh "$(NGINX_SRC)"
