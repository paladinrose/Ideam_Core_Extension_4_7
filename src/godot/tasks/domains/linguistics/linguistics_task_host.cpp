#include "linguistics_task_host.h"

namespace ideam::godot_ext {

void LinguisticsTaskHost::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_domain_resource", "resource"), &LinguisticsTaskHost::set_domain_resource);
    ClassDB::bind_method(D_METHOD("get_domain_resource"), &LinguisticsTaskHost::get_domain_resource);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "domain_resource", PROPERTY_HINT_RESOURCE_TYPE, "DataOrientedLinguistics"),
        "set_domain_resource",
        "get_domain_resource"
    );
}

} // namespace ideam::godot_ext
