#pragma once

#include <memory>
#include <wayland-client.h>

namespace luma::platform::wayland::wrapper {

struct DisplayDeleter { void operator()(wl_display* p) const noexcept { wl_display_disconnect(p); } };
struct RegistryDeleter { void operator()(wl_registry* p) const noexcept { wl_registry_destroy(p); } };
struct SurfaceDeleter { void operator()(wl_surface* p) const noexcept { wl_surface_destroy(p); } };
struct CompositorDeleter { void operator()(wl_compositor* p) const noexcept { wl_compositor_destroy(p); } };
struct ShmDeleter { void operator()(wl_shm* p) const noexcept { wl_shm_destroy(p); } };
struct OutputDeleter { void operator()(wl_output* p) const noexcept { wl_output_destroy(p); } };

using ScopedDisplay = std::unique_ptr<wl_display, DisplayDeleter>;
using ScopedRegistry = std::unique_ptr<wl_registry, RegistryDeleter>;
using ScopedSurface = std::unique_ptr<wl_surface, SurfaceDeleter>;
using ScopedCompositor = std::unique_ptr<wl_compositor, CompositorDeleter>;
using ScopedShm = std::unique_ptr<wl_shm, ShmDeleter>;
using ScopedOutput = std::unique_ptr<wl_output, OutputDeleter>;

} // namespace luma::platform::wayland::wrapper
