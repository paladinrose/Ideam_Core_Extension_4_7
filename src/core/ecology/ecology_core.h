#pragma once

/**
 * @file ecology_core.h
 * @brief Pure C++26 simulation kernels for IdeamEcology.
 *
 * Implements:
 * 1. Deterministic SplitMix64 seed derivation, 2D FBM ecotones & 8 default functional guilds
 * 2. Whittaker-Holdridge Gaussian Niche Fitness, Beer-Lambert Canopy Light Competition & NPP
 * 3. Century-Lite Soil Carbon/Nitrogen/Phosphorus (NPK) Cycling, Q10 Decomposition & Seral Succession
 * 4. Rothermel Surface/Crown Wildfire Spread, Pyromineralization Ash Pulse & Windthrow Disturbance
 * 5. Spatial Lotka-Volterra Multi-Guild Trophic Web (Grazers, Browsers, Pollinators, Apex Predators) &
 *    Alchemical Phytochemical Potency Synthesis
 * 6. Holdridge Life-Zone Partitioning, Shannon Biodiversity Index & Harvest/Hazard Queries
 */

#include "core/ecology/ecology_types.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace ideam::core {

struct EcologyKernelMath {
    // ========================================================================
    // 1. Deterministic SplitMix64 & 2D FBM Niche Noise
    // ========================================================================
    static constexpr uint64_t splitmix64(uint64_t x) noexcept {
        x += 0x9E3779B97F4A7C15ULL;
        x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
        x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
        return x ^ (x >> 31);
    }

    static constexpr float hash_to_unit_float(uint64_t h) noexcept {
        return static_cast<float>((h >> 40) & 0xFFFFFFULL) / static_cast<float>(0x1000000ULL);
    }

    static constexpr uint64_t hash_coords_2d(int32_t x, int32_t y, uint64_t seed) noexcept {
        uint64_t ux = static_cast<uint64_t>(static_cast<uint32_t>(x));
        uint64_t uy = static_cast<uint64_t>(static_cast<uint32_t>(y));
        return splitmix64(seed ^ (ux * 0xD1B54A32D192ED03ULL + uy * 0x8CB92BA72F3D8DD7ULL));
    }

    static inline float value_noise_2d(float x, float y, uint64_t seed) noexcept {
        const int32_t ix = static_cast<int32_t>(std::floor(x));
        const int32_t iy = static_cast<int32_t>(std::floor(y));
        const float fx = x - static_cast<float>(ix);
        const float fy = y - static_cast<float>(iy);
        const float u = fx * fx * (3.0f - 2.0f * fx);
        const float v = fy * fy * (3.0f - 2.0f * fy);

        const float n00 = hash_to_unit_float(hash_coords_2d(ix,     iy,     seed)) * 2.0f - 1.0f;
        const float n10 = hash_to_unit_float(hash_coords_2d(ix + 1, iy,     seed)) * 2.0f - 1.0f;
        const float n01 = hash_to_unit_float(hash_coords_2d(ix,     iy + 1, seed)) * 2.0f - 1.0f;
        const float n11 = hash_to_unit_float(hash_coords_2d(ix + 1, iy + 1, seed)) * 2.0f - 1.0f;

        const float nx0 = n00 + u * (n10 - n00);
        const float nx1 = n01 + u * (n11 - n01);
        return nx0 + v * (nx1 - nx0);
    }

    static inline float fbm_2d(float x, float y, uint64_t seed, int octaves = 4) noexcept {
        float sum = 0.0f;
        float amp = 0.5f;
        float freq = 1.0f;
        for (int i = 0; i < octaves; ++i) {
            sum += amp * value_noise_2d(x * freq, y * freq, seed + static_cast<uint64_t>(i) * 0x517CC1B727220A95ULL);
            freq *= 2.02f;
            amp *= 0.5f;
        }
        return sum;
    }

    static constexpr EcologySeedProfilePOD make_seed_profile(
        uint64_t master_seed,
        uint32_t width = 64,
        uint32_t height = 64,
        uint32_t guild_count = 8,
        float cell_size_m = 50.0f) noexcept {
        EcologySeedProfilePOD p{};
        p.master_seed                 = master_seed;
        p.whittaker_niche_seed        = splitmix64(master_seed ^ 0x01ULL);
        p.succession_dispersal_seed   = splitmix64(master_seed ^ 0x02ULL);
        p.trophic_lotka_volterra_seed = splitmix64(master_seed ^ 0x03ULL);
        p.rothermel_wildfire_seed     = splitmix64(master_seed ^ 0x04ULL);
        p.grid_width                  = width < 8 ? 8 : width;
        p.grid_height                 = height < 8 ? 8 : height;
        p.active_guild_count          = std::clamp<uint32_t>(guild_count, 1u, 16u);
        p.cell_size_meters            = cell_size_m > 1.0f ? cell_size_m : 50.0f;
        return p;
    }

