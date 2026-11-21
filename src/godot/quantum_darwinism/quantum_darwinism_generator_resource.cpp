#include "quantum_darwinism_generator_resource.h"
#include "ideam_quantum_darwinism_plugin.h"
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <algorithm>
#include <cmath>
#include <queue>
#include <vector>

namespace ideam::godot_ext {

// ============================================================================
// QuantumDarwinismGeneratorResource
// ============================================================================

void QuantumDarwinismGeneratorResource::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_solver_mode", "mode"), &QuantumDarwinismGeneratorResource::set_solver_mode);
    ClassDB::bind_method(D_METHOD("get_solver_mode"), &QuantumDarwinismGeneratorResource::get_solver_mode);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "solver_mode", PROPERTY_HINT_ENUM, "ConflictStabilizer:0,OutwardGenerative:1,HybridGenerateAndStabilize:2"),
        "set_solver_mode",
        "get_solver_mode"
    );

    ClassDB::bind_method(D_METHOD("set_max_wavefront_iterations", "iterations"), &QuantumDarwinismGeneratorResource::set_max_wavefront_iterations);
    ClassDB::bind_method(D_METHOD("get_max_wavefront_iterations"), &QuantumDarwinismGeneratorResource::get_max_wavefront_iterations);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_wavefront_iterations", PROPERTY_HINT_RANGE, "1,512,1"), "set_max_wavefront_iterations", "get_max_wavefront_iterations");

    ClassDB::bind_method(D_METHOD("set_perturbation_strength", "strength"), &QuantumDarwinismGeneratorResource::set_perturbation_strength);
    ClassDB::bind_method(D_METHOD("get_perturbation_strength"), &QuantumDarwinismGeneratorResource::get_perturbation_strength);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "perturbation_strength", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_perturbation_strength", "get_perturbation_strength");

    ClassDB::bind_method(D_METHOD("set_nucleation_temperature", "temperature"), &QuantumDarwinismGeneratorResource::set_nucleation_temperature);
    ClassDB::bind_method(D_METHOD("get_nucleation_temperature"), &QuantumDarwinismGeneratorResource::get_nucleation_temperature);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "nucleation_temperature", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_nucleation_temperature", "get_nucleation_temperature");

    ClassDB::bind_method(D_METHOD("set_min_frontier_redundancy", "min_redundancy"), &QuantumDarwinismGeneratorResource::set_min_frontier_redundancy);
    ClassDB::bind_method(D_METHOD("get_min_frontier_redundancy"), &QuantumDarwinismGeneratorResource::get_min_frontier_redundancy);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "min_frontier_redundancy", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_min_frontier_redundancy", "get_min_frontier_redundancy");

    ClassDB::bind_method(D_METHOD("set_pointer_redundancy_threshold", "threshold"), &QuantumDarwinismGeneratorResource::set_pointer_redundancy_threshold);
    ClassDB::bind_method(D_METHOD("get_pointer_redundancy_threshold"), &QuantumDarwinismGeneratorResource::get_pointer_redundancy_threshold);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "pointer_redundancy_threshold", PROPERTY_HINT_RANGE, "0.05,0.99,0.01"), "set_pointer_redundancy_threshold", "get_pointer_redundancy_threshold");

    ClassDB::bind_method(D_METHOD("set_enable_spontaneous_nucleation", "enable"), &QuantumDarwinismGeneratorResource::set_enable_spontaneous_nucleation);
    ClassDB::bind_method(D_METHOD("get_enable_spontaneous_nucleation"), &QuantumDarwinismGeneratorResource::get_enable_spontaneous_nucleation);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enable_spontaneous_nucleation"), "set_enable_spontaneous_nucleation", "get_enable_spontaneous_nucleation");

    ClassDB::bind_method(D_METHOD("set_enable_cross_channel_coupling", "enable"), &QuantumDarwinismGeneratorResource::set_enable_cross_channel_coupling);
    ClassDB::bind_method(D_METHOD("get_enable_cross_channel_coupling"), &QuantumDarwinismGeneratorResource::get_enable_cross_channel_coupling);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enable_cross_channel_coupling"), "set_enable_cross_channel_coupling", "get_enable_cross_channel_coupling");

    ClassDB::bind_method(D_METHOD("set_auto_stabilize_after_projection", "auto_stabilize"), &QuantumDarwinismGeneratorResource::set_auto_stabilize_after_projection);
    ClassDB::bind_method(D_METHOD("get_auto_stabilize_after_projection"), &QuantumDarwinismGeneratorResource::get_auto_stabilize_after_projection);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_stabilize_after_projection"), "set_auto_stabilize_after_projection", "get_auto_stabilize_after_projection");

    ClassDB::bind_method(D_METHOD("set_pattern", "pattern"), &QuantumDarwinismGeneratorResource::set_pattern);
    ClassDB::bind_method(D_METHOD("get_pattern"), &QuantumDarwinismGeneratorResource::get_pattern);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "pattern", PROPERTY_HINT_RESOURCE_TYPE, "ProceduralPattern"), "set_pattern", "get_pattern");

    ClassDB::bind_method(D_METHOD("set_channels", "channels"), &QuantumDarwinismGeneratorResource::set_channels);
    ClassDB::bind_method(D_METHOD("get_channels"), &QuantumDarwinismGeneratorResource::get_channels);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "channels", PROPERTY_HINT_ARRAY_TYPE, "QuantumDarwinismChannelSpec"), "set_channels", "get_channels");

    ClassDB::bind_method(D_METHOD("set_coupling_rules", "rules"), &QuantumDarwinismGeneratorResource::set_coupling_rules);
    ClassDB::bind_method(D_METHOD("get_coupling_rules"), &QuantumDarwinismGeneratorResource::get_coupling_rules);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "coupling_rules", PROPERTY_HINT_ARRAY_TYPE, "QuantumDarwinismCouplingRule"), "set_coupling_rules", "get_coupling_rules");

    ClassDB::bind_method(D_METHOD("set_help_topic_id", "topic_id"), &QuantumDarwinismGeneratorResource::set_help_topic_id);
    ClassDB::bind_method(D_METHOD("get_help_topic_id"), &QuantumDarwinismGeneratorResource::get_help_topic_id);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "help_topic_id"), "set_help_topic_id", "get_help_topic_id");

    ClassDB::bind_method(D_METHOD("find_channel_index", "channel_name"), &QuantumDarwinismGeneratorResource::find_channel_index);
    ClassDB::bind_method(D_METHOD("add_channel", "channel"), &QuantumDarwinismGeneratorResource::add_channel);
    ClassDB::bind_method(D_METHOD("remove_channel", "index"), &QuantumDarwinismGeneratorResource::remove_channel);
    ClassDB::bind_method(D_METHOD("add_coupling_rule", "rule"), &QuantumDarwinismGeneratorResource::add_coupling_rule);
    ClassDB::bind_method(D_METHOD("remove_coupling_rule", "index"), &QuantumDarwinismGeneratorResource::remove_coupling_rule);

    ClassDB::bind_method(D_METHOD("propagate_undo_redo", "undo_redo"), &QuantumDarwinismGeneratorResource::propagate_undo_redo);
    ClassDB::bind_method(D_METHOD("commit_solver_mode", "new_mode", "action"), &QuantumDarwinismGeneratorResource::commit_solver_mode, DEFVAL(String("Set QD Generator Mode")));
    ClassDB::bind_method(D_METHOD("commit_perturbation_strength", "strength", "action"), &QuantumDarwinismGeneratorResource::commit_perturbation_strength, DEFVAL(String("Set QD Perturbation Strength")));
    ClassDB::bind_method(D_METHOD("commit_nucleation_temperature", "temperature", "action"), &QuantumDarwinismGeneratorResource::commit_nucleation_temperature, DEFVAL(String("Set QD Nucleation Temperature")));
    ClassDB::bind_method(D_METHOD("commit_add_channel", "channel", "action"), &QuantumDarwinismGeneratorResource::commit_add_channel, DEFVAL(String("Add QD Channel")));
    ClassDB::bind_method(D_METHOD("commit_remove_channel", "index", "action"), &QuantumDarwinismGeneratorResource::commit_remove_channel, DEFVAL(String("Remove QD Channel")));
    ClassDB::bind_method(D_METHOD("commit_add_coupling_rule", "rule", "action"), &QuantumDarwinismGeneratorResource::commit_add_coupling_rule, DEFVAL(String("Add QD Coupling Rule")));
    ClassDB::bind_method(D_METHOD("commit_remove_coupling_rule", "index", "action"), &QuantumDarwinismGeneratorResource::commit_remove_coupling_rule, DEFVAL(String("Remove QD Coupling Rule")));
}

