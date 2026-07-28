# Vulkan Memory Management

Raw Vulkan memory allocations (`vkAllocateMemory`) are strictly forbidden throughout LumaWall.

## Vulkan Memory Allocator (VMA)
LumaWall deeply integrates AMD's **Vulkan Memory Allocator (VMA)** as the single source of truth for all device allocations. VMA handles the sub-allocation of large `VkDeviceMemory` blocks into smaller pools, dramatically reducing API overhead and eliminating memory fragmentation.

## Allocator Lifecycle
The `VmaAllocator` is instantiated within `VulkanLogicalDevice` and passed downwards. 

## Memory Types
We categorize memory strictly based on usage patterns:
1. **Device Local** (`VMA_MEMORY_USAGE_GPU_ONLY`): Exclusively used for static textures and index/vertex buffers.
2. **Host Visible** (`VMA_MEMORY_USAGE_CPU_TO_GPU`): Used for staging buffers (upload pipelines) and uniform buffers updated every frame.
3. **DMABUF Imports** (Future): Zero-copy video decoding buffers will map directly to Vulkan textures without passing through VMA.
