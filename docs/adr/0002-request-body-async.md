# ADR-0002: Asynchronous request-body dispatch

- Status: Accepted
- Date: 2026-09-30
- Scope: Nginx request adapter and nnx handler dispatch

## Context

Nginx workers are event-driven. Reading an HTTP request body may require additional socket events and may spill into Nginx-managed temporary files.

A synchronous framework API must not turn this into a blocking wait inside the worker.

The desired application handler remains simple:

```c
static void echo(nnx_ctx *ctx)
{
    size_t len;
    const void *body = nnx_body(ctx, &len);
    nnx_blob(ctx, 200, "application/octet-stream", body, len);
}
```

## Decision

The Nginx adapter uses `ngx_http_read_client_request_body()`.

If a body may be present, route matching happens first, then request handling is suspended while Nginx owns body acquisition.

```text
request arrives
    |
route match
    |
ngx_http_read_client_request_body()
    |
return NGX_DONE
    |
Nginx event loop
    |
body callback
    |
copy body into request-pool memory
    |
nnx_dispatch()
    |
middleware -> handler
```

The application middleware and handler run only after the body callback has made the body available.

The adapter supports both:

- Nginx in-memory body buffers;
- Nginx temp-file-backed body buffers.

Both are copied into request-pool-backed contiguous memory for the current synchronous nnx body API.

## Error behavior

Body-copy failures become nnx 500 error dispatches.

Framework/application body limits return 413 through the normal nnx error-handler path.

The current `nnx_body_limit()` middleware runs after body acquisition. It therefore provides application-level semantics, not an early socket-read cutoff. Early rejection based on content length or configured adapter limits is a separate optimization.

## Consequences

### Positive

- the worker does not spin or block while waiting for request-body network I/O;
- middleware sees a complete body;
- handlers keep a small synchronous API;
- temp-file-backed Nginx bodies work through the same application API;
- request memory follows the Nginx request-pool lifetime.

### Negative

- a contiguous copy is currently made before handler dispatch;
- large bodies can incur copy cost;
- BodyLimit does not yet prevent Nginx from reading a body smaller than Nginx's own configured maximum but larger than the application limit;
- truly asynchronous external application I/O still needs a separate design.

## Rejected approach

nnx will not block an Nginx worker or busy-wait until the request body is complete merely to make the framework appear synchronous.

The synchronous handler API is entered only after the Nginx asynchronous body lifecycle completes.
