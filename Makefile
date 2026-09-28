CC ?= cc
CFLAGS ?= -Wall -Wextra -Werror -std=c11
CPPFLAGS += -Iinclude -Isrc

CORE = src/app.c src/router.c

.PHONY: all test clean

all: test

tests/router_test: tests/router_test.c $(CORE) include/nnx.h src/nnx_internal.h
	$(CC) $(CFLAGS) $(CPPFLAGS) tests/router_test.c $(CORE) -o $@

test: tests/router_test
	./tests/router_test

clean:
	rm -f tests/router_test
