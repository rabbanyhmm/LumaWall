# Command Submission

Command recording inside LumaWall's Vulkan backend avoids rebuilding static commands where possible and strongly isolates workloads into specific queues.

## Dynamic Rendering
Instead of pre-compiling `VkRenderPass` and `VkFramebuffer` objects, LumaWall utilizes Vulkan 1.3 **Dynamic Rendering** (`VK_KHR_dynamic_rendering`). 
This drastically simplifies the `IRenderContext` by allowing us to begin a render pass by simply pointing to `VkImageView` attachments, reducing the need for sprawling render pass cache systems.

## Queue Families
LumaWall splits command submission across independent queues if the hardware supports it:
1. **Graphics Queue**: Heavy rendering (quads, post-processing).
2. **Transfer Queue**: Asynchronous texture uploads (`UploadContext`).
3. **Present Queue**: Frame presentation.

## Reusable Buffers
Command buffers are allocated from a thread-local `CommandPool`. When rendering static wallpapers, LumaWall preserves the recorded command buffer and resubmits it, completely skipping the CPU recording overhead until the scene mutates.