void QuantumDarwinismGeneratorResource::_set_solver_mode_direct(int p_mode) {
    const int c = std::clamp(p_mode, 0, 2);
    if (solver_mode_ == c) return;
    solver_mode_ = c;
    emit_changed();
}

void QuantumDarwinismGeneratorResource::_set_perturbation_strength_direct(float p_strength) {
    const float c = std::clamp(p_strength, 0.0f, 1.0f);
    if (perturbation_strength_ == c) return;
    perturbation_strength_ = c;
    emit_changed();
}

void QuantumDarwinismGeneratorResource::_set_nucleation_temperature_direct(float p_temp) {
    const float c = std::clamp(p_temp, 0.0f, 1.0f);
    if (nucleation_temperature_ == c) return;
    nucleation_temperature_ = c;
    emit_changed();
}

void QuantumDarwinismGeneratorResource::_set_channels_direct(const godot::TypedArray<QuantumDarwinismChannelSpec> &p_channels) {
    channels_ = p_channels;
    if (get_undo_redo().is_valid()) {
        for (int i = 0; i < channels_.size(); ++i) {
            godot::Ref<QuantumDarwinismChannelSpec> ch = channels_[i];
            if (ch.is_valid()) ch->set_undo_redo(get_undo_redo());
        }
    }
    emit_changed();
}

void QuantumDarwinismGeneratorResource::_set_coupling_rules_direct(const godot::TypedArray<QuantumDarwinismCouplingRule> &p_rules) {
    coupling_rules_ = p_rules;
    if (get_undo_redo().is_valid()) {
        for (int i = 0; i < coupling_rules_.size(); ++i) {
            godot::Ref<QuantumDarwinismCouplingRule> r = coupling_rules_[i];
            if (r.is_valid()) r->set_undo_redo(get_undo_redo());
        }
    }
    emit_changed();
}

void QuantumDarwinismGeneratorResource::set_solver_mode(int p_mode) { _set_solver_mode_direct(p_mode); }
void QuantumDarwinismGeneratorResource::set_max_wavefront_iterations(int p_iters) { max_wavefront_iterations_ = std::clamp(p_iters, 1, 512); emit_changed(); }
void QuantumDarwinismGeneratorResource::set_perturbation_strength(float p_strength) { _set_perturbation_strength_direct(p_strength); }
void QuantumDarwinismGeneratorResource::set_nucleation_temperature(float p_temp) { _set_nucleation_temperature_direct(p_temp); }
void QuantumDarwinismGeneratorResource::set_min_frontier_redundancy(float p_min_r) { min_frontier_redundancy_ = std::clamp(p_min_r, 0.0f, 1.0f); emit_changed(); }
void QuantumDarwinismGeneratorResource::set_pointer_redundancy_threshold(float p_thresh) { pointer_redundancy_threshold_ = std::clamp(p_thresh, 0.05f, 0.99f); emit_changed(); }
void QuantumDarwinismGeneratorResource::set_enable_spontaneous_nucleation(bool p_enable) { enable_spontaneous_nucleation_ = p_enable; emit_changed(); }
void QuantumDarwinismGeneratorResource::set_enable_cross_channel_coupling(bool p_enable) { enable_cross_channel_coupling_ = p_enable; emit_changed(); }
void QuantumDarwinismGeneratorResource::set_auto_stabilize_after_projection(bool p_auto) { auto_stabilize_after_projection_ = p_auto; emit_changed(); }
void QuantumDarwinismGeneratorResource::set_pattern(const godot::Ref<ProceduralPattern> &p_pattern) {
    pattern_ = p_pattern;
    if (pattern_.is_valid() && get_undo_redo().is_valid()) pattern_->set_undo_redo(get_undo_redo());
    emit_changed();
}
void QuantumDarwinismGeneratorResource::set_channels(const godot::TypedArray<QuantumDarwinismChannelSpec> &p_channels) { _set_channels_direct(p_channels); }
void QuantumDarwinismGeneratorResource::set_coupling_rules(const godot::TypedArray<QuantumDarwinismCouplingRule> &p_rules) { _set_coupling_rules_direct(p_rules); }

