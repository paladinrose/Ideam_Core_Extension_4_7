#include "procedural_generation_task_host.h"

namespace ideam::godot_ext {

void ProceduralGenerationTaskHost::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_domain_resource", "resource"), &ProceduralGenerationTaskHost::set_domain_resource);
    ClassDB::bind_method(D_METHOD("get_domain_resource"), &ProceduralGenerationTaskHost::get_domain_resource);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "domain_resource", PROPERTY_HINT_RESOURCE_TYPE, "DataOrientedProceduralGeneration"),
        "set_domain_resource",
        "get_domain_resource"
    );

    ClassDB::bind_method(D_METHOD("set_active_stage_mask", "stage_mask"), &ProceduralGenerationTaskHost::set_active_stage_mask);
    ClassDB::bind_method(D_METHOD("get_active_stage_mask"), &ProceduralGenerationTaskHost::get_active_stage_mask);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "active_stage_mask", PROPERTY_HINT_FLAGS, "Wave1_CascadeCompile:1,Wave2_ReflectionMatch:2,Wave3_DomainInit:4,Wave4_HierarchyPropagate:8"),
        "set_active_stage_mask",
        "get_active_stage_mask"
    );

    ClassDB::bind_method(D_METHOD("set_auto_stage_on_ready", "auto_stage"), &ProceduralGenerationTaskHost::set_auto_stage_on_ready);
    ClassDB::bind_method(D_METHOD("get_auto_stage_on_ready"), &ProceduralGenerationTaskHost::get_auto_stage_on_ready);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_stage_on_ready"), "set_auto_stage_on_ready", "get_auto_stage_on_ready");

    ClassDB::bind_method(D_METHOD("step_procedural_generation_pipeline"), &ProceduralGenerationTaskHost::step_procedural_generation_pipeline);

    ADD_SIGNAL(MethodInfo("procedural_generation_pipeline_executed", PropertyInfo(Variant::ARRAY, "slots")));
}

void ProceduralGenerationTaskHost::_ready() {
    if (auto_stage_on_ready_ && domain_resource_.is_valid()) {
        domain_resource_->stage_pattern_to_dod();
    }
}

bool ProceduralGenerationTaskHost::step_procedural_generation_pipeline() {
    if (domain_resource_.is_null()) return false;
    const bool ok = domain_resource_->execute_pipeline_direct(active_stage_mask_);
    if (ok) {
        emit_signal("procedural_generation_pipeline_executed", domain_resource_->read_environment_slots_as_dicts());
    }
    return ok;
}

} // namespace ideam::godot_ext
