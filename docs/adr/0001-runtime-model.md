# ADR-0001: Runtime model and the executable / Nginx worker boundary

- Status: Accepted for the initial implementation
- Date: 2026-09-29
- Scope: nnx runtime, build tooling, and Nginx integration

## Context

The desired nnx developer experience is intentionally close to a normal C
library:

```c
#include <nnx.h>

static void hello(nnx_ctx *ctx)
{
    nnx_send(ctx, 200, "Hello, nnx!\n");
}

int main(void)
{
    nnx_app *app = nnx_new();
    nnx_get(app, "/", hello);
    return nnx_run(app, 8080);
}
```

Ideally this could be built and run as:

```sh
gcc main.c -lnnx -o server
./server
```

At the same time, nnx wants requests to execute directly inside an Nginx
worker so it can use Nginx's HTTP request lifecycle, event loop, output
filters, and worker model rather than placing a separate HTTP server behind an
Nginx reverse proxy.

Those goals create a process-boundary problem.

After `nnx_get(app, "/", hello)`, the route table contains a C function
pointer to `hello` in the application's address space. If `nnx_run()`
forks and then calls `exec()` to start a stock external Nginx executable,
`exec()` replaces the child process image. The original application's route
table, code mapping, and function pointers are not preserved as callable
objects in the new Nginx process.

Therefore a stock Nginx worker cannot directly call a handler merely because
the parent executable registered it before `exec()`.

This is not an Nginx routing problem. It is a Unix process/address-space
boundary.

## Decision

For the initial architecture, application code is compiled into an
Nginx-loadable nnx module.

The build/runtime toolchain is responsible for hiding this implementation
detail from application developers.

Conceptually:

```text
user source
   |
   +-----------------------+
   |                       |
developer-facing       build tooling
entry point                |
                           +-- nnx core
                           +-- nginx adapter
                           +-- user handlers
                                  |
                                  v
                        ngx_http_nnx_module.so
                                  |
                                  v
                            Nginx worker
```

Inside the worker, nnx invokes an application registration hook:

```c
int nnx_register(nnx_app *app);
```

The hook registers ordinary C handlers after the module has been loaded into
the Nginx worker's address space. Handler pointers are therefore valid in the
same process that dispatches requests.

The public API does not expose `ngx_*` types.

## Why the current API uses nnx_register

The initial executable-style API and the native Nginx-worker architecture
cannot be connected by a plain `exec("nginx")`.

For the first working implementation, the explicit registration hook makes
the process model truthful:

```text
Nginx worker starts
      |
module init_process
      |
nnx_new()
      |
nnx_register(app)
      |
GET "/" -> hello
      |
request arrives
      |
hello(ctx)
```

This is an implementation-stage API, not necessarily the final developer
experience.

## Alternatives considered

### 1. dlopen / symbol lookup hacks

One idea is to start Nginx with a generic bridge module and resolve handlers
from the original executable.

A simple `dlopen(NULL)` does not solve the problem. After `exec()`, the
current process image is Nginx. `dlopen(NULL)` exposes symbols from the
current executable and its loaded objects, not symbols from the executable
that existed before `exec()`.

A workable variation is to compile user code into a shared object and have the
bridge module perform something equivalent to:

```c
void *app = dlopen("/path/to/user_app.so", RTLD_NOW);
register_fn = dlsym(app, "nnx_register");
```

This is viable and is closely related to the chosen module/tooling approach.
The important point is that user code must become a loadable object inside the
Nginx process; it cannot be recovered from the pre-exec address space by
symbol lookup alone.

### 2. CLI/compiler-driver hides module generation

Example UX:

```sh
nnx run main.c
```

or:

```sh
nnxcc main.c -o server
./server
```

Internally the tool may compile the application into a module, generate
`nginx.conf`, load the module, and launch Nginx.

This is the preferred direction.

The implementation can remain module-based while the visible developer
experience becomes increasingly similar to a conventional executable.

A future `nnxcc` may produce two internal artifacts:

```text
main.c
  |
 nnxcc
  |
  +-- launcher executable
  |
  +-- application/Nginx module
```

The module could eventually be embedded as data in the launcher executable
and extracted to an isolated runtime directory when the program starts. That
would provide a single-file distribution experience without pretending that
the Nginx worker shares the launcher's pre-exec memory.

This is an implementation possibility, not yet a committed file format.

### 3. Make Nginx an embeddable shared library

The cleanest theoretical model for the original API would be:

```text
single process
+--------------------------------+
| main()                         |
| user handlers                  |
| nnx_app                        |
| libnnx                         |
| embedded Nginx runtime         |
| event loop                     |
+--------------------------------+
```

Then `nnx_run(app, 8080)` could invoke the Nginx runtime without crossing an
`exec()` boundary, and handler pointers would naturally remain valid.

However, stock Nginx is not primarily distributed or supported as an
embeddable `libnginx.so` API. Pursuing this path would require substantial
ownership of Nginx initialization, configuration, globals, process lifecycle,
signals, modules, and compatibility.

That changes the project from "a C framework powered by Nginx" toward "an
embeddable fork/runtime derived from Nginx."

It remains an interesting experimental direction, but it is not the v0
architecture.

### 4. Separate nnx application process behind Nginx

Another straightforward design would keep the application executable alive
and make Nginx proxy requests to it over TCP or a Unix domain socket.

That solves the address-space problem but introduces an extra HTTP/IPC server,
serialization boundary, and proxy hop.

It also weakens the central nnx goal: handlers should execute as part of the
Nginx request/module path rather than implementing another upstream web server.

Therefore this is not the primary architecture.

## Consequences

### Positive

- handlers execute directly inside Nginx workers;
- nnx can use Nginx request and response APIs directly;
- no extra reverse-proxy application process is required;
- user-facing headers remain independent of Nginx internals;
- the framework can build progressively better tooling around a technically
  honest module architecture.

### Negative

- application builds depend on Nginx module ABI/build compatibility;
- the build toolchain is more involved than a normal `gcc main.c -lnnx`;
- debugging crosses framework, generated build artifacts, and Nginx lifecycle;
- synchronous blocking handlers can block an Nginx worker;
- a truly arbitrary stock executable cannot hand live C function pointers to
  a separately exec'd Nginx worker.

## Developer-experience roadmap

The internal architecture and external UX are intentionally allowed to differ.

### v0

```sh
./scripts/build-app-module.sh /path/to/nginx-source app.c
./scripts/run-example.sh /path/to/nginx-source
```

Application exports `nnx_register()`.

### v0.x

Introduce a dedicated tool:

```sh
nnx run app.c
```

The tool hides module compilation, configuration generation, temporary files,
and Nginx startup.

### v1 target

Explore:

```sh
nnxcc main.c -o server
./server
```

The resulting artifact behaves like a normal application launcher while
internally arranging for the application's handlers to be loaded into the
Nginx worker.

The exact packaging mechanism (sidecar module, embedded module payload, or
another compatible approach) should be decided separately.

## Invariants

Future implementations should preserve these rules unless another ADR
explicitly replaces this decision:

1. Application code must not need to include Nginx headers.
2. `ngx_*` types must not leak through the public nnx API.
3. We must not pretend a C function pointer survives an `exec()` boundary.
4. Nginx should remain an external dependency unless the project explicitly
   decides to own an embedded/forked Nginx runtime.
5. Build/runtime complexity should move into nnx tooling rather than into each
   user's application.
6. The framework should make process and blocking-I/O behavior explicit rather
   than hiding unsafe assumptions.
