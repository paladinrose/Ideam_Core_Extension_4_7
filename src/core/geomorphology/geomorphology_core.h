#pragma once

/**
 * @file geomorphology_core.h
 * @brief Pure C++26 deterministic mathematical kernels for `IdeamGeomorphology` (`src/core/geomorphology/`).
 */

#include "geomorphology_types.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace ideam::core {

class GeomorphologyKernelMath {
public:
    [[nodiscard]] static constexpr uint64_t splitmix64_next(uint64_t &r_state) noexcept {
        uint64_t z = (r_state += 0x9E3779B97F4A7C15ULL);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }

    [[nodiscard]] static constexpr uint64_t hash_u64(uint64_t p_val) noexcept {
        uint64_t state = p_val;
        return splitmix64_next(state);
    }

    [[nodiscard]] static constexpr float hash_to_unit_float(uint64_t p_seed, int32_t p_x, int32_t p_y) noexcept {
        uint64_t h = p_seed;
        h ^= static_cast<uint64_t>(static_cast<uint32_t>(p_x)) * 0xD6E8FEB86659FD93ULL;
        h ^= static_cast<uint64_t>(static_cast<uint32_t>(p_y)) * 0xA5A3564E27F8865BULL;
        h = hash_u64(h);
        return static_cast<float>((h >> 40) & 0xFFFFFFULL) / static_cast<float>(0x1000000ULL);
    }

    [[nodiscard]] static constexpr GeomorphologySeedProfilePOD make_seed_profile(
        uint64_t p_master_seed,
        uint32_t p_grid_width = 64u,
        uint32_t p_grid_height = 64u,
        uint32_t p_plate_count = 8u,
        float p_cell_size_meters = 50.0f
    ) noexcept {
        GeomorphologySeedProfilePOD profile{};
        uint64_t state = (p_master_seed != 0ULL) ? p_master_seed : 0x494445414D47454FULL;
        profile.master_seed       = state;
        profile.tectonic_seed     = splitmix64_next(state);
        profile.stratigraphy_seed = splitmix64_next(state);
        profile.hydrology_seed    = splitmix64_next(state);
        profile.karst_seed        = splitmix64_next(state);
        profile.resource_vein_seed= splitmix64_next(state);
        profile.grid_width        = std::max(2u, p_grid_width);
        profile.grid_height       = std::max(2u, p_grid_height);
        profile.plate_count       = std::clamp<uint32_t>(p_plate_count, 2u, 64u);
        profile.cell_size_meters  = std::max(1.0f, p_cell_size_meters);
        return profile;
    }

    [[nodiscard]] static inline float sample_value_noise_2d(uint64_t p_seed, float p_x, float p_y) noexcept {
        const int32_t ix = static_cast<int32_t>(std::floor(p_x));
        const int32_t iy = static_cast<int32_t>(std::floor(p_y));
        const float fx   = p_x - static_cast<float>(ix);
        const float fy   = p_y - static_cast<float>(iy);
        const float ux = fx * fx * fx * (fx * (fx * 6.0f - 15.0f) + 10.0f);
        const float uy = fy * fy * fy * (fy * (fy * 6.0f - 15.0f) + 10.0f);

        const float v00 = hash_to_unit_float(p_seed, ix,     iy)     * 2.0f - 1.0f;
        const float v10 = hash_to_unit_float(p_seed, ix + 1, iy)     * 2.0f - 1.0f;
        const float v01 = hash_to_unit_float(p_seed, ix,     iy + 1) * 2.0f - 1.0f;
        const float v11 = hash_to_unit_float(p_seed, ix + 1, iy + 1) * 2.0f - 1.0f;

        const float nx0 = v00 + (v10 - v00) * ux;
        const float nx1 = v01 + (v11 - v01) * ux;
        return nx0 + (nx1 - nx0) * uy;
    }

    [[nodiscard]] static inline float sample_fbm_2d(
        uint64_t p_seed,
        float p_x,
        float p_y,
        int p_octaves = 5,
        float p_lacunarity = 2.03f,
        float p_gain = 0.5f
    ) noexcept {
        float sum  = 0.0f;
        float amp  = 1.0f;
        float freq = 1.0f;
        float norm = 0.0f;
        for (int i = 0; i < p_octaves; ++i) {
            sum  += amp * sample_value_noise_2d(p_seed + static_cast<uint64_t>(i) * 1013ULL, p_x * freq, p_y * freq);
            norm += amp;
            amp  *= p_gain;
            freq *= p_lacunarity;
        }
        return (norm > 1e-6f) ? (sum / norm) : 0.0f;
    }

