#pragma once

/**
 * geomorphology_core.h
 * Pure C++26 deterministic mathematical kernels for `IdeamGeomorphology` (`src/core/geomorphology/`):
 *   1. SplitMix64 hierarchical seed derivation & 2D Simplex/Value FBM + Ridged Multifractal noise
 *   2. Default 8-archetype lithology catalog (`Granite`, `Basalt`, `Sandstone`, `Limestone`,
 *      `Schist`, `Marble`, `Regolith`, `Loam Soil`)
 *   3. Plate-tectonic Voronoi partitioning, boundary stress synthesis, Airy isostasy & orogeny (`GEO_T1`)
 *   4. Mass-conserving 2D Shallow-Water Virtual Pipe Hydrology (`GEO_T2`)
 *   5. Multi-stratum Stream-Power fluvial incision, sediment advection & Thermal Talus slumping (`GEO_T3`)
 *   6. Darcy groundwater percolation, Carbonate Karst dissolution/collapse & Soil Pedogenesis (`GEO_T4`)
 *   7. D8 drainage network, pit/lake classification, watershed basin flood-fill & Strahler order (`GEO_M1`)
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
    // ========================================================================
    // 1. Hierarchical Seed Derivation (SplitMix64) & Procedural Noise
    // ========================================================================

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

    [[nodiscard]] static constexpr GeomorphologySeedProfilePOD derive_seed_profile(
        uint64_t p_master_seed,
        uint32_t p_plate_count = 8u,
        float p_sea_level_m = 0.0f
    ) noexcept {
        GeomorphologySeedProfilePOD profile{};
        uint64_t state = (p_master_seed != 0ULL) ? p_master_seed : 0x494445414D47454FULL;
        profile.master_seed           = state;
        profile.tectonic_seed         = splitmix64_next(state);
        profile.lithology_seed        = splitmix64_next(state);
        profile.hydrology_seed        = splitmix64_next(state);
        profile.erosion_pedology_seed = splitmix64_next(state);
        profile.tectonic_plate_count  = std::clamp<uint32_t>(p_plate_count, 2u, 64u);
        profile.sea_level_m           = p_sea_level_m;
        return profile;
    }

    [[nodiscard]] static inline float sample_value_noise_2d(uint64_t p_seed, float p_x, float p_y) noexcept {
        const int32_t ix = static_cast<int32_t>(std::floor(p_x));
        const int32_t iy = static_cast<int32_t>(std::floor(p_y));
        const float fx   = p_x - static_cast<float>(ix);
        const float fy   = p_y - static_cast<float>(iy);

        // Quintic C2 smoothstep u = 6t^5 - 15t^4 + 10t^3
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

    [[nodiscard]] static inline float sample_ridged_multifractal_2d(
        uint64_t p_seed,
        float p_x,
        float p_y,
        int p_octaves = 4,
        float p_lacunarity = 2.1f,
        float p_gain = 0.5f
    ) noexcept {
        float sum    = 0.0f;
        float amp    = 0.5f;
        float freq   = 1.0f;
        float weight = 1.0f;
        for (int i = 0; i < p_octaves; ++i) {
            float n = 1.0f - std::abs(sample_value_noise_2d(p_seed + static_cast<uint64_t>(i) * 2749ULL, p_x * freq, p_y * freq));
            n = n * n * weight;
            weight = std::clamp(n * 2.0f, 0.0f, 1.0f);
            sum += n * amp;
            freq *= p_lacunarity;
            amp  *= p_gain;
        }
        return sum;
    }

    // ========================================================================
    // 2. Default 8-Entry Lithology Catalog
    // ========================================================================

    [[nodiscard]] static constexpr std::array<LithologyArchetypePOD, DEFAULT_LITHOLOGY_COUNT> build_default_lithology_catalog() noexcept {
        std::array<LithologyArchetypePOD, DEFAULT_LITHOLOGY_COUNT> cat{};

        // 0: Granite (Plutonic Felsic Basement)
        cat[LITHOLOGY_GRANITE] = LithologyArchetypePOD{
            LITHOLOGY_GRANITE, ROCK_IGNEOUS_PLUTONIC, 0u,
            LITHOLOGY_SCHIST, LITHOLOGY_REGOLITH,
            2680.0f, 200.0f, 0.00008f, 0.78f, 1.0e-7f, 0.02f, 0.0f, 0.72f, 0.25f, 300.0f,
            glm::vec4(0.68f, 0.56f, 0.54f, 1.0f)
        };

        // 1: Basalt (Volcanic Mafic Oceanic / Flood Basalt)
        cat[LITHOLOGY_BASALT] = LithologyArchetypePOD{
            LITHOLOGY_BASALT, ROCK_IGNEOUS_VOLCANIC, 0u,
            LITHOLOGY_SCHIST, LITHOLOGY_REGOLITH,
            2980.0f, 180.0f, 0.00012f, 0.75f, 5.0e-7f, 0.04f, 0.0f, 0.48f, 0.65f, 260.0f,
            glm::vec4(0.24f, 0.26f, 0.30f, 1.0f)
        };

        // 2: Sandstone (Clastic Sedimentary)
        cat[LITHOLOGY_SANDSTONE] = LithologyArchetypePOD{
            LITHOLOGY_SANDSTONE, ROCK_SEDIMENTARY_CLASTIC, 0u,
            LITHOLOGY_MARBLE, LITHOLOGY_REGOLITH,
            2320.0f, 75.0f, 0.00045f, 0.62f, 2.5e-4f, 0.22f, 0.02f, 0.80f, 0.30f, 140.0f,
            glm::vec4(0.84f, 0.70f, 0.46f, 1.0f)
        };

        // 3: Limestone (Chemical Carbonate Sedimentary — Karst Soluble)
        cat[LITHOLOGY_LIMESTONE] = LithologyArchetypePOD{
            LITHOLOGY_LIMESTONE, ROCK_SEDIMENTARY_CHEMICAL, 0u,
            LITHOLOGY_MARBLE, LITHOLOGY_REGOLITH,
            2550.0f, 95.0f, 0.00032f, 0.66f, 8.0e-5f, 0.14f, 0.85f, 0.10f, 0.45f, 150.0f,
            glm::vec4(0.78f, 0.80f, 0.74f, 1.0f)
        };

        // 4: Schist / Gneiss (Foliated Metamorphic Orogenic Core)
        cat[LITHOLOGY_SCHIST] = LithologyArchetypePOD{
            LITHOLOGY_SCHIST, ROCK_METAMORPHIC_FOLIATED, 0u,
            LITHOLOGY_SCHIST, LITHOLOGY_REGOLITH,
            2780.0f, 175.0f, 0.00014f, 0.72f, 4.0e-7f, 0.03f, 0.0f, 0.62f, 0.35f, 400.0f,
            glm::vec4(0.46f, 0.44f, 0.52f, 1.0f)
        };

        // 5: Marble / Quartzite (Non-Foliated Metamorphic)
        cat[LITHOLOGY_MARBLE] = LithologyArchetypePOD{
            LITHOLOGY_MARBLE, ROCK_METAMORPHIC_NON_FOLIATED, 0u,
            LITHOLOGY_MARBLE, LITHOLOGY_REGOLITH,
            2710.0f, 165.0f, 0.00015f, 0.74f, 2.0e-7f, 0.02f, 0.25f, 0.40f, 0.30f, 380.0f,
            glm::vec4(0.90f, 0.88f, 0.92f, 1.0f)
        };

        // 6: Regolith / Alluvium (Unconsolidated Talus & Gravel)
        cat[LITHOLOGY_REGOLITH] = LithologyArchetypePOD{
            LITHOLOGY_REGOLITH, ROCK_REGOLITH_ALLUVIUM, 0u,
            LITHOLOGY_SANDSTONE, LITHOLOGY_LOAM_SOIL,
            1850.0f, 8.0f, 0.00180f, 0.54f, 1.5e-3f, 0.36f, 0.0f, 0.55f, 0.55f, 60.0f,
            glm::vec4(0.60f, 0.54f, 0.45f, 1.0f)
        };

        // 7: Loam Topsoil (Pedogenic O/A/B Horizon)
        cat[LITHOLOGY_LOAM_SOIL] = LithologyArchetypePOD{
            LITHOLOGY_LOAM_SOIL, ROCK_SOIL_PEDOGENIC, 0u,
            LITHOLOGY_SANDSTONE, LITHOLOGY_LOAM_SOIL,
            1450.0f, 2.0f, 0.00280f, 0.48f, 6.0e-4f, 0.45f, 0.0f, 0.45f, 0.92f, 40.0f,
            glm::vec4(0.32f, 0.46f, 0.24f, 1.0f)
        };

        return cat;
    }

    // ========================================================================
    // 3. Stratigraphic Column Helpers
    // ========================================================================

    static inline void recompute_column_surface_elevation(StratigraphicColumnPOD &r_col) noexcept {
        const uint8_t count = std::min<uint8_t>(r_col.active_layer_count, static_cast<uint8_t>(MAX_STRATA_PER_COLUMN));
        float total_rock = 0.0f;
        uint16_t top_lith = LITHOLOGY_GRANITE;
        for (uint8_t i = 0u; i < count; ++i) {
            const float th = std::max(0.0f, r_col.layers[i].thickness_m);
            r_col.layers[i].thickness_m = th;
            total_rock += th;
            if (th > 0.01f) {
                top_lith = r_col.layers[i].lithology_id;
            }
        }
        r_col.regolith_thickness_m = std::max(0.0f, r_col.regolith_thickness_m);
        r_col.topsoil_thickness_m  = std::max(0.0f, r_col.topsoil_thickness_m);
        r_col.dominant_surface_lithology = (r_col.topsoil_thickness_m > 0.25f)
            ? LITHOLOGY_LOAM_SOIL
            : ((r_col.regolith_thickness_m > 0.5f) ? LITHOLOGY_REGOLITH : top_lith);
        r_col.surface_elevation_m =
            r_col.basement_elevation_m + total_rock + r_col.regolith_thickness_m + r_col.topsoil_thickness_m;
    }

    /**
     * Erodes `p_depth_m` meters from the top of the stratigraphic column downward:
     * first stripping `topsoil_thickness_m`, then `regolith_thickness_m`, then the uppermost
     * populated bedrock strata in `layers[active_layer_count - 1 .. 0]`.
     */
    static inline float erode_column_from_top(StratigraphicColumnPOD &r_col, float p_depth_m) noexcept {
        if (p_depth_m <= 0.0f) return 0.0f;
        float rem = p_depth_m;
        float removed = 0.0f;

        if (r_col.topsoil_thickness_m > 0.0f) {
            const float take = std::min(r_col.topsoil_thickness_m, rem);
            r_col.topsoil_thickness_m -= take;
            rem     -= take;
            removed += take;
        }
        if (rem > 0.0f && r_col.regolith_thickness_m > 0.0f) {
            const float take = std::min(r_col.regolith_thickness_m, rem);
            r_col.regolith_thickness_m -= take;
            rem     -= take;
            removed += take;
        }

        const uint8_t count = std::min<uint8_t>(r_col.active_layer_count, static_cast<uint8_t>(MAX_STRATA_PER_COLUMN));
        for (int i = static_cast<int>(count) - 1; i >= 0 && rem > 0.0f; --i) {
            // Keep a minimum 5m deep basement floor on layer 0
            const float min_floor = (i == 0) ? 5.0f : 0.0f;
            const float avail = std::max(0.0f, r_col.layers[i].thickness_m - min_floor);
            const float take  = std::min(avail, rem);
            r_col.layers[i].thickness_m -= take;
            rem     -= take;
            removed += take;
        }

        r_col.cumulative_erosion_delta_m -= removed;
        recompute_column_surface_elevation(r_col);
        return removed;
    }

    static inline void deposit_sediment_on_column(StratigraphicColumnPOD &r_col, float p_depth_m) noexcept {
        if (p_depth_m <= 0.0f) return;
        r_col.regolith_thickness_m += p_depth_m;
        r_col.cumulative_erosion_delta_m += p_depth_m;
        recompute_column_surface_elevation(r_col);
    }

    // ========================================================================
    // 4. Initial World Synthesis from `GeomorphologySeedProfilePOD`
    // ========================================================================

    static inline void synthesize_initial_plates_and_columns(
        const GeomorphologySeedProfilePOD &p_seed_profile,
        uint32_t p_width,
        uint32_t p_height,
        float p_cell_size_m,
        std::span<TectonicPlatePOD> r_plates,
        std::span<CrustalTectonicCellPOD> r_tectonic_cells,
        std::span<StratigraphicColumnPOD> r_columns,
        std::span<HydrologyCellStatePOD> r_hydro_cells,
        std::span<float> r_elevation_field
    ) noexcept {
        const size_t total_cells = static_cast<size_t>(p_width) * static_cast<size_t>(p_height);
        if (total_cells == 0u) return;

        // 1. Generate deterministic tectonic plates
        const size_t plate_count = std::min<size_t>(r_plates.size(), std::max<size_t>(1u, p_seed_profile.tectonic_plate_count));
        uint64_t rng = p_seed_profile.tectonic_seed ^ 0x517CC1B727220A95ULL;
        for (size_t p = 0; p < plate_count; ++p) {
            TectonicPlatePOD &pl = r_plates[p];
            pl.plate_id = static_cast<uint16_t>(p);
            const float rx = static_cast<float>((splitmix64_next(rng) >> 40) & 0xFFFFFFULL) / static_cast<float>(0x1000000ULL);
            const float ry = static_cast<float>((splitmix64_next(rng) >> 40) & 0xFFFFFFULL) / static_cast<float>(0x1000000ULL);
            const float vx = (static_cast<float>((splitmix64_next(rng) >> 40) & 0xFFFFFFULL) / static_cast<float>(0x800000ULL) - 1.0f) * 0.045f;
            const float vy = (static_cast<float>((splitmix64_next(rng) >> 40) & 0xFFFFFFULL) / static_cast<float>(0x800000ULL) - 1.0f) * 0.045f;
            pl.seed_position_uv     = glm::vec2(0.08f + 0.84f * rx, 0.08f + 0.84f * ry);
            pl.drift_velocity_m_kyr = glm::vec2(vx, vy);
            pl.is_oceanic           = (p % 3 == 0) ? 1u : 0u;
            pl.crustal_density_kg_m3= pl.is_oceanic ? 3000.0f : 2700.0f;
            pl.base_thickness_m     = pl.is_oceanic ? 8500.0f : p_seed_profile.crust_base_thickness_m;
            pl.buoyancy_offset_m    = pl.is_oceanic ? -180.0f : 120.0f;
            pl.primary_lithology_id = pl.is_oceanic ? LITHOLOGY_BASALT : LITHOLOGY_GRANITE;
        }

        // 2. Assign each cell to nearest & second-nearest domain-warped Voronoi plate
        const float inv_w = 1.0f / static_cast<float>(std::max(1u, p_width));
        const float inv_h = 1.0f / static_cast<float>(std::max(1u, p_height));
        (void)p_cell_size_m;

        for (uint32_t z = 0; z < p_height; ++z) {
            for (uint32_t x = 0; x < p_width; ++x) {
                const size_t idx = static_cast<size_t>(z) * p_width + x;
                const float u = (static_cast<float>(x) + 0.5f) * inv_w;
                const float v = (static_cast<float>(z) + 0.5f) * inv_h;

                // Domain-warped UV coordinates for organic tectonic sutures
                const float warp_u = u + 0.08f * sample_fbm_2d(p_seed_profile.tectonic_seed, u * 3.5f, v * 3.5f, 3);
                const float warp_v = v + 0.08f * sample_fbm_2d(p_seed_profile.tectonic_seed + 7919ULL, u * 3.5f, v * 3.5f, 3);
                const glm::vec2 uv_warped(warp_u, warp_v);

                size_t best_p  = 0;
                size_t sec_p   = 0;
                float best_d2  = 1e30f;
                float sec_d2   = 1e30f;

                for (size_t p = 0; p < plate_count; ++p) {
                    const glm::vec2 diff = uv_warped - r_plates[p].seed_position_uv;
                    const float d2 = glm::dot(diff, diff);
                    if (d2 < best_d2) {
                        sec_d2  = best_d2;
                        sec_p   = best_p;
                        best_d2 = d2;
                        best_p  = p;
                    } else if (d2 < sec_d2) {
                        sec_d2 = d2;
                        sec_p  = p;
                    }
                }

                const TectonicPlatePOD &p0 = r_plates[best_p];
                const TectonicPlatePOD &p1 = r_plates[sec_p];
                const float border_dist = std::sqrt(std::max(0.0f, sec_d2)) - std::sqrt(std::max(0.0f, best_d2));
                const float boundary_proximity = std::exp(-border_dist * 14.0f);

                // Relative convergence (+) vs divergence (-) along plate boundary normal
                glm::vec2 border_normal = p1.seed_position_uv - p0.seed_position_uv;
                const float bn_len = glm::length(border_normal);
                if (bn_len > 1e-5f) border_normal /= bn_len;
                const glm::vec2 rel_vel = p0.drift_velocity_m_kyr - p1.drift_velocity_m_kyr;
                const float conv_rate   = glm::dot(rel_vel, border_normal) * 20.0f;
                const glm::vec2 tangent(-border_normal.y, border_normal.x);
                const float shear_rate  = std::abs(glm::dot(rel_vel, tangent)) * 20.0f;

                CrustalTectonicCellPOD tec{};
                tec.plate_id              = p0.plate_id;
                tec.compressive_stress_mpa= conv_rate * boundary_proximity * 240.0f;
                tec.shear_stress_mpa      = shear_rate * boundary_proximity * 160.0f;

                if (boundary_proximity > 0.25f) {
                    if (conv_rate > 0.15f) {
                        tec.boundary_type = (p0.is_oceanic != p1.is_oceanic)
                            ? TECTONIC_BOUNDARY_CONVERGENT_SUBDUCTION
                            : TECTONIC_BOUNDARY_CONVERGENT_OROGENY;
                        if (tec.boundary_type == TECTONIC_BOUNDARY_CONVERGENT_SUBDUCTION) {
                            tec.volcanic_activity_mask = 2u; // Stratovolcanic arc
                        }
                    } else if (conv_rate < -0.15f) {
                        tec.boundary_type = TECTONIC_BOUNDARY_DIVERGENT_RIFT;
                        tec.volcanic_activity_mask = 1u; // Rift basaltic effusion
                    } else if (shear_rate > 0.15f) {
                        tec.boundary_type = TECTONIC_BOUNDARY_TRANSFORM_SHEAR;
                    }
                }

                const float ridge_noise = sample_ridged_multifractal_2d(p_seed_profile.tectonic_seed + 31337ULL, u * 4.2f, v * 4.2f, 4);
                const float cont_noise  = sample_fbm_2d(p_seed_profile.lithology_seed, u * 2.8f, v * 2.8f, 5);

                const float orogenic_bump = std::max(0.0f, tec.compressive_stress_mpa) * 1.45f * (0.5f + 0.5f * ridge_noise);
                const float rift_trough   = std::min(0.0f, tec.compressive_stress_mpa) * 0.85f;
                tec.crust_thickness_m     = std::clamp(p0.base_thickness_m + orogenic_bump * 35.0f + rift_trough * 20.0f, 5000.0f, 70000.0f);
                tec.uplift_rate_m_kyr     = (orogenic_bump + rift_trough) * 0.015f;
                tec.geothermal_heat_flux_w_m2 = 0.055f + 0.18f * boundary_proximity;
                tec.isostatic_equilibrium_m   = p0.buoyancy_offset_m + orogenic_bump + rift_trough + cont_noise * 140.0f;

                if (idx < r_tectonic_cells.size()) {
                    r_tectonic_cells[idx] = tec;
                }

                // 3. Populate 4-layer stratigraphic column per cell
                if (idx < r_columns.size()) {
                    StratigraphicColumnPOD col{};
                    col.basement_elevation_m = p0.buoyancy_offset_m - 420.0f + cont_noise * 90.0f;
                    col.active_layer_count   = 4u;

                    // Layer 0: Deep Plutonic/Oceanic Basement (Granite or Basalt)
                    col.layers[0].lithology_id = p0.primary_lithology_id;
                    col.layers[0].thickness_m  = 260.0f + std::max(0.0f, orogenic_bump * 0.65f);
                    col.layers[0].folding_dip_rad = boundary_proximity * 0.55f;

                    // Layer 1: Metamorphic Belt (Schist or Marble) thickened under orogenic stress
                    col.layers[1].lithology_id = (tec.compressive_stress_mpa > 90.0f) ? LITHOLOGY_SCHIST : LITHOLOGY_MARBLE;
                    col.layers[1].thickness_m  = 75.0f + std::max(0.0f, orogenic_bump * 0.45f);
                    col.layers[1].folding_dip_rad = boundary_proximity * 0.45f;
                    col.layers[1].mineral_enrichment_grade = std::clamp(boundary_proximity * 0.85f, 0.0f, 1.0f);

                    // Layer 2: Chemical Carbonate (Limestone) or Volcanic (Basalt)
                    const float carb_selector = sample_fbm_2d(p_seed_profile.lithology_seed + 9001ULL, u * 3.0f, v * 3.0f, 3);
                    col.layers[2].lithology_id = (tec.volcanic_activity_mask != 0u)
                        ? LITHOLOGY_BASALT
                        : ((carb_selector > -0.1f) ? LITHOLOGY_LIMESTONE : LITHOLOGY_SANDSTONE);
                    col.layers[2].thickness_m  = 55.0f + 35.0f * std::max(0.0f, carb_selector);

                    // Layer 3: Upper Clastic Sedimentary (Sandstone)
                    col.layers[3].lithology_id = LITHOLOGY_SANDSTONE;
                    col.layers[3].thickness_m  = 35.0f + 30.0f * std::max(0.0f, -cont_noise);

                    col.regolith_thickness_m   = 1.2f + 1.5f * std::max(0.0f, -ridge_noise + 0.5f);
                    col.topsoil_thickness_m    = (col.basement_elevation_m + 425.0f > p_seed_profile.sea_level_m) ? 0.45f : 0.05f;
                    col.soil_humus_fraction    = 0.22f;
                    col.soil_mineral_fertility = (tec.volcanic_activity_mask != 0u) ? 0.82f : 0.48f;

                    recompute_column_surface_elevation(col);
                    r_columns[idx] = col;

                    if (idx < r_elevation_field.size()) {
                        r_elevation_field[idx] = col.surface_elevation_m;
                    }
                }

                // 4. Initialize hydrology & water table
                if (idx < r_hydro_cells.size()) {
                    HydrologyCellStatePOD hyd{};
                    const float surf_h = (idx < r_columns.size()) ? r_columns[idx].surface_elevation_m : 0.0f;
                    if (surf_h < p_seed_profile.sea_level_m) {
                        hyd.surface_water_depth_m = p_seed_profile.sea_level_m - surf_h;
                        hyd.is_inundated_mask     = 1u; // Marine / ocean
                        hyd.drainage_class        = BASIN_EXORHEIC_OCEANIC;
                    } else {
                        hyd.surface_water_depth_m = 0.02f;
                        hyd.is_inundated_mask     = 0u;
                        hyd.drainage_class        = BASIN_EXORHEIC_OCEANIC;
                    }
                    hyd.water_table_elevation_m = std::max(p_seed_profile.sea_level_m, surf_h - 3.5f);
                    hyd.stream_order_strahler   = 1u;
                    hyd.basin_id                = static_cast<uint32_t>(best_p);
                    r_hydro_cells[idx] = hyd;
                }
            }
        }
    }

    // ========================================================================
    // 5. Stage 1 (`GEO_T1`): Tectonic Uplift, Airy Isostasy & Metamorphism
    // ========================================================================

    static inline void step_tectonic_uplift_and_orogeny(
        uint32_t p_width,
        uint32_t p_height,
        float p_dt_kyr,
        float p_uplift_scale,
        std::span<const LithologyArchetypePOD> p_catalog,
        std::span<CrustalTectonicCellPOD> r_tectonic_cells,
        std::span<StratigraphicColumnPOD> r_columns,
        std::span<float> r_elevation_field
    ) noexcept {
        const size_t total = static_cast<size_t>(p_width) * static_cast<size_t>(p_height);
        const size_t n = std::min({total, r_tectonic_cells.size(), r_columns.size()});
        const float dt = std::max(0.0f, p_dt_kyr) * p_uplift_scale;

        for (size_t i = 0; i < n; ++i) {
            CrustalTectonicCellPOD &tec = r_tectonic_cells[i];
            StratigraphicColumnPOD &col = r_columns[i];

            const float delta_uplift = tec.uplift_rate_m_kyr * dt;
            col.basement_elevation_m += delta_uplift;

            // Solid-state metamorphism when compressive stress exceeds rock threshold
            const uint8_t layer_cnt = std::min<uint8_t>(col.active_layer_count, static_cast<uint8_t>(MAX_STRATA_PER_COLUMN));
            for (uint8_t l = 0; l < layer_cnt; ++l) {
                const uint16_t lid = col.layers[l].lithology_id;
                if (lid < p_catalog.size()) {
                    const LithologyArchetypePOD &arch = p_catalog[lid];
                    if (tec.compressive_stress_mpa >= arch.metamorphic_stress_mpa) {
                        col.layers[l].lithology_id = arch.next_metamorphic_lithology_id;
                    }
                }
            }

            // Volcanic extrusion adds basaltic thickness at active volcanic arcs / rifts
            if (tec.volcanic_activity_mask != 0u && layer_cnt > 0u) {
                const uint8_t top_idx = layer_cnt - 1u;
                col.layers[top_idx].thickness_m += 0.08f * dt;
                col.layers[top_idx].mineral_enrichment_grade =
                    std::clamp(col.layers[top_idx].mineral_enrichment_grade + 0.01f * dt, 0.0f, 1.0f);
            }

            recompute_column_surface_elevation(col);
            if (i < r_elevation_field.size()) {
                r_elevation_field[i] = col.surface_elevation_m;
            }
        }
    }

    // ========================================================================
    // 6. Stage 2 (`GEO_T2`): 2D Shallow-Water Virtual Pipe Hydrology
    // ========================================================================

    static inline void step_shallow_water_pipe_hydrology(
        uint32_t p_width,
        uint32_t p_height,
        float p_cell_size_m,
        float p_dt_sec,
        float p_rainfall_rate_m_s,
        float p_evaporation_rate_m_s,
        float p_sea_level_m,
        float p_gravity_m_s2,
        std::span<const float> p_elevation_field,
        std::span<HydrologyCellStatePOD> r_hydro_cells
    ) noexcept {
        if (p_width < 2u || p_height < 2u) return;
        const size_t total = static_cast<size_t>(p_width) * static_cast<size_t>(p_height);
        if (p_elevation_field.size() < total || r_hydro_cells.size() < total) return;

        const float dx = std::max(1.0f, p_cell_size_m);
        const float cell_area = dx * dx;
        const float dt = std::clamp(p_dt_sec, 0.001f, 5.0f);
        const float pipe_accel = (dt * dx * p_gravity_m_s2) / dx; // dt * A * g / L with A = dx

        // Pass 1: Add rainfall & update 4-neighbor virtual pipe fluxes (N=-Z, E=+X, S=+Z, W=-X)
        for (uint32_t z = 0; z < p_height; ++z) {
            for (uint32_t x = 0; x < p_width; ++x) {
                const size_t idx = static_cast<size_t>(z) * p_width + x;
                HydrologyCellStatePOD &hyd = r_hydro_cells[idx];
                const float h_center = p_elevation_field[idx];

                if (h_center < p_sea_level_m) {
                    hyd.surface_water_depth_m = p_sea_level_m - h_center;
                    hyd.is_inundated_mask |= 1u;
                } else {
                    hyd.surface_water_depth_m = std::max(0.0f, hyd.surface_water_depth_m + p_rainfall_rate_m_s * dt);
                }

                const float total_head = h_center + hyd.surface_water_depth_m;

                const size_t idx_n = (z > 0u)            ? (static_cast<size_t>(z - 1u) * p_width + x) : idx;
                const size_t idx_e = (x + 1u < p_width)  ? (static_cast<size_t>(z) * p_width + (x + 1u)) : idx;
                const size_t idx_s = (z + 1u < p_height) ? (static_cast<size_t>(z + 1u) * p_width + x) : idx;
                const size_t idx_w = (x > 0u)            ? (static_cast<size_t>(z) * p_width + (x - 1u)) : idx;

                const float head_n = p_elevation_field[idx_n] + r_hydro_cells[idx_n].surface_water_depth_m;
                const float head_e = p_elevation_field[idx_e] + r_hydro_cells[idx_e].surface_water_depth_m;
                const float head_s = p_elevation_field[idx_s] + r_hydro_cells[idx_s].surface_water_depth_m;
                const float head_w = p_elevation_field[idx_w] + r_hydro_cells[idx_w].surface_water_depth_m;

                glm::vec4 flux = hyd.outflow_flux_n_e_s_w;
                flux.x = (z > 0u)            ? std::max(0.0f, flux.x * 0.96f + pipe_accel * (total_head - head_n)) : 0.0f;
                flux.y = (x + 1u < p_width)  ? std::max(0.0f, flux.y * 0.96f + pipe_accel * (total_head - head_e)) : 0.0f;
                flux.z = (z + 1u < p_height) ? std::max(0.0f, flux.z * 0.96f + pipe_accel * (total_head - head_s)) : 0.0f;
                flux.w = (x > 0u)            ? std::max(0.0f, flux.w * 0.96f + pipe_accel * (total_head - head_w)) : 0.0f;

                // Mass-conservation scaling factor K <= 1 so outflow never exceeds available cell water volume
                const float sum_out = flux.x + flux.y + flux.z + flux.w;
                if (sum_out * dt > hyd.surface_water_depth_m * cell_area && sum_out > 1e-6f) {
                    const float k_scale = (hyd.surface_water_depth_m * cell_area) / (sum_out * dt);
                    flux *= std::clamp(k_scale, 0.0f, 1.0f);
                }
                hyd.outflow_flux_n_e_s_w = flux;

                // Local terrain slope magnitude
                const float dh_dx = (p_elevation_field[idx_e] - p_elevation_field[idx_w]) / (2.0f * dx);
                const float dh_dz = (p_elevation_field[idx_s] - p_elevation_field[idx_n]) / (2.0f * dx);
                hyd.slope_magnitude = std::sqrt(dh_dx * dh_dx + dh_dz * dh_dz);
            }
        }

        // Pass 2: Diverge pipe fluxes to update water depth, velocity vector, and volumetric discharge Q
        for (uint32_t z = 0; z < p_height; ++z) {
            for (uint32_t x = 0; x < p_width; ++x) {
                const size_t idx = static_cast<size_t>(z) * p_width + x;
                HydrologyCellStatePOD &hyd = r_hydro_cells[idx];

                const float f_in_n = (z > 0u)            ? r_hydro_cells[static_cast<size_t>(z - 1u) * p_width + x].outflow_flux_n_e_s_w.z : 0.0f;
                const float f_in_e = (x + 1u < p_width)  ? r_hydro_cells[static_cast<size_t>(z) * p_width + (x + 1u)].outflow_flux_n_e_s_w.w : 0.0f;
                const float f_in_s = (z + 1u < p_height) ? r_hydro_cells[static_cast<size_t>(z + 1u) * p_width + x].outflow_flux_n_e_s_w.x : 0.0f;
                const float f_in_w = (x > 0u)            ? r_hydro_cells[static_cast<size_t>(z) * p_width + (x - 1u)].outflow_flux_n_e_s_w.y : 0.0f;

                const glm::vec4 f_out = hyd.outflow_flux_n_e_s_w;
                const float sum_in  = f_in_n + f_in_e + f_in_s + f_in_w;
                const float sum_out = f_out.x + f_out.y + f_out.z + f_out.w;

                const float dv = dt * (sum_in - sum_out);
                const float new_depth = std::max(0.0f, hyd.surface_water_depth_m + dv / cell_area - p_evaporation_rate_m_s * dt);
                const float avg_depth = std::max(0.01f, 0.5f * (hyd.surface_water_depth_m + new_depth));
                hyd.surface_water_depth_m = new_depth;

                const float wx = 0.5f * (f_in_w - f_out.w + f_out.y - f_in_e);
                const float wz = 0.5f * (f_in_n - f_out.x + f_out.z - f_in_s);
                hyd.velocity_uv   = glm::vec2(wx, wz) / (avg_depth * dx);
                hyd.discharge_m3_s = 0.5f * (sum_in + sum_out);
            }
        }
    }

    // ========================================================================
    // 7. Stage 3 (`GEO_T3`): Multi-Stratum Stream-Power & Thermal Talus Erosion
    // ========================================================================

    static inline void step_stream_power_and_talus_erosion(
        uint32_t p_width,
        uint32_t p_height,
        float p_cell_size_m,
        float p_dt_sec,
        float p_sediment_capacity_kc,
        float p_deposition_rate_kd,
        float p_talus_creep_rate,
        std::span<const LithologyArchetypePOD> p_catalog,
        std::span<StratigraphicColumnPOD> r_columns,
        std::span<HydrologyCellStatePOD> r_hydro_cells,
        std::span<float> r_elevation_field
    ) noexcept {
        if (p_width < 2u || p_height < 2u) return;
        const size_t total = static_cast<size_t>(p_width) * static_cast<size_t>(p_height);
        if (r_columns.size() < total || r_hydro_cells.size() < total || r_elevation_field.size() < total) return;

        const float dx = std::max(1.0f, p_cell_size_m);
        const float dt = std::clamp(p_dt_sec, 0.001f, 5.0f);

        for (uint32_t z = 0; z < p_height; ++z) {
            for (uint32_t x = 0; x < p_width; ++x) {
                const size_t idx = static_cast<size_t>(z) * p_width + x;
                StratigraphicColumnPOD &col = r_columns[idx];
                HydrologyCellStatePOD &hyd  = r_hydro_cells[idx];

                const uint16_t surf_lith = col.dominant_surface_lithology;
                const LithologyArchetypePOD &arch = (surf_lith < p_catalog.size())
                    ? p_catalog[surf_lith]
                    : p_catalog[0];

                // 1. Fluvial Stream-Power Incision & Sediment Capacity Transport
                const float speed = glm::length(hyd.velocity_uv);
                const float sin_alpha = hyd.slope_magnitude / std::sqrt(1.0f + hyd.slope_magnitude * hyd.slope_magnitude);
                const float transport_capacity = p_sediment_capacity_kc * speed * std::max(0.05f, sin_alpha) * std::min(2.0f, hyd.surface_water_depth_m + 0.1f);

                if (transport_capacity > hyd.suspended_sediment_m) {
                    const float erode_amount = std::min(
                        0.5f,
                        dt * arch.erodibility_kf * 40.0f * (transport_capacity - hyd.suspended_sediment_m)
                    );
                    const float actual_eroded = erode_column_from_top(col, erode_amount);
                    hyd.suspended_sediment_m += actual_eroded;
                } else if (hyd.suspended_sediment_m > transport_capacity) {
                    const float deposit_amount = std::min(
                        hyd.suspended_sediment_m,
                        dt * p_deposition_rate_kd * (hyd.suspended_sediment_m - transport_capacity)
                    );
                    hyd.suspended_sediment_m -= deposit_amount;
                    deposit_sediment_on_column(col, deposit_amount);
                }

                // 2. Thermal Talus Slumping against critical angle of repose `arch.thermal_talus_angle_rad`
                const float max_dh = std::tan(arch.thermal_talus_angle_rad) * dx;
                const size_t neighbors[4] = {
                    (z > 0u)            ? (static_cast<size_t>(z - 1u) * p_width + x) : idx,
                    (x + 1u < p_width)  ? (static_cast<size_t>(z) * p_width + (x + 1u)) : idx,
                    (z + 1u < p_height) ? (static_cast<size_t>(z + 1u) * p_width + x) : idx,
                    (x > 0u)            ? (static_cast<size_t>(z) * p_width + (x - 1u)) : idx
                };

                for (size_t nb_idx : neighbors) {
                    if (nb_idx == idx) continue;
                    const float dh = col.surface_elevation_m - r_columns[nb_idx].surface_elevation_m;
                    if (dh > max_dh) {
                        const float slump = 0.25f * p_talus_creep_rate * dt * (dh - max_dh);
                        const float removed = erode_column_from_top(col, slump);
                        deposit_sediment_on_column(r_columns[nb_idx], removed);
                        r_elevation_field[nb_idx] = r_columns[nb_idx].surface_elevation_m;
                    }
                }

                r_elevation_field[idx] = col.surface_elevation_m;
            }
        }
    }

    // ========================================================================
    // 8. Stage 4 (`GEO_T4`): Darcy Groundwater, Karst Dissolution & Pedogenesis
    // ========================================================================

    static inline void step_groundwater_karst_and_pedogenesis(
        uint32_t p_width,
        uint32_t p_height,
        float p_dt_sec,
        float p_karst_dissolution_rate,
        float p_pedogenesis_rate,
        std::span<const LithologyArchetypePOD> p_catalog,
        std::span<StratigraphicColumnPOD> r_columns,
        std::span<HydrologyCellStatePOD> r_hydro_cells,
        std::span<float> r_elevation_field
    ) noexcept {
        const size_t total = static_cast<size_t>(p_width) * static_cast<size_t>(p_height);
        const size_t n = std::min({total, r_columns.size(), r_hydro_cells.size(), r_elevation_field.size()});
        const float dt = std::clamp(p_dt_sec, 0.001f, 5.0f);

        for (size_t i = 0; i < n; ++i) {
            StratigraphicColumnPOD &col = r_columns[i];
            HydrologyCellStatePOD &hyd  = r_hydro_cells[i];

            const uint16_t surf_lith = col.dominant_surface_lithology;
            const LithologyArchetypePOD &arch = (surf_lith < p_catalog.size()) ? p_catalog[surf_lith] : p_catalog[0];

            // 1. Darcy infiltration from surface water into phreatic water table
            const float unsat_depth = std::max(0.0f, col.surface_elevation_m - hyd.water_table_elevation_m);
            const float infil = std::min(hyd.surface_water_depth_m, arch.hydraulic_conductivity_m_s * 500.0f * dt);
            hyd.surface_water_depth_m -= infil;
            hyd.water_table_elevation_m = std::min(
                col.surface_elevation_m,
                hyd.water_table_elevation_m + (infil / std::max(0.05f, arch.porosity)) - 0.002f * unsat_depth * dt
            );

            // 2. Carbonate Karst Dissolution & Sinkhole Collapse
            const uint8_t layer_cnt = std::min<uint8_t>(col.active_layer_count, static_cast<uint8_t>(MAX_STRATA_PER_COLUMN));
            for (uint8_t l = 0; l < layer_cnt; ++l) {
                const uint16_t lid = col.layers[l].lithology_id;
                if (lid < p_catalog.size() && p_catalog[lid].carbonate_solubility > 0.05f) {
                    const float dissolve = p_karst_dissolution_rate * p_catalog[lid].carbonate_solubility * (infil + 0.005f * dt);
                    const float actual = std::min(col.layers[l].thickness_m, dissolve);
                    col.layers[l].thickness_m  -= actual;
                    hyd.karst_cavern_void_m    += actual;
                    hyd.dissolved_mineral_load += actual;

                    // Sinkhole collapse when subterranean void exceeds roof stability
                    if (hyd.karst_cavern_void_m > 3.5f) {
                        col.basement_elevation_m -= hyd.karst_cavern_void_m * 0.65f;
                        col.regolith_thickness_m += hyd.karst_cavern_void_m * 0.25f;
                        hyd.karst_cavern_void_m  *= 0.2f;
                        hyd.drainage_class        = BASIN_KARST_SINK;
                    }
                }
            }

            // 3. Soil Pedogenesis: weather regolith into O/A/B topsoil on gentle slopes
            const float slope_factor = std::clamp(1.0f - hyd.slope_magnitude * 0.85f, 0.0f, 1.0f);
            if (col.regolith_thickness_m > 0.05f && slope_factor > 0.1f) {
                const float convert = std::min(col.regolith_thickness_m * 0.15f, p_pedogenesis_rate * slope_factor * dt);
                col.regolith_thickness_m -= convert;
                col.topsoil_thickness_m  += convert;
                col.soil_humus_fraction   = std::clamp(col.soil_humus_fraction + 0.015f * slope_factor * dt, 0.02f, 0.85f);
                col.soil_mineral_fertility= std::clamp(
                    0.6f * col.soil_mineral_fertility + 0.4f * arch.mineral_fertility_yield,
                    0.05f,
                    1.0f
                );
            }

            recompute_column_surface_elevation(col);
            r_elevation_field[i] = col.surface_elevation_m;
        }
    }

    // ========================================================================
    // 9. Stage 5 (`GEO_M1`): D8 Drainage Graph, Watershed Basins & Strahler Order
    // ========================================================================

    static inline void compute_drainage_basins_and_strahler_order(
        uint32_t p_width,
        uint32_t p_height,
        float p_sea_level_m,
        float p_river_discharge_threshold,
        std::span<const float> p_elevation_field,
        std::span<HydrologyCellStatePOD> r_hydro_cells
    ) noexcept {
        if (p_width < 2u || p_height < 2u) return;
        const size_t total = static_cast<size_t>(p_width) * static_cast<size_t>(p_height);
        if (p_elevation_field.size() < total || r_hydro_cells.size() < total) return;

        static constexpr int32_t d8_dx[8] = { 0,  1, 1, 1, 0, -1, -1, -1 };
        static constexpr int32_t d8_dz[8] = {-1, -1, 0, 1, 1,  1,  0, -1 };
        static constexpr float   d8_len[8]= {1.0f, 1.41421356f, 1.0f, 1.41421356f, 1.0f, 1.41421356f, 1.0f, 1.41421356f};

        std::vector<size_t> receiver(total, total);
        std::vector<uint8_t> in_degree(total, 0u);

        // 1. Find steepest D8 receiver for each cell
        for (uint32_t z = 0; z < p_height; ++z) {
            for (uint32_t x = 0; x < p_width; ++x) {
                const size_t idx = static_cast<size_t>(z) * p_width + x;
                const float h0   = p_elevation_field[idx];
                float best_slope = 0.0f;
                uint8_t best_dir = 255u;
                size_t best_nb   = idx;

                for (uint8_t d = 0; d < 8u; ++d) {
                    const int32_t nx = static_cast<int32_t>(x) + d8_dx[d];
                    const int32_t nz = static_cast<int32_t>(z) + d8_dz[d];
                    if (nx < 0 || nz < 0 || nx >= static_cast<int32_t>(p_width) || nz >= static_cast<int32_t>(p_height)) {
                        continue;
                    }
                    const size_t n_idx = static_cast<size_t>(nz) * p_width + static_cast<size_t>(nx);
                    const float slope = (h0 - p_elevation_field[n_idx]) / d8_len[d];
                    if (slope > best_slope) {
                        best_slope = slope;
                        best_dir   = d;
                        best_nb    = n_idx;
                    }
                }

                r_hydro_cells[idx].receiver_dir_d8   = best_dir;
                r_hydro_cells[idx].stream_order_strahler = 1u;
                if (best_nb != idx) {
                    receiver[idx] = best_nb;
                    in_degree[best_nb]++;
                }
            }
        }

        // 2. Topological queue from headwaters downward to evaluate Strahler stream order
        std::vector<size_t> queue;
        queue.reserve(total);
        std::vector<uint8_t> max_incoming_order(total, 0u);
        std::vector<uint8_t> max_incoming_count(total, 0u);

        for (size_t i = 0; i < total; ++i) {
            if (in_degree[i] == 0u) {
                queue.push_back(i);
            }
        }

        size_t head = 0u;
        while (head < queue.size()) {
            const size_t u = queue[head++];
            const uint8_t u_ord = r_hydro_cells[u].stream_order_strahler;
            const size_t v = receiver[u];
            if (v < total && v != u) {
                if (u_ord > max_incoming_order[v]) {
                    max_incoming_order[v] = u_ord;
                    max_incoming_count[v] = 1u;
                } else if (u_ord == max_incoming_order[v]) {
                    max_incoming_count[v]++;
                }
                if (--in_degree[v] == 0u) {
                    r_hydro_cells[v].stream_order_strahler =
                        (max_incoming_count[v] >= 2u)
                            ? static_cast<uint8_t>(std::min<int>(8, max_incoming_order[v] + 1))
                            : std::max<uint8_t>(1u, max_incoming_order[v]);
                    queue.push_back(v);
                }
            }
        }

        // 3. Classify drainage basins & assign watershed basin_id from terminal sinks
        for (size_t i = 0; i < total; ++i) {
            size_t curr = i;
            for (uint32_t hop = 0; hop < 64u && receiver[curr] < total && receiver[curr] != curr; ++hop) {
                curr = receiver[curr];
            }
            r_hydro_cells[i].basin_id = static_cast<uint32_t>(curr);

            const float h = p_elevation_field[i];
            if (h < p_sea_level_m) {
                r_hydro_cells[i].drainage_class = BASIN_EXORHEIC_OCEANIC;
            } else if (r_hydro_cells[i].karst_cavern_void_m > 1.0f) {
                r_hydro_cells[i].drainage_class = BASIN_KARST_SINK;
            } else if (r_hydro_cells[i].receiver_dir_d8 == 255u && r_hydro_cells[i].surface_water_depth_m > 0.4f) {
                r_hydro_cells[i].drainage_class = BASIN_LACUSTRINE_LAKE;
            } else if (r_hydro_cells[i].discharge_m3_s >= p_river_discharge_threshold || r_hydro_cells[i].stream_order_strahler >= 3u) {
                r_hydro_cells[i].drainage_class = BASIN_FLUVIAL_CHANNEL;
            } else if (p_elevation_field[curr] >= p_sea_level_m) {
                r_hydro_cells[i].drainage_class = BASIN_ENDORHEIC_CLOSED;
            } else {
                r_hydro_cells[i].drainage_class = BASIN_EXORHEIC_OCEANIC;
            }
        }
    }
};

} // namespace ideam::core
