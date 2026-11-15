#include "godot/ecology/ecology_biosphere_resource.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/color.hpp>

#include <algorithm>
#include <cmath>

namespace ideam::ecology {

// ============================================================================
// EcologySeedProfileResource
// ============================================================================

void EcologySeedProfileResource::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_master_seed", "seed"), &EcologySeedProfileResource::set_master_seed);
    ClassDB::bind_method(D_METHOD("get_master_seed"), &EcologySeedProfileResource::get_master_seed);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "master_seed"), "set_master_seed", "get_master_seed");

    ClassDB::bind_method(D_METHOD("set_grid_width", "width"), &EcologySeedProfileResource::set_grid_width);
    ClassDB::bind_method(D_METHOD("get_grid_width"), &EcologySeedProfileResource::get_grid_width);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "grid_width", PROPERTY_HINT_RANGE, "8,512,1"), "set_grid_width", "get_grid_width");

    ClassDB::bind_method(D_METHOD("set_grid_height", "height"), &EcologySeedProfileResource::set_grid_height);
    ClassDB::bind_method(D_METHOD("get_grid_height"), &EcologySeedProfileResource::get_grid_height);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "grid_height", PROPERTY_HINT_RANGE, "8,512,1"), "set_grid_height", "get_grid_height");

    ClassDB::bind_method(D_METHOD("set_active_guild_count", "count"), &EcologySeedProfileResource::set_active_guild_count);
    ClassDB::bind_method(D_METHOD("get_active_guild_count"), &EcologySeedProfileResource::get_active_guild_count);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "active_guild_count", PROPERTY_HINT_RANGE, "1,16,1"), "set_active_guild_count", "get_active_guild_count");

    ClassDB::bind_method(D_METHOD("set_cell_size_meters", "size"), &EcologySeedProfileResource::set_cell_size_meters);
    ClassDB::bind_method(D_METHOD("get_cell_size_meters"), &EcologySeedProfileResource::get_cell_size_meters);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "cell_size_meters", PROPERTY_HINT_RANGE, "1.0,1000.0,0.5"), "set_cell_size_meters", "get_cell_size_meters");

    ClassDB::bind_method(D_METHOD("set_ambient_co2_ppm", "co2"), &EcologySeedProfileResource::set_ambient_co2_ppm);
    ClassDB::bind_method(D_METHOD("get_ambient_co2_ppm"), &EcologySeedProfileResource::get_ambient_co2_ppm);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "ambient_co2_ppm", PROPERTY_HINT_RANGE, "180.0,1200.0,5.0"), "set_ambient_co2_ppm", "get_ambient_co2_ppm");

    ClassDB::bind_method(D_METHOD("set_beer_lambert_extinction_k", "k"), &EcologySeedProfileResource::set_beer_lambert_extinction_k);
    ClassDB::bind_method(D_METHOD("get_beer_lambert_extinction_k"), &EcologySeedProfileResource::get_beer_lambert_extinction_k);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "beer_lambert_extinction_k", PROPERTY_HINT_RANGE, "0.1,1.5,0.02"), "set_beer_lambert_extinction_k", "get_beer_lambert_extinction_k");

    ClassDB::bind_method(D_METHOD("set_base_npp_efficiency", "eff"), &EcologySeedProfileResource::set_base_npp_efficiency);
    ClassDB::bind_method(D_METHOD("get_base_npp_efficiency"), &EcologySeedProfileResource::get_base_npp_efficiency);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "base_npp_efficiency", PROPERTY_HINT_RANGE, "0.1,5.0,0.05"), "set_base_npp_efficiency", "get_base_npp_efficiency");

    ClassDB::bind_method(D_METHOD("set_q10_decomposition_factor", "q10"), &EcologySeedProfileResource::set_q10_decomposition_factor);
    ClassDB::bind_method(D_METHOD("get_q10_decomposition_factor"), &EcologySeedProfileResource::get_q10_decomposition_factor);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "q10_decomposition_factor", PROPERTY_HINT_RANGE, "1.1,4.0,0.05"), "set_q10_decomposition_factor", "get_q10_decomposition_factor");

    ClassDB::bind_method(D_METHOD("set_nitrogen_fixation_scale", "scale"), &EcologySeedProfileResource::set_nitrogen_fixation_scale);
    ClassDB::bind_method(D_METHOD("get_nitrogen_fixation_scale"), &EcologySeedProfileResource::get_nitrogen_fixation_scale);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "nitrogen_fixation_scale", PROPERTY_HINT_RANGE, "0.0,5.0,0.05"), "set_nitrogen_fixation_scale", "get_nitrogen_fixation_scale");

    ClassDB::bind_method(D_METHOD("set_rothermel_wind_spread_scale", "scale"), &EcologySeedProfileResource::set_rothermel_wind_spread_scale);
    ClassDB::bind_method(D_METHOD("get_rothermel_wind_spread_scale"), &EcologySeedProfileResource::get_rothermel_wind_spread_scale);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "rothermel_wind_spread_scale", PROPERTY_HINT_RANGE, "0.0,5.0,0.05"), "set_rothermel_wind_spread_scale", "get_rothermel_wind_spread_scale");

    ClassDB::bind_method(D_METHOD("set_herbivore_grazing_rate", "rate"), &EcologySeedProfileResource::set_herbivore_grazing_rate);
    ClassDB::bind_method(D_METHOD("get_herbivore_grazing_rate"), &EcologySeedProfileResource::get_herbivore_grazing_rate);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "herbivore_grazing_rate", PROPERTY_HINT_RANGE, "0.01,1.0,0.01"), "set_herbivore_grazing_rate", "get_herbivore_grazing_rate");

    ClassDB::bind_method(D_METHOD("set_predator_attack_rate", "rate"), &EcologySeedProfileResource::set_predator_attack_rate);
    ClassDB::bind_method(D_METHOD("get_predator_attack_rate"), &EcologySeedProfileResource::get_predator_attack_rate);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "predator_attack_rate", PROPERTY_HINT_RANGE, "0.01,1.0,0.01"), "set_predator_attack_rate", "get_predator_attack_rate");

    ClassDB::bind_method(D_METHOD("set_predator_conversion_eff", "eff"), &EcologySeedProfileResource::set_predator_conversion_eff);
    ClassDB::bind_method(D_METHOD("get_predator_conversion_eff"), &EcologySeedProfileResource::get_predator_conversion_eff);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "predator_conversion_eff", PROPERTY_HINT_RANGE, "0.05,0.9,0.02"), "set_predator_conversion_eff", "get_predator_conversion_eff");

    ClassDB::bind_method(D_METHOD("set_alchemical_leyline_resonance", "res"), &EcologySeedProfileResource::set_alchemical_leyline_resonance);
    ClassDB::bind_method(D_METHOD("get_alchemical_leyline_resonance"), &EcologySeedProfileResource::get_alchemical_leyline_resonance);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "alchemical_leyline_resonance", PROPERTY_HINT_RANGE, "0.0,5.0,0.05"), "set_alchemical_leyline_resonance", "get_alchemical_leyline_resonance");

    ClassDB::bind_method(D_METHOD("get_whittaker_niche_seed"), &EcologySeedProfileResource::get_whittaker_niche_seed);
    ClassDB::bind_method(D_METHOD("get_succession_dispersal_seed"), &EcologySeedProfileResource::get_succession_dispersal_seed);
    ClassDB::bind_method(D_METHOD("get_trophic_lotka_volterra_seed"), &EcologySeedProfileResource::get_trophic_lotka_volterra_seed);
    ClassDB::bind_method(D_METHOD("get_rothermel_wildfire_seed"), &EcologySeedProfileResource::get_rothermel_wildfire_seed);
}

