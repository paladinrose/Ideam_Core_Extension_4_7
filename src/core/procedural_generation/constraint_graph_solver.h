#pragma once

/**
 * constraint_graph_solver.h
 * Base C++26 Constraint Graph & Domain Propagation Solver (`src/core/procedural_generation/`).
 *
 * Acts as the foundational constraint solver over `ProceduralEnvironment`.
 * Downstream solvers (`WFCSolverCore` for forward entropy collapse and
 * `QuantumDarwinismSolver` for einselection pointer-state conflict stabilization)
 * share this common domain, landmark, and `ProceduralEnvironment` binding contract.
 */

#include "procedural_environment.h"
#include <vector>

namespace ideam::core {

class ConstraintGraphSolver {
protected:
    uint32_t state_count_ = 0;
    ProcGenSeedStreamPOD rng_{};
    std::vector<BitmaskDomain256POD> slot_domains_;
    std::vector<PinnedSlotConstraintPOD> pinned_landmarks_;
    ProcGenSolverStatus status_ = ProcGenSolverStatus::UNSOLVED;
    ProceduralEnvironment *bound_environment_ = nullptr;

public:
    ConstraintGraphSolver() = default;
    virtual ~ConstraintGraphSolver() = default;

    void bind_environment(ProceduralEnvironment *p_env) noexcept;
    [[nodiscard]] ProceduralEnvironment *bound_environment() const noexcept { return bound_environment_; }

    void configure_slots(uint32_t p_slot_count, uint32_t p_state_count, uint64_t p_seed = 1337ULL);
    void clear_pinned_landmarks() noexcept;
    void add_pinned_landmark(uint32_t p_slot_id, uint32_t p_state_id);

    [[nodiscard]] uint32_t slot_count() const noexcept { return static_cast<uint32_t>(slot_domains_.size()); }
    [[nodiscard]] uint32_t state_count() const noexcept { return state_count_; }
    [[nodiscard]] ProcGenSolverStatus status() const noexcept { return status_; }

    [[nodiscard]] const std::vector<BitmaskDomain256POD> &slot_domains() const noexcept { return slot_domains_; }
    [[nodiscard]] std::vector<BitmaskDomain256POD> &slot_domains_mut() noexcept { return slot_domains_; }
    [[nodiscard]] int32_t collapsed_state_at(uint32_t p_slot_id) const noexcept;

    /**
     * Applies all registered `pinned_landmarks_` (`domains[slot_A] = (1 << state_A)`)
     * before arc-consistency propagation begins, and syncs to `bound_environment_` if bound.
     */
    bool apply_pinned_landmarks();

    /**
     * Synchronizes `slot_domains_` with `bound_environment_` and runs
     * `ProceduralEnvironment::propagate_hierarchy_and_cardinality_constraints()`.
     */
    bool sync_and_propagate_environment_rules();

    /**
     * Selects the uncollapsed slot (> 1 remaining bits) with minimum remaining states,
     * using deterministic SplitMix64 tie-breaking.
     */
    [[nodiscard]] int32_t select_lowest_entropy_slot() noexcept;
};

} // namespace ideam::core
