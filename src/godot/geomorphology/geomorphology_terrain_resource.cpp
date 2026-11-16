#include "godot/geomorphology/geomorphology_terrain_resource.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <algorithm>
#include <cmath>

namespace ideam::godot_ext {

// ============================================================================
// GeomorphologySeedProfileResource
// ============================================================================

void GeomorphologySeedProfileResource::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_master_seed", "seed"), &GeomorphologySeedProfileResource::set_master_seed);
    ClassDB::bind_method(D_METHOD("get_master_seed"), &GeomorphologySeedProfileResource::get_master_seed);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "master_seed"), "set_master_seed", "get_master_seed");

    ClassDB::bind_method(D_METHOD("set_grid_width", "width"), &GeomorphologySeedProfileResource::set_grid_width);
    ClassDB::bind_method(D_METHOD("get_grid_width"), &GeomorphologySeedProfileResource::get_grid_width);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "grid_width", PROPERTY_HINT_RANGE, "8,512,1"), "set_grid_width", "get_grid_width");

    ClassDB::bind_method(D_METHOD("set_grid_height", "height"), &GeomorphologySeedProfileResource::set_grid_height);
    ClassDB::bind_method(D_METHOD("get_grid_height"), &GeomorphologySeedProfileResource::get_grid_height);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "grid_height", PROPERTY_HINT_RANGE, "8,512,1"), "set_grid_height", "get_grid_height");

    ClassDB::bind_method(D_METHOD("set_plate_count", "count"), &GeomorphologySeedProfileResource::set_plate_count);
    ClassDB::bind_method(D_METHOD("get_plate_count"), &GeomorphologySeedProfileResource::get_plate_count);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "plate_count", PROPERTY_HINT_RANGE, "2,32,1"), "set_plate_count", "get_plate_count");

    ClassDB::bind_method(D_METHOD("set_active_strata_per_column", "strata"), &GeomorphologySeedProfileResource::set_active_strata_per_column);
    ClassDB::bind_method(D_METHOD("get_active_strata_per_column"), &GeomorphologySeedProfileResource::get_active_strata_per_column);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "active_strata_per_column", PROPERTY_HINT_RANGE, "1,6,1"), "set_active_strata_per_column", "get_active_strata_per_column");

    ClassDB::bind_method(D_METHOD("set_cell_size_meters", "size"), &GeomorphologySeedProfileResource::set_cell_size_meters);
    ClassDB::bind_method(D_METHOD("get_cell_size_meters"), &GeomorphologySeedProfileResource::get_cell_size_meters);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "cell_size_meters", PROPERTY_HINT_RANGE, "1.0,1000.0,0.5"), "set_cell_size_meters", "get_cell_size_meters");

    ClassDB::bind_method(D_METHOD("set_base_crust_thickness_m", "thickness"), &GeomorphologySeedProfileResource::set_base_crust_thickness_m);
    ClassDB::bind_method(D_METHOD("get_base_crust_thickness_m"), &GeomorphologySeedProfileResource::get_base_crust_thickness_m);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "base_crust_thickness_m", PROPERTY_HINT_RANGE, "100.0,10000.0,10.0"), "set_base_crust_thickness_m", "get_base_crust_thickness_m");

    ClassDB::bind_method(D_METHOD("set_max_orogenic_uplift_m", "uplift"), &GeomorphologySeedProfileResource::set_max_orogenic_uplift_m);
    ClassDB::bind_method(D_METHOD("get_max_orogenic_uplift_m"), &GeomorphologySeedProfileResource::get_max_orogenic_uplift_m);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_orogenic_uplift_m", PROPERTY_HINT_RANGE, "10.0,5000.0,10.0"), "set_max_orogenic_uplift_m", "get_max_orogenic_uplift_m");

    ClassDB::bind_method(D_METHOD("set_sea_level_elevation_m", "sea_level"), &GeomorphologySeedProfileResource::set_sea_level_elevation_m);
    ClassDB::bind_method(D_METHOD("get_sea_level_elevation_m"), &GeomorphologySeedProfileResource::get_sea_level_elevation_m);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "sea_level_elevation_m", PROPERTY_HINT_RANGE, "0.0,5000.0,5.0"), "set_sea_level_elevation_m", "get_sea_level_elevation_m");

    ClassDB::bind_method(D_METHOD("set_tectonic_noise_frequency", "freq"), &GeomorphologySeedProfileResource::set_tectonic_noise_frequency);
    ClassDB::bind_method(D_METHOD("get_tectonic_noise_frequency"), &GeomorphologySeedProfileResource::get_tectonic_noise_frequency);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "tectonic_noise_frequency", PROPERTY_HINT_RANGE, "0.001,0.5,0.001"), "set_tectonic_noise_frequency", "get_tectonic_noise_frequency");

    ClassDB::bind_method(D_METHOD("set_hydro_gravity_mps2", "gravity"), &GeomorphologySeedProfileResource::set_hydro_gravity_mps2);
    ClassDB::bind_method(D_METHOD("get_hydro_gravity_mps2"), &GeomorphologySeedProfileResource::get_hydro_gravity_mps2);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "hydro_gravity_mps2", PROPERTY_HINT_RANGE, "0.5,30.0,0.05"), "set_hydro_gravity_mps2", "get_hydro_gravity_mps2");

    ClassDB::bind_method(D_METHOD("set_karst_dissolution_scale", "scale"), &GeomorphologySeedProfileResource::set_karst_dissolution_scale);
    ClassDB::bind_method(D_METHOD("get_karst_dissolution_scale"), &GeomorphologySeedProfileResource::get_karst_dissolution_scale);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "karst_dissolution_scale", PROPERTY_HINT_RANGE, "0.0,10.0,0.05"), "set_karst_dissolution_scale", "get_karst_dissolution_scale");

    ClassDB::bind_method(D_METHOD("set_pedogenesis_rate_scale", "scale"), &GeomorphologySeedProfileResource::set_pedogenesis_rate_scale);
    ClassDB::bind_method(D_METHOD("get_pedogenesis_rate_scale"), &GeomorphologySeedProfileResource::get_pedogenesis_rate_scale);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "pedogenesis_rate_scale", PROPERTY_HINT_RANGE, "0.0,10.0,0.05"), "set_pedogenesis_rate_scale", "get_pedogenesis_rate_scale");

    ClassDB::bind_method(D_METHOD("get_tectonic_seed"), &GeomorphologySeedProfileResource::get_tectonic_seed);
    ClassDB::bind_method(D_METHOD("get_stratigraphy_seed"), &GeomorphologySeedProfileResource::get_stratigraphy_seed);
    ClassDB::bind_method(D_METHOD("get_hydrology_seed"), &GeomorphologySeedProfileResource::get_hydrology_seed);
    ClassDB::bind_method(D_METHOD("get_karst_seed"), &GeomorphologySeedProfileResource::get_karst_seed);
    ClassDB::bind_method(D_METHOD("get_resource_vein_seed"), &GeomorphologySeedProfileResource::get_resource_vein_seed);
}

