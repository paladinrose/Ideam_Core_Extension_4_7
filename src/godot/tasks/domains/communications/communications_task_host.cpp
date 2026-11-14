#include "communications_task_host.h"

namespace ideam::godot_ext {

void CommunicationsTaskHost::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_domain_resource", "resource"), &CommunicationsTaskHost::set_domain_resource);
    ClassDB::bind_method(D_METHOD("get_domain_resource"), &CommunicationsTaskHost::get_domain_resource);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "domain_resource", PROPERTY_HINT_RESOURCE_TYPE, "DataOrientedCommunications"),
        "set_domain_resource",
        "get_domain_resource"
    );

    ClassDB::bind_method(D_METHOD("set_active_stage_mask", "stage_mask"), &CommunicationsTaskHost::set_active_stage_mask);
    ClassDB::bind_method(D_METHOD("get_active_stage_mask"), &CommunicationsTaskHost::get_active_stage_mask);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "active_stage_mask", PROPERTY_HINT_FLAGS, "Wave1_DiscourseTurn:1,Wave2_ChannelSNR:2,Wave3_PhonemeDegrade:4,Wave4_ReceiverMindHandoff:8"),
        "set_active_stage_mask",
        "get_active_stage_mask"
    );

    ClassDB::bind_method(D_METHOD("set_auto_stage_on_ready", "auto_stage"), &CommunicationsTaskHost::set_auto_stage_on_ready);
    ClassDB::bind_method(D_METHOD("get_auto_stage_on_ready"), &CommunicationsTaskHost::get_auto_stage_on_ready);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_stage_on_ready"), "set_auto_stage_on_ready", "get_auto_stage_on_ready");

    ClassDB::bind_method(D_METHOD("step_communications_pipeline"), &CommunicationsTaskHost::step_communications_pipeline);

    ADD_SIGNAL(MethodInfo("communications_pipeline_executed", PropertyInfo(Variant::ARRAY, "epistemic_handoffs")));
}

void CommunicationsTaskHost::_ready() {
    if (auto_stage_on_ready_ && domain_resource_.is_valid()) {
        domain_resource_->stage_resources_to_dod();
    }
}

bool CommunicationsTaskHost::step_communications_pipeline() {
    if (domain_resource_.is_null()) return false;
    const bool ok = domain_resource_->execute_pipeline_direct(active_stage_mask_);
    if (ok) {
        emit_signal("communications_pipeline_executed", domain_resource_->read_epistemic_handoffs_as_dicts());
    }
    return ok;
}

} // namespace ideam::godot_ext
