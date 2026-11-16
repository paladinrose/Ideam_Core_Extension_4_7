#pragma once

/**
 * @file geomorphology_terrain_resource.h
 * @brief Godot OOP Resources and 3D Terrain Node for IdeamGeomorphology.
 *
 * Defines:
 * - GeomorphologySeedProfileResource: Hierarchical SplitMix64 seed profile & domain grid configuration.
 * - LithologyStratumResource: Physical erodibility, porosity, permeability, and Karst/Pedogenesis parameters.
 * - GeomorphologyTerrainResource: Authoritative multi-stratum terrain & hydrology state container with
 *   deterministic generation, erosion stepping, map-layer image baking, and bore-hole column inspection.
 * - GeomorphologyTerrainNode3D: Scene-tree Node3D wrapper that binds a GeomorphologyTerrainResource
 *   and provides live 3D terrain mesh / heightmap queries.
 */

#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector2i.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include "core/geomorphology/geomorphology_core.h"
#include "core/tasks/domains/geomorphology/geomorphology_dod.h"
#include "godot/utilities/ideam_undo_redo.h"

#include <vector>

namespace ideam::godot_ext {

/**
 * @class GeomorphologySeedProfileResource
 * @brief Serializable Resource wrapping core::GeomorphologySeedProfilePOD with deterministic SplitMix64 sub-seed derivation.
 */
class GeomorphologySeedProfileResource : public godot::Resource {
    GDCLASS(GeomorphologySeedProfileResource, godot::Resource)

private:
    int64_t master_seed_ = static_cast<int64_t>(0x494445414D47454FULL);
    int grid_width_ = 64;
    int grid_height_ = 64;
    int plate_count_ = 8;
    int active_strata_per_column_ = 4;
    float cell_size_meters_ = 50.0f;
    float base_crust_thickness_m_ = 1200.0f;
    float max_orogenic_uplift_m_ = 950.0f;
    float sea_level_elevation_m_ = 1150.0f;
    float tectonic_noise_frequency_ = 0.035f;
    float hydro_gravity_mps2_ = 9.81f;
    float karst_dissolution_scale_ = 1.0f;
    float pedogenesis_rate_scale_ = 1.0f;

protected:
    static void _bind_methods();

public:
    GeomorphologySeedProfileResource() = default;
    ~GeomorphologySeedProfileResource() override = default;

    void set_master_seed(int64_t p_seed);
    int64_t get_master_seed() const;

    void set_grid_width(int p_width);
    int get_grid_width() const;

    void set_grid_height(int p_height);
    int get_grid_height() const;

    void set_plate_count(int p_count);
    int get_plate_count() const;

    void set_active_strata_per_column(int p_strata);
    int get_active_strata_per_column() const;

    void set_cell_size_meters(float p_size);
    float get_cell_size_meters() const;

    void set_base_crust_thickness_m(float p_thickness);
    float get_base_crust_thickness_m() const;

    void set_max_orogenic_uplift_m(float p_uplift);
    float get_max_orogenic_uplift_m() const;

    void set_sea_level_elevation_m(float p_sea_level);
    float get_sea_level_elevation_m() const;

    void set_tectonic_noise_frequency(float p_freq);
    float get_tectonic_noise_frequency() const;

    void set_hydro_gravity_mps2(float p_gravity);
    float get_hydro_gravity_mps2() const;

    void set_karst_dissolution_scale(float p_scale);
    float get_karst_dissolution_scale() const;

    void set_pedogenesis_rate_scale(float p_scale);
    float get_pedogenesis_rate_scale() const;

    int64_t get_tectonic_seed() const;
    int64_t get_stratigraphy_seed() const;
    int64_t get_hydrology_seed() const;
    int64_t get_karst_seed() const;
    int64_t get_resource_vein_seed() const;

