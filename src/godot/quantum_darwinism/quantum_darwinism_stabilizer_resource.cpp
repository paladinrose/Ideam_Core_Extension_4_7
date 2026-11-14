#include "quantum_darwinism_stabilizer_resource.h"
#include "ideam_quantum_darwinism_plugin.h"
#include <algorithm>

namespace ideam::godot_ext {

void QuantumDarwinismStabilizerResource::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_max_horizon_radius", "radius"), &QuantumDarwinismStabilizerResource::set_max_horizon_radius);
    ClassDB::bind_method(D_METHOD("get_max_horizon_radius"), &QuantumDarwinismStabilizerResource::get_max_horizon_radius);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "max_horizon_radius", PROPERTY_HINT_RANGE, "1,16,1"),
        "set_max_horizon_radius",
        "get_max_horizon_radius"
    );

    ClassDB::bind_method(D_METHOD("set_max_horizon_slots", "max_slots"), &QuantumDarwinismStabilizerResource::set_max_horizon_slots);
    ClassDB::bind_method(D_METHOD("get_max_horizon_slots"), &QuantumDarwinismStabilizerResource::get_max_horizon_slots);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "max_horizon_slots", PROPERTY_HINT_RANGE, "4,4096,1"),
        "set_max_horizon_slots",
        "get_max_horizon_slots"
    );

    ClassDB::bind_method(
        D_METHOD("set_pointer_redundancy_threshold", "threshold"),
        &QuantumDarwinismStabilizerResource::set_pointer_redundancy_threshold
    );
    ClassDB::bind_method(
        D_METHOD("get_pointer_redundancy_threshold"),
        &QuantumDarwinismStabilizerResource::get_pointer_redundancy_threshold
    );
    ADD_PROPERTY(
        PropertyInfo(Variant::FLOAT, "pointer_redundancy_threshold", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"),
        "set_pointer_redundancy_threshold",
        "get_pointer_redundancy_threshold"
    );

    ClassDB::bind_method(
        D_METHOD("set_fragile_redundancy_threshold", "threshold"),
        &QuantumDarwinismStabilizerResource::set_fragile_redundancy_threshold
    );
    ClassDB::bind_method(
        D_METHOD("get_fragile_redundancy_threshold"),
        &QuantumDarwinismStabilizerResource::get_fragile_redundancy_threshold
    );
    ADD_PROPERTY(
        PropertyInfo(Variant::FLOAT, "fragile_redundancy_threshold", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"),
        "set_fragile_redundancy_threshold",
        "get_fragile_redundancy_threshold"
    );

    ClassDB::bind_method(D_METHOD("set_min_locked_layer", "layer"), &QuantumDarwinismStabilizerResource::set_min_locked_layer);
    ClassDB::bind_method(D_METHOD("get_min_locked_layer"), &QuantumDarwinismStabilizerResource::get_min_locked_layer);
    ADD_PROPERTY(
        PropertyInfo(
            Variant::INT,
            "min_locked_layer",
            PROPERTY_HINT_ENUM,
            "Domain:0,HierarchyParent:1,SiblingDependency:2,ChildCardinality:3,SpatialSocket:4,VariantDetail:5,PinnedOverride:6"
        ),
        "set_min_locked_layer",
        "get_min_locked_layer"
    );

    ClassDB::bind_method(
        D_METHOD("set_allow_sibling_rebalancing", "allow"),
        &QuantumDarwinismStabilizerResource::set_allow_sibling_rebalancing
    );
    ClassDB::bind_method(
        D_METHOD("get_allow_sibling_rebalancing"),
        &QuantumDarwinismStabilizerResource::get_allow_sibling_rebalancing
    );
    ADD_PROPERTY(
        PropertyInfo(Variant::BOOL, "allow_sibling_rebalancing"),
        "set_allow_sibling_rebalancing",
        "get_allow_sibling_rebalancing"
    );

    ClassDB::bind_method(
        D_METHOD("set_lock_nuclear_semes", "lock"),
        &QuantumDarwinismStabilizerResource::set_lock_nuclear_semes
    );
    ClassDB::bind_method(
        D_METHOD("get_lock_nuclear_semes"),
        &QuantumDarwinismStabilizerResource::get_lock_nuclear_semes
    );
    ADD_PROPERTY(
        PropertyInfo(Variant::BOOL, "lock_nuclear_semes"),
        "set_lock_nuclear_semes",
        "get_lock_nuclear_semes"
    );

    ClassDB::bind_method(D_METHOD("set_pattern", "pattern"), &QuantumDarwinismStabilizerResource::set_pattern);
    ClassDB::bind_method(D_METHOD("get_pattern"), &QuantumDarwinismStabilizerResource::get_pattern);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "pattern", PROPERTY_HINT_RESOURCE_TYPE, "ProceduralPattern"),
        "set_pattern",
        "get_pattern"
    );

    ClassDB::bind_method(
        D_METHOD("commit_set_max_horizon_radius", "new_radius", "undo_redo"),
        &QuantumDarwinismStabilizerResource::commit_set_max_horizon_radius,
        DEFVAL(Variant())
    );
    ClassDB::bind_method(
        D_METHOD("commit_set_pointer_redundancy_threshold", "new_threshold", "undo_redo"),
        &QuantumDarwinismStabilizerResource::commit_set_pointer_redundancy_threshold,
        DEFVAL(Variant())
    );
}

