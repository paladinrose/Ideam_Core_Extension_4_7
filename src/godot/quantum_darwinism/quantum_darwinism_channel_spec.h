#pragma once

/**
 * quantum_darwinism_channel_spec.h
 * Authoring resources for Godot-canvas Quantum Darwinism channels and cross-channel couplings:
 *   - `QuantumDarwinismChannelSpec`: Defines a reflected property, ClassDB archetype, group/tag,
 *     sememe, method/signal, or custom trait channel propagated across multi-topology wavefronts.
 *   - `QuantumDarwinismCouplingRule`: Defines how locking a source channel shifts threshold tau_m,
 *     permeability gamma_m, and value/state bias of a target channel across scoped relatives.
 */

#include "../../core/quantum_darwinism/quantum_darwinism_common.h"
#include "../utilities/ideam_undo_redo.h"
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace ideam::godot_ext {

class QuantumDarwinismChannelSpec : public godot::Resource {
    GDCLASS(QuantumDarwinismChannelSpec, godot::Resource)

private:
    godot::StringName channel_name_  = godot::StringName("trait_channel");
    int aspect_                      = static_cast<int>(core::QDChannelAspect::ASPECT_REFLECTED_PROPERTY);
    int topology_mask_               = static_cast<int>(core::TOPOLOGY_ALL);
    bool is_discrete_mask_           = false;
    bool is_nuclear_invariant_       = false;
    int state_count_                 = 4;
    int trait_bit_mask_              = 0;
    float default_threshold_tau_     = 0.65f;
    float default_permeability_gamma_ = 0.78f;
    float decay_per_hop_             = 0.14f;
    float value_min_                 = 0.0f;
    float value_max_                 = 1.0f;
    float default_value_             = 0.0f;
    godot::StringName help_topic_id_ = godot::StringName("QuantumDarwinismChannelSpec");
    godot::Ref<IdeamUndoRedo> undo_redo_;

    [[nodiscard]] bool _can_record_undo() const noexcept { return undo_redo_.is_valid(); }
    void _set_channel_name_direct(const godot::StringName &p_name);
    void _set_default_threshold_tau_direct(float p_tau);
    void _set_default_permeability_gamma_direct(float p_gamma);

protected:
    static void _bind_methods();

public:
    QuantumDarwinismChannelSpec() = default;
    ~QuantumDarwinismChannelSpec() override = default;

    void set_undo_redo(const godot::Ref<IdeamUndoRedo> &p_ur) { undo_redo_ = p_ur; }
    [[nodiscard]] godot::Ref<IdeamUndoRedo> get_undo_redo() const { return undo_redo_; }

    void set_channel_name(const godot::StringName &p_name);
    [[nodiscard]] godot::StringName get_channel_name() const { return channel_name_; }

    void set_aspect(int p_aspect);
    [[nodiscard]] int get_aspect() const noexcept { return aspect_; }

    void set_topology_mask(int p_mask);
    [[nodiscard]] int get_topology_mask() const noexcept { return topology_mask_; }

    void set_is_discrete_mask(bool p_discrete);
    [[nodiscard]] bool get_is_discrete_mask() const noexcept { return is_discrete_mask_; }

    void set_is_nuclear_invariant(bool p_invariant);
    [[nodiscard]] bool get_is_nuclear_invariant() const noexcept { return is_nuclear_invariant_; }

    void set_state_count(int p_count);
    [[nodiscard]] int get_state_count() const noexcept { return state_count_; }

    void set_trait_bit_mask(int p_mask);
    [[nodiscard]] int get_trait_bit_mask() const noexcept { return trait_bit_mask_; }

    void set_default_threshold_tau(float p_tau);
    [[nodiscard]] float get_default_threshold_tau() const noexcept { return default_threshold_tau_; }

    void set_default_permeability_gamma(float p_gamma);
    [[nodiscard]] float get_default_permeability_gamma() const noexcept { return default_permeability_gamma_; }

    void set_decay_per_hop(float p_decay);
    [[nodiscard]] float get_decay_per_hop() const noexcept { return decay_per_hop_; }

    void set_value_min(float p_min);
    [[nodiscard]] float get_value_min() const noexcept { return value_min_; }

    void set_value_max(float p_max);
    [[nodiscard]] float get_value_max() const noexcept { return value_max_; }

    void set_default_value(float p_val);
    [[nodiscard]] float get_default_value() const noexcept { return default_value_; }

    void set_help_topic_id(const godot::StringName &p_topic) { help_topic_id_ = p_topic; }
    [[nodiscard]] godot::StringName get_help_topic_id() const { return help_topic_id_; }

    // --- IdeamUndoRedo Commit Wrappers ---
    void commit_channel_name(const godot::StringName &p_name, const godot::String &p_action = "Rename QD Channel");
    void commit_default_threshold_tau(float p_tau, const godot::String &p_action = "Set QD Channel Threshold Tau");
    void commit_default_permeability_gamma(float p_gamma, const godot::String &p_action = "Set QD Channel Permeability");

    [[nodiscard]] core::PropertyChannelSpecPOD to_pod(uint32_t p_channel_id = 0u) const noexcept;
};

class QuantumDarwinismCouplingRule : public godot::Resource {
    GDCLASS(QuantumDarwinismCouplingRule, godot::Resource)

private:
    godot::StringName source_channel_name_;
    godot::StringName target_channel_name_;
    int required_source_state_mask_ = 0;
    float source_value_min_         = -1e30f;
    float source_value_max_         =  1e30f;
    float delta_threshold_tau_      = 0.0f;
    float delta_permeability_gamma_ = 0.0f;
    float target_value_bias_        = 0.0f;
    float value_bias_weight_        = 0.0f;
    int target_state_bias_mask_     = 0;
    int target_forbid_state_mask_   = 0;
    int scope_                      = static_cast<int>(core::PatternRuleScope::SELF);
    int topology_mask_              = static_cast<int>(core::TOPOLOGY_ALL);
    int max_coupling_hops_          = 1;
    int priority_                   = 0;
    godot::StringName help_topic_id_ = godot::StringName("QuantumDarwinismCouplingRule");
    godot::Ref<IdeamUndoRedo> undo_redo_;

    [[nodiscard]] bool _can_record_undo() const noexcept { return undo_redo_.is_valid(); }
    void _set_delta_threshold_tau_direct(float p_delta);
    void _set_delta_permeability_gamma_direct(float p_delta);

protected:
    static void _bind_methods();

public:
    QuantumDarwinismCouplingRule() = default;
    ~QuantumDarwinismCouplingRule() override = default;

    void set_undo_redo(const godot::Ref<IdeamUndoRedo> &p_ur) { undo_redo_ = p_ur; }
    [[nodiscard]] godot::Ref<IdeamUndoRedo> get_undo_redo() const { return undo_redo_; }

    void set_source_channel_name(const godot::StringName &p_name) { source_channel_name_ = p_name; emit_changed(); }
    [[nodiscard]] godot::StringName get_source_channel_name() const { return source_channel_name_; }

    void set_target_channel_name(const godot::StringName &p_name) { target_channel_name_ = p_name; emit_changed(); }
    [[nodiscard]] godot::StringName get_target_channel_name() const { return target_channel_name_; }

    void set_required_source_state_mask(int p_mask) { required_source_state_mask_ = p_mask; emit_changed(); }
    [[nodiscard]] int get_required_source_state_mask() const noexcept { return required_source_state_mask_; }

    void set_source_value_min(float p_min) { source_value_min_ = p_min; emit_changed(); }
    [[nodiscard]] float get_source_value_min() const noexcept { return source_value_min_; }

    void set_source_value_max(float p_max) { source_value_max_ = p_max; emit_changed(); }
    [[nodiscard]] float get_source_value_max() const noexcept { return source_value_max_; }

    void set_delta_threshold_tau(float p_delta);
    [[nodiscard]] float get_delta_threshold_tau() const noexcept { return delta_threshold_tau_; }

    void set_delta_permeability_gamma(float p_delta);
    [[nodiscard]] float get_delta_permeability_gamma() const noexcept { return delta_permeability_gamma_; }

    void set_target_value_bias(float p_bias) { target_value_bias_ = p_bias; emit_changed(); }
    [[nodiscard]] float get_target_value_bias() const noexcept { return target_value_bias_; }

    void set_value_bias_weight(float p_weight);
    [[nodiscard]] float get_value_bias_weight() const noexcept { return value_bias_weight_; }

    void set_target_state_bias_mask(int p_mask) { target_state_bias_mask_ = p_mask; emit_changed(); }
    [[nodiscard]] int get_target_state_bias_mask() const noexcept { return target_state_bias_mask_; }

    void set_target_forbid_state_mask(int p_mask) { target_forbid_state_mask_ = p_mask; emit_changed(); }
    [[nodiscard]] int get_target_forbid_state_mask() const noexcept { return target_forbid_state_mask_; }

    void set_scope(int p_scope);
    [[nodiscard]] int get_scope() const noexcept { return scope_; }

    void set_topology_mask(int p_mask);
    [[nodiscard]] int get_topology_mask() const noexcept { return topology_mask_; }

    void set_max_coupling_hops(int p_hops);
    [[nodiscard]] int get_max_coupling_hops() const noexcept { return max_coupling_hops_; }

    void set_priority(int p_prio);
    [[nodiscard]] int get_priority() const noexcept { return priority_; }

    void set_help_topic_id(const godot::StringName &p_topic) { help_topic_id_ = p_topic; }
    [[nodiscard]] godot::StringName get_help_topic_id() const { return help_topic_id_; }

    // --- IdeamUndoRedo Commit Wrappers ---
    void commit_delta_threshold_tau(float p_delta, const godot::String &p_action = "Set QD Coupling Delta Tau");
    void commit_delta_permeability_gamma(float p_delta, const godot::String &p_action = "Set QD Coupling Delta Gamma");

    [[nodiscard]] core::ChannelCouplingRulePOD to_pod(
        uint32_t p_rule_id,
        uint32_t p_source_channel_id,
        uint32_t p_target_channel_id
    ) const noexcept;
};

} // namespace ideam::godot_ext