    [[nodiscard]] static constexpr std::array<LithologyArchetypePOD, DEFAULT_LITHOLOGY_COUNT> build_default_lithology_catalog() noexcept {
        std::array<LithologyArchetypePOD, DEFAULT_LITHOLOGY_COUNT> cat{};
        auto make_arch = [](uint16_t id, uint8_t gen, float dens, float mohs, float erod, float talus, float weath,
                            float por, float cond, float carb, float comp, float grain, float humus, uint16_t next_meta) constexpr {
            LithologyArchetypePOD a{};
            a.lithology_id = id;
            a.genesis_class = gen;
            a.density_kg_m3 = dens;
            a.hardness_mohs = mohs;
            a.fluvial_erodibility_k = erod;
            a.talus_angle_of_repose_rad = talus;
            a.thermal_weathering_rate = weath;
            a.porosity = por;
            a.hydraulic_conductivity = cond;
            a.carbonate_solubility = carb;
            a.compressive_strength_mpa = comp;
            a.sediment_grain_diameter_mm = grain;
            a.pedogenesis_humus_retention = humus;
            a.next_metamorphic_lithology_id = next_meta;
            a.weathering_product_id = LITHOLOGY_REGOLITH;
            return a;
        };
        cat[LITHOLOGY_GRANITE]   = make_arch(LITHOLOGY_GRANITE,   ROCK_IGNEOUS_PLUTONIC,         2680.0f, 6.5f, 0.00008f, 0.78f, 0.0005f, 0.02f, 1.0e-6f, 0.0f,  200.0f, 3.0f, 0.15f, LITHOLOGY_SCHIST);
        cat[LITHOLOGY_BASALT]    = make_arch(LITHOLOGY_BASALT,    ROCK_IGNEOUS_VOLCANIC,         2980.0f, 6.0f, 0.00012f, 0.75f, 0.0008f, 0.04f, 5.0e-6f, 0.0f,  180.0f, 2.0f, 0.35f, LITHOLOGY_SCHIST);
        cat[LITHOLOGY_SANDSTONE] = make_arch(LITHOLOGY_SANDSTONE, ROCK_SEDIMENTARY_CLASTIC,      2320.0f, 4.5f, 0.00045f, 0.62f, 0.0020f, 0.18f, 2.5e-4f, 0.05f,  85.0f, 0.8f, 0.25f, LITHOLOGY_MARBLE);
        cat[LITHOLOGY_LIMESTONE] = make_arch(LITHOLOGY_LIMESTONE, ROCK_SEDIMENTARY_CHEMICAL,     2540.0f, 3.5f, 0.00038f, 0.66f, 0.0018f, 0.12f, 8.0e-5f, 0.85f, 110.0f, 1.2f, 0.30f, LITHOLOGY_MARBLE);
        cat[LITHOLOGY_SCHIST]    = make_arch(LITHOLOGY_SCHIST,    ROCK_METAMORPHIC_FOLIATED,     2760.0f, 5.0f, 0.00022f, 0.68f, 0.0010f, 0.04f, 5.0e-6f, 0.0f,  140.0f, 1.5f, 0.20f, LITHOLOGY_SCHIST);
        cat[LITHOLOGY_MARBLE]    = make_arch(LITHOLOGY_MARBLE,    ROCK_METAMORPHIC_NON_FOLIATED, 2710.0f, 4.0f, 0.00020f, 0.72f, 0.0009f, 0.03f, 2.0e-6f, 0.45f, 160.0f, 1.8f, 0.22f, LITHOLOGY_MARBLE);
        cat[LITHOLOGY_REGOLITH]  = make_arch(LITHOLOGY_REGOLITH,  ROCK_REGOLITH_ALLUVIUM,        1850.0f, 2.0f, 0.00180f, 0.52f, 0.0040f, 0.35f, 1.2e-3f, 0.02f,  12.0f, 4.0f, 0.45f, LITHOLOGY_SANDSTONE);
        cat[LITHOLOGY_LOAM_SOIL] = make_arch(LITHOLOGY_LOAM_SOIL, ROCK_SOIL_PEDOGENIC,           1450.0f, 1.5f, 0.00240f, 0.46f, 0.0050f, 0.45f, 8.5e-4f, 0.01f,   5.0f, 0.2f, 0.88f, LITHOLOGY_SANDSTONE);
        return cat;
    }

