#pragma once

/**
 * wfc_solver_core.h
 * Pure C++26 Hypergraph & Spatial Wave Function Collapse Solver (`src/core/wave_function_collapse/`),
 * inheriting from `ideam::core::ConstraintGraphSolver` (`src/core/procedural_generation/`).
 */

#include "wfc_common.h"
#include "../procedural_generation/constraint_graph_solver.h"
#include <vector>

namespace ideam::core {

class WFCSolverCore : public ConstraintGraphSolver {
private:
    std::vector<WFCRuleEntryPOD> rule_tensor_;
    std::vector<WFCDirectedEdgePOD> adjacency_edges_;

public:
    WFCSolverCore() = default;
    ~WFCSolverCore() override = default;

    void clear_rules_and_edges() noexcept;
    void add_rule(uint32_t p_port_id, uint32_t p_source_state, const BitmaskDomain256POD &p_allowed_mask);
    void add_directed_edge(uint32_t p_from_slot, uint32_t p_to_slot, uint32_t p_port_id);

    [[nodiscard]] const std::vector<WFCRuleEntryPOD> &rule_tensor() const noexcept { return rule_tensor_; }
    [[nodiscard]] const std::vector<WFCDirectedEdgePOD> &adjacency_edges() const noexcept { return adjacency_edges_; }

    /**
     * Runs one BFS arc-consistency wave from all pinned landmarks (`A, B, C`) or recently
     * collapsed slots.
     */
    bool propagate_arc_consistency();

    /**
     * Collapses all slots using pinned landmark pre-collapse + AC-3 wave propagation.
     */
    ProcGenSolverStatus solve_all(uint32_t p_max_steps = 4096u);
};

} // namespace ideam::core
