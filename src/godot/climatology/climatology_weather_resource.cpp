#include "godot/climatology/climatology_weather_resource.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/color.hpp>

#include <algorithm>
#include <cmath>

namespace ideam::godot_ext {

// ============================================================================
// ClimatologySeedProfileResource
// ============================================================================

void ClimatologySeedProfileResource::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_master_seed", "seed"), &ClimatologySeedProfileResource::set_master_seed);
    ClassDB::bind_method(D_METHOD("get_master_seed"), &ClimatologySeedProfileResource::get_master_seed);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "master_seed"), "set_master_seed", "get_master_seed");

    ClassDB::bind_method(D_METHOD("set_grid_width", "width"), &ClimatologySeedProfileResource::set_grid_width);
    ClassDB::bind_method(D_METHOD("get_grid_width"), &ClimatologySeedProfileResource::get_grid_width);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "grid_width", PROPERTY_HINT_RANGE, "8,512,1"), "set_grid_width", "get_grid_width");

    ClassDB::bind_method(D_METHOD("set_grid_height", "height"), &ClimatologySeedProfileResource::set_grid_height);
    ClassDB::bind_method(D_METHOD("get_grid_height"), &ClimatologySeedProfileResource::get_grid_height);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "grid_height", PROPERTY_HINT_RANGE, "8,512,1"), "set_grid_height", "get_grid_height");

    ClassDB::bind_method(D_METHOD("set_max_vortices", "max_vortices"), &ClimatologySeedProfileResource::set_max_vortices);
    ClassDB::bind_method(D_METHOD("get_max_vortices"), &ClimatologySeedProfileResource::get_max_vortices);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_vortices", PROPERTY_HINT_RANGE, "1,16,1"), "set_max_vortices", "get_max_vortices");

    ClassDB::bind_method(D_METHOD("set_cell_size_meters", "size"), &ClimatologySeedProfileResource::set_cell_size_meters);
    ClassDB::bind_method(D_METHOD("get_cell_size_meters"), &ClimatologySeedProfileResource::get_cell_size_meters);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "cell_size_meters", PROPERTY_HINT_RANGE, "1.0,1000.0,0.5"), "set_cell_size_meters", "get_cell_size_meters");

    ClassDB::bind_method(D_METHOD("set_planet_latitude_deg", "lat"), &ClimatologySeedProfileResource::set_planet_latitude_deg);
    ClassDB::bind_method(D_METHOD("get_planet_latitude_deg"), &ClimatologySeedProfileResource::get_planet_latitude_deg);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "planet_latitude_deg", PROPERTY_HINT_RANGE, "-90.0,90.0,0.5"), "set_planet_latitude_deg", "get_planet_latitude_deg");

    ClassDB::bind_method(D_METHOD("set_seasonal_solar_decl_deg", "decl"), &ClimatologySeedProfileResource::set_seasonal_solar_decl_deg);
    ClassDB::bind_method(D_METHOD("get_seasonal_solar_decl_deg"), &ClimatologySeedProfileResource::get_seasonal_solar_decl_deg);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "seasonal_solar_decl_deg", PROPERTY_HINT_RANGE, "-23.5,23.5,0.5"), "set_seasonal_solar_decl_deg", "get_seasonal_solar_decl_deg");

    ClassDB::bind_method(D_METHOD("set_diurnal_hour_0_24", "hour"), &ClimatologySeedProfileResource::set_diurnal_hour_0_24);
    ClassDB::bind_method(D_METHOD("get_diurnal_hour_0_24"), &ClimatologySeedProfileResource::get_diurnal_hour_0_24);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "diurnal_hour_0_24", PROPERTY_HINT_RANGE, "0.0,24.0,0.25"), "set_diurnal_hour_0_24", "get_diurnal_hour_0_24");

    ClassDB::bind_method(D_METHOD("set_solar_constant_w_m2", "solar"), &ClimatologySeedProfileResource::set_solar_constant_w_m2);
    ClassDB::bind_method(D_METHOD("get_solar_constant_w_m2"), &ClimatologySeedProfileResource::get_solar_constant_w_m2);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "solar_constant_w_m2", PROPERTY_HINT_RANGE, "100.0,3000.0,10.0"), "set_solar_constant_w_m2", "get_solar_constant_w_m2");

    ClassDB::bind_method(D_METHOD("set_coriolis_parameter_f0", "f0"), &ClimatologySeedProfileResource::set_coriolis_parameter_f0);
    ClassDB::bind_method(D_METHOD("get_coriolis_parameter_f0"), &ClimatologySeedProfileResource::get_coriolis_parameter_f0);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "coriolis_parameter_f0"), "set_coriolis_parameter_f0", "get_coriolis_parameter_f0");

    ClassDB::bind_method(D_METHOD("set_sea_level_pressure_hpa", "pressure"), &ClimatologySeedProfileResource::set_sea_level_pressure_hpa);
    ClassDB::bind_method(D_METHOD("get_sea_level_pressure_hpa"), &ClimatologySeedProfileResource::get_sea_level_pressure_hpa);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "sea_level_pressure_hpa", PROPERTY_HINT_RANGE, "850.0,1080.0,0.5"), "set_sea_level_pressure_hpa", "get_sea_level_pressure_hpa");

    ClassDB::bind_method(D_METHOD("set_sea_level_temp_k", "temp"), &ClimatologySeedProfileResource::set_sea_level_temp_k);
    ClassDB::bind_method(D_METHOD("get_sea_level_temp_k"), &ClimatologySeedProfileResource::get_sea_level_temp_k);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "sea_level_temp_k", PROPERTY_HINT_RANGE, "230.0,330.0,0.5"), "set_sea_level_temp_k", "get_sea_level_temp_k");

    ClassDB::bind_method(D_METHOD("set_prevailing_zonal_wind_mps", "u"), &ClimatologySeedProfileResource::set_prevailing_zonal_wind_mps);
    ClassDB::bind_method(D_METHOD("get_prevailing_zonal_wind_mps"), &ClimatologySeedProfileResource::get_prevailing_zonal_wind_mps);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "prevailing_zonal_wind_mps", PROPERTY_HINT_RANGE, "-40.0,40.0,0.5"), "set_prevailing_zonal_wind_mps", "get_prevailing_zonal_wind_mps");

    ClassDB::bind_method(D_METHOD("set_prevailing_merid_wind_mps", "v"), &ClimatologySeedProfileResource::set_prevailing_merid_wind_mps);
    ClassDB::bind_method(D_METHOD("get_prevailing_merid_wind_mps"), &ClimatologySeedProfileResource::get_prevailing_merid_wind_mps);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "prevailing_merid_wind_mps", PROPERTY_HINT_RANGE, "-40.0,40.0,0.5"), "set_prevailing_merid_wind_mps", "get_prevailing_merid_wind_mps");

    ClassDB::bind_method(D_METHOD("set_synoptic_wave_amplitude_hpa", "amp"), &ClimatologySeedProfileResource::set_synoptic_wave_amplitude_hpa);
    ClassDB::bind_method(D_METHOD("get_synoptic_wave_amplitude_hpa"), &ClimatologySeedProfileResource::get_synoptic_wave_amplitude_hpa);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "synoptic_wave_amplitude_hpa", PROPERTY_HINT_RANGE, "0.0,45.0,0.5"), "set_synoptic_wave_amplitude_hpa", "get_synoptic_wave_amplitude_hpa");

    ClassDB::bind_method(D_METHOD("set_orographic_lift_scale", "scale"), &ClimatologySeedProfileResource::set_orographic_lift_scale);
    ClassDB::bind_method(D_METHOD("get_orographic_lift_scale"), &ClimatologySeedProfileResource::get_orographic_lift_scale);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "orographic_lift_scale", PROPERTY_HINT_RANGE, "0.0,5.0,0.05"), "set_orographic_lift_scale", "get_orographic_lift_scale");

    ClassDB::bind_method(D_METHOD("set_tornado_spawn_ehi_threshold", "ehi"), &ClimatologySeedProfileResource::set_tornado_spawn_ehi_threshold);
    ClassDB::bind_method(D_METHOD("get_tornado_spawn_ehi_threshold"), &ClimatologySeedProfileResource::get_tornado_spawn_ehi_threshold);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "tornado_spawn_ehi_threshold", PROPERTY_HINT_RANGE, "0.1,10.0,0.05"), "set_tornado_spawn_ehi_threshold", "get_tornado_spawn_ehi_threshold");

    ClassDB::bind_method(D_METHOD("get_solar_insolation_seed"), &ClimatologySeedProfileResource::get_solar_insolation_seed);
    ClassDB::bind_method(D_METHOD("get_baroclinic_wave_seed"), &ClimatologySeedProfileResource::get_baroclinic_wave_seed);
    ClassDB::bind_method(D_METHOD("get_convective_storm_seed"), &ClimatologySeedProfileResource::get_convective_storm_seed);
    ClassDB::bind_method(D_METHOD("get_tornado_vortex_seed"), &ClimatologySeedProfileResource::get_tornado_vortex_seed);
}

