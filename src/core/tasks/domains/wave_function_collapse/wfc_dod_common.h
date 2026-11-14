#pragma once

/**
 * wfc_dod_common.h
 * Foundational pure C++26 DOD buffer slots, stage flags, capacities, and table-driven
 * manifest specifications for `IdeamWaveFunctionCollapse` (`src/core/tasks/domains/wave_function_collapse/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES (`IdeamWaveFunctionCollapse` DOD Substrate)
 * ============================================================================
 * Operates directly on the shared `ProceduralEnvironment` DOD buffers populated by
 * `IdeamProceduralGeneration` (indices `0..7` match `ProceduralGenerationBufferSlot` 1:1
 * for zero-copy buffer sharing) while adding dedicated rule-tensor and contradiction-report
 * tables:
 *   - Wave 1 (`WFC_STAGE_WAVE_1_ENTROPY_PARTITION`):
 *       * `WFC1` `WFCEntropyPartitionMetadataLogic`: Computes frequency-weighted Shannon / MRV
 *         / hierarchy-depth entropy over `ENVIRONMENT_SLOT_POD` (`EnvironmentSlotPOD`), writes
 *         quantized buckets `[0..15]` into `r_selection.lod_levels`, and packs subtree group
 *         masks into `r_selection.group_masks` for lock-free parallel subtree collapse.
 *   - Wave 2 (`WFC_STAGE_WAVE_2_FORWARD_COLLAPSE_AND_PROPAGATE`):
 *       * `WFC2` `WFCForwardCollapseAndPropagateTransformLogic`: Collapses lowest-entropy
 *         slots and propagates both directional port adjacency (`WFC_RULE_TENSOR_POD` +
 *         `SPATIAL_NEIGHBOR_CSR`) and cascading `GenerationRulePOD` hierarchy/cardinality/sibling
 *         rules (`ACCEPT_PARENTS`, `ACCEPT_CHILDREN_CARDINALITY` `[min..max]`, `REQUIRE_SIBLING`,
 *         `FORBID_SIBLING`).
 *   - Wave 3 (`WFC_STAGE_WAVE_3_CONTRADICTION_DETECT`):
 *       * `WFC3` `WFCContradictionDetectQueryLogic`: Audits all slots and parent hierarchies
 *         for empty domains, cardinality deficits/excesses, or missing required siblings,
 *         populating `WFC_CONTRADICTION_REPORT_POD` and flagging `POINTER_OR_CONFLICT_MASK`
 *         for immediate handoff to `IdeamQuantumDarwinism`.
 */

#include "../domain_dod_common.h"
#include "../../../wave_function_collapse/wfc_solver_core.h"
#include "../../../memory/memory_common.h"
#include "../../../memory/memory_buffer_pod.h"
#include "../../../memory/memory_grant_pod.h"
#include "../../../memory/memory_manager_dod.h"
#include "../../../graphs/ideam_graph_dod.h"
#include "../../registration/ideam_task_registry.h"
#include <array>
#include <cstdint>
#include <string_view>

namespace ideam::core {

enum WaveFunctionCollapsePipelineStageFlags : uint32_t {
    WFC_STAGE_NONE                                   = 0u,
    WFC_STAGE_WAVE_1_ENTROPY_PARTITION               = 1u << 0, ///< WFC1: Shannon / MRV / Hierarchy Entropy Partition
    WFC_STAGE_WAVE_2_FORWARD_COLLAPSE_AND_PROPAGATE  = 1u << 1, ///< WFC2: Forward Collapse & Multi-Axis AC-3 Propagation
    WFC_STAGE_WAVE_3_CONTRADICTION_DETECT            = 1u << 2, ///< WFC3: Contradiction & Cardinality Conflict Audit

    WFC_STAGE_DEFAULT_PIPELINE_MASK =
        WFC_STAGE_WAVE_1_ENTROPY_PARTITION |
        WFC_STAGE_WAVE_2_FORWARD_COLLAPSE_AND_PROPAGATE |
        WFC_STAGE_WAVE_3_CONTRADICTION_DETECT,

