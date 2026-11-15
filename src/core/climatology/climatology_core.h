#pragma once

/**
 * @file climatology_core.h
 * @brief Pure C++26 simulation kernels for IdeamClimatology.
 *
 * Implements:
 * 1. Deterministic SplitMix64 seed derivation & 2D FBM synoptic baroclinic waves
 * 2. Solar Insolation, Longwave Stefan-Boltzmann Radiative Balance & Hypsometric Pressure
 * 3. Geostrophic Wind, Coriolis Deflection, Ekman Drag & Orographic Slope Lift
 * 4. Clausius-Clapeyron Saturation Vapor Pressure, Foehn Rain-Shadow Latent Heating & Cloud Microphysics
 * 5. Convective CAPE, Storm-Relative Helicity (SRH), Energy-Helicity Index (EHI) &
 *    Burgers-Rott / Modified Rankine Supercell Tornado Vortex Dynamics
 * 6. Köppen-Geiger Climate Classification, Beaufort Scale & Severe Weather Hazard Queries
 */

#include "core/climatology/climatology_types.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>

namespace ideam::climatology {

struct ClimatologyKernelMath {
    // ========================================================================
    // 1. Deterministic SplitMix64 & 2D FBM Synoptic Noise
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

    static constexpr ClimatologySeedProfilePOD make_seed_profile(
        uint64_t master_seed,
        uint32_t width = 64,
        uint32_t height = 64,
        uint32_t max_vortices = 8,
        float cell_size_m = 50.0f) noexcept {
        ClimatologySeedProfilePOD p{};
        p.master_seed           = master_seed;
        p.solar_insolation_seed = splitmix64(master_seed ^ 0x01ULL);
        p.baroclinic_wave_seed  = splitmix64(master_seed ^ 0x02ULL);
        p.convective_storm_seed = splitmix64(master_seed ^ 0x03ULL);
        p.tornado_vortex_seed   = splitmix64(master_seed ^ 0x04ULL);
        p.grid_width            = width < 8 ? 8 : width;
        p.grid_height           = height < 8 ? 8 : height;
        p.max_vortices          = std::clamp<uint32_t>(max_vortices, 1u, static_cast<uint32_t>(MAX_ACTIVE_TORNADO_VORTICES));
        p.cell_size_meters      = cell_size_m > 1.0f ? cell_size_m : 50.0f;
        return p;
    }

    static constexpr BeaufortWindScale classify_beaufort_scale(float wind_speed_mps) noexcept {
        if (wind_speed_mps < 0.5f)  return BeaufortWindScale::CALM_0;
        if (wind_speed_mps < 1.6f)  return BeaufortWindScale::LIGHT_AIR_1;
        if (wind_speed_mps < 3.4f)  return BeaufortWindScale::LIGHT_BREEZE_2;
        if (wind_speed_mps < 5.5f)  return BeaufortWindScale::GENTLE_BREEZE_3;
        if (wind_speed_mps < 8.0f)  return BeaufortWindScale::MODERATE_BREEZE_4;
        if (wind_speed_mps < 10.8f) return BeaufortWindScale::FRESH_BREEZE_5;
        if (wind_speed_mps < 13.9f) return BeaufortWindScale::STRONG_BREEZE_6;
        if (wind_speed_mps < 17.2f) return BeaufortWindScale::NEAR_GALE_7;
        if (wind_speed_mps < 20.8f) return BeaufortWindScale::GALE_8;
        if (wind_speed_mps < 24.5f) return BeaufortWindScale::STRONG_GALE_9;
        if (wind_speed_mps < 28.5f) return BeaufortWindScale::STORM_10;
        if (wind_speed_mps < 32.7f) return BeaufortWindScale::VIOLENT_STORM_11;
        return BeaufortWindScale::HURRICANE_12;
    }

    static constexpr EnhancedFujitaScale classify_ef_scale(float gust_mps) noexcept {
        if (gust_mps < 29.0f) return EnhancedFujitaScale::EF_NONE;
        if (gust_mps < 38.0f) return EnhancedFujitaScale::EF0_GALE;
        if (gust_mps < 49.0f) return EnhancedFujitaScale::EF1_MODERATE;
        if (gust_mps < 60.0f) return EnhancedFujitaScale::EF2_SIGNIFICANT;
        if (gust_mps < 74.0f) return EnhancedFujitaScale::EF3_SEVERE;
        if (gust_mps < 89.0f) return EnhancedFujitaScale::EF4_DEVASTATING;
        return EnhancedFujitaScale::EF5_INCREDIBLE;
    }

    /**
     * @brief Magnus-Tetens saturation vapor pressure e_s(T) in hPa for T in Kelvin.
     */
    static inline float saturation_vapor_pressure_hpa(float temp_k) noexcept {
        const float temp_c = std::clamp(temp_k - 273.15f, -80.0f, 60.0f);
        return 6.1094f * std::exp((17.625f * temp_c) / (temp_c + 243.04f));
    }