void ClimatologySeedProfileResource::set_master_seed(int64_t p_seed) { master_seed_ = p_seed; emit_changed(); }
int64_t ClimatologySeedProfileResource::get_master_seed() const { return master_seed_; }

void ClimatologySeedProfileResource::set_grid_width(int p_width) { grid_width_ = std::clamp(p_width, 8, 512); emit_changed(); }
int ClimatologySeedProfileResource::get_grid_width() const { return grid_width_; }

void ClimatologySeedProfileResource::set_grid_height(int p_height) { grid_height_ = std::clamp(p_height, 8, 512); emit_changed(); }
int ClimatologySeedProfileResource::get_grid_height() const { return grid_height_; }

void ClimatologySeedProfileResource::set_max_vortices(int p_max) { max_vortices_ = std::clamp(p_max, 1, static_cast<int>(core::MAX_ACTIVE_TORNADO_VORTICES)); emit_changed(); }
int ClimatologySeedProfileResource::get_max_vortices() const { return max_vortices_; }

void ClimatologySeedProfileResource::set_cell_size_meters(float p_size) { cell_size_meters_ = std::max(1.0f, p_size); emit_changed(); }
float ClimatologySeedProfileResource::get_cell_size_meters() const { return cell_size_meters_; }

void ClimatologySeedProfileResource::set_planet_latitude_deg(float p_lat) { planet_latitude_deg_ = std::clamp(p_lat, -90.0f, 90.0f); emit_changed(); }
float ClimatologySeedProfileResource::get_planet_latitude_deg() const { return planet_latitude_deg_; }

void ClimatologySeedProfileResource::set_seasonal_solar_decl_deg(float p_decl) { seasonal_solar_decl_deg_ = std::clamp(p_decl, -23.5f, 23.5f); emit_changed(); }
float ClimatologySeedProfileResource::get_seasonal_solar_decl_deg() const { return seasonal_solar_decl_deg_; }

void ClimatologySeedProfileResource::set_diurnal_hour_0_24(float p_hour) { diurnal_hour_0_24_ = std::clamp(p_hour, 0.0f, 24.0f); emit_changed(); }
float ClimatologySeedProfileResource::get_diurnal_hour_0_24() const { return diurnal_hour_0_24_; }

void ClimatologySeedProfileResource::set_solar_constant_w_m2(float p_solar) { solar_constant_w_m2_ = std::max(0.0f, p_solar); emit_changed(); }
float ClimatologySeedProfileResource::get_solar_constant_w_m2() const { return solar_constant_w_m2_; }

void ClimatologySeedProfileResource::set_coriolis_parameter_f0(float p_f0) { coriolis_parameter_f0_ = p_f0; emit_changed(); }
float ClimatologySeedProfileResource::get_coriolis_parameter_f0() const { return coriolis_parameter_f0_; }

void ClimatologySeedProfileResource::set_sea_level_pressure_hpa(float p_pres) { sea_level_pressure_hpa_ = std::max(300.0f, p_pres); emit_changed(); }
float ClimatologySeedProfileResource::get_sea_level_pressure_hpa() const { return sea_level_pressure_hpa_; }

void ClimatologySeedProfileResource::set_sea_level_temp_k(float p_temp) { sea_level_temp_k_ = std::clamp(p_temp, 200.0f, 350.0f); emit_changed(); }
float ClimatologySeedProfileResource::get_sea_level_temp_k() const { return sea_level_temp_k_; }

