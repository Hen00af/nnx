# ADR-0003: Core and adapter boundary

- Status: Accepted
- Date: 2026-09-30

## Decision

The nnx core must not include Nginx headers or call Nginx APIs.

Nginx-specific code lives under `src/adapter/nginx/`. Core response operations
cross the boundary through the internal `nnx_adapter` interface.

Dependency direction:

```text
application -> nnx core/router
                    ^
                    |
             adapter contract
                    ^
                    |
             adapter/nginx -> Nginx
```

Only the adapter may know both nnx internals and Nginx internals.

## Why

This keeps router, context, response and future middleware independently
testable without building Nginx. It also limits regressions caused by Nginx API
changes to the adapter and integration-test layer.

## Testing contract

- Core/router tests use no Nginx headers.
- Adapter behavior is covered separately.
- Real Nginx + HTTP requests form the integration boundary.
- Refactors must preserve the existing `GET / -> Hello, nnx!` smoke test.