    /**
     * @brief Saturation specific humidity q_s(T, P) in kg/kg.
     */
    static inline float saturation_specific_humidity(float temp_k, float pressure_hpa) noexcept {
        const float es = saturation_vapor_pressure_hpa(temp_k);
        const float p = std::max(250.0f, pressure_hpa);
        return std::clamp((0.622f * es) / std::max(10.0f, p - 0.378f * es), 0.0001f, 0.045f);
    }

    // ========================================================================
    // 2. Initial World Climate & Synthetic Orography Initialization
    // ========================================================================
    static inline void initialize_atmosphere_from_seed(
        const ClimatologySeedProfilePOD& seed,
        std::span<const float> optional_terrain_elevations_m,
        std::span<AtmosphericColumnCellPOD> out_atmo,
        std::span<MoistureAndCloudCellPOD> out_moisture,
        std::span<TornadoSupercellVortexPOD> out_vortices) noexcept {
        const uint32_t w = seed.grid_width;
        const uint32_t h = seed.grid_height;
        const size_t total = static_cast<size_t>(w) * static_cast<size_t>(h);
        if (out_atmo.size() < total || out_moisture.size() < total) {
            return;
        }

        for (size_t i = 0; i < out_vortices.size(); ++i) {
            out_vortices[i] = TornadoSupercellVortexPOD{};
            out_vortices[i].vortex_id = static_cast<uint16_t>(i);
        }

        for (uint32_t y = 0; y < h; ++y) {
            const float lat_norm = (static_cast<float>(y) / static_cast<float>(std::max(1u, h - 1))) * 2.0f - 1.0f;
            for (uint32_t x = 0; x < w; ++x) {
                const size_t idx = static_cast<size_t>(y * w + x);
                auto& atmo = out_atmo[idx];
                auto& moist = out_moisture[idx];
                atmo = AtmosphericColumnCellPOD{};
                moist = MoistureAndCloudCellPOD{};

                // Use provided terrain elevation or synthesize an orographic ridge + valley basin
                float elev_m = 0.0f;
                if (optional_terrain_elevations_m.size() >= total) {
                    elev_m = optional_terrain_elevations_m[idx];
                } else {
                    const float nx = static_cast<float>(x) / static_cast<float>(w);
                    const float ny = static_cast<float>(y) / static_cast<float>(h);
                    const float ridge = std::exp(-((nx - 0.42f) * (nx - 0.42f)) / 0.028f) * 1450.0f;
                    const float rough = (fbm_2d(nx * 4.5f, ny * 4.5f, seed.solar_insolation_seed, 4) + 0.5f) * 420.0f;
                    elev_m = std::max(0.0f, ridge + rough - 140.0f);
                }

                atmo.surface_elevation_m = elev_m;
                atmo.ocean_water_fraction = (elev_m <= 5.0f) ? 1.0f : 0.0f;
                atmo.surface_albedo = (elev_m > 1550.0f) ? 0.62f : (atmo.ocean_water_fraction > 0.5f ? 0.08f : 0.22f);
                atmo.surface_roughness_z0 = (atmo.ocean_water_fraction > 0.5f) ? 0.002f : (elev_m > 900.0f ? 0.65f : 0.18f);

                // Baroclinic pressure wave & temperature initialization
                const float wave = fbm_2d(static_cast<float>(x) * 0.055f, static_cast<float>(y) * 0.055f, seed.baroclinic_wave_seed, 3);
                const float lat_temp_offset = -12.0f * lat_norm;
                const float sea_temp_k = seed.sea_level_temp_k + lat_temp_offset + wave * 4.5f;
                atmo.surface_air_temperature_k = sea_temp_k - seed.dry_adiabatic_lapse_k_m * elev_m;
                atmo.potential_temperature_k = sea_temp_k;
                atmo.upper_troposphere_temp_k = atmo.surface_air_temperature_k - 46.0f + wave * 3.0f;

                // Hypsometric equation for surface pressure
                atmo.boundary_pressure_hpa = seed.sea_level_pressure_hpa *
                    std::exp(-elev_m / 8430.0f) + wave * seed.synoptic_wave_amplitude_hpa;
                atmo.air_density_kg_m3 = (atmo.boundary_pressure_hpa * 100.0f) /
                    (287.05f * std::max(210.0f, atmo.surface_air_temperature_k));

                atmo.wind_velocity_u_mps = seed.prevailing_zonal_wind_mps;
                atmo.wind_velocity_v_mps = seed.prevailing_merid_wind_mps;

                // Initial moisture
                const float rh_init = std::clamp(0.62f + 0.25f * atmo.ocean_water_fraction + 0.18f * wave, 0.18f, 0.96f);
                const float qs = saturation_specific_humidity(atmo.surface_air_temperature_k, atmo.boundary_pressure_hpa);
                moist.relative_humidity_01 = rh_init;
                moist.specific_humidity_kg_kg = rh_init * qs;
                moist.precipitable_water_mm = moist.specific_humidity_kg_kg * 2200.0f;
            }
        }

        // Run one full diagnostic pass so winds, Foehn clouds, CAPE, and Köppen classes are ready immediately
        step_radiative_and_synoptic_pressure(seed, 60.0f, out_atmo);
        step_geostrophic_and_orographic_wind(seed, 60.0f, out_atmo);
        step_thermodynamics_and_microphysics(seed, 60.0f, out_atmo, out_moisture);
        step_convective_cape_and_tornado_vortices(seed, 60.0f, out_atmo, out_moisture, out_vortices);
        compute_koppen_and_air_mass_partition(seed, out_atmo, out_moisture);
    }