    // ========================================================================
    // 2. Default 8-Guild Functional Flora/Mycorrhizal Catalog
    // ========================================================================
    static constexpr std::array<FloraGuildArchetypePOD, DEFAULT_ECOLOGY_GUILD_COUNT> build_default_guild_catalog() noexcept {
        std::array<FloraGuildArchetypePOD, DEFAULT_ECOLOGY_GUILD_COUNT> cat{};
        // 0: Boreal/Montane Conifer Canopy (Pine/Spruce/Fir)
        cat[0] = {0, TrophicGuildRole::PRIMARY_PRODUCER_CANOPY, 5,  6.0f,  8.5f,  900.0f, 550.0f, 0.20f, 0.45f, 0.65f, 0.70f, 0.26f, 32.0f, 0.5f, 0.52f, 0.55f, 0.18f};
        // 1: Temperate Broadleaf Deciduous Canopy (Oak/Maple/Beech)
        cat[1] = {1, TrophicGuildRole::PRIMARY_PRODUCER_CANOPY, 5, 14.5f,  8.0f, 1150.0f, 550.0f, 0.25f, 0.72f, 0.45f, 0.20f, 0.30f, 36.0f, 0.8f, 0.34f, 0.62f, 0.26f};
        // 2: Tropical/Subtropical Emergent Hardwood Canopy
        cat[2] = {2, TrophicGuildRole::PRIMARY_PRODUCER_CANOPY, 5, 25.5f,  6.5f, 2200.0f, 950.0f, 0.35f, 0.85f, 0.20f, 0.10f, 0.42f, 48.0f, 2.2f, 0.28f, 0.82f, 0.32f};
        // 3: Sclerophyll Fire-Chaparral & Thorn Scrub
        cat[3] = {3, TrophicGuildRole::PRIMARY_PRODUCER_UNDERSTORY, 3, 19.0f, 9.5f, 520.0f, 320.0f, 0.10f, 0.30f, 0.75f, 0.90f, 0.38f, 11.0f, 1.2f, 0.48f, 0.68f, 0.20f};
        // 4: Prairie C4 Tussock Grass & Steppe Bunchgrass
        cat[4] = {4, TrophicGuildRole::PRIMARY_PRODUCER_UNDERSTORY, 2, 16.0f, 11.0f, 480.0f, 300.0f, 0.12f, 0.20f, 0.35f, 0.85f, 0.55f,  6.5f, 0.9f, 0.22f, 0.42f, 0.35f};
        // 5: Nitrogen-Fixing Pioneer (Lupine / Alder / Dryas)
        cat[5] = {5, TrophicGuildRole::NITROGEN_FIXING_PIONEER, 1, 11.0f, 12.5f, 750.0f, 600.0f, 0.14f, 0.18f, 0.30f, 0.80f, 0.62f,  4.8f, 6.5f, 0.18f, 0.74f, 0.38f};
        // 6: Riparian Wetland Gallery (Willow / Cypress / Sedge)
        cat[6] = {6, TrophicGuildRole::PRIMARY_PRODUCER_CANOPY, 4, 15.0f, 10.5f, 1450.0f, 750.0f, 0.45f, 0.55f, 0.25f, 0.30f, 0.46f, 26.0f, 1.8f, 0.25f, 0.78f, 0.30f};
        // 7: Subterranean Karst & Old-Growth Mycorrhizal Fungi
        cat[7] = {7, TrophicGuildRole::MYCORRHIZAL_DECOMPOSER, 5, 12.0f, 10.0f, 1100.0f, 700.0f, 0.30f, 0.98f, 0.15f, 0.15f, 0.48f,  5.5f, 2.8f, 0.12f, 0.95f, 0.40f};
        return cat;
    }