void GeomorphologySeedProfileResource::set_master_seed(int64_t p_seed) {
    master_seed_ = p_seed;
    emit_changed();
}
int64_t GeomorphologySeedProfileResource::get_master_seed() const { return master_seed_; }

void GeomorphologySeedProfileResource::set_grid_width(int p_width) {
    grid_width_ = std::clamp(p_width, 8, 512);
    emit_changed();
}
int GeomorphologySeedProfileResource::get_grid_width() const { return grid_width_; }

void GeomorphologySeedProfileResource::set_grid_height(int p_height) {
    grid_height_ = std::clamp(p_height, 8, 512);
    emit_changed();
}
int GeomorphologySeedProfileResource::get_grid_height() const { return grid_height_; }

void GeomorphologySeedProfileResource::set_plate_count(int p_count) {
    plate_count_ = std::clamp(p_count, 2, 32);
    emit_changed();
}
int GeomorphologySeedProfileResource::get_plate_count() const { return plate_count_; }

void GeomorphologySeedProfileResource::set_active_strata_per_column(int p_strata) {
    active_strata_per_column_ = std::clamp(p_strata, 1, static_cast<int>(core::MAX_STRATA_PER_COLUMN));
    emit_changed();
}
int GeomorphologySeedProfileResource::get_active_strata_per_column() const { return active_strata_per_column_; }

void GeomorphologySeedProfileResource::set_cell_size_meters(float p_size) {
    cell_size_meters_ = std::max(1.0f, p_size);
    emit_changed();
}
float GeomorphologySeedProfileResource::get_cell_size_meters() const { return cell_size_meters_; }

void GeomorphologySeedProfileResource::set_base_crust_thickness_m(float p_thickness) {
    base_crust_thickness_m_ = std::max(50.0f, p_thickness);
    emit_changed();
}
float GeomorphologySeedProfileResource::get_base_crust_thickness_m() const { return base_crust_thickness_m_; }

void GeomorphologySeedProfileResource::set_max_orogenic_uplift_m(float p_uplift) {
    max_orogenic_uplift_m_ = std::max(10.0f, p_uplift);
    emit_changed();
}
float GeomorphologySeedProfileResource::get_max_orogenic_uplift_m() const { return max_orogenic_uplift_m_; }

void GeomorphologySeedProfileResource::set_sea_level_elevation_m(float p_sea_level) {
    sea_level_elevation_m_ = p_sea_level;
    emit_changed();
}
float GeomorphologySeedProfileResource::get_sea_level_elevation_m() const { return sea_level_elevation_m_; }

void GeomorphologySeedProfileResource::set_tectonic_noise_frequency(float p_freq) {
    tectonic_noise_frequency_ = std::clamp(p_freq, 0.001f, 0.5f);
    emit_changed();
}
float GeomorphologySeedProfileResource::get_tectonic_noise_frequency() const { return tectonic_noise_frequency_; }

void GeomorphologySeedProfileResource::set_hydro_gravity_mps2(float p_gravity) {
    hydro_gravity_mps2_ = std::max(0.1f, p_gravity);
    emit_changed();
}
float GeomorphologySeedProfileResource::get_hydro_gravity_mps2() const { return hydro_gravity_mps2_; }

void GeomorphologySeedProfileResource::set_karst_dissolution_scale(float p_scale) {
    karst_dissolution_scale_ = std::max(0.0f, p_scale);
    emit_changed();
}
float GeomorphologySeedProfileResource::get_karst_dissolution_scale() const { return karst_dissolution_scale_; }

void GeomorphologySeedProfileResource::set_pedogenesis_rate_scale(float p_scale) {
    pedogenesis_rate_scale_ = std::max(0.0f, p_scale);
    emit_changed();
}
float GeomorphologySeedProfileResource::get_pedogenesis_rate_scale() const { return pedogenesis_rate_scale_; }

int64_t GeomorphologySeedProfileResource::get_tectonic_seed() const {
    return static_cast<int64_t>(to_pod().tectonic_seed);
}
int64_t GeomorphologySeedProfileResource::get_stratigraphy_seed() const {
    return static_cast<int64_t>(to_pod().stratigraphy_seed);
}
int64_t GeomorphologySeedProfileResource::get_hydrology_seed() const {
    return static_cast<int64_t>(to_pod().hydrology_seed);
}
int64_t GeomorphologySeedProfileResource::get_karst_seed() const {
    return static_cast<int64_t>(to_pod().karst_seed);
}
int64_t GeomorphologySeedProfileResource::get_resource_vein_seed() const {
    return static_cast<int64_t>(to_pod().resource_vein_seed);
}

core::GeomorphologySeedProfilePOD GeomorphologySeedProfileResource::to_pod() const noexcept {
    core::GeomorphologySeedProfilePOD pod = core::GeomorphologyKernelMath::make_seed_profile(
        static_cast<uint64_t>(master_seed_),
        static_cast<uint32_t>(grid_width_),
        static_cast<uint32_t>(grid_height_),
        static_cast<uint32_t>(plate_count_),
        cell_size_meters_);
    pod.active_strata_per_column = static_cast<uint32_t>(active_strata_per_column_);
    pod.base_crust_thickness_m = base_crust_thickness_m_;
    pod.max_orogenic_uplift_m = max_orogenic_uplift_m_;
    pod.sea_level_elevation_m = sea_level_elevation_m_;
    pod.tectonic_noise_frequency = tectonic_noise_frequency_;
    pod.hydro_gravity_mps2 = hydro_gravity_mps2_;
    pod.karst_dissolution_scale = karst_dissolution_scale_;
    pod.pedogenesis_rate_scale = pedogenesis_rate_scale_;
    return pod;
}

