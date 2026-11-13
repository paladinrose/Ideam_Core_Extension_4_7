#include "sememe_resource.h"

namespace ideam::godot_ext {

void SememeResource::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_unique_id", "unique_id"), &SememeResource::set_unique_id);
    ClassDB::bind_method(D_METHOD("get_unique_id"), &SememeResource::get_unique_id);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "unique_id"), "set_unique_id", "get_unique_id");

    ClassDB::bind_method(D_METHOD("set_canonical_label", "label"), &SememeResource::set_canonical_label);
    ClassDB::bind_method(D_METHOD("get_canonical_label"), &SememeResource::get_canonical_label);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "canonical_label"), "set_canonical_label", "get_canonical_label");

    ClassDB::bind_method(D_METHOD("set_nuclear_seme_tags", "tags"), &SememeResource::set_nuclear_seme_tags);
    ClassDB::bind_method(D_METHOD("get_nuclear_seme_tags"), &SememeResource::get_nuclear_seme_tags);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "nuclear_seme_tags"), "set_nuclear_seme_tags", "get_nuclear_seme_tags");

    ClassDB::bind_method(D_METHOD("set_peripheral_seme_tags", "tags"), &SememeResource::set_peripheral_seme_tags);
    ClassDB::bind_method(D_METHOD("get_peripheral_seme_tags"), &SememeResource::get_peripheral_seme_tags);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "peripheral_seme_tags"), "set_peripheral_seme_tags", "get_peripheral_seme_tags");
}

void SemanticTagSetResource::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_sememes", "sememes"), &SemanticTagSetResource::set_sememes);
    ClassDB::bind_method(D_METHOD("get_sememes"), &SemanticTagSetResource::get_sememes);
    ADD_PROPERTY(
        PropertyInfo(Variant::ARRAY, "sememes", PROPERTY_HINT_ARRAY_TYPE, "SememeResource"),
        "set_sememes",
        "get_sememes"
    );
}

} // namespace ideam::godot_ext