    // ========================================================================
    // 3. Stage CLM_T1: Solar Insolation, Longwave Budget & Synoptic Pressure
    // ========================================================================
    static inline void step_radiative_and_synoptic_pressure(
        const ClimatologySeedProfilePOD& seed,
        float dt_seconds,
        std::span<AtmosphericColumnCellPOD> atmo_cells) noexcept {
        const uint32_t w = seed.grid_width;
        const uint32_t h = seed.grid_height;
        const size_t total = static_cast<size_t>(w) * static_cast<size_t>(h);
        if (atmo_cells.size() < total) {
            return;
        }

        constexpr float kPi = 3.1415926535f;
        const float lat_rad = seed.planet_latitude_deg * (kPi / 180.0f);
        const float decl_rad = seed.seasonal_solar_decl_deg * (kPi / 180.0f);
        const float hour_angle_rad = (seed.diurnal_hour_0_24 - 12.0f) * (kPi / 12.0f);
        const float base_cos_zenith = std::max(0.05f,
            std::sin(lat_rad) * std::sin(decl_rad) +
            std::cos(lat_rad) * std::cos(decl_rad) * std::cos(hour_angle_rad));

        const float relax = std::clamp(dt_seconds * 0.0025f, 0.005f, 0.25f);

        for (uint32_t y = 0; y < h; ++y) {
            const float lat_band = (static_cast<float>(y) / static_cast<float>(std::max(1u, h - 1))) * 2.0f - 1.0f;
            for (uint32_t x = 0; x < w; ++x) {
                const size_t idx = static_cast<size_t>(y * w + x);
                auto& c = atmo_cells[idx];

                const float local_zenith = std::clamp(base_cos_zenith - 0.18f * lat_band, 0.05f, 1.0f);
                c.shortwave_insolation_w_m2 = seed.solar_constant_w_m2 * local_zenith * (1.0f - c.surface_albedo);

                // linearized Stefan-Boltzmann outgoing longwave radiation
                const float t_norm = c.surface_air_temperature_k / 288.15f;
                c.longwave_outgoing_w_m2 = 235.0f * (t_norm * t_norm * t_norm * t_norm);

                const float net_rad = c.shortwave_insolation_w_m2 - c.longwave_outgoing_w_m2;
                const float heat_cap_scale = (c.ocean_water_fraction > 0.5f) ? 0.25f : 1.0f;
                const float target_theta = seed.sea_level_temp_k - 11.0f * lat_band + (net_rad * 0.018f * heat_cap_scale);

                c.potential_temperature_k = c.potential_temperature_k + relax * (target_theta - c.potential_temperature_k);
                c.surface_air_temperature_k = c.potential_temperature_k - seed.dry_adiabatic_lapse_k_m * c.surface_elevation_m;

                // Synoptic baroclinic wave + thermal low perturbation
                const float syn_wave = fbm_2d(
                    static_cast<float>(x) * 0.055f + dt_seconds * 0.0005f,
                    static_cast<float>(y) * 0.055f,
                    seed.baroclinic_wave_seed, 3);
                const float thermal_pressure_anomaly = -(c.potential_temperature_k - seed.sea_level_temp_k) * 0.65f;
                c.boundary_pressure_hpa = seed.sea_level_pressure_hpa * std::exp(-c.surface_elevation_m / 8430.0f) +
                    syn_wave * seed.synoptic_wave_amplitude_hpa + thermal_pressure_anomaly;

                c.air_density_kg_m3 = (c.boundary_pressure_hpa * 100.0f) /
                    (287.05f * std::max(210.0f, c.surface_air_temperature_k));
            }
        }
    }

