#pragma once

/**
 * quantum_darwinism_generator_resource.h
 * Standalone Godot-canvas Quantum Darwinism ProcGen resource and SceneTree host node:
 *   - `QuantumDarwinismGeneratorResource`: Full multi-channel outward generative configuration
 *     (channels, cross-channel coupling rules, perturbation, spontaneous nucleation, mode,
 *     and `IdeamUndoRedo` commit wrappers + propagation to sub-resources).
 *   - `QuantumDarwinismGeneratorNode`: Standalone `ProceduralGenerationHost` owning its own
 *     `ProceduralEnvironment` (with optional `WFCSolverNode` interop) and live Godot canvas
 *     write-back via `apply_resolved_channels_to_object_tree(Node *p_root)`.
 */

#include "quantum_darwinism_channel_spec.h"
#include "../procedural_generation/procedural_generator_resource.h"
#include "../procedural_generation/procedural_pattern.h"
#include "../wave_function_collapse/wfc_rule_tensor_resource.h"
#include "../../core/quantum_darwinism/quantum_darwinism_solver.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace ideam::godot_ext {

class QuantumDarwinismGeneratorResource : public ProceduralGeneratorResource {
    GDCLASS(QuantumDarwinismGeneratorResource, ProceduralGeneratorResource)

private:
    int solver_mode_                       = static_cast<int>(core::QuantumDarwinismMode::MODE_OUTWARD_GENERATIVE);
    int max_wavefront_iterations_          = 64;
    float perturbation_strength_           = 0.12f;
    float nucleation_temperature_          = 0.15f;
    float min_frontier_redundancy_         = 0.05f;
    float pointer_redundancy_threshold_    = 0.75f;
    bool enable_spontaneous_nucleation_    = true;
    bool enable_cross_channel_coupling_    = true;
    bool auto_stabilize_after_projection_  = true;
    godot::Ref<ProceduralPattern> pattern_;
    godot::TypedArray<QuantumDarwinismChannelSpec> channels_;
    godot::TypedArray<QuantumDarwinismCouplingRule> coupling_rules_;
    godot::StringName help_topic_id_       = godot::StringName("QuantumDarwinismGenerator");

    [[nodiscard]] bool _can_record_undo() const noexcept { return get_undo_redo().is_valid(); }
    void _set_solver_mode_direct(int p_mode);
    void _set_perturbation_strength_direct(float p_strength);
    void _set_nucleation_temperature_direct(float p_temp);
    void _set_channels_direct(const godot::TypedArray<QuantumDarwinismChannelSpec> &p_channels);
    void _set_coupling_rules_direct(const godot::TypedArray<QuantumDarwinismCouplingRule> &p_rules);

protected:
    static void _bind_methods();

public:
    QuantumDarwinismGeneratorResource() = default;
    ~QuantumDarwinismGeneratorResource() override = default;

    // --- Properties ---
    void set_solver_mode(int p_mode);
    [[nodiscard]] int get_solver_mode() const noexcept { return solver_mode_; }

    void set_max_wavefront_iterations(int p_iters);
    [[nodiscard]] int get_max_wavefront_iterations() const noexcept { return max_wavefront_iterations_; }

    void set_perturbation_strength(float p_strength);
    [[nodiscard]] float get_perturbation_strength() const noexcept { return perturbation_strength_; }

    void set_nucleation_temperature(float p_temp);
    [[nodiscard]] float get_nucleation_temperature() const noexcept { return nucleation_temperature_; }

    void set_min_frontier_redundancy(float p_min_r);
    [[nodiscard]] float get_min_frontier_redundancy() const noexcept { return min_frontier_redundancy_; }

    void set_pointer_redundancy_threshold(float p_thresh);
    [[nodiscard]] float get_pointer_redundancy_threshold() const noexcept { return pointer_redundancy_threshold_; }

    void set_enable_spontaneous_nucleation(bool p_enable);
    [[nodiscard]] bool get_enable_spontaneous_nucleation() const noexcept { return enable_spontaneous_nucleation_; }

    void set_enable_cross_channel_coupling(bool p_enable);
    [[nodiscard]] bool get_enable_cross_channel_coupling() const noexcept { return enable_cross_channel_coupling_; }

    void set_auto_stabilize_after_projection(bool p_auto);
    [[nodiscard]] bool get_auto_stabilize_after_projection() const noexcept { return auto_stabilize_after_projection_; }

    void set_pattern(const godot::Ref<ProceduralPattern> &p_pattern);
    [[nodiscard]] godot::Ref<ProceduralPattern> get_pattern() const { return pattern_; }

    void set_channels(const godot::TypedArray<QuantumDarwinismChannelSpec> &p_channels);
    [[nodiscard]] godot::TypedArray<QuantumDarwinismChannelSpec> get_channels() const { return channels_; }

    void set_coupling_rules(const godot::TypedArray<QuantumDarwinismCouplingRule> &p_rules);
    [[nodiscard]] godot::TypedArray<QuantumDarwinismCouplingRule> get_coupling_rules() const { return coupling_rules_; }

    void set_help_topic_id(const godot::StringName &p_topic) { help_topic_id_ = p_topic; }
    [[nodiscard]] godot::StringName get_help_topic_id() const { return help_topic_id_; }

    // --- Channel & Coupling Helpers ---
    [[nodiscard]] int find_channel_index(const godot::StringName &p_channel_name) const;
    void add_channel(const godot::Ref<QuantumDarwinismChannelSpec> &p_channel);
    void remove_channel(int p_index);
    void add_coupling_rule(const godot::Ref<QuantumDarwinismCouplingRule> &p_rule);
    void remove_coupling_rule(int p_index);

