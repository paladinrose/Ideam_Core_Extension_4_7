#include "godot/tasks/domains/geomorphology/data_oriented_geomorphology.h"

#include <godot_cpp/core/class_db.hpp>

namespace ideam::tasks::geomorphology {

void DataOrientedGeomorphology::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("bind_terrain_resource", "terrain"), &DataOrientedGeomorphology::bind_terrain_resource);
    ClassDB::bind_method(D_METHOD("get_bound_terrain_resource"), &DataOrientedGeomorphology::get_bound_terrain_resource);

    ClassDB::bind_method(D_METHOD("set_stage_mask", "mask"), &DataOrientedGeomorphology::set_stage_mask);
    ClassDB::bind_method(D_METHOD("get_stage_mask"), &DataOrientedGeomorphology::get_stage_mask);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "stage_mask"), "set_stage_mask", "get_stage_mask");

    ClassDB::bind_method(D_METHOD("initialize_from_seed"), &DataOrientedGeomorphology::initialize_from_seed);
    ClassDB::bind_method(D_METHOD("execute_pipeline_step", "dt_geological_myr", "dt_hydrological_sec", "precipitation_rate"), &DataOrientedGeomorphology::execute_pipeline_step);
    ClassDB::bind_method(D_METHOD("get_pipeline_telemetry"), &DataOrientedGeomorphology::get_pipeline_telemetry);
}

void DataOrientedGeomorphology::bind_terrain_resource(const godot::Ref<ideam::geomorphology::GeomorphologyTerrainResource>& p_terrain) {
    active_terrain_ = p_terrain;
}

godot::Ref<ideam::geomorphology::GeomorphologyTerrainResource> DataOrientedGeomorphology::get_bound_terrain_resource() const {
    return active_terrain_;
}

void DataOrientedGeomorphology::set_stage_mask(int p_mask) {
    stage_mask_ = static_cast<uint32_t>(p_mask);
}

int DataOrientedGeomorphology::get_stage_mask() const {
    return static_cast<int>(stage_mask_);
}

void DataOrientedGeomorphology::initialize_from_seed() {
    if (active_terrain_.is_valid()) {
        active_terrain_->generate_initial_world();
    }
}

void DataOrientedGeomorphology::execute_pipeline_step(
    float p_dt_geological_myr,
    float p_dt_hydrological_sec,
    float p_precipitation_rate) {
    if (active_terrain_.is_null() || active_terrain_->get_seed_profile().is_null()) {
        return;
    }
    const auto seed = active_terrain_->get_seed_profile()->to_pod();
    GeomorphologyDOD::execute_step(
        seed,
        active_terrain_->get_catalog_span(),
        active_terrain_->get_plates_span(),
        active_terrain_->get_crust_span(),
        active_terrain_->get_strata_span(),
        active_terrain_->get_hydrology_span(),
        active_terrain_->get_hazards_span(),
        p_dt_geological_myr,
        p_dt_hydrological_sec,
        p_precipitation_rate,
        stage_mask_);
}

godot::Dictionary DataOrientedGeomorphology::get_pipeline_telemetry() const {
    if (active_terrain_.is_valid()) {
        return active_terrain_->get_domain_telemetry();
    }
    return godot::Dictionary();
}

} // namespace ideam::tasks::geomorphology
