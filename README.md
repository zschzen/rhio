<picture>
  <source media="(prefers-color-scheme: dark)" srcset="./assets/logo.light.svg">
  <img alt="rhio Logo" src="./assets/logo.dark.svg" width="100px" align="left">
</picture>

### `rhio`

[![License: zlib](https://img.shields.io/badge/License-zlib-blue.svg)](https://opensource.org/licenses/Zlib)
[![C99](https://img.shields.io/badge/Standard-C99-orange.svg)](https://en.wikipedia.org/wiki/C99)
[![Platform](https://img.shields.io/badge/Platform-Desktop%20%7C%20Web-brightgreen.svg)]()

A minimal, high-performance Rendering Hardware Interface (RHI) for a simpler world. 🍃

<div flex="true">
  <a href="examples/">
    Examples
  </a>
  &nbsp;|&nbsp;
  <a href="rhio.h">
    Documentation
  </a>
  &nbsp;|&nbsp;
  <a href="https://github.com/zschzen/rhio/issues">
    Report Bug
  </a>
</div>

<br/>

---

## Introduction

**rhio** is a lightweight, single-header abstraction layer designed to unify graphics backends under a consistent vtable-based API. It enables developers to write portable rendering code without the overhead of heavy frameworks or backend-specific boilerplate.

It currently defines a unified interface for **OpenGL 3.3**, **GLES 2.0/3.0**, and **Vulkan 1.4**.

## Features

- **Header-only Architecture**: Self-contained integration; implementation via `RHIO_IMPLEMENTATION`.
- **C99 Compliance**: Written in pure C99 for maximum portability across platforms.
- **Backend Agnostic**: Native support for runtime backend switching.
- **Minimal Overhead**: Thin vtable-based abstraction with a small binary footprint.
- **Extensible**: Straightforward vtable hooks for custom backend implementations.

## Getting Started

### Manual Installation

Simply copy `rhio.h` to your project. Define the implementation in exactly one source file:

```c
#define RHIO_IMPLEMENTATION
#include "rhio.h"

// ...
```

### CMake (via [CPM.cmake](https://github.com/cpm-cmake/cpm.cmake))

```cmake
CPMAddPackage(
  NAME rhio
  GITHUB_REPOSITORY zschzen/rhio
  GIT_TAG v0.0.1
)
target_link_libraries(my_project PRIVATE rhio::rhio)
```

## Usage Example

> [!CAUTION]
> `rhio` is a work in progress. APIs and backends may evolve rapidly, and breaking changes are to be expected.

```c
#include "rhio.h"

int main() {
    riContextInfo info = {
        .base = { .appName = "My App" },
        .backend = RI_BACKEND_OPENGL
    };

    riContext ctx = rhioCreate(&info);
    if (ctx != RI_SUCCESS) return -1;

    while (running) {
        rhioBeginFrame(ctx);
        {
          // Rendering logic...
        }
        rhioEndFrame(ctx);
        rhioPresent(ctx);
    }

    rhioDestroy(ctx);
    return 0;
}
```

## Roadmap

- [ ] **Core Backends**: Finalize OpenGL 3.3 and GLES 3.0 driver implementations.
- [ ] **Vulkan**: Progress towards Vulkan 1.4 compliance and validation.
- [ ] **Resource Management**: Implement Buffer, Texture, and PSO abstractions.
- [ ] **Command API**: Draw command recording and Render Pass management.

## Build Options

| Option | Description | Default |
|--------|-------------|---------|
| `RHIO_BACKEND_OPENGL` | Enable OpenGL/ES backends | `ON` |
| `RHIO_BACKEND_VULKAN` | Enable Vulkan backend | `OFF` |
| `LOG_SUPPORT` | Enable internal logging | `ON` |
| `RHIO_BUILD_EXAMPLES` | Build project examples | `ON` |

## 📜 License

**rhio** is licensed under the **zlib/libpng** license. See [LICENSE](LICENSE) for more information.

Copyright (c) 2026 - SOHNE, Leandro Peres (@zschzen).