void ClimatologySeedProfileResource::set_prevailing_zonal_wind_mps(float p_u) { prevailing_zonal_wind_mps_ = p_u; emit_changed(); }
float ClimatologySeedProfileResource::get_prevailing_zonal_wind_mps() const { return prevailing_zonal_wind_mps_; }

void ClimatologySeedProfileResource::set_prevailing_merid_wind_mps(float p_v) { prevailing_merid_wind_mps_ = p_v; emit_changed(); }
float ClimatologySeedProfileResource::get_prevailing_merid_wind_mps() const { return prevailing_merid_wind_mps_; }

void ClimatologySeedProfileResource::set_synoptic_wave_amplitude_hpa(float p_amp) { synoptic_wave_amplitude_hpa_ = std::max(0.0f, p_amp); emit_changed(); }
float ClimatologySeedProfileResource::get_synoptic_wave_amplitude_hpa() const { return synoptic_wave_amplitude_hpa_; }

void ClimatologySeedProfileResource::set_orographic_lift_scale(float p_scale) { orographic_lift_scale_ = std::max(0.0f, p_scale); emit_changed(); }
float ClimatologySeedProfileResource::get_orographic_lift_scale() const { return orographic_lift_scale_; }

void ClimatologySeedProfileResource::set_tornado_spawn_ehi_threshold(float p_ehi) { tornado_spawn_ehi_threshold_ = std::max(0.05f, p_ehi); emit_changed(); }
float ClimatologySeedProfileResource::get_tornado_spawn_ehi_threshold() const { return tornado_spawn_ehi_threshold_; }

int64_t ClimatologySeedProfileResource::get_solar_insolation_seed() const { return static_cast<int64_t>(to_pod().solar_insolation_seed); }
int64_t ClimatologySeedProfileResource::get_baroclinic_wave_seed() const { return static_cast<int64_t>(to_pod().baroclinic_wave_seed); }
int64_t ClimatologySeedProfileResource::get_convective_storm_seed() const { return static_cast<int64_t>(to_pod().convective_storm_seed); }
int64_t ClimatologySeedProfileResource::get_tornado_vortex_seed() const { return static_cast<int64_t>(to_pod().tornado_vortex_seed); }

core::ClimatologySeedProfilePOD ClimatologySeedProfileResource::to_pod() const noexcept {
    core::ClimatologySeedProfilePOD pod = core::ClimatologyKernelMath::make_seed_profile(
        static_cast<uint64_t>(master_seed_),
        static_cast<uint32_t>(grid_width_),
        static_cast<uint32_t>(grid_height_),
        static_cast<uint32_t>(max_vortices_),
        cell_size_meters_);
    pod.planet_latitude_deg = planet_latitude_deg_;
    pod.seasonal_solar_decl_deg = seasonal_solar_decl_deg_;
    pod.diurnal_hour_0_24 = diurnal_hour_0_24_;
    pod.solar_constant_w_m2 = solar_constant_w_m2_;
    pod.coriolis_parameter_f0 = coriolis_parameter_f0_;
    pod.sea_level_pressure_hpa = sea_level_pressure_hpa_;
    pod.sea_level_temp_k = sea_level_temp_k_;
    pod.prevailing_zonal_wind_mps = prevailing_zonal_wind_mps_;
    pod.prevailing_merid_wind_mps = prevailing_merid_wind_mps_;
    pod.synoptic_wave_amplitude_hpa = synoptic_wave_amplitude_hpa_;
    pod.orographic_lift_scale = orographic_lift_scale_;
    pod.tornado_spawn_ehi_threshold = tornado_spawn_ehi_threshold_;
    return pod;
}

void ClimatologySeedProfileResource::from_pod(const core::ClimatologySeedProfilePOD& pod) {
    master_seed_ = static_cast<int64_t>(pod.master_seed);
    grid_width_ = static_cast<int>(pod.grid_width);
    grid_height_ = static_cast<int>(pod.grid_height);
    max_vortices_ = static_cast<int>(pod.max_vortices);
    cell_size_meters_ = pod.cell_size_meters;
    planet_latitude_deg_ = pod.planet_latitude_deg;
    seasonal_solar_decl_deg_ = pod.seasonal_solar_decl_deg;
    diurnal_hour_0_24_ = pod.diurnal_hour_0_24;
    solar_constant_w_m2_ = pod.solar_constant_w_m2;
    coriolis_parameter_f0_ = pod.coriolis_parameter_f0;
    sea_level_pressure_hpa_ = pod.sea_level_pressure_hpa;
    sea_level_temp_k_ = pod.sea_level_temp_k;
    prevailing_zonal_wind_mps_ = pod.prevailing_zonal_wind_mps;
    prevailing_merid_wind_mps_ = pod.prevailing_merid_wind_mps;
    synoptic_wave_amplitude_hpa_ = pod.synoptic_wave_amplitude_hpa;
    orographic_lift_scale_ = pod.orographic_lift_scale;
    tornado_spawn_ehi_threshold_ = pod.tornado_spawn_ehi_threshold;
    emit_changed();
}

// ============================================================================
// TornadoVortexProfileResource
// ============================================================================

