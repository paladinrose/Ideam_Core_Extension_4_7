#include "quantum_darwinism_channel_spec.h"
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <algorithm>

namespace ideam::godot_ext {

// ============================================================================
// QuantumDarwinismChannelSpec
// ============================================================================

void QuantumDarwinismChannelSpec::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_undo_redo", "undo_redo"), &QuantumDarwinismChannelSpec::set_undo_redo);
    ClassDB::bind_method(D_METHOD("get_undo_redo"), &QuantumDarwinismChannelSpec::get_undo_redo);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "undo_redo", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_undo_redo", "get_undo_redo");

    ClassDB::bind_method(D_METHOD("set_channel_name", "name"), &QuantumDarwinismChannelSpec::set_channel_name);
    ClassDB::bind_method(D_METHOD("get_channel_name"), &QuantumDarwinismChannelSpec::get_channel_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "channel_name"), "set_channel_name", "get_channel_name");

    ClassDB::bind_method(D_METHOD("set_aspect", "aspect"), &QuantumDarwinismChannelSpec::set_aspect);
    ClassDB::bind_method(D_METHOD("get_aspect"), &QuantumDarwinismChannelSpec::get_aspect);
    ADD_PROPERTY(
        PropertyInfo(
            Variant::INT,
            "aspect",
            PROPERTY_HINT_ENUM,
            "ReflectedProperty:0,ClassArchetype:1,GroupOrTag:2,NuclearSeme:3,PeripheralSeme:4,MethodTrait:5,SignalTrait:6,CustomTrait:7"
        ),
        "set_aspect",
        "get_aspect"
    );

    ClassDB::bind_method(D_METHOD("set_topology_mask", "mask"), &QuantumDarwinismChannelSpec::set_topology_mask);
    ClassDB::bind_method(D_METHOD("get_topology_mask"), &QuantumDarwinismChannelSpec::get_topology_mask);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "topology_mask", PROPERTY_HINT_FLAGS, "HierarchyDown,HierarchyUp,Siblings,GraphOrSpatial"),
        "set_topology_mask",
        "get_topology_mask"
    );

    ClassDB::bind_method(D_METHOD("set_is_discrete_mask", "discrete"), &QuantumDarwinismChannelSpec::set_is_discrete_mask);
    ClassDB::bind_method(D_METHOD("get_is_discrete_mask"), &QuantumDarwinismChannelSpec::get_is_discrete_mask);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "is_discrete_mask"), "set_is_discrete_mask", "get_is_discrete_mask");

    ClassDB::bind_method(D_METHOD("set_is_nuclear_invariant", "invariant"), &QuantumDarwinismChannelSpec::set_is_nuclear_invariant);
    ClassDB::bind_method(D_METHOD("get_is_nuclear_invariant"), &QuantumDarwinismChannelSpec::get_is_nuclear_invariant);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "is_nuclear_invariant"), "set_is_nuclear_invariant", "get_is_nuclear_invariant");

    ClassDB::bind_method(D_METHOD("set_state_count", "count"), &QuantumDarwinismChannelSpec::set_state_count);
    ClassDB::bind_method(D_METHOD("get_state_count"), &QuantumDarwinismChannelSpec::get_state_count);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "state_count", PROPERTY_HINT_RANGE, "1,32,1"), "set_state_count", "get_state_count");

    ClassDB::bind_method(D_METHOD("set_trait_bit_mask", "mask"), &QuantumDarwinismChannelSpec::set_trait_bit_mask);
    ClassDB::bind_method(D_METHOD("get_trait_bit_mask"), &QuantumDarwinismChannelSpec::get_trait_bit_mask);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "trait_bit_mask"), "set_trait_bit_mask", "get_trait_bit_mask");

    ClassDB::bind_method(D_METHOD("set_default_threshold_tau", "tau"), &QuantumDarwinismChannelSpec::set_default_threshold_tau);
    ClassDB::bind_method(D_METHOD("get_default_threshold_tau"), &QuantumDarwinismChannelSpec::get_default_threshold_tau);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "default_threshold_tau", PROPERTY_HINT_RANGE, "0.05,0.99,0.01"), "set_default_threshold_tau", "get_default_threshold_tau");

    ClassDB::bind_method(D_METHOD("set_default_permeability_gamma", "gamma"), &QuantumDarwinismChannelSpec::set_default_permeability_gamma);
    ClassDB::bind_method(D_METHOD("get_default_permeability_gamma"), &QuantumDarwinismChannelSpec::get_default_permeability_gamma);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "default_permeability_gamma", PROPERTY_HINT_RANGE, "0.05,1.0,0.01"), "set_default_permeability_gamma", "get_default_permeability_gamma");

    ClassDB::bind_method(D_METHOD("set_decay_per_hop", "decay"), &QuantumDarwinismChannelSpec::set_decay_per_hop);
    ClassDB::bind_method(D_METHOD("get_decay_per_hop"), &QuantumDarwinismChannelSpec::get_decay_per_hop);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "decay_per_hop", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_decay_per_hop", "get_decay_per_hop");

    ClassDB::bind_method(D_METHOD("set_value_min", "min"), &QuantumDarwinismChannelSpec::set_value_min);
    ClassDB::bind_method(D_METHOD("get_value_min"), &QuantumDarwinismChannelSpec::get_value_min);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "value_min"), "set_value_min", "get_value_min");

    ClassDB::bind_method(D_METHOD("set_value_max", "max"), &QuantumDarwinismChannelSpec::set_value_max);
    ClassDB::bind_method(D_METHOD("get_value_max"), &QuantumDarwinismChannelSpec::get_value_max);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "value_max"), "set_value_max", "get_value_max");

    ClassDB::bind_method(D_METHOD("set_default_value", "val"), &QuantumDarwinismChannelSpec::set_default_value);
    ClassDB::bind_method(D_METHOD("get_default_value"), &QuantumDarwinismChannelSpec::get_default_value);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "default_value"), "set_default_value", "get_default_value");

    ClassDB::bind_method(D_METHOD("set_help_topic_id", "topic_id"), &QuantumDarwinismChannelSpec::set_help_topic_id);
    ClassDB::bind_method(D_METHOD("get_help_topic_id"), &QuantumDarwinismChannelSpec::get_help_topic_id);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "help_topic_id"), "set_help_topic_id", "get_help_topic_id");

    ClassDB::bind_method(D_METHOD("commit_channel_name", "name", "action"), &QuantumDarwinismChannelSpec::commit_channel_name, DEFVAL(String("Rename QD Channel")));
    ClassDB::bind_method(D_METHOD("commit_default_threshold_tau", "tau", "action"), &QuantumDarwinismChannelSpec::commit_default_threshold_tau, DEFVAL(String("Set QD Channel Threshold Tau")));
    ClassDB::bind_method(D_METHOD("commit_default_permeability_gamma", "gamma", "action"), &QuantumDarwinismChannelSpec::commit_default_permeability_gamma, DEFVAL(String("Set QD Channel Permeability")));
}

