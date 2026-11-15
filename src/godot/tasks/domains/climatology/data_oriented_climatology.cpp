#include "godot/tasks/domains/climatology/data_oriented_climatology.h"

#include <godot_cpp/core/class_db.hpp>

namespace ideam::tasks::climatology {

void DataOrientedClimatology::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("bind_atmosphere_resource", "atmosphere"), &DataOrientedClimatology::bind_atmosphere_resource);
    ClassDB::bind_method(D_METHOD("get_bound_atmosphere_resource"), &DataOrientedClimatology::get_bound_atmosphere_resource);

    ClassDB::bind_method(D_METHOD("set_stage_mask", "mask"), &DataOrientedClimatology::set_stage_mask);
    ClassDB::bind_method(D_METHOD("get_stage_mask"), &DataOrientedClimatology::get_stage_mask);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "stage_mask"), "set_stage_mask", "get_stage_mask");

    ClassDB::bind_method(D_METHOD("initialize_from_seed"), &DataOrientedClimatology::initialize_from_seed);
    ClassDB::bind_method(D_METHOD("execute_pipeline_step", "dt_synoptic_sec", "dt_vortex_sec"), &DataOrientedClimatology::execute_pipeline_step);
    ClassDB::bind_method(D_METHOD("get_pipeline_telemetry"), &DataOrientedClimatology::get_pipeline_telemetry);
}

void DataOrientedClimatology::bind_atmosphere_resource(const godot::Ref<ideam::climatology::ClimatologyAtmosphereResource>& p_atmosphere) {
    active_atmosphere_ = p_atmosphere;
}

godot::Ref<ideam::climatology::ClimatologyAtmosphereResource> DataOrientedClimatology::get_bound_atmosphere_resource() const {
    return active_atmosphere_;
}

void DataOrientedClimatology::set_stage_mask(int p_mask) {
    stage_mask_ = static_cast<uint32_t>(p_mask);
}

int DataOrientedClimatology::get_stage_mask() const {
    return static_cast<int>(stage_mask_);
}

void DataOrientedClimatology::initialize_from_seed() {
    if (active_atmosphere_.is_valid()) {
        active_atmosphere_->generate_initial_climate();
    }
}

void DataOrientedClimatology::execute_pipeline_step(float p_dt_synoptic_sec, float p_dt_vortex_sec) {
    if (active_atmosphere_.is_null() || active_atmosphere_->get_seed_profile().is_null()) {
        return;
    }
    const auto seed = active_atmosphere_->get_seed_profile()->to_pod();
    ClimatologyDOD::execute_step(
        seed,
        active_atmosphere_->get_atmo_span(),
        active_atmosphere_->get_moisture_span(),
        active_atmosphere_->get_vortices_span(),
        active_atmosphere_->get_hazards_span(),
        p_dt_synoptic_sec,
        p_dt_vortex_sec,
        stage_mask_);
}

godot::Dictionary DataOrientedClimatology::get_pipeline_telemetry() const {
    if (active_atmosphere_.is_valid()) {
        return active_atmosphere_->get_domain_telemetry();
    }
    return godot::Dictionary();
}

} // namespace ideam::tasks::climatology
