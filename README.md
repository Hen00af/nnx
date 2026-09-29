# nnx

Experimental Echo-like C web framework running directly inside an Nginx HTTP module.

## Example

```c
#include <stdio.h>
#include <nnx.h>

static void hello(nnx_ctx *c)
{
    nnx_text(c, 200, "Hello, nnx!\n");
}

static void user(nnx_ctx *c)
{
    const char *id = nnx_param(c, "id");
    char out[128];
    snprintf(out, sizeof(out), "{\"id\":\"%s\"}\n", id);
    nnx_json(c, 200, out);
}

int nnx_register(nnx_app *app)
{
    nnx_get(app, "/", hello);
    nnx_get(app, "/users/:id", user);
    return 0;
}
```

## Current API

Routing:
- `nnx_get`, `nnx_post`
- static routes and `:param` segments
- static routes take priority over parameter routes

Request:
- `nnx_param(ctx, "id")`
- `nnx_query(ctx, "q")`
- `nnx_header(ctx, "Authorization")`
- `nnx_method_name(ctx)`
- `nnx_path(ctx)`

Response:
- `nnx_text(ctx, status, body)`
- `nnx_json(ctx, status, json)`
- `nnx_send` remains an alias for text responses

Middleware:
- `nnx_use(app, before, after)`
- before hooks run in registration order
- after hooks run in reverse order
- a before hook may short-circuit by sending a response

## Run

```sh
./scripts/run-example.sh /path/to/nginx-source
```

Then try:

```sh
curl http://127.0.0.1:8080/
curl http://127.0.0.1:8080/users/42
curl 'http://127.0.0.1:8080/search?q=nginx'
```

## Architecture

```text
Nginx event loop
  -> nnx adapter
  -> middleware(before)
  -> router
  -> C handler
  -> middleware(after)
  -> nnx response
  -> Nginx output filter
```

Nginx is external and is not vendored. Application code is compiled into the
Nginx module so handler pointers live in the Nginx worker address space.

## Deliberate limitation: request bodies

Nginx request-body reading is asynchronous and callback-driven. nnx does not
currently expose a fake synchronous `nnx_body()` that would block or violate
the Nginx event model. Body parsing will be added together with an explicit
async/request-lifecycle design.

See `docs/adr/0001-runtime-model.md` for the executable/process-boundary
decision.
