#include "RenderGraph.hpp"
#include "IRenderContext.hpp"

namespace luma::render {

void RenderGraph::addNode(std::unique_ptr<RenderGraphNode> node) {
    if (node) {
        m_nodes.push_back(std::move(node));
    }
}

void RenderGraph::execute(const RenderFrame& frame) {
    if (m_nodes.empty()) return;

    frame.context->begin(frame);
    for (auto& node : m_nodes) {
        node->execute(frame);
    }
    frame.context->end();
    frame.context->submit(frame);
}

} // namespace luma::render
