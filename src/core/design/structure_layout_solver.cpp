#include "structure_layout_solver.h"

namespace ideam::core {

void StructureLayoutSolver::clear() noexcept {
    nodes_.clear();
    sockets_.clear();
}

uint32_t StructureLayoutSolver::add_structure_node(const StructureNodePod &p_node) {
    StructureNodePod copy = p_node;
    copy.node_id = static_cast<uint32_t>(nodes_.size());
    nodes_.push_back(copy);
    return copy.node_id;
}

uint32_t StructureLayoutSolver::add_socket(const SocketPod &p_socket) {
    SocketPod copy = p_socket;
    copy.socket_id = static_cast<uint32_t>(sockets_.size());
    sockets_.push_back(copy);
    return copy.socket_id;
}

void StructureLayoutSolver::solve_layout_constraints() {
    for (auto &node : nodes_) {
        if (node.parent_id < nodes_.size()) {
            const auto &parent = nodes_[node.parent_id];
            node.fitted_transform = parent.fitted_transform * node.fitted_transform;
        }
    }
}

bool StructureLayoutSolver::are_sockets_compatible(const SocketPod &p_a, const SocketPod &p_b) noexcept {
    return (p_a.compatibility_bitmask & p_b.compatibility_bitmask) != 0ULL;
}

} // namespace ideam::core