int QuantumDarwinismGeneratorResource::find_channel_index(const godot::StringName &p_channel_name) const {
    for (int i = 0; i < channels_.size(); ++i) {
        godot::Ref<QuantumDarwinismChannelSpec> ch = channels_[i];
        if (ch.is_valid() && ch->get_channel_name() == p_channel_name) {
            return i;
        }
    }
    return -1;
}

void QuantumDarwinismGeneratorResource::add_channel(const godot::Ref<QuantumDarwinismChannelSpec> &p_channel) {
    if (p_channel.is_null()) return;
    if (get_undo_redo().is_valid()) p_channel->set_undo_redo(get_undo_redo());
    channels_.append(p_channel);
    emit_changed();
}

void QuantumDarwinismGeneratorResource::remove_channel(int p_index) {
    if (p_index < 0 || p_index >= channels_.size()) return;
    channels_.remove_at(p_index);
    emit_changed();
}

void QuantumDarwinismGeneratorResource::add_coupling_rule(const godot::Ref<QuantumDarwinismCouplingRule> &p_rule) {
    if (p_rule.is_null()) return;
    if (get_undo_redo().is_valid()) p_rule->set_undo_redo(get_undo_redo());
    coupling_rules_.append(p_rule);
    emit_changed();
}

void QuantumDarwinismGeneratorResource::remove_coupling_rule(int p_index) {
    if (p_index < 0 || p_index >= coupling_rules_.size()) return;
    coupling_rules_.remove_at(p_index);
    emit_changed();
}

void QuantumDarwinismGeneratorResource::propagate_undo_redo(const godot::Ref<IdeamUndoRedo> &p_undo_redo) {
    set_undo_redo(p_undo_redo);
    if (get_semantic_tags().is_valid()) get_semantic_tags()->set_undo_redo(p_undo_redo);
    if (pattern_.is_valid()) pattern_->set_undo_redo(p_undo_redo);
    for (int i = 0; i < channels_.size(); ++i) {
        godot::Ref<QuantumDarwinismChannelSpec> ch = channels_[i];
        if (ch.is_valid()) ch->set_undo_redo(p_undo_redo);
    }
    for (int i = 0; i < coupling_rules_.size(); ++i) {
        godot::Ref<QuantumDarwinismCouplingRule> r = coupling_rules_[i];
        if (r.is_valid()) r->set_undo_redo(p_undo_redo);
    }
}

void QuantumDarwinismGeneratorResource::commit_solver_mode(int p_new_mode, const godot::String &p_action) {
    const int c = std::clamp(p_new_mode, 0, 2);
    if (solver_mode_ == c) return;
    godot::Ref<IdeamUndoRedo> ur = _can_record_undo() ? get_undo_redo() : IdeamQuantumDarwinismPlugin::create_undo_redo_wrapper();
    if (ur.is_valid()) {
        const int old_v = solver_mode_;
        ur->create_action(p_action.is_empty() ? godot::String("Set QD Generator Mode") : p_action);
        ur->add_do_method(callable_mp(this, &QuantumDarwinismGeneratorResource::_set_solver_mode_direct).bind(c));
        ur->add_undo_method(callable_mp(this, &QuantumDarwinismGeneratorResource::_set_solver_mode_direct).bind(old_v));
        ur->commit_action();
    } else {
        _set_solver_mode_direct(c);
    }
}

void QuantumDarwinismGeneratorResource::commit_perturbation_strength(float p_strength, const godot::String &p_action) {
    const float c = std::clamp(p_strength, 0.0f, 1.0f);
    if (perturbation_strength_ == c) return;
    godot::Ref<IdeamUndoRedo> ur = _can_record_undo() ? get_undo_redo() : IdeamQuantumDarwinismPlugin::create_undo_redo_wrapper();
    if (ur.is_valid()) {
        const float old_v = perturbation_strength_;
        ur->create_action(p_action.is_empty() ? godot::String("Set QD Perturbation Strength") : p_action);
        ur->add_do_method(callable_mp(this, &QuantumDarwinismGeneratorResource::_set_perturbation_strength_direct).bind(c));
        ur->add_undo_method(callable_mp(this, &QuantumDarwinismGeneratorResource::_set_perturbation_strength_direct).bind(old_v));
        ur->commit_action();
    } else {
        _set_perturbation_strength_direct(c);
    }
}

void QuantumDarwinismGeneratorResource::commit_nucleation_temperature(float p_temp, const godot::String &p_action) {
    const float c = std::clamp(p_temp, 0.0f, 1.0f);
    if (nucleation_temperature_ == c) return;
    godot::Ref<IdeamUndoRedo> ur = _can_record_undo() ? get_undo_redo() : IdeamQuantumDarwinismPlugin::create_undo_redo_wrapper();
    if (ur.is_valid()) {
        const float old_v = nucleation_temperature_;
        ur->create_action(p_action.is_empty() ? godot::String("Set QD Nucleation Temperature") : p_action);
        ur->add_do_method(callable_mp(this, &QuantumDarwinismGeneratorResource::_set_nucleation_temperature_direct).bind(c));
        ur->add_undo_method(callable_mp(this, &QuantumDarwinismGeneratorResource::_set_nucleation_temperature_direct).bind(old_v));
        ur->commit_action();
    } else {
        _set_nucleation_temperature_direct(c);
    }
}

