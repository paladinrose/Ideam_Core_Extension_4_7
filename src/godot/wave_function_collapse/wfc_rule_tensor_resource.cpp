#include "wfc_rule_tensor_resource.h"
#include <godot_cpp/classes/scene_state.hpp>
#include <algorithm>

namespace ideam::godot_ext {

void WFCRuleTensorResource::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_state_count", "state_count"), &WFCRuleTensorResource::set_state_count);
    ClassDB::bind_method(D_METHOD("get_state_count"), &WFCRuleTensorResource::get_state_count);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "state_count", PROPERTY_HINT_RANGE, "1,256,1"),
        "set_state_count",
        "get_state_count"
    );

    ClassDB::bind_method(D_METHOD("clear_observed_rules"), &WFCRuleTensorResource::clear_observed_rules);
    ClassDB::bind_method(D_METHOD("get_observed_rule_count"), &WFCRuleTensorResource::get_observed_rule_count);
    ClassDB::bind_method(
        D_METHOD("add_observed_co_occurrence", "anchor_state", "neighbor_state", "port_tag"),
        &WFCRuleTensorResource::add_observed_co_occurrence
    );
}

void WFCRuleTensorResource::set_state_count(int p_count) {
    state_count_ = std::clamp(p_count, 1, 256);
    emit_changed();
}

void WFCRuleTensorResource::clear_observed_rules() {
    co_occurrence_rules_.clear();
    emit_changed();
}

void WFCRuleTensorResource::add_observed_co_occurrence(int p_anchor_state, int p_neighbor_state, int p_port_tag) {
    core::WFCObservedCoOccurrencePod rule{};
    rule.parent_or_anchor_state  = static_cast<uint32_t>(std::max(0, p_anchor_state));
    rule.child_or_neighbor_state = static_cast<uint32_t>(std::max(0, p_neighbor_state));
    rule.port_or_socket_tag      = static_cast<uint32_t>(std::max(0, p_port_tag));
    co_occurrence_rules_.push_back(rule);
    emit_changed();
}

void WFCSceneAnalyzer::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(
        D_METHOD("extract_rules_from_packed_scene", "scene"),
        &WFCSceneAnalyzer::extract_rules_from_packed_scene
    );
}

godot::Ref<WFCRuleTensorResource> WFCSceneAnalyzer::extract_rules_from_packed_scene(
    const godot::Ref<godot::PackedScene> &p_scene
) const {
    godot::Ref<WFCRuleTensorResource> rules;
    rules.instantiate();
    if (p_scene.is_null()) {
        return rules;
    }
    const godot::Ref<godot::SceneState> state = p_scene->get_state();
    if (state.is_null()) {
        return rules;
    }
    const int n_nodes = state->get_node_count();
    rules->set_state_count(std::clamp(n_nodes, 1, 256));
    for (int i = 1; i < n_nodes; ++i) {
        rules->add_observed_co_occurrence(0, i, i % 6);
    }
    return rules;
}

void WFCSolverNode::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_slot_count", "slot_count"), &WFCSolverNode::set_slot_count);
    ClassDB::bind_method(D_METHOD("get_slot_count"), &WFCSolverNode::get_slot_count);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "slot_count", PROPERTY_HINT_RANGE, "1,4096,1"),
        "set_slot_count",
        "get_slot_count"
    );

    ClassDB::bind_method(D_METHOD("pin_landmark_state", "slot_id", "state_id"), &WFCSolverNode::pin_landmark_state);
    ClassDB::bind_method(D_METHOD("clear_pinned_landmarks"), &WFCSolverNode::clear_pinned_landmarks);
    ClassDB::bind_method(D_METHOD("run_wfc_collapse"), &WFCSolverNode::run_wfc_collapse);
}

void WFCSolverNode::set_slot_count(int p_count) {
    slot_count_ = std::max(1, p_count);
}

void WFCSolverNode::pin_landmark_state(int p_slot_id, int p_state_id) {
    if (p_slot_id >= 0 && p_state_id >= 0) {
        solver_core_.add_pinned_landmark(static_cast<uint32_t>(p_slot_id), static_cast<uint32_t>(p_state_id));
    }
}

void WFCSolverNode::clear_pinned_landmarks() {
    solver_core_.clear_pinned_landmarks();
}

godot::PackedInt32Array WFCSolverNode::run_wfc_collapse() {
    int state_cnt = 4;
    int64_t seed  = 1337;
    if (get_generator_resource().is_valid()) {
        seed = get_generator_resource()->get_master_seed();
        if (auto *wfc_res = godot::Object::cast_to<WFCRuleTensorResource>(get_generator_resource().ptr())) {
            state_cnt = wfc_res->get_state_count();
        }
    }
    solver_core_.configure_slots(static_cast<uint32_t>(slot_count_), static_cast<uint32_t>(state_cnt), static_cast<uint64_t>(seed));
    solver_core_.solve_all();

    godot::PackedInt32Array out;
    out.resize(slot_count_);
    for (int i = 0; i < slot_count_; ++i) {
        out.set(i, solver_core_.collapsed_state_at(static_cast<uint32_t>(i)));
    }
    return out;
}

void WFCRuleTensorInspector::_bind_methods() {}

bool WFCRuleTensorInspector::_can_handle(godot::Object *p_object) {
    return p_object != nullptr && godot::Object::cast_to<WFCRuleTensorResource>(p_object) != nullptr;
}

} // namespace ideam::godot_ext
