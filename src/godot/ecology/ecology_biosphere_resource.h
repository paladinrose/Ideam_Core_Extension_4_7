#pragma once

/**
 * @file ecology_biosphere_resource.h
 * @brief Godot OOP Resources and 3D Biosphere Node for IdeamEcology.
 *
 * Defines:
 * - EcologySeedProfileResource: Hierarchical SplitMix64 seed profile & ecophysiological configuration.
 * - FloraGuildArchetypeResource: Functional plant/fungi guild parameters (Whittaker niche, shade/fire tolerance, N-fixation).
 * - EcologyBiosphereResource: Authoritative biosphere container holding Holdridge life zones, seral succession stages,
 *   canopy/understory/root/litter biomass, Century-lite soil C/N/P pools, Rothermel wildfire state, and Lotka-Volterra trophic densities.
 * - EcologyBiosphereNode3D: Scene-tree Node3D wrapper providing spatial biome, canopy height, forage, wildfire,
 *   and alchemical phytochemical potency queries at world coordinates.
 */

#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector2i.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include "core/ecology/ecology_core.h"
#include "core/tasks/domains/ecology/ecology_dod.h"

#include <vector>

namespace ideam::godot_ext {

/**
 * @class EcologySeedProfileResource
 * @brief Serializable Resource wrapping core::EcologySeedProfilePOD with SplitMix64 sub-seed derivation.
 */
class EcologySeedProfileResource : public godot::Resource {
    GDCLASS(EcologySeedProfileResource, godot::Resource)

private:
    int64_t master_seed_ = static_cast<int64_t>(0x494445414D45434FULL);
    int grid_width_ = 64;
    int grid_height_ = 64;
    int active_guild_count_ = 8;
    float cell_size_meters_ = 50.0f;
    float ambient_co2_ppm_ = 410.0f;
    float beer_lambert_extinction_k_ = 0.52f;
    float base_npp_efficiency_ = 1.0f;
    float q10_decomposition_factor_ = 2.1f;
    float nitrogen_fixation_scale_ = 1.0f;
    float rothermel_wind_spread_scale_ = 1.0f;
    float herbivore_grazing_rate_ = 0.14f;
    float predator_attack_rate_ = 0.22f;
    float predator_conversion_eff_ = 0.28f;
    float alchemical_leyline_resonance_ = 1.0f;

protected:
    static void _bind_methods();

public:
    EcologySeedProfileResource() = default;
    ~EcologySeedProfileResource() override = default;

    void set_master_seed(int64_t p_seed);
    int64_t get_master_seed() const;

    void set_grid_width(int p_width);
    int get_grid_width() const;

    void set_grid_height(int p_height);
    int get_grid_height() const;

    void set_active_guild_count(int p_count);
    int get_active_guild_count() const;

    void set_cell_size_meters(float p_size);
    float get_cell_size_meters() const;

    void set_ambient_co2_ppm(float p_co2);
    float get_ambient_co2_ppm() const;

    void set_beer_lambert_extinction_k(float p_k);
    float get_beer_lambert_extinction_k() const;

    void set_base_npp_efficiency(float p_eff);
    float get_base_npp_efficiency() const;

    void set_q10_decomposition_factor(float p_q10);
    float get_q10_decomposition_factor() const;

    void set_nitrogen_fixation_scale(float p_scale);
    float get_nitrogen_fixation_scale() const;

    void set_rothermel_wind_spread_scale(float p_scale);
    float get_rothermel_wind_spread_scale() const;

    void set_herbivore_grazing_rate(float p_rate);
    float get_herbivore_grazing_rate() const;

    void set_predator_attack_rate(float p_rate);
    float get_predator_attack_rate() const;

    void set_predator_conversion_eff(float p_eff);
    float get_predator_conversion_eff() const;

    void set_alchemical_leyline_resonance(float p_res);
    float get_alchemical_leyline_resonance() const;

    int64_t get_whittaker_niche_seed() const;
    int64_t get_succession_dispersal_seed() const;
    int64_t get_trophic_lotka_volterra_seed() const;
    int64_t get_rothermel_wildfire_seed() const;

