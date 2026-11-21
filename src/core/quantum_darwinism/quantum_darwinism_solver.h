#pragma once

/**
 * quantum_darwinism_solver.h
 * Pure C++26 Quantum Darwinism Dual-Mode Solver (`src/core/quantum_darwinism/`):
 *   1. Reactive WFC / ProcGen Conflict Stabilization (`MODE_CONFLICT_STABILIZER`)
 *   2. Standalone Godot-Canvas Outward Generative ProcGen (`MODE_OUTWARD_GENERATIVE`)
 *   3. Hybrid Outward Generation + Inward Coherence Repair (`MODE_HYBRID_GENERATE_AND_STABILIZE`)
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
    GenerativeWavefrontConfigPOD gen_config_{};

    // --- Conflict Stabilization State ---
    std::vector<HorizonSlotPartitionPOD> horizon_partitions_;
    std::vector<uint32_t> horizon_slot_indices_;
    std::vector<PointerStateRecordPOD> pointer_states_;
    std::vector<CoherenceBridgeSlotPOD> bridge_slots_;
    EinselectionStatus einselection_status_ = EinselectionStatus::IDLE;

    // --- Outward Generative Multi-Channel State ---
    std::vector<PropertyChannelSpecPOD> channel_specs_;
    std::vector<SlotChannelStatePOD> slot_channel_states_;
    std::vector<ChannelCouplingRulePOD> coupling_rules_;
    std::vector<WavefrontNucleationEventPOD> nucleation_events_;
    std::vector<float> external_perturbation_field_;
    uint32_t current_wave_index_ = 0;

    [[nodiscard]] float sample_perturbation(uint32_t p_slot_id, uint32_t p_channel_id, uint32_t p_salt = 0u) const noexcept;
    void apply_couplings_from_locked_channel(
        const ProceduralEnvironment &p_environment,
        uint32_t p_source_slot_id,
        uint32_t p_source_channel_id
    );
    bool nucleate_frontier_on_stall(const ProceduralEnvironment &p_environment);

public:
    QuantumDarwinismSolver() = default;
    ~QuantumDarwinismSolver() override = default;

    // --- Configuration ---
    void set_horizon_config(const HorizonBoxConfigPOD &p_cfg) noexcept { config_ = p_cfg; }
    [[nodiscard]] const HorizonBoxConfigPOD &horizon_config() const noexcept { return config_; }
    [[nodiscard]] HorizonBoxConfigPOD &horizon_config_mut() noexcept { return config_; }

    void set_generative_config(const GenerativeWavefrontConfigPOD &p_cfg) noexcept { gen_config_ = p_cfg; }
    [[nodiscard]] const GenerativeWavefrontConfigPOD &generative_config() const noexcept { return gen_config_; }
    [[nodiscard]] GenerativeWavefrontConfigPOD &generative_config_mut() noexcept { return gen_config_; }

    // --- Readback Accessors ---
    [[nodiscard]] const std::vector<HorizonSlotPartitionPOD> &horizon_partitions() const noexcept {
        return horizon_partitions_;
    }
    [[nodiscard]] const std::vector<uint32_t> &horizon_slot_indices() const noexcept {
        return horizon_slot_indices_;
    }
    [[nodiscard]] const std::vector<PointerStateRecordPOD> &pointer_states() const noexcept { return pointer_states_; }
    [[nodiscard]] const std::vector<CoherenceBridgeSlotPOD> &bridge_slots() const noexcept { return bridge_slots_; }
    [[nodiscard]] EinselectionStatus einselection_status() const noexcept { return einselection_status_; }

    [[nodiscard]] const std::vector<PropertyChannelSpecPOD> &channel_specs() const noexcept { return channel_specs_; }
    [[nodiscard]] const std::vector<SlotChannelStatePOD> &slot_channel_states() const noexcept { return slot_channel_states_; }
    [[nodiscard]] std::vector<SlotChannelStatePOD> &slot_channel_states_mut() noexcept { return slot_channel_states_; }
    [[nodiscard]] const std::vector<ChannelCouplingRulePOD> &coupling_rules() const noexcept { return coupling_rules_; }
    [[nodiscard]] const std::vector<WavefrontNucleationEventPOD> &nucleation_events() const noexcept { return nucleation_events_; }
    [[nodiscard]] uint32_t current_wave_index() const noexcept { return current_wave_index_; }

    // --- Channel & Coupling Setup (Godot-Canvas ProcGen) ---
    void clear_channels() noexcept;
    uint32_t add_channel_spec(const PropertyChannelSpecPOD &p_spec);
    void clear_coupling_rules() noexcept;
    uint32_t add_coupling_rule(const ChannelCouplingRulePOD &p_rule);
    void set_external_perturbation_field(std::span<const float> p_field);

    /**
     * Initializes `slot_channel_states_` for all slots in `p_environment`, seeding any
     * already-pinned (`LAYER_PINNED_OVERRIDE`) or collapsed slots as initial Pointer States ($R_\delta = 1.0$).
     */
    bool initialize_channel_states_from_environment(const ProceduralEnvironment &p_environment);

    /**
     * Explicitly seeds a single `(slot, channel)` pair as a pinned Landmark Pointer State ($R_\delta = 1.0$),
     * allowing partial-property seeding without locking the entire archetype token.
     */
    bool pin_slot_channel(
        uint32_t p_slot_id,
        uint32_t p_channel_id,
        float p_value,
        uint32_t p_state_mask = 0u
    );

    /**
     * Executes one outward multi-topology wavefront step across parent/child/sibling/graph edges,
     * applies cross-channel threshold/permeability couplings, and performs spontaneous nucleation if stalled.
     * Returns `true` if any channel progressed or crystallized; `false` once fully settled.
     */
    bool step_outward_wavefront(ProceduralEnvironment &r_environment);

    /**
     * Projects converged per-channel states back onto `EnvironmentSlotPOD::domain` by scoring
     * `TokenDescriptorPOD` reflection signatures and seme masks, then propagates hierarchy/cardinality rules.
     */
    bool project_channels_to_environment(ProceduralEnvironment &r_environment);

    /**
     * Runs the complete standalone outward generative einselection pipeline on `r_environment`.
     * If `auto_stabilize_after_projection` or `MODE_HYBRID_GENERATE_AND_STABILIZE` is active,
     * repairs any remaining cardinality/sibling conflicts via the 4-stage inward pass.
     */
    EinselectionStatus run_generative_einselection(
        ProceduralEnvironment &r_environment,
        WFCSolverCore *p_optional_wfc_solver = nullptr
    );

    // --- 4-Stage Quantum Darwinism Conflict Stabilization Pipeline (Inward Repair) ---
    bool isolate_horizon_box(
        const ProceduralEnvironment &p_environment,
        std::span<const uint32_t> p_conflict_slot_ids
    );
    bool establish_pointer_states(ProceduralEnvironment &r_environment);
    bool evaluate_coherence_bridge_paths(const ProceduralEnvironment &p_environment);
    EinselectionStatus propagate_pointer_coherence_to_conflict(
        ProceduralEnvironment &r_environment,
        WFCSolverCore *p_optional_wfc_solver = nullptr
    );
    EinselectionStatus stabilize_conflict_in_environment(
        ProceduralEnvironment &r_environment,
        uint32_t p_conflict_slot_id
    );
    EinselectionStatus stabilize_all_conflicts_in_environment(
        ProceduralEnvironment &r_environment,
        WFCSolverCore *p_optional_wfc_solver = nullptr
    );
};

} // namespace ideam::core