    static constexpr void recompute_column_surface_elevation(StratigraphicColumnPOD &r_col) noexcept {
        float strata_sum = 0.0f;
        uint16_t top_lith = LITHOLOGY_GRANITE;
        const uint8_t cnt = std::min<uint8_t>(r_col.active_layer_count, static_cast<uint8_t>(MAX_STRATA_PER_COLUMN));
        for (uint8_t i = 0; i < cnt; ++i) {
            const float th = std::max(0.0f, r_col.layers[i].thickness_m);
            r_col.layers[i].thickness_m = th;
            strata_sum += th;
            if (th > 0.05f) {
                top_lith = r_col.layers[i].lithology_id;
            }
        }
        r_col.regolith_thickness_m = std::max(0.0f, r_col.regolith_thickness_m);
        r_col.topsoil_thickness_m  = std::max(0.0f, r_col.topsoil_thickness_m);
        if (r_col.topsoil_thickness_m > 0.15f) {
            r_col.dominant_surface_lithology = LITHOLOGY_LOAM_SOIL;
        } else if (r_col.regolith_thickness_m > 0.35f) {
            r_col.dominant_surface_lithology = LITHOLOGY_REGOLITH;
        } else {
            r_col.dominant_surface_lithology = top_lith;
        }
        r_col.total_column_height_m = std::max(0.0f, r_col.base_elevation_m + strata_sum + r_col.regolith_thickness_m + r_col.topsoil_thickness_m - r_col.karst_cavity_void_m * 0.25f);
    }

    static inline void generate_tectonic_plates(
        const GeomorphologySeedProfilePOD &p_seed,
        std::span<TectonicPlatePOD> r_plates
    ) noexcept {
        uint64_t rng = (p_seed.tectonic_seed != 0ULL) ? p_seed.tectonic_seed : 0x544543544F4E4943ULL;
        const size_t n = r_plates.size();
        for (size_t i = 0; i < n; ++i) {
            TectonicPlatePOD p{};
            p.plate_id = static_cast<uint16_t>(i);
            const float rx = static_cast<float>((splitmix64_next(rng) >> 40) & 0xFFFFFFULL) / static_cast<float>(0x1000000ULL);
            const float ry = static_cast<float>((splitmix64_next(rng) >> 40) & 0xFFFFFFULL) / static_cast<float>(0x1000000ULL);
            const float vx = static_cast<float>((splitmix64_next(rng) >> 40) & 0xFFFFFFULL) / static_cast<float>(0x1000000ULL) * 2.0f - 1.0f;
            const float vy = static_cast<float>((splitmix64_next(rng) >> 40) & 0xFFFFFFULL) / static_cast<float>(0x1000000ULL) * 2.0f - 1.0f;
            p.seed_position_uv     = glm::vec2(0.08f + 0.84f * rx, 0.08f + 0.84f * ry);
            p.drift_velocity_m_kyr = glm::vec2(vx * 0.04f, vy * 0.04f);
            p.is_oceanic           = ((i % 3u) == 0u) ? 1u : 0u;
            p.primary_lithology_id = p.is_oceanic ? LITHOLOGY_BASALT : LITHOLOGY_GRANITE;
            p.crustal_density_kg_m3= p.is_oceanic ? 2980.0f : 2700.0f;
            p.base_thickness_m     = p.is_oceanic ? (p_seed.base_crust_thickness_m * 0.65f) : p_seed.base_crust_thickness_m;
            p.buoyancy_offset_m    = p.is_oceanic ? -220.0f : 240.0f;
            r_plates[i] = p;
        }
    }

