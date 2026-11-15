#include "godot/tasks/domains/geomorphology/geomorphology_task_host.h"

#include <godot_cpp/core/class_db.hpp>

namespace ideam::tasks::geomorphology {

GeomorphologyTaskHost::GeomorphologyTaskHost() {
    dod_geomorphology_.instantiate();
}

void GeomorphologyTaskHost::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_terrain_resource", "terrain"), &GeomorphologyTaskHost::set_terrain_resource);
    ClassDB::bind_method(D_METHOD("get_terrain_resource"), &GeomorphologyTaskHost::get_terrain_resource);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "terrain_resource", PROPERTY_HINT_RESOURCE_TYPE, "GeomorphologyTerrainResource"), "set_terrain_resource", "get_terrain_resource");

    ClassDB::bind_method(D_METHOD("get_dod_geomorphology"), &GeomorphologyTaskHost::get_dod_geomorphology);

    ClassDB::bind_method(D_METHOD("set_auto_step_in_process", "enabled"), &GeomorphologyTaskHost::set_auto_step_in_process);
    ClassDB::bind_method(D_METHOD("get_auto_step_in_process"), &GeomorphologyTaskHost::get_auto_step_in_process);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_step_in_process"), "set_auto_step_in_process", "get_auto_step_in_process");

    ClassDB::bind_method(D_METHOD("set_geological_dt_per_step_myr", "dt"), &GeomorphologyTaskHost::set_geological_dt_per_step_myr);
    ClassDB::bind_method(D_METHOD("get_geological_dt_per_step_myr"), &GeomorphologyTaskHost::get_geological_dt_per_step_myr);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "geological_dt_per_step_myr"), "set_geological_dt_per_step_myr", "get_geological_dt_per_step_myr");

    ClassDB::bind_method(D_METHOD("set_hydrological_dt_per_step_sec", "dt"), &GeomorphologyTaskHost::set_hydrological_dt_per_step_sec);
    ClassDB::bind_method(D_METHOD("get_hydrological_dt_per_step_sec"), &GeomorphologyTaskHost::get_hydrological_dt_per_step_sec);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "hydrological_dt_per_step_sec"), "set_hydrological_dt_per_step_sec", "get_hydrological_dt_per_step_sec");

    ClassDB::bind_method(D_METHOD("set_precipitation_rate", "rate"), &GeomorphologyTaskHost::set_precipitation_rate);
    ClassDB::bind_method(D_METHOD("get_precipitation_rate"), &GeomorphologyTaskHost::get_precipitation_rate);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "precipitation_rate"), "set_precipitation_rate", "get_precipitation_rate");

    ClassDB::bind_method(D_METHOD("step_once"), &GeomorphologyTaskHost::step_once);
}

void GeomorphologyTaskHost::_ready() {
    if (terrain_resource_.is_valid() && dod_geomorphology_.is_valid()) {
        dod_geomorphology_->bind_terrain_resource(terrain_resource_);
    }
    set_process(auto_step_in_process_);
}

void GeomorphologyTaskHost::_process(double p_delta) {
    (void)p_delta;
    if (auto_step_in_process_) {
        step_once();
    }
}

void GeomorphologyTaskHost::set_terrain_resource(const godot::Ref<ideam::geomorphology::GeomorphologyTerrainResource>& p_terrain) {
    terrain_resource_ = p_terrain;
    if (dod_geomorphology_.is_valid()) {
        dod_geomorphology_->bind_terrain_resource(terrain_resource_);
    }
}

godot::Ref<ideam::geomorphology::GeomorphologyTerrainResource> GeomorphologyTaskHost::get_terrain_resource() const {
    return terrain_resource_;
}

godot::Ref<DataOrientedGeomorphology> GeomorphologyTaskHost::get_dod_geomorphology() const {
    return dod_geomorphology_;
}

void GeomorphologyTaskHost::set_auto_step_in_process(bool p_enabled) {
    auto_step_in_process_ = p_enabled;
    set_process(auto_step_in_process_);
}

bool GeomorphologyTaskHost::get_auto_step_in_process() const {
    return auto_step_in_process_;
}

void GeomorphologyTaskHost::set_geological_dt_per_step_myr(float p_dt) {
    geological_dt_per_step_myr_ = p_dt;
}

float GeomorphologyTaskHost::get_geological_dt_per_step_myr() const {
    return geological_dt_per_step_myr_;
}

void GeomorphologyTaskHost::set_hydrological_dt_per_step_sec(float p_dt) {
    hydrological_dt_per_step_sec_ = p_dt;
}

float GeomorphologyTaskHost::get_hydrological_dt_per_step_sec() const {
    return hydrological_dt_per_step_sec_;
}

void GeomorphologyTaskHost::set_precipitation_rate(float p_rate) {
    precipitation_rate_ = p_rate;
}

float GeomorphologyTaskHost::get_precipitation_rate() const {
    return precipitation_rate_;
}

void GeomorphologyTaskHost::step_once() {
    if (dod_geomorphology_.is_valid()) {
        dod_geomorphology_->execute_pipeline_step(
            geological_dt_per_step_myr_,
            hydrological_dt_per_step_sec_,
            precipitation_rate_);
    }
}

} // namespace ideam::tasks::geomorphology
