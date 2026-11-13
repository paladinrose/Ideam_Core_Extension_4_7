#include "analysis_rule.h"

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

    ClassDB::bind_method(D_METHOD("set_child_rules", "child_rules"), &AnalysisRule::set_child_rules);
    ClassDB::bind_method(D_METHOD("get_child_rules"), &AnalysisRule::get_child_rules);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "child_rules", PROPERTY_HINT_ARRAY_TYPE, "AnalysisRule"), "set_child_rules", "get_child_rules");
}

void AnalysisStyle::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_rules", "rules"), &AnalysisStyle::set_rules);
    ClassDB::bind_method(D_METHOD("get_rules"), &AnalysisStyle::get_rules);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "rules", PROPERTY_HINT_ARRAY_TYPE, "AnalysisRule"), "set_rules", "get_rules");
}

} // namespace ideam::godot_ext
