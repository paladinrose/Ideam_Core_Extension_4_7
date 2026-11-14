#include "generation_rule.h"
#include <algorithm>

namespace ideam::godot_ext {

void GenerationRule::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_target_class", "target_class"), &GenerationRule::set_target_class);
    ClassDB::bind_method(D_METHOD("get_target_class"), &GenerationRule::get_target_class);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "target_class"), "set_target_class", "get_target_class");

    ClassDB::bind_method(D_METHOD("set_subject_token_name", "subject_token_name"), &GenerationRule::set_subject_token_name);
    ClassDB::bind_method(D_METHOD("get_subject_token_name"), &GenerationRule::get_subject_token_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "subject_token_name"), "set_subject_token_name", "get_subject_token_name");

    ClassDB::bind_method(D_METHOD("set_name_prefix", "name_prefix"), &GenerationRule::set_name_prefix);
    ClassDB::bind_method(D_METHOD("get_name_prefix"), &GenerationRule::get_name_prefix);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "name_prefix"), "set_name_prefix", "get_name_prefix");

    ClassDB::bind_method(D_METHOD("set_name_suffix", "name_suffix"), &GenerationRule::set_name_suffix);
    ClassDB::bind_method(D_METHOD("get_name_suffix"), &GenerationRule::get_name_suffix);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "name_suffix"), "set_name_suffix", "get_name_suffix");

    ClassDB::bind_method(D_METHOD("set_operation", "operation"), &GenerationRule::set_operation);
    ClassDB::bind_method(D_METHOD("get_operation"), &GenerationRule::get_operation);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "operation", PROPERTY_HINT_ENUM, "AdmitArchetype:0,AcceptParents:1,AcceptChildrenCardinality:2,RequireSibling:3,ForbidSibling:4,AllowSpatialNeighbor:5,ConstrainPropertyRange:6"),
        "set_operation",
        "get_operation"
    );

    ClassDB::bind_method(D_METHOD("set_scope", "scope"), &GenerationRule::set_scope);
    ClassDB::bind_method(D_METHOD("get_scope"), &GenerationRule::get_scope);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "scope", PROPERTY_HINT_ENUM, "Self:0,Children:1,Descendants:2,Siblings:3,All:4"),
        "set_scope",
        "get_scope"
    );

    ClassDB::bind_method(D_METHOD("set_cascade_layer", "layer"), &GenerationRule::set_cascade_layer);
    ClassDB::bind_method(D_METHOD("get_cascade_layer"), &GenerationRule::get_cascade_layer);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "cascade_layer", PROPERTY_HINT_ENUM, "Domain:0,HierarchyParent:1,SiblingDependency:2,ChildCardinality:3,SpatialSocket:4,VariantDetail:5,PinnedOverride:6"),
        "set_cascade_layer",
        "get_cascade_layer"
    );

    ClassDB::bind_method(D_METHOD("set_priority", "priority"), &GenerationRule::set_priority);
    ClassDB::bind_method(D_METHOD("get_priority"), &GenerationRule::get_priority);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "priority"), "set_priority", "get_priority");

    ClassDB::bind_method(D_METHOD("set_is_important", "is_important"), &GenerationRule::set_is_important);
    ClassDB::bind_method(D_METHOD("get_is_important"), &GenerationRule::get_is_important);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "is_important"), "set_is_important", "get_is_important");

    ClassDB::bind_method(D_METHOD("set_min_occurrences", "min_occurrences"), &GenerationRule::set_min_occurrences);
    ClassDB::bind_method(D_METHOD("get_min_occurrences"), &GenerationRule::get_min_occurrences);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "min_occurrences"), "set_min_occurrences", "get_min_occurrences");

    ClassDB::bind_method(D_METHOD("set_max_occurrences", "max_occurrences"), &GenerationRule::set_max_occurrences);
    ClassDB::bind_method(D_METHOD("get_max_occurrences"), &GenerationRule::get_max_occurrences);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_occurrences"), "set_max_occurrences", "get_max_occurrences");

    ClassDB::bind_method(D_METHOD("set_allowed_relatives", "allowed_relatives"), &GenerationRule::set_allowed_relatives);
    ClassDB::bind_method(D_METHOD("get_allowed_relatives"), &GenerationRule::get_allowed_relatives);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "allowed_relatives"), "set_allowed_relatives", "get_allowed_relatives");

    ClassDB::bind_method(D_METHOD("set_required_sibling", "required_sibling"), &GenerationRule::set_required_sibling);
    ClassDB::bind_method(D_METHOD("get_required_sibling"), &GenerationRule::get_required_sibling);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "required_sibling"), "set_required_sibling", "get_required_sibling");

    ClassDB::bind_method(D_METHOD("set_forbidden_sibling", "forbidden_sibling"), &GenerationRule::set_forbidden_sibling);
    ClassDB::bind_method(D_METHOD("get_forbidden_sibling"), &GenerationRule::get_forbidden_sibling);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "forbidden_sibling"), "set_forbidden_sibling", "get_forbidden_sibling");

    ClassDB::bind_method(D_METHOD("set_semantic_tags", "tags"), &GenerationRule::set_semantic_tags);
    ClassDB::bind_method(D_METHOD("get_semantic_tags"), &GenerationRule::get_semantic_tags);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "semantic_tags", PROPERTY_HINT_RESOURCE_TYPE, "SemanticTagSetResource"),
        "set_semantic_tags",
        "get_semantic_tags"
    );

    ClassDB::bind_method(D_METHOD("set_child_rules", "child_rules"), &GenerationRule::set_child_rules);
    ClassDB::bind_method(D_METHOD("get_child_rules"), &GenerationRule::get_child_rules);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "child_rules", PROPERTY_HINT_ARRAY_TYPE, "GenerationRule"), "set_child_rules", "get_child_rules");

    ClassDB::bind_method(D_METHOD("compute_specificity"), &GenerationRule::compute_specificity);
}

