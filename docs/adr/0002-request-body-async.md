# ADR-0002: Request bodies and asynchronous handler evolution

- Status: Proposed
- Date: 2026-09-30

## Context

The first nnx handler API is synchronous:

```c
void handler(nnx_ctx *ctx);
```

Headers, URI parameters and query arguments are already available when Nginx
enters a content handler. A request body is different: Nginx may need to read
it asynchronously and resume through a callback.

Pretending that `nnx_body(ctx)` is always immediately available would either
block a worker, depend on accidental buffering, or hide Nginx lifecycle rules.

## Decision for the current milestone

Do not expose request-body access yet.

Keep the synchronous API for data that is already available, and design body
reading together with the next handler model.

Candidate directions:

1. explicit async body callback;
2. framework-managed two-phase dispatch (read body first, then invoke the
   existing synchronous handler);
3. a future coroutine/task abstraction.

The preferred next experiment is framework-managed two-phase dispatch because
it can preserve the simple user handler signature while allowing Nginx to
complete body reads asynchronously before entering user code.

## Invariant

nnx must not perform blocking reads in an Nginx worker merely to preserve a
synchronous-looking API.
