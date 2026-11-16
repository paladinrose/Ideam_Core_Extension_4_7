#include "wfc_rule_tensor_resource.h"
#include "ideam_wave_function_collapse_plugin.h"
#include <godot_cpp/classes/scene_state.hpp>
#include <algorithm>
#include <string>

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

    ClassDB::bind_method(D_METHOD("set_entropy_heuristic", "heuristic"), &WFCRuleTensorResource::set_entropy_heuristic);
    ClassDB::bind_method(D_METHOD("get_entropy_heuristic"), &WFCRuleTensorResource::get_entropy_heuristic);
    ADD_PROPERTY(
        PropertyInfo(
            Variant::INT,
            "entropy_heuristic",
            PROPERTY_HINT_ENUM,
            "ShannonWeighted:0,PopcountMRV:1,HierarchyTopDownShannon:2,CascadeLayerPriority:3"
        ),
        "set_entropy_heuristic",
        "get_entropy_heuristic"
    );

    ClassDB::bind_method(
        D_METHOD("set_enable_hierarchy_and_cardinality_rules", "enable"),
        &WFCRuleTensorResource::set_enable_hierarchy_and_cardinality_rules
    );
    ClassDB::bind_method(
        D_METHOD("get_enable_hierarchy_and_cardinality_rules"),
        &WFCRuleTensorResource::get_enable_hierarchy_and_cardinality_rules
    );
    ADD_PROPERTY(
        PropertyInfo(Variant::BOOL, "enable_hierarchy_and_cardinality_rules"),
        "set_enable_hierarchy_and_cardinality_rules",
        "get_enable_hierarchy_and_cardinality_rules"
    );

    ClassDB::bind_method(
        D_METHOD("set_contradiction_horizon_radius", "radius"),
        &WFCRuleTensorResource::set_contradiction_horizon_radius
    );
    ClassDB::bind_method(
        D_METHOD("get_contradiction_horizon_radius"),
        &WFCRuleTensorResource::get_contradiction_horizon_radius
    );
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "contradiction_horizon_radius", PROPERTY_HINT_RANGE, "1,16,1"),
        "set_contradiction_horizon_radius",
        "get_contradiction_horizon_radius"
    );

    ClassDB::bind_method(D_METHOD("set_token_names", "names"), &WFCRuleTensorResource::set_token_names);
    ClassDB::bind_method(D_METHOD("get_token_names"), &WFCRuleTensorResource::get_token_names);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "token_names"), "set_token_names", "get_token_names");

    ClassDB::bind_method(D_METHOD("set_token_weights", "weights"), &WFCRuleTensorResource::set_token_weights);
    ClassDB::bind_method(D_METHOD("get_token_weights"), &WFCRuleTensorResource::get_token_weights);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "token_weights"), "set_token_weights", "get_token_weights");

    ClassDB::bind_method(D_METHOD("set_source_pattern", "pattern"), &WFCRuleTensorResource::set_source_pattern);
    ClassDB::bind_method(D_METHOD("get_source_pattern"), &WFCRuleTensorResource::get_source_pattern);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "source_pattern", PROPERTY_HINT_RESOURCE_TYPE, "ProceduralPattern"),
        "set_source_pattern",
        "get_source_pattern"
    );

    ClassDB::bind_method(D_METHOD("clear_observed_rules"), &WFCRuleTensorResource::clear_observed_rules);
    ClassDB::bind_method(D_METHOD("get_observed_rule_count"), &WFCRuleTensorResource::get_observed_rule_count);
    ClassDB::bind_method(
        D_METHOD("add_observed_co_occurrence", "anchor_state", "neighbor_state", "port_tag"),
        &WFCRuleTensorResource::add_observed_co_occurrence
    );
    ClassDB::bind_method(
        D_METHOD(
            "add_observed_hierarchy_rule",
            "parent_state",
            "child_state",
            "min_occurrences",
            "max_occurrences",
            "required_sibling_state",
            "forbidden_sibling_state"
        ),
        &WFCRuleTensorResource::add_observed_hierarchy_rule,
        DEFVAL(-1),
        DEFVAL(-1)
    );
    ClassDB::bind_method(D_METHOD("get_observed_rules_as_dicts"), &WFCRuleTensorResource::get_observed_rules_as_dicts);
    ClassDB::bind_method(
        D_METHOD("compile_from_procedural_pattern", "pattern"),
        &WFCRuleTensorResource::compile_from_procedural_pattern
    );

    ClassDB::bind_method(
        D_METHOD("commit_set_state_count", "new_count", "undo_redo"),
        &WFCRuleTensorResource::commit_set_state_count,
        DEFVAL(Variant())
    );
    ClassDB::bind_method(
        D_METHOD("commit_add_observed_co_occurrence", "anchor_state", "neighbor_state", "port_tag", "undo_redo"),
        &WFCRuleTensorResource::commit_add_observed_co_occurrence,
        DEFVAL(Variant())
    );
    ClassDB::bind_method(
        D_METHOD("commit_clear_observed_rules", "undo_redo"),
        &WFCRuleTensorResource::commit_clear_observed_rules,
        DEFVAL(Variant())
    );
}