    // ========================================================================
    // 3. Initial Biosphere Generation from Seed
    // ========================================================================
    static inline void initialize_biosphere_from_seed(
        const EcologySeedProfilePOD& seed,
        std::span<const FloraGuildArchetypePOD> guild_catalog,
        std::span<EcologicalCellStatePOD> out_eco,
        std::span<BiogeochemicalAndTrophicCellPOD> out_biogeo) noexcept {
        const uint32_t w = seed.grid_width;
        const uint32_t h = seed.grid_height;
        const size_t total = static_cast<size_t>(w) * static_cast<size_t>(h);
        if (out_eco.size() < total || out_biogeo.size() < total) {
            return;
        }

        const auto default_cat = build_default_guild_catalog();
        const auto get_guild = [&](size_t idx) noexcept -> const FloraGuildArchetypePOD& {
            if (!guild_catalog.empty()) {
                return guild_catalog[idx % guild_catalog.size()];
            }
            return default_cat[idx % default_cat.size()];
        };
        const size_t n_guilds = guild_catalog.empty() ? default_cat.size() : guild_catalog.size();

        for (uint32_t y = 0; y < h; ++y) {
            const float lat_norm = static_cast<float>(y) / static_cast<float>(std::max(1u, h - 1));
            for (uint32_t x = 0; x < w; ++x) {
                const size_t idx = static_cast<size_t>(y * w + x);
                auto& eco = out_eco[idx];
                auto& bio = out_biogeo[idx];
                eco = EcologicalCellStatePOD{};
                bio = BiogeochemicalAndTrophicCellPOD{};

                const float nx = static_cast<float>(x) / static_cast<float>(w);
                const float ny = static_cast<float>(y) / static_cast<float>(h);
                const float elev_noise = fbm_2d(nx * 3.8f, ny * 3.8f, seed.whittaker_niche_seed, 4);
                const float moisture_noise = fbm_2d(nx * 3.2f + 7.1f, ny * 3.2f + 3.4f, seed.succession_dispersal_seed, 4);

                // Synthesize realistic Whittaker temperature (-6 C to +28 C) and precipitation (150 mm to 2800 mm) gradients
                eco.mean_temperature_c = 27.0f - 31.0f * lat_norm + elev_noise * 6.5f;
                eco.annual_precipitation_mm = std::clamp(1150.0f + moisture_noise * 1250.0f - lat_norm * 350.0f, 120.0f, 3200.0f);
                eco.soil_moisture_01 = std::clamp(eco.annual_precipitation_mm / 1800.0f, 0.08f, 0.96f);
                eco.terrain_slope_gradient = std::clamp(std::abs(elev_noise) * 0.45f, 0.01f, 0.65f);
                eco.wind_speed_mps = 4.5f + 3.5f * std::abs(elev_noise);

                // Find best-fitting guild in Whittaker space
                float best_fit = -1.0f;
                uint8_t best_id = 0;
                uint32_t mask = 0;
                for (size_t g = 0; g < n_guilds; ++g) {
                    const auto& arch = get_guild(g);
                    const float dt = (eco.mean_temperature_c - arch.optimal_temp_c) / std::max(1.0f, arch.temp_tolerance_sigma_c);
                    const float dp = (eco.annual_precipitation_mm - arch.optimal_precip_mm) / std::max(50.0f, arch.precip_tolerance_sigma_mm);
                    const float fit = std::exp(-0.5f * (dt * dt + dp * dp));
                    if (fit > 0.25f) {
                        mask |= (1u << g);
                    }
                    if (fit > best_fit) {
                        best_fit = fit;
                        best_id = static_cast<uint8_t>(arch.guild_id);
                    }
                }
                eco.dominant_guild_id = best_id;
                bio.active_guild_presence_mask = mask != 0 ? mask : (1u << best_id);

                const auto& dom = get_guild(best_id);
                eco.canopy_biomass_kg_m2 = dom.max_carrying_biomass_kg_m2 * std::clamp(best_fit * 0.78f, 0.02f, 0.95f);
                eco.understory_biomass_kg_m2 = std::clamp(eco.soil_moisture_01 * 5.2f * (1.0f - 0.45f * (eco.canopy_biomass_kg_m2 / 40.0f)), 0.1f, 6.5f);
                eco.root_mycorrhizal_kg_m2 = (eco.canopy_biomass_kg_m2 + eco.understory_biomass_kg_m2) * 0.32f;
                eco.dead_fuel_litter_kg_m2 = 0.6f + 0.12f * eco.canopy_biomass_kg_m2;

                bio.soil_organic_carbon_kg_m2 = 3.5f + eco.soil_moisture_01 * 9.5f;
                bio.mineral_nitrogen_g_m2 = 22.0f + eco.soil_moisture_01 * 38.0f;
                bio.available_phosphorus_g_m2 = 8.0f + (1.0f - eco.terrain_slope_gradient) * 10.0f;
                bio.dead_fuel_moisture_01 = std::clamp(eco.soil_moisture_01 * 0.65f, 0.05f, 0.85f);

                bio.grazer_herbivore_density = std::clamp(eco.understory_biomass_kg_m2 * 0.12f, 0.02f, 0.95f);
                bio.browser_herbivore_density = std::clamp(eco.canopy_biomass_kg_m2 * 0.025f, 0.02f, 0.85f);
                bio.pollinator_disperser_density = std::clamp(best_fit * 0.75f, 0.05f, 0.98f);
                bio.apex_predator_density = std::clamp((bio.grazer_herbivore_density + bio.browser_herbivore_density) * 0.25f, 0.01f, 0.65f);
            }
        }

        // Run one full diagnostic cycle so LAI, albedo, NPP, life zones, and seral stages are synchronized
        step_whittaker_niche_and_canopy_npp(seed, guild_catalog, 0.25f, out_eco, out_biogeo);
        step_biogeochemical_npk_and_succession(seed, guild_catalog, 0.25f, out_eco, out_biogeo);
        step_lotka_volterra_trophic_web(seed, 0.25f, out_eco, out_biogeo);
        compute_holdridge_and_seral_partition(seed, out_eco, out_biogeo);
    }

