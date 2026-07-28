# LumaWall

**LumaWall** is a production-quality, open-source Linux live wallpaper engine designed to be extremely lightweight, utilizing modern C++23, Vulkan, and Qt6. It aims to be the native Wallpaper Engine equivalent for Linux desktop environments (Wayland and X11).

## Features

- **Extremely Low CPU Usage**: Hardware video decoding and zero-copy rendering.
- **Low RAM**: Memory pools and optimized textures keeping footprint under 120MB on average.
- **Vulkan Rendering**: Native Vulkan GPU rendering with OpenGL fallback.
- **Multi-Monitor**: Support for different wallpapers, refresh rates, and settings per monitor.
- **Power Efficient**: Automatically pauses on screen lock, fullscreen applications, or low battery.
- **Plugin System**: Extendable via C-API plugins for audio visualizers, widgets, and custom media decoders.

## Architecture

Please see the [Documentation](docs/) folder for architectural diagrams, API references, and plugin guides.

## Building

Requires CMake 3.25+, C++23 compatible compiler (GCC 13+ / Clang 16+), Qt6, Vulkan SDK, and FFmpeg.

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## License

This project is licensed under the GPLv3 License - see the [LICENSE](LICENSE) file for details.
