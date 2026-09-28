# nnx

Experimental Echo-like C web framework powered by external Nginx.

```text
ngx_http_request_t
 -> nginx_adapter
 -> nnx_ctx
 -> nnx_match_route
 -> user handler
 -> nnx_send
 -> ngx_http_output_filter
```

Nginx is not vendored and public code never sees `ngx_*` types.

## Core
```sh
make test
```

Implemented: GET/POST registration, exact routing, Nginx URI/method dispatch and Nginx response output.

Next milestone: bind application registration to the Nginx worker lifecycle, then make the target `nnx_run(app, 8080)` experience real.