    // ========================================================================
    // 4. Stage ECO_T1: Whittaker Niche, Beer-Lambert Canopy Light & NPP
    // ========================================================================
    static inline void step_whittaker_niche_and_canopy_npp(
        const EcologySeedProfilePOD& seed,
        std::span<const FloraGuildArchetypePOD> guild_catalog,
        float dt_years,
        std::span<EcologicalCellStatePOD> eco_cells,
        std::span<BiogeochemicalAndTrophicCellPOD> biogeo_cells) noexcept {
        const size_t total = static_cast<size_t>(seed.grid_width) * static_cast<size_t>(seed.grid_height);
        if (eco_cells.size() < total || biogeo_cells.size() < total) {
            return;
        }

        const auto default_cat = build_default_guild_catalog();
        const auto get_guild = [&](size_t idx) noexcept -> const FloraGuildArchetypePOD& {
            if (!guild_catalog.empty()) {
                return guild_catalog[idx % guild_catalog.size()];
            }
            return default_cat[idx % default_cat.size()];
        };
        const size_t n_guilds = guild_catalog.empty() ? default_cat.size() : guild_catalog.size();
        const float dt = std::clamp(dt_years, 0.01f, 5.0f);

        for (size_t i = 0; i < total; ++i) {
            auto& eco = eco_cells[i];
            auto& bio = biogeo_cells[i];

            // Evaluate guild niche fitness & Shannon diversity H' = -sum(p_i * ln(p_i))
            float fit_sum = 0.0f;
            float best_fit = 0.0f;
            uint8_t best_id = eco.dominant_guild_id;
            std::array<float, 16> fits{};
            const size_t lim_g = std::min<size_t>(n_guilds, 16);

            for (size_t g = 0; g < lim_g; ++g) {
                const auto& arch = get_guild(g);
                const float d_temp = (eco.mean_temperature_c - arch.optimal_temp_c) / std::max(1.0f, arch.temp_tolerance_sigma_c);
                const float d_prec = (eco.annual_precipitation_mm - arch.optimal_precip_mm) / std::max(50.0f, arch.precip_tolerance_sigma_mm);
                const float water_stress = std::clamp((eco.soil_moisture_01 - arch.drought_wilting_point_01) / 0.45f, 0.05f, 1.0f);
                const float f = std::exp(-0.5f * (d_temp * d_temp + d_prec * d_prec)) * water_stress;
                fits[g] = f;
                fit_sum += f;
                if (f > best_fit) {
                    best_fit = f;
                    best_id = static_cast<uint8_t>(arch.guild_id);
                }
            }
            eco.dominant_guild_id = best_id;

            float shannon_h = 0.0f;
            if (fit_sum > 1e-4f) {
                for (size_t g = 0; g < lim_g; ++g) {
                    const float p = fits[g] / fit_sum;
                    if (p > 1e-4f) {
                        shannon_h -= p * std::log(p);
                    }
                }
            }
            eco.shannon_diversity_h = shannon_h;

            const auto& dom = get_guild(best_id);
            const float npk_factor = std::clamp((bio.mineral_nitrogen_g_m2 / 35.0f) * (bio.available_phosphorus_g_m2 / 10.0f), 0.25f, 1.45f);
            const float co2_fertilization = std::clamp(seed.ambient_co2_ppm / 400.0f, 0.75f, 1.35f);

            // Beer-Lambert canopy light interception: I_transmitted = exp(-k * LAI)
            eco.leaf_area_index_lai = std::clamp(eco.canopy_biomass_kg_m2 * dom.specific_leaf_area_m2_kg + eco.understory_biomass_kg_m2 * 0.35f, 0.05f, 9.5f);
            const float canopy_light_abs = 1.0f - std::exp(-seed.beer_lambert_extinction_k * eco.leaf_area_index_lai);
            const float understory_light = std::exp(-seed.beer_lambert_extinction_k * (eco.canopy_biomass_kg_m2 * dom.specific_leaf_area_m2_kg));

            // Logistic canopy & understory growth
            const float k_cap = std::max(1.0f, dom.max_carrying_biomass_kg_m2 * best_fit * npk_factor);
            const float r_eff = dom.intrinsic_growth_rate_r * seed.base_npp_efficiency * co2_fertilization * best_fit;
            const float d_canopy = r_eff * eco.canopy_biomass_kg_m2 * (1.0f - eco.canopy_biomass_kg_m2 / k_cap) * dt;
            eco.canopy_biomass_kg_m2 = std::clamp(eco.canopy_biomass_kg_m2 + d_canopy, 0.01f, 65.0f);

            const float u_cap = std::max(0.5f, 8.5f * eco.soil_moisture_01 * (0.25f + 0.75f * understory_light));
            const float d_under = 0.48f * eco.understory_biomass_kg_m2 * (1.0f - eco.understory_biomass_kg_m2 / u_cap) * dt;
            eco.understory_biomass_kg_m2 = std::clamp(eco.understory_biomass_kg_m2 + d_under, 0.02f, 12.0f);

            eco.root_mycorrhizal_kg_m2 = 0.85f * eco.root_mycorrhizal_kg_m2 + 0.15f * ((eco.canopy_biomass_kg_m2 + eco.understory_biomass_kg_m2) * 0.32f);
            eco.canopy_height_m = std::clamp(std::sqrt(eco.canopy_biomass_kg_m2) * 5.2f, 0.15f, 48.0f);

            // Net Primary Productivity (kg/m2/yr), Evapotranspiration, Biogenic Albedo & Roughness z0
            bio.net_primary_productivity_npp = std::max(0.0f, (canopy_light_abs * 2.4f * best_fit * npk_factor));
            eco.evapotranspiration_mm_day = std::clamp((0.8f + 0.65f * eco.leaf_area_index_lai) * eco.soil_moisture_01 * std::max(0.2f, (eco.mean_temperature_c + 10.0f) / 25.0f), 0.1f, 8.5f);
            eco.biogenic_albedo = std::clamp(0.28f - 0.16f * canopy_light_abs, 0.09f, 0.35f);
            eco.aerodynamic_roughness_z0 = std::clamp(0.02f + 0.075f * eco.canopy_height_m, 0.01f, 3.2f);
        }
    }