void QuantumDarwinismGeneratorResource::commit_add_channel(const godot::Ref<QuantumDarwinismChannelSpec> &p_channel, const godot::String &p_action) {
    if (p_channel.is_null()) return;
    godot::TypedArray<QuantumDarwinismChannelSpec> before = channels_.duplicate();
    godot::TypedArray<QuantumDarwinismChannelSpec> after  = channels_.duplicate();
    after.append(p_channel);
    godot::Ref<IdeamUndoRedo> ur = _can_record_undo() ? get_undo_redo() : IdeamQuantumDarwinismPlugin::create_undo_redo_wrapper();
    if (ur.is_valid()) {
        ur->create_action(p_action.is_empty() ? godot::String("Add QD Channel") : p_action);
        ur->add_do_method(callable_mp(this, &QuantumDarwinismGeneratorResource::_set_channels_direct).bind(after));
        ur->add_undo_method(callable_mp(this, &QuantumDarwinismGeneratorResource::_set_channels_direct).bind(before));
        ur->commit_action();
    } else {
        _set_channels_direct(after);
    }
}

void QuantumDarwinismGeneratorResource::commit_remove_channel(int p_index, const godot::String &p_action) {
    if (p_index < 0 || p_index >= channels_.size()) return;
    godot::TypedArray<QuantumDarwinismChannelSpec> before = channels_.duplicate();
    godot::TypedArray<QuantumDarwinismChannelSpec> after  = channels_.duplicate();
    after.remove_at(p_index);
    godot::Ref<IdeamUndoRedo> ur = _can_record_undo() ? get_undo_redo() : IdeamQuantumDarwinismPlugin::create_undo_redo_wrapper();
    if (ur.is_valid()) {
        ur->create_action(p_action.is_empty() ? godot::String("Remove QD Channel") : p_action);
        ur->add_do_method(callable_mp(this, &QuantumDarwinismGeneratorResource::_set_channels_direct).bind(after));
        ur->add_undo_method(callable_mp(this, &QuantumDarwinismGeneratorResource::_set_channels_direct).bind(before));
        ur->commit_action();
    } else {
        _set_channels_direct(after);
    }
}

void QuantumDarwinismGeneratorResource::commit_add_coupling_rule(const godot::Ref<QuantumDarwinismCouplingRule> &p_rule, const godot::String &p_action) {
    if (p_rule.is_null()) return;
    godot::TypedArray<QuantumDarwinismCouplingRule> before = coupling_rules_.duplicate();
    godot::TypedArray<QuantumDarwinismCouplingRule> after  = coupling_rules_.duplicate();
    after.append(p_rule);
    godot::Ref<IdeamUndoRedo> ur = _can_record_undo() ? get_undo_redo() : IdeamQuantumDarwinismPlugin::create_undo_redo_wrapper();
    if (ur.is_valid()) {
        ur->create_action(p_action.is_empty() ? godot::String("Add QD Coupling Rule") : p_action);
        ur->add_do_method(callable_mp(this, &QuantumDarwinismGeneratorResource::_set_coupling_rules_direct).bind(after));
        ur->add_undo_method(callable_mp(this, &QuantumDarwinismGeneratorResource::_set_coupling_rules_direct).bind(before));
        ur->commit_action();
    } else {
        _set_coupling_rules_direct(after);
    }
}

void QuantumDarwinismGeneratorResource::commit_remove_coupling_rule(int p_index, const godot::String &p_action) {
    if (p_index < 0 || p_index >= coupling_rules_.size()) return;
    godot::TypedArray<QuantumDarwinismCouplingRule> before = coupling_rules_.duplicate();
    godot::TypedArray<QuantumDarwinismCouplingRule> after  = coupling_rules_.duplicate();
    after.remove_at(p_index);
    godot::Ref<IdeamUndoRedo> ur = _can_record_undo() ? get_undo_redo() : IdeamQuantumDarwinismPlugin::create_undo_redo_wrapper();
    if (ur.is_valid()) {
        ur->create_action(p_action.is_empty() ? godot::String("Remove QD Coupling Rule") : p_action);
        ur->add_do_method(callable_mp(this, &QuantumDarwinismGeneratorResource::_set_coupling_rules_direct).bind(after));
        ur->add_undo_method(callable_mp(this, &QuantumDarwinismGeneratorResource::_set_coupling_rules_direct).bind(before));
        ur->commit_action();
    } else {
        _set_coupling_rules_direct(after);
    }
}

core::GenerativeWavefrontConfigPOD QuantumDarwinismGeneratorResource::to_generative_pod() const noexcept {
    core::GenerativeWavefrontConfigPOD cfg{};
    cfg.master_seed                     = static_cast<uint64_t>(get_master_seed());
    cfg.max_wavefront_iterations        = static_cast<uint32_t>(std::clamp(max_wavefront_iterations_, 1, 512));
    cfg.perturbation_strength           = perturbation_strength_;
    cfg.nucleation_temperature          = nucleation_temperature_;
    cfg.min_frontier_redundancy         = min_frontier_redundancy_;
    cfg.mode                            = static_cast<core::QuantumDarwinismMode>(std::clamp(solver_mode_, 0, 2));
    cfg.enable_spontaneous_nucleation   = enable_spontaneous_nucleation_ ? 1u : 0u;
    cfg.enable_cross_channel_coupling   = enable_cross_channel_coupling_ ? 1u : 0u;
    cfg.auto_stabilize_after_projection = auto_stabilize_after_projection_ ? 1u : 0u;
    return cfg;
}

core::HorizonBoxConfigPOD QuantumDarwinismGeneratorResource::to_horizon_pod() const noexcept {
    core::HorizonBoxConfigPOD cfg{};
    cfg.pointer_redundancy_thresh = pointer_redundancy_threshold_;
    cfg.max_horizon_slots         = 4096u;
    return cfg;
}

void QuantumDarwinismGeneratorResource::apply_to_solver(core::QuantumDarwinismSolver &r_solver) const {
    r_solver.set_generative_config(to_generative_pod());
    r_solver.set_horizon_config(to_horizon_pod());
    r_solver.clear_channels();
    r_solver.clear_coupling_rules();

    for (int i = 0; i < channels_.size(); ++i) {
        godot::Ref<QuantumDarwinismChannelSpec> ch = channels_[i];
        if (ch.is_valid()) {
            r_solver.add_channel_spec(ch->to_pod(static_cast<uint32_t>(i)));
        }
    }

    uint32_t r_id = 0u;
    for (int i = 0; i < coupling_rules_.size(); ++i) {
        godot::Ref<QuantumDarwinismCouplingRule> rule = coupling_rules_[i];
        if (!rule.is_valid()) continue;
        const int src_idx = find_channel_index(rule->get_source_channel_name());
        const int dst_idx = find_channel_index(rule->get_target_channel_name());
        if (src_idx >= 0 && dst_idx >= 0) {
            r_solver.add_coupling_rule(rule->to_pod(r_id++, static_cast<uint32_t>(src_idx), static_cast<uint32_t>(dst_idx)));
        }
    }
}