void TornadoVortexProfileResource::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_core_radius_m", "val"), &TornadoVortexProfileResource::set_core_radius_m);
    ClassDB::bind_method(D_METHOD("get_core_radius_m"), &TornadoVortexProfileResource::get_core_radius_m);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "core_radius_m", PROPERTY_HINT_RANGE, "10.0,600.0,5.0"), "set_core_radius_m", "get_core_radius_m");

    ClassDB::bind_method(D_METHOD("set_outer_damage_radius_m", "val"), &TornadoVortexProfileResource::set_outer_damage_radius_m);
    ClassDB::bind_method(D_METHOD("get_outer_damage_radius_m"), &TornadoVortexProfileResource::get_outer_damage_radius_m);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "outer_damage_radius_m", PROPERTY_HINT_RANGE, "50.0,2500.0,10.0"), "set_outer_damage_radius_m", "get_outer_damage_radius_m");

    ClassDB::bind_method(D_METHOD("set_max_tangential_wind_mps", "val"), &TornadoVortexProfileResource::set_max_tangential_wind_mps);
    ClassDB::bind_method(D_METHOD("get_max_tangential_wind_mps"), &TornadoVortexProfileResource::get_max_tangential_wind_mps);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_tangential_wind_mps", PROPERTY_HINT_RANGE, "25.0,135.0,1.0"), "set_max_tangential_wind_mps", "get_max_tangential_wind_mps");

    ClassDB::bind_method(D_METHOD("set_central_pressure_deficit_hpa", "val"), &TornadoVortexProfileResource::set_central_pressure_deficit_hpa);
    ClassDB::bind_method(D_METHOD("get_central_pressure_deficit_hpa"), &TornadoVortexProfileResource::get_central_pressure_deficit_hpa);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "central_pressure_deficit_hpa", PROPERTY_HINT_RANGE, "5.0,150.0,1.0"), "set_central_pressure_deficit_hpa", "get_central_pressure_deficit_hpa");

    ClassDB::bind_method(D_METHOD("set_vertical_suction_updraft_mps", "val"), &TornadoVortexProfileResource::set_vertical_suction_updraft_mps);
    ClassDB::bind_method(D_METHOD("get_vertical_suction_updraft_mps"), &TornadoVortexProfileResource::get_vertical_suction_updraft_mps);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "vertical_suction_updraft_mps", PROPERTY_HINT_RANGE, "5.0,95.0,1.0"), "set_vertical_suction_updraft_mps", "get_vertical_suction_updraft_mps");

    ClassDB::bind_method(D_METHOD("set_radial_inflow_ratio", "val"), &TornadoVortexProfileResource::set_radial_inflow_ratio);
    ClassDB::bind_method(D_METHOD("get_radial_inflow_ratio"), &TornadoVortexProfileResource::get_radial_inflow_ratio);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "radial_inflow_ratio", PROPERTY_HINT_RANGE, "0.05,1.5,0.02"), "set_radial_inflow_ratio", "get_radial_inflow_ratio");

    ClassDB::bind_method(D_METHOD("set_swirl_ratio_s", "val"), &TornadoVortexProfileResource::set_swirl_ratio_s);
    ClassDB::bind_method(D_METHOD("get_swirl_ratio_s"), &TornadoVortexProfileResource::get_swirl_ratio_s);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "swirl_ratio_s", PROPERTY_HINT_RANGE, "0.1,4.0,0.05"), "set_swirl_ratio_s", "get_swirl_ratio_s");

    ClassDB::bind_method(D_METHOD("set_default_lifetime_sec", "val"), &TornadoVortexProfileResource::set_default_lifetime_sec);
    ClassDB::bind_method(D_METHOD("get_default_lifetime_sec"), &TornadoVortexProfileResource::get_default_lifetime_sec);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "default_lifetime_sec", PROPERTY_HINT_RANGE, "10.0,3600.0,10.0"), "set_default_lifetime_sec", "get_default_lifetime_sec");

    ClassDB::bind_method(D_METHOD("get_ef_rating"), &TornadoVortexProfileResource::get_ef_rating);
}

void TornadoVortexProfileResource::set_core_radius_m(float p_val) { core_radius_m_ = std::max(10.0f, p_val); emit_changed(); }
float TornadoVortexProfileResource::get_core_radius_m() const { return core_radius_m_; }

void TornadoVortexProfileResource::set_outer_damage_radius_m(float p_val) { outer_damage_radius_m_ = std::max(30.0f, p_val); emit_changed(); }
float TornadoVortexProfileResource::get_outer_damage_radius_m() const { return outer_damage_radius_m_; }

void TornadoVortexProfileResource::set_max_tangential_wind_mps(float p_val) { max_tangential_wind_mps_ = std::max(15.0f, p_val); emit_changed(); }
float TornadoVortexProfileResource::get_max_tangential_wind_mps() const { return max_tangential_wind_mps_; }

void TornadoVortexProfileResource::set_central_pressure_deficit_hpa(float p_val) { central_pressure_deficit_hpa_ = std::max(1.0f, p_val); emit_changed(); }
float TornadoVortexProfileResource::get_central_pressure_deficit_hpa() const { return central_pressure_deficit_hpa_; }

void TornadoVortexProfileResource::set_vertical_suction_updraft_mps(float p_val) { vertical_suction_updraft_mps_ = std::max(1.0f, p_val); emit_changed(); }
float TornadoVortexProfileResource::get_vertical_suction_updraft_mps() const { return vertical_suction_updraft_mps_; }

void TornadoVortexProfileResource::set_radial_inflow_ratio(float p_val) { radial_inflow_ratio_ = std::clamp(p_val, 0.05f, 1.5f); emit_changed(); }
float TornadoVortexProfileResource::get_radial_inflow_ratio() const { return radial_inflow_ratio_; }

void TornadoVortexProfileResource::set_swirl_ratio_s(float p_val) { swirl_ratio_s_ = std::clamp(p_val, 0.1f, 4.0f); emit_changed(); }
float TornadoVortexProfileResource::get_swirl_ratio_s() const { return swirl_ratio_s_; }

void TornadoVortexProfileResource::set_default_lifetime_sec(float p_val) { default_lifetime_sec_ = std::max(5.0f, p_val); emit_changed(); }
float TornadoVortexProfileResource::get_default_lifetime_sec() const { return default_lifetime_sec_; }

int TornadoVortexProfileResource::get_ef_rating() const {
    return static_cast<int>(core::ClimatologyKernelMath::classify_ef_scale(max_tangential_wind_mps_));
}

// ============================================================================
// ClimatologyAtmosphereResource
// ============================================================================

ClimatologyAtmosphereResource::ClimatologyAtmosphereResource() {
    seed_profile_.instantiate();
    default_tornado_profile_.instantiate();
    generate_initial_climate();
}

