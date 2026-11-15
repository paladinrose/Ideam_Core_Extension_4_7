#pragma once

/**
 * @file drainage_basin_and_strahler_partition_metadata_logic.h
 * @brief MetadataLogic task (GEO_M1: DrainageBasinAndStrahlerPartition) for IdeamGeomorphology.
 *
 * Computes D8 steepest-descent flow routing, flood-fills contiguous watershed
 * drainage basins (endorheic vs. exorheic), and assigns Strahler stream order
 * (1..8) as LOD tiers alongside bitmask flags for ridge divides, valley thalwegs,
 * and coastal deltas.
 */

#include "../../../../geomorphology/geomorphology_core.h"
#include "../geomorphology_dod_common.h"
#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/memory_buffer_selection_pod.h"
#include "../../../../memory/views/stencil_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../metadata_logic/metadata_logic_traits.h"

#include <algorithm>
#include <cstdint>
#include <span>
#include <string_view>

namespace ideam::core {

template <typename T = int32_t>
struct alignas(64) DrainageBasinAndStrahlerPartitionMetadataLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = StencilView<T, DefaultStrategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR | BufferLayoutType::ANY_SPATIAL;
    static constexpr DataType required_types              = DataType::INT32 | DataType::FLOAT32;

    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Drainage Basin & Strahler Partition";
    static constexpr std::string_view ui_category  = "Geomorphology";

    uint32_t target_buffer_id           = INVALID_ID;
    uint32_t seed_profile_buffer_id     = INVALID_ID;
    uint32_t strata_column_buffer_id    = INVALID_ID;
    uint32_t hydrology_state_buffer_id  = INVALID_ID;
    float    thalweg_discharge_min_m3s  = 0.85f;
    float    ridge_divide_slope_min     = 0.25f;
    uint8_t  write_to_partition_ids     = 1;
    uint8_t  write_to_lod_levels        = 1;
    uint8_t  reserved_padding[38]       = {0};

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
        add_prop("strata_column_buffer_id",   godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("hydrology_state_buffer_id", godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("thalweg_discharge_min_m3s", godot::Variant::FLOAT, 0.85);
        add_prop("ridge_divide_slope_min",    godot::Variant::FLOAT, 0.25);
        return props;
    }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id"))          target_buffer_id          = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("seed_profile_buffer_id"))    seed_profile_buffer_id    = static_cast<uint32_t>(static_cast<int64_t>(p_props["seed_profile_buffer_id"]));
        if (p_props.has("strata_column_buffer_id"))   strata_column_buffer_id   = static_cast<uint32_t>(static_cast<int64_t>(p_props["strata_column_buffer_id"]));
        if (p_props.has("hydrology_state_buffer_id")) hydrology_state_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["hydrology_state_buffer_id"]));
        if (p_props.has("thalweg_discharge_min_m3s")) thalweg_discharge_min_m3s = static_cast<float>(static_cast<double>(p_props["thalweg_discharge_min_m3s"]));
        if (p_props.has("ridge_divide_slope_min"))    ridge_divide_slope_min    = static_cast<float>(static_cast<double>(p_props["ridge_divide_slope_min"]));
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        return (target_buffer_id != INVALID_ID) ? target_buffer_id : hydrology_state_buffer_id;
    }

    template <typename T_View>
    inline void configure_view(T_View &p_view, const TaskContextPOD &p_context) const noexcept {
        (void)p_view;
        (void)p_context;
    }

    template <typename T_View, typename T_Strategy>
    inline void execute_metadata(
        T_View &p_view,
        MemoryBufferSelectionPOD &r_selection,
        const TaskContextPOD &p_context
    ) const noexcept {
        (void)p_view;
        const GrantPartPOD *seed_part   = p_context.get_grant_part(seed_profile_buffer_id);
        const GrantPartPOD *strata_part = p_context.get_grant_part(strata_column_buffer_id);
        const GrantPartPOD *hydro_part  = p_context.get_grant_part(hydrology_state_buffer_id);

        if (!seed_part || !seed_part->raw_base_ptr || !strata_part || !strata_part->raw_base_ptr || !hydro_part || !hydro_part->raw_base_ptr) {
            return;
        }

        const auto *seed   = reinterpret_cast<const ideam::geomorphology::GeomorphologySeedProfilePOD *>(seed_part->raw_base_ptr);
        const auto *strata = reinterpret_cast<const ideam::geomorphology::StratigraphicColumnPOD *>(strata_part->raw_base_ptr);
        auto *hydro        = reinterpret_cast<ideam::geomorphology::HydrologyCellStatePOD *>(hydro_part->raw_base_ptr);

        const size_t n = static_cast<size_t>(seed->grid_width) * static_cast<size_t>(seed->grid_height);
        ideam::geomorphology::GeomorphologyKernelMath::compute_d8_drainage_and_strahler(
            *seed,
            std::span<const ideam::geomorphology::StratigraphicColumnPOD>(strata, n),
            std::span<ideam::geomorphology::HydrologyCellStatePOD>(hydro, n));

        const size_t sel_lim = static_cast<size_t>(std::max<int64_t>(0, r_selection.element_count));
        for (size_t i = 0; i < n && i < sel_lim; ++i) {
            if (write_to_partition_ids && r_selection.partition_ids) {
                r_selection.partition_ids[i] = static_cast<int32_t>(hydro[i].drainage_basin_id);
            }
            if (write_to_lod_levels && r_selection.lod_levels) {
                r_selection.lod_levels[i] = hydro[i].strahler_order;
            }
        }
    }
};

static_assert(sizeof(DrainageBasinAndStrahlerPartitionMetadataLogic<>) == 64, "DrainageBasinAndStrahlerPartitionMetadataLogic must be 64 bytes!");

} // namespace ideam::core