// ============================================================================
// QuantumDarwinismGeneratorNode
// ============================================================================

QuantumDarwinismGeneratorNode::QuantumDarwinismGeneratorNode() {
    environment_.configure_slots(static_cast<uint32_t>(slot_count_), 0u);
    solver_.bind_environment(&environment_);
}

void QuantumDarwinismGeneratorNode::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_slot_count", "count"), &QuantumDarwinismGeneratorNode::set_slot_count);
    ClassDB::bind_method(D_METHOD("get_slot_count"), &QuantumDarwinismGeneratorNode::get_slot_count);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "slot_count", PROPERTY_HINT_RANGE, "1,4096,1"), "set_slot_count", "get_slot_count");

    ClassDB::bind_method(D_METHOD("set_procedural_pattern", "pattern"), &QuantumDarwinismGeneratorNode::set_procedural_pattern);
    ClassDB::bind_method(D_METHOD("get_procedural_pattern"), &QuantumDarwinismGeneratorNode::get_procedural_pattern);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "procedural_pattern", PROPERTY_HINT_RESOURCE_TYPE, "ProceduralPattern"), "set_procedural_pattern", "get_procedural_pattern");

    ClassDB::bind_method(D_METHOD("set_target_wfc_solver_path", "path"), &QuantumDarwinismGeneratorNode::set_target_wfc_solver_path);
    ClassDB::bind_method(D_METHOD("get_target_wfc_solver_path"), &QuantumDarwinismGeneratorNode::get_target_wfc_solver_path);
    ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "target_wfc_solver_path"), "set_target_wfc_solver_path", "get_target_wfc_solver_path");

    ClassDB::bind_method(D_METHOD("configure_environment_shape", "slot_count", "slot_hierarchy_dicts"), &QuantumDarwinismGeneratorNode::configure_environment_shape);
    ClassDB::bind_method(D_METHOD("set_slot_children", "parent_slot_id", "child_slot_ids"), &QuantumDarwinismGeneratorNode::set_slot_children);
    ClassDB::bind_method(D_METHOD("add_directed_edge", "from_slot", "to_slot", "port_id", "distance_weight", "bidirectional"), &QuantumDarwinismGeneratorNode::add_directed_edge, DEFVAL(1.0f), DEFVAL(false));
    ClassDB::bind_method(D_METHOD("pin_landmark_state", "slot_id", "state_id"), &QuantumDarwinismGeneratorNode::pin_landmark_state);
    ClassDB::bind_method(D_METHOD("pin_landmark_by_name", "slot_id", "token_name"), &QuantumDarwinismGeneratorNode::pin_landmark_by_name);
    ClassDB::bind_method(D_METHOD("pin_slot_channel_value", "slot_id", "channel_name", "value", "state_mask"), &QuantumDarwinismGeneratorNode::pin_slot_channel_value, DEFVAL(0));
    ClassDB::bind_method(D_METHOD("set_perturbation_field", "field"), &QuantumDarwinismGeneratorNode::set_perturbation_field);
    ClassDB::bind_method(D_METHOD("clear_pinned_landmarks"), &QuantumDarwinismGeneratorNode::clear_pinned_landmarks);

    ClassDB::bind_method(D_METHOD("run_generative_einselection"), &QuantumDarwinismGeneratorNode::run_generative_einselection);
    ClassDB::bind_method(D_METHOD("step_generative_wave"), &QuantumDarwinismGeneratorNode::step_generative_wave);
    ClassDB::bind_method(D_METHOD("project_channels_to_tokens"), &QuantumDarwinismGeneratorNode::project_channels_to_tokens);
    ClassDB::bind_method(D_METHOD("get_einselection_status"), &QuantumDarwinismGeneratorNode::get_einselection_status);
    ClassDB::bind_method(D_METHOD("get_current_wave_index"), &QuantumDarwinismGeneratorNode::get_current_wave_index);
    ClassDB::bind_method(D_METHOD("get_collapsed_token_name", "slot_id"), &QuantumDarwinismGeneratorNode::get_collapsed_token_name);
    ClassDB::bind_method(D_METHOD("get_slot_channel_states_as_dicts", "slot_id"), &QuantumDarwinismGeneratorNode::get_slot_channel_states_as_dicts);
    ClassDB::bind_method(D_METHOD("get_nucleation_events_as_dicts"), &QuantumDarwinismGeneratorNode::get_nucleation_events_as_dicts);
    ClassDB::bind_method(D_METHOD("export_solved_configuration_summary"), &QuantumDarwinismGeneratorNode::export_solved_configuration_summary);
    ClassDB::bind_method(D_METHOD("apply_resolved_channels_to_object_tree", "root_node"), &QuantumDarwinismGeneratorNode::apply_resolved_channels_to_object_tree);
}

WFCSolverNode *QuantumDarwinismGeneratorNode::resolve_optional_wfc_node() const {
    if (!target_wfc_solver_path_.is_empty()) {
        if (godot::Node *n = get_node_or_null(target_wfc_solver_path_)) {
            return godot::Object::cast_to<WFCSolverNode>(n);
        }
    }
    return nullptr;
}

core::ProceduralEnvironment &QuantumDarwinismGeneratorNode::active_environment() {
    if (WFCSolverNode *wfc = resolve_optional_wfc_node()) return wfc->environment();
    return environment_;
}

const core::ProceduralEnvironment &QuantumDarwinismGeneratorNode::active_environment() const {
    if (WFCSolverNode *wfc = resolve_optional_wfc_node()) return wfc->environment();
    return environment_;
}

godot::Ref<QuantumDarwinismGeneratorResource> QuantumDarwinismGeneratorNode::resolve_generator_resource() const {
    if (get_generator_resource().is_valid()) {
        if (auto *gen = godot::Object::cast_to<QuantumDarwinismGeneratorResource>(get_generator_resource().ptr())) {
            return godot::Ref<QuantumDarwinismGeneratorResource>(gen);
        }
    }
    return godot::Ref<QuantumDarwinismGeneratorResource>();
}

