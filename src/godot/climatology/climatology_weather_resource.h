#pragma once

/**
 * @file climatology_weather_resource.h
 * @brief Godot OOP Resources and 3D Weather Node for IdeamClimatology.
 *
 * Defines:
 * - ClimatologySeedProfileResource: Hierarchical SplitMix64 seed profile & planetary climate configuration.
 * - TornadoVortexProfileResource: Burgers-Rott / Modified Rankine supercell tornado parameters.
 * - ClimatologyAtmosphereResource: Authoritative synoptic atmosphere, Foehn cloud microphysics,
 *   Köppen-Geiger climate zones, and supercell tornado vortex state container.
 * - ClimatologyWeatherNode3D: Scene-tree Node3D wrapper providing live 3D wind vector,
 *   Rankine tornado tangential + suction force, barometric pressure, and precipitation queries.
 */

#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/vector2i.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include "core/climatology/climatology_core.h"
#include "core/tasks/domains/climatology/climatology_dod.h"

#include <vector>

namespace ideam::godot_ext {

/**
 * @class ClimatologySeedProfileResource
 * @brief Serializable Resource wrapping core::ClimatologySeedProfilePOD with SplitMix64 sub-seed derivation.
 */
class ClimatologySeedProfileResource : public godot::Resource {
    GDCLASS(ClimatologySeedProfileResource, godot::Resource)

private:
    int64_t master_seed_ = static_cast<int64_t>(0x494445414D434C4DULL);
    int grid_width_ = 64;
    int grid_height_ = 64;
    int max_vortices_ = 8;
    float cell_size_meters_ = 50.0f;
    float planet_latitude_deg_ = 38.0f;
    float seasonal_solar_decl_deg_ = 12.5f;
    float diurnal_hour_0_24_ = 14.5f;
    float solar_constant_w_m2_ = 1361.0f;
    float coriolis_parameter_f0_ = 0.00009f;
    float sea_level_pressure_hpa_ = 1013.25f;
    float sea_level_temp_k_ = 291.15f;
    float prevailing_zonal_wind_mps_ = 9.5f;
    float prevailing_merid_wind_mps_ = 2.0f;
    float synoptic_wave_amplitude_hpa_ = 14.0f;
    float orographic_lift_scale_ = 1.0f;
    float tornado_spawn_ehi_threshold_ = 1.25f;

protected:
    static void _bind_methods();

public:
    ClimatologySeedProfileResource() = default;
    ~ClimatologySeedProfileResource() override = default;

    void set_master_seed(int64_t p_seed);
    int64_t get_master_seed() const;

    void set_grid_width(int p_width);
    int get_grid_width() const;

    void set_grid_height(int p_height);
    int get_grid_height() const;

    void set_max_vortices(int p_max);
    int get_max_vortices() const;

    void set_cell_size_meters(float p_size);
    float get_cell_size_meters() const;

    void set_planet_latitude_deg(float p_lat);
    float get_planet_latitude_deg() const;

    void set_seasonal_solar_decl_deg(float p_decl);
    float get_seasonal_solar_decl_deg() const;

    void set_diurnal_hour_0_24(float p_hour);
    float get_diurnal_hour_0_24() const;

    void set_solar_constant_w_m2(float p_solar);
    float get_solar_constant_w_m2() const;

    void set_coriolis_parameter_f0(float p_f0);
    float get_coriolis_parameter_f0() const;

    void set_sea_level_pressure_hpa(float p_pres);
    float get_sea_level_pressure_hpa() const;

    void set_sea_level_temp_k(float p_temp);
    float get_sea_level_temp_k() const;

    void set_prevailing_zonal_wind_mps(float p_u);
    float get_prevailing_zonal_wind_mps() const;

    void set_prevailing_merid_wind_mps(float p_v);
    float get_prevailing_merid_wind_mps() const;

    void set_synoptic_wave_amplitude_hpa(float p_amp);
    float get_synoptic_wave_amplitude_hpa() const;

    void set_orographic_lift_scale(float p_scale);
    float get_orographic_lift_scale() const;

    void set_tornado_spawn_ehi_threshold(float p_ehi);
    float get_tornado_spawn_ehi_threshold() const;

