# nnx

An experimental Echo-like web framework for C, powered by Nginx.

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

## Design rule

Application code sees `nnx.h`. Nginx-specific `ngx_*` types stay behind the adapter.

```text
user app -> nnx.h -> core/router -> nginx_adapter -> Nginx -> OS
```

## Current status

- public API skeleton
- GET/POST route registration
- exact route matching
- router unit test
- Nginx boundary reserved

Run the current tests with:

```sh
make test
```

## Next milestone

Make `curl localhost:8080` return `Hello, nnx!` through a real Nginx request handler.
