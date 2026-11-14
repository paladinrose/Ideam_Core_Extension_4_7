#pragma once

/**
 * quantum_darwinism_solver.h
 * Pure C++26 Quantum Darwinism Einselection & Coherence Repair Solver (`src/core/quantum_darwinism/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES (SECTION 1.5)
 * ============================================================================
 * Operates on `ProceduralEnvironment` alongside `WFCSolverCore`:
 *   1. `isolate_horizon_box()`: Draws the bounded k-hop environment box (`E_box`) around
 *      conflicted slots across both spatial neighbor CSR edges and parent/child/sibling
 *      hierarchy links.
 *   2. `establish_pointer_states()`: Evaluates `GenerationRulePOD` specificity scores,
 *      `IdeamLinguistics` nuclear seme masks, and neighbor witness redundancy ($R_\delta$)
 *      to promote the most settled cells to immutable `PointerStateRecordPOD`s.
 *   3. `evaluate_coherence_bridge_paths()`: Inspects intermediate cells between each Pointer
 *      State and the problem cell, using the CSS cascade layers (`PatternCascadeLayer`) to
 *      determine which child/sibling/variant properties can change without losing cell coherence.
 *   4. `propagate_pointer_coherence_to_conflict()`: Relaxes lower-precedence leaf/cardinality
 *      layers along bridge paths while preserving high-precedence structural/nuclear layers,
 *      propagating Pointer State traits inward and resolving the conflict cell.
 */

#include "quantum_darwinism_common.h"
#include "../procedural_generation/constraint_graph_solver.h"
#include "../wave_function_collapse/wfc_solver_core.h"
#include <span>
#include <vector>

namespace ideam::core {

class QuantumDarwinismSolver : public ConstraintGraphSolver {
private:
    HorizonBoxConfigPOD config_{};
    std::vector<HorizonSlotPartitionPOD> horizon_partitions_;
    std::vector<uint32_t> horizon_slot_indices_;
    std::vector<PointerStateRecordPOD> pointer_states_;
    std::vector<CoherenceBridgeSlotPOD> bridge_slots_;
    EinselectionStatus einselection_status_ = EinselectionStatus::IDLE;

public:
    QuantumDarwinismSolver() = default;
    ~QuantumDarwinismSolver() override = default;

    void set_horizon_config(const HorizonBoxConfigPOD &p_cfg) noexcept { config_ = p_cfg; }
    [[nodiscard]] const HorizonBoxConfigPOD &horizon_config() const noexcept { return config_; }
    [[nodiscard]] HorizonBoxConfigPOD &horizon_config_mut() noexcept { return config_; }

    [[nodiscard]] const std::vector<HorizonSlotPartitionPOD> &horizon_partitions() const noexcept {
        return horizon_partitions_;
    }
    [[nodiscard]] const std::vector<uint32_t> &horizon_slot_indices() const noexcept {
        return horizon_slot_indices_;
    }
    [[nodiscard]] const std::vector<PointerStateRecordPOD> &pointer_states() const noexcept { return pointer_states_; }
    [[nodiscard]] const std::vector<CoherenceBridgeSlotPOD> &bridge_slots() const noexcept { return bridge_slots_; }
    [[nodiscard]] EinselectionStatus einselection_status() const noexcept { return einselection_status_; }

    // --- 4-Stage Quantum Darwinism Einselection Pipeline ---
    /**
     * Stage 1: Isolates a bounded k-hop Environment Horizon Box (`E_box`) around one or more
     * conflicted slots across parent, child, sibling, and spatial neighbor links.
     */
    bool isolate_horizon_box(
        const ProceduralEnvironment &p_environment,
        std::span<const uint32_t> p_conflict_slot_ids
    );

    /**
     * Stage 2: Computes Einselection Redundancy $R_\delta(i)$ across `E_box` and promotes
     * high-redundancy settled cells and perimeter anchors to `PointerStateRecordPOD`s.
     */
    bool establish_pointer_states(ProceduralEnvironment &r_environment);

    /**
     * Stage 3: Traces shortest paths inside `E_box` from each Pointer State toward its
     * nearest conflict slot, computing `CoherenceBridgeSlotPOD` relaxation masks.
     */
    bool evaluate_coherence_bridge_paths(const ProceduralEnvironment &p_environment);

    /**
     * Stage 4: Relaxes lower-precedence CSS cascade layers (`LAYER_CHILD_CARDINALITY`,
     * `LAYER_SIBLING_DEPENDENCY`, `LAYER_VARIANT_DETAIL`, and peripheral semes) on bridge
     * slots while locking `LAYER_DOMAIN`, `LAYER_HIERARCHY_PARENT`, `LAYER_PINNED_OVERRIDE`,
     * and nuclear semes, then propagates constraints inward to resolve the conflict.
     */
    EinselectionStatus propagate_pointer_coherence_to_conflict(
        ProceduralEnvironment &r_environment,
        WFCSolverCore *p_optional_wfc_solver = nullptr
    );

    /**
     * Runs the complete 4-stage stabilization pass around `p_conflict_slot_id`.
     */
    EinselectionStatus stabilize_conflict_in_environment(
        ProceduralEnvironment &r_environment,
        uint32_t p_conflict_slot_id
    );

    /**
     * Scans `r_environment` for all slots with `has_conflict != 0` or `domain.empty()`
     * and stabilizes them in a single pass.
     */
    EinselectionStatus stabilize_all_conflicts_in_environment(
        ProceduralEnvironment &r_environment,
        WFCSolverCore *p_optional_wfc_solver = nullptr
    );
};

} // namespace ideam::core