void WFCRuleTensorResource::set_state_count(int p_count) {
    state_count_ = std::clamp(p_count, 1, 256);
    emit_changed();
}

void WFCRuleTensorResource::set_entropy_heuristic(int p_heuristic) {
    entropy_heuristic_ = std::clamp(p_heuristic, 0, 3);
    emit_changed();
}

void WFCRuleTensorResource::set_enable_hierarchy_and_cardinality_rules(bool p_enable) {
    enable_hierarchy_and_cardinality_rules_ = p_enable;
    emit_changed();
}

void WFCRuleTensorResource::set_contradiction_horizon_radius(int p_radius) {
    contradiction_horizon_radius_ = std::clamp(p_radius, 1, 16);
    emit_changed();
}

void WFCRuleTensorResource::set_token_names(const godot::PackedStringArray &p_names) {
    token_names_ = p_names;
    if (token_names_.size() > 0) {
        state_count_ = std::clamp(static_cast<int>(token_names_.size()), 1, 256);
    }
    emit_changed();
}

void WFCRuleTensorResource::set_token_weights(const godot::PackedFloat32Array &p_weights) {
    token_weights_ = p_weights;
    emit_changed();
}

void WFCRuleTensorResource::set_source_pattern(const godot::Ref<ProceduralPattern> &p_pattern) {
    source_pattern_ = p_pattern;
    if (source_pattern_.is_valid()) {
        compile_from_procedural_pattern(source_pattern_);
    }
    emit_changed();
}

void WFCRuleTensorResource::clear_observed_rules() {
    co_occurrence_rules_.clear();
    explicit_rule_entries_.clear();
    emit_changed();
}

void WFCRuleTensorResource::add_observed_co_occurrence(int p_anchor_state, int p_neighbor_state, int p_port_tag) {
    core::WFCObservedCoOccurrencePod rule{};
    rule.parent_or_anchor_state  = static_cast<uint32_t>(std::clamp(p_anchor_state, 0, 255));
    rule.child_or_neighbor_state = static_cast<uint32_t>(std::clamp(p_neighbor_state, 0, 255));
    rule.port_or_socket_tag      = static_cast<uint32_t>(std::max(0, p_port_tag));
    rule.axis                    = core::WFCConstraintAxis::AXIS_SPATIAL_PORT;
    co_occurrence_rules_.push_back(rule);
    emit_changed();
}

void WFCRuleTensorResource::add_observed_hierarchy_rule(
    int p_parent_state,
    int p_child_state,
    int p_min_occurrences,
    int p_max_occurrences,
    int p_required_sibling_state,
    int p_forbidden_sibling_state
) {
    core::WFCObservedCoOccurrencePod rule{};
    rule.parent_or_anchor_state  = static_cast<uint32_t>(std::clamp(p_parent_state, 0, 255));
    rule.child_or_neighbor_state = static_cast<uint32_t>(std::clamp(p_child_state, 0, 255));
    rule.port_or_socket_tag      = 0u;
    rule.min_occurrences         = static_cast<uint32_t>(std::max(0, p_min_occurrences));
    rule.max_occurrences         = static_cast<uint32_t>(std::max(static_cast<int>(rule.min_occurrences), p_max_occurrences));
    rule.required_sibling_state  = (p_required_sibling_state >= 0)
        ? static_cast<uint32_t>(p_required_sibling_state)
        : 0xFFFFFFFFu;
    rule.forbidden_sibling_state = (p_forbidden_sibling_state >= 0)
        ? static_cast<uint32_t>(p_forbidden_sibling_state)
        : 0xFFFFFFFFu;
    rule.axis                    = core::WFCConstraintAxis::AXIS_HIERARCHY_PARENT_TO_CHILD;
    co_occurrence_rules_.push_back(rule);
    emit_changed();
}