    // ========================================================================
    // 4. Stage CLM_T2: Geostrophic, Coriolis, Ekman & Orographic Wind Solver
    // ========================================================================
    static inline void step_geostrophic_and_orographic_wind(
        const ClimatologySeedProfilePOD& seed,
        float dt_seconds,
        std::span<AtmosphericColumnCellPOD> atmo_cells) noexcept {
        const uint32_t w = seed.grid_width;
        const uint32_t h = seed.grid_height;
        const size_t total = static_cast<size_t>(w) * static_cast<size_t>(h);
        if (atmo_cells.size() < total) {
            return;
        }

        const float inv_2dx = 1.0f / (2.0f * std::max(1.0f, seed.cell_size_meters));
        const float blend = std::clamp(dt_seconds * 0.08f, 0.05f, 0.65f);

        for (uint32_t y = 0; y < h; ++y) {
            const uint32_t ym = (y > 0) ? (y - 1) : 0;
            const uint32_t yp = (y + 1 < h) ? (y + 1) : (h - 1);
            for (uint32_t x = 0; x < w; ++x) {
                const uint32_t xm = (x > 0) ? (x - 1) : 0;
                const uint32_t xp = (x + 1 < w) ? (x + 1) : (w - 1);
                const size_t idx = static_cast<size_t>(y * w + x);
                auto& c = atmo_cells[idx];

                // Sea-level reduced pressure gradient so geostrophic wind responds to synoptic pressure systems
                const auto p_sl = [&](uint32_t gx, uint32_t gy) noexcept -> float {
                    const auto& cell = atmo_cells[static_cast<size_t>(gy * w + gx)];
                    return cell.boundary_pressure_hpa * std::exp(cell.surface_elevation_m / 8430.0f);
                };

                const float dpdx_pa_m = (p_sl(xp, y) - p_sl(xm, y)) * 100.0f * inv_2dx;
                const float dpdy_pa_m = (p_sl(x, yp) - p_sl(x, ym)) * 100.0f * inv_2dx;

                // Cross-isobar Ekman-balanced geostrophic wind + prevailing jet baseline
                const float drag_factor = 1.0f / (1.0f + 1.8f * c.surface_roughness_z0);
                const float u_geo = (seed.prevailing_zonal_wind_mps - dpdy_pa_m * 140.0f - dpdx_pa_m * 45.0f) * drag_factor;
                const float v_geo = (seed.prevailing_merid_wind_mps + dpdx_pa_m * 140.0f - dpdy_pa_m * 45.0f) * drag_factor;

                c.wind_velocity_u_mps = c.wind_velocity_u_mps + blend * (u_geo - c.wind_velocity_u_mps);
                c.wind_velocity_v_mps = c.wind_velocity_v_mps + blend * (v_geo - c.wind_velocity_v_mps);

                // Terrain elevation gradient for Orographic Lift: w_oro = V_h dot grad(z_surface)
                const float dzdx = (atmo_cells[static_cast<size_t>(y * w + xp)].surface_elevation_m -
                                    atmo_cells[static_cast<size_t>(y * w + xm)].surface_elevation_m) * inv_2dx;
                const float dzdy = (atmo_cells[static_cast<size_t>(yp * w + x)].surface_elevation_m -
                                    atmo_cells[static_cast<size_t>(ym * w + x)].surface_elevation_m) * inv_2dx;

                c.orographic_lift_mps = (c.wind_velocity_u_mps * dzdx + c.wind_velocity_v_mps * dzdy) * seed.orographic_lift_scale;
                const float thermal_buoyancy_w = std::max(0.0f, (c.potential_temperature_k - seed.sea_level_temp_k) * 0.12f);
                c.vertical_updraft_w_mps = c.orographic_lift_mps + thermal_buoyancy_w;

                const float spd = std::sqrt(c.wind_velocity_u_mps * c.wind_velocity_u_mps +
                                            c.wind_velocity_v_mps * c.wind_velocity_v_mps);
                c.beaufort_scale = classify_beaufort_scale(spd);
            }
        }
    }

