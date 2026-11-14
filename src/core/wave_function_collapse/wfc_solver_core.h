#pragma once

/**
 * wfc_solver_core.h
 * Pure C++26 Hypergraph, Hierarchy, Cardinality & Spatial Wave Function Collapse Solver
 * (`src/core/wave_function_collapse/`), inheriting from `ideam::core::ConstraintGraphSolver`
 * (`src/core/procedural_generation/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES (SECTION 1.4)
 * ============================================================================
 * Operates over both standalone hypergraph slot topologies and the shared
 * `ProceduralEnvironment` (`src/core/procedural_generation/procedural_environment.h`)
 * where the domain space is the Godot environment itself (scene trees, resource graphs,
 * gameplay rule stacks, and whole-game configuration shapes).
 *
 * Simultaneously enforces:
 *   1. Directional port / socket / reflection / sememe rules (`WFCRuleEntryPOD`).
 *   2. Parent-child hierarchy masks (`ACCEPT_PARENTS`, e.g. `CastleTower` under `CastleGrounds | CastleRuins`).
 *   3. Child cardinality budgets (`ACCEPT_CHILDREN_CARDINALITY` `[min..max]`, e.g. `TowerGate [1..2]`).
 *   4. Sibling co-occurrence dependencies (`REQUIRE_SIBLING` / `FORBID_SIBLING`, e.g. `TowerLanding requires TowerStairs`).
 *   5. Contradiction auditing (`WFCContradictionReportPOD`) for zero-copy handoff to `IdeamQuantumDarwinism`.
 */

#include "wfc_common.h"
#include "../procedural_generation/constraint_graph_solver.h"
#include <span>
#include <vector>

namespace ideam::core {

class WFCSolverCore : public ConstraintGraphSolver {
private:
    std::vector<WFCRuleEntryPOD> rule_tensor_;
    std::vector<WFCDirectedEdgePOD> adjacency_edges_;
    std::vector<float> token_weights_;
    std::vector<uint8_t> slot_entropy_buckets_;
    std::vector<float> slot_entropies_;
    std::vector<WFCContradictionReportPOD> contradiction_reports_;
    std::vector<WFCCollapseStepRecordPOD> collapse_history_;

    WFCEntropyHeuristic entropy_heuristic_       = WFCEntropyHeuristic::HEURISTIC_SHANNON_WEIGHTED;
    uint32_t recommended_horizon_radius_         = 4u;
    bool enable_hierarchy_and_cardinality_rules_ = true;

    [[nodiscard]] uint32_t compute_slot_hierarchy_depth(uint32_t p_slot_id) const noexcept;

public:
    WFCSolverCore() = default;
    ~WFCSolverCore() override = default;

    // --- Configuration & Heuristics ---
    void set_entropy_heuristic(WFCEntropyHeuristic p_heuristic) noexcept { entropy_heuristic_ = p_heuristic; }
    [[nodiscard]] WFCEntropyHeuristic entropy_heuristic() const noexcept { return entropy_heuristic_; }

    void set_recommended_horizon_radius(uint32_t p_radius) noexcept { recommended_horizon_radius_ = p_radius; }
    [[nodiscard]] uint32_t recommended_horizon_radius() const noexcept { return recommended_horizon_radius_; }

    void set_enable_hierarchy_and_cardinality_rules(bool p_enable) noexcept {
        enable_hierarchy_and_cardinality_rules_ = p_enable;
    }
    [[nodiscard]] bool enable_hierarchy_and_cardinality_rules() const noexcept {
        return enable_hierarchy_and_cardinality_rules_;
    }

    void set_token_weights(std::span<const float> p_weights);
    [[nodiscard]] const std::vector<float> &token_weights() const noexcept { return token_weights_; }

