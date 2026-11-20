#include "cymatics_task_host.h"

namespace ideam::godot_ext {

void CymaticsTaskHost::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_domain_resource", "res"), &CymaticsTaskHost::set_domain_resource);
    ClassDB::bind_method(D_METHOD("get_domain_resource"), &CymaticsTaskHost::get_domain_resource);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "domain_resource", PROPERTY_HINT_RESOURCE_TYPE, "DataOrientedCymatics"),
        "set_domain_resource",
        "get_domain_resource"
    );

    ClassDB::bind_method(D_METHOD("set_active_stage_mask", "mask"), &CymaticsTaskHost::set_active_stage_mask);
    ClassDB::bind_method(D_METHOD("get_active_stage_mask"), &CymaticsTaskHost::get_active_stage_mask);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "active_stage_mask"), "set_active_stage_mask", "get_active_stage_mask");

    ClassDB::bind_method(D_METHOD("set_auto_stage_on_ready", "auto"), &CymaticsTaskHost::set_auto_stage_on_ready);
    ClassDB::bind_method(D_METHOD("get_auto_stage_on_ready"), &CymaticsTaskHost::get_auto_stage_on_ready);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_stage_on_ready"), "set_auto_stage_on_ready", "get_auto_stage_on_ready");

    ClassDB::bind_method(D_METHOD("step_cymatics_pipeline"), &CymaticsTaskHost::step_cymatics_pipeline);
}

void CymaticsTaskHost::_ready() {
    TaskGraphHost::_ready();
    if (auto_stage_on_ready_ && domain_resource_.is_valid()) {
        step_cymatics_pipeline();
    }
}

bool CymaticsTaskHost::step_cymatics_pipeline() {
    if (domain_resource_.is_null()) {
        return false;
    }
    return domain_resource_->execute_pipeline_direct(active_stage_mask_);
}

} // namespace ideam::godot_ext