void EcologySeedProfileResource::set_master_seed(int64_t p_seed) { master_seed_ = p_seed; emit_changed(); }
int64_t EcologySeedProfileResource::get_master_seed() const { return master_seed_; }

void EcologySeedProfileResource::set_grid_width(int p_width) { grid_width_ = std::clamp(p_width, 8, 512); emit_changed(); }
int EcologySeedProfileResource::get_grid_width() const { return grid_width_; }

void EcologySeedProfileResource::set_grid_height(int p_height) { grid_height_ = std::clamp(p_height, 8, 512); emit_changed(); }
int EcologySeedProfileResource::get_grid_height() const { return grid_height_; }

void EcologySeedProfileResource::set_active_guild_count(int p_count) { active_guild_count_ = std::clamp(p_count, 1, 16); emit_changed(); }
int EcologySeedProfileResource::get_active_guild_count() const { return active_guild_count_; }

void EcologySeedProfileResource::set_cell_size_meters(float p_size) { cell_size_meters_ = std::max(1.0f, p_size); emit_changed(); }
float EcologySeedProfileResource::get_cell_size_meters() const { return cell_size_meters_; }

void EcologySeedProfileResource::set_ambient_co2_ppm(float p_co2) { ambient_co2_ppm_ = std::max(100.0f, p_co2); emit_changed(); }
float EcologySeedProfileResource::get_ambient_co2_ppm() const { return ambient_co2_ppm_; }

void EcologySeedProfileResource::set_beer_lambert_extinction_k(float p_k) { beer_lambert_extinction_k_ = std::clamp(p_k, 0.05f, 2.0f); emit_changed(); }
float EcologySeedProfileResource::get_beer_lambert_extinction_k() const { return beer_lambert_extinction_k_; }

void EcologySeedProfileResource::set_base_npp_efficiency(float p_eff) { base_npp_efficiency_ = std::max(0.05f, p_eff); emit_changed(); }
float EcologySeedProfileResource::get_base_npp_efficiency() const { return base_npp_efficiency_; }

void EcologySeedProfileResource::set_q10_decomposition_factor(float p_q10) { q10_decomposition_factor_ = std::clamp(p_q10, 1.05f, 5.0f); emit_changed(); }
float EcologySeedProfileResource::get_q10_decomposition_factor() const { return q10_decomposition_factor_; }

void EcologySeedProfileResource::set_nitrogen_fixation_scale(float p_scale) { nitrogen_fixation_scale_ = std::max(0.0f, p_scale); emit_changed(); }
float EcologySeedProfileResource::get_nitrogen_fixation_scale() const { return nitrogen_fixation_scale_; }

void EcologySeedProfileResource::set_rothermel_wind_spread_scale(float p_scale) { rothermel_wind_spread_scale_ = std::max(0.0f, p_scale); emit_changed(); }
float EcologySeedProfileResource::get_rothermel_wind_spread_scale() const { return rothermel_wind_spread_scale_; }

void EcologySeedProfileResource::set_herbivore_grazing_rate(float p_rate) { herbivore_grazing_rate_ = std::max(0.01f, p_rate); emit_changed(); }
float EcologySeedProfileResource::get_herbivore_grazing_rate() const { return herbivore_grazing_rate_; }

void EcologySeedProfileResource::set_predator_attack_rate(float p_rate) { predator_attack_rate_ = std::max(0.01f, p_rate); emit_changed(); }
float EcologySeedProfileResource::get_predator_attack_rate() const { return predator_attack_rate_; }

void EcologySeedProfileResource::set_predator_conversion_eff(float p_eff) { predator_conversion_eff_ = std::clamp(p_eff, 0.01f, 1.0f); emit_changed(); }
float EcologySeedProfileResource::get_predator_conversion_eff() const { return predator_conversion_eff_; }

void EcologySeedProfileResource::set_alchemical_leyline_resonance(float p_res) { alchemical_leyline_resonance_ = std::max(0.0f, p_res); emit_changed(); }
float EcologySeedProfileResource::get_alchemical_leyline_resonance() const { return alchemical_leyline_resonance_; }

int64_t EcologySeedProfileResource::get_whittaker_niche_seed() const { return static_cast<int64_t>(to_pod().whittaker_niche_seed); }
int64_t EcologySeedProfileResource::get_succession_dispersal_seed() const { return static_cast<int64_t>(to_pod().succession_dispersal_seed); }
int64_t EcologySeedProfileResource::get_trophic_lotka_volterra_seed() const { return static_cast<int64_t>(to_pod().trophic_lotka_volterra_seed); }
int64_t EcologySeedProfileResource::get_rothermel_wildfire_seed() const { return static_cast<int64_t>(to_pod().rothermel_wildfire_seed); }

EcologySeedProfilePOD EcologySeedProfileResource::to_pod() const noexcept {
    EcologySeedProfilePOD pod = EcologyKernelMath::make_seed_profile(
        static_cast<uint64_t>(master_seed_),
        static_cast<uint32_t>(grid_width_),
        static_cast<uint32_t>(grid_height_),
        static_cast<uint32_t>(active_guild_count_),
        cell_size_meters_);
    pod.ambient_co2_ppm = ambient_co2_ppm_;
    pod.beer_lambert_extinction_k = beer_lambert_extinction_k_;
    pod.base_npp_efficiency = base_npp_efficiency_;
    pod.q10_decomposition_factor = q10_decomposition_factor_;
    pod.nitrogen_fixation_scale = nitrogen_fixation_scale_;
    pod.rothermel_wind_spread_scale = rothermel_wind_spread_scale_;
    pod.herbivore_grazing_rate = herbivore_grazing_rate_;
    pod.predator_attack_rate = predator_attack_rate_;
    pod.predator_conversion_eff = predator_conversion_eff_;
    pod.alchemical_leyline_resonance = alchemical_leyline_resonance_;
    return pod;
}