godot::Array WFCRuleTensorResource::get_observed_rules_as_dicts() const {
    godot::Array out;
    for (const auto &r : co_occurrence_rules_) {
        godot::Dictionary d;
        d["anchor_state"]            = static_cast<int>(r.parent_or_anchor_state);
        d["neighbor_state"]          = static_cast<int>(r.child_or_neighbor_state);
        d["port_tag"]                = static_cast<int>(r.port_or_socket_tag);
        d["min_occurrences"]         = static_cast<int>(r.min_occurrences);
        d["max_occurrences"]         = static_cast<int>(r.max_occurrences);
        d["required_sibling_state"]  = (r.required_sibling_state == 0xFFFFFFFFu)
            ? -1
            : static_cast<int>(r.required_sibling_state);
        d["forbidden_sibling_state"] = (r.forbidden_sibling_state == 0xFFFFFFFFu)
            ? -1
            : static_cast<int>(r.forbidden_sibling_state);
        d["axis"]                    = static_cast<int>(r.axis);
        out.push_back(d);
    }
    return out;
}

void WFCRuleTensorResource::compile_from_procedural_pattern(const godot::Ref<ProceduralPattern> &p_pattern) {
    if (p_pattern.is_null()) {
        return;
    }
    source_pattern_ = p_pattern;
    core::ProceduralEnvironment temp_env;
    p_pattern->compile_to_environment(temp_env, 1u);

    const auto &names  = temp_env.token_names();
    const auto &tokens = temp_env.tokens();
    token_names_.clear();
    token_weights_.clear();
    for (size_t i = 0; i < names.size() && i < 256u; ++i) {
        token_names_.push_back(godot::String(names[i].c_str()));
        token_weights_.push_back(i < tokens.size() ? tokens[i].base_frequency_weight : 1.0f);
    }
    state_count_ = std::clamp(static_cast<int>(token_names_.size()), 1, 256);

    // Synthesize explicit WFC rule entries from the compiled ProceduralEnvironment rules
    explicit_rule_entries_.clear();
    const auto &rules = temp_env.active_rules();
    const auto &masks = temp_env.rule_token_masks();
    for (const auto &rule : rules) {
        if (rule.target_token_mask_index >= masks.size()) {
            continue;
        }
        const core::BitmaskDomain256POD &mask = masks[rule.target_token_mask_index];
        for (size_t t = 0; t < tokens.size(); ++t) {
            if (!rule.selector.matches_candidate(tokens[t].signature)) {
                continue;
            }
            core::WFCRuleEntryPOD entry{};
            entry.source_state    = static_cast<uint32_t>(t);
            entry.allowed_mask    = mask;
            entry.min_cardinality = static_cast<uint8_t>(std::min<uint16_t>(255u, rule.min_occurrences));
            entry.max_cardinality = static_cast<uint8_t>(std::min<uint16_t>(255u, rule.max_occurrences));
            if (rule.operation == core::GenerationRuleOp::ACCEPT_PARENTS) {
                entry.port_id = 0u;
                entry.axis    = core::WFCConstraintAxis::AXIS_HIERARCHY_CHILD_TO_PARENT;
                explicit_rule_entries_.push_back(entry);
            } else if (rule.operation == core::GenerationRuleOp::ACCEPT_CHILDREN_CARDINALITY) {
                entry.port_id = 0u;
                entry.axis    = core::WFCConstraintAxis::AXIS_HIERARCHY_PARENT_TO_CHILD;
                explicit_rule_entries_.push_back(entry);
            } else if (rule.operation == core::GenerationRuleOp::ALLOW_SPATIAL_NEIGHBOR) {
                entry.port_id = static_cast<uint32_t>(std::max<int16_t>(0, rule.priority));
                entry.axis    = core::WFCConstraintAxis::AXIS_SPATIAL_PORT;
                explicit_rule_entries_.push_back(entry);
            }
        }
    }
    emit_changed();
}