    // ========================================================================
    // 5. Stage CLM_T3: Clausius-Clapeyron, Foehn Rain-Shadow & Microphysics
    // ========================================================================
    static inline void step_thermodynamics_and_microphysics(
        const ClimatologySeedProfilePOD& seed,
        float dt_seconds,
        std::span<AtmosphericColumnCellPOD> atmo_cells,
        std::span<MoistureAndCloudCellPOD> moisture_cells) noexcept {
        const uint32_t w = seed.grid_width;
        const uint32_t h = seed.grid_height;
        const size_t total = static_cast<size_t>(w) * static_cast<size_t>(h);
        if (atmo_cells.size() < total || moisture_cells.size() < total) {
            return;
        }

        const float dt_hr = std::max(0.001f, dt_seconds / 3600.0f);

        // Semi-Lagrangian upwind moisture advection buffer
        std::vector<float> advected_q(total);
        for (uint32_t y = 0; y < h; ++y) {
            for (uint32_t x = 0; x < w; ++x) {
                const size_t idx = static_cast<size_t>(y * w + x);
                const auto& c = atmo_cells[idx];
                const float src_x = std::clamp(static_cast<float>(x) - (c.wind_velocity_u_mps * dt_seconds) / seed.cell_size_meters, 0.0f, static_cast<float>(w - 1));
                const float src_y = std::clamp(static_cast<float>(y) - (c.wind_velocity_v_mps * dt_seconds) / seed.cell_size_meters, 0.0f, static_cast<float>(h - 1));
                const uint32_t ix = static_cast<uint32_t>(std::round(src_x));
                const uint32_t iy = static_cast<uint32_t>(std::round(src_y));
                advected_q[idx] = moisture_cells[static_cast<size_t>(iy * w + ix)].specific_humidity_kg_kg;
            }
        }

        for (size_t idx = 0; idx < total; ++idx) {
            auto& c = atmo_cells[idx];
            auto& m = moisture_cells[idx];

            m.specific_humidity_kg_kg = 0.75f * m.specific_humidity_kg_kg + 0.25f * advected_q[idx];

            // Surface evaporation over oceans/wet terrain
            const float wind_spd = std::sqrt(c.wind_velocity_u_mps * c.wind_velocity_u_mps + c.wind_velocity_v_mps * c.wind_velocity_v_mps);
            const float qs = saturation_specific_humidity(c.surface_air_temperature_k, c.boundary_pressure_hpa);
            const float evap_potential = std::max(0.0f, qs - m.specific_humidity_kg_kg) * (0.25f + 0.75f * c.ocean_water_fraction) * (1.0f + 0.12f * wind_spd);
            m.surface_evaporation_mm_hr = evap_potential * 180.0f;
            m.specific_humidity_kg_kg = std::min(0.042f, m.specific_humidity_kg_kg + evap_potential * 0.08f);

            // Orographic adiabatic cooling on windward ascent vs. Foehn warming on leeward descent
            if (c.orographic_lift_mps > 0.05f) {
                // Windward condensation & latent heat release (Foehn source)
                const float lift_cond = std::min(m.specific_humidity_kg_kg * 0.22f, c.orographic_lift_mps * 0.0012f);
                m.specific_humidity_kg_kg -= lift_cond;
                m.cloud_liquid_water_kg_m2 += lift_cond * 180.0f;
                c.potential_temperature_k += lift_cond * 210.0f; // Latent heat warms potential temperature!
            } else if (c.orographic_lift_mps < -0.05f) {
                // Leeward Foehn dry-adiabatic descent warming & cloud evaporation
                const float sub_evap = std::min(m.cloud_liquid_water_kg_m2, std::abs(c.orographic_lift_mps) * 0.18f);
                m.cloud_liquid_water_kg_m2 -= sub_evap;
                c.surface_air_temperature_k += std::min(4.5f, std::abs(c.orographic_lift_mps) * 0.55f);
            }

            // Relative humidity & supersaturation condensation
            m.relative_humidity_01 = std::clamp(m.specific_humidity_kg_kg / std::max(0.0002f, qs), 0.02f, 1.0f);
            if (m.specific_humidity_kg_kg > qs) {
                const float excess = m.specific_humidity_kg_kg - qs;
                m.specific_humidity_kg_kg = qs;
                if (c.surface_air_temperature_k < 271.15f) {
                    m.cloud_ice_water_kg_m2 += excess * 220.0f;
                } else {
                    m.cloud_liquid_water_kg_m2 += excess * 220.0f;
                }
            }

            // Espy's equation for Lifting Condensation Level (LCL above ground)
            const float dew_depression_k = (1.0f - m.relative_humidity_01) * 22.0f;
            m.lifting_condensation_lvl_m = std::max(80.0f, 125.0f * dew_depression_k);

            // Cloud autoconversion to precipitation (rain vs. snow)
            const float total_cloud_water = m.cloud_liquid_water_kg_m2 + m.cloud_ice_water_kg_m2;
            m.cloud_cover_fraction = std::clamp(total_cloud_water / 1.6f, 0.0f, 1.0f);

            if (total_cloud_water > 0.35f) {
                const float precip_out = (total_cloud_water - 0.25f) * 0.38f;
                m.cloud_liquid_water_kg_m2 *= 0.72f;
                m.cloud_ice_water_kg_m2 *= 0.72f;
                if (c.surface_air_temperature_k < 273.15f) {
                    m.snowfall_rate_mm_hr = precip_out * 14.0f;
                    m.rainfall_rate_mm_hr = 0.0f;
                    m.snowpack_water_equiv_mm += m.snowfall_rate_mm_hr * dt_hr;
                } else {
                    m.rainfall_rate_mm_hr = precip_out * 14.0f;
                    m.snowfall_rate_mm_hr = 0.0f;
                }
            } else {
                m.rainfall_rate_mm_hr = 0.0f;
                m.snowfall_rate_mm_hr = 0.0f;
            }

            // Snowpack thermal melt
            if (c.surface_air_temperature_k > 274.15f && m.snowpack_water_equiv_mm > 0.0f) {
                const float melt = std::min(m.snowpack_water_equiv_mm, (c.surface_air_temperature_k - 273.15f) * 0.45f * dt_hr);
                m.snowpack_water_equiv_mm -= melt;
                m.rainfall_rate_mm_hr += melt / dt_hr;
            }

            m.precipitable_water_mm = m.specific_humidity_kg_kg * 2200.0f + total_cloud_water;
            m.latent_heat_flux_w_m2 = m.surface_evaporation_mm_hr * 68.0f;
        }
    }

