#pragma once

/**
 * quantum_darwinism_solver.h
 * Pure C++26 Quantum Darwinism Einselection & Coherence Repair Solver (`src/core/quantum_darwinism/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES
 * ============================================================================
 * Operates on `ProceduralEnvironment` alongside `WFCSolverCore`:
 *   1. `isolate_horizon_box()`: Draws the bounded k-hop environment box around a conflicted slot.
 *   2. `establish_pointer_states()`: Evaluates `GenerationRulePOD` scores and neighbor witness
 *      redundancy (R_delta) to promote the most settled cells to `PointerStateRecordPOD`s.
 *   3. `evaluate_coherence_bridge_paths()`: Inspects intermediate cells between each Pointer State
 *      and the problem cell, using the CSS cascade layers (`PatternCascadeLayer`) to determine
 *      which child/sibling/variant properties can change without losing cell coherence.
 *   4. `propagate_pointer_coherence_to_conflict()`: Propagates pointer state properties inward
 *      along the bridge paths to resolve the conflict cell.
 */

#include "quantum_darwinism_common.h"
#include "../procedural_generation/constraint_graph_solver.h"
#include <vector>

namespace ideam::core {

class QuantumDarwinismSolver : public ConstraintGraphSolver {
private:
    HorizonBoxConfigPOD config_{};
    std::vector<PointerStateRecordPOD> pointer_states_;
    std::vector<CoherenceBridgeSlotPOD> bridge_slots_;
    EinselectionStatus einselection_status_ = EinselectionStatus::IDLE;

public:
    QuantumDarwinismSolver() = default;
    ~QuantumDarwinismSolver() override = default;

    void set_horizon_config(const HorizonBoxConfigPOD &p_cfg) noexcept { config_ = p_cfg; }
    [[nodiscard]] const HorizonBoxConfigPOD &horizon_config() const noexcept { return config_; }

    [[nodiscard]] const std::vector<PointerStateRecordPOD> &pointer_states() const noexcept { return pointer_states_; }
    [[nodiscard]] const std::vector<CoherenceBridgeSlotPOD> &bridge_slots() const noexcept { return bridge_slots_; }
    [[nodiscard]] EinselectionStatus einselection_status() const noexcept { return einselection_status_; }

    EinselectionStatus stabilize_conflict_in_environment(
        ProceduralEnvironment &r_environment,
        uint32_t p_conflict_slot_id
    );
};

} // namespace ideam::core
