#include "communication_medium_resource.h"

namespace ideam::godot_ext {

void CommunicationMediumResource::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_ambient_noise_db", "noise_db"), &CommunicationMediumResource::set_ambient_noise_db);
    ClassDB::bind_method(D_METHOD("get_ambient_noise_db"), &CommunicationMediumResource::get_ambient_noise_db);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "ambient_noise_db"), "set_ambient_noise_db", "get_ambient_noise_db");

    ClassDB::bind_method(D_METHOD("set_attenuation_db_per_m", "attenuation"), &CommunicationMediumResource::set_attenuation_db_per_m);
    ClassDB::bind_method(D_METHOD("get_attenuation_db_per_m"), &CommunicationMediumResource::get_attenuation_db_per_m);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "attenuation_db_per_m"), "set_attenuation_db_per_m", "get_attenuation_db_per_m");

    ClassDB::bind_method(D_METHOD("set_occlusion_damping_db", "occlusion_db"), &CommunicationMediumResource::set_occlusion_damping_db);
    ClassDB::bind_method(D_METHOD("get_occlusion_damping_db"), &CommunicationMediumResource::get_occlusion_damping_db);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "occlusion_damping_db"), "set_occlusion_damping_db", "get_occlusion_damping_db");
}

} // namespace ideam::godot_ext