    int64_t get_solar_insolation_seed() const;
    int64_t get_baroclinic_wave_seed() const;
    int64_t get_convective_storm_seed() const;
    int64_t get_tornado_vortex_seed() const;

    core::ClimatologySeedProfilePOD to_pod() const noexcept;
    void from_pod(const core::ClimatologySeedProfilePOD& pod);
};

/**
 * @class TornadoVortexProfileResource
 * @brief Serializable Resource configuring a Burgers-Rott / Modified Rankine tornado vortex archetype.
 */
class TornadoVortexProfileResource : public godot::Resource {
    GDCLASS(TornadoVortexProfileResource, godot::Resource)

private:
    float core_radius_m_ = 85.0f;
    float outer_damage_radius_m_ = 450.0f;
    float max_tangential_wind_mps_ = 62.0f;
    float central_pressure_deficit_hpa_ = 48.0f;
    float vertical_suction_updraft_mps_ = 38.0f;
    float radial_inflow_ratio_ = 0.42f;
    float swirl_ratio_s_ = 1.15f;
    float default_lifetime_sec_ = 240.0f;

protected:
    static void _bind_methods();

public:
    TornadoVortexProfileResource() = default;
    ~TornadoVortexProfileResource() override = default;

    void set_core_radius_m(float p_val);
    float get_core_radius_m() const;

    void set_outer_damage_radius_m(float p_val);
    float get_outer_damage_radius_m() const;

    void set_max_tangential_wind_mps(float p_val);
    float get_max_tangential_wind_mps() const;

    void set_central_pressure_deficit_hpa(float p_val);
    float get_central_pressure_deficit_hpa() const;

    void set_vertical_suction_updraft_mps(float p_val);
    float get_vertical_suction_updraft_mps() const;

    void set_radial_inflow_ratio(float p_val);
    float get_radial_inflow_ratio() const;

    void set_swirl_ratio_s(float p_val);
    float get_swirl_ratio_s() const;

    void set_default_lifetime_sec(float p_val);
    float get_default_lifetime_sec() const;

    int get_ef_rating() const;
};

/**
 * @class ClimatologyAtmosphereResource
 * @brief Authoritative climate & synoptic meteorology resource holding atmospheric columns,
 *        moisture/cloud microphysics, Köppen-Geiger zones, and active supercell tornado vortices.
 */
class ClimatologyAtmosphereResource : public godot::Resource {
    GDCLASS(ClimatologyAtmosphereResource, godot::Resource)

private:
    godot::Ref<ClimatologySeedProfileResource> seed_profile_;
    godot::Ref<TornadoVortexProfileResource> default_tornado_profile_;

    float default_synoptic_dt_sec_ = 30.0f;
    float default_vortex_dt_sec_ = 10.0f;
    int active_stage_mask_ = static_cast<int>(core::ClimatologyStageFlags::ALL_STAGES);

    std::vector<float> custom_terrain_elevations_m_;
    std::vector<core::AtmosphericColumnCellPOD> pod_atmo_;
    std::vector<core::MoistureAndCloudCellPOD> pod_moisture_;
    std::vector<core::TornadoSupercellVortexPOD> pod_vortices_;
    std::vector<core::ClimatologyHazardAndBiomeSamplePOD> pod_hazards_;
    uint32_t simulation_step_counter_ = 0;

    void _ensure_buffers_allocated();

protected:
    static void _bind_methods();

public:
    ClimatologyAtmosphereResource();
    ~ClimatologyAtmosphereResource() override = default;

    void set_seed_profile(const godot::Ref<ClimatologySeedProfileResource>& p_profile);
    godot::Ref<ClimatologySeedProfileResource> get_seed_profile() const;

    void set_default_tornado_profile(const godot::Ref<TornadoVortexProfileResource>& p_profile);
    godot::Ref<TornadoVortexProfileResource> get_default_tornado_profile() const;

    void set_default_synoptic_dt_sec(float p_dt);
    float get_default_synoptic_dt_sec() const;

    void set_default_vortex_dt_sec(float p_dt);
    float get_default_vortex_dt_sec() const;