    // ========================================================================
    // 5. Stage ECO_T2: Century-Lite Soil C/N/P Cycling & Seral Succession
    // ========================================================================
    static inline void step_biogeochemical_npk_and_succession(
        const EcologySeedProfilePOD& seed,
        std::span<const FloraGuildArchetypePOD> guild_catalog,
        float dt_years,
        std::span<EcologicalCellStatePOD> eco_cells,
        std::span<BiogeochemicalAndTrophicCellPOD> biogeo_cells) noexcept {
        const size_t total = static_cast<size_t>(seed.grid_width) * static_cast<size_t>(seed.grid_height);
        if (eco_cells.size() < total || biogeo_cells.size() < total) {
            return;
        }

        const auto default_cat = build_default_guild_catalog();
        const auto get_guild = [&](size_t idx) noexcept -> const FloraGuildArchetypePOD& {
            if (!guild_catalog.empty()) {
                return guild_catalog[idx % guild_catalog.size()];
            }
            return default_cat[idx % default_cat.size()];
        };
        const float dt = std::clamp(dt_years, 0.01f, 5.0f);

        for (size_t i = 0; i < total; ++i) {
            auto& eco = eco_cells[i];
            auto& bio = biogeo_cells[i];
            const auto& dom = get_guild(eco.dominant_guild_id);

            // Litterfall from canopy & understory turnover
            const float litter_input = (eco.canopy_biomass_kg_m2 * 0.06f + eco.understory_biomass_kg_m2 * 0.22f) * dt;
            eco.dead_fuel_litter_kg_m2 = std::clamp(eco.dead_fuel_litter_kg_m2 + litter_input, 0.05f, 18.0f);

            // Q10 microbial decomposition rate: k_dec = k0 * Q10^((T - 15)/10) * moisture
            const float q10_term = std::pow(seed.q10_decomposition_factor, (eco.mean_temperature_c - 15.0f) * 0.1f);
            const float lignin_Penalty = 1.0f - 0.55f * dom.litter_lignin_recalcitrance;
            bio.decomposer_microbial_activity = std::clamp(0.32f * q10_term * eco.soil_moisture_01 * lignin_Penalty, 0.02f, 1.5f);

            const float decomposed_litter = std::min(eco.dead_fuel_litter_kg_m2 * 0.75f, eco.dead_fuel_litter_kg_m2 * bio.decomposer_microbial_activity * dt);
            eco.dead_fuel_litter_kg_m2 -= decomposed_litter;

            // Humification into soil organic carbon & mineralization of N and P
            bio.soil_organic_carbon_kg_m2 = std::clamp(
                bio.soil_organic_carbon_kg_m2 + decomposed_litter * 0.42f - bio.soil_organic_carbon_kg_m2 * 0.03f * bio.decomposer_microbial_activity * dt,
                0.2f, 45.0f);

            const float n_fix = dom.nitrogen_fixation_g_m2_yr * seed.nitrogen_fixation_scale * dt;
            const float n_mineralized = decomposed_litter * 12.0f;
            const float n_uptake = bio.net_primary_productivity_npp * 4.5f * dt;
            bio.mineral_nitrogen_g_m2 = std::clamp(bio.mineral_nitrogen_g_m2 + n_fix + n_mineralized - n_uptake, 2.0f, 160.0f);
            bio.available_phosphorus_g_m2 = std::clamp(bio.available_phosphorus_g_m2 + decomposed_litter * 2.5f - n_uptake * 0.18f, 1.0f, 60.0f);

            bio.burn_scar_recovery_years += dt;
            bio.dead_fuel_moisture_01 = std::clamp(eco.soil_moisture_01 * 0.72f + (eco.annual_precipitation_mm / 3500.0f) * 0.25f, 0.03f, 0.95f);
        }
    }