    core::EcologySeedProfilePOD to_pod() const noexcept;
    void from_pod(const core::EcologySeedProfilePOD& pod);
};

/**
 * @class FloraGuildArchetypeResource
 * @brief Serializable Resource defining ecophysiological parameters of a functional plant/fungi guild.
 */
class FloraGuildArchetypeResource : public godot::Resource {
    GDCLASS(FloraGuildArchetypeResource, godot::Resource)

private:
    int guild_id_ = 0;
    int trophic_role_ = static_cast<int>(core::TrophicGuildRole::PRIMARY_PRODUCER_CANOPY);
    int preferred_seral_stage_ = 5;
    godot::String guild_name_ = "Temperate Broadleaf Canopy";
    float optimal_temp_c_ = 14.5f;
    float temp_tolerance_sigma_c_ = 8.0f;
    float optimal_precip_mm_ = 1150.0f;
    float precip_tolerance_sigma_mm_ = 550.0f;
    float drought_wilting_point_01_ = 0.22f;
    float shade_tolerance_01_ = 0.72f;
    float fire_resistance_bark_01_ = 0.45f;
    float serotinous_reseed_boost_01_ = 0.25f;
    float intrinsic_growth_rate_r_ = 0.30f;
    float max_carrying_biomass_kg_m2_ = 36.0f;
    float nitrogen_fixation_g_m2_yr_ = 0.8f;
    float litter_lignin_recalcitrance_ = 0.34f;
    float alchemical_reagent_affinity_ = 0.62f;
    float specific_leaf_area_m2_kg_ = 0.26f;

protected:
    static void _bind_methods();

public:
    FloraGuildArchetypeResource() = default;
    ~FloraGuildArchetypeResource() override = default;

    void set_guild_id(int p_id);
    int get_guild_id() const;

    void set_trophic_role(int p_role);
    int get_trophic_role() const;

    void set_preferred_seral_stage(int p_stage);
    int get_preferred_seral_stage() const;

    void set_guild_name(const godot::String& p_name);
    godot::String get_guild_name() const;

    void set_optimal_temp_c(float p_val);
    float get_optimal_temp_c() const;

    void set_temp_tolerance_sigma_c(float p_val);
    float get_temp_tolerance_sigma_c() const;

    void set_optimal_precip_mm(float p_val);
    float get_optimal_precip_mm() const;

    void set_precip_tolerance_sigma_mm(float p_val);
    float get_precip_tolerance_sigma_mm() const;

    void set_drought_wilting_point_01(float p_val);
    float get_drought_wilting_point_01() const;

    void set_shade_tolerance_01(float p_val);
    float get_shade_tolerance_01() const;

    void set_fire_resistance_bark_01(float p_val);
    float get_fire_resistance_bark_01() const;

    void set_serotinous_reseed_boost_01(float p_val);
    float get_serotinous_reseed_boost_01() const;

    void set_intrinsic_growth_rate_r(float p_val);
    float get_intrinsic_growth_rate_r() const;

    void set_max_carrying_biomass_kg_m2(float p_val);
    float get_max_carrying_biomass_kg_m2() const;

    void set_nitrogen_fixation_g_m2_yr(float p_val);
    float get_nitrogen_fixation_g_m2_yr() const;

    void set_litter_lignin_recalcitrance(float p_val);
    float get_litter_lignin_recalcitrance() const;

    void set_alchemical_reagent_affinity(float p_val);
    float get_alchemical_reagent_affinity() const;

    void set_specific_leaf_area_m2_kg(float p_val);
    float get_specific_leaf_area_m2_kg() const;

    core::FloraGuildArchetypePOD to_pod() const noexcept;
    void from_pod(const core::FloraGuildArchetypePOD& pod);
};

/**
 * @class EcologyBiosphereResource
 * @brief Authoritative biosphere resource holding Holdridge life zones, seral stages,
 *        canopy/understory/root/litter biomass, soil C/N/P pools, wildfire contagion,
 *        and Lotka-Volterra trophic populations.
 */
class EcologyBiosphereResource : public godot::Resource {
    GDCLASS(EcologyBiosphereResource, godot::Resource)

private:
    godot::Ref<EcologySeedProfileResource> seed_profile_;
    godot::TypedArray<FloraGuildArchetypeResource> guild_catalog_;

    float default_step_dt_years_ = 0.25f;
    int active_stage_mask_ = static_cast<int>(core::EcologyStageFlags::ALL_STAGES);

    std::vector<core::FloraGuildArchetypePOD> pod_guilds_;
    std::vector<core::EcologicalCellStatePOD> pod_eco_;
    std::vector<core::BiogeochemicalAndTrophicCellPOD> pod_biogeo_;
    std::vector<core::EcologyHarvestAndDisturbanceSamplePOD> pod_samples_;
    uint32_t simulation_step_counter_ = 0;