void WFCRuleTensorResource::apply_to_solver_and_environment(
    core::WFCSolverCore &r_solver,
    core::ProceduralEnvironment &r_env,
    int p_slot_count
) const {
    const uint32_t slots_cnt  = static_cast<uint32_t>(std::max(1, p_slot_count));
    const uint32_t states_cnt = static_cast<uint32_t>(std::clamp(state_count_, 1, 256));

    if (source_pattern_.is_valid()) {
        source_pattern_->compile_to_environment(r_env, slots_cnt);
    } else if (r_env.tokens().empty() || r_env.slots().size() != slots_cnt) {
        r_env.configure_slots(slots_cnt, states_cnt);
        for (uint32_t t = 0; t < states_cnt; ++t) {
            std::string t_name = (static_cast<int>(t) < token_names_.size())
                ? std::string(token_names_[static_cast<int>(t)].utf8().get_data())
                : ("State_" + std::to_string(t));
            core::TokenDescriptorPOD desc{};
            desc.token_id              = t;
            desc.base_frequency_weight = (static_cast<int>(t) < token_weights_.size())
                ? token_weights_[static_cast<int>(t)]
                : 1.0f;
            desc.signature = core::CascadeRuleEngine::build_candidate_signature("Node", t_name);
            r_env.register_token(t_name, desc);
        }
    }

    r_solver.bind_environment(&r_env);
    r_solver.set_entropy_heuristic(static_cast<core::WFCEntropyHeuristic>(std::clamp(entropy_heuristic_, 0, 3)));
    r_solver.set_enable_hierarchy_and_cardinality_rules(enable_hierarchy_and_cardinality_rules_);
    r_solver.set_recommended_horizon_radius(static_cast<uint32_t>(std::clamp(contradiction_horizon_radius_, 1, 16)));

    r_solver.initialize_from_bound_environment(static_cast<uint64_t>(get_master_seed()));

    if (token_weights_.size() > 0) {
        std::vector<float> weights(static_cast<size_t>(token_weights_.size()));
        for (int i = 0; i < token_weights_.size(); ++i) {
            weights[static_cast<size_t>(i)] = token_weights_[i];
        }
        r_solver.set_token_weights(weights);
    }

    r_solver.compile_rules_from_observed_co_occurrences(co_occurrence_rules_);
    for (const auto &entry : explicit_rule_entries_) {
        r_solver.add_rule(entry.port_id, entry.source_state, entry.allowed_mask, entry.axis, entry.rule_weight);
    }
}

void WFCRuleTensorResource::commit_set_state_count(int p_new_count, const godot::Ref<IdeamUndoRedo> &p_undo_redo) {
    const int clamped = std::clamp(p_new_count, 1, 256);
    if (clamped == state_count_) {
        return;
    }
    godot::Ref<IdeamUndoRedo> ur = p_undo_redo.is_valid()
        ? p_undo_redo
        : IdeamWaveFunctionCollapsePlugin::create_undo_redo_wrapper();
    if (ur.is_valid()) {
        const int old_count = state_count_;
        ur->create_action("Set WFC State Count");
        ur->add_do_method(this, "set_state_count", clamped);
        ur->add_undo_method(this, "set_state_count", old_count);
        ur->commit_action();
    } else {
        set_state_count(clamped);
    }
}

void WFCRuleTensorResource::commit_add_observed_co_occurrence(
    int p_anchor_state,
    int p_neighbor_state,
    int p_port_tag,
    const godot::Ref<IdeamUndoRedo> &p_undo_redo
) {
    godot::Ref<IdeamUndoRedo> ur = p_undo_redo.is_valid()
        ? p_undo_redo
        : IdeamWaveFunctionCollapsePlugin::create_undo_redo_wrapper();
    if (ur.is_valid()) {
        ur->create_action("Add WFC Observed Co-Occurrence");
        ur->add_do_method(this, "add_observed_co_occurrence", p_anchor_state, p_neighbor_state, p_port_tag);
        ur->commit_action();
    } else {
        add_observed_co_occurrence(p_anchor_state, p_neighbor_state, p_port_tag);
    }
}

