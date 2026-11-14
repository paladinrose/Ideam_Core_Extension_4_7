#pragma once

/**
 * quantum_darwinism_dod_common.h
 * Foundational pure C++26 DOD buffer slots, stage flags, capacities, and table-driven
 * manifest specifications for `IdeamQuantumDarwinism` (`src/core/tasks/domains/quantum_darwinism/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES (`IdeamQuantumDarwinism` DOD Substrate)
 * ============================================================================
 * Operates directly on the shared `ProceduralEnvironment` DOD buffers populated by
 * `IdeamProceduralGeneration` and `IdeamWaveFunctionCollapse` (slots `0..7` match
 * `ProceduralGenerationBufferSlot` and `WaveFunctionCollapseBufferSlot` 1:1 for
 * zero-copy buffer sharing):
 *   - Wave 1 (`QD_STAGE_WAVE_1_CONFLICT_HORIZON_BOX_PARTITION`):
 *       * `QD1` `ConflictHorizonBoxPartitionMetadataLogic`: Runs a zero-allocation
 *         bounded $k$-hop BFS around conflicted slots across parent, child, sibling,
 *         and spatial neighbor CSR links to isolate $\mathcal{E}_{\text{box}}$
 *         (`HORIZON_PARTITION_POD`).
 *   - Wave 2 (`QD_STAGE_WAVE_2_POINTER_STATE_EINSELECTION`):
 *       * `QD2` `PointerStateEinselectionQueryLogic`: Evaluates mutual-information
 *         witness redundancy ($R_\delta$) and CSS selector specificity across $\mathcal{E}_{\text{box}}$,
 *         promoting high-redundancy settled cells and perimeter anchors to immutable
 *         `PointerStateRecordPOD`s (`POINTER_STATE_RECORD_POD`).
 *   - Wave 3 (`QD_STAGE_WAVE_3_COHERENCE_CASCADE_RELAX_AND_PROPAGATE`):
 *       * `QD3` `CoherenceCascadeRelaxAndPropagateTransformLogic`: Traces bridge paths
 *         (`COHERENCE_BRIDGE_SLOT_POD`), locks high-precedence structural layers and
 *         nuclear semes while relaxing lower-precedence leaf/cardinality layers, and
 *         propagates Pointer State coherence inward to resolve the conflict!
 */

#include "../domain_dod_common.h"
#include "../../../quantum_darwinism/quantum_darwinism_solver.h"
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

enum QuantumDarwinismPipelineStageFlags : uint32_t {
    QD_STAGE_NONE                                             = 0u,
    QD_STAGE_WAVE_1_CONFLICT_HORIZON_BOX_PARTITION            = 1u << 0, ///< QD1: Bounded k-hop Horizon Box E_box BFS
    QD_STAGE_WAVE_2_POINTER_STATE_EINSELECTION                = 1u << 1, ///< QD2: R_delta Redundancy & Pointer State Selection
    QD_STAGE_WAVE_3_COHERENCE_CASCADE_RELAX_AND_PROPAGATE     = 1u << 2, ///< QD3: CSS Cascade Relaxation & Inward Propagation

    QD_STAGE_DEFAULT_PIPELINE_MASK =
        QD_STAGE_WAVE_1_CONFLICT_HORIZON_BOX_PARTITION |
        QD_STAGE_WAVE_2_POINTER_STATE_EINSELECTION |
        QD_STAGE_WAVE_3_COHERENCE_CASCADE_RELAX_AND_PROPAGATE,

    QD_STAGE_ALL_MASK = QD_STAGE_DEFAULT_PIPELINE_MASK
};

using QuantumDarwinismNodeExecutionConfig = DomainNodeExecutionConfig<QD_STAGE_DEFAULT_PIPELINE_MASK>;
using QuantumDarwinismStageAnchor         = DomainStageAnchor;
using QuantumDarwinismPipelineTopology    = DomainPipelineTopology<3>;

enum class QuantumDarwinismCapacityKind : uint8_t {
    MAX_TOKENS = 0,
    MAX_ENVIRONMENT_SLOTS,
    MAX_CASCADE_RULES,
    MAX_CHILD_CSR_EDGES,
    MAX_SPATIAL_CSR_EDGES,
    MAX_HORIZON_SLOTS,
    COUNT
};