void ClimatologyAtmosphereResource::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_seed_profile", "profile"), &ClimatologyAtmosphereResource::set_seed_profile);
    ClassDB::bind_method(D_METHOD("get_seed_profile"), &ClimatologyAtmosphereResource::get_seed_profile);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "seed_profile", PROPERTY_HINT_RESOURCE_TYPE, "ClimatologySeedProfileResource"), "set_seed_profile", "get_seed_profile");

    ClassDB::bind_method(D_METHOD("set_default_tornado_profile", "profile"), &ClimatologyAtmosphereResource::set_default_tornado_profile);
    ClassDB::bind_method(D_METHOD("get_default_tornado_profile"), &ClimatologyAtmosphereResource::get_default_tornado_profile);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "default_tornado_profile", PROPERTY_HINT_RESOURCE_TYPE, "TornadoVortexProfileResource"), "set_default_tornado_profile", "get_default_tornado_profile");

    ClassDB::bind_method(D_METHOD("set_default_synoptic_dt_sec", "dt"), &ClimatologyAtmosphereResource::set_default_synoptic_dt_sec);
    ClassDB::bind_method(D_METHOD("get_default_synoptic_dt_sec"), &ClimatologyAtmosphereResource::get_default_synoptic_dt_sec);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "default_synoptic_dt_sec", PROPERTY_HINT_RANGE, "1.0,600.0,1.0"), "set_default_synoptic_dt_sec", "get_default_synoptic_dt_sec");

    ClassDB::bind_method(D_METHOD("set_default_vortex_dt_sec", "dt"), &ClimatologyAtmosphereResource::set_default_vortex_dt_sec);
    ClassDB::bind_method(D_METHOD("get_default_vortex_dt_sec"), &ClimatologyAtmosphereResource::get_default_vortex_dt_sec);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "default_vortex_dt_sec", PROPERTY_HINT_RANGE, "0.5,120.0,0.5"), "set_default_vortex_dt_sec", "get_default_vortex_dt_sec");

    ClassDB::bind_method(D_METHOD("set_active_stage_mask", "mask"), &ClimatologyAtmosphereResource::set_active_stage_mask);
    ClassDB::bind_method(D_METHOD("get_active_stage_mask"), &ClimatologyAtmosphereResource::get_active_stage_mask);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "active_stage_mask"), "set_active_stage_mask", "get_active_stage_mask");

    ClassDB::bind_method(D_METHOD("get_simulation_step_counter"), &ClimatologyAtmosphereResource::get_simulation_step_counter);
    ClassDB::bind_method(D_METHOD("set_terrain_elevation_grid", "elevations_m"), &ClimatologyAtmosphereResource::set_terrain_elevation_grid);
    ClassDB::bind_method(D_METHOD("generate_initial_climate"), &ClimatologyAtmosphereResource::generate_initial_climate);
    ClassDB::bind_method(D_METHOD("step_simulation", "steps", "dt_synoptic_sec", "dt_vortex_sec"), &ClimatologyAtmosphereResource::step_simulation, DEFVAL(1), DEFVAL(-1.0f), DEFVAL(-1.0f));
    ClassDB::bind_method(D_METHOD("spawn_tornado_at_cell", "x", "y", "max_wind_mps", "core_radius_m"), &ClimatologyAtmosphereResource::spawn_tornado_at_cell, DEFVAL(-1.0f), DEFVAL(-1.0f));
    ClassDB::bind_method(D_METHOD("inspect_cell_sounding", "x", "y"), &ClimatologyAtmosphereResource::inspect_cell_sounding);
    ClassDB::bind_method(D_METHOD("render_map_layer_image", "layer"), &ClimatologyAtmosphereResource::render_map_layer_image);
    ClassDB::bind_method(D_METHOD("get_domain_telemetry"), &ClimatologyAtmosphereResource::get_domain_telemetry);
}

void ClimatologyAtmosphereResource::set_seed_profile(const godot::Ref<ClimatologySeedProfileResource>& p_profile) {
    seed_profile_ = p_profile;
    if (seed_profile_.is_null()) {
        seed_profile_.instantiate();
    }
    emit_changed();
}
godot::Ref<ClimatologySeedProfileResource> ClimatologyAtmosphereResource::get_seed_profile() const {
    return seed_profile_;
}

void ClimatologyAtmosphereResource::set_default_tornado_profile(const godot::Ref<TornadoVortexProfileResource>& p_profile) {
    default_tornado_profile_ = p_profile;
    if (default_tornado_profile_.is_null()) {
        default_tornado_profile_.instantiate();
    }
    emit_changed();
}
godot::Ref<TornadoVortexProfileResource> ClimatologyAtmosphereResource::get_default_tornado_profile() const {
    return default_tornado_profile_;
}

void ClimatologyAtmosphereResource::set_default_synoptic_dt_sec(float p_dt) {
    default_synoptic_dt_sec_ = std::max(0.5f, p_dt);
    emit_changed();
}
float ClimatologyAtmosphereResource::get_default_synoptic_dt_sec() const { return default_synoptic_dt_sec_; }

void ClimatologyAtmosphereResource::set_default_vortex_dt_sec(float p_dt) {
    default_vortex_dt_sec_ = std::max(0.1f, p_dt);
    emit_changed();
}
float ClimatologyAtmosphereResource::get_default_vortex_dt_sec() const { return default_vortex_dt_sec_; }

void ClimatologyAtmosphereResource::set_active_stage_mask(int p_mask) {
    active_stage_mask_ = p_mask;
    emit_changed();
}
int ClimatologyAtmosphereResource::get_active_stage_mask() const { return active_stage_mask_; }

void ClimatologyAtmosphereResource::set_terrain_elevation_grid(const godot::PackedFloat32Array& p_elevations_m) {
    custom_terrain_elevations_m_.resize(p_elevations_m.size());
    for (int i = 0; i < p_elevations_m.size(); ++i) {
        custom_terrain_elevations_m_[static_cast<size_t>(i)] = p_elevations_m[i];
    }
    generate_initial_climate();
}

void ClimatologyAtmosphereResource::_ensure_buffers_allocated() {
    if (seed_profile_.is_null()) {
        seed_profile_.instantiate();
    }
    if (default_tornado_profile_.is_null()) {
        default_tornado_profile_.instantiate();
    }
    const core::ClimatologySeedProfilePOD seed = seed_profile_->to_pod();
    const size_t cell_count = static_cast<size_t>(seed.grid_width) * static_cast<size_t>(seed.grid_height);
    const size_t vort_count = std::max<size_t>(1, seed.max_vortices);

    pod_atmo_.resize(cell_count);
    pod_moisture_.resize(cell_count);
    pod_vortices_.resize(vort_count);
    pod_hazards_.resize(cell_count);
}

