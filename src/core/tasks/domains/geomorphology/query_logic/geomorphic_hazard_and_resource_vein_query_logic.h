#pragma once

/**
 * @file geomorphic_hazard_and_resource_vein_query_logic.h
 * @brief QueryLogic task (GEO_Q1: GeomorphicHazardAndResourceVein) for IdeamGeomorphology.
 *
 * Evaluates slope instability / landslide hazard, karst sinkhole collapse risk,
 * flash-flood stream-power hazard, hydrothermal ore veins along tectonic boundaries,
 * placer heavy-mineral deposits at river confluences, and endorheic evaporite brines.
 */

#include "../../../../geomorphology/geomorphology_core.h"
#include "../geomorphology_dod_common.h"
#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/memory_buffer_selection_pod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../query_logic/query_logic_traits.h"

#include <algorithm>
#include <cstdint>
#include <span>
#include <string_view>

namespace ideam::core {

template <typename T = int32_t>
struct alignas(64) GeomorphicHazardAndResourceVeinQueryLogic {
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

    static constexpr std::string_view display_name = "Geomorphic Hazard & Resource Vein Query";
    static constexpr std::string_view ui_category  = "Geomorphology";

    uint32_t target_buffer_id           = INVALID_ID;
    uint32_t seed_profile_buffer_id     = INVALID_ID;
    uint32_t crust_cell_buffer_id       = INVALID_ID;
    uint32_t strata_column_buffer_id    = INVALID_ID;
    uint32_t hydrology_state_buffer_id  = INVALID_ID;
    uint32_t hazard_sample_buffer_id    = INVALID_ID;
    float    hazard_threshold           = 0.35f;
    float    ore_grade_threshold        = 0.40f;
    bool     negate_result              = false;
    uint8_t  reserved_padding[31]       = {0};

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_prop = [&](const char *p_name, godot::Variant::Type p_type, godot::Variant p_def) {
            godot::Dictionary d;
            d["name"]    = p_name;
            d["type"]    = static_cast<int>(p_type);
            d["default"] = p_def;
            props.push_back(d);
        };
        add_prop("target_buffer_id",          godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("seed_profile_buffer_id",    godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("crust_cell_buffer_id",      godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("strata_column_buffer_id",   godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("hydrology_state_buffer_id", godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("hazard_sample_buffer_id",   godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("hazard_threshold",          godot::Variant::FLOAT, 0.35);
        add_prop("ore_grade_threshold",       godot::Variant::FLOAT, 0.40);
        add_prop("negate_result",             godot::Variant::BOOL,  false);
        return props;
    }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id"))          target_buffer_id          = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("seed_profile_buffer_id"))    seed_profile_buffer_id    = static_cast<uint32_t>(static_cast<int64_t>(p_props["seed_profile_buffer_id"]));
        if (p_props.has("crust_cell_buffer_id"))      crust_cell_buffer_id      = static_cast<uint32_t>(static_cast<int64_t>(p_props["crust_cell_buffer_id"]));
        if (p_props.has("strata_column_buffer_id"))   strata_column_buffer_id   = static_cast<uint32_t>(static_cast<int64_t>(p_props["strata_column_buffer_id"]));
        if (p_props.has("hydrology_state_buffer_id")) hydrology_state_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["hydrology_state_buffer_id"]));
        if (p_props.has("hazard_sample_buffer_id"))   hazard_sample_buffer_id   = static_cast<uint32_t>(static_cast<int64_t>(p_props["hazard_sample_buffer_id"]));
        if (p_props.has("hazard_threshold"))          hazard_threshold          = static_cast<float>(static_cast<double>(p_props["hazard_threshold"]));
        if (p_props.has("ore_grade_threshold"))       ore_grade_threshold       = static_cast<float>(static_cast<double>(p_props["ore_grade_threshold"]));
        if (p_props.has("negate_result"))             negate_result             = static_cast<bool>(p_props["negate_result"]);
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        return (target_buffer_id != INVALID_ID) ? target_buffer_id : hazard_sample_buffer_id;
    }

    template <typename T_View>
    inline void configure_view(T_View &p_view, const TaskContextPOD &p_context) const noexcept {
        (void)p_view;
        (void)p_context;
    }

    template <QueryOp Op, typename T_View, typename T_Strategy>
    inline void execute(
        MemoryBufferSelectionPOD &r_selection,
        const TaskContextPOD &p_context,
        const T_View &p_view
    ) const noexcept {
        const uint32_t tgt_id = get_target_buffer_id();
        const int64_t cap = r_selection.capacity;
        int64_t out_sparse = 0;
        for (int64_t i = 0; i < cap; ++i) {
            const bool match = evaluate_element<T_View, T_Strategy>(i, p_view, p_context);
            if constexpr (Op == QueryOp::ADD) {
                if (match) {
                    if (r_selection.mode == SelectionMode::DENSE && r_selection.data.bitset) {
                        r_selection.data.bitset[i >> 6] |= (1ULL << (i & 63));
                    } else if (r_selection.mode == SelectionMode::SPARSE && r_selection.data.indices && out_sparse < r_selection.capacity) {
                        r_selection.data.indices[out_sparse++] = i;
                    }
                    p_context.queue_selection_command(tgt_id, i);
                }
            } else {
                if (!match && r_selection.mode == SelectionMode::DENSE && r_selection.data.bitset) {
                    r_selection.data.bitset[i >> 6] &= ~(1ULL << (i & 63));
                } else if (match && r_selection.mode == SelectionMode::SPARSE && r_selection.data.indices && out_sparse < r_selection.capacity) {
                    r_selection.data.indices[out_sparse++] = i;
                }
            }
        }
        if (r_selection.mode == SelectionMode::SPARSE && r_selection.data.indices) {
            r_selection.element_count = out_sparse;
        }
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
        const auto *samples = reinterpret_cast<const ideam::core::GeomorphicHazardSamplePOD *>(haz_part->raw_base_ptr);
        const auto &s = samples[p_index];
        const float max_haz = std::max({s.landslide_hazard_probability, s.karst_sinkhole_hazard, s.flash_flood_hazard});
        const float max_ore = std::max({s.hydrothermal_ore_grade, s.placer_alluvial_grade, s.evaporite_brine_grade});
        const bool match = (max_haz >= hazard_threshold) || (max_ore >= ore_grade_threshold);
        return negate_result ? !match : match;
    }
};

static_assert(sizeof(GeomorphicHazardAndResourceVeinQueryLogic<>) == 64, "GeomorphicHazardAndResourceVeinQueryLogic must be 64 bytes!");

} // namespace ideam::core
