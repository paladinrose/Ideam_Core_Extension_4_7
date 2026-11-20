#include "tutorial_resource.h"

namespace ideam::godot_ext {

void TutorialResource::_bind_methods() {
    using namespace godot;

    BIND_ENUM_CONSTANT(CONTEXT_EDITOR);
    BIND_ENUM_CONSTANT(CONTEXT_RUNTIME);
    BIND_ENUM_CONSTANT(CONTEXT_BOTH);

    ClassDB::bind_method(D_METHOD("set_tutorial_id", "id"), &TutorialResource::set_tutorial_id);
    ClassDB::bind_method(D_METHOD("get_tutorial_id"), &TutorialResource::get_tutorial_id);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "tutorial_id"), "set_tutorial_id", "get_tutorial_id");

    ClassDB::bind_method(D_METHOD("set_title", "title"), &TutorialResource::set_title);
    ClassDB::bind_method(D_METHOD("get_title"), &TutorialResource::get_title);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "title"), "set_title", "get_title");

    ClassDB::bind_method(D_METHOD("set_description", "description"), &TutorialResource::set_description);
    ClassDB::bind_method(D_METHOD("get_description"), &TutorialResource::get_description);
    ADD_PROPERTY(
        PropertyInfo(Variant::STRING, "description", PROPERTY_HINT_MULTILINE_TEXT),
        "set_description",
        "get_description"
    );

    ClassDB::bind_method(D_METHOD("set_steps", "steps"), &TutorialResource::set_steps);
    ClassDB::bind_method(D_METHOD("get_steps"), &TutorialResource::get_steps);
    ADD_PROPERTY(
        PropertyInfo(Variant::ARRAY, "steps", PROPERTY_HINT_ARRAY_TYPE, "TutorialStepResource"),
        "set_steps",
        "get_steps"
    );
    ClassDB::bind_method(D_METHOD("add_step", "step"), &TutorialResource::add_step);

    ClassDB::bind_method(
        D_METHOD("set_required_active_plugins", "plugins"),
        &TutorialResource::set_required_active_plugins
    );
    ClassDB::bind_method(D_METHOD("get_required_active_plugins"), &TutorialResource::get_required_active_plugins);
    ADD_PROPERTY(
        PropertyInfo(Variant::PACKED_STRING_ARRAY, "required_active_plugins"),
        "set_required_active_plugins",
        "get_required_active_plugins"
    );

    ClassDB::bind_method(D_METHOD("set_target_context", "context"), &TutorialResource::set_target_context);
    ClassDB::bind_method(D_METHOD("get_target_context"), &TutorialResource::get_target_context);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "target_context", PROPERTY_HINT_ENUM, "Editor:0,Runtime:1,Both:2"),
        "set_target_context",
        "get_target_context"
    );

    ClassDB::bind_method(
        D_METHOD("set_prerequisite_graph", "graph"),
        &TutorialResource::set_prerequisite_graph
    );
    ClassDB::bind_method(D_METHOD("get_prerequisite_graph"), &TutorialResource::get_prerequisite_graph);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "prerequisite_graph", PROPERTY_HINT_RESOURCE_TYPE, "IdeamGraphResource"),
        "set_prerequisite_graph",
        "get_prerequisite_graph"
    );

    ClassDB::bind_method(D_METHOD("set_help_topic_ids", "topics"), &TutorialResource::set_help_topic_ids);
    ClassDB::bind_method(D_METHOD("get_help_topic_ids"), &TutorialResource::get_help_topic_ids);
    ADD_PROPERTY(
        PropertyInfo(Variant::PACKED_STRING_ARRAY, "help_topic_ids"),
        "set_help_topic_ids",
        "get_help_topic_ids"
    );

    ClassDB::bind_method(D_METHOD("set_semantic_tags", "tags"), &TutorialResource::set_semantic_tags);
    ClassDB::bind_method(D_METHOD("get_semantic_tags"), &TutorialResource::get_semantic_tags);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "semantic_tags"), "set_semantic_tags", "get_semantic_tags");
}

void TutorialResource::add_step(const godot::Ref<TutorialStepResource> &p_step) {
    if (p_step.is_valid()) {
        steps_.push_back(p_step);
    }
}

} // namespace ideam::godot_ext