    WFC_STAGE_ALL_MASK = WFC_STAGE_DEFAULT_PIPELINE_MASK
};

using WaveFunctionCollapseNodeExecutionConfig = DomainNodeExecutionConfig<WFC_STAGE_DEFAULT_PIPELINE_MASK>;
using WaveFunctionCollapseStageAnchor         = DomainStageAnchor;
using WaveFunctionCollapsePipelineTopology    = DomainPipelineTopology<3>;

enum class WaveFunctionCollapseCapacityKind : uint8_t {
    MAX_TOKENS = 0,
    MAX_ENVIRONMENT_SLOTS,
    MAX_CASCADE_RULES,
    MAX_CHILD_CSR_EDGES,
    MAX_SPATIAL_CSR_EDGES,
    MAX_WFC_RULE_ENTRIES,
    COUNT
};

enum class WaveFunctionCollapseTableGroup : uint8_t {
    TOKEN_AND_RULE_CATALOG = 0,
    ENVIRONMENT_SLOTS_AND_TOPOLOGY,
    QUANTUM_DARWINISM_AND_WFC_STATE,
    COUNT
};

struct WaveFunctionCollapseBufferCapacities {
    int64_t max_tokens            = 256;
    int64_t max_environment_slots = 4096;
    int64_t max_cascade_rules     = 512;
    int64_t max_child_csr_edges   = 16384;
    int64_t max_spatial_csr_edges = 32768;
    int64_t max_wfc_rule_entries  = 2048;