    core::GeomorphologySeedProfilePOD to_pod() const noexcept;
    void from_pod(const core::GeomorphologySeedProfilePOD& pod);
};

/**
 * @class LithologyStratumResource
 * @brief Serializable Resource defining physical properties of a rock or soil archetype.
 */
class LithologyStratumResource : public godot::Resource {
    GDCLASS(LithologyStratumResource, godot::Resource)

private:
    int lithology_id_ = static_cast<int>(core::CanonicalLithologyId::GRANITE);
    int genesis_class_ = static_cast<int>(core::RockGenesisClass::IGNEOUS_PLUTONIC);
    godot::String archetype_name_ = "Granite";
    float density_kg_m3_ = 2700.0f;
    float hardness_mohs_ = 6.5f;
    float fluvial_erodibility_k_ = 0.0008f;
    float talus_angle_of_repose_rad_ = 0.78f;
    float thermal_weathering_rate_ = 0.0012f;
    float porosity_ = 0.015f;
    float hydraulic_conductivity_ = 0.0005f;
    float carbonate_solubility_ = 0.0001f;
    float compressive_strength_mpa_ = 200.0f;
    float sediment_grain_diameter_mm_ = 2.5f;
    float pedogenesis_humus_retention_ = 0.15f;

protected:
    static void _bind_methods();

public:
    LithologyStratumResource() = default;
    ~LithologyStratumResource() override = default;

    void set_lithology_id(int p_id);
    int get_lithology_id() const;

    void set_genesis_class(int p_class);
    int get_genesis_class() const;

    void set_archetype_name(const godot::String& p_name);
    godot::String get_archetype_name() const;

    void set_density_kg_m3(float p_val);
    float get_density_kg_m3() const;

    void set_hardness_mohs(float p_val);
    float get_hardness_mohs() const;

    void set_fluvial_erodibility_k(float p_val);
    float get_fluvial_erodibility_k() const;

    void set_talus_angle_of_repose_rad(float p_val);
    float get_talus_angle_of_repose_rad() const;

    void set_thermal_weathering_rate(float p_val);
    float get_thermal_weathering_rate() const;

    void set_porosity(float p_val);
    float get_porosity() const;

    void set_hydraulic_conductivity(float p_val);
    float get_hydraulic_conductivity() const;

    void set_carbonate_solubility(float p_val);
    float get_carbonate_solubility() const;

    void set_compressive_strength_mpa(float p_val);
    float get_compressive_strength_mpa() const;

    void set_sediment_grain_diameter_mm(float p_val);
    float get_sediment_grain_diameter_mm() const;

    void set_pedogenesis_humus_retention(float p_val);
    float get_pedogenesis_humus_retention() const;

    core::LithologyArchetypePOD to_pod() const noexcept;
    void from_pod(const core::LithologyArchetypePOD& pod);
};

/**
 * @class GeomorphologyTerrainResource
 * @brief Authoritative terrain resource holding multi-stratum stratigraphy, crust tectonics,
 *        shallow-water hydrology, D8 drainage / Strahler order, and hazard/resource queries.
 */
class GeomorphologyTerrainResource : public godot::Resource {
    GDCLASS(GeomorphologyTerrainResource, godot::Resource)

private:
    godot::Ref<GeomorphologySeedProfileResource> seed_profile_;
    godot::TypedArray<LithologyStratumResource> lithology_catalog_;

    float default_geological_dt_myr_ = 0.02f;
    float default_hydrological_dt_sec_ = 0.25f;
    float uniform_precipitation_rate_ = 0.018f;
    int active_stage_mask_ = static_cast<int>(core::GeomorphologyStageFlags::ALL_STAGES);

    std::vector<core::LithologyArchetypePOD> pod_catalog_;
    std::vector<core::TectonicPlatePOD> pod_plates_;
    std::vector<core::CrustalTectonicCellPOD> pod_crust_;
    std::vector<core::StratigraphicColumnPOD> pod_strata_;
    std::vector<core::HydrologyCellStatePOD> pod_hydrology_;
    std::vector<core::GeomorphicHazardSamplePOD> pod_hazards_;
    uint32_t simulation_step_counter_ = 0;

    void _sync_pod_catalog();
    void _ensure_buffers_allocated();

protected:
    static void _bind_methods();

public:
    GeomorphologyTerrainResource();
    ~GeomorphologyTerrainResource() override = default;

    void set_seed_profile(const godot::Ref<GeomorphologySeedProfileResource>& p_profile);
    godot::Ref<GeomorphologySeedProfileResource> get_seed_profile() const;

    void set_lithology_catalog(const godot::TypedArray<LithologyStratumResource>& p_catalog);
    godot::TypedArray<LithologyStratumResource> get_lithology_catalog() const;