void QuantumDarwinismStabilizerResource::set_max_horizon_radius(int p_radius) {
    max_horizon_radius_ = std::clamp(p_radius, 1, 16);
    emit_changed();
}

void QuantumDarwinismStabilizerResource::set_max_horizon_slots(int p_max_slots) {
    max_horizon_slots_ = std::clamp(p_max_slots, 4, 4096);
    emit_changed();
}

void QuantumDarwinismStabilizerResource::set_pointer_redundancy_threshold(float p_thresh) {
    pointer_redundancy_thresh_ = std::clamp(p_thresh, 0.0f, 1.0f);
    emit_changed();
}

void QuantumDarwinismStabilizerResource::set_fragile_redundancy_threshold(float p_thresh) {
    fragile_redundancy_thresh_ = std::clamp(p_thresh, 0.0f, 1.0f);
    emit_changed();
}

void QuantumDarwinismStabilizerResource::set_min_locked_layer(int p_layer) {
    min_locked_layer_ = std::clamp(p_layer, 0, 6);
    emit_changed();
}

void QuantumDarwinismStabilizerResource::set_allow_sibling_rebalancing(bool p_allow) {
    allow_sibling_rebalancing_ = p_allow;
    emit_changed();
}

void QuantumDarwinismStabilizerResource::set_lock_nuclear_semes(bool p_lock) {
    lock_nuclear_semes_ = p_lock;
    emit_changed();
}

core::HorizonBoxConfigPOD QuantumDarwinismStabilizerResource::to_pod() const noexcept {
    core::HorizonBoxConfigPOD cfg{};
    cfg.max_graph_hop_radius      = static_cast<uint32_t>(std::clamp(max_horizon_radius_, 1, 16));
    cfg.max_horizon_slots         = static_cast<uint32_t>(std::clamp(max_horizon_slots_, 4, 4096));
    cfg.pointer_redundancy_thresh = pointer_redundancy_thresh_;
    cfg.fragile_redundancy_thresh = fragile_redundancy_thresh_;
    cfg.min_locked_layer          = static_cast<core::PatternCascadeLayer>(std::clamp(min_locked_layer_, 0, 6));
    cfg.allow_sibling_rebalancing = allow_sibling_rebalancing_ ? 1u : 0u;
    cfg.lock_nuclear_semes        = lock_nuclear_semes_ ? 1u : 0u;
    return cfg;
}