void GeomorphologySeedProfileResource::from_pod(const core::GeomorphologySeedProfilePOD& pod) {
    master_seed_ = static_cast<int64_t>(pod.master_seed);
    grid_width_ = static_cast<int>(pod.grid_width);
    grid_height_ = static_cast<int>(pod.grid_height);
    plate_count_ = static_cast<int>(pod.plate_count);
    active_strata_per_column_ = static_cast<int>(pod.active_strata_per_column);
    cell_size_meters_ = pod.cell_size_meters;
    base_crust_thickness_m_ = pod.base_crust_thickness_m;
    max_orogenic_uplift_m_ = pod.max_orogenic_uplift_m;
    sea_level_elevation_m_ = pod.sea_level_elevation_m;
    tectonic_noise_frequency_ = pod.tectonic_noise_frequency;
    hydro_gravity_mps2_ = pod.hydro_gravity_mps2;
    karst_dissolution_scale_ = pod.karst_dissolution_scale;
    pedogenesis_rate_scale_ = pod.pedogenesis_rate_scale;
    emit_changed();
}

// ============================================================================
// LithologyStratumResource
// ============================================================================

void LithologyStratumResource::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_lithology_id", "id"), &LithologyStratumResource::set_lithology_id);
    ClassDB::bind_method(D_METHOD("get_lithology_id"), &LithologyStratumResource::get_lithology_id);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "lithology_id"), "set_lithology_id", "get_lithology_id");

    ClassDB::bind_method(D_METHOD("set_genesis_class", "genesis_class"), &LithologyStratumResource::set_genesis_class);
    ClassDB::bind_method(D_METHOD("get_genesis_class"), &LithologyStratumResource::get_genesis_class);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "genesis_class", PROPERTY_HINT_ENUM, "IgneousPlutonic,IgneousVolcanic,SedimentaryClastic,SedimentaryCarbonate,MetamorphicFoliated,MetamorphicNonFoliated,UnconsolidatedRegolith,PedogenicSoil"), "set_genesis_class", "get_genesis_class");

    ClassDB::bind_method(D_METHOD("set_archetype_name", "name"), &LithologyStratumResource::set_archetype_name);
    ClassDB::bind_method(D_METHOD("get_archetype_name"), &LithologyStratumResource::get_archetype_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "archetype_name"), "set_archetype_name", "get_archetype_name");

    ClassDB::bind_method(D_METHOD("set_density_kg_m3", "val"), &LithologyStratumResource::set_density_kg_m3);
    ClassDB::bind_method(D_METHOD("get_density_kg_m3"), &LithologyStratumResource::get_density_kg_m3);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "density_kg_m3"), "set_density_kg_m3", "get_density_kg_m3");

    ClassDB::bind_method(D_METHOD("set_hardness_mohs", "val"), &LithologyStratumResource::set_hardness_mohs);
    ClassDB::bind_method(D_METHOD("get_hardness_mohs"), &LithologyStratumResource::get_hardness_mohs);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "hardness_mohs"), "set_hardness_mohs", "get_hardness_mohs");

    ClassDB::bind_method(D_METHOD("set_fluvial_erodibility_k", "val"), &LithologyStratumResource::set_fluvial_erodibility_k);
    ClassDB::bind_method(D_METHOD("get_fluvial_erodibility_k"), &LithologyStratumResource::get_fluvial_erodibility_k);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "fluvial_erodibility_k"), "set_fluvial_erodibility_k", "get_fluvial_erodibility_k");

    ClassDB::bind_method(D_METHOD("set_talus_angle_of_repose_rad", "val"), &LithologyStratumResource::set_talus_angle_of_repose_rad);
    ClassDB::bind_method(D_METHOD("get_talus_angle_of_repose_rad"), &LithologyStratumResource::get_talus_angle_of_repose_rad);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "talus_angle_of_repose_rad"), "set_talus_angle_of_repose_rad", "get_talus_angle_of_repose_rad");

    ClassDB::bind_method(D_METHOD("set_thermal_weathering_rate", "val"), &LithologyStratumResource::set_thermal_weathering_rate);
    ClassDB::bind_method(D_METHOD("get_thermal_weathering_rate"), &LithologyStratumResource::get_thermal_weathering_rate);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "thermal_weathering_rate"), "set_thermal_weathering_rate", "get_thermal_weathering_rate");

    ClassDB::bind_method(D_METHOD("set_porosity", "val"), &LithologyStratumResource::set_porosity);
    ClassDB::bind_method(D_METHOD("get_porosity"), &LithologyStratumResource::get_porosity);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "porosity"), "set_porosity", "get_porosity");

    ClassDB::bind_method(D_METHOD("set_hydraulic_conductivity", "val"), &LithologyStratumResource::set_hydraulic_conductivity);
    ClassDB::bind_method(D_METHOD("get_hydraulic_conductivity"), &LithologyStratumResource::get_hydraulic_conductivity);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "hydraulic_conductivity"), "set_hydraulic_conductivity", "get_hydraulic_conductivity");

    ClassDB::bind_method(D_METHOD("set_carbonate_solubility", "val"), &LithologyStratumResource::set_carbonate_solubility);
    ClassDB::bind_method(D_METHOD("get_carbonate_solubility"), &LithologyStratumResource::get_carbonate_solubility);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "carbonate_solubility"), "set_carbonate_solubility", "get_carbonate_solubility");

    ClassDB::bind_method(D_METHOD("set_compressive_strength_mpa", "val"), &LithologyStratumResource::set_compressive_strength_mpa);
    ClassDB::bind_method(D_METHOD("get_compressive_strength_mpa"), &LithologyStratumResource::get_compressive_strength_mpa);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "compressive_strength_mpa"), "set_compressive_strength_mpa", "get_compressive_strength_mpa");

    ClassDB::bind_method(D_METHOD("set_sediment_grain_diameter_mm", "val"), &LithologyStratumResource::set_sediment_grain_diameter_mm);
    ClassDB::bind_method(D_METHOD("get_sediment_grain_diameter_mm"), &LithologyStratumResource::get_sediment_grain_diameter_mm);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "sediment_grain_diameter_mm"), "set_sediment_grain_diameter_mm", "get_sediment_grain_diameter_mm");

    ClassDB::bind_method(D_METHOD("set_pedogenesis_humus_retention", "val"), &LithologyStratumResource::set_pedogenesis_humus_retention);
    ClassDB::bind_method(D_METHOD("get_pedogenesis_humus_retention"), &LithologyStratumResource::get_pedogenesis_humus_retention);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "pedogenesis_humus_retention"), "set_pedogenesis_humus_retention", "get_pedogenesis_humus_retention");
}

void LithologyStratumResource::set_lithology_id(int p_id) { lithology_id_ = p_id; emit_changed(); }
int LithologyStratumResource::get_lithology_id() const { return lithology_id_; }

void LithologyStratumResource::set_genesis_class(int p_class) { genesis_class_ = p_class; emit_changed(); }
int LithologyStratumResource::get_genesis_class() const { return genesis_class_; }