void ClimatologyAtmosphereResource::generate_initial_climate() {
    _ensure_buffers_allocated();
    const core::ClimatologySeedProfilePOD seed = seed_profile_->to_pod();
    core::ClimatologyDOD::initialize_atmosphere_from_seed(
        seed,
        custom_terrain_elevations_m_,
        pod_atmo_,
        pod_moisture_,
        pod_vortices_);
    core::ClimatologyKernelMath::evaluate_severe_weather_hazards(
        seed,
        pod_atmo_,
        pod_moisture_,
        pod_hazards_);
    simulation_step_counter_ = 0;
    emit_changed();
}

void ClimatologyAtmosphereResource::step_simulation(int p_steps, float p_dt_synoptic_sec, float p_dt_vortex_sec) {
    _ensure_buffers_allocated();
    const core::ClimatologySeedProfilePOD seed = seed_profile_->to_pod();
    const float dt_syn = (p_dt_synoptic_sec > 0.0f) ? p_dt_synoptic_sec : default_synoptic_dt_sec_;
    const float dt_vor = (p_dt_vortex_sec > 0.0f) ? p_dt_vortex_sec : default_vortex_dt_sec_;
    const int steps = std::max(1, p_steps);

    for (int i = 0; i < steps; ++i) {
        core::ClimatologyDOD::execute_step(
            seed,
            pod_atmo_,
            pod_moisture_,
            pod_vortices_,
            pod_hazards_,
            dt_syn,
            dt_vor,
            static_cast<uint32_t>(active_stage_mask_));
        ++simulation_step_counter_;
    }
    emit_changed();
}

bool ClimatologyAtmosphereResource::spawn_tornado_at_cell(int p_x, int p_y, float p_max_wind_mps, float p_core_radius_m) {
    _ensure_buffers_allocated();
    const core::ClimatologySeedProfilePOD seed = seed_profile_->to_pod();
    const uint32_t gx = static_cast<uint32_t>(std::clamp(p_x, 0, seed_profile_->get_grid_width() - 1));
    const uint32_t gy = static_cast<uint32_t>(std::clamp(p_y, 0, seed_profile_->get_grid_height() - 1));
    const float v_max = (p_max_wind_mps > 0.0f) ? p_max_wind_mps : default_tornado_profile_->get_max_tangential_wind_mps();
    const float r_core = (p_core_radius_m > 0.0f) ? p_core_radius_m : default_tornado_profile_->get_core_radius_m();

    const bool ok = core::ClimatologyKernelMath::spawn_supercell_tornado_at_grid(
        seed, gx, gy, v_max, r_core, pod_vortices_);
    if (ok) {
        core::ClimatologyKernelMath::step_convective_cape_and_tornado_vortices(
            seed, 1.0f, pod_atmo_, pod_moisture_, pod_vortices_);
        core::ClimatologyKernelMath::evaluate_severe_weather_hazards(
            seed, pod_atmo_, pod_moisture_, pod_hazards_);
        emit_changed();
    }
    return ok;
}

godot::Dictionary ClimatologyAtmosphereResource::inspect_cell_sounding(int p_x, int p_y) const {
    godot::Dictionary d;
    if (seed_profile_.is_null() || pod_atmo_.empty()) {
        return d;
    }
    const int w = seed_profile_->get_grid_width();
    const int h = seed_profile_->get_grid_height();
    const int cx = std::clamp(p_x, 0, w - 1);
    const int cy = std::clamp(p_y, 0, h - 1);
    const size_t idx = static_cast<size_t>(cy * w + cx);
    if (idx >= pod_atmo_.size()) {
        return d;
    }

    const auto& atmo = pod_atmo_[idx];
    const auto& moist = pod_moisture_[idx];
    const auto& haz = pod_hazards_[idx];

    static const char* kKoppenNames[10] = {
        "Af (Tropical Rainforest)",
        "Aw (Tropical Savanna)",
        "BWh (Hot Arid Desert)",
        "BSk (Cold Semi-Arid Steppe)",
        "Csa (Mediterranean)",
        "Cfa (Humid Subtropical)",
        "Cfb (Oceanic Temperate)",
        "Dfc (Subarctic Boreal)",
        "ET (Alpine Tundra)",
        "EF (Polar Ice Cap)"
    };

    d["grid_x"] = cx;
    d["grid_y"] = cy;
    d["surface_elevation_m"] = atmo.surface_elevation_m;
    d["surface_air_temp_c"] = atmo.surface_air_temperature_k - 273.15f;
    d["potential_temp_k"] = atmo.potential_temperature_k;
    d["upper_troposphere_temp_c"] = atmo.upper_troposphere_temp_k - 273.15f;
    d["boundary_pressure_hpa"] = atmo.boundary_pressure_hpa;
    d["wind_u_mps"] = atmo.wind_velocity_u_mps;
    d["wind_v_mps"] = atmo.wind_velocity_v_mps;
    d["orographic_lift_mps"] = atmo.orographic_lift_mps;
    d["vertical_updraft_w_mps"] = atmo.vertical_updraft_w_mps;
    d["beaufort_scale"] = static_cast<int>(atmo.beaufort_scale);
    d["koppen_zone"] = static_cast<int>(atmo.koppen_zone);
    d["koppen_name"] = godot::String(kKoppenNames[static_cast<int>(atmo.koppen_zone) % 10]);

    d["relative_humidity_pct"] = moist.relative_humidity_01 * 100.0f;
    d["specific_humidity_g_kg"] = moist.specific_humidity_kg_kg * 1000.0f;
    d["cloud_liquid_water_kg_m2"] = moist.cloud_liquid_water_kg_m2;
    d["lifting_condensation_lvl_m"] = moist.lifting_condensation_lvl_m;
    d["convective_cape_j_kg"] = moist.convective_cape_j_kg;
    d["storm_rel_helicity_m2_s2"] = moist.storm_rel_helicity_m2_s2;
    d["rainfall_rate_mm_hr"] = moist.rainfall_rate_mm_hr;
    d["snowfall_rate_mm_hr"] = moist.snowfall_rate_mm_hr;
    d["snowpack_mm"] = moist.snowpack_water_equiv_mm;
    d["vortex_tangential_wind_mps"] = moist.vortex_tangential_wind_mps;
    d["local_ef_tier"] = static_cast<int>(moist.local_ef_tier);
    d["hydrometeor_phase"] = static_cast<int>(moist.hydrometeor_phase);

    d["tornado_wind_gust_mps"] = haz.tornado_wind_gust_mps;
    d["barometric_suction_pa"] = haz.barometric_suction_pa;
    d["lightning_strike_prob"] = haz.lightning_strike_prob;
    return d;
}