    void _sync_pod_guilds();
    void _ensure_buffers_allocated();

protected:
    static void _bind_methods();

public:
    EcologyBiosphereResource();
    ~EcologyBiosphereResource() override = default;

    void set_seed_profile(const godot::Ref<EcologySeedProfileResource>& p_profile);
    godot::Ref<EcologySeedProfileResource> get_seed_profile() const;

    void set_guild_catalog(const godot::TypedArray<FloraGuildArchetypeResource>& p_catalog);
    godot::TypedArray<FloraGuildArchetypeResource> get_guild_catalog() const;

    void set_default_step_dt_years(float p_dt);
    float get_default_step_dt_years() const;

    void set_active_stage_mask(int p_mask);
    int get_active_stage_mask() const;

    int get_simulation_step_counter() const { return static_cast<int>(simulation_step_counter_); }

    /**
     * @brief Populates the default 8 canonical flora/fungi guilds.
     */
    void reset_default_guild_catalog();

    /**
     * @brief Deterministically initializes biomes, biomass, soil NPK, and trophic densities from seed.
     */
    void generate_initial_biosphere();

    /**
     * @brief Advances the ecological succession, wildfire, and trophic web simulation by `p_steps` iterations.
     */
    void step_simulation(int p_steps = 1, float p_dt_years = -1.0f);

    /**
     * @brief Ignites a Rothermel wildfire at grid cell (x, y) for interactive testing.
     */
    bool ignite_wildfire_at_cell(int p_x, int p_y, float p_intensity_kw_m = 1600.0f);

    /**
     * @brief Returns a Dictionary summarizing quadrat biomass, Whittaker ordination, soil NPK, and trophic densities at (x, y).
     */
    godot::Dictionary inspect_cell_quadrat(int p_x, int p_y) const;

    /**
     * @brief Renders a 2D heatmap preview image of the requested core::EcologyMapLayer.
     */
    godot::Ref<godot::Image> render_map_layer_image(int p_layer) const;

    /**
     * @brief Returns global summary metrics (mean canopy biomass, total soil carbon, active wildfire cells, mean diversity).
     */
    godot::Dictionary get_domain_telemetry() const;

    // Raw span accessors for DOD / TaskHost zero-copy bridging
    std::span<const core::FloraGuildArchetypePOD> get_guilds_span() const noexcept { return pod_guilds_; }
    std::span<core::EcologicalCellStatePOD> get_eco_span() noexcept { return pod_eco_; }
    std::span<core::BiogeochemicalAndTrophicCellPOD> get_biogeo_span() noexcept { return pod_biogeo_; }
    std::span<core::EcologyHarvestAndDisturbanceSamplePOD> get_samples_span() noexcept { return pod_samples_; }

    std::span<const core::EcologicalCellStatePOD> get_eco_const_span() const noexcept { return pod_eco_; }
    std::span<const core::BiogeochemicalAndTrophicCellPOD> get_biogeo_const_span() const noexcept { return pod_biogeo_; }
    std::span<const core::EcologyHarvestAndDisturbanceSamplePOD> get_samples_const_span() const noexcept { return pod_samples_; }
};

/**
 * @class EcologyBiosphereNode3D
 * @brief Scene-tree Node3D holding an EcologyBiosphereResource and providing spatial biome,
 *        canopy height, forage, and alchemical botanical queries at world coordinates.
 */
class EcologyBiosphereNode3D : public godot::Node3D {
    GDCLASS(EcologyBiosphereNode3D, godot::Node3D)

private:
    godot::Ref<EcologyBiosphereResource> biosphere_resource_;
    bool auto_generate_on_ready_ = true;

protected:
    static void _bind_methods();

public:
    EcologyBiosphereNode3D();
    ~EcologyBiosphereNode3D() override = default;

    void _ready() override;

    void set_biosphere_resource(const godot::Ref<EcologyBiosphereResource>& p_resource);
    godot::Ref<EcologyBiosphereResource> get_biosphere_resource() const;

    void set_auto_generate_on_ready(bool p_auto);
    bool get_auto_generate_on_ready() const;

    godot::Vector2i world_to_grid(const godot::Vector3& p_world_pos) const;
    float sample_canopy_height_at_world(const godot::Vector3& p_world_pos) const;
    godot::Dictionary sample_quadrat_at_world(const godot::Vector3& p_world_pos) const;
};

} // namespace ideam::godot_ext