core::GenerationRulePOD GenerationRule::to_pod(uint32_t p_rule_id, uint32_t p_parent_id) const {
    core::GenerationRulePOD pod{};
    pod.rule_id         = p_rule_id;
    pod.parent_rule_id  = p_parent_id;
    pod.min_occurrences = static_cast<uint16_t>(std::clamp(min_occurrences_, 0, 65535));
    pod.max_occurrences = static_cast<uint16_t>(std::clamp(max_occurrences_, 0, 65535));
    pod.priority        = static_cast<int16_t>(std::clamp(priority_, -32768, 32767));
    pod.scope           = static_cast<core::PatternRuleScope>(std::clamp(scope_, 0, 4));
    pod.layer           = static_cast<core::PatternCascadeLayer>(std::clamp(cascade_layer_, 0, 6));
    pod.operation       = static_cast<core::GenerationRuleOp>(std::clamp(operation_, 0, 6));
    pod.is_important    = is_important_ ? 1u : 0u;

    const godot::String cls_str(target_class_);
    const godot::String subj_str(subject_token_name_);
    pod.selector.class_name_hash  = core::hash_reflection_symbol(cls_str.utf8().get_data());
    pod.selector.object_name_hash = core::hash_reflection_symbol(subj_str.utf8().get_data());
    pod.selector.name_prefix_hash = core::hash_reflection_symbol(name_prefix_.utf8().get_data());
    pod.selector.name_suffix_hash = core::hash_reflection_symbol(name_suffix_.utf8().get_data());

    if (semantic_tags_.is_valid()) {
        pod.selector.group_or_tag_mask = static_cast<uint32_t>(
            semantic_tags_->get_core_descriptor().nuclear_seme_words[0] & 0xFFFFFFFFULL
        );
    }

    pod.specificity = core::CascadeRuleEngine::compute_generation_specificity(pod);
    return pod;
}

int GenerationRule::compute_specificity() const noexcept {
    return static_cast<int>(to_pod().specificity);
}

} // namespace ideam::godot_ext
