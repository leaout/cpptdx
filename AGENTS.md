# Repository Guidelines

## Project Structure & Module Organization

This repository implements a C++17 TDX stock-data API server for standard HQ and extended ExHq. Application startup and HTTP routes are in `src/`; TDX protocol and API code are in `src/tdx/`; request/response message definitions are in `src/msg/`. Public API declarations live in `include/tdxhq_api.h` and `include/tdxexhq_api.h`; DuckDB persistence is in `src/storage/`. Runtime settings are in `config/application.yml`, vendored headers are under `deps/`, and unit tests are in `tests/`.

## Build, Test, and Development Commands

Install the system dependencies listed in `README.md` (yaml-cpp, Boost, Asio, and zlib), then run from the repository root:

```sh
cmake -S . -B _build       # Configure
cmake --build _build -j    # Build server, library, and tests
./_build/tests/run_tests   # Run unit tests
./_build/tdxhq_server      # Start the API server (default port 8022)
```

The CI workflow uses the same configure/build/test sequence on Ubuntu. Edit `config/application.yml` to change server settings and TDX hosts.

## Coding Style & Naming Conventions

Use C++17 and follow the existing four-space indentation. Keep implementation and declarations in `.cpp`/`.h` pairs where practical. Use `snake_case` for files, functions, and variables (for example, `http_service.cpp` and `get_security_list`); use `PascalCase` for types (for example, `HttpServer`). Keep protocol parsing and message construction close to their existing modules. The project enables `-Wall -Wextra`; resolve new warnings.

## Testing Guidelines

Add focused tests to `tests/test_tdx_api.cpp` for protocol request construction and parsing, following the existing `test_<behavior>` function naming pattern. Build and run `./_build/tests/run_tests` before submitting. Tests use assertions and local sample data; keep them deterministic and independent of live TDX servers.

## Commit & Pull Request Guidelines

Recent commits use short, imperative summaries such as `Add tests and httplib dependency` and `Fix CI: release only on tags`. Follow that style and keep each commit focused. PRs should explain the behavior or fix, note configuration or API changes, link a related issue when available, and state the build/test commands run. Include request/response examples for API changes.
