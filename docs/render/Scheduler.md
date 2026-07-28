# Render Scheduler

LumaWall's `RenderScheduler` is the beating heart of the graphics engine. Instead of the renderer calling `swap_buffers` when it's done, the `RenderScheduler` governs the exact moment a frame should begin rendering, based strictly on the monitor's refresh rate and hardware VBlank intervals.

## Core Responsibilities
1. **Frame Pacing**: Queries `IMonitor` for refresh rates (e.g., 60Hz = 16.6ms, 144Hz = 6.9ms).
2. **Throttling**: The scheduler thread sleeps using `std::this_thread::sleep_for` if the rendering finishes early. This guarantees our constraint of `<0.2% CPU` on static or low-FPS loops.
3. **Dropped Frame Handling**: If a frame takes longer than the allotted interval, the scheduler corrects the pacing clock to prevent cumulative drift.
4. **Graph Dispatching**: It drives the `RenderGraph` execution.

## Threading Model
The `RenderScheduler` spawns a dedicated worker thread `std::thread`. All rendering commands (via `IRenderContext`) are executed strictly on this thread, avoiding synchronization penalties.