void EcologySeedProfileResource::from_pod(const EcologySeedProfilePOD& pod) {
    master_seed_ = static_cast<int64_t>(pod.master_seed);
    grid_width_ = static_cast<int>(pod.grid_width);
    grid_height_ = static_cast<int>(pod.grid_height);
    active_guild_count_ = static_cast<int>(pod.active_guild_count);
    cell_size_meters_ = pod.cell_size_meters;
    ambient_co2_ppm_ = pod.ambient_co2_ppm;
    beer_lambert_extinction_k_ = pod.beer_lambert_extinction_k;
    base_npp_efficiency_ = pod.base_npp_efficiency;
    q10_decomposition_factor_ = pod.q10_decomposition_factor;
    nitrogen_fixation_scale_ = pod.nitrogen_fixation_scale;
    rothermel_wind_spread_scale_ = pod.rothermel_wind_spread_scale;
    herbivore_grazing_rate_ = pod.herbivore_grazing_rate;
    predator_attack_rate_ = pod.predator_attack_rate;
    predator_conversion_eff_ = pod.predator_conversion_eff;
    alchemical_leyline_resonance_ = pod.alchemical_leyline_resonance;
    emit_changed();
}

// ============================================================================
// FloraGuildArchetypeResource
// ============================================================================

void FloraGuildArchetypeResource::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_guild_id", "id"), &FloraGuildArchetypeResource::set_guild_id);
    ClassDB::bind_method(D_METHOD("get_guild_id"), &FloraGuildArchetypeResource::get_guild_id);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "guild_id"), "set_guild_id", "get_guild_id");

    ClassDB::bind_method(D_METHOD("set_trophic_role", "role"), &FloraGuildArchetypeResource::set_trophic_role);
    ClassDB::bind_method(D_METHOD("get_trophic_role"), &FloraGuildArchetypeResource::get_trophic_role);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "trophic_role", PROPERTY_HINT_ENUM, "PrimaryProducerCanopy,PrimaryProducerUnderstory,NitrogenFixingPioneer,MycorrhizalDecomposer,GrazingMegaherbivore,BrowsingMesoherbivore,PollinatorSeedDisperser,ApexPredatorCarnivore"), "set_trophic_role", "get_trophic_role");

    ClassDB::bind_method(D_METHOD("set_preferred_seral_stage", "stage"), &FloraGuildArchetypeResource::set_preferred_seral_stage);
    ClassDB::bind_method(D_METHOD("get_preferred_seral_stage"), &FloraGuildArchetypeResource::get_preferred_seral_stage);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "preferred_seral_stage"), "set_preferred_seral_stage", "get_preferred_seral_stage");

    ClassDB::bind_method(D_METHOD("set_guild_name", "name"), &FloraGuildArchetypeResource::set_guild_name);
    ClassDB::bind_method(D_METHOD("get_guild_name"), &FloraGuildArchetypeResource::get_guild_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "guild_name"), "set_guild_name", "get_guild_name");

    ClassDB::bind_method(D_METHOD("set_optimal_temp_c", "val"), &FloraGuildArchetypeResource::set_optimal_temp_c);
    ClassDB::bind_method(D_METHOD("get_optimal_temp_c"), &FloraGuildArchetypeResource::get_optimal_temp_c);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "optimal_temp_c"), "set_optimal_temp_c", "get_optimal_temp_c");

    ClassDB::bind_method(D_METHOD("set_temp_tolerance_sigma_c", "val"), &FloraGuildArchetypeResource::set_temp_tolerance_sigma_c);
    ClassDB::bind_method(D_METHOD("get_temp_tolerance_sigma_c"), &FloraGuildArchetypeResource::get_temp_tolerance_sigma_c);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "temp_tolerance_sigma_c"), "set_temp_tolerance_sigma_c", "get_temp_tolerance_sigma_c");

    ClassDB::bind_method(D_METHOD("set_optimal_precip_mm", "val"), &FloraGuildArchetypeResource::set_optimal_precip_mm);
    ClassDB::bind_method(D_METHOD("get_optimal_precip_mm"), &FloraGuildArchetypeResource::get_optimal_precip_mm);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "optimal_precip_mm"), "set_optimal_precip_mm", "get_optimal_precip_mm");

    ClassDB::bind_method(D_METHOD("set_precip_tolerance_sigma_mm", "val"), &FloraGuildArchetypeResource::set_precip_tolerance_sigma_mm);
    ClassDB::bind_method(D_METHOD("get_precip_tolerance_sigma_mm"), &FloraGuildArchetypeResource::get_precip_tolerance_sigma_mm);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "precip_tolerance_sigma_mm"), "set_precip_tolerance_sigma_mm", "get_precip_tolerance_sigma_mm");

    ClassDB::bind_method(D_METHOD("set_drought_wilting_point_01", "val"), &FloraGuildArchetypeResource::set_drought_wilting_point_01);
    ClassDB::bind_method(D_METHOD("get_drought_wilting_point_01"), &FloraGuildArchetypeResource::get_drought_wilting_point_01);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "drought_wilting_point_01"), "set_drought_wilting_point_01", "get_drought_wilting_point_01");

    ClassDB::bind_method(D_METHOD("set_shade_tolerance_01", "val"), &FloraGuildArchetypeResource::set_shade_tolerance_01);
    ClassDB::bind_method(D_METHOD("get_shade_tolerance_01"), &FloraGuildArchetypeResource::get_shade_tolerance_01);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "shade_tolerance_01"), "set_shade_tolerance_01", "get_shade_tolerance_01");

    ClassDB::bind_method(D_METHOD("set_fire_resistance_bark_01", "val"), &FloraGuildArchetypeResource::set_fire_resistance_bark_01);
    ClassDB::bind_method(D_METHOD("get_fire_resistance_bark_01"), &FloraGuildArchetypeResource::get_fire_resistance_bark_01);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "fire_resistance_bark_01"), "set_fire_resistance_bark_01", "get_fire_resistance_bark_01");

    ClassDB::bind_method(D_METHOD("set_serotinous_reseed_boost_01", "val"), &FloraGuildArchetypeResource::set_serotinous_reseed_boost_01);
    ClassDB::bind_method(D_METHOD("get_serotinous_reseed_boost_01"), &FloraGuildArchetypeResource::get_serotinous_reseed_boost_01);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "serotinous_reseed_boost_01"), "set_serotinous_reseed_boost_01", "get_serotinous_reseed_boost_01");

    ClassDB::bind_method(D_METHOD("set_intrinsic_growth_rate_r", "val"), &FloraGuildArchetypeResource::set_intrinsic_growth_rate_r);
    ClassDB::bind_method(D_METHOD("get_intrinsic_growth_rate_r"), &FloraGuildArchetypeResource::get_intrinsic_growth_rate_r);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "intrinsic_growth_rate_r"), "set_intrinsic_growth_rate_r", "get_intrinsic_growth_rate_r");

    ClassDB::bind_method(D_METHOD("set_max_carrying_biomass_kg_m2", "val"), &FloraGuildArchetypeResource::set_max_carrying_biomass_kg_m2);
    ClassDB::bind_method(D_METHOD("get_max_carrying_biomass_kg_m2"), &FloraGuildArchetypeResource::get_max_carrying_biomass_kg_m2);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_carrying_biomass_kg_m2"), "set_max_carrying_biomass_kg_m2", "get_max_carrying_biomass_kg_m2");

    ClassDB::bind_method(D_METHOD("set_nitrogen_fixation_g_m2_yr", "val"), &FloraGuildArchetypeResource::set_nitrogen_fixation_g_m2_yr);
    ClassDB::bind_method(D_METHOD("get_nitrogen_fixation_g_m2_yr"), &FloraGuildArchetypeResource::get_nitrogen_fixation_g_m2_yr);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "nitrogen_fixation_g_m2_yr"), "set_nitrogen_fixation_g_m2_yr", "get_nitrogen_fixation_g_m2_yr");

    ClassDB::bind_method(D_METHOD("set_litter_lignin_recalcitrance", "val"), &FloraGuildArchetypeResource::set_litter_lignin_recalcitrance);
    ClassDB::bind_method(D_METHOD("get_litter_lignin_recalcitrance"), &FloraGuildArchetypeResource::get_litter_lignin_recalcitrance);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "litter_lignin_recalcitrance"), "set_litter_lignin_recalcitrance", "get_litter_lignin_recalcitrance");

    ClassDB::bind_method(D_METHOD("set_alchemical_reagent_affinity", "val"), &FloraGuildArchetypeResource::set_alchemical_reagent_affinity);
    ClassDB::bind_method(D_METHOD("get_alchemical_reagent_affinity"), &FloraGuildArchetypeResource::get_alchemical_reagent_affinity);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "alchemical_reagent_affinity"), "set_alchemical_reagent_affinity", "get_alchemical_reagent_affinity");

    ClassDB::bind_method(D_METHOD("set_specific_leaf_area_m2_kg", "val"), &FloraGuildArchetypeResource::set_specific_leaf_area_m2_kg);
    ClassDB::bind_method(D_METHOD("get_specific_leaf_area_m2_kg"), &FloraGuildArchetypeResource::get_specific_leaf_area_m2_kg);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "specific_leaf_area_m2_kg"), "set_specific_leaf_area_m2_kg", "get_specific_leaf_area_m2_kg");
}

