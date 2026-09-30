CC ?= cc
CFLAGS ?= -Wall -Wextra -Werror -std=c11
CPPFLAGS += -Iinclude -Isrc/internal

CORE = src/core/app.c src/core/context.c src/core/response.c
ROUTER = src/router/router.c
MIDDLEWARE = src/middleware/builtin.c

.PHONY: all test clean

all: test

tests/router/router_test: tests/router/router_test.c $(CORE) $(ROUTER) include/nnx.h src/internal/nnx_internal.h
	$(CC) $(CFLAGS) $(CPPFLAGS) tests/router/router_test.c $(CORE) $(ROUTER) -o $@

tests/core/response_test: tests/core/response_test.c src/core/context.c src/core/response.c include/nnx.h src/internal/nnx_internal.h
	$(CC) $(CFLAGS) $(CPPFLAGS) tests/core/response_test.c src/core/context.c src/core/response.c -o $@

tests/core/middleware_test: tests/core/middleware_test.c $(CORE) $(ROUTER) include/nnx.h src/internal/nnx_internal.h
	$(CC) $(CFLAGS) $(CPPFLAGS) tests/core/middleware_test.c $(CORE) $(ROUTER) -o $@

tests/middleware/builtin_test: tests/middleware/builtin_test.c $(CORE) $(ROUTER) $(MIDDLEWARE) include/nnx.h include/nnx/middleware.h src/internal/nnx_internal.h
	$(CC) $(CFLAGS) $(CPPFLAGS) tests/middleware/builtin_test.c $(CORE) $(ROUTER) $(MIDDLEWARE) -o $@

test: tests/router/router_test tests/core/response_test tests/core/middleware_test tests/middleware/builtin_test
	./tests/router/router_test
	./tests/core/response_test
	./tests/core/middleware_test
	./tests/middleware/builtin_test

clean:
	rm -f tests/router/router_test tests/core/response_test tests/core/middleware_test tests/middleware/builtin_test
