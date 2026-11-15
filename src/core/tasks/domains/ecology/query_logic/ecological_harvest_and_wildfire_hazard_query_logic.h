#pragma once

/**
 * @file ecological_harvest_and_wildfire_hazard_query_logic.h
 * @brief QueryLogic task (ECO_Q1: EcologicalHarvestAndWildfireHazard) for IdeamEcology.
 *
 * Filters cells by wildfire spread risk, harvestable timber/forage yield,
 * or high-potency alchemical botanical reagents.
 */

#include "../../../../ecology/ecology_core.h"
#include "../ecology_dod_common.h"
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
struct alignas(64) EcologicalHarvestAndWildfireHazardQueryLogic {
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

    static constexpr std::string_view display_name = "Ecological Harvest & Wildfire Hazard Query";
    static constexpr std::string_view ui_category  = "Ecology";

    uint32_t target_buffer_id           = INVALID_ID;
    uint32_t harvest_sample_buffer_id   = INVALID_ID;
    float    wildfire_risk_threshold    = 0.45f;
    float    alchemical_grade_threshold = 0.55f;
    float    timber_yield_threshold     = 15.0f;
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
        add_prop("target_buffer_id",           godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("harvest_sample_buffer_id",   godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("wildfire_risk_threshold",    godot::Variant::FLOAT, 0.45);
        add_prop("alchemical_grade_threshold", godot::Variant::FLOAT, 0.55);
        add_prop("timber_yield_threshold",     godot::Variant::FLOAT, 15.0);
        add_prop("negate_result",              godot::Variant::BOOL,  false);
        return props;
    }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id"))           target_buffer_id           = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("harvest_sample_buffer_id"))   harvest_sample_buffer_id   = static_cast<uint32_t>(static_cast<int64_t>(p_props["harvest_sample_buffer_id"]));
        if (p_props.has("wildfire_risk_threshold"))    wildfire_risk_threshold    = static_cast<float>(static_cast<double>(p_props["wildfire_risk_threshold"]));
        if (p_props.has("alchemical_grade_threshold")) alchemical_grade_threshold = static_cast<float>(static_cast<double>(p_props["alchemical_grade_threshold"]));
        if (p_props.has("timber_yield_threshold"))     timber_yield_threshold     = static_cast<float>(static_cast<double>(p_props["timber_yield_threshold"]));
        if (p_props.has("negate_result"))              negate_result              = static_cast<bool>(p_props["negate_result"]);
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        return (target_buffer_id != INVALID_ID) ? target_buffer_id : harvest_sample_buffer_id;
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
        const GrantPartPOD *harv_part = p_context.get_grant_part(harvest_sample_buffer_id);
        if (!harv_part || !harv_part->raw_base_ptr || p_index < 0 || p_index >= harv_part->selection.capacity) {
            return !negate_result;
        }
        const auto *samples = reinterpret_cast<const ideam::ecology::EcologyHarvestAndDisturbanceSamplePOD *>(harv_part->raw_base_ptr);
        const auto &s = samples[p_index];
        const bool match = (s.wildfire_spread_risk_01 >= wildfire_risk_threshold) ||
                           (s.alchemical_botanical_grade >= alchemical_grade_threshold) ||
                           (s.timber_canopy_yield_kg_m2 >= timber_yield_threshold);
        return negate_result ? !match : match;
    }
};

static_assert(sizeof(EcologicalHarvestAndWildfireHazardQueryLogic<>) == 64, "EcologicalHarvestAndWildfireHazardQueryLogic must be 64 bytes!");

} // namespace ideam::core
