# Platform Comparison

LumaWall implements unified backend abstractions via the `PlatformFactory` and `IPlatformBackend`. The design philosophy strictly decouples windowing logic from rendering logic.

| Feature | Wayland | X11 |
|---------|---------|-----|
| Client Library | `libwayland-client` | `libxcb` |
| Surface Creation | `wlr-layer-shell` | `_NET_WM_WINDOW_TYPE_DESKTOP` |
| Monitor Detection | `wl_output` | `xcb-randr` |
| Zero-Copy Video | Yes (`wp_linux_dmabuf`) | Limited (Shm / Deprecated APIs) |
| Isolation | Secure, protocol-driven | X Server global scope |

## Architecture & Integration
The Platform Layer acts exclusively as an adapter. The future `IRenderBackend` (Vulkan/OpenGL) and `FFmpeg` pipeline will communicate only with the generic `IWallpaperSurface` and `WallpaperManager`, completely unaware of Wayland or X11 mechanics.

## Limitations & Parity
- **Wayland**: Strictly relies on the `wlr-layer-shell` extension (standardized across wlroots compositors like Sway and Hyprland, and KDE). GNOME Wayland lacks native layer-shell support, meaning it may require an extension (like `layer-shell` GNOME extension) or a fallback to run LumaWall properly.
- **X11**: The `_NET_WM_WINDOW_TYPE_DESKTOP` window strategy works universally on EWMH-compliant window managers, placing the wallpaper window on the bottom-most layer. However, heavy composite managers (like Picom) can occasionally interfere with rendering.
- **Feature Parity**: Both backends provide `IWallpaperSurface` natively without leaking platform-specific headers, guaranteeing a single compile-time unified rendering pipeline.