void WFCRuleTensorResource::commit_clear_observed_rules(const godot::Ref<IdeamUndoRedo> &p_undo_redo) {
    godot::Ref<IdeamUndoRedo> ur = p_undo_redo.is_valid()
        ? p_undo_redo
        : IdeamWaveFunctionCollapsePlugin::create_undo_redo_wrapper();
    if (ur.is_valid()) {
        ur->create_action("Clear WFC Observed Rules");
        ur->add_do_method(this, "clear_observed_rules");
        ur->commit_action();
    } else {
        clear_observed_rules();
    }
}

// ============================================================================
// WFCSceneAnalyzer
// ============================================================================

void WFCSceneAnalyzer::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(
        D_METHOD("extract_rules_from_packed_scene", "scene", "style"),
        &WFCSceneAnalyzer::extract_rules_from_packed_scene,
        DEFVAL(Variant())
    );
    ClassDB::bind_method(
        D_METHOD("extract_rules_from_object_hierarchy", "root", "style"),
        &WFCSceneAnalyzer::extract_rules_from_object_hierarchy,
        DEFVAL(Variant())
    );
    ClassDB::bind_method(
        D_METHOD("extract_rules_from_procedural_pattern", "pattern"),
        &WFCSceneAnalyzer::extract_rules_from_procedural_pattern
    );
}

godot::Ref<WFCRuleTensorResource> WFCSceneAnalyzer::extract_rules_from_packed_scene(
    const godot::Ref<godot::PackedScene> &p_scene,
    const godot::Ref<AnalysisStyle> &p_style
) const {
    godot::Ref<WFCRuleTensorResource> rules;
    rules.instantiate();
    if (p_scene.is_null()) {
        return rules;
    }

    // Use ProceduralPatterner to harvest the full CSS cascade pattern from SceneState
    godot::Ref<ProceduralPatterner> patterner;
    patterner.instantiate();
    if (p_style.is_valid()) patterner->set_analysis_style(p_style);
    godot::Ref<ProceduralPattern> pattern = patterner->create_pattern_from_packed_scene(p_scene);
    if (pattern.is_valid()) {
        rules->compile_from_procedural_pattern(pattern);
    }

    const godot::Ref<godot::SceneState> state = p_scene->get_state();
    if (state.is_valid()) {
        const int n_nodes = state->get_node_count();
        if (rules->get_token_names().is_empty()) {
            rules->set_state_count(std::clamp(n_nodes, 1, 256));
        }
        for (int i = 1; i < n_nodes; ++i) {
            rules->add_observed_co_occurrence(0, i % rules->get_state_count(), i % 6);
        }
    }
    return rules;
}

godot::Ref<WFCRuleTensorResource> WFCSceneAnalyzer::extract_rules_from_object_hierarchy(
    godot::Object *p_root,
    const godot::Ref<AnalysisStyle> &p_style
) const {
    godot::Ref<WFCRuleTensorResource> rules;
    rules.instantiate();
    if (!p_root) {
        return rules;
    }
    godot::Ref<ProceduralPatterner> patterner;
    patterner.instantiate();
    if (p_style.is_valid()) patterner->set_analysis_style(p_style);
    godot::Ref<ProceduralPattern> pattern = patterner->create_pattern_from_object(p_root);
    if (pattern.is_valid()) {
        rules->compile_from_procedural_pattern(pattern);
    }
    return rules;
}

godot::Ref<WFCRuleTensorResource> WFCSceneAnalyzer::extract_rules_from_procedural_pattern(
    const godot::Ref<ProceduralPattern> &p_pattern
) const {
    godot::Ref<WFCRuleTensorResource> rules;
    rules.instantiate();
    if (p_pattern.is_valid()) {
        rules->compile_from_procedural_pattern(p_pattern);
    }
    return rules;
}