    // ========================================================================
    // 6. Stage ECO_T3: Rothermel Wildfire Spread, Pyromineralization & Windthrow
    // ========================================================================
    static inline void step_rothermel_wildfire_and_disturbance(
        const EcologySeedProfilePOD& seed,
        std::span<const FloraGuildArchetypePOD> guild_catalog,
        float dt_years,
        std::span<EcologicalCellStatePOD> eco_cells,
        std::span<BiogeochemicalAndTrophicCellPOD> biogeo_cells) noexcept {
        const uint32_t w = seed.grid_width;
        const uint32_t h = seed.grid_height;
        const size_t total = static_cast<size_t>(w) * static_cast<size_t>(h);
        if (eco_cells.size() < total || biogeo_cells.size() < total) {
            return;
        }

        const auto default_cat = build_default_guild_catalog();
        const auto get_guild = [&](size_t idx) noexcept -> const FloraGuildArchetypePOD& {
            if (!guild_catalog.empty()) {
                return guild_catalog[idx % guild_catalog.size()];
            }
            return default_cat[idx % default_cat.size()];
        };

        // Check neighbor fire contagion before updating
        std::vector<float> neighbor_fire_heat(total, 0.0f);
        for (uint32_t y = 0; y < h; ++y) {
            for (uint32_t x = 0; x < w; ++x) {
                const size_t idx = static_cast<size_t>(y * w + x);
                const float I_fire = biogeo_cells[idx].wildfire_intensity_kw_m;
                if (I_fire <= 25.0f) {
                    continue;
                }
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        if (dx == 0 && dy == 0) continue;
                        const int nx = static_cast<int>(x) + dx;
                        const int ny = static_cast<int>(y) + dy;
                        if (nx >= 0 && nx < static_cast<int>(w) && ny >= 0 && ny < static_cast<int>(h)) {
                            const size_t n_idx = static_cast<size_t>(ny * static_cast<int>(w) + nx);
                            neighbor_fire_heat[n_idx] += I_fire * 0.32f;
                        }
                    }
                }
            }
        }

        for (size_t i = 0; i < total; ++i) {
            auto& eco = eco_cells[i];
            auto& bio = biogeo_cells[i];
            const auto& dom = get_guild(eco.dominant_guild_id);

            // Windthrow blowdown when wind > 28 m/s (Gale / Tornado track)
            if (eco.wind_speed_mps > 28.0f && eco.canopy_biomass_kg_m2 > 2.0f) {
                const float blowdown = std::clamp((eco.wind_speed_mps - 26.0f) * 0.025f, 0.05f, 0.85f);
                const float felled = eco.canopy_biomass_kg_m2 * blowdown;
                eco.canopy_biomass_kg_m2 -= felled;
                eco.dead_fuel_litter_kg_m2 += felled * 0.85f;
                bio.windthrow_damage_01 = std::max(bio.windthrow_damage_01, blowdown);
                eco.disturbance_state = EcologicalDisturbanceState::WINDTHROW_TORNADO_BLOWDOWN;
            } else {
                bio.windthrow_damage_01 *= 0.85f;
            }

            // Rothermel fuel flammability & reaction intensity
            const float moisture_damping = std::clamp(1.0f - (bio.dead_fuel_moisture_01 / 0.42f), 0.0f, 1.0f);
            const float wind_slope_phi = (1.0f + 0.18f * eco.wind_speed_mps * seed.rothermel_wind_spread_scale) *
                                         (1.0f + 2.5f * eco.terrain_slope_gradient);
            const float available_fuel = eco.dead_fuel_litter_kg_m2 + eco.understory_biomass_kg_m2 * 0.5f;

            if (bio.wildfire_intensity_kw_m > 10.0f || (neighbor_fire_heat[i] > 180.0f && moisture_damping > 0.25f)) {
                const float I_r = available_fuel * 280.0f * moisture_damping * wind_slope_phi;
                bio.wildfire_intensity_kw_m = I_r;
                if (I_r > 1200.0f && eco.canopy_biomass_kg_m2 > 4.0f) {
                    eco.disturbance_state = EcologicalDisturbanceState::ACTIVE_CROWN_WILDFIRE;
                    const float burn_frac = std::clamp(1.0f - dom.fire_resistance_bark_01 * 0.65f, 0.25f, 0.92f);
                    const float burned_canopy = eco.canopy_biomass_kg_m2 * burn_frac;
                    eco.canopy_biomass_kg_m2 -= burned_canopy;
                    eco.understory_biomass_kg_m2 *= 0.12f;
                    eco.dead_fuel_litter_kg_m2 *= 0.15f;
                    // Pyromineralization ash pulse enriches mineral N and P!
                    bio.mineral_nitrogen_g_m2 += burned_canopy * 1.8f;
                    bio.available_phosphorus_g_m2 += burned_canopy * 0.65f;
                    bio.burn_scar_recovery_years = 0.0f;
                    // Serotinous cone re-seeding boost
                    eco.understory_biomass_kg_m2 += dom.serotinous_reseed_boost_01 * 0.85f;
                } else if (I_r > 80.0f) {
                    eco.disturbance_state = EcologicalDisturbanceState::ACTIVE_SURFACE_BRUSHFIRE;
                    eco.understory_biomass_kg_m2 *= 0.35f;
                    eco.dead_fuel_litter_kg_m2 *= 0.30f;
                    bio.mineral_nitrogen_g_m2 += available_fuel * 1.2f;
                    bio.burn_scar_recovery_years = 0.0f;
                } else {
                    bio.wildfire_intensity_kw_m = 0.0f;
                    eco.disturbance_state = EcologicalDisturbanceState::POST_DISTURBANCE_SNAG_BURN;
                }
                // After burning out fuel in this step, decay active flame intensity so fire front advances rather than burning forever
                bio.wildfire_intensity_kw_m *= 0.35f;
            } else {
                bio.wildfire_intensity_kw_m = 0.0f;
                if (bio.burn_scar_recovery_years < 3.0f) {
                    eco.disturbance_state = EcologicalDisturbanceState::POST_DISTURBANCE_SNAG_BURN;
                } else if (eco.soil_moisture_01 < 0.12f && eco.canopy_biomass_kg_m2 > 8.0f) {
                    eco.disturbance_state = EcologicalDisturbanceState::DROUGHT_BARK_BEETLE_DIEBACK;
                    eco.canopy_biomass_kg_m2 *= (1.0f - 0.05f * std::min(1.0f, dt_years));
                } else if (bio.windthrow_damage_01 < 0.1f) {
                    eco.disturbance_state = EcologicalDisturbanceState::UNDISTURBED;
                }
            }
        }
    }

    /**
     * @brief Ignites a wildfire at grid cell (gx, gy) for interactive testing or lightning strikes.
     */
    static inline bool ignite_wildfire_at_grid(
        const EcologySeedProfilePOD& seed,
        uint32_t gx,
        uint32_t gy,
        float initial_intensity_kw_m,
        std::span<EcologicalCellStatePOD> eco_cells,
        std::span<BiogeochemicalAndTrophicCellPOD> biogeo_cells) noexcept {
        const uint32_t w = seed.grid_width;
        const uint32_t h = seed.grid_height;
        if (gx >= w || gy >= h) {
            return false;
        }
        const size_t idx = static_cast<size_t>(gy * w + gx);
        if (idx >= eco_cells.size() || idx >= biogeo_cells.size()) {
            return false;
        }
        biogeo_cells[idx].dead_fuel_moisture_01 = std::min(biogeo_cells[idx].dead_fuel_moisture_01, 0.12f);
        biogeo_cells[idx].wildfire_intensity_kw_m = std::max(850.0f, initial_intensity_kw_m);
        eco_cells[idx].disturbance_state = EcologicalDisturbanceState::ACTIVE_CROWN_WILDFIRE;
        biogeo_cells[idx].burn_scar_recovery_years = 0.0f;
        return true;
    }

    // ========================================================================
    // 7. Stage ECO_T4: Spatial Lotka-Volterra Trophic Web & Alchemical Potency
    // ========================================================================
    static inline void step_lotka_volterra_trophic_web(
        const EcologySeedProfilePOD& seed,
        float dt_years,
        std::span<EcologicalCellStatePOD> eco_cells,
        std::span<BiogeochemicalAndTrophicCellPOD> biogeo_cells) noexcept {
        const uint32_t w = seed.grid_width;
        const uint32_t h = seed.grid_height;
        const size_t total = static_cast<size_t>(w) * static_cast<size_t>(h);
        if (eco_cells.size() < total || biogeo_cells.size() < total) {
            return;
        }

        const float dt = std::clamp(dt_years, 0.01f, 2.0f);

        for (uint32_t y = 0; y < h; ++y) {
            for (uint32_t x = 0; x < w; ++x) {
                const size_t idx = static_cast<size_t>(y * w + x);
                auto& eco = eco_cells[idx];
                auto& bio = biogeo_cells[idx];

                const float forage_graze = eco.understory_biomass_kg_m2;
                const float forage_browse = eco.canopy_biomass_kg_m2 * 0.25f;

                // Grazing & Browsing Herbivore Lotka-Volterra equations with Holling Type-II saturation
                const float g_intake = (seed.herbivore_grazing_rate * forage_graze) / (1.5f + forage_graze);
                const float b_intake = (seed.herbivore_grazing_rate * forage_browse) / (2.5f + forage_browse);

                const float pred_loss_g = seed.predator_attack_rate * bio.grazer_herbivore_density * bio.apex_predator_density;
                const float pred_loss_b = seed.predator_attack_rate * bio.browser_herbivore_density * bio.apex_predator_density;

                bio.grazer_herbivore_density = std::clamp(
                    bio.grazer_herbivore_density + (bio.grazer_herbivore_density * (g_intake * 1.8f - 0.14f) - pred_loss_g) * dt,
                    0.01f, 2.5f);
                bio.browser_herbivore_density = std::clamp(
                    bio.browser_herbivore_density + (bio.browser_herbivore_density * (b_intake * 1.6f - 0.12f) - pred_loss_b) * dt,
                    0.01f, 2.5f);

                // Herbivory defoliation & manure nitrogen return
                const float consumed_understory = std::min(eco.understory_biomass_kg_m2 * 0.35f, bio.grazer_herbivore_density * 0.28f * dt);
                eco.understory_biomass_kg_m2 -= consumed_understory;
                bio.mineral_nitrogen_g_m2 += consumed_understory * 3.2f;

                // Pollinator mutualism tracks floral diversity & understory bloom
                const float target_pollinator = std::clamp(0.25f + 0.35f * eco.shannon_diversity_h + 0.06f * eco.understory_biomass_kg_m2, 0.05f, 1.5f);
                bio.pollinator_disperser_density = 0.75f * bio.pollinator_disperser_density + 0.25f * target_pollinator;

                // Apex Predator Lotka-Volterra dynamics
                const float total_prey = bio.grazer_herbivore_density + bio.browser_herbivore_density;
                const float pred_growth = seed.predator_conversion_eff * seed.predator_attack_rate * total_prey - 0.16f;
                bio.apex_predator_density = std::clamp(
                    bio.apex_predator_density + (bio.apex_predator_density * pred_growth) * dt,
                    0.005f, 1.2f);

                // Alchemical Phytochemical Potency (peaks in high-diversity ecotones, old-growth mycorrhizal networks & post-fire ash blooms)
                const float leyline = 0.5f + 0.5f * value_noise_2d(
                    static_cast<float>(x) * 0.08f, static_cast<float>(y) * 0.08f, seed.whittaker_niche_seed ^ 0xA1C4ULL);
                bio.alchemical_phytochemical_grade = std::clamp(
                    (0.28f * eco.shannon_diversity_h +
                     0.04f * eco.root_mycorrhizal_kg_m2 +
                     0.35f * leyline * seed.alchemical_leyline_resonance),
                    0.05f, 1.0f);
            }
        }
    }

    // ========================================================================
    // 8. Stage ECO_M1: Holdridge Life-Zone & Seral Succession Partitioning
    // ========================================================================
    static inline void compute_holdridge_and_seral_partition(
        const EcologySeedProfilePOD& seed,
        std::span<EcologicalCellStatePOD> eco_cells,
        std::span<BiogeochemicalAndTrophicCellPOD> biogeo_cells) noexcept {
        const size_t total = static_cast<size_t>(seed.grid_width) * static_cast<size_t>(seed.grid_height);
        if (eco_cells.size() < total || biogeo_cells.size() < total) {
            return;
        }

        for (size_t i = 0; i < total; ++i) {
            auto& eco = eco_cells[i];
            const auto& bio = biogeo_cells[i];

            // 1. Classify Seral Stage from biomass & disturbance recovery
            const float total_bio = eco.canopy_biomass_kg_m2 + eco.understory_biomass_kg_m2;
            if (eco.disturbance_state == EcologicalDisturbanceState::ACTIVE_CROWN_WILDFIRE ||
                eco.disturbance_state == EcologicalDisturbanceState::ACTIVE_SURFACE_BRUSHFIRE ||
                bio.burn_scar_recovery_years < 2.0f) {
                eco.seral_stage = SuccessionSeralStage::POST_DISTURBANCE_SNAG_BURN;
            } else if (total_bio < 0.35f) {
                eco.seral_stage = SuccessionSeralStage::BARE_SUBSTRATE;
            } else if (total_bio < 1.8f) {
                eco.seral_stage = SuccessionSeralStage::PIONEER_LICHEN_BRYOPHYTE;
            } else if (eco.canopy_biomass_kg_m2 < 2.5f) {
                eco.seral_stage = SuccessionSeralStage::EARLY_HERBACEOUS_MEADOW;
            } else if (eco.canopy_biomass_kg_m2 < 8.0f) {
                eco.seral_stage = SuccessionSeralStage::INTERMEDIATE_SHRUB_SCRUB;
            } else if (eco.canopy_biomass_kg_m2 < 18.0f) {
                eco.seral_stage = SuccessionSeralStage::YOUNG_SUBCLIMAX_WOODLAND;
            } else {
                eco.seral_stage = SuccessionSeralStage::OLD_GROWTH_CLIMAX_CANOPY;
            }

            // 2. Classify Holdridge Life-Zone from Biotemperature & Annual Precipitation
            const float t = eco.mean_temperature_c;
            const float p = eco.annual_precipitation_mm;

            if (bio.karst_refugium_humidity_bonus > 0.35f) {
                eco.life_zone = HoldridgeLifeZone::KARST_SUBTERRANEAN_REFUGIUM;
            } else if (eco.soil_moisture_01 > 0.88f && t > 4.0f) {
                eco.life_zone = HoldridgeLifeZone::RIPARIAN_WETLAND_GALLERY;
            } else if (t < -1.5f) {
                eco.life_zone = HoldridgeLifeZone::POLAR_ALPINE_TUNDRA;
            } else if (t < 3.5f) {
                eco.life_zone = HoldridgeLifeZone::SUBALPINE_KRUMMHOLZ_MEADOW;
            } else if (t < 8.5f) {
                eco.life_zone = (p > 420.0f)
                    ? HoldridgeLifeZone::BOREAL_CONIFEROUS_TAIGA
                    : HoldridgeLifeZone::TEMPERATE_GRASSLAND_PRAIRIE;
            } else if (t < 18.5f) {
                if (p < 320.0f) {
                    eco.life_zone = HoldridgeLifeZone::HOT_SUBTROPICAL_DESERT;
                } else if (p < 620.0f) {
                    eco.life_zone = HoldridgeLifeZone::TEMPERATE_GRASSLAND_PRAIRIE;
                } else if (p < 950.0f) {
                    eco.life_zone = HoldridgeLifeZone::WARM_TEMPERATE_WOODLAND;
                } else {
                    eco.life_zone = HoldridgeLifeZone::TEMPERATE_DECIDUOUS_FOREST;
                }
            } else {
                if (p < 280.0f) {
                    eco.life_zone = HoldridgeLifeZone::HOT_SUBTROPICAL_DESERT;
                } else if (p < 650.0f) {
                    eco.life_zone = HoldridgeLifeZone::SUBTROPICAL_THORN_STEPPE;
                } else if (p < 1650.0f) {
                    eco.life_zone = HoldridgeLifeZone::TROPICAL_SEASONAL_FOREST;
                } else {
                    eco.life_zone = HoldridgeLifeZone::TROPICAL_RAIN_FOREST;
                }
            }
        }
    }

    // ========================================================================
    // 9. Stage ECO_Q1: Ecological Harvest, Wildfire Risk & Botanical Query
    // ========================================================================
    static inline void evaluate_ecological_harvest_and_hazards(
        const EcologySeedProfilePOD& seed,
        std::span<const EcologicalCellStatePOD> eco_cells,
        std::span<const BiogeochemicalAndTrophicCellPOD> biogeo_cells,
        std::span<EcologyHarvestAndDisturbanceSamplePOD> out_samples) noexcept {
        const size_t total = static_cast<size_t>(seed.grid_width) * static_cast<size_t>(seed.grid_height);
        if (eco_cells.size() < total || biogeo_cells.size() < total || out_samples.size() < total) {
            return;
        }

        for (size_t i = 0; i < total; ++i) {
            const auto& eco = eco_cells[i];
            const auto& bio = biogeo_cells[i];
            auto& s = out_samples[i];

            const float dryness = std::clamp(1.0f - bio.dead_fuel_moisture_01 * 1.6f, 0.0f, 1.0f);
            const float fuel_load = std::clamp((eco.dead_fuel_litter_kg_m2 + eco.understory_biomass_kg_m2 * 0.5f) / 10.0f, 0.0f, 1.0f);

            s.net_primary_productivity   = bio.net_primary_productivity_npp;
            s.wildfire_spread_risk_01    = std::clamp(dryness * fuel_load * (1.0f + 0.08f * eco.wind_speed_mps), 0.0f, 1.0f);
            s.timber_canopy_yield_kg_m2  = eco.canopy_biomass_kg_m2;
            s.forage_carrying_capacity   = eco.understory_biomass_kg_m2 + eco.canopy_biomass_kg_m2 * 0.15f;
            s.alchemical_botanical_grade = bio.alchemical_phytochemical_grade;
            const float prey = bio.grazer_herbivore_density + bio.browser_herbivore_density;
            s.trophic_cascade_stability  = std::clamp(1.0f - std::abs(prey * 0.25f - bio.apex_predator_density), 0.0f, 1.0f);
            s.cell_index                 = static_cast<uint32_t>(i);
            s.life_zone                  = eco.life_zone;
            s.seral_stage                = eco.seral_stage;
            s.disturbance_state          = eco.disturbance_state;
            s.dominant_guild_id          = eco.dominant_guild_id;
        }
    }
};

} // namespace ideam::core