void LithologyStratumResource::set_archetype_name(const godot::String& p_name) { archetype_name_ = p_name; emit_changed(); }
godot::String LithologyStratumResource::get_archetype_name() const { return archetype_name_; }

void LithologyStratumResource::set_density_kg_m3(float p_val) { density_kg_m3_ = p_val; emit_changed(); }
float LithologyStratumResource::get_density_kg_m3() const { return density_kg_m3_; }

void LithologyStratumResource::set_hardness_mohs(float p_val) { hardness_mohs_ = p_val; emit_changed(); }
float LithologyStratumResource::get_hardness_mohs() const { return hardness_mohs_; }

void LithologyStratumResource::set_fluvial_erodibility_k(float p_val) { fluvial_erodibility_k_ = p_val; emit_changed(); }
float LithologyStratumResource::get_fluvial_erodibility_k() const { return fluvial_erodibility_k_; }

void LithologyStratumResource::set_talus_angle_of_repose_rad(float p_val) { talus_angle_of_repose_rad_ = p_val; emit_changed(); }
float LithologyStratumResource::get_talus_angle_of_repose_rad() const { return talus_angle_of_repose_rad_; }

void LithologyStratumResource::set_thermal_weathering_rate(float p_val) { thermal_weathering_rate_ = p_val; emit_changed(); }
float LithologyStratumResource::get_thermal_weathering_rate() const { return thermal_weathering_rate_; }

void LithologyStratumResource::set_porosity(float p_val) { porosity_ = p_val; emit_changed(); }
float LithologyStratumResource::get_porosity() const { return porosity_; }

void LithologyStratumResource::set_hydraulic_conductivity(float p_val) { hydraulic_conductivity_ = p_val; emit_changed(); }
float LithologyStratumResource::get_hydraulic_conductivity() const { return hydraulic_conductivity_; }

void LithologyStratumResource::set_carbonate_solubility(float p_val) { carbonate_solubility_ = p_val; emit_changed(); }
float LithologyStratumResource::get_carbonate_solubility() const { return carbonate_solubility_; }

void LithologyStratumResource::set_compressive_strength_mpa(float p_val) { compressive_strength_mpa_ = p_val; emit_changed(); }
float LithologyStratumResource::get_compressive_strength_mpa() const { return compressive_strength_mpa_; }

void LithologyStratumResource::set_sediment_grain_diameter_mm(float p_val) { sediment_grain_diameter_mm_ = p_val; emit_changed(); }
float LithologyStratumResource::get_sediment_grain_diameter_mm() const { return sediment_grain_diameter_mm_; }

void LithologyStratumResource::set_pedogenesis_humus_retention(float p_val) { pedogenesis_humus_retention_ = p_val; emit_changed(); }
float LithologyStratumResource::get_pedogenesis_humus_retention() const { return pedogenesis_humus_retention_; }

core::LithologyArchetypePOD LithologyStratumResource::to_pod() const noexcept {
    core::LithologyArchetypePOD pod{};
    pod.lithology_id = static_cast<uint16_t>(lithology_id_);
    pod.genesis_class = static_cast<core::RockGenesisClass>(genesis_class_);
    pod.density_kg_m3 = density_kg_m3_;
    pod.hardness_mohs = hardness_mohs_;
    pod.fluvial_erodibility_k = fluvial_erodibility_k_;
    pod.talus_angle_of_repose_rad = talus_angle_of_repose_rad_;
    pod.thermal_weathering_rate = thermal_weathering_rate_;
    pod.porosity = porosity_;
    pod.hydraulic_conductivity = hydraulic_conductivity_;
    pod.carbonate_solubility = carbonate_solubility_;
    pod.compressive_strength_mpa = compressive_strength_mpa_;
    pod.sediment_grain_diameter_mm = sediment_grain_diameter_mm_;
    pod.pedogenesis_humus_retention = pedogenesis_humus_retention_;
    return pod;
}

void LithologyStratumResource::from_pod(const core::LithologyArchetypePOD& pod) {
    lithology_id_ = static_cast<int>(pod.lithology_id);
    genesis_class_ = static_cast<int>(pod.genesis_class);
    density_kg_m3_ = pod.density_kg_m3;
    hardness_mohs_ = pod.hardness_mohs;
    fluvial_erodibility_k_ = pod.fluvial_erodibility_k;
    talus_angle_of_repose_rad_ = pod.talus_angle_of_repose_rad;
    thermal_weathering_rate_ = pod.thermal_weathering_rate;
    porosity_ = pod.porosity;
    hydraulic_conductivity_ = pod.hydraulic_conductivity;
    carbonate_solubility_ = pod.carbonate_solubility;
    compressive_strength_mpa_ = pod.compressive_strength_mpa;
    sediment_grain_diameter_mm_ = pod.sediment_grain_diameter_mm;
    pedogenesis_humus_retention_ = pod.pedogenesis_humus_retention;
}

// ============================================================================
// GeomorphologyTerrainResource
// ============================================================================

GeomorphologyTerrainResource::GeomorphologyTerrainResource() {
    seed_profile_.instantiate();
    reset_default_lithology_catalog();
    generate_initial_world();
}