void QuantumDarwinismChannelSpec::_set_channel_name_direct(const godot::StringName &p_name) {
    if (channel_name_ == p_name) return;
    channel_name_ = p_name;
    emit_changed();
}

void QuantumDarwinismChannelSpec::_set_default_threshold_tau_direct(float p_tau) {
    const float clamped = std::clamp(p_tau, 0.05f, 0.99f);
    if (default_threshold_tau_ == clamped) return;
    default_threshold_tau_ = clamped;
    emit_changed();
}

void QuantumDarwinismChannelSpec::_set_default_permeability_gamma_direct(float p_gamma) {
    const float clamped = std::clamp(p_gamma, 0.05f, 1.0f);
    if (default_permeability_gamma_ == clamped) return;
    default_permeability_gamma_ = clamped;
    emit_changed();
}

void QuantumDarwinismChannelSpec::set_channel_name(const godot::StringName &p_name) { _set_channel_name_direct(p_name); }
void QuantumDarwinismChannelSpec::set_aspect(int p_aspect) { aspect_ = std::clamp(p_aspect, 0, 7); emit_changed(); }
void QuantumDarwinismChannelSpec::set_topology_mask(int p_mask) { topology_mask_ = p_mask & 0x0F; emit_changed(); }
void QuantumDarwinismChannelSpec::set_is_discrete_mask(bool p_discrete) { is_discrete_mask_ = p_discrete; emit_changed(); }
void QuantumDarwinismChannelSpec::set_is_nuclear_invariant(bool p_invariant) { is_nuclear_invariant_ = p_invariant; emit_changed(); }
void QuantumDarwinismChannelSpec::set_state_count(int p_count) { state_count_ = std::clamp(p_count, 1, 32); emit_changed(); }
void QuantumDarwinismChannelSpec::set_trait_bit_mask(int p_mask) { trait_bit_mask_ = p_mask; emit_changed(); }
void QuantumDarwinismChannelSpec::set_default_threshold_tau(float p_tau) { _set_default_threshold_tau_direct(p_tau); }
void QuantumDarwinismChannelSpec::set_default_permeability_gamma(float p_gamma) { _set_default_permeability_gamma_direct(p_gamma); }
void QuantumDarwinismChannelSpec::set_decay_per_hop(float p_decay) { decay_per_hop_ = std::clamp(p_decay, 0.0f, 1.0f); emit_changed(); }
void QuantumDarwinismChannelSpec::set_value_min(float p_min) { value_min_ = p_min; emit_changed(); }
void QuantumDarwinismChannelSpec::set_value_max(float p_max) { value_max_ = p_max; emit_changed(); }
void QuantumDarwinismChannelSpec::set_default_value(float p_val) { default_value_ = p_val; emit_changed(); }

