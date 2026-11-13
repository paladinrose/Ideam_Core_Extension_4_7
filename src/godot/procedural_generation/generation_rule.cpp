#include "generation_rule.h"

namespace ideam::godot_ext {

void GenerationRule::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_target_class", "target_class"), &GenerationRule::set_target_class);
    ClassDB::bind_method(D_METHOD("get_target_class"), &GenerationRule::get_target_class);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "target_class"), "set_target_class", "get_target_class");

    ClassDB::bind_method(D_METHOD("set_subject_token_name", "subject_token_name"), &GenerationRule::set_subject_token_name);
    ClassDB::bind_method(D_METHOD("get_subject_token_name"), &GenerationRule::get_subject_token_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "subject_token_name"), "set_subject_token_name", "get_subject_token_name");

    ClassDB::bind_method(D_METHOD("set_name_suffix", "name_suffix"), &GenerationRule::set_name_suffix);
    ClassDB::bind_method(D_METHOD("get_name_suffix"), &GenerationRule::get_name_suffix);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "name_suffix"), "set_name_suffix", "get_name_suffix");

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

    ClassDB::bind_method(D_METHOD("set_child_rules", "child_rules"), &GenerationRule::set_child_rules);
    ClassDB::bind_method(D_METHOD("get_child_rules"), &GenerationRule::get_child_rules);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "child_rules", PROPERTY_HINT_ARRAY_TYPE, "GenerationRule"), "set_child_rules", "get_child_rules");
}

} // namespace ideam::godot_ext
