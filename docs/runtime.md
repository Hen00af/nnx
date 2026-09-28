# Runtime design

nnx does not vendor Nginx.

The intended developer experience is:

```c
nnx_app *app = nnx_new();
nnx_get(app, "/", hello);
return nnx_run(app, 8080);
```

## Constraint

A C pointer such as `nnx_app *` cannot survive `exec()`. If nnx launches an
external Nginx executable, it cannot simply hand the in-memory router to the
new process.

That gives the runtime two honest implementation paths:

1. build/load an nnx-generated Nginx module containing application
   registration; or
2. keep application code in a separate process and have Nginx proxy to it.

The current project is pursuing the module path because the goal is to make
nnx a C framework backed directly by Nginx rather than another upstream HTTP
server hidden behind a reverse proxy.

`src/runtime.c` now owns external-Nginx discovery/process lifecycle. The next
step is generated configuration plus application registration at Nginx worker
initialization.
