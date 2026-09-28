# nnx

An experimental Echo-like web framework for C, powered by Nginx.

nnx does **not** vendor Nginx. Nginx remains an external dependency and nnx talks to its module API through a thin adapter.

```text
user application
      |
    nnx.h
      |
 nnx core/router
      |
 nginx_adapter.c
      |
 external Nginx
      |
      OS
```

## Target API

```c
#include <nnx.h>

static void hello(nnx_ctx *ctx)
{
    nnx_send(ctx, 200, "Hello, nnx!");
}

int main(void)
{
    nnx_app *app = nnx_new();
    nnx_get(app, "/", hello);
    return nnx_run(app, 8080);
}
```

The public API never exposes `ngx_*` types.

## Core tests

No Nginx installation is needed for the core router tests:

```sh
make test
```

## Nginx adapter

`src/nginx_adapter.c` is an Nginx HTTP module boundary. It is compiled against an external Nginx source/build tree rather than copying Nginx into this repository.

A minimal Nginx location can enable it with:

```nginx
location / {
    nnx;
}
```

The current adapter milestone returns `nnx adapter is alive` through the real Nginx output API. The next step is replacing that fixed response with:

```text
ngx_http_request_t
 -> nnx_ctx
 -> nnx_match_route()
 -> user handler
 -> nnx_send()
 -> ngx_http_output_filter()
```

## Status

- public C API skeleton
- GET/POST registration
- exact route matching
- router unit test
- external-Nginx module boundary
- Nginx request/output path smoke handler
