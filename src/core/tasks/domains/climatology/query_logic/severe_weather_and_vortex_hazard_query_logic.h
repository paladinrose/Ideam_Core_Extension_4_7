#pragma once

/**
 * @file severe_weather_and_vortex_hazard_query_logic.h
 * @brief QueryLogic task (CLM_Q1: SevereWeatherAndVortexHazard) for IdeamClimatology.
 *
 * Evaluates tornado wind gusts, cyclostrophic barometric suction drop, lofted
 * debris flux, blizzard whiteout index, drought aridity, and lightning probability.
 */

#include "../../../../climatology/climatology_core.h"
#include "../climatology_dod_common.h"
#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/memory_buffer_selection_pod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../query_logic/query_logic_traits.h"

#include <algorithm>
#include <cstdint>
#include <string_view>

namespace ideam::core {

template <typename T = int32_t>
struct alignas(64) SevereWeatherAndVortexHazardQueryLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::INT32 | DataType::FLOAT32 | DataType::BYTE;

    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr bool supports_cull               = true;
    static constexpr bool supports_addition           = true;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Severe Weather & Tornado Vortex Hazard Query";
    static constexpr std::string_view ui_category  = "Climatology";

    uint32_t target_buffer_id           = INVALID_ID;
    uint32_t hazard_sample_buffer_id    = INVALID_ID;
    float    wind_gust_threshold_mps    = 25.0f;
    float    lightning_prob_threshold   = 0.35f;
    float    blizzard_index_threshold   = 0.35f;
    bool     negate_result              = false;
    uint8_t  reserved_padding[43]       = {0};

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_prop = [&](const char *p_name, godot::Variant::Type p_type, godot::Variant p_def) {
            godot::Dictionary d;
            d["name"]    = p_name;
            d["type"]    = static_cast<int>(p_type);
            d["default"] = p_def;
            props.push_back(d);
        };
        add_prop("target_buffer_id",         godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("hazard_sample_buffer_id",  godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("wind_gust_threshold_mps",  godot::Variant::FLOAT, 25.0);
        add_prop("lightning_prob_threshold", godot::Variant::FLOAT, 0.35);
        add_prop("blizzard_index_threshold", godot::Variant::FLOAT, 0.35);
        add_prop("negate_result",            godot::Variant::BOOL,  false);
        return props;
    }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id"))         target_buffer_id         = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("hazard_sample_buffer_id"))  hazard_sample_buffer_id  = static_cast<uint32_t>(static_cast<int64_t>(p_props["hazard_sample_buffer_id"]));
        if (p_props.has("wind_gust_threshold_mps"))  wind_gust_threshold_mps  = static_cast<float>(static_cast<double>(p_props["wind_gust_threshold_mps"]));
        if (p_props.has("lightning_prob_threshold")) lightning_prob_threshold = static_cast<float>(static_cast<double>(p_props["lightning_prob_threshold"]));
        if (p_props.has("blizzard_index_threshold")) blizzard_index_threshold = static_cast<float>(static_cast<double>(p_props["blizzard_index_threshold"]));
        if (p_props.has("negate_result"))            negate_result            = static_cast<bool>(p_props["negate_result"]);
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        return (target_buffer_id != INVALID_ID) ? target_buffer_id : hazard_sample_buffer_id;
    }

    template <typename T_View>
    inline void configure_view(T_View &p_view, const TaskContextPOD &p_context) const noexcept {
        (void)p_view;
        (void)p_context;
    }

    template <typename T_View, typename T_Strategy>
    [[nodiscard]] inline bool evaluate_element(
        int64_t p_index,
        const T_View &p_view,
        const TaskContextPOD &p_context
    ) const noexcept {
        (void)p_view;
        const GrantPartPOD *haz_part = p_context.get_grant_part(hazard_sample_buffer_id);
        if (!haz_part || !haz_part->raw_base_ptr || p_index < 0 || p_index >= haz_part->selection.capacity) {
            return !negate_result;
        }
        const auto *samples = reinterpret_cast<const ideam::climatology::ClimatologyHazardAndBiomeSamplePOD *>(haz_part->raw_base_ptr);
        const auto &s = samples[p_index];
        const bool match = (s.tornado_wind_gust_mps >= wind_gust_threshold_mps) ||
                           (s.lightning_strike_prob >= lightning_prob_threshold) ||
                           (s.blizzard_whiteout_index >= blizzard_index_threshold);
        return negate_result ? !match : match;
    }
};

static_assert(sizeof(SevereWeatherAndVortexHazardQueryLogic<>) == 64, "SevereWeatherAndVortexHazardQueryLogic must be 64 bytes!");

} // namespace ideam::core
