#pragma once

/**
 * tutorial_step_resource.h
 * Data Layer resource (`godot::Resource`) representing a single guided learning step
 * (`src/godot/tutorials/resources/`).
 */

#include "../../../core/tutorials/tutorial_common.h"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace ideam::godot_ext {

enum TutorialCompletionMode : int32_t {
    MANUAL_CHECKBOX        = static_cast<int32_t>(core::tutorials::TutorialCompletionMode::MANUAL_CHECKBOX),
    PROPERTY_EQUALS        = static_cast<int32_t>(core::tutorials::TutorialCompletionMode::PROPERTY_EQUALS),
    SIGNAL_EMITTED         = static_cast<int32_t>(core::tutorials::TutorialCompletionMode::SIGNAL_EMITTED),
    UNDO_REDO_ACTION_MATCH = static_cast<int32_t>(core::tutorials::TutorialCompletionMode::UNDO_REDO_ACTION_MATCH),
    TASK_GRAPH_VALIDATED   = static_cast<int32_t>(core::tutorials::TutorialCompletionMode::TASK_GRAPH_VALIDATED)
};

class TutorialStepResource : public godot::Resource {
    GDCLASS(TutorialStepResource, godot::Resource)

private:
    godot::StringName step_id_;
    godot::String title_;
    godot::String markdown_body_;
    TutorialCompletionMode completion_mode_ = MANUAL_CHECKBOX;
    godot::String target_ui_selector_;
    godot::StringName target_property_;
    godot::Variant expected_value_;
    godot::String expected_action_name_;
    godot::Callable auto_demonstrate_callable_;
    godot::PackedStringArray semantic_tags_;

protected:
    static void _bind_methods();

public:
    TutorialStepResource() = default;
    ~TutorialStepResource() override = default;

    void set_step_id(const godot::StringName &p_id) { step_id_ = p_id; }
    [[nodiscard]] godot::StringName get_step_id() const { return step_id_; }

    void set_title(const godot::String &p_title) { title_ = p_title; }
    [[nodiscard]] godot::String get_title() const { return title_; }

    void set_markdown_body(const godot::String &p_body) { markdown_body_ = p_body; }
    [[nodiscard]] godot::String get_markdown_body() const { return markdown_body_; }

    void set_completion_mode(TutorialCompletionMode p_mode) { completion_mode_ = p_mode; }
    [[nodiscard]] TutorialCompletionMode get_completion_mode() const noexcept { return completion_mode_; }

    void set_target_ui_selector(const godot::String &p_selector) { target_ui_selector_ = p_selector; }
    [[nodiscard]] godot::String get_target_ui_selector() const { return target_ui_selector_; }

    void set_target_property(const godot::StringName &p_prop) { target_property_ = p_prop; }
    [[nodiscard]] godot::StringName get_target_property() const { return target_property_; }

    void set_expected_value(const godot::Variant &p_value) { expected_value_ = p_value; }
    [[nodiscard]] godot::Variant get_expected_value() const { return expected_value_; }

    void set_expected_action_name(const godot::String &p_action) { expected_action_name_ = p_action; }
    [[nodiscard]] godot::String get_expected_action_name() const { return expected_action_name_; }

    void set_auto_demonstrate_callable(const godot::Callable &p_cb) { auto_demonstrate_callable_ = p_cb; }
    [[nodiscard]] godot::Callable get_auto_demonstrate_callable() const { return auto_demonstrate_callable_; }

    void set_semantic_tags(const godot::PackedStringArray &p_tags) { semantic_tags_ = p_tags; }
    [[nodiscard]] godot::PackedStringArray get_semantic_tags() const { return semantic_tags_; }
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::TutorialCompletionMode);
