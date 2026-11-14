#include "analysis_rule.h"
#include <godot_cpp/classes/node.hpp>

namespace ideam::godot_ext {

void AnalysisRule::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_target_class", "target_class"), &AnalysisRule::set_target_class);
    ClassDB::bind_method(D_METHOD("get_target_class"), &AnalysisRule::get_target_class);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "target_class"), "set_target_class", "get_target_class");

    ClassDB::bind_method(D_METHOD("set_name_prefix", "name_prefix"), &AnalysisRule::set_name_prefix);
    ClassDB::bind_method(D_METHOD("get_name_prefix"), &AnalysisRule::get_name_prefix);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "name_prefix"), "set_name_prefix", "get_name_prefix");

    ClassDB::bind_method(D_METHOD("set_name_suffix", "name_suffix"), &AnalysisRule::set_name_suffix);
    ClassDB::bind_method(D_METHOD("get_name_suffix"), &AnalysisRule::get_name_suffix);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "name_suffix"), "set_name_suffix", "get_name_suffix");

    ClassDB::bind_method(D_METHOD("set_ignore_non_matching", "ignore"), &AnalysisRule::set_ignore_non_matching);
    ClassDB::bind_method(D_METHOD("get_ignore_non_matching"), &AnalysisRule::get_ignore_non_matching);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "ignore_non_matching"), "set_ignore_non_matching", "get_ignore_non_matching");

    ClassDB::bind_method(D_METHOD("set_priority", "priority"), &AnalysisRule::set_priority);
    ClassDB::bind_method(D_METHOD("get_priority"), &AnalysisRule::get_priority);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "priority"), "set_priority", "get_priority");

    ClassDB::bind_method(D_METHOD("set_is_important", "is_important"), &AnalysisRule::set_is_important);
    ClassDB::bind_method(D_METHOD("get_is_important"), &AnalysisRule::get_is_important);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "is_important"), "set_is_important", "get_is_important");

    ClassDB::bind_method(D_METHOD("set_scope", "scope"), &AnalysisRule::set_scope);
    ClassDB::bind_method(D_METHOD("get_scope"), &AnalysisRule::get_scope);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "scope", PROPERTY_HINT_ENUM, "Self:0,Children:1,Descendants:2,Siblings:3,All:4"),
        "set_scope",
        "get_scope"
    );

    ClassDB::bind_method(D_METHOD("set_cascade_layer", "layer"), &AnalysisRule::set_cascade_layer);
    ClassDB::bind_method(D_METHOD("get_cascade_layer"), &AnalysisRule::get_cascade_layer);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "cascade_layer", PROPERTY_HINT_ENUM, "Domain:0,HierarchyParent:1,SiblingDependency:2,ChildCardinality:3,SpatialSocket:4,VariantDetail:5,PinnedOverride:6"),
        "set_cascade_layer",
        "get_cascade_layer"
    );

    ClassDB::bind_method(D_METHOD("set_directives_mask", "mask"), &AnalysisRule::set_directives_mask);
    ClassDB::bind_method(D_METHOD("get_directives_mask"), &AnalysisRule::get_directives_mask);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "directives_mask", PROPERTY_HINT_FLAGS, "FollowSceneHierarchy:1,UseNodeNamesAsTokens:2,ExtractParentAcceptance:4,ExtractChildCardinality:8,ExtractSiblingRequirements:16,ExtractSpatialOffsets:32,ExtractPropertyRanges:64,IgnoreNonMatchingNodes:128"), "set_directives_mask", "get_directives_mask");

    ClassDB::bind_method(D_METHOD("set_property_range_min", "min_val"), &AnalysisRule::set_property_range_min);
    ClassDB::bind_method(D_METHOD("get_property_range_min"), &AnalysisRule::get_property_range_min);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "property_range_min"), "set_property_range_min", "get_property_range_min");

    ClassDB::bind_method(D_METHOD("set_property_range_max", "max_val"), &AnalysisRule::set_property_range_max);
    ClassDB::bind_method(D_METHOD("get_property_range_max"), &AnalysisRule::get_property_range_max);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "property_range_max"), "set_property_range_max", "get_property_range_max");

    ClassDB::bind_method(D_METHOD("set_required_properties", "properties"), &AnalysisRule::set_required_properties);
    ClassDB::bind_method(D_METHOD("get_required_properties"), &AnalysisRule::get_required_properties);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "required_properties"), "set_required_properties", "get_required_properties");

    ClassDB::bind_method(D_METHOD("set_required_methods", "methods"), &AnalysisRule::set_required_methods);
    ClassDB::bind_method(D_METHOD("get_required_methods"), &AnalysisRule::get_required_methods);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "required_methods"), "set_required_methods", "get_required_methods");

    ClassDB::bind_method(D_METHOD("set_required_signals", "signals"), &AnalysisRule::set_required_signals);
    ClassDB::bind_method(D_METHOD("get_required_signals"), &AnalysisRule::get_required_signals);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "required_signals"), "set_required_signals", "get_required_signals");

    ClassDB::bind_method(D_METHOD("set_child_rules", "child_rules"), &AnalysisRule::set_child_rules);
    ClassDB::bind_method(D_METHOD("get_child_rules"), &AnalysisRule::get_child_rules);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "child_rules", PROPERTY_HINT_ARRAY_TYPE, "AnalysisRule"), "set_child_rules", "get_child_rules");

    ClassDB::bind_method(D_METHOD("compute_specificity"), &AnalysisRule::compute_specificity);
    ClassDB::bind_method(D_METHOD("matches_godot_object", "object"), &AnalysisRule::matches_godot_object);
}