void GeomorphologyTerrainResource::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_seed_profile", "profile"), &GeomorphologyTerrainResource::set_seed_profile);
    ClassDB::bind_method(D_METHOD("get_seed_profile"), &GeomorphologyTerrainResource::get_seed_profile);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "seed_profile", PROPERTY_HINT_RESOURCE_TYPE, "GeomorphologySeedProfileResource"), "set_seed_profile", "get_seed_profile");

    ClassDB::bind_method(D_METHOD("set_lithology_catalog", "catalog"), &GeomorphologyTerrainResource::set_lithology_catalog);
    ClassDB::bind_method(D_METHOD("get_lithology_catalog"), &GeomorphologyTerrainResource::get_lithology_catalog);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "lithology_catalog", PROPERTY_HINT_ARRAY_TYPE, "LithologyStratumResource"), "set_lithology_catalog", "get_lithology_catalog");

    ClassDB::bind_method(D_METHOD("set_default_geological_dt_myr", "dt"), &GeomorphologyTerrainResource::set_default_geological_dt_myr);
    ClassDB::bind_method(D_METHOD("get_default_geological_dt_myr"), &GeomorphologyTerrainResource::get_default_geological_dt_myr);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "default_geological_dt_myr", PROPERTY_HINT_RANGE, "0.0,1.0,0.005"), "set_default_geological_dt_myr", "get_default_geological_dt_myr");

    ClassDB::bind_method(D_METHOD("set_default_hydrological_dt_sec", "dt"), &GeomorphologyTerrainResource::set_default_hydrological_dt_sec);
    ClassDB::bind_method(D_METHOD("get_default_hydrological_dt_sec"), &GeomorphologyTerrainResource::get_default_hydrological_dt_sec);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "default_hydrological_dt_sec", PROPERTY_HINT_RANGE, "0.01,5.0,0.01"), "set_default_hydrological_dt_sec", "get_default_hydrological_dt_sec");

    ClassDB::bind_method(D_METHOD("set_uniform_precipitation_rate", "rate"), &GeomorphologyTerrainResource::set_uniform_precipitation_rate);
    ClassDB::bind_method(D_METHOD("get_uniform_precipitation_rate"), &GeomorphologyTerrainResource::get_uniform_precipitation_rate);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "uniform_precipitation_rate", PROPERTY_HINT_RANGE, "0.0,0.5,0.001"), "set_uniform_precipitation_rate", "get_uniform_precipitation_rate");

    ClassDB::bind_method(D_METHOD("set_active_stage_mask", "mask"), &GeomorphologyTerrainResource::set_active_stage_mask);
    ClassDB::bind_method(D_METHOD("get_active_stage_mask"), &GeomorphologyTerrainResource::get_active_stage_mask);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "active_stage_mask"), "set_active_stage_mask", "get_active_stage_mask");

    ClassDB::bind_method(D_METHOD("get_simulation_step_counter"), &GeomorphologyTerrainResource::get_simulation_step_counter);
    ClassDB::bind_method(D_METHOD("reset_default_lithology_catalog"), &GeomorphologyTerrainResource::reset_default_lithology_catalog);
    ClassDB::bind_method(D_METHOD("generate_initial_world"), &GeomorphologyTerrainResource::generate_initial_world);
    ClassDB::bind_method(D_METHOD("step_simulation", "steps", "dt_geological_myr", "dt_hydrological_sec"), &GeomorphologyTerrainResource::step_simulation, DEFVAL(1), DEFVAL(-1.0f), DEFVAL(-1.0f));
    ClassDB::bind_method(D_METHOD("inspect_cell_column", "x", "y"), &GeomorphologyTerrainResource::inspect_cell_column);
    ClassDB::bind_method(D_METHOD("render_map_layer_image", "layer"), &GeomorphologyTerrainResource::render_map_layer_image);
    ClassDB::bind_method(D_METHOD("get_domain_telemetry"), &GeomorphologyTerrainResource::get_domain_telemetry);
    ClassDB::bind_method(D_METHOD("get_snapshot_state"), &GeomorphologyTerrainResource::get_snapshot_state);
    ClassDB::bind_method(D_METHOD("apply_snapshot_state", "state"), &GeomorphologyTerrainResource::apply_snapshot_state);
}

void GeomorphologyTerrainResource::set_seed_profile(const godot::Ref<GeomorphologySeedProfileResource>& p_profile) {
    seed_profile_ = p_profile;
    if (seed_profile_.is_null()) {
        seed_profile_.instantiate();
    }
    emit_changed();
}
godot::Ref<GeomorphologySeedProfileResource> GeomorphologyTerrainResource::get_seed_profile() const {
    return seed_profile_;
}

void GeomorphologyTerrainResource::set_lithology_catalog(const godot::TypedArray<LithologyStratumResource>& p_catalog) {
    lithology_catalog_ = p_catalog;
    _sync_pod_catalog();
    emit_changed();
}
godot::TypedArray<LithologyStratumResource> GeomorphologyTerrainResource::get_lithology_catalog() const {
    return lithology_catalog_;
}

void GeomorphologyTerrainResource::set_default_geological_dt_myr(float p_dt) {
    default_geological_dt_myr_ = std::max(0.0f, p_dt);
    emit_changed();
}
float GeomorphologyTerrainResource::get_default_geological_dt_myr() const { return default_geological_dt_myr_; }

void GeomorphologyTerrainResource::set_default_hydrological_dt_sec(float p_dt) {
    default_hydrological_dt_sec_ = std::max(0.001f, p_dt);
    emit_changed();
}
float GeomorphologyTerrainResource::get_default_hydrological_dt_sec() const { return default_hydrological_dt_sec_; }

void GeomorphologyTerrainResource::set_uniform_precipitation_rate(float p_rate) {
    uniform_precipitation_rate_ = std::max(0.0f, p_rate);
    emit_changed();
}
float GeomorphologyTerrainResource::get_uniform_precipitation_rate() const { return uniform_precipitation_rate_; }

void GeomorphologyTerrainResource::set_active_stage_mask(int p_mask) {
    active_stage_mask_ = p_mask;
    emit_changed();
}
int GeomorphologyTerrainResource::get_active_stage_mask() const { return active_stage_mask_; }

void GeomorphologyTerrainResource::reset_default_lithology_catalog() {
    auto defaults = core::GeomorphologyKernelMath::build_default_lithology_catalog();
    static const char* kNames[8] = {
        "Granite (Plutonic)",
        "Basalt (Volcanic)",
        "Sandstone (Clastic)",
        "Limestone (Carbonate)",
        "Schist (Foliated)",
        "Marble (Metamorphic)",
        "Regolith (Colluvium)",
        "Loam Soil (Pedogenic)"
    };
    lithology_catalog_.clear();
    for (size_t i = 0; i < defaults.size(); ++i) {
        godot::Ref<LithologyStratumResource> res;
        res.instantiate();
        res->from_pod(defaults[i]);
        res->set_archetype_name(kNames[i]);
        lithology_catalog_.push_back(res);
    }
    _sync_pod_catalog();
}

void GeomorphologyTerrainResource::_sync_pod_catalog() {
    pod_catalog_.clear();
    for (int i = 0; i < lithology_catalog_.size(); ++i) {
        godot::Ref<LithologyStratumResource> item = lithology_catalog_[i];
        if (item.is_valid()) {
            pod_catalog_.push_back(item->to_pod());
        }
    }
    if (pod_catalog_.empty()) {
        auto defaults = core::GeomorphologyKernelMath::build_default_lithology_catalog();
        pod_catalog_.assign(defaults.begin(), defaults.end());
    }
}

