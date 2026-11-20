#include "tutorial_step_resource.h"

namespace ideam::godot_ext {

void TutorialStepResource::_bind_methods() {
    using namespace godot;

    BIND_ENUM_CONSTANT(MANUAL_CHECKBOX);
    BIND_ENUM_CONSTANT(PROPERTY_EQUALS);
    BIND_ENUM_CONSTANT(SIGNAL_EMITTED);
    BIND_ENUM_CONSTANT(UNDO_REDO_ACTION_MATCH);
    BIND_ENUM_CONSTANT(TASK_GRAPH_VALIDATED);

    ClassDB::bind_method(D_METHOD("set_step_id", "id"), &TutorialStepResource::set_step_id);
    ClassDB::bind_method(D_METHOD("get_step_id"), &TutorialStepResource::get_step_id);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "step_id"), "set_step_id", "get_step_id");

    ClassDB::bind_method(D_METHOD("set_title", "title"), &TutorialStepResource::set_title);
    ClassDB::bind_method(D_METHOD("get_title"), &TutorialStepResource::get_title);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "title"), "set_title", "get_title");

    ClassDB::bind_method(D_METHOD("set_markdown_body", "body"), &TutorialStepResource::set_markdown_body);
    ClassDB::bind_method(D_METHOD("get_markdown_body"), &TutorialStepResource::get_markdown_body);
    ADD_PROPERTY(
        PropertyInfo(Variant::STRING, "markdown_body", PROPERTY_HINT_MULTILINE_TEXT),
        "set_markdown_body",
        "get_markdown_body"
    );

    ClassDB::bind_method(D_METHOD("set_completion_mode", "mode"), &TutorialStepResource::set_completion_mode);
    ClassDB::bind_method(D_METHOD("get_completion_mode"), &TutorialStepResource::get_completion_mode);
    ADD_PROPERTY(
        PropertyInfo(
            Variant::INT,
            "completion_mode",
            PROPERTY_HINT_ENUM,
            "Manual Checkbox:0,Property Equals:1,Signal Emitted:2,Undo/Redo Action Match:3,TaskGraph Validated:4"
        ),
        "set_completion_mode",
        "get_completion_mode"
    );

    ClassDB::bind_method(
        D_METHOD("set_target_ui_selector", "selector"),
        &TutorialStepResource::set_target_ui_selector
    );
    ClassDB::bind_method(D_METHOD("get_target_ui_selector"), &TutorialStepResource::get_target_ui_selector);
    ADD_PROPERTY(
        PropertyInfo(Variant::STRING, "target_ui_selector"),
        "set_target_ui_selector",
        "get_target_ui_selector"
    );

    ClassDB::bind_method(D_METHOD("set_target_property", "property"), &TutorialStepResource::set_target_property);
    ClassDB::bind_method(D_METHOD("get_target_property"), &TutorialStepResource::get_target_property);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "target_property"), "set_target_property", "get_target_property");

    ClassDB::bind_method(D_METHOD("set_expected_value", "value"), &TutorialStepResource::set_expected_value);
    ClassDB::bind_method(D_METHOD("get_expected_value"), &TutorialStepResource::get_expected_value);
    ADD_PROPERTY(
        PropertyInfo(Variant::NIL, "expected_value", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT),
        "set_expected_value",
        "get_expected_value"
    );

    ClassDB::bind_method(
        D_METHOD("set_expected_action_name", "action_name"),
        &TutorialStepResource::set_expected_action_name
    );
    ClassDB::bind_method(D_METHOD("get_expected_action_name"), &TutorialStepResource::get_expected_action_name);
    ADD_PROPERTY(
        PropertyInfo(Variant::STRING, "expected_action_name"),
        "set_expected_action_name",
        "get_expected_action_name"
    );

    ClassDB::bind_method(
        D_METHOD("set_auto_demonstrate_callable", "callable"),
        &TutorialStepResource::set_auto_demonstrate_callable
    );
    ClassDB::bind_method(
        D_METHOD("get_auto_demonstrate_callable"),
        &TutorialStepResource::get_auto_demonstrate_callable
    );

    ClassDB::bind_method(D_METHOD("set_semantic_tags", "tags"), &TutorialStepResource::set_semantic_tags);
    ClassDB::bind_method(D_METHOD("get_semantic_tags"), &TutorialStepResource::get_semantic_tags);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "semantic_tags"), "set_semantic_tags", "get_semantic_tags");
}

} // namespace ideam::godot_ext
