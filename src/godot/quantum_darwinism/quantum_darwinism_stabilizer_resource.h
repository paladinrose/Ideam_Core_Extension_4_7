#pragma once

/**
 * quantum_darwinism_stabilizer_resource.h
 * `QuantumDarwinismStabilizerResource` and `QuantumDarwinismStabilizerNode`
 * (`src/godot/quantum_darwinism/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES (SECTION 1.5)
 * ============================================================================
 * Leans on `ProceduralGeneratorResource` and `ProceduralGenerationHost` from
 * `src/godot/procedural_generation/`, operating on the same `ProceduralPattern`,
 * `ProceduralEnvironment`, and `WFCSolverNode` as `IdeamWaveFunctionCollapse`.
 */

#include "../procedural_generation/procedural_generator_resource.h"
#include "../procedural_generation/procedural_pattern.h"
#include "../wave_function_collapse/wfc_rule_tensor_resource.h"
#include "../../core/quantum_darwinism/quantum_darwinism_solver.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>

namespace ideam::godot_ext {

class QuantumDarwinismStabilizerResource : public ProceduralGeneratorResource {
    GDCLASS(QuantumDarwinismStabilizerResource, ProceduralGeneratorResource)

private:
    int max_horizon_radius_          = 4;
    int max_horizon_slots_           = 128;
    float pointer_redundancy_thresh_ = 0.75f;
    float fragile_redundancy_thresh_ = 0.30f;
    int min_locked_layer_            = static_cast<int>(core::PatternCascadeLayer::LAYER_HIERARCHY_PARENT);
    bool allow_sibling_rebalancing_  = true;
    bool lock_nuclear_semes_         = true;
    godot::Ref<ProceduralPattern> pattern_;

protected:
    static void _bind_methods();

public:
    QuantumDarwinismStabilizerResource() = default;
    ~QuantumDarwinismStabilizerResource() override = default;

    void set_max_horizon_radius(int p_radius);
    [[nodiscard]] int get_max_horizon_radius() const noexcept { return max_horizon_radius_; }

    void set_max_horizon_slots(int p_max_slots);
    [[nodiscard]] int get_max_horizon_slots() const noexcept { return max_horizon_slots_; }

    void set_pointer_redundancy_threshold(float p_thresh);
    [[nodiscard]] float get_pointer_redundancy_threshold() const noexcept { return pointer_redundancy_thresh_; }

    void set_fragile_redundancy_threshold(float p_thresh);
    [[nodiscard]] float get_fragile_redundancy_threshold() const noexcept { return fragile_redundancy_thresh_; }

    void set_min_locked_layer(int p_layer);
    [[nodiscard]] int get_min_locked_layer() const noexcept { return min_locked_layer_; }

    void set_allow_sibling_rebalancing(bool p_allow);
    [[nodiscard]] bool get_allow_sibling_rebalancing() const noexcept { return allow_sibling_rebalancing_; }

    void set_lock_nuclear_semes(bool p_lock);
    [[nodiscard]] bool get_lock_nuclear_semes() const noexcept { return lock_nuclear_semes_; }

    void set_pattern(const godot::Ref<ProceduralPattern> &p_pattern) { pattern_ = p_pattern; emit_changed(); }
    [[nodiscard]] godot::Ref<ProceduralPattern> get_pattern() const { return pattern_; }

    [[nodiscard]] core::HorizonBoxConfigPOD to_pod() const noexcept;

    // --- Tier 1 `IdeamUndoRedo` Commit Wrappers ---
    void commit_set_max_horizon_radius(
        int p_new_radius,
        const godot::Ref<IdeamUndoRedo> &p_undo_redo = godot::Ref<IdeamUndoRedo>()
    );
    void commit_set_pointer_redundancy_threshold(
        float p_new_thresh,
        const godot::Ref<IdeamUndoRedo> &p_undo_redo = godot::Ref<IdeamUndoRedo>()
    );
};

class QuantumDarwinismStabilizerNode : public ProceduralGenerationHost {
    GDCLASS(QuantumDarwinismStabilizerNode, ProceduralGenerationHost)

private:
    core::QuantumDarwinismSolver solver_;
    godot::NodePath target_wfc_solver_path_;

    void sync_config_from_resource();
    [[nodiscard]] WFCSolverNode *resolve_target_wfc_node() const;

protected:
    static void _bind_methods();

public:
    QuantumDarwinismStabilizerNode() = default;
    ~QuantumDarwinismStabilizerNode() override = default;

    void set_target_wfc_solver_path(const godot::NodePath &p_path) { target_wfc_solver_path_ = p_path; }
    [[nodiscard]] godot::NodePath get_target_wfc_solver_path() const { return target_wfc_solver_path_; }

    // --- Einselection Stabilization Operations ---
    int stabilize_wfc_slot(int p_conflict_slot_id);
    int stabilize_all_wfc_conflicts();

    [[nodiscard]] int get_einselection_status() const noexcept {
        return static_cast<int>(solver_.einselection_status());
    }
    [[nodiscard]] godot::PackedInt32Array get_isolated_horizon_slots() const;
    [[nodiscard]] godot::Array get_pointer_states_as_dicts() const;
    [[nodiscard]] godot::Array get_coherence_bridge_slots_as_dicts() const;

    [[nodiscard]] core::QuantumDarwinismSolver &solver() noexcept { return solver_; }
    [[nodiscard]] const core::QuantumDarwinismSolver &solver() const noexcept { return solver_; }
};

} // namespace ideam::godot_ext
