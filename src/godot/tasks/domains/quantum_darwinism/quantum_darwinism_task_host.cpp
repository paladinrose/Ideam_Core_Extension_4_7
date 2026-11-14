#include "quantum_darwinism_task_host.h"

namespace ideam::godot_ext {

void QuantumDarwinismTaskHost::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_domain_resource", "resource"), &QuantumDarwinismTaskHost::set_domain_resource);
    ClassDB::bind_method(D_METHOD("get_domain_resource"), &QuantumDarwinismTaskHost::get_domain_resource);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "domain_resource", PROPERTY_HINT_RESOURCE_TYPE, "DataOrientedQuantumDarwinism"),
        "set_domain_resource",
        "get_domain_resource"
    );

    ClassDB::bind_method(D_METHOD("set_active_stage_mask", "mask"), &QuantumDarwinismTaskHost::set_active_stage_mask);
    ClassDB::bind_method(D_METHOD("get_active_stage_mask"), &QuantumDarwinismTaskHost::get_active_stage_mask);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "active_stage_mask"),
        "set_active_stage_mask",
        "get_active_stage_mask"
    );

    ClassDB::bind_method(
        D_METHOD("set_auto_stage_on_ready", "auto_stage"),
        &QuantumDarwinismTaskHost::set_auto_stage_on_ready
    );
    ClassDB::bind_method(
        D_METHOD("get_auto_stage_on_ready"),
        &QuantumDarwinismTaskHost::get_auto_stage_on_ready
    );
    ADD_PROPERTY(
        PropertyInfo(Variant::BOOL, "auto_stage_on_ready"),
        "set_auto_stage_on_ready",
        "get_auto_stage_on_ready"
    );

    ClassDB::bind_method(
        D_METHOD("step_quantum_darwinism_pipeline"),
        &QuantumDarwinismTaskHost::step_quantum_darwinism_pipeline
    );
}

void QuantumDarwinismTaskHost::_ready() {
    if (auto_stage_on_ready_ && domain_resource_.is_valid()) {
        domain_resource_->initialize_dod_buffers();
        domain_resource_->stage_environment_to_dod();
    }
}

bool QuantumDarwinismTaskHost::step_quantum_darwinism_pipeline() {
    if (domain_resource_.is_null()) {
        return false;
    }
    return domain_resource_->execute_pipeline_direct(active_stage_mask_);
}

} // namespace ideam::godot_ext
