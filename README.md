# nnx

Experimental Echo-like C web framework running directly as an Nginx HTTP module.

Nginx is an external dependency; its source is not vendored into this repository.

## Application

```c
#include <nnx.h>

static void hello(nnx_ctx *ctx)
{
    nnx_send(ctx, 200, "Hello, nnx!\n");
}

int nnx_register(nnx_app *app)
{
    return nnx_get(app, "/", hello);
}
```

The registration hook executes inside each Nginx worker. That is important:
ordinary C handler function pointers remain in the same process/address space.

## Request path

```text
Nginx socket/event loop
  -> ngx_http_request_t
  -> nnx adapter
  -> nnx router
  -> user C handler
  -> nnx_send
  -> Nginx output filter
```

No `ngx_*` type appears in application code.

## Build and run the hello app

Use Nginx source compatible with the Nginx binary/module you intend to run:

```sh
./scripts/run-example.sh /path/to/nginx-source
```

Then:

```sh
curl http://127.0.0.1:8080/
# Hello, nnx!
```

For a custom app:

```sh
./scripts/build-app-module.sh /path/to/nginx-source ./my_app.c
```

## Why not pass nnx_app through exec?

`exec()` replaces the process image, so an in-memory router and its C function
pointers cannot be handed to a stock external Nginx process. nnx solves this
without a reverse proxy: application registration is compiled into the Nginx
dynamic module and initialized in the Nginx worker lifecycle.

## Current scope

- GET and POST route registration
- exact path matching
- plain-text responses
- 404 / 405 handling through Nginx
- worker-local app lifecycle
- dynamic-module build helper
- standalone example runner

Blocking I/O inside a handler will block that Nginx worker. Async I/O,
parameters, request bodies, headers, middleware, TLS helpers and production
packaging are future work.