void GeomorphologyTerrainResource::_ensure_buffers_allocated() {
    if (seed_profile_.is_null()) {
        seed_profile_.instantiate();
    }
    _sync_pod_catalog();
    const core::GeomorphologySeedProfilePOD seed = seed_profile_->to_pod();
    const size_t cell_count = static_cast<size_t>(seed.grid_width) * static_cast<size_t>(seed.grid_height);
    const size_t plate_count = std::max<size_t>(2, seed.plate_count);

    pod_plates_.resize(plate_count);
    pod_crust_.resize(cell_count);
    pod_strata_.resize(cell_count);
    pod_hydrology_.resize(cell_count);
    pod_hazards_.resize(cell_count);
}

void GeomorphologyTerrainResource::generate_initial_world() {
    _ensure_buffers_allocated();
    const core::GeomorphologySeedProfilePOD seed = seed_profile_->to_pod();
    core::GeomorphologyDOD::initialize_world_from_seed(
        seed,
        pod_catalog_,
        pod_plates_,
        pod_crust_,
        pod_strata_,
        pod_hydrology_);
    core::GeomorphologyKernelMath::evaluate_hazards_and_resource_veins(
        seed,
        pod_crust_,
        pod_strata_,
        pod_hydrology_,
        pod_hazards_);
    simulation_step_counter_ = 0;
    emit_changed();
}

void GeomorphologyTerrainResource::step_simulation(int p_steps, float p_dt_geological_myr, float p_dt_hydrological_sec) {
    _ensure_buffers_allocated();
    const core::GeomorphologySeedProfilePOD seed = seed_profile_->to_pod();
    const float dt_geo = (p_dt_geological_myr >= 0.0f) ? p_dt_geological_myr : default_geological_dt_myr_;
    const float dt_hydro = (p_dt_hydrological_sec > 0.0f) ? p_dt_hydrological_sec : default_hydrological_dt_sec_;
    const int steps = std::max(1, p_steps);

    for (int i = 0; i < steps; ++i) {
        core::GeomorphologyDOD::execute_step(
            seed,
            pod_catalog_,
            pod_plates_,
            pod_crust_,
            pod_strata_,
            pod_hydrology_,
            pod_hazards_,
            dt_geo,
            dt_hydro,
            uniform_precipitation_rate_,
            static_cast<uint32_t>(active_stage_mask_));
        ++simulation_step_counter_;
    }
    emit_changed();
}

godot::Dictionary GeomorphologyTerrainResource::inspect_cell_column(int p_x, int p_y) const {
    godot::Dictionary d;
    if (seed_profile_.is_null() || pod_strata_.empty()) {
        return d;
    }
    const int w = seed_profile_->get_grid_width();
    const int h = seed_profile_->get_grid_height();
    const int cx = std::clamp(p_x, 0, w - 1);
    const int cy = std::clamp(p_y, 0, h - 1);
    const size_t idx = static_cast<size_t>(cy * w + cx);
    if (idx >= pod_strata_.size()) {
        return d;
    }

    const auto& col = pod_strata_[idx];
    const auto& crust = pod_crust_[idx];
    const auto& hydro = pod_hydrology_[idx];
    const auto& haz = pod_hazards_[idx];

    d["grid_x"] = cx;
    d["grid_y"] = cy;
    d["base_elevation_m"] = col.base_elevation_m;
    d["total_column_height_m"] = col.total_column_height_m;
    d["regolith_thickness_m"] = col.regolith_thickness_m;
    d["soil_organic_humus"] = col.soil_organic_humus;
    d["karst_cavity_void_m"] = col.karst_cavity_void_m;
    d["active_layer_count"] = static_cast<int>(col.active_layer_count);

    d["plate_id"] = static_cast<int>(crust.primary_plate_id);
    d["neighbor_plate_id"] = static_cast<int>(crust.secondary_plate_id);
    d["boundary_type"] = static_cast<int>(crust.boundary_type);
    d["convergent_stress"] = crust.convergent_stress;
    d["uplift_rate_mm_yr"] = crust.uplift_rate_mm_yr;
    d["geothermal_flux_w_m2"] = crust.geothermal_flux_w_m2;

    d["water_depth_m"] = hydro.water_depth_m;
    d["groundwater_table_m"] = hydro.groundwater_table_m;
    d["suspended_sediment_kg"] = hydro.suspended_sediment_kg;
    d["volumetric_discharge_m3s"] = hydro.volumetric_discharge_m3s;
    d["stream_power_index"] = hydro.stream_power_index;
    d["strahler_order"] = static_cast<int>(hydro.strahler_order);
    d["drainage_basin_id"] = static_cast<int>(hydro.drainage_basin_id);
    d["drainage_basin_class"] = static_cast<int>(hydro.basin_class);
    d["upstream_catchment_cells"] = static_cast<int>(hydro.upstream_catchment_cells);

    d["landslide_hazard"] = haz.landslide_hazard_probability;
    d["sinkhole_hazard"] = haz.karst_sinkhole_hazard;
    d["flash_flood_hazard"] = haz.flash_flood_hazard;
    d["hydrothermal_ore_grade"] = haz.hydrothermal_ore_grade;
    d["placer_alluvial_grade"] = haz.placer_alluvial_grade;
    d["evaporite_brine_grade"] = haz.evaporite_brine_grade;

    godot::Array layers_arr;
    for (uint8_t k = 0; k < col.active_layer_count; ++k) {
        godot::Dictionary ld;
        const auto& layer = col.layers[k];
        ld["lithology_id"] = static_cast<int>(layer.lithology_id);
        ld["thickness_m"] = layer.thickness_m;
        ld["porosity_fraction"] = layer.porosity_fraction;
        ld["fracture_density"] = layer.fracture_density;
        ld["mineral_vein_grade"] = layer.mineral_vein_grade;
        ld["water_saturation"] = layer.water_saturation;

        godot::String lith_name = "Rock Stratum";
        if (layer.lithology_id < lithology_catalog_.size()) {
            godot::Ref<LithologyStratumResource> lr = lithology_catalog_[layer.lithology_id];
            if (lr.is_valid()) {
                lith_name = lr->get_archetype_name();
            }
        }
        ld["archetype_name"] = lith_name;
        layers_arr.push_back(ld);
    }
    d["strata"] = layers_arr;
    return d;
}