    // --- Rule & Hypergraph Topology Management ---
    void clear_rules_and_edges() noexcept;
    void add_rule(
        uint32_t p_port_id,
        uint32_t p_source_state,
        const BitmaskDomain256POD &p_allowed_mask,
        WFCConstraintAxis p_axis = WFCConstraintAxis::AXIS_SPATIAL_PORT,
        float p_weight = 1.0f
    );
    void add_directed_edge(
        uint32_t p_from_slot,
        uint32_t p_to_slot,
        uint32_t p_port_id,
        WFCConstraintAxis p_axis = WFCConstraintAxis::AXIS_SPATIAL_PORT,
        bool p_bidirectional = false
    );

    /**
     * Compiles observed co-occurrence rules (`WFCObservedCoOccurrencePod`) into `rule_tensor_`
     * and (when bound to a `ProceduralEnvironment`) injects corresponding `ACCEPT_PARENTS`,
     * `ACCEPT_CHILDREN_CARDINALITY`, `REQUIRE_SIBLING`, and `FORBID_SIBLING` rules.
     */
    void compile_rules_from_observed_co_occurrences(std::span<const WFCObservedCoOccurrencePod> p_observed);

    /**
     * Synchronizes slots, domains, token weights, spatial neighbor CSR edges, and
     * cascading `GenerationRulePOD`s from the bound `ProceduralEnvironment`.
     */
    void initialize_from_bound_environment(uint64_t p_seed = 1337ULL);
    void sync_from_bound_environment();

    /**
     * Writes current `slot_domains_`, `collapsed_token_id`, `entropy_bucket`, `has_conflict`,
     * and Quantum Darwinism `einselection_score` ($R_\delta$) back to `bound_environment_`.
     */
    void sync_to_bound_environment();

    // --- Accessors ---
    [[nodiscard]] const std::vector<WFCRuleEntryPOD> &rule_tensor() const noexcept { return rule_tensor_; }
    [[nodiscard]] const std::vector<WFCDirectedEdgePOD> &adjacency_edges() const noexcept { return adjacency_edges_; }
    [[nodiscard]] const std::vector<uint8_t> &slot_entropy_buckets() const noexcept { return slot_entropy_buckets_; }
    [[nodiscard]] const std::vector<float> &slot_entropies() const noexcept { return slot_entropies_; }
    [[nodiscard]] const std::vector<WFCContradictionReportPOD> &contradiction_reports() const noexcept {
        return contradiction_reports_;
    }
    [[nodiscard]] const std::vector<WFCCollapseStepRecordPOD> &collapse_history() const noexcept {
        return collapse_history_;
    }

    // --- Entropy Partitioning & Sampling ---
    [[nodiscard]] float compute_slot_entropy(uint32_t p_slot_id) const noexcept;
    void partition_entropy_buckets() noexcept;
    [[nodiscard]] int32_t select_next_slot_to_collapse() noexcept;
    [[nodiscard]] int32_t sample_state_for_slot(uint32_t p_slot_id) noexcept;

    // --- Propagation, Contradiction Auditing & Forward Collapse ---
    /**
     * Runs fixed-point arc-consistency propagation across both directional port/reflection
     * edges (`rule_tensor_` + `adjacency_edges_`) AND cascading `ProceduralEnvironment`
     * hierarchy, cardinality (`[min..max]`), and sibling (`REQUIRE_SIBLING` / `FORBID_SIBLING`)
     * rules. Populates `contradiction_reports_` if any domain or cardinality constraint fails.
     */
    bool propagate_arc_consistency();

    /**
     * Audits all slots and parent hierarchies for domain emptiness, parent incompatibility,
     * cardinality budget violations, or missing required siblings, populating `contradiction_reports_`
     * and flagging `EnvironmentSlotPOD::has_conflict = 1` for `IdeamQuantumDarwinism`.
     */
    bool audit_contradictions();

    /**
     * Executes a single minimum-entropy collapse step followed by full arc-consistency propagation.
     */
    ProcGenSolverStatus step_collapse();

    /**
     * Collapses all slots using pinned landmark pre-collapse + unified hierarchy/cardinality/port
     * AC-3 wave propagation.
     */
    ProcGenSolverStatus solve_all(uint32_t p_max_steps = 4096u);
};

} // namespace ideam::core
