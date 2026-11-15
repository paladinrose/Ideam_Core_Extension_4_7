#include "characters_task_host.h"

namespace ideam::godot_ext {

void CharactersTaskHost::_ready() {
    TaskGraphHost::_ready();
    if (auto_stage_on_ready_ && domain_resource_.is_valid()) {
        domain_resource_->initialize_dod_buffers();
        domain_resource_->stage_active_sources_to_dod();
    }
}

bool CharactersTaskHost::step_characters_pipeline() {
    if (domain_resource_.is_null()) {
        return false;
    }
    return domain_resource_->execute_pipeline_direct(active_stage_mask_);
}

void CharactersTaskHost::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_domain_resource", "res"), &CharactersTaskHost::set_domain_resource);
    ClassDB::bind_method(D_METHOD("get_domain_resource"), &CharactersTaskHost::get_domain_resource);

    ClassDB::bind_method(D_METHOD("set_active_stage_mask", "mask"), &CharactersTaskHost::set_active_stage_mask);
    ClassDB::bind_method(D_METHOD("get_active_stage_mask"), &CharactersTaskHost::get_active_stage_mask);

    ClassDB::bind_method(D_METHOD("set_auto_stage_on_ready", "auto_stage"), &CharactersTaskHost::set_auto_stage_on_ready);
    ClassDB::bind_method(D_METHOD("get_auto_stage_on_ready"), &CharactersTaskHost::get_auto_stage_on_ready);

    ClassDB::bind_method(D_METHOD("step_characters_pipeline"), &CharactersTaskHost::step_characters_pipeline);

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "domain_resource", PROPERTY_HINT_RESOURCE_TYPE, "DataOrientedCharacters"), "set_domain_resource", "get_domain_resource");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "active_stage_mask"), "set_active_stage_mask", "get_active_stage_mask");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_stage_on_ready"), "set_auto_stage_on_ready", "get_auto_stage_on_ready");
}

} // namespace ideam::godot_ext