    static inline void initialize_crust_and_stratigraphy(
        const GeomorphologySeedProfilePOD &p_seed,
        std::span<const TectonicPlatePOD> p_plates,
        std::span<const LithologyArchetypePOD> p_catalog,
        std::span<CrustalTectonicCellPOD> r_crust,
        std::span<StratigraphicColumnPOD> r_strata,
        std::span<HydrologyCellStatePOD> r_hydrology
    ) noexcept {
        (void)p_catalog;
        const uint32_t w = std::max(1u, p_seed.grid_width);
        const uint32_t h = std::max(1u, p_seed.grid_height);
        const size_t total = static_cast<size_t>(w) * static_cast<size_t>(h);
        if (r_crust.size() < total || r_strata.size() < total || r_hydrology.size() < total) return;

        for (uint32_t y = 0; y < h; ++y) {
            const float v = (static_cast<float>(y) + 0.5f) / static_cast<float>(h);
            for (uint32_t x = 0; x < w; ++x) {
                const float u = (static_cast<float>(x) + 0.5f) / static_cast<float>(w);
                const size_t idx = static_cast<size_t>(y) * w + x;
                const glm::vec2 uv(u, v);

                const float warp_x = sample_fbm_2d(p_seed.tectonic_seed + 11ULL, u * 3.2f, v * 3.2f, 3) * 0.08f;
                const float warp_y = sample_fbm_2d(p_seed.tectonic_seed + 37ULL, u * 3.2f, v * 3.2f, 3) * 0.08f;
                const glm::vec2 w_uv = uv + glm::vec2(warp_x, warp_y);

                size_t best_p = 0u;
                size_t second_p = 0u;
                float d1 = 1e9f;
                float d2 = 1e9f;
                for (size_t p = 0; p < p_plates.size(); ++p) {
                    const float d = glm::length(w_uv - p_plates[p].seed_position_uv);
                    if (d < d1) {
                        d2 = d1; second_p = best_p;
                        d1 = d;  best_p = p;
                    } else if (d < d2) {
                        d2 = d;  second_p = p;
                    }
                }

                const TectonicPlatePOD p0 = !p_plates.empty() ? p_plates[best_p] : TectonicPlatePOD{};
                const TectonicPlatePOD p1 = !p_plates.empty() ? p_plates[second_p] : p0;
                const float boundary_prox = std::exp(-std::max(0.0f, d2 - d1) * 14.0f);
                const glm::vec2 rel_v = p0.drift_velocity_m_kyr - p1.drift_velocity_m_kyr;
                const glm::vec2 n_dir = glm::normalize((p1.seed_position_uv - p0.seed_position_uv) + glm::vec2(1e-5f));
                const float conv = glm::dot(rel_v, n_dir) * 25.0f;

                CrustalTectonicCellPOD c{};
                c.primary_plate_id   = p0.plate_id;
                c.secondary_plate_id = p1.plate_id;
                c.convergent_stress  = conv * boundary_prox;
                c.boundary_type      = (boundary_prox < 0.25f) ? TECTONIC_BOUNDARY_INTERIOR
                                     : (conv > 0.15f ? TECTONIC_BOUNDARY_CONVERGENT_OROGENY
                                     : (conv < -0.15f ? TECTONIC_BOUNDARY_DIVERGENT_RIFT : TECTONIC_BOUNDARY_TRANSFORM_SHEAR));
                c.uplift_rate_mm_yr  = c.convergent_stress * 3.5f;
                c.crust_thickness_m  = p0.base_thickness_m + boundary_prox * p_seed.max_orogenic_uplift_m * 0.35f;
                c.geothermal_flux_w_m2 = 0.06f + 0.18f * boundary_prox;
                r_crust[idx] = c;

                const float fbm = sample_fbm_2d(p_seed.stratigraphy_seed, u * 4.0f, v * 4.0f, 5);
                StratigraphicColumnPOD col{};
                col.base_elevation_m   = 250.0f + p0.buoyancy_offset_m + fbm * 180.0f + boundary_prox * p_seed.max_orogenic_uplift_m * 0.45f;
                col.active_layer_count = static_cast<uint8_t>(std::clamp<uint32_t>(p_seed.active_strata_per_column, 1u, static_cast<uint32_t>(MAX_STRATA_PER_COLUMN)));
                for (uint8_t k = 0; k < col.active_layer_count; ++k) {
                    col.layers[k].lithology_id = static_cast<uint16_t>((k == 0) ? p0.primary_lithology_id : ((k + best_p) % DEFAULT_LITHOLOGY_COUNT));
                    col.layers[k].thickness_m  = 160.0f + 45.0f * static_cast<float>(col.active_layer_count - k);
                    col.layers[k].porosity_fraction = 0.05f + 0.03f * static_cast<float>(k);
                    col.layers[k].fracture_density  = boundary_prox * 0.65f;
                    col.layers[k].mineral_vein_grade= std::clamp(boundary_prox * 0.75f, 0.0f, 1.0f);
                }
                col.regolith_thickness_m = 1.2f;
                col.topsoil_thickness_m  = 0.35f;
                col.soil_organic_humus   = 0.25f;
                recompute_column_surface_elevation(col);
                r_strata[idx] = col;

                HydrologyCellStatePOD hyd{};
                if (col.total_column_height_m < p_seed.sea_level_elevation_m) {
                    hyd.water_depth_m = p_seed.sea_level_elevation_m - col.total_column_height_m;
                    hyd.is_inundated_mask = 1u;
                } else {
                    hyd.water_depth_m = 0.05f;
                }
                hyd.groundwater_table_m = std::max(0.0f, col.total_column_height_m - 4.0f);
                hyd.strahler_order = 1u;
                hyd.drainage_basin_id = static_cast<uint32_t>(best_p);
                r_hydrology[idx] = hyd;
            }
        }
    }