    // ========================================================================
    // 6. Stage CLM_T4: Convective CAPE, Helicity & Supercell Tornado Dynamics
    // ========================================================================
    static inline void step_convective_cape_and_tornado_vortices(
        const ClimatologySeedProfilePOD& seed,
        float dt_seconds,
        std::span<AtmosphericColumnCellPOD> atmo_cells,
        std::span<MoistureAndCloudCellPOD> moisture_cells,
        std::span<TornadoSupercellVortexPOD> vortices) noexcept {
        const uint32_t w = seed.grid_width;
        const uint32_t h = seed.grid_height;
        const size_t total = static_cast<size_t>(w) * static_cast<size_t>(h);
        if (atmo_cells.size() < total || moisture_cells.size() < total) {
            return;
        }

        // 1. Evaluate CAPE and 0-3 km Storm-Relative Helicity (SRH) across grid
        uint32_t peak_ehi_x = w / 2;
        uint32_t peak_ehi_y = h / 2;
        float peak_ehi = 0.0f;

        for (uint32_t y = 0; y < h; ++y) {
            for (uint32_t x = 0; x < w; ++x) {
                const size_t idx = static_cast<size_t>(y * w + x);
                auto& c = atmo_cells[idx];
                auto& m = moisture_cells[idx];

                const float parcel_excess_k = (c.surface_air_temperature_k + m.specific_humidity_kg_kg * 1850.0f) -
                                              (c.upper_troposphere_temp_k + 42.0f);
                m.convective_cape_j_kg = std::clamp(parcel_excess_k * 165.0f, 0.0f, 5200.0f);

                const float shear_mag = std::sqrt(c.wind_velocity_u_mps * c.wind_velocity_u_mps +
                                                  c.wind_velocity_v_mps * c.wind_velocity_v_mps);
                m.storm_rel_helicity_m2_s2 = std::clamp(shear_mag * 14.5f + std::max(0.0f, c.vertical_updraft_w_mps) * 35.0f, 0.0f, 650.0f);

                // Energy-Helicity Index (EHI) = (CAPE * SRH) / 160,000
                const float ehi = (m.convective_cape_j_kg * m.storm_rel_helicity_m2_s2) / 160000.0f;
                if (ehi > peak_ehi) {
                    peak_ehi = ehi;
                    peak_ehi_x = x;
                    peak_ehi_y = y;
                }

                m.vortex_tangential_wind_mps = 0.0f;
                m.local_ef_tier = EnhancedFujitaScale::EF_NONE;
            }
        }

        // 2. Spawn a Supercell Tornado Vortex at the peak EHI cell if none active and threshold met
        bool any_active = false;
        for (const auto& v : vortices) {
            if (v.is_active != 0) {
                any_active = true;
                break;
            }
        }
        if (!any_active && !vortices.empty() && peak_ehi >= seed.tornado_spawn_ehi_threshold) {
            spawn_supercell_tornado_at_grid(seed, peak_ehi_x, peak_ehi_y, 64.0f, 95.0f, vortices);
        }

        // 3. Advect active Tornado Vortices and stamp Burgers-Rott / Modified Rankine Vortex fields onto cells
        for (auto& v : vortices) {
            if (v.is_active == 0) {
                continue;
            }
            v.position_x_m += v.translation_u_mps * dt_seconds;
            v.position_z_m += v.translation_v_mps * dt_seconds;
            v.remaining_lifetime_sec -= dt_seconds;

            const float domain_w_m = static_cast<float>(w) * seed.cell_size_meters;
            const float domain_h_m = static_cast<float>(h) * seed.cell_size_meters;
            if (v.remaining_lifetime_sec <= 0.0f ||
                v.position_x_m < 0.0f || v.position_x_m >= domain_w_m ||
                v.position_z_m < 0.0f || v.position_z_m >= domain_h_m) {
                v.is_active = 0;
                continue;
            }

            v.parent_cell_x = static_cast<uint16_t>(std::clamp<int>(static_cast<int>(v.position_x_m / seed.cell_size_meters), 0, static_cast<int>(w - 1)));
            v.parent_cell_y = static_cast<uint16_t>(std::clamp<int>(static_cast<int>(v.position_z_m / seed.cell_size_meters), 0, static_cast<int>(h - 1)));
            v.ef_rating = classify_ef_scale(v.max_tangential_wind_mps);

            const float rc = std::max(10.0f, v.core_radius_m);
            const float r_Influence = std::max(rc * 3.0f, v.outer_damage_radius_m);

            for (uint32_t y = 0; y < h; ++y) {
                const float cell_z = (static_cast<float>(y) + 0.5f) * seed.cell_size_meters;
                const float dz = cell_z - v.position_z_m;
                if (std::abs(dz) > r_Influence) {
                    continue;
                }
                for (uint32_t x = 0; x < w; ++x) {
                    const float cell_x = (static_cast<float>(x) + 0.5f) * seed.cell_size_meters;
                    const float dx = cell_x - v.position_x_m;
                    const float r2 = dx * dx + dz * dz;
                    if (r2 > r_Influence * r_Influence) {
                        continue;
                    }

                    const float r = std::sqrt(r2);
                    // Burgers-Rott / Smooth Rankine tangential velocity profile: V_theta(r) = V_max * (2 * r * Rc) / (r^2 + Rc^2)
                    const float v_theta = v.max_tangential_wind_mps * (2.0f * r * rc) / (r2 + rc * rc);
                    const float core_gauss = std::exp(-r2 / (2.0f * rc * rc));

                    const size_t idx = static_cast<size_t>(y * w + x);
                    auto& c = atmo_cells[idx];
                    auto& m = moisture_cells[idx];

                    if (r > 1e-3f) {
                        const float inv_r = 1.0f / r;
                        // Counter-clockwise cyclonic tangential unit vector (-dz/r, dx/r) + radial inflow (-dx/r, -dz/r)
                        const float u_vort = v_theta * ((-dz * inv_r) - v.radial_inflow_ratio * (dx * inv_r));
                        const float v_vort = v_theta * (( dx * inv_r) - v.radial_inflow_ratio * (dz * inv_r));
                        c.wind_velocity_u_mps += u_vort * 0.35f;
                        c.wind_velocity_v_mps += v_vort * 0.35f;
                    }

                    c.boundary_pressure_hpa -= v.central_pressure_deficit_hpa * core_gauss;
                    c.vertical_updraft_w_mps += v.vertical_suction_updraft_mps * core_gauss;
                    m.vortex_tangential_wind_mps = std::max(m.vortex_tangential_wind_mps, v_theta);
                    m.local_ef_tier = classify_ef_scale(m.vortex_tangential_wind_mps);
                    if (m.vortex_tangential_wind_mps >= 29.0f) {
                        m.hydrometeor_phase = HydrometeorPhaseType::SUPERCELL_MESOCYCLONE;
                    }
                    const float total_spd = std::hypot(c.wind_velocity_u_mps, c.wind_velocity_v_mps) + m.vortex_tangential_wind_mps;
                    c.beaufort_scale = classify_beaufort_scale(total_spd);
                }
            }
        }
    }