    [[nodiscard]] constexpr int64_t resolve(WaveFunctionCollapseCapacityKind p_kind) const noexcept {
        switch (p_kind) {
            case WaveFunctionCollapseCapacityKind::MAX_TOKENS:            return max_tokens;
            case WaveFunctionCollapseCapacityKind::MAX_ENVIRONMENT_SLOTS: return max_environment_slots;
            case WaveFunctionCollapseCapacityKind::MAX_CASCADE_RULES:     return max_cascade_rules;
            case WaveFunctionCollapseCapacityKind::MAX_CHILD_CSR_EDGES:   return max_child_csr_edges;
            case WaveFunctionCollapseCapacityKind::MAX_SPATIAL_CSR_EDGES: return max_spatial_csr_edges;
            case WaveFunctionCollapseCapacityKind::MAX_WFC_RULE_ENTRIES:  return max_wfc_rule_entries;
            default:                                                      return 256;
        }
    }
};

/**
 * WaveFunctionCollapseBufferSlot
 * Slots `0..7` align 1:1 with `ProceduralGenerationBufferSlot` so `WaveFunctionCollapseDOD`
 * can bind directly to existing `ProceduralGenerationDOD` buffers without copying.
 */
enum class WaveFunctionCollapseBufferSlot : uint8_t {
    TOKEN_DESCRIPTOR_POD         = 0, ///< `TokenDescriptorPOD` (64B)
    ENVIRONMENT_SLOT_POD         = 1, ///< `EnvironmentSlotPOD` (64B)
    CASCADE_RULE_POD             = 2, ///< `GenerationRulePOD` (64B)
    CHILD_SLOT_CSR               = 3, ///< `uint32_t` parent -> child slot ID CSR array
    POINTER_OR_CONFLICT_MASK     = 4, ///< `uint32_t` `PointerOrConflictSlotFlags` per slot
    RULE_TOKEN_DOMAIN_MASKS      = 5, ///< `BitmaskDomain256POD` (32B) per `GenerationRulePOD`
    SPATIAL_NEIGHBOR_CSR         = 6, ///< `SpatialNeighborEdgePOD` (16B) CSR array
    SLOT_ENTROPY_FLOATS          = 7, ///< `float` Shannon entropy per slot
    WFC_RULE_TENSOR_POD          = 8, ///< `WFCRuleEntryPOD` (48B) directional port/socket rules
    WFC_CONTRADICTION_REPORT_POD = 9, ///< `WFCContradictionReportPOD` (32B) per slot
    COUNT                        = 10
};

struct WaveFunctionCollapseBufferSpec {
    WaveFunctionCollapseBufferSlot   slot;
    std::string_view                 buffer_name;
    std::string_view                 group_name;
    DataType                         data_type;
    size_t                           element_bytes;
    WaveFunctionCollapseCapacityKind capacity_kind;
    WaveFunctionCollapseTableGroup   table_group;
};

inline constexpr std::array<WaveFunctionCollapseBufferSpec, static_cast<size_t>(WaveFunctionCollapseBufferSlot::COUNT)>
WFC_BUFFER_SPECS = {{
    { WaveFunctionCollapseBufferSlot::TOKEN_DESCRIPTOR_POD,         "WFC_TokenDescriptorPOD",        "WFC_Catalog",     DataType::BYTE,    sizeof(TokenDescriptorPOD),        WaveFunctionCollapseCapacityKind::MAX_TOKENS,            WaveFunctionCollapseTableGroup::TOKEN_AND_RULE_CATALOG },
    { WaveFunctionCollapseBufferSlot::ENVIRONMENT_SLOT_POD,         "WFC_EnvironmentSlotPOD",        "WFC_Environment", DataType::BYTE,    sizeof(EnvironmentSlotPOD),        WaveFunctionCollapseCapacityKind::MAX_ENVIRONMENT_SLOTS, WaveFunctionCollapseTableGroup::ENVIRONMENT_SLOTS_AND_TOPOLOGY },
    { WaveFunctionCollapseBufferSlot::CASCADE_RULE_POD,             "WFC_CascadeRulePOD",            "WFC_Catalog",     DataType::BYTE,    sizeof(GenerationRulePOD),         WaveFunctionCollapseCapacityKind::MAX_CASCADE_RULES,     WaveFunctionCollapseTableGroup::TOKEN_AND_RULE_CATALOG },
    { WaveFunctionCollapseBufferSlot::CHILD_SLOT_CSR,               "WFC_ChildSlotCSR",              "WFC_Environment", DataType::INT32,   sizeof(uint32_t),                  WaveFunctionCollapseCapacityKind::MAX_CHILD_CSR_EDGES,   WaveFunctionCollapseTableGroup::ENVIRONMENT_SLOTS_AND_TOPOLOGY },
    { WaveFunctionCollapseBufferSlot::POINTER_OR_CONFLICT_MASK,     "WFC_PointerOrConflictMask",     "WFC_QDState",     DataType::INT32,   sizeof(uint32_t),                  WaveFunctionCollapseCapacityKind::MAX_ENVIRONMENT_SLOTS, WaveFunctionCollapseTableGroup::QUANTUM_DARWINISM_AND_WFC_STATE },
    { WaveFunctionCollapseBufferSlot::RULE_TOKEN_DOMAIN_MASKS,      "WFC_RuleTokenDomainMasks",      "WFC_Catalog",     DataType::BYTE,    sizeof(BitmaskDomain256POD),       WaveFunctionCollapseCapacityKind::MAX_CASCADE_RULES,     WaveFunctionCollapseTableGroup::TOKEN_AND_RULE_CATALOG },
    { WaveFunctionCollapseBufferSlot::SPATIAL_NEIGHBOR_CSR,         "WFC_SpatialNeighborCSR",        "WFC_Environment", DataType::BYTE,    sizeof(SpatialNeighborEdgePOD),    WaveFunctionCollapseCapacityKind::MAX_SPATIAL_CSR_EDGES, WaveFunctionCollapseTableGroup::ENVIRONMENT_SLOTS_AND_TOPOLOGY },
    { WaveFunctionCollapseBufferSlot::SLOT_ENTROPY_FLOATS,          "WFC_SlotEntropyFloats",         "WFC_QDState",     DataType::FLOAT32, sizeof(float),                     WaveFunctionCollapseCapacityKind::MAX_ENVIRONMENT_SLOTS, WaveFunctionCollapseTableGroup::QUANTUM_DARWINISM_AND_WFC_STATE },
    { WaveFunctionCollapseBufferSlot::WFC_RULE_TENSOR_POD,          "WFC_RuleTensorPOD",             "WFC_Catalog",     DataType::BYTE,    sizeof(WFCRuleEntryPOD),           WaveFunctionCollapseCapacityKind::MAX_WFC_RULE_ENTRIES,  WaveFunctionCollapseTableGroup::TOKEN_AND_RULE_CATALOG },
    { WaveFunctionCollapseBufferSlot::WFC_CONTRADICTION_REPORT_POD, "WFC_ContradictionReportPOD",    "WFC_QDState",     DataType::BYTE,    sizeof(WFCContradictionReportPOD), WaveFunctionCollapseCapacityKind::MAX_ENVIRONMENT_SLOTS, WaveFunctionCollapseTableGroup::QUANTUM_DARWINISM_AND_WFC_STATE }
}};

consteval bool validate_wfc_buffer_specs() {
    for (size_t i = 0; i < WFC_BUFFER_SPECS.size(); ++i) {
        if (static_cast<size_t>(WFC_BUFFER_SPECS[i].slot) != i) return false;
        if (WFC_BUFFER_SPECS[i].element_bytes == 0) return false;
    }
    return true;
}
static_assert(validate_wfc_buffer_specs(), "WFC_BUFFER_SPECS must align 1:1 with WaveFunctionCollapseBufferSlot!");

using WaveFunctionCollapseBufferManifest = DomainBufferManifest<WaveFunctionCollapseBufferSlot>;

} // namespace ideam::core
