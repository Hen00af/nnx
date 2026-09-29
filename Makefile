CC ?= cc
CFLAGS ?= -Wall -Wextra -Werror -std=c11
CPPFLAGS += -Iinclude -Isrc

CORE = src/app.c src/router.c

.PHONY: all test runtime-check clean

all: test runtime-check

tests/router_test: tests/router_test.c $(CORE) include/nnx.h src/nnx_internal.h
	$(CC) $(CFLAGS) $(CPPFLAGS) tests/router_test.c $(CORE) -o $@

test: tests/router_test
	./tests/router_test

runtime-check: src/runtime.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c src/runtime.c -o /tmp/nnx-runtime.o
	rm -f /tmp/nnx-runtime.o

clean:
	rm -f tests/router_test
