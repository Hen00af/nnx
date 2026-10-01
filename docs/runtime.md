# nnx runtime

## Current architecture

nnx keeps Nginx external and integrates through an Nginx HTTP module.

```text
application source
      |
nnx build tooling
      |
  nnx core
      |
 Nginx adapter
      |
application handlers
      |
ngx_http_nnx_module.so
      |
 Nginx worker
```

The application-facing API contains no `ngx_*` types.

## Current development runner

The v0.1 prerelease does **not** expose an `nnx_run()` runtime API.

The current one-command development path is:

```sh
./scripts/run-app.sh /path/to/nginx-source ./app.c [port]
```

The runner:

1. validates the Nginx source directory and application source;
2. builds the nnx/application dynamic module;
3. creates an isolated temporary Nginx prefix;
4. generates a minimal `nginx.conf`;
5. launches the built Nginx binary in the foreground;
6. removes the temporary runtime directory when the process exits.

The generated configuration contains one nnx location:

```nginx
location / {
    nnx;
}
```

Application routes remain in C. nnx does not generate one Nginx `location` per route.

## Why application code is compiled into the module

A registered handler is a C function pointer in one process address space.

```text
nnx_get(app, "/", hello)
                  |
                  +-- pointer to hello()
```

Calling `exec("nginx")` replaces that process image. The original router and function pointers do not become callable objects inside the new Nginx process.

Therefore the current build puts the application registration code and handlers into the loadable module. Each Nginx worker creates its own nnx application and calls:

```c
int nnx_register(nnx_app *app);
```

This keeps handler pointers valid in the same address space that dispatches requests.

See ADR-0001 for the full decision.

## Request lifecycle

```text
socket ready
    |
Nginx parses HTTP request
    |
nnx adapter creates request context
    |
router matches route
    |
request body needed?
    | yes
    v
ngx_http_read_client_request_body()
    |
body callback
    |
copy body into request-pool memory
    |
middleware chain
    |
handler
    |
nnx response API
    |
Nginx output filter
```

The worker is never intentionally blocked waiting for the request body to become available.

## Development vs process fault isolation

`scripts/run-app.sh` currently uses:

```nginx
daemon off;
master_process off;
```

This is convenient for foreground development and debugging, but it is a single-process mode.

Production-style Nginx normally uses a master process and one or more workers. That process boundary is the intended fault-isolation mechanism for native C handler crashes. nnx does not attempt to recover in-process from `SIGSEGV` or similar memory faults.

A dedicated worker-crash/respawn integration test is still required before the v0.1 release.

## Blocking handlers

The current handler signature is synchronous:

```c
typedef void (*nnx_handler)(nnx_ctx *ctx);
```

Blocking database, filesystem, or external-network operations inside a handler block the Nginx worker executing that handler.

A future external-I/O async model must integrate with the Nginx event lifecycle explicitly. nnx will not simulate asynchronous behavior by blocking the worker behind a synchronous API.