// ============================================================================
// WFCSolverNode
// ============================================================================

WFCSolverNode::WFCSolverNode() {
    solver_core_.bind_environment(&environment_);
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

    ClassDB::bind_method(D_METHOD("set_procedural_pattern", "pattern"), &WFCSolverNode::set_procedural_pattern);
    ClassDB::bind_method(D_METHOD("get_procedural_pattern"), &WFCSolverNode::get_procedural_pattern);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "procedural_pattern", PROPERTY_HINT_RESOURCE_TYPE, "ProceduralPattern"),
        "set_procedural_pattern",
        "get_procedural_pattern"
    );

    ClassDB::bind_method(
        D_METHOD("set_slot_children", "parent_slot_id", "child_slot_ids"),
        &WFCSolverNode::set_slot_children
    );
    ClassDB::bind_method(
        D_METHOD("add_directed_edge", "from_slot", "to_slot", "port_id", "axis", "bidirectional"),
        &WFCSolverNode::add_directed_edge,
        DEFVAL(0),
        DEFVAL(false)
    );
    ClassDB::bind_method(
        D_METHOD("configure_environment_shape", "slot_count", "slot_hierarchy_dicts"),
        &WFCSolverNode::configure_environment_shape
    );

    ClassDB::bind_method(D_METHOD("pin_landmark_state", "slot_id", "state_id"), &WFCSolverNode::pin_landmark_state);
    ClassDB::bind_method(D_METHOD("pin_landmark_by_name", "slot_id", "token_name"), &WFCSolverNode::pin_landmark_by_name);
    ClassDB::bind_method(D_METHOD("clear_pinned_landmarks"), &WFCSolverNode::clear_pinned_landmarks);

    ClassDB::bind_method(D_METHOD("run_wfc_collapse"), &WFCSolverNode::run_wfc_collapse);
    ClassDB::bind_method(D_METHOD("step_wfc_collapse"), &WFCSolverNode::step_wfc_collapse);
    ClassDB::bind_method(D_METHOD("get_solver_status"), &WFCSolverNode::get_solver_status);
    ClassDB::bind_method(D_METHOD("get_collapsed_token_name", "slot_id"), &WFCSolverNode::get_collapsed_token_name);
    ClassDB::bind_method(D_METHOD("get_slot_entropy", "slot_id"), &WFCSolverNode::get_slot_entropy);
    ClassDB::bind_method(D_METHOD("get_contradiction_slots"), &WFCSolverNode::get_contradiction_slots);
    ClassDB::bind_method(D_METHOD("get_contradiction_reports_as_dicts"), &WFCSolverNode::get_contradiction_reports_as_dicts);
    ClassDB::bind_method(
        D_METHOD("export_solved_configuration_summary"),
        &WFCSolverNode::export_solved_configuration_summary
    );
}

void WFCSolverNode::set_slot_count(int p_count) {
    slot_count_ = std::clamp(p_count, 1, 4096);
}

void WFCSolverNode::set_procedural_pattern(const godot::Ref<ProceduralPattern> &p_pattern) {
    procedural_pattern_ = p_pattern;
}

void WFCSolverNode::ensure_environment_prepared() {
    solver_core_.bind_environment(&environment_);
    if (get_generator_resource().is_valid()) {
        if (auto *wfc_res = godot::Object::cast_to<WFCRuleTensorResource>(get_generator_resource().ptr())) {
            wfc_res->apply_to_solver_and_environment(solver_core_, environment_, slot_count_);
            return;
        }
    }
    if (procedural_pattern_.is_valid()) {
        procedural_pattern_->compile_to_environment(environment_, static_cast<uint32_t>(slot_count_));
        int64_t seed = get_generator_resource().is_valid() ? get_generator_resource()->get_master_seed() : 1337;
        solver_core_.initialize_from_bound_environment(static_cast<uint64_t>(seed));
        return;
    }
    if (environment_.slots().size() != static_cast<size_t>(slot_count_)) {
        const uint32_t state_cnt = std::max<uint32_t>(4u, static_cast<uint32_t>(environment_.tokens().size()));
        environment_.configure_slots(static_cast<uint32_t>(slot_count_), state_cnt);
        int64_t seed = get_generator_resource().is_valid() ? get_generator_resource()->get_master_seed() : 1337;
        solver_core_.configure_slots(static_cast<uint32_t>(slot_count_), state_cnt, static_cast<uint64_t>(seed));
        solver_core_.sync_from_bound_environment();
    }
}