void QuantumDarwinismStabilizerResource::commit_set_max_horizon_radius(
    int p_new_radius,
    const godot::Ref<IdeamUndoRedo> &p_undo_redo
) {
    const int clamped = std::clamp(p_new_radius, 1, 16);
    if (clamped == max_horizon_radius_) {
        return;
    }
    godot::Ref<IdeamUndoRedo> ur = p_undo_redo.is_valid()
        ? p_undo_redo
        : IdeamQuantumDarwinismPlugin::create_undo_redo_wrapper();
    if (ur.is_valid()) {
        const int old_val = max_horizon_radius_;
        ur->create_action("Set Quantum Darwinism Horizon Radius");
        ur->add_do_method(this, "set_max_horizon_radius", clamped);
        ur->add_undo_method(this, "set_max_horizon_radius", old_val);
        ur->commit_action();
    } else {
        set_max_horizon_radius(clamped);
    }
}

void QuantumDarwinismStabilizerResource::commit_set_pointer_redundancy_threshold(
    float p_new_thresh,
    const godot::Ref<IdeamUndoRedo> &p_undo_redo
) {
    const float clamped = std::clamp(p_new_thresh, 0.0f, 1.0f);
    godot::Ref<IdeamUndoRedo> ur = p_undo_redo.is_valid()
        ? p_undo_redo
        : IdeamQuantumDarwinismPlugin::create_undo_redo_wrapper();
    if (ur.is_valid()) {
        const float old_val = pointer_redundancy_thresh_;
        ur->create_action("Set Pointer State Redundancy Threshold");
        ur->add_do_method(this, "set_pointer_redundancy_threshold", clamped);
        ur->add_undo_method(this, "set_pointer_redundancy_threshold", old_val);
        ur->commit_action();
    } else {
        set_pointer_redundancy_threshold(clamped);
    }
}

// ============================================================================
// QuantumDarwinismStabilizerNode
// ============================================================================

void QuantumDarwinismStabilizerNode::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(
        D_METHOD("set_target_wfc_solver_path", "path"),
        &QuantumDarwinismStabilizerNode::set_target_wfc_solver_path
    );
    ClassDB::bind_method(
        D_METHOD("get_target_wfc_solver_path"),
        &QuantumDarwinismStabilizerNode::get_target_wfc_solver_path
    );
    ADD_PROPERTY(
        PropertyInfo(Variant::NODE_PATH, "target_wfc_solver_path"),
        "set_target_wfc_solver_path",
        "get_target_wfc_solver_path"
    );

    ClassDB::bind_method(
        D_METHOD("stabilize_wfc_slot", "conflict_slot_id"),
        &QuantumDarwinismStabilizerNode::stabilize_wfc_slot
    );
    ClassDB::bind_method(
        D_METHOD("stabilize_all_wfc_conflicts"),
        &QuantumDarwinismStabilizerNode::stabilize_all_wfc_conflicts
    );
    ClassDB::bind_method(
        D_METHOD("get_einselection_status"),
        &QuantumDarwinismStabilizerNode::get_einselection_status
    );
    ClassDB::bind_method(
        D_METHOD("get_isolated_horizon_slots"),
        &QuantumDarwinismStabilizerNode::get_isolated_horizon_slots
    );
    ClassDB::bind_method(
        D_METHOD("get_pointer_states_as_dicts"),
        &QuantumDarwinismStabilizerNode::get_pointer_states_as_dicts
    );
    ClassDB::bind_method(
        D_METHOD("get_coherence_bridge_slots_as_dicts"),
        &QuantumDarwinismStabilizerNode::get_coherence_bridge_slots_as_dicts
    );
}

void QuantumDarwinismStabilizerNode::sync_config_from_resource() {
    if (get_generator_resource().is_valid()) {
        if (auto *qd_res = godot::Object::cast_to<QuantumDarwinismStabilizerResource>(get_generator_resource().ptr())) {
            solver_.set_horizon_config(qd_res->to_pod());
        }
    }
}