void FloraGuildArchetypeResource::set_guild_id(int p_id) { guild_id_ = p_id; emit_changed(); }
int FloraGuildArchetypeResource::get_guild_id() const { return guild_id_; }

void FloraGuildArchetypeResource::set_trophic_role(int p_role) { trophic_role_ = p_role; emit_changed(); }
int FloraGuildArchetypeResource::get_trophic_role() const { return trophic_role_; }

void FloraGuildArchetypeResource::set_preferred_seral_stage(int p_stage) { preferred_seral_stage_ = p_stage; emit_changed(); }
int FloraGuildArchetypeResource::get_preferred_seral_stage() const { return preferred_seral_stage_; }

void FloraGuildArchetypeResource::set_guild_name(const godot::String& p_name) { guild_name_ = p_name; emit_changed(); }
godot::String FloraGuildArchetypeResource::get_guild_name() const { return guild_name_; }

void FloraGuildArchetypeResource::set_optimal_temp_c(float p_val) { optimal_temp_c_ = p_val; emit_changed(); }
float FloraGuildArchetypeResource::get_optimal_temp_c() const { return optimal_temp_c_; }

void FloraGuildArchetypeResource::set_temp_tolerance_sigma_c(float p_val) { temp_tolerance_sigma_c_ = p_val; emit_changed(); }
float FloraGuildArchetypeResource::get_temp_tolerance_sigma_c() const { return temp_tolerance_sigma_c_; }

void FloraGuildArchetypeResource::set_optimal_precip_mm(float p_val) { optimal_precip_mm_ = p_val; emit_changed(); }
float FloraGuildArchetypeResource::get_optimal_precip_mm() const { return optimal_precip_mm_; }

void FloraGuildArchetypeResource::set_precip_tolerance_sigma_mm(float p_val) { precip_tolerance_sigma_mm_ = p_val; emit_changed(); }
float FloraGuildArchetypeResource::get_precip_tolerance_sigma_mm() const { return precip_tolerance_sigma_mm_; }

void FloraGuildArchetypeResource::set_drought_wilting_point_01(float p_val) { drought_wilting_point_01_ = p_val; emit_changed(); }
float FloraGuildArchetypeResource::get_drought_wilting_point_01() const { return drought_wilting_point_01_; }

void FloraGuildArchetypeResource::set_shade_tolerance_01(float p_val) { shade_tolerance_01_ = p_val; emit_changed(); }
float FloraGuildArchetypeResource::get_shade_tolerance_01() const { return shade_tolerance_01_; }

void FloraGuildArchetypeResource::set_fire_resistance_bark_01(float p_val) { fire_resistance_bark_01_ = p_val; emit_changed(); }
float FloraGuildArchetypeResource::get_fire_resistance_bark_01() const { return fire_resistance_bark_01_; }

void FloraGuildArchetypeResource::set_serotinous_reseed_boost_01(float p_val) { serotinous_reseed_boost_01_ = p_val; emit_changed(); }
float FloraGuildArchetypeResource::get_serotinous_reseed_boost_01() const { return serotinous_reseed_boost_01_; }

void FloraGuildArchetypeResource::set_intrinsic_growth_rate_r(float p_val) { intrinsic_growth_rate_r_ = p_val; emit_changed(); }
float FloraGuildArchetypeResource::get_intrinsic_growth_rate_r() const { return intrinsic_growth_rate_r_; }

void FloraGuildArchetypeResource::set_max_carrying_biomass_kg_m2(float p_val) { max_carrying_biomass_kg_m2_ = p_val; emit_changed(); }
float FloraGuildArchetypeResource::get_max_carrying_biomass_kg_m2() const { return max_carrying_biomass_kg_m2_; }

void FloraGuildArchetypeResource::set_nitrogen_fixation_g_m2_yr(float p_val) { nitrogen_fixation_g_m2_yr_ = p_val; emit_changed(); }
float FloraGuildArchetypeResource::get_nitrogen_fixation_g_m2_yr() const { return nitrogen_fixation_g_m2_yr_; }

void FloraGuildArchetypeResource::set_litter_lignin_recalcitrance(float p_val) { litter_lignin_recalcitrance_ = p_val; emit_changed(); }
float FloraGuildArchetypeResource::get_litter_lignin_recalcitrance() const { return litter_lignin_recalcitrance_; }

void FloraGuildArchetypeResource::set_alchemical_reagent_affinity(float p_val) { alchemical_reagent_affinity_ = p_val; emit_changed(); }
float FloraGuildArchetypeResource::get_alchemical_reagent_affinity() const { return alchemical_reagent_affinity_; }

void FloraGuildArchetypeResource::set_specific_leaf_area_m2_kg(float p_val) { specific_leaf_area_m2_kg_ = p_val; emit_changed(); }
float FloraGuildArchetypeResource::get_specific_leaf_area_m2_kg() const { return specific_leaf_area_m2_kg_; }