void QuantumDarwinismChannelSpec::commit_channel_name(const godot::StringName &p_name, const godot::String &p_action) {
    if (channel_name_ == p_name) return;
    if (_can_record_undo()) {
        const godot::StringName old_v = channel_name_;
        undo_redo_->create_action(p_action.is_empty() ? godot::String("Rename QD Channel") : p_action);
        undo_redo_->add_do_method(callable_mp(this, &QuantumDarwinismChannelSpec::_set_channel_name_direct).bind(p_name));
        undo_redo_->add_undo_method(callable_mp(this, &QuantumDarwinismChannelSpec::_set_channel_name_direct).bind(old_v));
        undo_redo_->commit_action();
    } else {
        _set_channel_name_direct(p_name);
    }
}

void QuantumDarwinismChannelSpec::commit_default_threshold_tau(float p_tau, const godot::String &p_action) {
    const float clamped = std::clamp(p_tau, 0.05f, 0.99f);
    if (default_threshold_tau_ == clamped) return;
    if (_can_record_undo()) {
        const float old_v = default_threshold_tau_;
        undo_redo_->create_action(p_action.is_empty() ? godot::String("Set QD Channel Threshold Tau") : p_action);
        undo_redo_->add_do_method(callable_mp(this, &QuantumDarwinismChannelSpec::_set_default_threshold_tau_direct).bind(clamped));
        undo_redo_->add_undo_method(callable_mp(this, &QuantumDarwinismChannelSpec::_set_default_threshold_tau_direct).bind(old_v));
        undo_redo_->commit_action();
    } else {
        _set_default_threshold_tau_direct(clamped);
    }
}

void QuantumDarwinismChannelSpec::commit_default_permeability_gamma(float p_gamma, const godot::String &p_action) {
    const float clamped = std::clamp(p_gamma, 0.05f, 1.0f);
    if (default_permeability_gamma_ == clamped) return;
    if (_can_record_undo()) {
        const float old_v = default_permeability_gamma_;
        undo_redo_->create_action(p_action.is_empty() ? godot::String("Set QD Channel Permeability") : p_action);
        undo_redo_->add_do_method(callable_mp(this, &QuantumDarwinismChannelSpec::_set_default_permeability_gamma_direct).bind(clamped));
        undo_redo_->add_undo_method(callable_mp(this, &QuantumDarwinismChannelSpec::_set_default_permeability_gamma_direct).bind(old_v));
        undo_redo_->commit_action();
    } else {
        _set_default_permeability_gamma_direct(clamped);
    }
}

core::PropertyChannelSpecPOD QuantumDarwinismChannelSpec::to_pod(uint32_t p_channel_id) const noexcept {
    core::PropertyChannelSpecPOD pod{};
    pod.channel_id                 = p_channel_id;
    const godot::CharString utf8   = godot::String(channel_name_).utf8();
    pod.symbol_hash                = core::hash_reflection_symbol(std::string_view(utf8.get_data(), static_cast<size_t>(utf8.length())));
    pod.trait_bit_mask             = static_cast<uint32_t>(trait_bit_mask_);
    pod.default_threshold_tau      = default_threshold_tau_;
    pod.default_permeability_gamma = default_permeability_gamma_;
    pod.decay_per_hop              = decay_per_hop_;
    pod.value_min                  = value_min_;
    pod.value_max                  = std::max(value_min_, value_max_);
    pod.default_value              = std::clamp(default_value_, pod.value_min, pod.value_max);
    pod.state_count                = static_cast<uint32_t>(std::clamp(state_count_, 1, 32));
    pod.aspect                     = static_cast<core::QDChannelAspect>(std::clamp(aspect_, 0, 7));
    pod.topology_mask              = static_cast<uint8_t>(topology_mask_ & 0x0F);
    pod.is_discrete_mask           = is_discrete_mask_ ? 1u : 0u;
    pod.is_nuclear_invariant       = is_nuclear_invariant_ ? 1u : 0u;
    return pod;
}

