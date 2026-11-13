#pragma once

/**
 * generation_rule.h
 * `GenerationRule` (`src/godot/procedural_generation/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES
 * ============================================================================
 * Represents a single CSS-style cascading generation law inside a `ProceduralPattern`.
 *
 * Example Rubric (`CastleTower` Scene Hierarchy):
 *   - Root Rule: `target_class = "Node3D"`, `name_suffix = "_cell"`
 *     ("Consider only Node3D's with the suffix '_cell'")
 *   - Cascading Rule 1 (`ACCEPT_PARENTS`):
 *     "If a cell is named 'CastleTower', then it will accept parents named
 *      'CastleGrounds' or 'CastleRuins'."
 *   - Cascading Sibling Rule 2 (`ACCEPT_CHILDREN_CARDINALITY`):
 *     "If a cell is named 'CastleTower', then it will accept children cells:
 *      'TowerGate' (min 1, max 2), 'TowerStairs' (max 1), and
 *      'TowerLanding' (max 3; requires 'TowerStairs' be present)."
 *
 * During Quantum Darwinism conflict stabilization, the cascade hierarchy and
 * specificity of each `GenerationRule` determine which child/sibling properties
 * can be relaxed without breaking the cell's core structural coherence.
 */

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include "../../core/procedural_generation/cascade_rules.h"

namespace ideam::godot_ext {

class GenerationRule : public godot::Resource {
    GDCLASS(GenerationRule, godot::Resource)

private:
    godot::StringName target_class_;
    godot::StringName subject_token_name_;
    godot::String name_suffix_;
    int operation_        = static_cast<int>(core::GenerationRuleOp::ADMIT_ARCHETYPE);
    int cascade_layer_    = static_cast<int>(core::PatternCascadeLayer::LAYER_DOMAIN);
    int min_occurrences_  = 0;
    int max_occurrences_  = 1;
    godot::PackedStringArray allowed_relatives_;
    godot::StringName required_sibling_;
    godot::StringName forbidden_sibling_;
    godot::TypedArray<GenerationRule> child_rules_;

protected:
    static void _bind_methods();

public:
    GenerationRule() = default;
    ~GenerationRule() override = default;

    void set_target_class(const godot::StringName &p_class) { target_class_ = p_class; emit_changed(); }
    [[nodiscard]] godot::StringName get_target_class() const { return target_class_; }

    void set_subject_token_name(const godot::StringName &p_name) { subject_token_name_ = p_name; emit_changed(); }
    [[nodiscard]] godot::StringName get_subject_token_name() const { return subject_token_name_; }

    void set_name_suffix(const godot::String &p_suffix) { name_suffix_ = p_suffix; emit_changed(); }
    [[nodiscard]] godot::String get_name_suffix() const { return name_suffix_; }

    void set_min_occurrences(int p_min) { min_occurrences_ = p_min; emit_changed(); }
    [[nodiscard]] int get_min_occurrences() const { return min_occurrences_; }

    void set_max_occurrences(int p_max) { max_occurrences_ = p_max; emit_changed(); }
    [[nodiscard]] int get_max_occurrences() const { return max_occurrences_; }

    void set_allowed_relatives(const godot::PackedStringArray &p_rel) { allowed_relatives_ = p_rel; emit_changed(); }
    [[nodiscard]] godot::PackedStringArray get_allowed_relatives() const { return allowed_relatives_; }

    void set_required_sibling(const godot::StringName &p_req) { required_sibling_ = p_req; emit_changed(); }
    [[nodiscard]] godot::StringName get_required_sibling() const { return required_sibling_; }

    void set_child_rules(const godot::TypedArray<GenerationRule> &p_rules) { child_rules_ = p_rules; emit_changed(); }
    [[nodiscard]] godot::TypedArray<GenerationRule> get_child_rules() const { return child_rules_; }
};

} // namespace ideam::godot_ext
