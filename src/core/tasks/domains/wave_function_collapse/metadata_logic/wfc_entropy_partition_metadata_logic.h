#pragma once

/**
 * wfc_entropy_partition_metadata_logic.h
 * 64-byte cache-aligned MetadataLogic task struct for `IdeamWaveFunctionCollapse`.
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES
 * ============================================================================
 * - Operates on the shared `ProceduralEnvironment` populated by `IdeamProceduralGeneration`.
 * - Runs minimum-entropy slot collapse and propagates both spatial port adjacency and
 *   cascading `GenerationRule` hierarchy constraints (`CastleGrounds -> CastleTower`,
 *   child cardinality `[min..max]`, and sibling dependencies like `TowerLanding requires TowerStairs`).
 * - Flags any contradiction slots in `POINTER_OR_CONFLICT_MASK` so `IdeamQuantumDarwinism`
 *   TaskGraph nodes can stabilize the local environment horizon box without full-grid backtracking.
 */

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_grant_pod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../metadata_logic/metadata_logic_traits.h"
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <cstdint>
#include <string_view>

namespace ideam::core {

template <
    typename T = uint32_t,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) WFCEntropyPartitionMetadataLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts =
        BufferLayoutType::FLAT | BufferLayoutType::AOS | BufferLayoutType::SOA;
    static constexpr DataType required_types =
        DataType::INT32 | DataType::UINT32 | DataType::FLOAT32;

    static constexpr size_t dimensions             = 0;
    static constexpr bool requires_static_kernel   = false;
    static constexpr size_t kernel_size            = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "WFC Shannon & Bitmask Entropy Partition";
    static constexpr std::string_view ui_category  = "WaveFunctionCollapse";

    uint32_t target_buffer_id    = INVALID_ID;
    uint32_t metadata_buffer_id  = INVALID_ID;
    uint32_t max_horizon_radius  = 4;
    uint8_t  reserved_padding[52]= {0};

    static godot::Array get_ui_properties() { return godot::Array(); }
    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept { return target_buffer_id; }
    void apply_properties(const godot::Dictionary &p_props) noexcept { (void)p_props; }

    template <typename T_ActiveView>
    inline void configure_view(T_ActiveView &p_view, const TaskContextPOD &p_ctx) const noexcept {
        (void)p_view; (void)p_ctx;
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    void execute(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        (void)p_ctx; (void)p_view;
        // Architectural stub: Full DOD metadata partitioning / specificity sorting.
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    inline void execute_metadata(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        execute<T_ActiveView, T_ActiveStrategy>(p_ctx, p_view);
    }
};

static_assert(sizeof(WFCEntropyPartitionMetadataLogic<>) == 64, "WFCEntropyPartitionMetadataLogic must be 64 bytes!");
static_assert(IsMetadataLogic<WFCEntropyPartitionMetadataLogic<>>, "WFCEntropyPartitionMetadataLogic must satisfy IsMetadataLogic!");

} // namespace ideam::core