void WFCSolverNode::set_slot_children(int p_parent_slot_id, const godot::PackedInt32Array &p_child_slot_ids) {
    ensure_environment_prepared();
    if (p_parent_slot_id < 0 || static_cast<size_t>(p_parent_slot_id) >= environment_.slots().size()) {
        return;
    }
    std::vector<uint32_t> kids;
    kids.reserve(static_cast<size_t>(p_child_slot_ids.size()));
    for (int i = 0; i < p_child_slot_ids.size(); ++i) {
        if (p_child_slot_ids[i] >= 0) {
            kids.push_back(static_cast<uint32_t>(p_child_slot_ids[i]));
        }
    }
    environment_.set_slot_children(static_cast<uint32_t>(p_parent_slot_id), kids);
}

void WFCSolverNode::add_directed_edge(
    int p_from_slot,
    int p_to_slot,
    int p_port_id,
    int p_axis,
    bool p_bidirectional
) {
    if (p_from_slot >= 0 && p_to_slot >= 0) {
        solver_core_.add_directed_edge(
            static_cast<uint32_t>(p_from_slot),
            static_cast<uint32_t>(p_to_slot),
            static_cast<uint32_t>(std::max(0, p_port_id)),
            static_cast<core::WFCConstraintAxis>(std::clamp(p_axis, 0, 5)),
            p_bidirectional
        );
    }
}

void WFCSolverNode::configure_environment_shape(int p_slot_count, const godot::Array &p_slot_hierarchy_dicts) {
    set_slot_count(p_slot_count);
    ensure_environment_prepared();
    for (int i = 0; i < p_slot_hierarchy_dicts.size(); ++i) {
        if (p_slot_hierarchy_dicts[i].get_type() != godot::Variant::DICTIONARY) {
            continue;
        }
        const godot::Dictionary d = p_slot_hierarchy_dicts[i];
        const int parent_slot = d.get("slot_id", i);
        if (d.has("children")) {
            const godot::PackedInt32Array kids = d["children"];
            set_slot_children(parent_slot, kids);
        }
        if (d.has("pinned_token")) {
            pin_landmark_by_name(parent_slot, d["pinned_token"]);
        } else if (d.has("pinned_state")) {
            pin_landmark_state(parent_slot, static_cast<int>(d["pinned_state"]));
        }
    }
}

void WFCSolverNode::pin_landmark_state(int p_slot_id, int p_state_id) {
    if (p_slot_id >= 0 && p_state_id >= 0) {
        solver_core_.add_pinned_landmark(static_cast<uint32_t>(p_slot_id), static_cast<uint32_t>(p_state_id));
    }
}

void WFCSolverNode::pin_landmark_by_name(int p_slot_id, const godot::String &p_token_name) {
    ensure_environment_prepared();
    const int32_t tok_id = environment_.find_token_by_name(p_token_name.utf8().get_data());
    if (p_slot_id >= 0 && tok_id >= 0) {
        solver_core_.add_pinned_landmark(static_cast<uint32_t>(p_slot_id), static_cast<uint32_t>(tok_id));
    }
}

void WFCSolverNode::clear_pinned_landmarks() {
    solver_core_.clear_pinned_landmarks();
}

godot::PackedInt32Array WFCSolverNode::run_wfc_collapse() {
    ensure_environment_prepared();
    solver_core_.solve_all();

    godot::PackedInt32Array out;
    out.resize(slot_count_);
    for (int i = 0; i < slot_count_; ++i) {
        out.set(i, solver_core_.collapsed_state_at(static_cast<uint32_t>(i)));
    }
    return out;
}

int WFCSolverNode::step_wfc_collapse() {
    if (solver_core_.status() == core::ProcGenSolverStatus::UNSOLVED) {
        ensure_environment_prepared();
    }
    return static_cast<int>(solver_core_.step_collapse());
}

