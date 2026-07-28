# X11 Native Backend Architecture

## Overview
LumaWall's X11 backend is designed using the native C `libxcb` library, completely bypassing the legacy `Xlib`. The backend focuses on achieving extreme low overhead and avoiding synchronous blocking calls by employing caching and a dedicated event loop thread.

## Components
- **Connection**: RAII managed connection to the X server (`xcb_connect`).
- **AtomCache**: Prefetches commonly used string-based atoms (`_NET_WM_WINDOW_TYPE_DESKTOP`, etc.) asynchronously on startup to prevent blocking roundtrips.
- **OutputManager**: Handles monitor detection using the X11 RandR extension (`xcb_randr`).
- **EventLoop**: Uses `poll()` on the XCB file descriptor to process events completely decoupled from the main thread.
- **DesktopWindow**: A `_NET_WM_WINDOW_TYPE_DESKTOP` window placed at the bottom of the stack (`_NET_WM_STATE_BELOW`), ensuring it behaves correctly as a desktop wallpaper across major window managers.

## RAII
We provide strict RAII wrappers for XCB types (`ScopedConnection`, `ScopedWindow`, `ScopedPixmap`) using standard C++ `unique_ptr` with custom deleters to guarantee no resource leaks or hanging file descriptors.