    static inline void step_tectonic_uplift_and_isostasy(
        const GeomorphologySeedProfilePOD &p_seed,
        std::span<const TectonicPlatePOD> p_plates,
        std::span<const LithologyArchetypePOD> p_catalog,
        float p_dt_myr,
        std::span<CrustalTectonicCellPOD> r_crust,
        std::span<StratigraphicColumnPOD> r_strata
    ) noexcept {
        (void)p_seed;
        (void)p_plates;
        (void)p_catalog;
        const size_t n = std::min(r_crust.size(), r_strata.size());
        const float dt = std::max(0.0f, p_dt_myr);
        for (size_t i = 0; i < n; ++i) {
            r_strata[i].base_elevation_m += r_crust[i].uplift_rate_mm_yr * dt * 25.0f;
            recompute_column_surface_elevation(r_strata[i]);
        }
    }

    static inline void step_shallow_water_pipe_hydrology(
        const GeomorphologySeedProfilePOD &p_seed,
        std::span<const StratigraphicColumnPOD> p_strata,
        std::span<const float> p_precip_flux,
        float p_uniform_precip_rate,
        float p_dt_sec,
        std::span<HydrologyCellStatePOD> r_hydrology
    ) noexcept {
        const uint32_t w = std::max(2u, p_seed.grid_width);
        const uint32_t h = std::max(2u, p_seed.grid_height);
        const size_t total = static_cast<size_t>(w) * static_cast<size_t>(h);
        if (p_strata.size() < total || r_hydrology.size() < total) return;

        const float dx = std::max(1.0f, p_seed.cell_size_meters);
        const float dt = std::clamp(p_dt_sec, 0.001f, 5.0f);

        for (uint32_t y = 0; y < h; ++y) {
            for (uint32_t x = 0; x < w; ++x) {
                const size_t idx = static_cast<size_t>(y) * w + x;
                HydrologyCellStatePOD &hyd = r_hydrology[idx];
                const float rain = (idx < p_precip_flux.size()) ? p_precip_flux[idx] : p_uniform_precip_rate;
                const float elev = p_strata[idx].total_column_height_m;
                if (elev < p_seed.sea_level_elevation_m) {
                    hyd.water_depth_m = p_seed.sea_level_elevation_m - elev;
                } else {
                    hyd.water_depth_m = std::max(0.0f, hyd.water_depth_m + rain * dt);
                }
                const size_t idx_e = (x + 1u < w) ? (idx + 1u) : idx;
                const size_t idx_w = (x > 0u)     ? (idx - 1u) : idx;
                const size_t idx_s = (y + 1u < h) ? (idx + w)  : idx;
                const size_t idx_n = (y > 0u)     ? (idx - w)  : idx;
                const float dh_dx = (p_strata[idx_e].total_column_height_m - p_strata[idx_w].total_column_height_m) / (2.0f * dx);
                const float dh_dy = (p_strata[idx_s].total_column_height_m - p_strata[idx_n].total_column_height_m) / (2.0f * dx);
                hyd.slope_magnitude = std::sqrt(dh_dx * dh_dx + dh_dy * dh_dy);
                hyd.volumetric_discharge_m3s = hyd.water_depth_m * (0.5f + 4.0f * hyd.slope_magnitude) * dx * 0.1f;
                hyd.stream_power_index = hyd.volumetric_discharge_m3s * hyd.slope_magnitude;
            }
        }
    }