core::AnalysisRulePOD AnalysisRule::to_pod(uint32_t p_rule_id, uint32_t p_parent_id) const {
    core::AnalysisRulePOD pod{};
    pod.rule_id            = p_rule_id;
    pod.parent_rule_id     = p_parent_id;
    pod.directives_mask    = static_cast<uint32_t>(directives_mask_);
    pod.priority           = priority_;
    pod.property_range_min = property_range_min_;
    pod.property_range_max = property_range_max_;
    pod.scope              = static_cast<core::PatternRuleScope>(std::clamp(scope_, 0, 4));
    pod.layer              = static_cast<core::PatternCascadeLayer>(std::clamp(cascade_layer_, 0, 6));
    pod.is_important       = is_important_ ? 1u : 0u;
    pod.ignore_unmatched   = ignore_non_matching_ ? 1u : 0u;

    const godot::String cls_str(target_class_);
    pod.selector.class_name_hash  = core::hash_reflection_symbol(cls_str.utf8().get_data());
    pod.selector.name_prefix_hash = core::hash_reflection_symbol(name_prefix_.utf8().get_data());
    pod.selector.name_suffix_hash = core::hash_reflection_symbol(name_suffix_.utf8().get_data());

    for (int i = 0; i < required_properties_.size() && i < 32; ++i) {
        pod.selector.property_trait_mask |= (1u << static_cast<uint32_t>(i));
    }
    for (int i = 0; i < required_methods_.size() && i < 32; ++i) {
        pod.selector.method_trait_mask |= (1u << static_cast<uint32_t>(i));
    }
    for (int i = 0; i < required_signals_.size() && i < 32; ++i) {
        pod.selector.signal_trait_mask |= (1u << static_cast<uint32_t>(i));
    }

    pod.specificity = core::CascadeRuleEngine::compute_analysis_specificity(pod);
    return pod;
}

int AnalysisRule::compute_specificity() const noexcept {
    return static_cast<int>(to_pod().specificity);
}

bool AnalysisRule::matches_godot_object(godot::Object *p_obj) const {
    if (!p_obj) return false;
    if (!target_class_.is_empty()) {
        const godot::StringName obj_cls = p_obj->get_class();
        if (obj_cls != target_class_ && !godot::ClassDB::is_parent_class(obj_cls, target_class_)) {
            return false;
        }
    }
    if (auto *node = godot::Object::cast_to<godot::Node>(p_obj)) {
        const godot::String node_name(node->get_name());
        if (!name_prefix_.is_empty() && !node_name.begins_with(name_prefix_)) return false;
        if (!name_suffix_.is_empty() && !node_name.ends_with(name_suffix_)) return false;
    }
    for (int i = 0; i < required_methods_.size(); ++i) {
        if (!p_obj->has_method(required_methods_[i])) return false;
    }
    for (int i = 0; i < required_signals_.size(); ++i) {
        if (!p_obj->has_signal(required_signals_[i])) return false;
    }
    return true;
}

void AnalysisRule::flatten_cascade(
    std::vector<core::AnalysisRulePOD> &r_out,
    const core::AnalysisRulePOD *p_parent_pod,
    godot::String *r_effective_class,
    godot::String *r_effective_suffix
) const {
    const uint32_t my_id     = static_cast<uint32_t>(r_out.size());
    const uint32_t parent_id = p_parent_pod ? p_parent_pod->rule_id : 0xFFFFFFFFu;
    core::AnalysisRulePOD my_pod = to_pod(my_id, parent_id);
    if (p_parent_pod) {
        core::CascadeRuleEngine::narrow_child_analysis_rule(*p_parent_pod, my_pod);
    }
    r_out.push_back(my_pod);

    if (r_effective_class && !target_class_.is_empty()) {
        *r_effective_class = godot::String(target_class_);
    }
    if (r_effective_suffix && !name_suffix_.is_empty()) {
        *r_effective_suffix = name_suffix_;
    }

    for (int i = 0; i < child_rules_.size(); ++i) {
        const godot::Ref<AnalysisRule> child = child_rules_[i];
        if (child.is_valid()) {
            child->flatten_cascade(r_out, &my_pod, r_effective_class, r_effective_suffix);
        }
    }
}

void AnalysisStyle::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_rules", "rules"), &AnalysisStyle::set_rules);
    ClassDB::bind_method(D_METHOD("get_rules"), &AnalysisStyle::get_rules);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "rules", PROPERTY_HINT_ARRAY_TYPE, "AnalysisRule"), "set_rules", "get_rules");

    ClassDB::bind_method(D_METHOD("add_rule", "rule"), &AnalysisStyle::add_rule);
    ClassDB::bind_method(D_METHOD("clear_rules"), &AnalysisStyle::clear_rules);
}

void AnalysisStyle::add_rule(const godot::Ref<AnalysisRule> &p_rule) {
    if (p_rule.is_valid()) {
        rules_.push_back(p_rule);
        emit_changed();
    }
}

void AnalysisStyle::clear_rules() {
    rules_.clear();
    emit_changed();
}

void AnalysisStyle::compile_flattened_rules(
    std::vector<core::AnalysisRulePOD> &r_out,
    godot::String *r_effective_class,
    godot::String *r_effective_suffix
) const {
    r_out.clear();
    for (int i = 0; i < rules_.size(); ++i) {
        const godot::Ref<AnalysisRule> r = rules_[i];
        if (r.is_valid()) {
            r->flatten_cascade(r_out, nullptr, r_effective_class, r_effective_suffix);
        }
    }
    core::CascadeRuleEngine::sort_analysis_rules_by_cascade(r_out);
}

} // namespace ideam::godot_ext