enum class QuantumDarwinismTableGroup : uint8_t {
    TOKEN_AND_RULE_CATALOG = 0,
    ENVIRONMENT_SLOTS_AND_TOPOLOGY,
    QUANTUM_DARWINISM_AND_WFC_STATE,
    COUNT
};

struct QuantumDarwinismBufferCapacities {
    int64_t max_tokens            = 256;
    int64_t max_environment_slots = 4096;
    int64_t max_cascade_rules     = 512;
    int64_t max_child_csr_edges   = 16384;
    int64_t max_spatial_csr_edges = 32768;
    int64_t max_horizon_slots     = 512;

    [[nodiscard]] constexpr int64_t resolve(QuantumDarwinismCapacityKind p_kind) const noexcept {
        switch (p_kind) {
            case QuantumDarwinismCapacityKind::MAX_TOKENS:            return max_tokens;
            case QuantumDarwinismCapacityKind::MAX_ENVIRONMENT_SLOTS: return max_environment_slots;
            case QuantumDarwinismCapacityKind::MAX_CASCADE_RULES:     return max_cascade_rules;
            case QuantumDarwinismCapacityKind::MAX_CHILD_CSR_EDGES:   return max_child_csr_edges;
            case QuantumDarwinismCapacityKind::MAX_SPATIAL_CSR_EDGES: return max_spatial_csr_edges;
            case QuantumDarwinismCapacityKind::MAX_HORIZON_SLOTS:     return max_horizon_slots;
            default:                                                  return 256;
        }
    }
};

/**
 * QuantumDarwinismBufferSlot
 * Slots `0..7` align 1:1 with `ProceduralGenerationBufferSlot` and `WaveFunctionCollapseBufferSlot`
 * for zero-copy cross-domain buffer sharing.
 */
enum class QuantumDarwinismBufferSlot : uint8_t {
    TOKEN_DESCRIPTOR_POD      = 0,  ///< `TokenDescriptorPOD` (64B)
    ENVIRONMENT_SLOT_POD      = 1,  ///< `EnvironmentSlotPOD` (64B)
    CASCADE_RULE_POD          = 2,  ///< `GenerationRulePOD` (64B)
    CHILD_SLOT_CSR            = 3,  ///< `uint32_t` parent -> child slot ID CSR array
    POINTER_OR_CONFLICT_MASK  = 4,  ///< `uint32_t` `PointerOrConflictSlotFlags` per slot
    RULE_TOKEN_DOMAIN_MASKS   = 5,  ///< `BitmaskDomain256POD` (32B) per `GenerationRulePOD`
    SPATIAL_NEIGHBOR_CSR      = 6,  ///< `SpatialNeighborEdgePOD` (16B) CSR array
    SLOT_ENTROPY_FLOATS       = 7,  ///< `float` Shannon entropy per slot
    HORIZON_PARTITION_POD     = 8,  ///< `HorizonSlotPartitionPOD` (16B) per slot
    POINTER_STATE_RECORD_POD  = 9,  ///< `PointerStateRecordPOD` (32B) per slot
    COHERENCE_BRIDGE_SLOT_POD = 10, ///< `CoherenceBridgeSlotPOD` (32B) per slot
    COUNT                     = 11
};

struct QuantumDarwinismBufferSpec {
    QuantumDarwinismBufferSlot   slot;
    std::string_view             buffer_name;
    std::string_view             group_name;
    DataType                     data_type;
    size_t                       element_bytes;
    QuantumDarwinismCapacityKind capacity_kind;
    QuantumDarwinismTableGroup   table_group;
};

