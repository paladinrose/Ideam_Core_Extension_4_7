#include "godot/tasks/domains/climatology/climatology_task_host.h"

#include <godot_cpp/core/class_db.hpp>

namespace ideam::tasks::climatology {

ClimatologyTaskHost::ClimatologyTaskHost() {
    dod_climatology_.instantiate();
}

void ClimatologyTaskHost::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_atmosphere_resource", "atmosphere"), &ClimatologyTaskHost::set_atmosphere_resource);
    ClassDB::bind_method(D_METHOD("get_atmosphere_resource"), &ClimatologyTaskHost::get_atmosphere_resource);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "atmosphere_resource", PROPERTY_HINT_RESOURCE_TYPE, "ClimatologyAtmosphereResource"), "set_atmosphere_resource", "get_atmosphere_resource");

    ClassDB::bind_method(D_METHOD("get_dod_climatology"), &ClimatologyTaskHost::get_dod_climatology);

    ClassDB::bind_method(D_METHOD("set_auto_step_in_process", "enabled"), &ClimatologyTaskHost::set_auto_step_in_process);
    ClassDB::bind_method(D_METHOD("get_auto_step_in_process"), &ClimatologyTaskHost::get_auto_step_in_process);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_step_in_process"), "set_auto_step_in_process", "get_auto_step_in_process");

    ClassDB::bind_method(D_METHOD("set_synoptic_dt_per_step_sec", "dt"), &ClimatologyTaskHost::set_synoptic_dt_per_step_sec);
    ClassDB::bind_method(D_METHOD("get_synoptic_dt_per_step_sec"), &ClimatologyTaskHost::get_synoptic_dt_per_step_sec);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "synoptic_dt_per_step_sec"), "set_synoptic_dt_per_step_sec", "get_synoptic_dt_per_step_sec");

    ClassDB::bind_method(D_METHOD("set_vortex_dt_per_step_sec", "dt"), &ClimatologyTaskHost::set_vortex_dt_per_step_sec);
    ClassDB::bind_method(D_METHOD("get_vortex_dt_per_step_sec"), &ClimatologyTaskHost::get_vortex_dt_per_step_sec);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "vortex_dt_per_step_sec"), "set_vortex_dt_per_step_sec", "get_vortex_dt_per_step_sec");

    ClassDB::bind_method(D_METHOD("step_once"), &ClimatologyTaskHost::step_once);
}

void ClimatologyTaskHost::_ready() {
    if (atmosphere_resource_.is_valid() && dod_climatology_.is_valid()) {
        dod_climatology_->bind_atmosphere_resource(atmosphere_resource_);
    }
    set_process(auto_step_in_process_);
}

void ClimatologyTaskHost::_process(double p_delta) {
    (void)p_delta;
    if (auto_step_in_process_) {
        step_once();
    }
}

void ClimatologyTaskHost::set_atmosphere_resource(const godot::Ref<ideam::climatology::ClimatologyAtmosphereResource>& p_atmosphere) {
    atmosphere_resource_ = p_atmosphere;
    if (dod_climatology_.is_valid()) {
        dod_climatology_->bind_atmosphere_resource(atmosphere_resource_);
    }
}

godot::Ref<ideam::climatology::ClimatologyAtmosphereResource> ClimatologyTaskHost::get_atmosphere_resource() const {
    return atmosphere_resource_;
}

godot::Ref<DataOrientedClimatology> ClimatologyTaskHost::get_dod_climatology() const {
    return dod_climatology_;
}

void ClimatologyTaskHost::set_auto_step_in_process(bool p_enabled) {
    auto_step_in_process_ = p_enabled;
    set_process(auto_step_in_process_);
}

bool ClimatologyTaskHost::get_auto_step_in_process() const {
    return auto_step_in_process_;
}

void ClimatologyTaskHost::set_synoptic_dt_per_step_sec(float p_dt) {
    synoptic_dt_per_step_sec_ = p_dt;
}

float ClimatologyTaskHost::get_synoptic_dt_per_step_sec() const {
    return synoptic_dt_per_step_sec_;
}

void ClimatologyTaskHost::set_vortex_dt_per_step_sec(float p_dt) {
    vortex_dt_per_step_sec_ = p_dt;
}

float ClimatologyTaskHost::get_vortex_dt_per_step_sec() const {
    return vortex_dt_per_step_sec_;
}

void ClimatologyTaskHost::step_once() {
    if (dod_climatology_.is_valid()) {
        dod_climatology_->execute_pipeline_step(
            synoptic_dt_per_step_sec_,
            vortex_dt_per_step_sec_);
    }
}

} // namespace ideam::tasks::climatology
