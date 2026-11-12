#pragma once

/**
 * constraint_graph_solver.h
 * Base C++26 Constraint Graph & Domain Propagation Solver (`src/core/procedural_generation/`).
 *
 * Provides the shared `BitmaskDomain256POD` variable table, pinned landmark initialization,
 * and minimum-entropy slot selection that `WFCSolverCore` (`src/core/wave_function_collapse/`)
 * inherits and specializes.
 */

#include "procgen_common.h"
#include <vector>

namespace ideam::core {

class ConstraintGraphSolver {
protected:
    uint32_t state_count_ = 0;
    ProcGenSeedStreamPOD rng_{};
    std::vector<BitmaskDomain256POD> slot_domains_;
    std::vector<PinnedSlotConstraintPOD> pinned_landmarks_;
    ProcGenSolverStatus status_ = ProcGenSolverStatus::UNSOLVED;

public:
    ConstraintGraphSolver() = default;
    virtual ~ConstraintGraphSolver() = default;

    void configure_slots(uint32_t p_slot_count, uint32_t p_state_count, uint64_t p_seed = 1337ULL);
    void clear_pinned_landmarks() noexcept;
    void add_pinned_landmark(uint32_t p_slot_id, uint32_t p_state_id);

    [[nodiscard]] uint32_t slot_count() const noexcept { return static_cast<uint32_t>(slot_domains_.size()); }
    [[nodiscard]] uint32_t state_count() const noexcept { return state_count_; }
    [[nodiscard]] ProcGenSolverStatus status() const noexcept { return status_; }

    [[nodiscard]] const std::vector<BitmaskDomain256POD> &slot_domains() const noexcept { return slot_domains_; }
    [[nodiscard]] int32_t collapsed_state_at(uint32_t p_slot_id) const noexcept;

    /**
     * Applies all registered `pinned_landmarks_` (`domains[slot_A] = (1 << state_A)`)
     * before arc-consistency propagation begins.
     */
    bool apply_pinned_landmarks();

    /**
     * Selects the uncollapsed slot (> 1 remaining bits) with minimum remaining states.
     */
    [[nodiscard]] int32_t select_lowest_entropy_slot() noexcept;
};

} // namespace ideam::core
