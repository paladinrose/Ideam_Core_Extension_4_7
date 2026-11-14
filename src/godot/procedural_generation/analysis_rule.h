#pragma once

/**
 * analysis_rule.h
 * `AnalysisRule` and `AnalysisStyle` (`src/godot/procedural_generation/`).
 */

#include "../../core/procedural_generation/cascade_rules.h"
#include "../utilities/ideam_undo_redo.h"
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <vector>

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
    int scope_                = static_cast<int>(core::PatternRuleScope::ALL);
    int cascade_layer_        = static_cast<int>(core::PatternCascadeLayer::LAYER_DOMAIN);
    int directives_mask_      = static_cast<int>(core::AnalysisDirectiveFlags::FOLLOW_SCENE_HIERARCHY) |
                                static_cast<int>(core::AnalysisDirectiveFlags::USE_NODE_NAMES_AS_TOKENS) |
                                static_cast<int>(core::AnalysisDirectiveFlags::EXTRACT_PARENT_ACCEPTANCE) |
                                static_cast<int>(core::AnalysisDirectiveFlags::EXTRACT_CHILD_CARDINALITY) |
                                static_cast<int>(core::AnalysisDirectiveFlags::EXTRACT_SIBLING_REQUIREMENTS);
    float property_range_min_ = 0.0f;
    float property_range_max_ = 1.0f;

    godot::TypedArray<godot::StringName> required_properties_;
    godot::TypedArray<godot::StringName> required_methods_;
    godot::TypedArray<godot::StringName> required_signals_;
    godot::TypedArray<AnalysisRule> child_rules_;
    godot::Ref<IdeamUndoRedo> undo_redo_;

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

    void set_priority(int p_prio) { priority_ = p_prio; emit_changed(); }
    [[nodiscard]] int get_priority() const noexcept { return priority_; }

    void set_is_important(bool p_imp) { is_important_ = p_imp; emit_changed(); }
    [[nodiscard]] bool get_is_important() const noexcept { return is_important_; }

    void set_scope(int p_scope) { scope_ = std::clamp(p_scope, 0, 4); emit_changed(); }
    [[nodiscard]] int get_scope() const noexcept { return scope_; }

    void set_cascade_layer(int p_layer) { cascade_layer_ = std::clamp(p_layer, 0, 6); emit_changed(); }
    [[nodiscard]] int get_cascade_layer() const noexcept { return cascade_layer_; }

    void set_directives_mask(int p_mask) { directives_mask_ = p_mask; emit_changed(); }
    [[nodiscard]] int get_directives_mask() const noexcept { return directives_mask_; }

    void set_property_range_min(float p_min) { property_range_min_ = p_min; emit_changed(); }
    [[nodiscard]] float get_property_range_min() const noexcept { return property_range_min_; }

    void set_property_range_max(float p_max) { property_range_max_ = p_max; emit_changed(); }
    [[nodiscard]] float get_property_range_max() const noexcept { return property_range_max_; }

    void set_required_properties(const godot::TypedArray<godot::StringName> &p_props) { required_properties_ = p_props; emit_changed(); }
    [[nodiscard]] godot::TypedArray<godot::StringName> get_required_properties() const { return required_properties_; }

    void set_required_methods(const godot::TypedArray<godot::StringName> &p_methods) { required_methods_ = p_methods; emit_changed(); }
    [[nodiscard]] godot::TypedArray<godot::StringName> get_required_methods() const { return required_methods_; }

    void set_required_signals(const godot::TypedArray<godot::StringName> &p_signals) { required_signals_ = p_signals; emit_changed(); }
    [[nodiscard]] godot::TypedArray<godot::StringName> get_required_signals() const { return required_signals_; }

    void set_child_rules(const godot::TypedArray<AnalysisRule> &p_rules) { child_rules_ = p_rules; emit_changed(); }
    [[nodiscard]] godot::TypedArray<AnalysisRule> get_child_rules() const { return child_rules_; }

    [[nodiscard]] core::AnalysisRulePOD to_pod(uint32_t p_rule_id = 0, uint32_t p_parent_id = 0xFFFFFFFFu) const;
    [[nodiscard]] int compute_specificity() const noexcept;
    [[nodiscard]] bool matches_godot_object(godot::Object *p_obj) const;

    void flatten_cascade(
        std::vector<core::AnalysisRulePOD> &r_out,
        const core::AnalysisRulePOD *p_parent_pod = nullptr,
        godot::String *r_effective_class = nullptr,
        godot::String *r_effective_suffix = nullptr
    ) const;
};

class AnalysisStyle : public godot::Resource {
    GDCLASS(AnalysisStyle, godot::Resource)

private:
    godot::TypedArray<AnalysisRule> rules_;
    godot::Ref<IdeamUndoRedo> undo_redo_;

protected:
    static void _bind_methods();

public:
    AnalysisStyle() = default;
    ~AnalysisStyle() override = default;

    void set_rules(const godot::TypedArray<AnalysisRule> &p_rules) { rules_ = p_rules; emit_changed(); }
    [[nodiscard]] godot::TypedArray<AnalysisRule> get_rules() const { return rules_; }

    void add_rule(const godot::Ref<AnalysisRule> &p_rule);
    void clear_rules();

    void compile_flattened_rules(
        std::vector<core::AnalysisRulePOD> &r_out,
        godot::String *r_effective_class = nullptr,
        godot::String *r_effective_suffix = nullptr
    ) const;
};

} // namespace ideam::godot_ext