WFCSolverNode *QuantumDarwinismStabilizerNode::resolve_target_wfc_node() const {
    if (!target_wfc_solver_path_.is_empty()) {
        if (godot::Node *n = get_node_or_null(target_wfc_solver_path_)) {
            if (auto *wfc = godot::Object::cast_to<WFCSolverNode>(n)) {
                return wfc;
            }
        }
    }
    if (godot::Node *p = get_parent()) {
        if (auto *wfc = godot::Object::cast_to<WFCSolverNode>(p)) {
            return wfc;
        }
    }
    return nullptr;
}

int QuantumDarwinismStabilizerNode::stabilize_wfc_slot(int p_conflict_slot_id) {
    sync_config_from_resource();
    WFCSolverNode *wfc = resolve_target_wfc_node();
    if (!wfc || p_conflict_slot_id < 0) {
        return static_cast<int>(core::EinselectionStatus::UNRESOLVABLE_HORIZON);
    }
    const core::EinselectionStatus st = solver_.stabilize_conflict_in_environment(
        wfc->environment(),
        static_cast<uint32_t>(p_conflict_slot_id)
    );
    wfc->solver_core().sync_from_bound_environment();
    return static_cast<int>(st);
}

int QuantumDarwinismStabilizerNode::stabilize_all_wfc_conflicts() {
    sync_config_from_resource();
    WFCSolverNode *wfc = resolve_target_wfc_node();
    if (!wfc) {
        return static_cast<int>(core::EinselectionStatus::UNRESOLVABLE_HORIZON);
    }
    const core::EinselectionStatus st = solver_.stabilize_all_conflicts_in_environment(
        wfc->environment(),
        &wfc->solver_core()
    );
    return static_cast<int>(st);
}

godot::PackedInt32Array QuantumDarwinismStabilizerNode::get_isolated_horizon_slots() const {
    godot::PackedInt32Array out;
    for (uint32_t s_id : solver_.horizon_slot_indices()) {
        out.push_back(static_cast<int32_t>(s_id));
    }
    return out;
}

godot::Array QuantumDarwinismStabilizerNode::get_pointer_states_as_dicts() const {
    godot::Array out;
    for (const auto &p : solver_.pointer_states()) {
        godot::Dictionary d;
        d["slot_id"]                  = static_cast<int>(p.slot_id);
        d["settled_token_id"]         = static_cast<int>(p.settled_token_id);
        d["einselection_redundancy"]  = p.einselection_redundancy;
        d["highest_satisfied_spec"]   = static_cast<int>(p.highest_satisfied_spec);
        d["preserved_trait_mask"]     = static_cast<int>(p.preserved_trait_mask);
        d["hop_distance_to_conflict"] = static_cast<int>(p.hop_distance_to_conflict);
        out.push_back(d);
    }
    return out;
}

godot::Array QuantumDarwinismStabilizerNode::get_coherence_bridge_slots_as_dicts() const {
    godot::Array out;
    for (const auto &b : solver_.bridge_slots()) {
        godot::Dictionary d;
        d["slot_id"]                 = static_cast<int>(b.slot_id);
        d["source_pointer_slot_id"]  = static_cast<int>(b.source_pointer_slot_id);
        d["target_conflict_slot_id"] = static_cast<int>(b.target_conflict_slot_id);
        d["hops_from_pointer"]       = static_cast<int>(b.hops_from_pointer);
        d["hops_to_conflict"]        = static_cast<int>(b.hops_to_conflict);
        d["mutable_trait_mask"]      = static_cast<int>(b.mutable_trait_mask);
        d["relax_down_to"]           = static_cast<int>(b.relax_down_to);
        out.push_back(d);
    }
    return out;
}

} // namespace ideam::godot_ext