FloraGuildArchetypePOD FloraGuildArchetypeResource::to_pod() const noexcept {
    FloraGuildArchetypePOD pod{};
    pod.guild_id = static_cast<uint16_t>(guild_id_);
    pod.trophic_role = static_cast<TrophicGuildRole>(trophic_role_);
    pod.preferred_seral_stage = static_cast<uint8_t>(preferred_seral_stage_);
    pod.optimal_temp_c = optimal_temp_c_;
    pod.temp_tolerance_sigma_c = temp_tolerance_sigma_c_;
    pod.optimal_precip_mm = optimal_precip_mm_;
    pod.precip_tolerance_sigma_mm = precip_tolerance_sigma_mm_;
    pod.drought_wilting_point_01 = drought_wilting_point_01_;
    pod.shade_tolerance_01 = shade_tolerance_01_;
    pod.fire_resistance_bark_01 = fire_resistance_bark_01_;
    pod.serotinous_reseed_boost_01 = serotinous_reseed_boost_01_;
    pod.intrinsic_growth_rate_r = intrinsic_growth_rate_r_;
    pod.max_carrying_biomass_kg_m2 = max_carrying_biomass_kg_m2_;
    pod.nitrogen_fixation_g_m2_yr = nitrogen_fixation_g_m2_yr_;
    pod.litter_lignin_recalcitrance = litter_lignin_recalcitrance_;
    pod.alchemical_reagent_affinity = alchemical_reagent_affinity_;
    pod.specific_leaf_area_m2_kg = specific_leaf_area_m2_kg_;
    return pod;
}

void FloraGuildArchetypeResource::from_pod(const FloraGuildArchetypePOD& pod) {
    guild_id_ = static_cast<int>(pod.guild_id);
    trophic_role_ = static_cast<int>(pod.trophic_role);
    preferred_seral_stage_ = static_cast<int>(pod.preferred_seral_stage);
    optimal_temp_c_ = pod.optimal_temp_c;
    temp_tolerance_sigma_c_ = pod.temp_tolerance_sigma_c;
    optimal_precip_mm_ = pod.optimal_precip_mm;
    precip_tolerance_sigma_mm_ = pod.precip_tolerance_sigma_mm;
    drought_wilting_point_01_ = pod.drought_wilting_point_01;
    shade_tolerance_01_ = pod.shade_tolerance_01;
    fire_resistance_bark_01_ = pod.fire_resistance_bark_01;
    serotinous_reseed_boost_01_ = pod.serotinous_reseed_boost_01;
    intrinsic_growth_rate_r_ = pod.intrinsic_growth_rate_r;
    max_carrying_biomass_kg_m2_ = pod.max_carrying_biomass_kg_m2;
    nitrogen_fixation_g_m2_yr_ = pod.nitrogen_fixation_g_m2_yr;
    litter_lignin_recalcitrance_ = pod.litter_lignin_recalcitrance;
    alchemical_reagent_affinity_ = pod.alchemical_reagent_affinity;
    specific_leaf_area_m2_kg_ = pod.specific_leaf_area_m2_kg;
}

// ============================================================================
// EcologyBiosphereResource
// ============================================================================

EcologyBiosphereResource::EcologyBiosphereResource() {
    seed_profile_.instantiate();
    reset_default_guild_catalog();
    generate_initial_biosphere();
}

void EcologyBiosphereResource::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_seed_profile", "profile"), &EcologyBiosphereResource::set_seed_profile);
    ClassDB::bind_method(D_METHOD("get_seed_profile"), &EcologyBiosphereResource::get_seed_profile);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "seed_profile", PROPERTY_HINT_RESOURCE_TYPE, "EcologySeedProfileResource"), "set_seed_profile", "get_seed_profile");

    ClassDB::bind_method(D_METHOD("set_guild_catalog", "catalog"), &EcologyBiosphereResource::set_guild_catalog);
    ClassDB::bind_method(D_METHOD("get_guild_catalog"), &EcologyBiosphereResource::get_guild_catalog);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "guild_catalog", PROPERTY_HINT_ARRAY_TYPE, "FloraGuildArchetypeResource"), "set_guild_catalog", "get_guild_catalog");

    ClassDB::bind_method(D_METHOD("set_default_step_dt_years", "dt"), &EcologyBiosphereResource::set_default_step_dt_years);
    ClassDB::bind_method(D_METHOD("get_default_step_dt_years"), &EcologyBiosphereResource::get_default_step_dt_years);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "default_step_dt_years", PROPERTY_HINT_RANGE, "0.02,5.0,0.02"), "set_default_step_dt_years", "get_default_step_dt_years");

    ClassDB::bind_method(D_METHOD("set_active_stage_mask", "mask"), &EcologyBiosphereResource::set_active_stage_mask);
    ClassDB::bind_method(D_METHOD("get_active_stage_mask"), &EcologyBiosphereResource::get_active_stage_mask);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "active_stage_mask"), "set_active_stage_mask", "get_active_stage_mask");

    ClassDB::bind_method(D_METHOD("get_simulation_step_counter"), &EcologyBiosphereResource::get_simulation_step_counter);
    ClassDB::bind_method(D_METHOD("reset_default_guild_catalog"), &EcologyBiosphereResource::reset_default_guild_catalog);
    ClassDB::bind_method(D_METHOD("generate_initial_biosphere"), &EcologyBiosphereResource::generate_initial_biosphere);
    ClassDB::bind_method(D_METHOD("step_simulation", "steps", "dt_years"), &EcologyBiosphereResource::step_simulation, DEFVAL(1), DEFVAL(-1.0f));
    ClassDB::bind_method(D_METHOD("ignite_wildfire_at_cell", "x", "y", "intensity_kw_m"), &EcologyBiosphereResource::ignite_wildfire_at_cell, DEFVAL(1600.0f));
    ClassDB::bind_method(D_METHOD("inspect_cell_quadrat", "x", "y"), &EcologyBiosphereResource::inspect_cell_quadrat);
    ClassDB::bind_method(D_METHOD("render_map_layer_image", "layer"), &EcologyBiosphereResource::render_map_layer_image);
    ClassDB::bind_method(D_METHOD("get_domain_telemetry"), &EcologyBiosphereResource::get_domain_telemetry);
}

void EcologyBiosphereResource::set_seed_profile(const godot::Ref<EcologySeedProfileResource>& p_profile) {
    seed_profile_ = p_profile;
    if (seed_profile_.is_null()) {
        seed_profile_.instantiate();
    }
    emit_changed();
}
godot::Ref<EcologySeedProfileResource> EcologyBiosphereResource::get_seed_profile() const {
    return seed_profile_;
}

void EcologyBiosphereResource::set_guild_catalog(const godot::TypedArray<FloraGuildArchetypeResource>& p_catalog) {
    guild_catalog_ = p_catalog;
    _sync_pod_guilds();
    emit_changed();
}
godot::TypedArray<FloraGuildArchetypeResource> EcologyBiosphereResource::get_guild_catalog() const {
    return guild_catalog_;
}