inline constexpr std::array<QuantumDarwinismBufferSpec, static_cast<size_t>(QuantumDarwinismBufferSlot::COUNT)>
QD_BUFFER_SPECS = {{
    { QuantumDarwinismBufferSlot::TOKEN_DESCRIPTOR_POD,      "QD_TokenDescriptorPOD",     "QD_Catalog",     DataType::BYTE,    sizeof(TokenDescriptorPOD),      QuantumDarwinismCapacityKind::MAX_TOKENS,            QuantumDarwinismTableGroup::TOKEN_AND_RULE_CATALOG },
    { QuantumDarwinismBufferSlot::ENVIRONMENT_SLOT_POD,      "QD_EnvironmentSlotPOD",     "QD_Environment", DataType::BYTE,    sizeof(EnvironmentSlotPOD),      QuantumDarwinismCapacityKind::MAX_ENVIRONMENT_SLOTS, QuantumDarwinismTableGroup::ENVIRONMENT_SLOTS_AND_TOPOLOGY },
    { QuantumDarwinismBufferSlot::CASCADE_RULE_POD,          "QD_CascadeRulePOD",         "QD_Catalog",     DataType::BYTE,    sizeof(GenerationRulePOD),       QuantumDarwinismCapacityKind::MAX_CASCADE_RULES,     QuantumDarwinismTableGroup::TOKEN_AND_RULE_CATALOG },
    { QuantumDarwinismBufferSlot::CHILD_SLOT_CSR,            "QD_ChildSlotCSR",           "QD_Environment", DataType::INT32,   sizeof(uint32_t),                QuantumDarwinismCapacityKind::MAX_CHILD_CSR_EDGES,   QuantumDarwinismTableGroup::ENVIRONMENT_SLOTS_AND_TOPOLOGY },
    { QuantumDarwinismBufferSlot::POINTER_OR_CONFLICT_MASK,  "QD_PointerOrConflictMask",  "QD_State",       DataType::INT32,   sizeof(uint32_t),                QuantumDarwinismCapacityKind::MAX_ENVIRONMENT_SLOTS, QuantumDarwinismTableGroup::QUANTUM_DARWINISM_AND_WFC_STATE },
    { QuantumDarwinismBufferSlot::RULE_TOKEN_DOMAIN_MASKS,   "QD_RuleTokenDomainMasks",   "QD_Catalog",     DataType::BYTE,    sizeof(BitmaskDomain256POD),     QuantumDarwinismCapacityKind::MAX_CASCADE_RULES,     QuantumDarwinismTableGroup::TOKEN_AND_RULE_CATALOG },
    { QuantumDarwinismBufferSlot::SPATIAL_NEIGHBOR_CSR,      "QD_SpatialNeighborCSR",     "QD_Environment", DataType::BYTE,    sizeof(SpatialNeighborEdgePOD),  QuantumDarwinismCapacityKind::MAX_SPATIAL_CSR_EDGES, QuantumDarwinismTableGroup::ENVIRONMENT_SLOTS_AND_TOPOLOGY },
    { QuantumDarwinismBufferSlot::SLOT_ENTROPY_FLOATS,       "QD_SlotEntropyFloats",      "QD_State",       DataType::FLOAT32, sizeof(float),                   QuantumDarwinismCapacityKind::MAX_ENVIRONMENT_SLOTS, QuantumDarwinismTableGroup::QUANTUM_DARWINISM_AND_WFC_STATE },
    { QuantumDarwinismBufferSlot::HORIZON_PARTITION_POD,     "QD_HorizonPartitionPOD",    "QD_State",       DataType::BYTE,    sizeof(HorizonSlotPartitionPOD), QuantumDarwinismCapacityKind::MAX_ENVIRONMENT_SLOTS, QuantumDarwinismTableGroup::QUANTUM_DARWINISM_AND_WFC_STATE },
    { QuantumDarwinismBufferSlot::POINTER_STATE_RECORD_POD,  "QD_PointerStateRecordPOD",  "QD_State",       DataType::BYTE,    sizeof(PointerStateRecordPOD),   QuantumDarwinismCapacityKind::MAX_HORIZON_SLOTS,     QuantumDarwinismTableGroup::QUANTUM_DARWINISM_AND_WFC_STATE },
    { QuantumDarwinismBufferSlot::COHERENCE_BRIDGE_SLOT_POD, "QD_CoherenceBridgeSlotPOD", "QD_State",       DataType::BYTE,    sizeof(CoherenceBridgeSlotPOD),  QuantumDarwinismCapacityKind::MAX_HORIZON_SLOTS,     QuantumDarwinismTableGroup::QUANTUM_DARWINISM_AND_WFC_STATE }
}};

consteval bool validate_qd_buffer_specs() {
    for (size_t i = 0; i < QD_BUFFER_SPECS.size(); ++i) {
        if (static_cast<size_t>(QD_BUFFER_SPECS[i].slot) != i) return false;
        if (QD_BUFFER_SPECS[i].element_bytes == 0) return false;
    }
    return true;
}
static_assert(validate_qd_buffer_specs(), "QD_BUFFER_SPECS must align 1:1 with QuantumDarwinismBufferSlot!");

using QuantumDarwinismBufferManifest = DomainBufferManifest<QuantumDarwinismBufferSlot>;

} // namespace ideam::core