// ============================================================================
// QuantumDarwinismCouplingRule
// ============================================================================

void QuantumDarwinismCouplingRule::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_undo_redo", "undo_redo"), &QuantumDarwinismCouplingRule::set_undo_redo);
    ClassDB::bind_method(D_METHOD("get_undo_redo"), &QuantumDarwinismCouplingRule::get_undo_redo);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "undo_redo", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_undo_redo", "get_undo_redo");

    ClassDB::bind_method(D_METHOD("set_source_channel_name", "name"), &QuantumDarwinismCouplingRule::set_source_channel_name);
    ClassDB::bind_method(D_METHOD("get_source_channel_name"), &QuantumDarwinismCouplingRule::get_source_channel_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "source_channel_name"), "set_source_channel_name", "get_source_channel_name");

    ClassDB::bind_method(D_METHOD("set_target_channel_name", "name"), &QuantumDarwinismCouplingRule::set_target_channel_name);
    ClassDB::bind_method(D_METHOD("get_target_channel_name"), &QuantumDarwinismCouplingRule::get_target_channel_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "target_channel_name"), "set_target_channel_name", "get_target_channel_name");

    ClassDB::bind_method(D_METHOD("set_required_source_state_mask", "mask"), &QuantumDarwinismCouplingRule::set_required_source_state_mask);
    ClassDB::bind_method(D_METHOD("get_required_source_state_mask"), &QuantumDarwinismCouplingRule::get_required_source_state_mask);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "required_source_state_mask"), "set_required_source_state_mask", "get_required_source_state_mask");

    ClassDB::bind_method(D_METHOD("set_source_value_min", "min"), &QuantumDarwinismCouplingRule::set_source_value_min);
    ClassDB::bind_method(D_METHOD("get_source_value_min"), &QuantumDarwinismCouplingRule::get_source_value_min);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "source_value_min"), "set_source_value_min", "get_source_value_min");

    ClassDB::bind_method(D_METHOD("set_source_value_max", "max"), &QuantumDarwinismCouplingRule::set_source_value_max);
    ClassDB::bind_method(D_METHOD("get_source_value_max"), &QuantumDarwinismCouplingRule::get_source_value_max);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "source_value_max"), "set_source_value_max", "get_source_value_max");

    ClassDB::bind_method(D_METHOD("set_delta_threshold_tau", "delta"), &QuantumDarwinismCouplingRule::set_delta_threshold_tau);
    ClassDB::bind_method(D_METHOD("get_delta_threshold_tau"), &QuantumDarwinismCouplingRule::get_delta_threshold_tau);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "delta_threshold_tau", PROPERTY_HINT_RANGE, "-1.0,1.0,0.01"), "set_delta_threshold_tau", "get_delta_threshold_tau");

    ClassDB::bind_method(D_METHOD("set_delta_permeability_gamma", "delta"), &QuantumDarwinismCouplingRule::set_delta_permeability_gamma);
    ClassDB::bind_method(D_METHOD("get_delta_permeability_gamma"), &QuantumDarwinismCouplingRule::get_delta_permeability_gamma);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "delta_permeability_gamma", PROPERTY_HINT_RANGE, "-1.0,1.0,0.01"), "set_delta_permeability_gamma", "get_delta_permeability_gamma");

    ClassDB::bind_method(D_METHOD("set_target_value_bias", "bias"), &QuantumDarwinismCouplingRule::set_target_value_bias);
    ClassDB::bind_method(D_METHOD("get_target_value_bias"), &QuantumDarwinismCouplingRule::get_target_value_bias);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "target_value_bias"), "set_target_value_bias", "get_target_value_bias");

    ClassDB::bind_method(D_METHOD("set_value_bias_weight", "weight"), &QuantumDarwinismCouplingRule::set_value_bias_weight);
    ClassDB::bind_method(D_METHOD("get_value_bias_weight"), &QuantumDarwinismCouplingRule::get_value_bias_weight);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "value_bias_weight", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_value_bias_weight", "get_value_bias_weight");

    ClassDB::bind_method(D_METHOD("set_target_state_bias_mask", "mask"), &QuantumDarwinismCouplingRule::set_target_state_bias_mask);
    ClassDB::bind_method(D_METHOD("get_target_state_bias_mask"), &QuantumDarwinismCouplingRule::get_target_state_bias_mask);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "target_state_bias_mask"), "set_target_state_bias_mask", "get_target_state_bias_mask");

    ClassDB::bind_method(D_METHOD("set_target_forbid_state_mask", "mask"), &QuantumDarwinismCouplingRule::set_target_forbid_state_mask);
    ClassDB::bind_method(D_METHOD("get_target_forbid_state_mask"), &QuantumDarwinismCouplingRule::get_target_forbid_state_mask);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "target_forbid_state_mask"), "set_target_forbid_state_mask", "get_target_forbid_state_mask");

    ClassDB::bind_method(D_METHOD("set_scope", "scope"), &QuantumDarwinismCouplingRule::set_scope);
    ClassDB::bind_method(D_METHOD("get_scope"), &QuantumDarwinismCouplingRule::get_scope);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "scope", PROPERTY_HINT_ENUM, "Self:0,Children:1,Descendants:2,Siblings:3,All:4"), "set_scope", "get_scope");

    ClassDB::bind_method(D_METHOD("set_topology_mask", "mask"), &QuantumDarwinismCouplingRule::set_topology_mask);
    ClassDB::bind_method(D_METHOD("get_topology_mask"), &QuantumDarwinismCouplingRule::get_topology_mask);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "topology_mask", PROPERTY_HINT_FLAGS, "HierarchyDown,HierarchyUp,Siblings,GraphOrSpatial"), "set_topology_mask", "get_topology_mask");

    ClassDB::bind_method(D_METHOD("set_max_coupling_hops", "hops"), &QuantumDarwinismCouplingRule::set_max_coupling_hops);
    ClassDB::bind_method(D_METHOD("get_max_coupling_hops"), &QuantumDarwinismCouplingRule::get_max_coupling_hops);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_coupling_hops", PROPERTY_HINT_RANGE, "0,8,1"), "set_max_coupling_hops", "get_max_coupling_hops");

    ClassDB::bind_method(D_METHOD("set_priority", "priority"), &QuantumDarwinismCouplingRule::set_priority);
    ClassDB::bind_method(D_METHOD("get_priority"), &QuantumDarwinismCouplingRule::get_priority);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "priority", PROPERTY_HINT_RANGE, "0,255,1"), "set_priority", "get_priority");

    ClassDB::bind_method(D_METHOD("set_help_topic_id", "topic_id"), &QuantumDarwinismCouplingRule::set_help_topic_id);
    ClassDB::bind_method(D_METHOD("get_help_topic_id"), &QuantumDarwinismCouplingRule::get_help_topic_id);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "help_topic_id"), "set_help_topic_id", "get_help_topic_id");

    ClassDB::bind_method(D_METHOD("commit_delta_threshold_tau", "delta", "action"), &QuantumDarwinismCouplingRule::commit_delta_threshold_tau, DEFVAL(String("Set QD Coupling Delta Tau")));
    ClassDB::bind_method(D_METHOD("commit_delta_permeability_gamma", "delta", "action"), &QuantumDarwinismCouplingRule::commit_delta_permeability_gamma, DEFVAL(String("Set QD Coupling Delta Gamma")));
}

