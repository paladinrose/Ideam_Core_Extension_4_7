#include "design_task_host.h"

namespace ideam::godot_ext {

void DesignTaskHost::_ready() {
    TaskGraphHost::_ready();
    if (auto_stage_on_ready_ && domain_resource_.is_valid()) {
        domain_resource_->initialize_dod_buffers();
        domain_resource_->stage_active_sources_to_dod();
    }
}

bool DesignTaskHost::step_design_pipeline() {
    if (domain_resource_.is_null()) {
        return false;
    }
    return domain_resource_->execute_pipeline_direct(active_stage_mask_);
}

void DesignTaskHost::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_domain_resource", "res"), &DesignTaskHost::set_domain_resource);
    ClassDB::bind_method(D_METHOD("get_domain_resource"), &DesignTaskHost::get_domain_resource);

    ClassDB::bind_method(D_METHOD("set_active_stage_mask", "mask"), &DesignTaskHost::set_active_stage_mask);
    ClassDB::bind_method(D_METHOD("get_active_stage_mask"), &DesignTaskHost::get_active_stage_mask);

    ClassDB::bind_method(D_METHOD("set_auto_stage_on_ready", "auto_stage"), &DesignTaskHost::set_auto_stage_on_ready);
    ClassDB::bind_method(D_METHOD("get_auto_stage_on_ready"), &DesignTaskHost::get_auto_stage_on_ready);

    ClassDB::bind_method(D_METHOD("step_design_pipeline"), &DesignTaskHost::step_design_pipeline);

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "domain_resource", PROPERTY_HINT_RESOURCE_TYPE, "DataOrientedDesign"), "set_domain_resource", "get_domain_resource");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "active_stage_mask"), "set_active_stage_mask", "get_active_stage_mask");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_stage_on_ready"), "set_auto_stage_on_ready", "get_auto_stage_on_ready");
}

} // namespace ideam::godot_ext
