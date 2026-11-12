#pragma once

/**
 * structure_layout_solver.h
 * Pure C++26 1D/2D/3D Container & Socket Contact Constraint Solver (`src/core/design/`).
 */

#include "design_common.h"
#include <vector>

namespace ideam::core {

class StructureLayoutSolver {
private:
    std::vector<StructureNodePod> nodes_;
    std::vector<SocketPod> sockets_;

public:
    StructureLayoutSolver() = default;
    ~StructureLayoutSolver() = default;

    void clear() noexcept;

    uint32_t add_structure_node(const StructureNodePod &p_node);
    uint32_t add_socket(const SocketPod &p_socket);

    [[nodiscard]] const std::vector<StructureNodePod> &nodes() const noexcept { return nodes_; }
    [[nodiscard]] std::vector<StructureNodePod> &nodes_mut() noexcept { return nodes_; }
    [[nodiscard]] const std::vector<SocketPod> &sockets() const noexcept { return sockets_; }

    /**
     * Solves 1D/2D/3D container fitting and socket contact constraints across `nodes_`.
     */
    void solve_layout_constraints();

    /**
     * Checks if two sockets can mate (compatible bitmasks & opposing normals).
     */
    [[nodiscard]] static bool are_sockets_compatible(const SocketPod &p_a, const SocketPod &p_b) noexcept;
};

} // namespace ideam::core