    void set_default_geological_dt_myr(float p_dt);
    float get_default_geological_dt_myr() const;

    void set_default_hydrological_dt_sec(float p_dt);
    float get_default_hydrological_dt_sec() const;

    void set_uniform_precipitation_rate(float p_rate);
    float get_uniform_precipitation_rate() const;

    void set_active_stage_mask(int p_mask);
    int get_active_stage_mask() const;

    int get_simulation_step_counter() const { return static_cast<int>(simulation_step_counter_); }

    /**
     * @brief Populates the default 8 canonical rock/soil archetypes if the catalog is empty.
     */
    void reset_default_lithology_catalog();

    /**
     * @brief Deterministically initializes plates, crust, stratigraphic columns, and D8 drainage from seed.
     */
    void generate_initial_world();

    /**
     * @brief Advances the geomorphology simulation by `p_steps` iterations.
     */
    void step_simulation(int p_steps = 1, float p_dt_geological_myr = -1.0f, float p_dt_hydrological_sec = -1.0f);

    /**
     * @brief Returns a Dictionary summarizing the bore-hole stratigraphic column and hydrology at (x, y).
     */
    godot::Dictionary inspect_cell_column(int p_x, int p_y) const;

    /**
     * @brief Renders a 2D heatmap preview image of the requested core::GeomorphologyMapLayer.
     */
    godot::Ref<godot::Image> render_map_layer_image(int p_layer) const;

    /**
     * @brief Returns global summary metrics (min/max elevation, river cells, karst cavities, max Strahler order).
     */
    godot::Dictionary get_domain_telemetry() const;

    /**
     * @brief Captures a lightweight state dictionary for IdeamUndoRedo snapshot support.
     */
    godot::Dictionary get_snapshot_state() const;
    void apply_snapshot_state(const godot::Dictionary& p_state);

    // Raw span accessors for DOD / TaskHost zero-copy bridging
    std::span<const core::LithologyArchetypePOD> get_catalog_span() const noexcept { return pod_catalog_; }
    std::span<core::TectonicPlatePOD> get_plates_span() noexcept { return pod_plates_; }
    std::span<core::CrustalTectonicCellPOD> get_crust_span() noexcept { return pod_crust_; }
    std::span<core::StratigraphicColumnPOD> get_strata_span() noexcept { return pod_strata_; }
    std::span<core::HydrologyCellStatePOD> get_hydrology_span() noexcept { return pod_hydrology_; }
    std::span<core::GeomorphicHazardSamplePOD> get_hazards_span() noexcept { return pod_hazards_; }

    std::span<const core::StratigraphicColumnPOD> get_strata_const_span() const noexcept { return pod_strata_; }
    std::span<const core::CrustalTectonicCellPOD> get_crust_const_span() const noexcept { return pod_crust_; }
    std::span<const core::HydrologyCellStatePOD> get_hydrology_const_span() const noexcept { return pod_hydrology_; }
    std::span<const core::GeomorphicHazardSamplePOD> get_hazards_const_span() const noexcept { return pod_hazards_; }
};

/**
 * @class GeomorphologyTerrainNode3D
 * @brief Scene-tree Node3D holding a GeomorphologyTerrainResource and providing spatial world queries.
 */
class GeomorphologyTerrainNode3D : public godot::Node3D {
    GDCLASS(GeomorphologyTerrainNode3D, godot::Node3D)

private:
    godot::Ref<GeomorphologyTerrainResource> terrain_resource_;
    bool auto_generate_on_ready_ = true;

protected:
    static void _bind_methods();

public:
    GeomorphologyTerrainNode3D();
    ~GeomorphologyTerrainNode3D() override = default;

    void _ready() override;

    void set_terrain_resource(const godot::Ref<GeomorphologyTerrainResource>& p_resource);
    godot::Ref<GeomorphologyTerrainResource> get_terrain_resource() const;

    void set_auto_generate_on_ready(bool p_auto);
    bool get_auto_generate_on_ready() const;

    godot::Vector2i world_to_grid(const godot::Vector3& p_world_pos) const;
    float sample_surface_elevation_at_world(const godot::Vector3& p_world_pos) const;
    godot::Dictionary sample_column_at_world(const godot::Vector3& p_world_pos) const;
};

} // namespace ideam::godot_ext
