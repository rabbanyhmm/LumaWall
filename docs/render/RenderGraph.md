# Render Graph

The Render Graph (`RenderGraph`) provides a Directed Acyclic Graph (DAG) for rendering execution.

## Motivation
Hardcoding rendering steps (e.g., upload textures -> render quad) makes adding post-processing (like blur or color correction) extremely difficult later. The `RenderGraph` decouples the *what* from the *how*.

## Node Execution
Each `RenderGraphNode` is executed sequentially (initially) inside the `RenderScheduler`'s frame loop. A node receives a `RenderFrame` context, which guarantees an active `IRenderContext` and an acquired `IRenderSurface`.

```cpp
class RenderGraphNode {
public:
    virtual void execute(const RenderFrame& frame) = 0;
};
```

## Future Extensions
- **Dependency Tracking**: Nodes will define their inputs (textures/buffers) and outputs (render targets), allowing the graph to automatically insert Vulkan memory barriers or reorder execution.
- **Pass Culling**: If a node's output isn't required for the final composite, it can be bypassed to save power.
