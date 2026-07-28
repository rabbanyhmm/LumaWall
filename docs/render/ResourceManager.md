# Resource Manager

The `RenderResourceManager` guarantees predictable, leak-free GPU resource lifetimes. 

## Management Strategy
Raw handles like `ITexture`, `IBuffer`, and `IShader` are moved into the `RenderResourceManager` via `std::unique_ptr`. The manager assumes total ownership.

## Cleanup Protocol
Because rendering happens asynchronously on the GPU (particularly in Vulkan), resources cannot be freed the moment they go out of scope on the CPU. The `RenderResourceManager` will:
1. Defer destruction for N frames (inflight frames count).
2. Wait for `IRenderDevice::waitIdle()` during full engine shutdown to safely purge all GPU allocations.

## Extensibility
In future milestones, the resource manager will handle:
- Asynchronous threaded uploads (staging buffers).
- Zero-copy DMABUF imports for hardware video decoding.
- Memory pooling (VMA - Vulkan Memory Allocator integration).
