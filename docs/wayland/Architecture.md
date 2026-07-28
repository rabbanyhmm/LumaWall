# Wayland Backend Architecture

The Wayland backend for LumaWall strictly bypasses Qt and relies directly on `libwayland-client` to achieve maximum control over surface layers, scaling, and zero-copy rendering.

## Core Modules

- **`core/DisplayConnection`**: Manages the `wl_display` lifecycle and file descriptors.
- **`core/EventLoop`**: A dedicated, non-blocking thread utilizing `poll()` and `eventfd` to read and dispatch `wl_display` events independently of the main thread.
- **`core/Registry`**: Parses globals advertised by the compositor to build `BackendCapabilities` dynamically.
- **`wrapper/ScopedProxy`**: A C++23 RAII abstraction guaranteeing leak-free destruction of raw `wl_proxy` objects via custom deleters.

## Event Driven

The `EventLoop` receives Wayland events and pushes them into the `EventBus`. The rendering threads or UI threads pull from the bus, ensuring thread-safety and no blocking bottlenecks.
