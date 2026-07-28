# Synchronization

Vulkan synchronization is notoriously difficult. LumaWall simplifies this by leveraging **Timeline Semaphores** and the **Synchronization2** API from Vulkan 1.3.

## Timeline Semaphores
Legacy binary semaphores are difficult to reason about and can deadlock easily. Timeline semaphores act as a monotonically increasing 64-bit integer on the GPU.

### Usage
- `RenderScheduler` submits a frame and associates it with `Timeline == N`.
- The CPU can instantly query the GPU's current timeline value without blocking.
- When waiting for previous frames to finish, the CPU waits for `Timeline >= (N - MAX_FRAMES_IN_FLIGHT)`.

## CPU vs GPU Synchronization
We minimize CPU blocking (`vkQueueWaitIdle` or `vkDeviceWaitIdle`) heavily.
- **Fences**: Used primarily during swapchain acquisition (`vkAcquireNextImageKHR`) to throttle the CPU loop to the monitor's VSync interval.
- **Barriers**: Handled exclusively via `vkCmdPipelineBarrier2` to dictate strict image layout transitions (e.g., `UNDEFINED` -> `COLOR_ATTACHMENT_OPTIMAL`).
