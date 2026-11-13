#include "wfc_task_host.h"

namespace ideam::godot_ext {

void WaveFunctionCollapseTaskHost::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_domain_resource", "resource"), &WaveFunctionCollapseTaskHost::set_domain_resource);
    ClassDB::bind_method(D_METHOD("get_domain_resource"), &WaveFunctionCollapseTaskHost::get_domain_resource);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "domain_resource", PROPERTY_HINT_RESOURCE_TYPE, "DataOrientedWaveFunctionCollapse"),
        "set_domain_resource",
        "get_domain_resource"
    );
}

} // namespace ideam::godot_ext
