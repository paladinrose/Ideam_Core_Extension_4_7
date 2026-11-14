#pragma once

/**
 * generation_rule.h
 * `GenerationRule` (`src/godot/procedural_generation/`).
 */

#include "../../core/procedural_generation/cascade_rules.h"
#include "../linguistics/sememe_resource.h"
#include "../utilities/ideam_undo_redo.h"
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace ideam::godot_ext {

class GenerationRule : public godot::Resource {
    GDCLASS(GenerationRule, godot::Resource)

private:
    godot::StringName target_class_;
    godot::StringName subject_token_name_;
    godot::String name_prefix_;
    godot::String name_suffix_;
    int operation_         = static_cast<int>(core::GenerationRuleOp::ADMIT_ARCHETYPE);
    int scope_             = static_cast<int>(core::PatternRuleScope::SELF);
    int cascade_layer_     = static_cast<int>(core::PatternCascadeLayer::LAYER_DOMAIN);
    int priority_          = 0;
    bool is_important_     = false;
    int min_occurrences_   = 0;
    int max_occurrences_   = 1;
    godot::PackedStringArray allowed_relatives_;
    godot::StringName required_sibling_;
    godot::StringName forbidden_sibling_;
    godot::Ref<SemanticTagSetResource> semantic_tags_;
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

    void set_name_prefix(const godot::String &p_prefix) { name_prefix_ = p_prefix; emit_changed(); }
    [[nodiscard]] godot::String get_name_prefix() const { return name_prefix_; }

    void set_name_suffix(const godot::String &p_suffix) { name_suffix_ = p_suffix; emit_changed(); }
    [[nodiscard]] godot::String get_name_suffix() const { return name_suffix_; }

    void set_operation(int p_op) { operation_ = std::clamp(p_op, 0, 6); emit_changed(); }
    [[nodiscard]] int get_operation() const noexcept { return operation_; }

    void set_scope(int p_scope) { scope_ = std::clamp(p_scope, 0, 4); emit_changed(); }
    [[nodiscard]] int get_scope() const noexcept { return scope_; }

    void set_cascade_layer(int p_layer) { cascade_layer_ = std::clamp(p_layer, 0, 6); emit_changed(); }
    [[nodiscard]] int get_cascade_layer() const noexcept { return cascade_layer_; }

    void set_priority(int p_prio) { priority_ = p_prio; emit_changed(); }
    [[nodiscard]] int get_priority() const noexcept { return priority_; }

    void set_is_important(bool p_imp) { is_important_ = p_imp; emit_changed(); }
    [[nodiscard]] bool get_is_important() const noexcept { return is_important_; }

    void set_min_occurrences(int p_min) { min_occurrences_ = std::max(0, p_min); emit_changed(); }
    [[nodiscard]] int get_min_occurrences() const noexcept { return min_occurrences_; }

    void set_max_occurrences(int p_max) { max_occurrences_ = std::max(0, p_max); emit_changed(); }
    [[nodiscard]] int get_max_occurrences() const noexcept { return max_occurrences_; }

    void set_allowed_relatives(const godot::PackedStringArray &p_rel) { allowed_relatives_ = p_rel; emit_changed(); }
    [[nodiscard]] godot::PackedStringArray get_allowed_relatives() const { return allowed_relatives_; }

    void set_required_sibling(const godot::StringName &p_req) { required_sibling_ = p_req; emit_changed(); }
    [[nodiscard]] godot::StringName get_required_sibling() const { return required_sibling_; }

    void set_forbidden_sibling(const godot::StringName &p_forb) { forbidden_sibling_ = p_forb; emit_changed(); }
    [[nodiscard]] godot::StringName get_forbidden_sibling() const { return forbidden_sibling_; }

    void set_semantic_tags(const godot::Ref<SemanticTagSetResource> &p_tags) { semantic_tags_ = p_tags; emit_changed(); }
    [[nodiscard]] godot::Ref<SemanticTagSetResource> get_semantic_tags() const { return semantic_tags_; }

    void set_child_rules(const godot::TypedArray<GenerationRule> &p_rules) { child_rules_ = p_rules; emit_changed(); }
    [[nodiscard]] godot::TypedArray<GenerationRule> get_child_rules() const { return child_rules_; }

    [[nodiscard]] core::GenerationRulePOD to_pod(uint32_t p_rule_id = 0, uint32_t p_parent_id = 0xFFFFFFFFu) const;
    [[nodiscard]] int compute_specificity() const noexcept;
};

} // namespace ideam::godot_ext