godot::Ref<godot::Image> ClimatologyAtmosphereResource::render_map_layer_image(int p_layer) const {
    if (seed_profile_.is_null() || pod_atmo_.empty()) {
        return godot::Ref<godot::Image>();
    }
    const int w = seed_profile_->get_grid_width();
    const int h = seed_profile_->get_grid_height();
    godot::Ref<godot::Image> img = godot::Image::create_empty(w, h, false, godot::Image::FORMAT_RGBA8);

    const auto layer_enum = static_cast<core::ClimatologyMapLayer>(p_layer);

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const size_t idx = static_cast<size_t>(y * w + x);
            const auto& atmo = pod_atmo_[idx];
            const auto& moist = pod_moisture_[idx];

            godot::Color c(0.12f, 0.16f, 0.22f, 1.0f);
            switch (layer_enum) {
                case core::ClimatologyMapLayer::POTENTIAL_TEMPERATURE_KELVIN: {
                    const float t_norm = std::clamp((atmo.surface_air_temperature_k - 258.15f) / 48.0f, 0.0f, 1.0f);
                    c = godot::Color::from_hsv((1.0f - t_norm) * 0.66f, 0.78f, 0.88f);
                    break;
                }
                case core::ClimatologyMapLayer::SURFACE_PRESSURE_HPA: {
                    const float p_sl = atmo.boundary_pressure_hpa * std::exp(atmo.surface_elevation_m / 8430.0f);
                    const float p_norm = std::clamp((p_sl - 975.0f) / 65.0f, 0.0f, 1.0f);
                    c = godot::Color(0.25f + 0.65f * p_norm, 0.22f + 0.45f * (1.0f - std::abs(p_norm - 0.5f) * 2.0f), 0.90f - 0.65f * p_norm, 1.0f);
                    break;
                }
                case core::ClimatologyMapLayer::GEOSTROPHIC_AND_OROGRAPHIC_WIND: {
                    const float spd = std::hypot(atmo.wind_velocity_u_mps, atmo.wind_velocity_v_mps);
                    const float s_norm = std::clamp(spd / 28.0f, 0.0f, 1.0f);
                    const float oro = std::clamp(atmo.orographic_lift_mps * 0.25f, -0.5f, 0.5f);
                    c = godot::Color(0.15f + 0.75f * s_norm + std::max(0.0f, oro), 0.32f + 0.55f * s_norm, 0.55f - std::min(0.0f, oro), 1.0f);
                    break;
                }
                case core::ClimatologyMapLayer::RELATIVE_HUMIDITY_AND_CLOUD_WATER: {
                    const float rh = std::clamp(moist.relative_humidity_01, 0.0f, 1.0f);
                    const float cld = std::clamp(moist.cloud_cover_fraction, 0.0f, 1.0f);
                    c = godot::Color(0.18f + 0.72f * cld, 0.35f + 0.55f * rh, 0.45f + 0.52f * std::max(rh, cld), 1.0f);
                    break;
                }
                case core::ClimatologyMapLayer::PRECIPITATION_AND_SNOWPACK: {
                    const float rain = std::clamp(moist.rainfall_rate_mm_hr / 18.0f, 0.0f, 1.0f);
                    const float snow = std::clamp((moist.snowfall_rate_mm_hr + moist.snowpack_water_equiv_mm * 0.25f) / 15.0f, 0.0f, 1.0f);
                    c = godot::Color(0.14f + 0.82f * snow, 0.28f + 0.65f * std::max(rain, snow), 0.35f + 0.62f * std::max(rain, snow), 1.0f);
                    break;
                }
                case core::ClimatologyMapLayer::CONVECTIVE_CAPE_AND_HELICITY: {
                    const float cape_norm = std::clamp(moist.convective_cape_j_kg / 3500.0f, 0.0f, 1.0f);
                    const float srh_norm = std::clamp(moist.storm_rel_helicity_m2_s2 / 400.0f, 0.0f, 1.0f);
                    c = godot::Color(0.20f + 0.80f * cape_norm, 0.15f + 0.55f * srh_norm, 0.30f + 0.60f * (1.0f - cape_norm), 1.0f);
                    break;
                }
                case core::ClimatologyMapLayer::SUPERCELL_TORNADO_VORTICITY: {
                    const float v_norm = std::clamp(moist.vortex_tangential_wind_mps / 85.0f, 0.0f, 1.0f);
                    c = godot::Color(0.14f + 0.86f * v_norm, 0.14f + 0.25f * (1.0f - v_norm), 0.26f + 0.70f * v_norm, 1.0f);
                    break;
                }
                case core::ClimatologyMapLayer::KOPPEN_CLIMATE_CLASSIFICATION: {
                    static const godot::Color kKoppenColors[10] = {
                        godot::Color(0.05f, 0.42f, 0.18f, 1.0f), // Af
                        godot::Color(0.32f, 0.68f, 0.24f, 1.0f), // Aw
                        godot::Color(0.90f, 0.68f, 0.26f, 1.0f), // BWh
                        godot::Color(0.78f, 0.62f, 0.38f, 1.0f), // BSk
                        godot::Color(0.86f, 0.82f, 0.24f, 1.0f), // Csa
                        godot::Color(0.45f, 0.78f, 0.42f, 1.0f), // Cfa
                        godot::Color(0.28f, 0.62f, 0.52f, 1.0f), // Cfb
                        godot::Color(0.24f, 0.46f, 0.68f, 1.0f), // Dfc
                        godot::Color(0.62f, 0.70f, 0.78f, 1.0f), // ET
                        godot::Color(0.90f, 0.94f, 0.98f, 1.0f)  // EF
                    };
                    c = kKoppenColors[static_cast<int>(atmo.koppen_zone) % 10];
                    break;
                }
                case core::ClimatologyMapLayer::SURFACE_RADIATIVE_ENERGY_BUDGET: {
                    const float net_rad = atmo.shortwave_insolation_w_m2 - atmo.longwave_outgoing_w_m2;
                    const float r_norm = std::clamp((net_rad + 150.0f) / 850.0f, 0.0f, 1.0f);
                    c = godot::Color(0.18f + 0.80f * r_norm, 0.22f + 0.62f * r_norm, 0.48f * (1.0f - r_norm), 1.0f);
                    break;
                }
                default:
                    break;
            }
            img->set_pixel(x, y, c);
        }
    }
    return img;
}

