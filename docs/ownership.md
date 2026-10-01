# Ownership and lifetime rules

C APIs become difficult to use safely when ownership is implicit. nnx v0.1 uses the rules below.

## Application lifetime

```c
nnx_app *app = nnx_new();
/* register routes, groups, middleware */
nnx_free(app);
```

`nnx_free()` releases framework-owned application state, including:

- copied route paths;
- route arrays;
- route groups and group prefixes;
- global middleware entries;
- group middleware entries;
- middleware state that has a `destroy` callback.

## Middleware ownership

A middleware value may contain owned state:

```c
typedef struct nnx_middleware {
    nnx_middleware_fn fn;
    void *data;
    void (*destroy)(void *data);
} nnx_middleware;
```

When `nnx_use(app, middleware)` or `nnx_group_use(group, middleware)` is called, nnx consumes the middleware value.

On successful registration, nnx owns `middleware.data` and eventually calls `destroy(data)` from `nnx_free()`.

If registration fails after a middleware value is passed in, nnx also calls the provided `destroy` callback. Application code must not free the same state again.

Built-in stateful middleware follows this rule.

## Request-scoped allocation

```c
void *nnx_alloc(nnx_ctx *ctx, size_t size);
```

In the Nginx adapter, `nnx_alloc()` uses the current Nginx request pool.

Memory returned by `nnx_alloc()`:

- belongs to the current request;
- is automatically reclaimed with the request pool;
- must not be retained after the request ends;
- must not be passed to `free()` by application code.

Use this for temporary request objects:

```c
User *user = nnx_alloc(ctx, sizeof(*user));
if (!user)
    return;

nnx_ctx_set(ctx, "user", user);
```

## Context key/value store

```c
int nnx_ctx_set(nnx_ctx *ctx, const char *key, void *value);
void *nnx_ctx_get(nnx_ctx *ctx, const char *key);
```

The store is **non-owning**.

nnx copies the key into request context storage, but it stores the value pointer exactly as supplied. nnx does not call `free()` on arbitrary context values.

Unsafe pattern:

```c
User *user = malloc(sizeof(*user));
nnx_ctx_set(ctx, "user", user);
/* leaks unless application code frees user itself */
```

Preferred request-lifetime pattern:

```c
User *user = nnx_alloc(ctx, sizeof(*user));
nnx_ctx_set(ctx, "user", user);
```

## Request-derived strings

Values returned from request APIs should be treated as request-lifetime pointers unless explicitly documented otherwise.

This includes values such as:

- method and path;
- route parameters and wildcard;
- query/header values;
- cookie and form values;
- host, scheme, client IP, and request ID;
- request body pointers.

Do not retain those pointers in global or long-lived application state.

If data must survive the request, copy it into application-owned storage.

## Application state is worker-local

Each Nginx worker initializes its own `nnx_app`. Mutable global or app-level state is therefore worker-local unless the application deliberately uses shared memory or another external coordination mechanism.

Do not assume that changing an in-memory value in one worker changes the same value in another worker.