    /**
     * @brief Spawns or activates a Supercell Tornado Vortex at grid cell (gx, gy).
     */
    static inline bool spawn_supercell_tornado_at_grid(
        const ClimatologySeedProfilePOD& seed,
        uint32_t gx,
        uint32_t gy,
        float max_tangential_wind_mps,
        float core_radius_m,
        std::span<TornadoSupercellVortexPOD> vortices) noexcept {
        if (vortices.empty()) {
            return false;
        }
        size_t slot = 0;
        for (size_t i = 0; i < vortices.size(); ++i) {
            if (vortices[i].is_active == 0) {
                slot = i;
                break;
            }
        }
        auto& v = vortices[slot];
        v.vortex_id = static_cast<uint16_t>(slot);
        v.parent_cell_x = static_cast<uint16_t>(std::min(gx, seed.grid_width - 1));
        v.parent_cell_y = static_cast<uint16_t>(std::min(gy, seed.grid_height - 1));
        v.is_active = 1;
        v.position_x_m = (static_cast<float>(v.parent_cell_x) + 0.5f) * seed.cell_size_meters;
        v.position_z_m = (static_cast<float>(v.parent_cell_y) + 0.5f) * seed.cell_size_meters;
        v.translation_u_mps = seed.prevailing_zonal_wind_mps * 0.85f;
        v.translation_v_mps = seed.prevailing_merid_wind_mps * 0.85f + 2.5f;
        v.core_radius_m = std::max(15.0f, core_radius_m);
        v.outer_damage_radius_m = v.core_radius_m * 5.5f;
        v.max_tangential_wind_mps = std::max(30.0f, max_tangential_wind_mps);
        v.central_pressure_deficit_hpa = (v.max_tangential_wind_mps * v.max_tangential_wind_mps) * 0.012f;
        v.vertical_suction_updraft_mps = v.max_tangential_wind_mps * 0.62f;
        v.radial_inflow_ratio = 0.42f;
        v.debris_load_kg_m3 = std::clamp((v.max_tangential_wind_mps - 25.0f) * 0.015f, 0.05f, 1.5f);
        v.swirl_ratio_s = 1.18f;
        v.remaining_lifetime_sec = 240.0f;
        v.ef_rating = classify_ef_scale(v.max_tangential_wind_mps);
        return true;
    }

