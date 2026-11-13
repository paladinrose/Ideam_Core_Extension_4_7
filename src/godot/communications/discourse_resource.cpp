#include "discourse_resource.h"

namespace ideam::godot_ext {

void UtteranceResource::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_raw_text", "raw_text"), &UtteranceResource::set_raw_text);
    ClassDB::bind_method(D_METHOD("get_raw_text"), &UtteranceResource::get_raw_text);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "raw_text"), "set_raw_text", "get_raw_text");

    ClassDB::bind_method(D_METHOD("set_prosody_spline", "prosody_spline"), &UtteranceResource::set_prosody_spline);
    ClassDB::bind_method(D_METHOD("get_prosody_spline"), &UtteranceResource::get_prosody_spline);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "prosody_spline", PROPERTY_HINT_RESOURCE_TYPE, "Spline2DResource"),
        "set_prosody_spline",
        "get_prosody_spline"
    );
}

void DiscourseResource::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_medium", "medium"), &DiscourseResource::set_medium);
    ClassDB::bind_method(D_METHOD("get_medium"), &DiscourseResource::get_medium);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "medium", PROPERTY_HINT_RESOURCE_TYPE, "CommunicationMediumResource"),
        "set_medium",
        "get_medium"
    );

    ClassDB::bind_method(D_METHOD("set_current_turn", "current_turn"), &DiscourseResource::set_current_turn);
    ClassDB::bind_method(D_METHOD("get_current_turn"), &DiscourseResource::get_current_turn);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "current_turn"), "set_current_turn", "get_current_turn");
}

} // namespace ideam::godot_ext