void QuantumDarwinismGeneratorNode::ensure_environment_and_solver_prepared() {
    auto &env = active_environment();
    godot::Ref<QuantumDarwinismGeneratorResource> gen_res = resolve_generator_resource();
    godot::Ref<ProceduralPattern> pat = procedural_pattern_.is_valid()
        ? procedural_pattern_
        : (gen_res.is_valid() ? gen_res->get_pattern() : godot::Ref<ProceduralPattern>());

    if (pat.is_valid() && env.tokens().empty()) {
        pat->compile_to_environment(env, static_cast<uint32_t>(slot_count_));
    } else if (env.slots().size() != static_cast<size_t>(slot_count_)) {
        env.configure_slots(static_cast<uint32_t>(slot_count_), static_cast<uint32_t>(env.tokens().size()));
    }

    if (gen_res.is_valid()) {
        gen_res->apply_to_solver(solver_);
    }
    solver_.bind_environment(&env);
}

void QuantumDarwinismGeneratorNode::set_slot_count(int p_count) {
    slot_count_ = std::clamp(p_count, 1, 4096);
    environment_.configure_slots(static_cast<uint32_t>(slot_count_), static_cast<uint32_t>(environment_.tokens().size()));
}

void QuantumDarwinismGeneratorNode::set_procedural_pattern(const godot::Ref<ProceduralPattern> &p_pattern) {
    procedural_pattern_ = p_pattern;
    if (procedural_pattern_.is_valid()) {
        procedural_pattern_->compile_to_environment(environment_, static_cast<uint32_t>(slot_count_));
    }
}

void QuantumDarwinismGeneratorNode::configure_environment_shape(int p_slot_count, const godot::Array &p_slot_hierarchy_dicts) {
    slot_count_ = std::clamp(p_slot_count, 1, 4096);
    auto &env = active_environment();
    env.configure_slots(static_cast<uint32_t>(slot_count_), static_cast<uint32_t>(env.tokens().size()));
    for (int i = 0; i < p_slot_hierarchy_dicts.size(); ++i) {
        if (p_slot_hierarchy_dicts[i].get_type() != godot::Variant::DICTIONARY) continue;
        godot::Dictionary d = p_slot_hierarchy_dicts[i];
        if (!d.has("parent_slot") || !d.has("children")) continue;
        const int p_id = static_cast<int>(d["parent_slot"]);
        godot::PackedInt32Array kids = d["children"];
        set_slot_children(p_id, kids);
    }
}

void QuantumDarwinismGeneratorNode::set_slot_children(int p_parent_slot_id, const godot::PackedInt32Array &p_child_slot_ids) {
    if (p_parent_slot_id < 0) return;
    std::vector<uint32_t> kids;
    kids.reserve(static_cast<size_t>(p_child_slot_ids.size()));
    for (int i = 0; i < p_child_slot_ids.size(); ++i) {
        if (p_child_slot_ids[i] >= 0) kids.push_back(static_cast<uint32_t>(p_child_slot_ids[i]));
    }
    active_environment().set_slot_children(static_cast<uint32_t>(p_parent_slot_id), kids);
}

void QuantumDarwinismGeneratorNode::add_directed_edge(int p_from_slot, int p_to_slot, int p_port_id, float p_distance_weight, bool p_bidirectional) {
    auto &env = active_environment();
    if (p_from_slot < 0 || p_to_slot < 0 ||
        static_cast<size_t>(p_from_slot) >= env.slots().size() ||
        static_cast<size_t>(p_to_slot) >= env.slots().size()) {
        return;
    }

    auto append_edge = [&](uint32_t u, uint32_t v) {
        std::vector<core::SpatialNeighborEdgePOD> edges;
        const auto &slot = env.slots()[u];
        const auto &csr  = env.spatial_neighbor_csr();
        if (slot.spatial_neighbor_count > 0u &&
            static_cast<size_t>(slot.spatial_csr_offset + slot.spatial_neighbor_count) <= csr.size()) {
            edges.assign(csr.begin() + slot.spatial_csr_offset, csr.begin() + slot.spatial_csr_offset + slot.spatial_neighbor_count);
        }
        core::SpatialNeighborEdgePOD e{};
        e.neighbor_slot_id     = v;
        e.port_or_direction_id = static_cast<uint32_t>(std::max(0, p_port_id));
        e.distance_weight      = std::max(0.1f, p_distance_weight);
        edges.push_back(e);
        env.set_slot_spatial_neighbors(u, edges);
    };

    append_edge(static_cast<uint32_t>(p_from_slot), static_cast<uint32_t>(p_to_slot));
    if (p_bidirectional) {
        append_edge(static_cast<uint32_t>(p_to_slot), static_cast<uint32_t>(p_from_slot));
    }
}

void QuantumDarwinismGeneratorNode::pin_landmark_state(int p_slot_id, int p_state_id) {
    auto &env = active_environment();
    if (p_slot_id < 0 || p_state_id < 0 || static_cast<size_t>(p_slot_id) >= env.slots().size()) return;
    auto &slot = env.slots()[static_cast<size_t>(p_slot_id)];
    slot.domain.clear();
    slot.domain.set_bit(static_cast<uint32_t>(p_state_id));
    slot.collapsed_token_id = p_state_id;
    slot.locked_layer       = core::PatternCascadeLayer::LAYER_PINNED_OVERRIDE;
    slot.einselection_score = 1.0f;
    slot.is_pointer_state   = 1u;
    slot.has_conflict       = 0u;
    solver_.add_pinned_landmark(static_cast<uint32_t>(p_slot_id), static_cast<uint32_t>(p_state_id));
}

void QuantumDarwinismGeneratorNode::pin_landmark_by_name(int p_slot_id, const godot::String &p_token_name) {
    ensure_environment_and_solver_prepared();
    const godot::CharString utf8 = p_token_name.utf8();
    const int32_t tok_id = active_environment().find_token_by_name(std::string_view(utf8.get_data(), static_cast<size_t>(utf8.length())));
    if (tok_id >= 0) {
        pin_landmark_state(p_slot_id, tok_id);
    }
}