godot::Ref<godot::Image> GeomorphologyTerrainResource::render_map_layer_image(int p_layer) const {
    if (seed_profile_.is_null() || pod_strata_.empty()) {
        return godot::Ref<godot::Image>();
    }
    const int w = seed_profile_->get_grid_width();
    const int h = seed_profile_->get_grid_height();
    godot::Ref<godot::Image> img = godot::Image::create_empty(w, h, false, godot::Image::FORMAT_RGBA8);

    const float sea_level = seed_profile_->get_sea_level_elevation_m();
    float min_elev = 1e9f;
    float max_elev = -1e9f;
    for (const auto& col : pod_strata_) {
        min_elev = std::min(min_elev, col.total_column_height_m);
        max_elev = std::max(max_elev, col.total_column_height_m);
    }
    const float elev_span = std::max(1.0f, max_elev - min_elev);
    const auto layer_enum = static_cast<core::GeomorphologyMapLayer>(p_layer);

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const size_t idx = static_cast<size_t>(y * w + x);
            const auto& col = pod_strata_[idx];
            const auto& crust = pod_crust_[idx];
            const auto& hydro = pod_hydrology_[idx];
            const auto& haz = pod_hazards_[idx];

            godot::Color c(0.15f, 0.18f, 0.22f, 1.0f);
            switch (layer_enum) {
                case core::GeomorphologyMapLayer::ELEVATION_SHADED_RELIEF: {
                    const float norm = std::clamp((col.total_column_height_m - min_elev) / elev_span, 0.0f, 1.0f);
                    if (col.total_column_height_m < sea_level) {
                        c = godot::Color(0.08f, 0.24f + 0.35f * norm, 0.58f + 0.25f * norm, 1.0f);
                    } else {
                        c = godot::Color(0.20f + 0.65f * norm, 0.42f + 0.38f * norm, 0.22f + 0.55f * (norm * norm), 1.0f);
                    }
                    break;
                }
                case core::GeomorphologyMapLayer::TECTONIC_PLATES_AND_STRESS: {
                    const float hue = std::fmod(static_cast<float>(crust.primary_plate_id) * 0.21f, 1.0f);
                    c = godot::Color::from_hsv(hue, 0.55f, 0.65f);
                    const float stress = std::clamp(std::abs(crust.convergent_stress) * 0.45f, 0.0f, 1.0f);
                    c = c.lerp(godot::Color(1.0f, 0.28f, 0.12f, 1.0f), stress);
                    break;
                }
                case core::GeomorphologyMapLayer::TOP_EXPOSED_LITHOLOGY: {
                    const uint16_t top_id = col.top_exposed_lithology();
                    static const godot::Color kLithColors[8] = {
                        godot::Color(0.76f, 0.48f, 0.52f, 1.0f), // Granite
                        godot::Color(0.24f, 0.26f, 0.30f, 1.0f), // Basalt
                        godot::Color(0.84f, 0.72f, 0.42f, 1.0f), // Sandstone
                        godot::Color(0.72f, 0.80f, 0.84f, 1.0f), // Limestone
                        godot::Color(0.46f, 0.52f, 0.58f, 1.0f), // Schist
                        godot::Color(0.92f, 0.90f, 0.94f, 1.0f), // Marble
                        godot::Color(0.62f, 0.54f, 0.42f, 1.0f), // Regolith
                        godot::Color(0.32f, 0.48f, 0.24f, 1.0f)  // Loam Soil
                    };
                    c = kLithColors[top_id % 8];
                    break;
                }
                case core::GeomorphologyMapLayer::SURFACE_WATER_DEPTH: {
                    const float d_norm = std::clamp(hydro.water_depth_m / 5.0f, 0.0f, 1.0f);
                    const float q_norm = std::clamp(std::log1p(hydro.volumetric_discharge_m3s) / 5.0f, 0.0f, 1.0f);
                    c = godot::Color(0.12f * (1.0f - d_norm), 0.30f + 0.55f * q_norm, 0.45f + 0.55f * std::max(d_norm, q_norm), 1.0f);
                    break;
                }
                case core::GeomorphologyMapLayer::STREAM_POWER_EROSION: {
                    const float sp = std::clamp(std::log1p(hydro.stream_power_index) / 6.0f, 0.0f, 1.0f);
                    c = godot::Color(0.18f + 0.82f * sp, 0.15f + 0.55f * sp, 0.35f * (1.0f - sp), 1.0f);
                    break;
                }
                case core::GeomorphologyMapLayer::STRAHLER_STREAM_ORDER: {
                    const float s_norm = std::clamp(static_cast<float>(hydro.strahler_order) / 6.0f, 0.0f, 1.0f);
                    const float basin_hue = std::fmod(static_cast<float>(hydro.drainage_basin_id) * 0.137f, 1.0f);
                    c = godot::Color::from_hsv(basin_hue, 0.35f, 0.35f);
                    if (hydro.strahler_order >= 2) {
                        c = c.lerp(godot::Color(0.15f, 0.85f, 1.0f, 1.0f), s_norm);
                    }
                    break;
                }
                case core::GeomorphologyMapLayer::KARST_CAVITY_POTENTIAL: {
                    const float k_norm = std::clamp(col.karst_cavity_void_m / 8.0f, 0.0f, 1.0f);
                    c = godot::Color(0.22f + 0.68f * k_norm, 0.16f + 0.20f * k_norm, 0.32f + 0.65f * k_norm, 1.0f);
                    break;
                }
                case core::GeomorphologyMapLayer::SOIL_HUMUS_FERTILITY: {
                    const float h_norm = std::clamp(col.soil_organic_humus, 0.0f, 1.0f);
                    c = godot::Color(0.38f * (1.0f - 0.5f * h_norm), 0.25f + 0.65f * h_norm, 0.18f, 1.0f);
                    break;
                }
                case core::GeomorphologyMapLayer::MINERAL_VEIN_POTENTIAL: {
                    const float ore = std::clamp(std::max({haz.hydrothermal_ore_grade, haz.placer_alluvial_grade, haz.evaporite_brine_grade}), 0.0f, 1.0f);
                    c = godot::Color(0.20f + 0.80f * ore, 0.18f + 0.68f * ore, 0.15f, 1.0f);
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

godot::Dictionary GeomorphologyTerrainResource::get_domain_telemetry() const {
    godot::Dictionary d;
    if (pod_strata_.empty()) {
        return d;
    }
    float min_elev = 1e9f;
    float max_elev = -1e9f;
    int river_cells = 0;
    int karst_cells = 0;
    int max_strahler = 0;
    float mean_humus = 0.0f;

    for (size_t i = 0; i < pod_strata_.size(); ++i) {
        const float elev = pod_strata_[i].total_column_height_m;
        min_elev = std::min(min_elev, elev);
        max_elev = std::max(max_elev, elev);
        mean_humus += pod_strata_[i].soil_organic_humus;
        if (pod_strata_[i].karst_cavity_void_m > 0.15f) {
            ++karst_cells;
        }
        if (pod_hydrology_[i].strahler_order >= 2) {
            ++river_cells;
        }
        max_strahler = std::max(max_strahler, static_cast<int>(pod_hydrology_[i].strahler_order));
    }
    mean_humus /= static_cast<float>(pod_strata_.size());

    d["cell_count"] = static_cast<int>(pod_strata_.size());
    d["simulation_steps"] = static_cast<int>(simulation_step_counter_);
    d["min_elevation_m"] = min_elev;
    d["max_elevation_m"] = max_elev;
    d["river_channel_cells"] = river_cells;
    d["karst_cavity_cells"] = karst_cells;
    d["max_strahler_order"] = max_strahler;
    d["mean_soil_humus"] = mean_humus;
    return d;
}

godot::Dictionary GeomorphologyTerrainResource::get_snapshot_state() const {
    godot::Dictionary state;
    if (seed_profile_.is_valid()) {
        state["master_seed"] = seed_profile_->get_master_seed();
        state["grid_width"] = seed_profile_->get_grid_width();
        state["grid_height"] = seed_profile_->get_grid_height();
        state["plate_count"] = seed_profile_->get_plate_count();
    }
    state["simulation_step_counter"] = static_cast<int>(simulation_step_counter_);
    state["uniform_precipitation_rate"] = uniform_precipitation_rate_;
    state["active_stage_mask"] = active_stage_mask_;
    return state;
}

void GeomorphologyTerrainResource::apply_snapshot_state(const godot::Dictionary& p_state) {
    if (seed_profile_.is_valid()) {
        if (p_state.has("master_seed")) {
            seed_profile_->set_master_seed(static_cast<int64_t>(p_state["master_seed"]));
        }
        if (p_state.has("grid_width")) {
            seed_profile_->set_grid_width(static_cast<int>(p_state["grid_width"]));
        }
        if (p_state.has("grid_height")) {
            seed_profile_->set_grid_height(static_cast<int>(p_state["grid_height"]));
        }
        if (p_state.has("plate_count")) {
            seed_profile_->set_plate_count(static_cast<int>(p_state["plate_count"]));
        }
    }
    if (p_state.has("uniform_precipitation_rate")) {
        uniform_precipitation_rate_ = static_cast<float>(p_state["uniform_precipitation_rate"]);
    }
    if (p_state.has("active_stage_mask")) {
        active_stage_mask_ = static_cast<int>(p_state["active_stage_mask"]);
    }
    generate_initial_world();
    const int target_steps = p_state.get("simulation_step_counter", 0);
    if (target_steps > 0) {
        step_simulation(target_steps);
    }
}

// ============================================================================
// GeomorphologyTerrainNode3D
// ============================================================================

GeomorphologyTerrainNode3D::GeomorphologyTerrainNode3D() {
    terrain_resource_.instantiate();
}

void GeomorphologyTerrainNode3D::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_terrain_resource", "resource"), &GeomorphologyTerrainNode3D::set_terrain_resource);
    ClassDB::bind_method(D_METHOD("get_terrain_resource"), &GeomorphologyTerrainNode3D::get_terrain_resource);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "terrain_resource", PROPERTY_HINT_RESOURCE_TYPE, "GeomorphologyTerrainResource"), "set_terrain_resource", "get_terrain_resource");

    ClassDB::bind_method(D_METHOD("set_auto_generate_on_ready", "auto_generate"), &GeomorphologyTerrainNode3D::set_auto_generate_on_ready);
    ClassDB::bind_method(D_METHOD("get_auto_generate_on_ready"), &GeomorphologyTerrainNode3D::get_auto_generate_on_ready);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_generate_on_ready"), "set_auto_generate_on_ready", "get_auto_generate_on_ready");

    ClassDB::bind_method(D_METHOD("world_to_grid", "world_pos"), &GeomorphologyTerrainNode3D::world_to_grid);
    ClassDB::bind_method(D_METHOD("sample_surface_elevation_at_world", "world_pos"), &GeomorphologyTerrainNode3D::sample_surface_elevation_at_world);
    ClassDB::bind_method(D_METHOD("sample_column_at_world", "world_pos"), &GeomorphologyTerrainNode3D::sample_column_at_world);
}