    static inline void step_stream_power_and_talus_erosion(
        const GeomorphologySeedProfilePOD &p_seed,
        std::span<const LithologyArchetypePOD> p_catalog,
        float p_dt_sec,
        std::span<CrustalTectonicCellPOD> r_crust,
        std::span<StratigraphicColumnPOD> r_strata,
        std::span<HydrologyCellStatePOD> r_hydrology
    ) noexcept {
        (void)p_seed;
        (void)r_crust;
        const size_t n = std::min(r_strata.size(), r_hydrology.size());
        const float dt = std::clamp(p_dt_sec, 0.001f, 5.0f);
        for (size_t i = 0; i < n; ++i) {
            StratigraphicColumnPOD &col = r_strata[i];
            HydrologyCellStatePOD &hyd  = r_hydrology[i];
            const uint16_t top_id = col.top_exposed_lithology();
            const float erod_k = (top_id < p_catalog.size()) ? p_catalog[top_id].fluvial_erodibility_k : 0.0005f;
            const float incise = std::min(1.5f, erod_k * hyd.stream_power_index * dt * 20.0f);
            if (col.active_layer_count > 0u) {
                float &top_th = col.layers[col.active_layer_count - 1u].thickness_m;
                top_th = std::max(1.0f, top_th - incise);
            }
            hyd.suspended_sediment_kg += incise * 100.0f;
            recompute_column_surface_elevation(col);
        }
    }

    static inline void step_groundwater_karst_and_pedogenesis(
        const GeomorphologySeedProfilePOD &p_seed,
        std::span<const LithologyArchetypePOD> p_catalog,
        float p_dt_sec,
        std::span<StratigraphicColumnPOD> r_strata,
        std::span<HydrologyCellStatePOD> r_hydrology
    ) noexcept {
        const size_t n = std::min(r_strata.size(), r_hydrology.size());
        const float dt = std::clamp(p_dt_sec, 0.001f, 5.0f);
        for (size_t i = 0; i < n; ++i) {
            StratigraphicColumnPOD &col = r_strata[i];
            HydrologyCellStatePOD &hyd  = r_hydrology[i];
            for (uint8_t k = 0; k < col.active_layer_count && k < MAX_STRATA_PER_COLUMN; ++k) {
                const uint16_t lid = col.layers[k].lithology_id;
                const float sol = (lid < p_catalog.size()) ? p_catalog[lid].carbonate_solubility : 0.0f;
                if (sol > 0.05f) {
                    const float diss = sol * p_seed.karst_dissolution_scale * dt * 0.05f;
                    col.karst_cavity_void_m += diss;
                }
            }
            const float slope_factor = std::clamp(1.0f - hyd.slope_magnitude, 0.0f, 1.0f);
            col.soil_organic_humus = std::clamp(col.soil_organic_humus + 0.01f * p_seed.pedogenesis_rate_scale * slope_factor * dt, 0.0f, 1.0f);
            recompute_column_surface_elevation(col);
        }
    }