bool QuantumDarwinismGeneratorNode::pin_slot_channel_value(int p_slot_id, const godot::StringName &p_channel_name, float p_value, int p_state_mask) {
    ensure_environment_and_solver_prepared();
    godot::Ref<QuantumDarwinismGeneratorResource> gen_res = resolve_generator_resource();
    if (!gen_res.is_valid() || p_slot_id < 0) return false;
    const int ch_idx = gen_res->find_channel_index(p_channel_name);
    if (ch_idx < 0) return false;

    auto &env = active_environment();
    if (solver_.slot_channel_states().size() != env.slots().size() * std::max<size_t>(1u, solver_.channel_specs().size())) {
        solver_.initialize_channel_states_from_environment(env);
    }
    return solver_.pin_slot_channel(static_cast<uint32_t>(p_slot_id), static_cast<uint32_t>(ch_idx), p_value, static_cast<uint32_t>(p_state_mask));
}

void QuantumDarwinismGeneratorNode::set_perturbation_field(const godot::PackedFloat32Array &p_field) {
    std::vector<float> tmp(static_cast<size_t>(p_field.size()));
    for (int i = 0; i < p_field.size(); ++i) tmp[static_cast<size_t>(i)] = p_field[i];
    solver_.set_external_perturbation_field(tmp);
}

void QuantumDarwinismGeneratorNode::clear_pinned_landmarks() {
    solver_.clear_pinned_landmarks();
    for (auto &slot : active_environment().slots()) {
        if (slot.locked_layer == core::PatternCascadeLayer::LAYER_PINNED_OVERRIDE) {
            slot.locked_layer     = core::PatternCascadeLayer::LAYER_DOMAIN;
            slot.is_pointer_state = 0u;
        }
    }
}

int QuantumDarwinismGeneratorNode::run_generative_einselection() {
    ensure_environment_and_solver_prepared();
    WFCSolverNode *wfc = resolve_optional_wfc_node();
    core::ProceduralEnvironment &env = active_environment();

    if (solver_.generative_config().mode == core::QuantumDarwinismMode::MODE_CONFLICT_STABILIZER) {
        return static_cast<int>(solver_.stabilize_all_conflicts_in_environment(env, wfc ? &wfc->solver_core() : nullptr));
    }

    const core::EinselectionStatus st = solver_.run_generative_einselection(env, wfc ? &wfc->solver_core() : nullptr);
    return static_cast<int>(st);
}

bool QuantumDarwinismGeneratorNode::step_generative_wave() {
    ensure_environment_and_solver_prepared();
    return solver_.step_outward_wavefront(active_environment());
}

bool QuantumDarwinismGeneratorNode::project_channels_to_tokens() {
    ensure_environment_and_solver_prepared();
    return solver_.project_channels_to_environment(active_environment());
}

godot::String QuantumDarwinismGeneratorNode::get_collapsed_token_name(int p_slot_id) const {
    const auto &env = active_environment();
    if (p_slot_id < 0 || static_cast<size_t>(p_slot_id) >= env.slots().size()) return godot::String();
    const int32_t tid = env.slots()[static_cast<size_t>(p_slot_id)].collapsed_token_id;
    if (tid < 0) return godot::String();
    const std::string_view sv = env.get_token_name(static_cast<uint32_t>(tid));
    return godot::String::utf8(sv.data(), static_cast<int>(sv.size()));
}

godot::Array QuantumDarwinismGeneratorNode::get_slot_channel_states_as_dicts(int p_slot_id) const {
    godot::Array out;
    const uint32_t n_ch = static_cast<uint32_t>(solver_.channel_specs().size());
    if (p_slot_id < 0 || n_ch == 0u) return out;
    const size_t base = static_cast<size_t>(p_slot_id) * n_ch;
    const auto &states = solver_.slot_channel_states();
    if (base + n_ch > states.size()) return out;

    godot::Ref<QuantumDarwinismGeneratorResource> gen_res = resolve_generator_resource();
    godot::TypedArray<QuantumDarwinismChannelSpec> ch_res = gen_res.is_valid() ? gen_res->get_channels() : godot::TypedArray<QuantumDarwinismChannelSpec>();

    for (uint32_t c = 0u; c < n_ch; ++c) {
        const auto &st = states[base + c];
        godot::Dictionary d;
        d["slot_id"]                 = static_cast<int>(st.slot_id);
        d["channel_id"]              = static_cast<int>(st.channel_id);
        if (static_cast<int>(c) < ch_res.size()) {
            godot::Ref<QuantumDarwinismChannelSpec> sp = ch_res[static_cast<int>(c)];
            d["channel_name"] = sp.is_valid() ? godot::String(sp->get_channel_name()) : godot::String("channel_") + godot::itos(static_cast<int64_t>(c));
        } else {
            d["channel_name"] = godot::String("channel_") + godot::itos(static_cast<int64_t>(c));
        }
        d["value"]                   = st.value;
        d["candidate_state_mask"]    = static_cast<int>(st.candidate_state_mask);
        d["redundancy_r_delta"]      = st.redundancy_r_delta;
        d["effective_threshold_tau"] = st.effective_threshold_tau;
        d["effective_permeability"]  = st.effective_permeability;
        d["is_locked"]               = (st.is_locked != 0u);
        d["is_pinned"]               = (st.is_pinned != 0u);
        d["is_nucleated"]            = (st.is_nucleated != 0u);
        d["locked_wave_index"]       = static_cast<int>(st.locked_wave_index);
        out.push_back(d);
    }
    return out;
}

godot::Array QuantumDarwinismGeneratorNode::get_nucleation_events_as_dicts() const {
    godot::Array out;
    for (const auto &ev : solver_.nucleation_events()) {
        godot::Dictionary d;
        d["slot_id"]                   = static_cast<int>(ev.slot_id);
        d["channel_id"]                = static_cast<int>(ev.channel_id);
        d["wave_index"]                = static_cast<int>(ev.wave_index);
        d["pre_nucleation_redundancy"] = ev.pre_nucleation_redundancy;
        out.push_back(d);
    }
    return out;
}

