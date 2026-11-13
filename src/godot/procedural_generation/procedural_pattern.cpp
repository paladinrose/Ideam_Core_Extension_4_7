#include "procedural_pattern.h"

namespace ideam::godot_ext {

void ProceduralPattern::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_generation_rules", "rules"), &ProceduralPattern::set_generation_rules);
    ClassDB::bind_method(D_METHOD("get_generation_rules"), &ProceduralPattern::get_generation_rules);
    ADD_PROPERTY(
        PropertyInfo(Variant::ARRAY, "generation_rules", PROPERTY_HINT_ARRAY_TYPE, "GenerationRule"),
        "set_generation_rules",
        "get_generation_rules"
    );

    ClassDB::bind_method(D_METHOD("add_generation_rule", "rule"), &ProceduralPattern::add_generation_rule);
    ClassDB::bind_method(D_METHOD("clear_generation_rules"), &ProceduralPattern::clear_generation_rules);
    ClassDB::bind_method(D_METHOD("get_generation_rule_count"), &ProceduralPattern::get_generation_rule_count);
}

void ProceduralPattern::set_generation_rules(const godot::TypedArray<GenerationRule> &p_rules) {
    generation_rules_ = p_rules;
    emit_changed();
}

void ProceduralPattern::add_generation_rule(const godot::Ref<GenerationRule> &p_rule) {
    if (p_rule.is_valid()) {
        generation_rules_.push_back(p_rule);
        emit_changed();
    }
}

void ProceduralPattern::clear_generation_rules() {
    generation_rules_.clear();
    emit_changed();
}

} // namespace ideam::godot_ext