void EcologyBiosphereResource::set_default_step_dt_years(float p_dt) {
    default_step_dt_years_ = std::max(0.01f, p_dt);
    emit_changed();
}
float EcologyBiosphereResource::get_default_step_dt_years() const { return default_step_dt_years_; }

void EcologyBiosphereResource::set_active_stage_mask(int p_mask) {
    active_stage_mask_ = p_mask;
    emit_changed();
}
int EcologyBiosphereResource::get_active_stage_mask() const { return active_stage_mask_; }

void EcologyBiosphereResource::reset_default_guild_catalog() {
    auto defaults = EcologyKernelMath::build_default_guild_catalog();
    static const char* kGuildNames[8] = {
        "Boreal/Montane Conifer Canopy",
        "Temperate Broadleaf Deciduous",
        "Tropical Emergent Hardwood",
        "Sclerophyll Fire-Chaparral Scrub",
        "Prairie C4 Tussock Grass",
        "Nitrogen-Fixing Pioneer (Lupine/Alder)",
        "Riparian Gallery Willow/Cypress",
        "Karst & Old-Growth Mycorrhizal Fungi"
    };
    guild_catalog_.clear();
    for (size_t i = 0; i < defaults.size(); ++i) {
        godot::Ref<FloraGuildArchetypeResource> res;
        res.instantiate();
        res->from_pod(defaults[i]);
        res->set_guild_name(kGuildNames[i]);
        guild_catalog_.push_back(res);
    }
    _sync_pod_guilds();
}

void EcologyBiosphereResource::_sync_pod_guilds() {
    pod_guilds_.clear();
    for (int i = 0; i < guild_catalog_.size(); ++i) {
        godot::Ref<FloraGuildArchetypeResource> item = guild_catalog_[i];
        if (item.is_valid()) {
            pod_guilds_.push_back(item->to_pod());
        }
    }
    if (pod_guilds_.empty()) {
        auto defaults = EcologyKernelMath::build_default_guild_catalog();
        pod_guilds_.assign(defaults.begin(), defaults.end());
    }
}

void EcologyBiosphereResource::_ensure_buffers_allocated() {
    if (seed_profile_.is_null()) {
        seed_profile_.instantiate();
    }
    _sync_pod_guilds();
    const EcologySeedProfilePOD seed = seed_profile_->to_pod();
    const size_t cell_count = static_cast<size_t>(seed.grid_width) * static_cast<size_t>(seed.grid_height);
    pod_eco_.resize(cell_count);
    pod_biogeo_.resize(cell_count);
    pod_samples_.resize(cell_count);
}

void EcologyBiosphereResource::generate_initial_biosphere() {
    _ensure_buffers_allocated();
    const EcologySeedProfilePOD seed = seed_profile_->to_pod();
    ideam::tasks::ecology::EcologyDOD::initialize_biosphere_from_seed(
        seed,
        pod_guilds_,
        pod_eco_,
        pod_biogeo_);
    EcologyKernelMath::evaluate_ecological_harvest_and_hazards(
        seed,
        pod_eco_,
        pod_biogeo_,
        pod_samples_);
    simulation_step_counter_ = 0;
    emit_changed();
}

void EcologyBiosphereResource::step_simulation(int p_steps, float p_dt_years) {
    _ensure_buffers_allocated();
    const EcologySeedProfilePOD seed = seed_profile_->to_pod();
    const float dt = (p_dt_years > 0.0f) ? p_dt_years : default_step_dt_years_;
    const int steps = std::max(1, p_steps);

    for (int i = 0; i < steps; ++i) {
        ideam::tasks::ecology::EcologyDOD::execute_step(
            seed,
            pod_guilds_,
            pod_eco_,
            pod_biogeo_,
            pod_samples_,
            dt,
            static_cast<uint32_t>(active_stage_mask_));
        ++simulation_step_counter_;
    }
    emit_changed();
}

bool EcologyBiosphereResource::ignite_wildfire_at_cell(int p_x, int p_y, float p_intensity_kw_m) {
    _ensure_buffers_allocated();
    const EcologySeedProfilePOD seed = seed_profile_->to_pod();
    const uint32_t gx = static_cast<uint32_t>(std::clamp(p_x, 0, seed_profile_->get_grid_width() - 1));
    const uint32_t gy = static_cast<uint32_t>(std::clamp(p_y, 0, seed_profile_->get_grid_height() - 1));
    const bool ok = EcologyKernelMath::ignite_wildfire_at_grid(
        seed, gx, gy, p_intensity_kw_m, pod_eco_, pod_biogeo_);
    if (ok) {
        EcologyKernelMath::evaluate_ecological_harvest_and_hazards(
            seed, pod_eco_, pod_biogeo_, pod_samples_);
        emit_changed();
    }
    return ok;
}