void QuantumDarwinismCouplingRule::_set_delta_threshold_tau_direct(float p_delta) {
    const float clamped = std::clamp(p_delta, -1.0f, 1.0f);
    if (delta_threshold_tau_ == clamped) return;
    delta_threshold_tau_ = clamped;
    emit_changed();
}

void QuantumDarwinismCouplingRule::_set_delta_permeability_gamma_direct(float p_delta) {
    const float clamped = std::clamp(p_delta, -1.0f, 1.0f);
    if (delta_permeability_gamma_ == clamped) return;
    delta_permeability_gamma_ = clamped;
    emit_changed();
}

void QuantumDarwinismCouplingRule::set_delta_threshold_tau(float p_delta) { _set_delta_threshold_tau_direct(p_delta); }
void QuantumDarwinismCouplingRule::set_delta_permeability_gamma(float p_delta) { _set_delta_permeability_gamma_direct(p_delta); }
void QuantumDarwinismCouplingRule::set_value_bias_weight(float p_weight) { value_bias_weight_ = std::clamp(p_weight, 0.0f, 1.0f); emit_changed(); }
void QuantumDarwinismCouplingRule::set_scope(int p_scope) { scope_ = std::clamp(p_scope, 0, 4); emit_changed(); }
void QuantumDarwinismCouplingRule::set_topology_mask(int p_mask) { topology_mask_ = p_mask & 0x0F; emit_changed(); }
void QuantumDarwinismCouplingRule::set_max_coupling_hops(int p_hops) { max_coupling_hops_ = std::clamp(p_hops, 0, 8); emit_changed(); }
void QuantumDarwinismCouplingRule::set_priority(int p_prio) { priority_ = std::clamp(p_prio, 0, 255); emit_changed(); }