    void set_active_stage_mask(int p_mask);
    int get_active_stage_mask() const;

    int get_simulation_step_counter() const { return static_cast<int>(simulation_step_counter_); }

    /**
     * @brief Injects an external elevation heightfield (in meters) into the atmospheric boundary layer.
     */
    void set_terrain_elevation_grid(const godot::PackedFloat32Array& p_elevations_m);

    /**
     * @brief Deterministically initializes synoptic pressure waves, winds, clouds, and Köppen zones from seed.
     */
    void generate_initial_climate();

    /**
     * @brief Advances the synoptic & mesoscale weather simulation by `p_steps` iterations.
     */
    void step_simulation(int p_steps = 1, float p_dt_synoptic_sec = -1.0f, float p_dt_vortex_sec = -1.0f);

    /**
     * @brief Spawns a Burgers-Rott / Modified Rankine supercell tornado vortex at grid cell (x, y).
     */
    bool spawn_tornado_at_cell(int p_x, int p_y, float p_max_wind_mps = -1.0f, float p_core_radius_m = -1.0f);

    /**
     * @brief Returns a Dictionary containing radiosonde / Skew-T vertical sounding & vortex telemetry at (x, y).
     */
    godot::Dictionary inspect_cell_sounding(int p_x, int p_y) const;

    /**
     * @brief Renders a 2D heatmap preview image of the requested core::ClimatologyMapLayer.
     */
    godot::Ref<godot::Image> render_map_layer_image(int p_layer) const;

    /**
     * @brief Returns global summary metrics (mean temp, max wind gust, active tornadoes, max CAPE).
     */
    godot::Dictionary get_domain_telemetry() const;

    // Raw span accessors for DOD / TaskHost zero-copy bridging
    std::span<core::AtmosphericColumnCellPOD> get_atmo_span() noexcept { return pod_atmo_; }
    std::span<core::MoistureAndCloudCellPOD> get_moisture_span() noexcept { return pod_moisture_; }
    std::span<core::TornadoSupercellVortexPOD> get_vortices_span() noexcept { return pod_vortices_; }
    std::span<core::ClimatologyHazardAndBiomeSamplePOD> get_hazards_span() noexcept { return pod_hazards_; }

    std::span<const core::AtmosphericColumnCellPOD> get_atmo_const_span() const noexcept { return pod_atmo_; }
    std::span<const core::MoistureAndCloudCellPOD> get_moisture_const_span() const noexcept { return pod_moisture_; }
    std::span<const core::TornadoSupercellVortexPOD> get_vortices_const_span() const noexcept { return pod_vortices_; }
    std::span<const core::ClimatologyHazardAndBiomeSamplePOD> get_hazards_const_span() const noexcept { return pod_hazards_; }
};

/**
 * @class ClimatologyWeatherNode3D
 * @brief Scene-tree Node3D holding a ClimatologyAtmosphereResource and providing 3D wind,
 *        tornado suction/tangential force, and precipitation sampling at world coordinates.
 */
class ClimatologyWeatherNode3D : public godot::Node3D {
    GDCLASS(ClimatologyWeatherNode3D, godot::Node3D)

private:
    godot::Ref<ClimatologyAtmosphereResource> atmosphere_resource_;
    bool auto_generate_on_ready_ = true;

protected:
    static void _bind_methods();

public:
    ClimatologyWeatherNode3D();
    ~ClimatologyWeatherNode3D() override = default;

    void _ready() override;

    void set_atmosphere_resource(const godot::Ref<ClimatologyAtmosphereResource>& p_resource);
    godot::Ref<ClimatologyAtmosphereResource> get_atmosphere_resource() const;

    void set_auto_generate_on_ready(bool p_auto);
    bool get_auto_generate_on_ready() const;

    godot::Vector2i world_to_grid(const godot::Vector3& p_world_pos) const;
    godot::Vector3 sample_wind_velocity_at_world(const godot::Vector3& p_world_pos) const;
    godot::Dictionary sample_sounding_at_world(const godot::Vector3& p_world_pos) const;
};

} // namespace ideam::godot_ext
