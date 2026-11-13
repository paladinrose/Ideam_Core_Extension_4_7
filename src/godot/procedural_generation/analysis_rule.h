#pragma once

/**
 * analysis_rule.h
 * `AnalysisRule` and `AnalysisStyle` (`src/godot/procedural_generation/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES
 * ============================================================================
 * 1. ROLE:
 *    - Defines the CSS-like cascading rules that govern how `ProceduralPatterner` inspects
 *      a source Godot pattern (`PackedScene` via `SceneState`, live `Node` tree, `Resource`,
 *      or `RefCounted`) to synthesize `GenerationRule`s inside a `ProceduralPattern`.
 *
 * 2. CASCADING SELECTORS BOUNDED BY GODOT REFLECTION:
 *    - `target_class`: Matches via `ClassDB::is_parent_class` (e.g., `"Node3D"`).
 *    - `name_prefix` / `name_suffix`: Pattern match on object/node name (e.g., `"_cell"`).
 *    - `required_properties`, `required_methods`, `required_signals`: Duck-typed reflection
 *      selectors (paralleling `PartSignature` in `GameplayRule`).
 *    - `property_value_constraints`: Dictionary/array of property value or `[min, max]` range checks.
 *    - `child_rules`: Cascading child `AnalysisRule` resources that refine parent rules
 *      (e.g., Rule 1: "Follow scene hierarchy using Node names" -> Child Rule 1.1:
 *      "...but only where object is `Node3D` with suffix `_cell`; ignore others").
 */

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include "../../core/procedural_generation/cascade_rules.h"

namespace ideam::godot_ext {

class AnalysisRule : public godot::Resource {
    GDCLASS(AnalysisRule, godot::Resource)

private:
    godot::StringName target_class_;
    godot::String name_prefix_;
    godot::String name_suffix_;
    bool ignore_non_matching_ = false;
    int priority_             = 0;
    bool is_important_        = false;
    uint32_t directives_mask_ = static_cast<uint32_t>(core::AnalysisDirectiveFlags::FOLLOW_SCENE_HIERARCHY) |
                                static_cast<uint32_t>(core::AnalysisDirectiveFlags::USE_NODE_NAMES_AS_TOKENS);
    godot::TypedArray<godot::StringName> required_properties_;
    godot::TypedArray<godot::StringName> required_methods_;
    godot::TypedArray<godot::StringName> required_signals_;
    godot::TypedArray<AnalysisRule> child_rules_;

protected:
    static void _bind_methods();

public:
    AnalysisRule() = default;
    ~AnalysisRule() override = default;

    void set_target_class(const godot::StringName &p_class) { target_class_ = p_class; emit_changed(); }
    [[nodiscard]] godot::StringName get_target_class() const { return target_class_; }

    void set_name_prefix(const godot::String &p_prefix) { name_prefix_ = p_prefix; emit_changed(); }
    [[nodiscard]] godot::String get_name_prefix() const { return name_prefix_; }

    void set_name_suffix(const godot::String &p_suffix) { name_suffix_ = p_suffix; emit_changed(); }
    [[nodiscard]] godot::String get_name_suffix() const { return name_suffix_; }

    void set_ignore_non_matching(bool p_ignore) { ignore_non_matching_ = p_ignore; emit_changed(); }
    [[nodiscard]] bool get_ignore_non_matching() const { return ignore_non_matching_; }

    void set_child_rules(const godot::TypedArray<AnalysisRule> &p_rules) { child_rules_ = p_rules; emit_changed(); }
    [[nodiscard]] godot::TypedArray<AnalysisRule> get_child_rules() const { return child_rules_; }
};

class AnalysisStyle : public godot::Resource {
    GDCLASS(AnalysisStyle, godot::Resource)

private:
    godot::TypedArray<AnalysisRule> rules_;

protected:
    static void _bind_methods();

public:
    AnalysisStyle() = default;
    ~AnalysisStyle() override = default;

    void set_rules(const godot::TypedArray<AnalysisRule> &p_rules) { rules_ = p_rules; emit_changed(); }
    [[nodiscard]] godot::TypedArray<AnalysisRule> get_rules() const { return rules_; }
};

} // namespace ideam::godot_ext
