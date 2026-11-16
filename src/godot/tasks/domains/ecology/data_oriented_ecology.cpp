#include "godot/tasks/domains/ecology/data_oriented_ecology.h"

#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

void DataOrientedEcology::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("bind_biosphere_resource", "biosphere"), &DataOrientedEcology::bind_biosphere_resource);
    ClassDB::bind_method(D_METHOD("get_bound_biosphere_resource"), &DataOrientedEcology::get_bound_biosphere_resource);

    ClassDB::bind_method(D_METHOD("set_stage_mask", "mask"), &DataOrientedEcology::set_stage_mask);
    ClassDB::bind_method(D_METHOD("get_stage_mask"), &DataOrientedEcology::get_stage_mask);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "stage_mask"), "set_stage_mask", "get_stage_mask");

    ClassDB::bind_method(D_METHOD("initialize_from_seed"), &DataOrientedEcology::initialize_from_seed);
    ClassDB::bind_method(D_METHOD("execute_pipeline_step", "dt_years"), &DataOrientedEcology::execute_pipeline_step);
    ClassDB::bind_method(D_METHOD("get_pipeline_telemetry"), &DataOrientedEcology::get_pipeline_telemetry);
}

void DataOrientedEcology::bind_biosphere_resource(const godot::Ref<EcologyBiosphereResource>& p_biosphere) {
    active_biosphere_ = p_biosphere;
}

godot::Ref<EcologyBiosphereResource> DataOrientedEcology::get_bound_biosphere_resource() const {
    return active_biosphere_;
}

void DataOrientedEcology::set_stage_mask(int p_mask) {
    stage_mask_ = static_cast<uint32_t>(p_mask);
}

int DataOrientedEcology::get_stage_mask() const {
    return static_cast<int>(stage_mask_);
}

void DataOrientedEcology::initialize_from_seed() {
    if (active_biosphere_.is_valid()) {
        active_biosphere_->generate_initial_biosphere();
    }
}

void DataOrientedEcology::execute_pipeline_step(float p_dt_years) {
    if (active_biosphere_.is_null() || active_biosphere_->get_seed_profile().is_null()) {
        return;
    }
    const auto seed = active_biosphere_->get_seed_profile()->to_pod();
    core::EcologyDOD::execute_step(
        seed,
        active_biosphere_->get_guilds_span(),
        active_biosphere_->get_eco_span(),
        active_biosphere_->get_biogeo_span(),
        active_biosphere_->get_samples_span(),
        p_dt_years,
        stage_mask_);
}

godot::Dictionary DataOrientedEcology::get_pipeline_telemetry() const {
    if (active_biosphere_.is_valid()) {
        return active_biosphere_->get_domain_telemetry();
    }
    return godot::Dictionary();
}

} // namespace ideam::godot_ext