void GeomorphologyTerrainNode3D::_ready() {
    if (auto_generate_on_ready_ && terrain_resource_.is_valid() && terrain_resource_->get_strata_const_span().empty()) {
        terrain_resource_->generate_initial_world();
    }
}

void GeomorphologyTerrainNode3D::set_terrain_resource(const godot::Ref<GeomorphologyTerrainResource>& p_resource) {
    terrain_resource_ = p_resource;
}
godot::Ref<GeomorphologyTerrainResource> GeomorphologyTerrainNode3D::get_terrain_resource() const {
    return terrain_resource_;
}

void GeomorphologyTerrainNode3D::set_auto_generate_on_ready(bool p_auto) {
    auto_generate_on_ready_ = p_auto;
}
bool GeomorphologyTerrainNode3D::get_auto_generate_on_ready() const {
    return auto_generate_on_ready_;
}

godot::Vector2i GeomorphologyTerrainNode3D::world_to_grid(const godot::Vector3& p_world_pos) const {
    if (terrain_resource_.is_null() || terrain_resource_->get_seed_profile().is_null()) {
        return godot::Vector2i(0, 0);
    }
    const godot::Vector3 local = to_local(p_world_pos);
    const auto seed = terrain_resource_->get_seed_profile();
    const float cell_m = std::max(1.0f, seed->get_cell_size_meters());
    const int gx = std::clamp(static_cast<int>(std::floor(local.x / cell_m)), 0, seed->get_grid_width() - 1);
    const int gy = std::clamp(static_cast<int>(std::floor(local.z / cell_m)), 0, seed->get_grid_height() - 1);
    return godot::Vector2i(gx, gy);
}

float GeomorphologyTerrainNode3D::sample_surface_elevation_at_world(const godot::Vector3& p_world_pos) const {
    if (terrain_resource_.is_null()) {
        return 0.0f;
    }
    const godot::Vector2i g = world_to_grid(p_world_pos);
    const godot::Dictionary col = terrain_resource_->inspect_cell_column(g.x, g.y);
    return static_cast<float>(col.get("total_column_height_m", 0.0f));
}

godot::Dictionary GeomorphologyTerrainNode3D::sample_column_at_world(const godot::Vector3& p_world_pos) const {
    if (terrain_resource_.is_null()) {
        return godot::Dictionary();
    }
    const godot::Vector2i g = world_to_grid(p_world_pos);
    return terrain_resource_->inspect_cell_column(g.x, g.y);
}

} // namespace ideam::godot_ext