godot::Dictionary EcologyBiosphereResource::inspect_cell_quadrat(int p_x, int p_y) const {
    godot::Dictionary d;
    if (seed_profile_.is_null() || pod_eco_.empty()) {
        return d;
    }
    const int w = seed_profile_->get_grid_width();
    const int h = seed_profile_->get_grid_height();
    const int cx = std::clamp(p_x, 0, w - 1);
    const int cy = std::clamp(p_y, 0, h - 1);
    const size_t idx = static_cast<size_t>(cy * w + cx);
    if (idx >= pod_eco_.size()) {
        return d;
    }

    const auto& eco = pod_eco_[idx];
    const auto& bio = pod_biogeo_[idx];
    const auto& smp = pod_samples_[idx];

    static const char* kBiomeNames[12] = {
        "Tropical Rain Forest",
        "Tropical Seasonal Forest",
        "Subtropical Thorn Steppe",
        "Hot Subtropical Desert",
        "Warm Temperate Woodland",
        "Temperate Deciduous Forest",
        "Temperate Grassland Prairie",
        "Boreal Coniferous Taiga",
        "Subalpine Krummholz Meadow",
        "Polar Alpine Tundra",
        "Riparian Wetland Gallery",
        "Karst Subterranean Refugium"
    };

    static const char* kSeralNames[7] = {
        "S0: Bare Substrate",
        "S1: Pioneer Lichen/Bryophyte",
        "S2: Early Herbaceous Meadow",
        "S3: Intermediate Shrub Scrub",
        "S4: Young Subclimax Woodland",
        "S5: Old-Growth Climax Canopy",
        "S6: Post-Disturbance Snag/Burn"
    };

    d["grid_x"] = cx;
    d["grid_y"] = cy;
    d["mean_temperature_c"] = eco.mean_temperature_c;
    d["annual_precipitation_mm"] = eco.annual_precipitation_mm;
    d["soil_moisture_01"] = eco.soil_moisture_01;
    d["canopy_biomass_kg_m2"] = eco.canopy_biomass_kg_m2;
    d["understory_biomass_kg_m2"] = eco.understory_biomass_kg_m2;
    d["root_mycorrhizal_kg_m2"] = eco.root_mycorrhizal_kg_m2;
    d["dead_fuel_litter_kg_m2"] = eco.dead_fuel_litter_kg_m2;
    d["leaf_area_index_lai"] = eco.leaf_area_index_lai;
    d["canopy_height_m"] = eco.canopy_height_m;
    d["evapotranspiration_mm_day"] = eco.evapotranspiration_mm_day;
    d["biogenic_albedo"] = eco.biogenic_albedo;
    d["shannon_diversity_h"] = eco.shannon_diversity_h;
    d["life_zone"] = static_cast<int>(eco.life_zone);
    d["life_zone_name"] = godot::String(kBiomeNames[static_cast<int>(eco.life_zone) % 12]);
    d["seral_stage"] = static_cast<int>(eco.seral_stage);
    d["seral_stage_name"] = godot::String(kSeralNames[static_cast<int>(eco.seral_stage) % 7]);
    d["disturbance_state"] = static_cast<int>(eco.disturbance_state);
    d["dominant_guild_id"] = static_cast<int>(eco.dominant_guild_id);

    d["soil_organic_carbon_kg_m2"] = bio.soil_organic_carbon_kg_m2;
    d["mineral_nitrogen_g_m2"] = bio.mineral_nitrogen_g_m2;
    d["available_phosphorus_g_m2"] = bio.available_phosphorus_g_m2;
    d["wildfire_intensity_kw_m"] = bio.wildfire_intensity_kw_m;
    d["grazer_herbivore_density"] = bio.grazer_herbivore_density;
    d["browser_herbivore_density"] = bio.browser_herbivore_density;
    d["pollinator_disperser_density"] = bio.pollinator_disperser_density;
    d["apex_predator_density"] = bio.apex_predator_density;
    d["net_primary_productivity_npp"] = bio.net_primary_productivity_npp;
    d["alchemical_phytochemical_grade"] = bio.alchemical_phytochemical_grade;
    d["wildfire_spread_risk_01"] = smp.wildfire_spread_risk_01;
    return d;
}

godot::Ref<godot::Image> EcologyBiosphereResource::render_map_layer_image(int p_layer) const {
    if (seed_profile_.is_null() || pod_eco_.empty()) {
        return godot::Ref<godot::Image>();
    }
    const int w = seed_profile_->get_grid_width();
    const int h = seed_profile_->get_grid_height();
    godot::Ref<godot::Image> img = godot::Image::create_empty(w, h, false, godot::Image::FORMAT_RGBA8);

    const auto layer_enum = static_cast<EcologyMapLayer>(p_layer);

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const size_t idx = static_cast<size_t>(y * w + x);
            const auto& eco = pod_eco_[idx];
            const auto& bio = pod_biogeo_[idx];
            const auto& smp = pod_samples_[idx];

            godot::Color c(0.12f, 0.18f, 0.14f, 1.0f);
            switch (layer_enum) {
                case EcologyMapLayer::WHITTAKER_HOLDRIDGE_BIOME: {
                    static const godot::Color kBiomeColors[12] = {
                        godot::Color(0.04f, 0.44f, 0.18f, 1.0f), // Tropical Rain Forest
                        godot::Color(0.18f, 0.58f, 0.22f, 1.0f), // Tropical Seasonal Forest
                        godot::Color(0.68f, 0.62f, 0.28f, 1.0f), // Subtropical Thorn Steppe
                        godot::Color(0.88f, 0.72f, 0.36f, 1.0f), // Hot Subtropical Desert
                        godot::Color(0.42f, 0.66f, 0.28f, 1.0f), // Warm Temperate Woodland
                        godot::Color(0.18f, 0.54f, 0.32f, 1.0f), // Temperate Deciduous Forest
                        godot::Color(0.62f, 0.76f, 0.34f, 1.0f), // Temperate Grassland Prairie
                        godot::Color(0.12f, 0.36f, 0.32f, 1.0f), // Boreal Coniferous Taiga
                        godot::Color(0.46f, 0.58f, 0.48f, 1.0f), // Subalpine Krummholz
                        godot::Color(0.78f, 0.84f, 0.88f, 1.0f), // Polar Alpine Tundra
                        godot::Color(0.14f, 0.64f, 0.58f, 1.0f), // Riparian Wetland Gallery
                        godot::Color(0.56f, 0.32f, 0.74f, 1.0f)  // Karst Subterranean Refugium
                    };
                    c = kBiomeColors[static_cast<int>(eco.life_zone) % 12];
                    break;
                }
                case EcologyMapLayer::SUCCESSION_SERAL_STAGE: {
                    const int s = static_cast<int>(eco.seral_stage);
                    if (s == 6) {
                        c = godot::Color(0.78f, 0.24f, 0.16f, 1.0f);
                    } else {
                        const float f = std::clamp(static_cast<float>(s) / 5.0f, 0.0f, 1.0f);
                        c = godot::Color(0.48f * (1.0f - 0.65f * f), 0.32f + 0.56f * f, 0.22f, 1.0f);
                    }
                    break;
                }
                case EcologyMapLayer::CANOPY_AND_UNDERSTORY_BIOMASS: {
                    const float b_norm = std::clamp((eco.canopy_biomass_kg_m2 + eco.understory_biomass_kg_m2) / 42.0f, 0.0f, 1.0f);
                    c = godot::Color(0.28f * (1.0f - b_norm), 0.22f + 0.72f * b_norm, 0.16f + 0.20f * b_norm, 1.0f);
                    break;
                }
                case EcologyMapLayer::SOIL_CARBON_AND_NITROGEN_NPK: {
                    const float c_norm = std::clamp(bio.soil_organic_carbon_kg_m2 / 22.0f, 0.0f, 1.0f);
                    const float n_norm = std::clamp(bio.mineral_nitrogen_g_m2 / 85.0f, 0.0f, 1.0f);
                    c = godot::Color(0.24f + 0.45f * c_norm, 0.20f + 0.62f * n_norm, 0.14f + 0.25f * c_norm, 1.0f);
                    break;
                }
                case EcologyMapLayer::ROTHERMEL_WILDFIRE_INTENSITY: {
                    const float risk = smp.wildfire_spread_risk_01;
                    const float flame = std::clamp(bio.wildfire_intensity_kw_m / 1200.0f, 0.0f, 1.0f);
                    c = godot::Color(0.22f + 0.78f * std::max(risk, flame), 0.18f + 0.55f * flame, 0.12f, 1.0f);
                    break;
                }
                case EcologyMapLayer::TROPHIC_HERBIVORE_PREDATOR_DENSITY: {
                    const float herb = std::clamp((bio.grazer_herbivore_density + bio.browser_herbivore_density) / 1.8f, 0.0f, 1.0f);
                    const float pred = std::clamp(bio.apex_predator_density / 0.65f, 0.0f, 1.0f);
                    c = godot::Color(0.20f + 0.75f * pred, 0.25f + 0.68f * herb, 0.28f, 1.0f);
                    break;
                }
                case EcologyMapLayer::EVAPOTRANSPIRATION_AND_ALBEDO_FEEDBACK: {
                    const float et = std::clamp(eco.evapotranspiration_mm_day / 6.5f, 0.0f, 1.0f);
                    c = godot::Color(0.16f * (1.0f - et), 0.34f + 0.55f * et, 0.42f + 0.54f * et, 1.0f);
                    break;
                }
                case EcologyMapLayer::SHANNON_BIODIVERSITY_INDEX: {
                    const float h_norm = std::clamp(eco.shannon_diversity_h / 2.2f, 0.0f, 1.0f);
                    c = godot::Color::from_hsv(0.78f - 0.52f * h_norm, 0.72f, 0.86f);
                    break;
                }
                case EcologyMapLayer::ALCHEMICAL_PHYTOCHEMICAL_POTENCY: {
                    const float alc = std::clamp(bio.alchemical_phytochemical_grade, 0.0f, 1.0f);
                    c = godot::Color(0.24f + 0.68f * alc, 0.16f + 0.72f * (alc * alc), 0.36f + 0.62f * alc, 1.0f);
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

godot::Dictionary EcologyBiosphereResource::get_domain_telemetry() const {
    godot::Dictionary d;
    if (pod_eco_.empty()) {
        return d;
    }
    float mean_canopy = 0.0f;
    float mean_soc = 0.0f;
    float mean_diversity = 0.0f;
    int active_fires = 0;

    for (size_t i = 0; i < pod_eco_.size(); ++i) {
        mean_canopy += pod_eco_[i].canopy_biomass_kg_m2;
        mean_soc += pod_biogeo_[i].soil_organic_carbon_kg_m2;
        mean_diversity += pod_eco_[i].shannon_diversity_h;
        if (pod_eco_[i].disturbance_state == EcologicalDisturbanceState::ACTIVE_CROWN_WILDFIRE ||
            pod_eco_[i].disturbance_state == EcologicalDisturbanceState::ACTIVE_SURFACE_BRUSHFIRE) {
            ++active_fires;
        }
    }
    const float inv_n = 1.0f / static_cast<float>(pod_eco_.size());
    mean_canopy *= inv_n;
    mean_soc *= inv_n;
    mean_diversity *= inv_n;

    d["cell_count"] = static_cast<int>(pod_eco_.size());
    d["simulation_steps"] = static_cast<int>(simulation_step_counter_);
    d["mean_canopy_biomass_kg_m2"] = mean_canopy;
    d["mean_soil_carbon_kg_m2"] = mean_soc;
    d["mean_shannon_diversity_h"] = mean_diversity;
    d["active_wildfire_cells"] = active_fires;
    return d;
}

// ============================================================================
// EcologyBiosphereNode3D
// ============================================================================

EcologyBiosphereNode3D::EcologyBiosphereNode3D() {
    biosphere_resource_.instantiate();
}

void EcologyBiosphereNode3D::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_biosphere_resource", "resource"), &EcologyBiosphereNode3D::set_biosphere_resource);
    ClassDB::bind_method(D_METHOD("get_biosphere_resource"), &EcologyBiosphereNode3D::get_biosphere_resource);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "biosphere_resource", PROPERTY_HINT_RESOURCE_TYPE, "EcologyBiosphereResource"), "set_biosphere_resource", "get_biosphere_resource");

    ClassDB::bind_method(D_METHOD("set_auto_generate_on_ready", "auto_generate"), &EcologyBiosphereNode3D::set_auto_generate_on_ready);
    ClassDB::bind_method(D_METHOD("get_auto_generate_on_ready"), &EcologyBiosphereNode3D::get_auto_generate_on_ready);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_generate_on_ready"), "set_auto_generate_on_ready", "get_auto_generate_on_ready");

    ClassDB::bind_method(D_METHOD("world_to_grid", "world_pos"), &EcologyBiosphereNode3D::world_to_grid);
    ClassDB::bind_method(D_METHOD("sample_canopy_height_at_world", "world_pos"), &EcologyBiosphereNode3D::sample_canopy_height_at_world);
    ClassDB::bind_method(D_METHOD("sample_quadrat_at_world", "world_pos"), &EcologyBiosphereNode3D::sample_quadrat_at_world);
}

