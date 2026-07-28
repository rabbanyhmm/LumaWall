#pragma once
#include <vector>
#include <memory>
#include <functional>
#include "RenderFrame.hpp"

namespace luma::render {

class RenderGraphNode {
public:
    virtual ~RenderGraphNode() = default;
    virtual void execute(const RenderFrame& frame) = 0;
};

class RenderGraph {
public:
    void addNode(std::unique_ptr<RenderGraphNode> node);
    void execute(const RenderFrame& frame);

private:
    std::vector<std::unique_ptr<RenderGraphNode>> m_nodes;
};

} // namespace luma::render