void QuantumDarwinismCouplingRule::commit_delta_threshold_tau(float p_delta, const godot::String &p_action) {
    const float clamped = std::clamp(p_delta, -1.0f, 1.0f);
    if (delta_threshold_tau_ == clamped) return;
    if (_can_record_undo()) {
        const float old_v = delta_threshold_tau_;
        undo_redo_->create_action(p_action.is_empty() ? godot::String("Set QD Coupling Delta Tau") : p_action);
        undo_redo_->add_do_method(callable_mp(this, &QuantumDarwinismCouplingRule::_set_delta_threshold_tau_direct).bind(clamped));
        undo_redo_->add_undo_method(callable_mp(this, &QuantumDarwinismCouplingRule::_set_delta_threshold_tau_direct).bind(old_v));
        undo_redo_->commit_action();
    } else {
        _set_delta_threshold_tau_direct(clamped);
    }
}

void QuantumDarwinismCouplingRule::commit_delta_permeability_gamma(float p_delta, const godot::String &p_action) {
    const float clamped = std::clamp(p_delta, -1.0f, 1.0f);
    if (delta_permeability_gamma_ == clamped) return;
    if (_can_record_undo()) {
        const float old_v = delta_permeability_gamma_;
        undo_redo_->create_action(p_action.is_empty() ? godot::String("Set QD Coupling Delta Gamma") : p_action);
        undo_redo_->add_do_method(callable_mp(this, &QuantumDarwinismCouplingRule::_set_delta_permeability_gamma_direct).bind(clamped));
        undo_redo_->add_undo_method(callable_mp(this, &QuantumDarwinismCouplingRule::_set_delta_permeability_gamma_direct).bind(old_v));
        undo_redo_->commit_action();
    } else {
        _set_delta_permeability_gamma_direct(clamped);
    }
}

core::ChannelCouplingRulePOD QuantumDarwinismCouplingRule::to_pod(
    uint32_t p_rule_id,
    uint32_t p_source_channel_id,
    uint32_t p_target_channel_id
) const noexcept {
    core::ChannelCouplingRulePOD pod{};
    pod.rule_id                    = p_rule_id;
    pod.source_channel_id          = p_source_channel_id;
    pod.target_channel_id          = p_target_channel_id;
    pod.required_source_state_mask = static_cast<uint32_t>(required_source_state_mask_);
    pod.source_value_min           = source_value_min_;
    pod.source_value_max           = source_value_max_;
    pod.delta_threshold_tau        = delta_threshold_tau_;
    pod.delta_permeability_gamma   = delta_permeability_gamma_;
    pod.target_value_bias          = target_value_bias_;
    pod.value_bias_weight          = value_bias_weight_;
    pod.target_state_bias_mask     = static_cast<uint32_t>(target_state_bias_mask_);
    pod.target_forbid_state_mask   = static_cast<uint32_t>(target_forbid_state_mask_);
    pod.scope                      = static_cast<core::PatternRuleScope>(std::clamp(scope_, 0, 4));
    pod.topology_mask              = static_cast<uint8_t>(topology_mask_ & 0x0F);
    pod.max_coupling_hops          = static_cast<uint8_t>(std::clamp(max_coupling_hops_, 0, 8));
    pod.priority                   = static_cast<uint8_t>(std::clamp(priority_, 0, 255));
    return pod;
}

} // namespace ideam::godot_ext