void EcologyBiosphereNode3D::_ready() {
    if (auto_generate_on_ready_ && biosphere_resource_.is_valid() && biosphere_resource_->get_eco_const_span().empty()) {
        biosphere_resource_->generate_initial_biosphere();
    }
}

void EcologyBiosphereNode3D::set_biosphere_resource(const godot::Ref<EcologyBiosphereResource>& p_resource) {
    biosphere_resource_ = p_resource;
}
godot::Ref<EcologyBiosphereResource> EcologyBiosphereNode3D::get_biosphere_resource() const {
    return biosphere_resource_;
}

void EcologyBiosphereNode3D::set_auto_generate_on_ready(bool p_auto) {
    auto_generate_on_ready_ = p_auto;
}
bool EcologyBiosphereNode3D::get_auto_generate_on_ready() const {
    return auto_generate_on_ready_;
}

godot::Vector2i EcologyBiosphereNode3D::world_to_grid(const godot::Vector3& p_world_pos) const {
    if (biosphere_resource_.is_null() || biosphere_resource_->get_seed_profile().is_null()) {
        return godot::Vector2i(0, 0);
    }
    const godot::Vector3 local = to_local(p_world_pos);
    const auto seed = biosphere_resource_->get_seed_profile();
    const float cell_m = std::max(1.0f, seed->get_cell_size_meters());
    const int gx = std::clamp(static_cast<int>(std::floor(local.x / cell_m)), 0, seed->get_grid_width() - 1);
    const int gy = std::clamp(static_cast<int>(std::floor(local.z / cell_m)), 0, seed->get_grid_height() - 1);
    return godot::Vector2i(gx, gy);
}

float EcologyBiosphereNode3D::sample_canopy_height_at_world(const godot::Vector3& p_world_pos) const {
    if (biosphere_resource_.is_null()) {
        return 0.0f;
    }
    const godot::Vector2i g = world_to_grid(p_world_pos);
    const godot::Dictionary q = biosphere_resource_->inspect_cell_quadrat(g.x, g.y);
    return static_cast<float>(q.get("canopy_height_m", 0.0f));
}

godot::Dictionary EcologyBiosphereNode3D::sample_quadrat_at_world(const godot::Vector3& p_world_pos) const {
    if (biosphere_resource_.is_null()) {
        return godot::Dictionary();
    }
    const godot::Vector2i g = world_to_grid(p_world_pos);
    return biosphere_resource_->inspect_cell_quadrat(g.x, g.y);
}

} // namespace ideam::ecology