godot::String WFCSolverNode::get_collapsed_token_name(int p_slot_id) const {
    const int32_t st = solver_core_.collapsed_state_at(static_cast<uint32_t>(std::max(0, p_slot_id)));
    if (st < 0) {
        return godot::String();
    }
    const std::string_view name = environment_.get_token_name(static_cast<uint32_t>(st));
    return godot::String::utf8(name.data(), static_cast<int>(name.size()));
}

float WFCSolverNode::get_slot_entropy(int p_slot_id) const {
    if (p_slot_id < 0) {
        return 0.0f;
    }
    return solver_core_.compute_slot_entropy(static_cast<uint32_t>(p_slot_id));
}

godot::PackedInt32Array WFCSolverNode::get_contradiction_slots() const {
    godot::PackedInt32Array out;
    for (const auto &rep : solver_core_.contradiction_reports()) {
        out.push_back(static_cast<int32_t>(rep.slot_id));
    }
    return out;
}

godot::Array WFCSolverNode::get_contradiction_reports_as_dicts() const {
    godot::Array out;
    for (const auto &rep : solver_core_.contradiction_reports()) {
        godot::Dictionary d;
        d["slot_id"]                    = static_cast<int>(rep.slot_id);
        d["parent_slot_id"]             = (rep.parent_slot_id == 0xFFFFFFFFu) ? -1 : static_cast<int>(rep.parent_slot_id);
        d["violated_rule_id"]           = (rep.violated_rule_id == 0xFFFFFFFFu) ? -1 : static_cast<int>(rep.violated_rule_id);
        d["contradiction_flags"]        = static_cast<int>(rep.contradiction_flags);
        d["deficit_or_excess_token_id"] = (rep.deficit_or_excess_token_id == 0xFFFFFFFFu) ? -1 : static_cast<int>(rep.deficit_or_excess_token_id);
        d["recommended_horizon_radius"] = static_cast<int>(rep.recommended_horizon_radius);
        d["local_einselection_score"]   = rep.local_einselection_score;
        d["failed_cascade_layer"]       = static_cast<int>(rep.failed_cascade_layer);
        out.push_back(d);
    }
    return out;
}

godot::Array WFCSolverNode::export_solved_configuration_summary() const {
    godot::Array out;
    const auto &slots = environment_.slots();
    const auto &child_csr = environment_.child_slot_csr();
    for (size_t i = 0; i < slots.size(); ++i) {
        godot::Dictionary d;
        const auto &s = slots[i];
        d["slot_id"]            = static_cast<int>(s.slot_id);
        d["parent_slot_id"]     = (s.parent_slot_id == 0xFFFFFFFFu) ? -1 : static_cast<int>(s.parent_slot_id);
        d["collapsed_token_id"] = s.collapsed_token_id;
        if (s.collapsed_token_id >= 0) {
            const std::string_view name = environment_.get_token_name(static_cast<uint32_t>(s.collapsed_token_id));
            d["token_name"] = godot::String::utf8(name.data(), static_cast<int>(name.size()));
        } else {
            d["token_name"] = godot::String();
        }
        d["einselection_score"] = s.einselection_score;
        d["is_pointer_state"]   = (s.is_pointer_state != 0u);
        d["has_conflict"]       = (s.has_conflict != 0u);
        d["entropy_bucket"]     = static_cast<int>(s.entropy_bucket);

        godot::PackedInt32Array children;
        for (uint32_t k = 0; k < s.children_count && (s.children_csr_offset + k) < child_csr.size(); ++k) {
            children.push_back(static_cast<int32_t>(child_csr[s.children_csr_offset + k]));
        }
        d["children"] = children;
        out.push_back(d);
    }
    return out;
}

// ============================================================================
// WFCRuleTensorInspector
// ============================================================================

void WFCRuleTensorInspector::_bind_methods() {}

bool WFCRuleTensorInspector::_can_handle(godot::Object *p_object) {
    if (p_object == nullptr) {
        return false;
    }
    return godot::Object::cast_to<WFCRuleTensorResource>(p_object) != nullptr ||
           godot::Object::cast_to<WFCSolverNode>(p_object) != nullptr;
}

} // namespace ideam::godot_ext
