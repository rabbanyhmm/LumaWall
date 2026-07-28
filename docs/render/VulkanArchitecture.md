# Vulkan Architecture

LumaWall implements a Vulkan 1.3 backend focusing on extreme low-overhead rendering, heavily utilizing dynamic rendering and modern synchronization features.

## Philosophy
The overarching goal is to achieve an idle GPU/CPU state when the wallpaper is static, dropping CPU utilization to `< 0.2%` while still maintaining rapid responsiveness for video decoding.

## Physical Device Selection
The `VulkanPhysicalDevice` queries and scores all hardware GPUs. 
- Discrete GPUs are strongly preferred (+1000 score).
- Features like `VK_KHR_dynamic_rendering`, `timelineSemaphore`, and dedicated transfer queues are cached inside the `GpuCapabilities` struct to dynamically enable/disable execution paths.

## Validation Layers
Vulkan Validation Layers (`VK_LAYER_KHRONOS_validation`) are enabled implicitly in Debug builds via `VulkanInstance`, feeding directly into `spdlog` for centralized terminal output. Any validation error represents a hard failure condition for LumaWall's test suite.