    // ========================================================================
    // 7. Stage CLM_M1: Köppen-Geiger & Air-Mass Front Partitioning
    // ========================================================================
    static inline void compute_koppen_and_air_mass_partition(
        const ClimatologySeedProfilePOD& seed,
        std::span<AtmosphericColumnCellPOD> atmo_cells,
        std::span<MoistureAndCloudCellPOD> moisture_cells) noexcept {
        const size_t total = static_cast<size_t>(seed.grid_width) * static_cast<size_t>(seed.grid_height);
        if (atmo_cells.size() < total || moisture_cells.size() < total) {
            return;
        }

        for (size_t i = 0; i < total; ++i) {
            auto& c = atmo_cells[i];
            auto& m = moisture_cells[i];

            const float temp_c = c.surface_air_temperature_k - 273.15f;
            const float ann_precip_mm = std::clamp(
                (m.precipitable_water_mm * 38.0f) + (m.rainfall_rate_mm_hr + m.snowfall_rate_mm_hr) * 240.0f,
                25.0f, 4500.0f);
            m.annual_precip_estimate_mm = static_cast<uint16_t>(ann_precip_mm);

            // Classify dominant Hydrometeor phase (unless already in a Supercell Mesocyclone)
            if (m.vortex_tangential_wind_mps >= 29.0f) {
                m.hydrometeor_phase = HydrometeorPhaseType::SUPERCELL_MESOCYCLONE;
            } else if (m.snowfall_rate_mm_hr > 1.5f) {
                m.hydrometeor_phase = HydrometeorPhaseType::BLIZZARD_SNOW;
            } else if (m.convective_cape_j_kg > 1800.0f && m.rainfall_rate_mm_hr > 2.0f) {
                m.hydrometeor_phase = HydrometeorPhaseType::GRAUPEL_HAIL_SQUALL;
            } else if (m.convective_cape_j_kg > 950.0f && m.cloud_cover_fraction > 0.55f) {
                m.hydrometeor_phase = HydrometeorPhaseType::CUMULONIMBUS_STORM;
            } else if (m.rainfall_rate_mm_hr > 0.25f) {
                m.hydrometeor_phase = HydrometeorPhaseType::NIMBOSTRATUS_RAIN;
            } else if (m.relative_humidity_01 > 0.92f && c.surface_elevation_m < 400.0f) {
                m.hydrometeor_phase = HydrometeorPhaseType::STRATUS_FOG;
            } else if (m.cloud_cover_fraction > 0.20f) {
                m.hydrometeor_phase = HydrometeorPhaseType::CUMULUS_FAIR;
            } else {
                m.hydrometeor_phase = HydrometeorPhaseType::CLEAR_SKY;
            }

            // Classify Köppen-Geiger climate zone
            if (temp_c < -6.0f) {
                c.koppen_zone = KoppenClimateZone::POLAR_ICE_CAP_EF;
            } else if (temp_c < 2.0f || c.surface_elevation_m > 1650.0f) {
                c.koppen_zone = KoppenClimateZone::ALPINE_TUNDRA_ET;
            } else if (ann_precip_mm < 220.0f) {
                c.koppen_zone = KoppenClimateZone::HOT_ARID_DESERT_BWH;
            } else if (ann_precip_mm < 420.0f) {
                c.koppen_zone = KoppenClimateZone::COLD_STEPPE_BSK;
            } else if (temp_c >= 22.0f) {
                c.koppen_zone = (ann_precip_mm > 1500.0f)
                    ? KoppenClimateZone::TROPICAL_RAINFOREST_AF
                    : KoppenClimateZone::TROPICAL_SAVANNA_AW;
            } else if (temp_c < 8.5f) {
                c.koppen_zone = KoppenClimateZone::CONTINENTAL_BOREAL_DFC;
            } else if (ann_precip_mm < 680.0f && temp_c > 15.0f) {
                c.koppen_zone = KoppenClimateZone::MEDITERRANEAN_CSA;
            } else if (temp_c > 16.5f) {
                c.koppen_zone = KoppenClimateZone::HUMID_SUBTROPICAL_CFA;
            } else {
                c.koppen_zone = KoppenClimateZone::OCEANIC_TEMPERATE_CFB;
            }

            // Air-mass front ID: 0 = Warm Tropical, 1 = Baroclinic Frontal Zone, 2 = Cold Polar
            if (c.potential_temperature_k > seed.sea_level_temp_k + 3.0f) {
                c.air_mass_front_id = 0;
            } else if (c.potential_temperature_k < seed.sea_level_temp_k - 3.0f) {
                c.air_mass_front_id = 2;
            } else {
                c.air_mass_front_id = 1;
            }
        }
    }

    // ========================================================================
    // 8. Stage CLM_Q1: Severe Weather & Vortex Hazard Query Evaluation
    // ========================================================================
    static inline void evaluate_severe_weather_hazards(
        const ClimatologySeedProfilePOD& seed,
        std::span<const AtmosphericColumnCellPOD> atmo_cells,
        std::span<const MoistureAndCloudCellPOD> moisture_cells,
        std::span<ClimatologyHazardAndBiomeSamplePOD> out_hazards) noexcept {
        const size_t total = static_cast<size_t>(seed.grid_width) * static_cast<size_t>(seed.grid_height);
        if (atmo_cells.size() < total || moisture_cells.size() < total || out_hazards.size() < total) {
            return;
        }

        for (size_t i = 0; i < total; ++i) {
            const auto& c = atmo_cells[i];
            const auto& m = moisture_cells[i];
            auto& h = out_hazards[i];

            const float base_wind = std::hypot(c.wind_velocity_u_mps, c.wind_velocity_v_mps);
            h.tornado_wind_gust_mps = base_wind + m.vortex_tangential_wind_mps;
            const float expected_p = seed.sea_level_pressure_hpa * std::exp(-c.surface_elevation_m / 8430.0f);
            h.barometric_suction_pa = std::max(0.0f, (expected_p - c.boundary_pressure_hpa) * 100.0f);
            h.debris_ballistic_flux = std::clamp((h.tornado_wind_gust_mps - 25.0f) / 65.0f, 0.0f, 1.0f);
            h.blizzard_whiteout_index = std::clamp((m.snowfall_rate_mm_hr * 0.15f) * (base_wind / 15.0f), 0.0f, 1.0f);
            h.drought_aridity_index = std::clamp(1.0f - (m.relative_humidity_01 * 1.15f), 0.0f, 1.0f);
            h.lightning_strike_prob = std::clamp((m.convective_cape_j_kg / 3500.0f) * m.cloud_cover_fraction, 0.0f, 1.0f);
            h.cell_index = static_cast<uint32_t>(i);
            h.koppen_zone = c.koppen_zone;
            h.ef_damage_tier = m.local_ef_tier;
            h.beaufort_scale = c.beaufort_scale;
            h.hydrometeor_phase = m.hydrometeor_phase;
        }
    }
};

} // namespace ideam::climatology