godot::Dictionary ClimatologyAtmosphereResource::get_domain_telemetry() const {
    godot::Dictionary d;
    if (pod_atmo_.empty()) {
        return d;
    }
    float mean_temp_c = 0.0f;
    float max_gust_mps = 0.0f;
    float max_cape = 0.0f;
    int active_tornadoes = 0;

    for (size_t i = 0; i < pod_atmo_.size(); ++i) {
        mean_temp_c += (pod_atmo_[i].surface_air_temperature_k - 273.15f);
        max_gust_mps = std::max(max_gust_mps, pod_hazards_[i].tornado_wind_gust_mps);
        max_cape = std::max(max_cape, pod_moisture_[i].convective_cape_j_kg);
    }
    mean_temp_c /= static_cast<float>(pod_atmo_.size());

    for (const auto& v : pod_vortices_) {
        if (v.is_active != 0) {
            ++active_tornadoes;
        }
    }

    d["cell_count"] = static_cast<int>(pod_atmo_.size());
    d["simulation_steps"] = static_cast<int>(simulation_step_counter_);
    d["mean_surface_temp_c"] = mean_temp_c;
    d["max_wind_gust_mps"] = max_gust_mps;
    d["max_convective_cape_j_kg"] = max_cape;
    d["active_tornado_vortices"] = active_tornadoes;
    return d;
}

// ============================================================================
// ClimatologyWeatherNode3D
// ============================================================================

ClimatologyWeatherNode3D::ClimatologyWeatherNode3D() {
    atmosphere_resource_.instantiate();
}

void ClimatologyWeatherNode3D::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_atmosphere_resource", "resource"), &ClimatologyWeatherNode3D::set_atmosphere_resource);
    ClassDB::bind_method(D_METHOD("get_atmosphere_resource"), &ClimatologyWeatherNode3D::get_atmosphere_resource);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "atmosphere_resource", PROPERTY_HINT_RESOURCE_TYPE, "ClimatologyAtmosphereResource"), "set_atmosphere_resource", "get_atmosphere_resource");

    ClassDB::bind_method(D_METHOD("set_auto_generate_on_ready", "auto_generate"), &ClimatologyWeatherNode3D::set_auto_generate_on_ready);
    ClassDB::bind_method(D_METHOD("get_auto_generate_on_ready"), &ClimatologyWeatherNode3D::get_auto_generate_on_ready);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_generate_on_ready"), "set_auto_generate_on_ready", "get_auto_generate_on_ready");

    ClassDB::bind_method(D_METHOD("world_to_grid", "world_pos"), &ClimatologyWeatherNode3D::world_to_grid);
    ClassDB::bind_method(D_METHOD("sample_wind_velocity_at_world", "world_pos"), &ClimatologyWeatherNode3D::sample_wind_velocity_at_world);
    ClassDB::bind_method(D_METHOD("sample_sounding_at_world", "world_pos"), &ClimatologyWeatherNode3D::sample_sounding_at_world);
}

void ClimatologyWeatherNode3D::_ready() {
    if (auto_generate_on_ready_ && atmosphere_resource_.is_valid() && atmosphere_resource_->get_atmo_const_span().empty()) {
        atmosphere_resource_->generate_initial_climate();
    }
}

void ClimatologyWeatherNode3D::set_atmosphere_resource(const godot::Ref<ClimatologyAtmosphereResource>& p_resource) {
    atmosphere_resource_ = p_resource;
}
godot::Ref<ClimatologyAtmosphereResource> ClimatologyWeatherNode3D::get_atmosphere_resource() const {
    return atmosphere_resource_;
}

void ClimatologyWeatherNode3D::set_auto_generate_on_ready(bool p_auto) {
    auto_generate_on_ready_ = p_auto;
}
bool ClimatologyWeatherNode3D::get_auto_generate_on_ready() const {
    return auto_generate_on_ready_;
}

godot::Vector2i ClimatologyWeatherNode3D::world_to_grid(const godot::Vector3& p_world_pos) const {
    if (atmosphere_resource_.is_null() || atmosphere_resource_->get_seed_profile().is_null()) {
        return godot::Vector2i(0, 0);
    }
    const godot::Vector3 local = to_local(p_world_pos);
    const auto seed = atmosphere_resource_->get_seed_profile();
    const float cell_m = std::max(1.0f, seed->get_cell_size_meters());
    const int gx = std::clamp(static_cast<int>(std::floor(local.x / cell_m)), 0, seed->get_grid_width() - 1);
    const int gy = std::clamp(static_cast<int>(std::floor(local.z / cell_m)), 0, seed->get_grid_height() - 1);
    return godot::Vector2i(gx, gy);
}

godot::Vector3 ClimatologyWeatherNode3D::sample_wind_velocity_at_world(const godot::Vector3& p_world_pos) const {
    if (atmosphere_resource_.is_null()) {
        return godot::Vector3();
    }
    const godot::Vector2i g = world_to_grid(p_world_pos);
    const godot::Dictionary s = atmosphere_resource_->inspect_cell_sounding(g.x, g.y);
    return godot::Vector3(
        static_cast<float>(s.get("wind_u_mps", 0.0f)),
        static_cast<float>(s.get("vertical_updraft_w_mps", 0.0f)),
        static_cast<float>(s.get("wind_v_mps", 0.0f)));
}

godot::Dictionary ClimatologyWeatherNode3D::sample_sounding_at_world(const godot::Vector3& p_world_pos) const {
    if (atmosphere_resource_.is_null()) {
        return godot::Dictionary();
    }
    const godot::Vector2i g = world_to_grid(p_world_pos);
    return atmosphere_resource_->inspect_cell_sounding(g.x, g.y);
}

} // namespace ideam::godot_ext