    static inline void compute_d8_drainage_and_strahler(
        const GeomorphologySeedProfilePOD &p_seed,
        std::span<const StratigraphicColumnPOD> p_strata,
        std::span<HydrologyCellStatePOD> r_hydrology
    ) noexcept {
        const uint32_t w = std::max(2u, p_seed.grid_width);
        const uint32_t h = std::max(2u, p_seed.grid_height);
        const size_t total = static_cast<size_t>(w) * static_cast<size_t>(h);
        if (p_strata.size() < total || r_hydrology.size() < total) return;

        static constexpr int32_t d8_dx[8] = { 0,  1, 1, 1, 0, -1, -1, -1 };
        static constexpr int32_t d8_dy[8] = {-1, -1, 0, 1, 1,  1,  0, -1 };
        static constexpr float   d8_len[8]= {1.0f, 1.41421356f, 1.0f, 1.41421356f, 1.0f, 1.41421356f, 1.0f, 1.41421356f};

        std::vector<size_t> receiver(total, total);
        std::vector<uint8_t> in_degree(total, 0u);

        for (uint32_t y = 0; y < h; ++y) {
            for (uint32_t x = 0; x < w; ++x) {
                const size_t idx = static_cast<size_t>(y) * w + x;
                const float h0   = p_strata[idx].total_column_height_m;
                float best_slope = 0.0f;
                uint8_t best_dir = 255u;
                size_t best_nb   = idx;

                for (uint8_t d = 0; d < 8u; ++d) {
                    const int32_t nx = static_cast<int32_t>(x) + d8_dx[d];
                    const int32_t ny = static_cast<int32_t>(y) + d8_dy[d];
                    if (nx < 0 || ny < 0 || nx >= static_cast<int32_t>(w) || ny >= static_cast<int32_t>(h)) continue;
                    const size_t n_idx = static_cast<size_t>(ny) * w + static_cast<size_t>(nx);
                    const float slope = (h0 - p_strata[n_idx].total_column_height_m) / d8_len[d];
                    if (slope > best_slope) {
                        best_slope = slope;
                        best_dir   = d;
                        best_nb    = n_idx;
                    }
                }
                r_hydrology[idx].receiver_dir_d8 = best_dir;
                r_hydrology[idx].strahler_order  = 1u;
                r_hydrology[idx].upstream_catchment_cells = 1u;
                if (best_nb != idx) {
                    receiver[idx] = best_nb;
                    in_degree[best_nb]++;
                }
            }
        }

        std::vector<size_t> queue;
        queue.reserve(total);
        std::vector<uint8_t> max_ord(total, 0u);
        std::vector<uint8_t> max_cnt(total, 0u);

        for (size_t i = 0; i < total; ++i) {
            if (in_degree[i] == 0u) queue.push_back(i);
        }

        size_t head = 0u;
        while (head < queue.size()) {
            const size_t u = queue[head++];
            const uint8_t u_ord = r_hydrology[u].strahler_order;
            const size_t v = receiver[u];
            if (v < total && v != u) {
                r_hydrology[v].upstream_catchment_cells += r_hydrology[u].upstream_catchment_cells;
                if (u_ord > max_ord[v]) {
                    max_ord[v] = u_ord;
                    max_cnt[v] = 1u;
                } else if (u_ord == max_ord[v]) {
                    max_cnt[v]++;
                }
                if (--in_degree[v] == 0u) {
                    r_hydrology[v].strahler_order = (max_cnt[v] >= 2u)
                        ? static_cast<uint8_t>(std::min<int>(8, max_ord[v] + 1))
                        : std::max<uint8_t>(1u, max_ord[v]);
                    queue.push_back(v);
                }
            }
        }

        for (size_t i = 0; i < total; ++i) {
            size_t curr = i;
            for (uint32_t hop = 0; hop < 64u && receiver[curr] < total && receiver[curr] != curr; ++hop) {
                curr = receiver[curr];
            }
            r_hydrology[i].drainage_basin_id = static_cast<uint32_t>(curr);
            r_hydrology[i].basin_class = (p_strata[i].total_column_height_m < p_seed.sea_level_elevation_m)
                ? BASIN_EXORHEIC_OCEANIC
                : ((r_hydrology[i].strahler_order >= 2u) ? BASIN_FLUVIAL_CHANNEL : BASIN_ENDORHEIC_CLOSED);
        }
    }

    static inline void evaluate_hazards_and_resource_veins(
        const GeomorphologySeedProfilePOD &p_seed,
        std::span<const CrustalTectonicCellPOD> p_crust,
        std::span<const StratigraphicColumnPOD> p_strata,
        std::span<const HydrologyCellStatePOD> p_hydrology,
        std::span<GeomorphicHazardSamplePOD> r_hazards
    ) noexcept {
        (void)p_seed;
        const size_t n = std::min({p_crust.size(), p_strata.size(), p_hydrology.size(), r_hazards.size()});
        for (size_t i = 0; i < n; ++i) {
            GeomorphicHazardSamplePOD h{};
            h.cell_index = static_cast<uint32_t>(i);
            h.landslide_hazard_probability = std::clamp(p_hydrology[i].slope_magnitude * 0.75f, 0.0f, 1.0f);
            h.karst_sinkhole_hazard        = std::clamp(p_strata[i].karst_cavity_void_m / 6.0f, 0.0f, 1.0f);
            h.flash_flood_hazard           = std::clamp(p_hydrology[i].volumetric_discharge_m3s / 25.0f, 0.0f, 1.0f);
            h.hydrothermal_ore_grade       = std::clamp(p_crust[i].geothermal_flux_w_m2 * 3.5f, 0.0f, 1.0f);
            h.placer_alluvial_grade        = (p_hydrology[i].strahler_order >= 2u)
                ? std::clamp(p_hydrology[i].suspended_sediment_kg * 0.01f, 0.0f, 1.0f)
                : 0.0f;
            h.evaporite_brine_grade        = (p_hydrology[i].basin_class == BASIN_ENDORHEIC_CLOSED) ? 0.55f : 0.05f;
            r_hazards[i] = h;
        }
    }
};

} // namespace ideam::core
