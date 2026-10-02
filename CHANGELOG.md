# Changelog

All notable changes to nnx will be documented in this file.

## [0.1.0] - 2026-10-02

First public release of the Nginx-native C web framework.

### Added

- Nginx dynamic-module runtime with worker-local nnx application registration
- Nginx-independent core / adapter boundary
- GET, POST, PUT, PATCH, DELETE, HEAD, OPTIONS, and Any route registration
- static, named-parameter, and wildcard route matching
- nested route groups and group-scoped middleware
- global middleware with before/after unwind semantics
- Logger, RequestID, CORS, BasicAuth, Secure, and BodyLimit middleware
- asynchronous Nginx request-body acquisition
- in-memory and temp-file-backed body support
- query, header, cookie, form, body, host, scheme, and client-IP request access
- request-scoped allocation and context key/value storage
- text, JSON, HTML, blob, redirect, header, and cookie responses
- centralized 404, 405, 413, and 500 handling
- HEAD-to-GET fallback and 405 Allow headers
- duplicate exact-route rejection and guarded middleware `nnx_next()`
- real Nginx integration tests
- ASan and UBSan test coverage
- Nginx worker crash/respawn isolation test
- one-command development runner for custom C applications

### Known limitations

- the public API is pre-v1 and may change;
- the development runner currently uses Nginx single-process mode;
- BodyLimit applies after Nginx finishes asynchronous body acquisition;
- blocking application I/O blocks the Nginx worker;
- external asynchronous DB/network I/O does not yet have an nnx API;
- multipart uploads, streaming, SSE, WebSocket helpers, and production packaging are not part of v0.1.
