# nnx runtime

## Chosen architecture

nnx does not copy or vendor Nginx. It integrates through the supported Nginx
module boundary.

```text
application API
    |
 nnx core
    |
 Nginx HTTP module (.so or static module)
    |
 external Nginx binary
```

The module must be built for a compatible Nginx binary. Nginx validates module
version/build compatibility.

## Runtime responsibilities

`nnx_run()` is responsible for process-level concerns:

1. locate the external Nginx executable (or use `NNX_NGINX_BIN`);
2. create an isolated temporary prefix;
3. generate an nnx-specific nginx.conf;
4. start Nginx in the foreground;
5. forward termination and wait for shutdown;
6. clean temporary runtime files.

The HTTP adapter owns request-level concerns:

```text
ngx_http_request_t -> nnx_ctx -> router -> handler
                   <- nnx_send <- response
```

## Important limitation

An in-memory `nnx_app *` cannot cross an `exec()` boundary. Therefore the
final standalone application build must put the application's registration
code into the Nginx-loadable module (or statically linked Nginx module), rather
than attempting to pass C function pointers to an already-running unrelated
Nginx process.

That build step is part of the framework/toolchain, not something the public
handler API should expose.

## Blocking handlers

Nginx workers are event-driven. A synchronous handler that performs blocking
database/network I/O blocks that worker. The initial nnx API is intentionally
synchronous; asynchronous I/O needs a separate API rather than hiding blocking
work behind the current handler signature.
