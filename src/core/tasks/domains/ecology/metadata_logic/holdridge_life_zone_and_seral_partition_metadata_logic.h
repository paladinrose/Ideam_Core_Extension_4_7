#pragma once

/**
 * @file holdridge_life_zone_and_seral_partition_metadata_logic.h
 * @brief MetadataLogic task (ECO_M1: HoldridgeLifeZoneAndSeralPartition) for IdeamEcology.
 *
 * Classifies every cell into canonical Holdridge Life-Zones (`partition_ids`)
 * and assigns seral succession stages (`0..6`) into `lod_levels`.
 */

#include "../../../../ecology/ecology_core.h"
#include "../ecology_dod_common.h"
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
struct alignas(64) HoldridgeLifeZoneAndSeralPartitionMetadataLogic {
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

    static constexpr std::string_view display_name = "Holdridge Life-Zone & Seral Partition";
    static constexpr std::string_view ui_category  = "Ecology";

    uint32_t target_buffer_id          = INVALID_ID;
    uint32_t seed_profile_buffer_id    = INVALID_ID;
    uint32_t ecological_cell_buffer_id = INVALID_ID;
    uint32_t biogeo_trophic_buffer_id  = INVALID_ID;
    uint8_t  write_to_partition_ids    = 1;
    uint8_t  write_to_lod_levels       = 1;
    uint8_t  reserved_padding[46]      = {0};

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_prop = [&](const char *p_name, godot::Variant::Type p_type, godot::Variant p_def) {
            godot::Dictionary d;
            d["name"]    = p_name;
            d["type"]    = static_cast<int>(p_type);
            d["default"] = p_def;
            props.push_back(d);
        };
        add_prop("target_buffer_id",          godot::Variant::INT, static_cast<int64_t>(INVALID_ID));
        add_prop("seed_profile_buffer_id",    godot::Variant::INT, static_cast<int64_t>(INVALID_ID));
        add_prop("ecological_cell_buffer_id", godot::Variant::INT, static_cast<int64_t>(INVALID_ID));
        add_prop("biogeo_trophic_buffer_id",  godot::Variant::INT, static_cast<int64_t>(INVALID_ID));
        return props;
    }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id"))          target_buffer_id          = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("seed_profile_buffer_id"))    seed_profile_buffer_id    = static_cast<uint32_t>(static_cast<int64_t>(p_props["seed_profile_buffer_id"]));
        if (p_props.has("ecological_cell_buffer_id")) ecological_cell_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["ecological_cell_buffer_id"]));
        if (p_props.has("biogeo_trophic_buffer_id"))  biogeo_trophic_buffer_id  = static_cast<uint32_t>(static_cast<int64_t>(p_props["biogeo_trophic_buffer_id"]));
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        return (target_buffer_id != INVALID_ID) ? target_buffer_id : ecological_cell_buffer_id;
    }

    template <typename T_View>
    inline void configure_view(T_View &p_view, const TaskContextPOD &p_context) const noexcept {
        (void)p_view;
        (void)p_context;
    }

    template <typename T_View, typename T_Strategy>
    inline void execute(
        MemoryBufferSelectionPOD &r_selection,
        const TaskContextPOD &p_context,
        T_View &p_view
    ) const noexcept {
        execute_metadata<T_View, T_Strategy>(p_view, r_selection, p_context);
    }

    template <typename T_View, typename T_Strategy>
    inline void execute_metadata(
        T_View &p_view,
        MemoryBufferSelectionPOD &r_selection,
        const TaskContextPOD &p_context
    ) const noexcept {
        (void)p_view;
        const GrantPartPOD *seed_part = p_context.get_grant_part(seed_profile_buffer_id);
        const GrantPartPOD *eco_part  = p_context.get_grant_part(ecological_cell_buffer_id);
        const GrantPartPOD *bio_part  = p_context.get_grant_part(biogeo_trophic_buffer_id);

        if (!seed_part || !seed_part->raw_base_ptr || !eco_part || !eco_part->raw_base_ptr || !bio_part || !bio_part->raw_base_ptr) {
            return;
        }

        const auto *seed = reinterpret_cast<const ideam::core::EcologySeedProfilePOD *>(seed_part->raw_base_ptr);
        auto *eco        = reinterpret_cast<ideam::core::EcologicalCellStatePOD *>(eco_part->raw_base_ptr);
        auto *bio        = reinterpret_cast<ideam::core::BiogeochemicalAndTrophicCellPOD *>(bio_part->raw_base_ptr);

        const size_t n = static_cast<size_t>(seed->grid_width) * static_cast<size_t>(seed->grid_height);
        ideam::core::EcologyKernelMath::compute_holdridge_and_seral_partition(
            *seed,
            std::span<ideam::core::EcologicalCellStatePOD>(eco, n),
            std::span<ideam::core::BiogeochemicalAndTrophicCellPOD>(bio, n));

        const size_t sel_lim = static_cast<size_t>(std::max<int64_t>(0, r_selection.element_count));
        for (size_t i = 0; i < n && i < sel_lim; ++i) {
            if (write_to_partition_ids && r_selection.partition_ids) {
                r_selection.partition_ids[i] = static_cast<int32_t>(eco[i].life_zone);
            }
            if (write_to_lod_levels && r_selection.lod_levels) {
                r_selection.lod_levels[i] = static_cast<uint8_t>(eco[i].seral_stage);
            }
        }
    }
};

static_assert(sizeof(HoldridgeLifeZoneAndSeralPartitionMetadataLogic<>) == 64, "HoldridgeLifeZoneAndSeralPartitionMetadataLogic must be 64 bytes!");

} // namespace ideam::core