    // --- IdeamUndoRedo Propagation & Commit Wrappers ---
    void propagate_undo_redo(const godot::Ref<IdeamUndoRedo> &p_undo_redo);
    void commit_solver_mode(int p_new_mode, const godot::String &p_action = "Set QD Generator Mode");
    void commit_perturbation_strength(float p_strength, const godot::String &p_action = "Set QD Perturbation Strength");
    void commit_nucleation_temperature(float p_temp, const godot::String &p_action = "Set QD Nucleation Temperature");
    void commit_add_channel(const godot::Ref<QuantumDarwinismChannelSpec> &p_channel, const godot::String &p_action = "Add QD Channel");
    void commit_remove_channel(int p_index, const godot::String &p_action = "Remove QD Channel");
    void commit_add_coupling_rule(const godot::Ref<QuantumDarwinismCouplingRule> &p_rule, const godot::String &p_action = "Add QD Coupling Rule");
    void commit_remove_coupling_rule(int p_index, const godot::String &p_action = "Remove QD Coupling Rule");

    // --- Core Solver Compilation ---
    [[nodiscard]] core::GenerativeWavefrontConfigPOD to_generative_pod() const noexcept;
    [[nodiscard]] core::HorizonBoxConfigPOD to_horizon_pod() const noexcept;
    void apply_to_solver(core::QuantumDarwinismSolver &r_solver) const;
};

class QuantumDarwinismGeneratorNode : public ProceduralGenerationHost {
    GDCLASS(QuantumDarwinismGeneratorNode, ProceduralGenerationHost)

private:
    core::ProceduralEnvironment environment_;
    core::QuantumDarwinismSolver solver_;
    godot::Ref<ProceduralPattern> procedural_pattern_;
    godot::NodePath target_wfc_solver_path_;
    int slot_count_ = 16;

    void ensure_environment_and_solver_prepared();
    [[nodiscard]] WFCSolverNode *resolve_optional_wfc_node() const;
    [[nodiscard]] core::ProceduralEnvironment &active_environment();
    [[nodiscard]] const core::ProceduralEnvironment &active_environment() const;
    [[nodiscard]] godot::Ref<QuantumDarwinismGeneratorResource> resolve_generator_resource() const;

protected:
    static void _bind_methods();

public:
    QuantumDarwinismGeneratorNode();
    ~QuantumDarwinismGeneratorNode() override = default;

    void set_slot_count(int p_count);
    [[nodiscard]] int get_slot_count() const noexcept { return slot_count_; }

    void set_procedural_pattern(const godot::Ref<ProceduralPattern> &p_pattern);
    [[nodiscard]] godot::Ref<ProceduralPattern> get_procedural_pattern() const { return procedural_pattern_; }

    void set_target_wfc_solver_path(const godot::NodePath &p_path) { target_wfc_solver_path_ = p_path; }
    [[nodiscard]] godot::NodePath get_target_wfc_solver_path() const { return target_wfc_solver_path_; }

    // --- Godot-Canvas Topology & Landmark Seeding ---
    void configure_environment_shape(int p_slot_count, const godot::Array &p_slot_hierarchy_dicts);
    void set_slot_children(int p_parent_slot_id, const godot::PackedInt32Array &p_child_slot_ids);
    void add_directed_edge(int p_from_slot, int p_to_slot, int p_port_id, float p_distance_weight = 1.0f, bool p_bidirectional = false);
    void pin_landmark_state(int p_slot_id, int p_state_id);
    void pin_landmark_by_name(int p_slot_id, const godot::String &p_token_name);
    bool pin_slot_channel_value(int p_slot_id, const godot::StringName &p_channel_name, float p_value, int p_state_mask = 0);
    void set_perturbation_field(const godot::PackedFloat32Array &p_field);
    void clear_pinned_landmarks();

    // --- Generative Solving & Introspection ---
    int run_generative_einselection();
    bool step_generative_wave();
    bool project_channels_to_tokens();

    [[nodiscard]] int get_einselection_status() const noexcept { return static_cast<int>(solver_.einselection_status()); }
    [[nodiscard]] int get_current_wave_index() const noexcept { return static_cast<int>(solver_.current_wave_index()); }
    [[nodiscard]] godot::String get_collapsed_token_name(int p_slot_id) const;
    [[nodiscard]] godot::Array get_slot_channel_states_as_dicts(int p_slot_id) const;
    [[nodiscard]] godot::Array get_nucleation_events_as_dicts() const;
    [[nodiscard]] godot::Array export_solved_configuration_summary() const;

    /**
     * Applies resolved channel values, Godot groups, and metadata directly onto live nodes
     * under `p_root` (matching by `ideam_qd_slot_id` metadata when present, or BFS traversal order).
     * Returns a Dictionary telemetry summary (`applied_nodes`, `properties_written`, `groups_added`, `metadata_stamped`).
     */
    godot::Dictionary apply_resolved_channels_to_object_tree(godot::Node *p_root) const;

    [[nodiscard]] core::ProceduralEnvironment &environment() noexcept { return environment_; }
    [[nodiscard]] const core::ProceduralEnvironment &environment() const noexcept { return environment_; }
    [[nodiscard]] core::QuantumDarwinismSolver &solver() noexcept { return solver_; }
    [[nodiscard]] const core::QuantumDarwinismSolver &solver() const noexcept { return solver_; }
};

} // namespace ideam::godot_ext