godot::Array QuantumDarwinismGeneratorNode::export_solved_configuration_summary() const {
    godot::Array out;
    const auto &env = active_environment();
    for (size_t i = 0; i < env.slots().size(); ++i) {
        const auto &s = env.slots()[i];
        godot::Dictionary d;
        d["slot_id"]            = static_cast<int>(s.slot_id);
        d["parent_slot_id"]     = (s.parent_slot_id == 0xFFFFFFFFu) ? -1 : static_cast<int>(s.parent_slot_id);
        d["collapsed_token_id"] = s.collapsed_token_id;
        d["token_name"]         = get_collapsed_token_name(static_cast<int>(i));
        d["einselection_score"] = s.einselection_score;
        d["is_pointer_state"]   = (s.is_pointer_state != 0u);
        d["has_conflict"]       = (s.has_conflict != 0u);
        d["channels"]           = get_slot_channel_states_as_dicts(static_cast<int>(i));
        out.push_back(d);
    }
    return out;
}

godot::Dictionary QuantumDarwinismGeneratorNode::apply_resolved_channels_to_object_tree(godot::Node *p_root) const {
    godot::Dictionary report;
    int applied_nodes      = 0;
    int properties_written = 0;
    int groups_added       = 0;
    int metadata_stamped   = 0;

    if (!p_root) {
        report["applied_nodes"]      = 0;
        report["properties_written"] = 0;
        report["groups_added"]       = 0;
        report["metadata_stamped"]   = 0;
        return report;
    }

    const auto &env      = active_environment();
    const auto &slots    = env.slots();
    const auto &specs    = solver_.channel_specs();
    const auto &states   = solver_.slot_channel_states();
    const uint32_t n_ch  = static_cast<uint32_t>(specs.size());
    godot::Ref<QuantumDarwinismGeneratorResource> gen_res = resolve_generator_resource();
    godot::TypedArray<QuantumDarwinismChannelSpec> ch_res = gen_res.is_valid()
        ? gen_res->get_channels()
        : godot::TypedArray<QuantumDarwinismChannelSpec>();

    // Collect nodes in BFS order so slot indices map cleanly to SceneTree hierarchy when no explicit slot meta is set
    std::vector<godot::Node *> bfs_nodes;
    std::queue<godot::Node *> q;
    q.push(p_root);
    while (!q.empty()) {
        godot::Node *cur = q.front();
        q.pop();
        bfs_nodes.push_back(cur);
        for (int i = 0; i < cur->get_child_count(); ++i) {
            if (godot::Node *child = cur->get_child(i)) {
                q.push(child);
            }
        }
    }

    for (size_t idx = 0; idx < bfs_nodes.size(); ++idx) {
        godot::Node *node = bfs_nodes[idx];
        int slot_id = static_cast<int>(idx);
        if (node->has_meta("ideam_qd_slot_id")) {
            slot_id = static_cast<int>(node->get_meta("ideam_qd_slot_id"));
        }
        if (slot_id < 0 || static_cast<size_t>(slot_id) >= slots.size()) continue;

        const core::EnvironmentSlotPOD &slot = slots[static_cast<size_t>(slot_id)];
        node->set_meta("ideam_qd_slot_id", slot_id);
        node->set_meta("ideam_qd_einselection_score", slot.einselection_score);
        node->set_meta("ideam_qd_is_pointer_state", slot.is_pointer_state != 0u);
        const godot::String tok_name = get_collapsed_token_name(slot_id);
        if (!tok_name.is_empty()) {
            node->set_meta("ideam_qd_resolved_token", tok_name);
        }
        metadata_stamped += 3;
        ++applied_nodes;

        const size_t base = static_cast<size_t>(slot_id) * n_ch;
        if (n_ch == 0u || base + n_ch > states.size()) continue;

        for (uint32_t c = 0u; c < n_ch; ++c) {
            const core::SlotChannelStatePOD &st   = states[base + c];
            const core::PropertyChannelSpecPOD &sp = specs[c];
            if (st.is_locked == 0u && st.redundancy_r_delta <= 1e-4f) continue;

            godot::StringName ch_name = godot::StringName(godot::String("channel_") + godot::itos(static_cast<int64_t>(c)));
            if (static_cast<int>(c) < ch_res.size()) {
                godot::Ref<QuantumDarwinismChannelSpec> cr = ch_res[static_cast<int>(c)];
                if (cr.is_valid() && cr->get_channel_name() != godot::StringName()) {
                    ch_name = cr->get_channel_name();
                }
            }

            switch (sp.aspect) {
                case core::QDChannelAspect::ASPECT_REFLECTED_PROPERTY: {
                    const godot::Variant current = node->get(ch_name);
                    if (current.get_type() != godot::Variant::NIL) {
                        switch (current.get_type()) {
                            case godot::Variant::BOOL:
                                node->set(ch_name, st.value >= 0.5f);
                                break;
                            case godot::Variant::INT:
                                node->set(ch_name, static_cast<int64_t>(std::llround(st.value)));
                                break;
                            default:
                                node->set(ch_name, st.value);
                                break;
                        }
                        ++properties_written;
                    } else {
                        node->set_meta(godot::String("ideam_qd_prop_") + godot::String(ch_name), st.value);
                        ++metadata_stamped;
                    }
                    break;
                }
                case core::QDChannelAspect::ASPECT_GROUP_OR_TAG: {
                    const bool active = (sp.is_discrete_mask != 0u) ? (st.candidate_state_mask != 0u) : (st.value >= 0.5f);
                    if (active && !node->is_in_group(ch_name)) {
                        node->add_to_group(ch_name, true);
                        ++groups_added;
                    }
                    break;
                }
                default: {
                    const godot::String meta_key = godot::String("ideam_qd_channel_") + godot::String(ch_name);
                    node->set_meta(meta_key, sp.is_discrete_mask ? godot::Variant(static_cast<int>(st.candidate_state_mask)) : godot::Variant(st.value));
                    ++metadata_stamped;
                    break;
                }
            }
        }
    }

    report["applied_nodes"]      = applied_nodes;
    report["properties_written"] = properties_written;
    report["groups_added"]       = groups_added;
    report["metadata_stamped"]   = metadata_stamped;
    return report;
}

} // namespace ideam::godot_ext
