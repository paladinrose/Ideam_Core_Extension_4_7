#include "godot/tasks/domains/ecology/ecology_task_host.h"

#include <godot_cpp/core/class_db.hpp>

namespace ideam::tasks::ecology {

EcologyTaskHost::EcologyTaskHost() {
    dod_ecology_.instantiate();
}

void EcologyTaskHost::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_biosphere_resource", "biosphere"), &EcologyTaskHost::set_biosphere_resource);
    ClassDB::bind_method(D_METHOD("get_biosphere_resource"), &EcologyTaskHost::get_biosphere_resource);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "biosphere_resource", PROPERTY_HINT_RESOURCE_TYPE, "EcologyBiosphereResource"), "set_biosphere_resource", "get_biosphere_resource");

    ClassDB::bind_method(D_METHOD("get_dod_ecology"), &EcologyTaskHost::get_dod_ecology);

    ClassDB::bind_method(D_METHOD("set_auto_step_in_process", "enabled"), &EcologyTaskHost::set_auto_step_in_process);
    ClassDB::bind_method(D_METHOD("get_auto_step_in_process"), &EcologyTaskHost::get_auto_step_in_process);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_step_in_process"), "set_auto_step_in_process", "get_auto_step_in_process");

    ClassDB::bind_method(D_METHOD("set_dt_years_per_step", "dt"), &EcologyTaskHost::set_dt_years_per_step);
    ClassDB::bind_method(D_METHOD("get_dt_years_per_step"), &EcologyTaskHost::get_dt_years_per_step);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "dt_years_per_step"), "set_dt_years_per_step", "get_dt_years_per_step");

    ClassDB::bind_method(D_METHOD("step_once"), &EcologyTaskHost::step_once);
}

void EcologyTaskHost::_ready() {
    if (biosphere_resource_.is_valid() && dod_ecology_.is_valid()) {
        dod_ecology_->bind_biosphere_resource(biosphere_resource_);
    }
    set_process(auto_step_in_process_);
}

void EcologyTaskHost::_process(double p_delta) {
    (void)p_delta;
    if (auto_step_in_process_) {
        step_once();
    }
}

void EcologyTaskHost::set_biosphere_resource(const godot::Ref<ideam::ecology::EcologyBiosphereResource>& p_biosphere) {
    biosphere_resource_ = p_biosphere;
    if (dod_ecology_.is_valid()) {
        dod_ecology_->bind_biosphere_resource(biosphere_resource_);
    }
}

godot::Ref<ideam::ecology::EcologyBiosphereResource> EcologyTaskHost::get_biosphere_resource() const {
    return biosphere_resource_;
}

godot::Ref<DataOrientedEcology> EcologyTaskHost::get_dod_ecology() const {
    return dod_ecology_;
}

void EcologyTaskHost::set_auto_step_in_process(bool p_enabled) {
    auto_step_in_process_ = p_enabled;
    set_process(auto_step_in_process_);
}

bool EcologyTaskHost::get_auto_step_in_process() const {
    return auto_step_in_process_;
}

void EcologyTaskHost::set_dt_years_per_step(float p_dt) {
    dt_years_per_step_ = p_dt;
}

float EcologyTaskHost::get_dt_years_per_step() const {
    return dt_years_per_step_;
}

void EcologyTaskHost::step_once() {
    if (dod_ecology_.is_valid()) {
        dod_ecology_->execute_pipeline_step(dt_years_per_step_);
    }
}

} // namespace ideam::tasks::ecology
