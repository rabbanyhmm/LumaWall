# Wayland Protocols

LumaWall dynamically discovers and utilizes various Wayland Extension Protocols via the `Registry`.

## Current Protocols

- **`wlr-layer-shell`** (`zwlr_layer_shell_v1`): Used by `WaylandWallpaperSurface` to anchor a surface to the bottom-most layer (`ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND`), bypassing standard window management. Supported primarily on wlroots-based compositors (Hyprland, Sway).
- **`xdg-output`** (Planned): To accurately retrieve monitor names and fractional scaling offsets.
- **`wp_fractional_scale_v1`** (Planned): To ensure the renderer creates Vulkan framebuffers at the precise logical size needed by the compositor.

## Integration

Protocols are handled by isolated C++ wrappers inside the `src/platform/wayland/protocols/` directory. They consume raw XML files tracked internally under `resources/protocols/` and generated via `wayland-scanner` in CMake.
