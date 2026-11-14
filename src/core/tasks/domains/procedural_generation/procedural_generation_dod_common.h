#pragma once

/**
 * procedural_generation_dod_common.h
 * Foundational pure C++26 DOD buffer slots, stage flags, capacities, and table-driven
 * manifest specifications for `IdeamProceduralGeneration` (`src/core/tasks/domains/procedural_generation/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES (`IdeamProceduralGeneration` DOD Substrate)
 * ============================================================================
 * Bridges `AnalysisRule`, `GenerationRule`, `ProceduralPattern`, and `ProceduralPatterner`
 * into the shared `ProceduralEnvironment` DOD tables consumed by `IdeamWaveFunctionCollapse`
 * and `IdeamQuantumDarwinism`:
 *   - Wave 1 (`PROCGEN_STAGE_WAVE_1_CASCADE_RULE_COMPILE`):
 *       * `PCG1` `CascadeRuleCompileMetadataLogic`: Computes CSS selector specificity
 *         (`!important > PatternCascadeLayer > priority > specificity`), sorts
 *         `GenerationRulePOD`s in cascade order, binds `TokenDescriptorPOD::coherence_trait_mask`,
 *         and stamps `PatternCascadeLayer` into `r_selection.lod_levels`.
 *   - Wave 2 (`PROCGEN_STAGE_WAVE_2_REFLECTION_SELECTOR_MATCH`):
 *       * `PCG2` `ReflectionSelectorMatchQueryLogic`: Evaluates `ReflectionSignaturePOD`
 *         selectors over `TokenDescriptorPOD` buffers, populating `r_selection.data.indices`
 *         and `RULE_TOKEN_DOMAIN_MASKS` (`BitmaskDomain256POD`).
 *   - Wave 3 (`PROCGEN_STAGE_WAVE_3_PATTERN_DOMAIN_INITIALIZE`):
 *       * `PCG3` `PatternDomainInitializeTransformLogic`: Initializes each
 *         `EnvironmentSlotPOD::domain` from `ADMIT_ARCHETYPE` rules and pinned overrides,
 *         populating `SLOT_ENTROPY_FLOATS` and clearing `POINTER_OR_CONFLICT_MASK`.
 *   - Wave 4 (`PROCGEN_STAGE_WAVE_4_HIERARCHY_CARDINALITY_PROPAGATE`):
 *       * `PCG4` `HierarchyCardinalityPropagateTransformLogic`: Enforces `ACCEPT_PARENTS`,
 *         `ACCEPT_CHILDREN_CARDINALITY` (`[min..max]`), `REQUIRE_SIBLING`, and `FORBID_SIBLING`
 *         across `ENVIRONMENT_SLOT_POD` and `CHILD_SLOT_CSR`, computing Quantum Darwinism
 *         Einselection Redundancy (`einselection_score`, $R_\delta$) and flagging
 *         `POINTER_OR_CONFLICT_MASK` (`bit 0 = is_pointer_state`, `bit 1 = has_conflict`)!
 */

#include "../domain_dod_common.h"
#include "../../../procedural_generation/constraint_graph_solver.h"
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

enum ProceduralGenerationPipelineStageFlags : uint32_t {
    PROCGEN_STAGE_NONE                                    = 0u,
    PROCGEN_STAGE_WAVE_1_CASCADE_RULE_COMPILE             = 1u << 0, ///< PCG1: CSS Cascade Specificity & Coherence Trait Compile
    PROCGEN_STAGE_WAVE_2_REFLECTION_SELECTOR_MATCH        = 1u << 1, ///< PCG2: ReflectionSignaturePOD Selector Query
    PROCGEN_STAGE_WAVE_3_PATTERN_DOMAIN_INITIALIZE        = 1u << 2, ///< PCG3: EnvironmentSlotPOD BitmaskDomain256POD Initialize
    PROCGEN_STAGE_WAVE_4_HIERARCHY_CARDINALITY_PROPAGATE  = 1u << 3, ///< PCG4: Parent/Child/Sibling Propagation & QD R_delta Score

    PROCGEN_STAGE_DEFAULT_PIPELINE_MASK =
        PROCGEN_STAGE_WAVE_1_CASCADE_RULE_COMPILE |
        PROCGEN_STAGE_WAVE_2_REFLECTION_SELECTOR_MATCH |
        PROCGEN_STAGE_WAVE_3_PATTERN_DOMAIN_INITIALIZE |
        PROCGEN_STAGE_WAVE_4_HIERARCHY_CARDINALITY_PROPAGATE,

    PROCGEN_STAGE_ALL_MASK = PROCGEN_STAGE_DEFAULT_PIPELINE_MASK
};

/**
 * Bit flags written into `POINTER_OR_CONFLICT_MASK` (`uint32_t` per slot) for
 * zero-copy handoff to `IdeamWaveFunctionCollapse` and `IdeamQuantumDarwinism`.
 */
// PointerOrConflictSlotFlags defined in procedural_environment.h


using ProceduralGenerationNodeExecutionConfig = DomainNodeExecutionConfig<PROCGEN_STAGE_DEFAULT_PIPELINE_MASK>;
using ProceduralGenerationStageAnchor         = DomainStageAnchor;
using ProceduralGenerationPipelineTopology    = DomainPipelineTopology<4>;

enum class ProceduralGenerationCapacityKind : uint8_t {
    MAX_TOKENS = 0,
    MAX_ENVIRONMENT_SLOTS,
    MAX_CASCADE_RULES,
    MAX_CHILD_CSR_EDGES,
    MAX_SPATIAL_CSR_EDGES,
    COUNT
};

enum class ProceduralGenerationTableGroup : uint8_t {
    TOKEN_AND_RULE_CATALOG = 0,
    ENVIRONMENT_SLOTS_AND_TOPOLOGY,
    QUANTUM_DARWINISM_AND_WFC_STATE,
    COUNT
};

struct ProceduralGenerationBufferCapacities {
    int64_t max_tokens            = 256;
    int64_t max_environment_slots = 4096;
    int64_t max_cascade_rules     = 512;
    int64_t max_child_csr_edges   = 16384;
    int64_t max_spatial_csr_edges = 32768;

    [[nodiscard]] constexpr int64_t resolve(ProceduralGenerationCapacityKind p_kind) const noexcept {
        switch (p_kind) {
            case ProceduralGenerationCapacityKind::MAX_TOKENS:            return max_tokens;
            case ProceduralGenerationCapacityKind::MAX_ENVIRONMENT_SLOTS: return max_environment_slots;
            case ProceduralGenerationCapacityKind::MAX_CASCADE_RULES:     return max_cascade_rules;
            case ProceduralGenerationCapacityKind::MAX_CHILD_CSR_EDGES:   return max_child_csr_edges;
            case ProceduralGenerationCapacityKind::MAX_SPATIAL_CSR_EDGES: return max_spatial_csr_edges;
            default:                                                      return 256;
        }
    }
};

/**
 * ProceduralGenerationBufferSlot
 * Preserves exact legacy scaffold enum names (`TOKEN_DESCRIPTOR_POD`..`POINTER_OR_CONFLICT_MASK`)
 * while adding SOA and CSR tables for WFC and Quantum Darwinism interoperability.
 */
enum class ProceduralGenerationBufferSlot : uint8_t {
    TOKEN_DESCRIPTOR_POD     = 0, ///< `TokenDescriptorPOD` (64B)
    ENVIRONMENT_SLOT_POD     = 1, ///< `EnvironmentSlotPOD` (64B)
    CASCADE_RULE_POD         = 2, ///< `GenerationRulePOD` (64B)
    CHILD_SLOT_CSR           = 3, ///< `uint32_t` parent -> child slot ID CSR array
    POINTER_OR_CONFLICT_MASK = 4, ///< `uint32_t` `PointerOrConflictSlotFlags` per slot
    RULE_TOKEN_DOMAIN_MASKS  = 5, ///< `BitmaskDomain256POD` (32B) per `GenerationRulePOD`
    SPATIAL_NEIGHBOR_CSR     = 6, ///< `SpatialNeighborEdgePOD` (16B) CSR array
    SLOT_ENTROPY_FLOATS      = 7, ///< `float` Shannon entropy per slot
    TOKEN_COHERENCE_MASKS    = 8, ///< `uint32_t` SOA `coherence_trait_mask` per token
    ANALYSIS_RULE_POD        = 9, ///< `AnalysisRulePOD` (64B)
    COUNT                    = 10
};

struct ProceduralGenerationBufferSpec {
    ProceduralGenerationBufferSlot   slot;
    std::string_view                 buffer_name;
    std::string_view                 group_name;
    DataType                         data_type;
    size_t                           element_bytes;
    ProceduralGenerationCapacityKind capacity_kind;
    ProceduralGenerationTableGroup   table_group;
};

inline constexpr std::array<ProceduralGenerationBufferSpec, static_cast<size_t>(ProceduralGenerationBufferSlot::COUNT)>
PROCGEN_BUFFER_SPECS = {{
    { ProceduralGenerationBufferSlot::TOKEN_DESCRIPTOR_POD,     "ProcGen_TokenDescriptorPOD",    "ProcGen_Catalog",     DataType::BYTE,    sizeof(TokenDescriptorPOD),     ProceduralGenerationCapacityKind::MAX_TOKENS,            ProceduralGenerationTableGroup::TOKEN_AND_RULE_CATALOG },
    { ProceduralGenerationBufferSlot::ENVIRONMENT_SLOT_POD,     "ProcGen_EnvironmentSlotPOD",    "ProcGen_Environment", DataType::BYTE,    sizeof(EnvironmentSlotPOD),     ProceduralGenerationCapacityKind::MAX_ENVIRONMENT_SLOTS, ProceduralGenerationTableGroup::ENVIRONMENT_SLOTS_AND_TOPOLOGY },
    { ProceduralGenerationBufferSlot::CASCADE_RULE_POD,         "ProcGen_CascadeRulePOD",        "ProcGen_Catalog",     DataType::BYTE,    sizeof(GenerationRulePOD),      ProceduralGenerationCapacityKind::MAX_CASCADE_RULES,     ProceduralGenerationTableGroup::TOKEN_AND_RULE_CATALOG },
    { ProceduralGenerationBufferSlot::CHILD_SLOT_CSR,           "ProcGen_ChildSlotCSR",          "ProcGen_Environment", DataType::INT32,   sizeof(uint32_t),               ProceduralGenerationCapacityKind::MAX_CHILD_CSR_EDGES,   ProceduralGenerationTableGroup::ENVIRONMENT_SLOTS_AND_TOPOLOGY },
    { ProceduralGenerationBufferSlot::POINTER_OR_CONFLICT_MASK, "ProcGen_PointerOrConflictMask", "ProcGen_QDState",     DataType::INT32,   sizeof(uint32_t),               ProceduralGenerationCapacityKind::MAX_ENVIRONMENT_SLOTS, ProceduralGenerationTableGroup::QUANTUM_DARWINISM_AND_WFC_STATE },
    { ProceduralGenerationBufferSlot::RULE_TOKEN_DOMAIN_MASKS,  "ProcGen_RuleTokenDomainMasks",  "ProcGen_Catalog",     DataType::BYTE,    sizeof(BitmaskDomain256POD),    ProceduralGenerationCapacityKind::MAX_CASCADE_RULES,     ProceduralGenerationTableGroup::TOKEN_AND_RULE_CATALOG },
    { ProceduralGenerationBufferSlot::SPATIAL_NEIGHBOR_CSR,     "ProcGen_SpatialNeighborCSR",    "ProcGen_Environment", DataType::BYTE,    sizeof(SpatialNeighborEdgePOD), ProceduralGenerationCapacityKind::MAX_SPATIAL_CSR_EDGES, ProceduralGenerationTableGroup::ENVIRONMENT_SLOTS_AND_TOPOLOGY },
    { ProceduralGenerationBufferSlot::SLOT_ENTROPY_FLOATS,      "ProcGen_SlotEntropyFloats",     "ProcGen_QDState",     DataType::FLOAT32, sizeof(float),                  ProceduralGenerationCapacityKind::MAX_ENVIRONMENT_SLOTS, ProceduralGenerationTableGroup::QUANTUM_DARWINISM_AND_WFC_STATE },
    { ProceduralGenerationBufferSlot::TOKEN_COHERENCE_MASKS,    "ProcGen_TokenCoherenceMasks",   "ProcGen_Catalog",     DataType::INT32,   sizeof(uint32_t),               ProceduralGenerationCapacityKind::MAX_TOKENS,            ProceduralGenerationTableGroup::TOKEN_AND_RULE_CATALOG },
    { ProceduralGenerationBufferSlot::ANALYSIS_RULE_POD,        "ProcGen_AnalysisRulePOD",       "ProcGen_Catalog",     DataType::BYTE,    sizeof(AnalysisRulePOD),        ProceduralGenerationCapacityKind::MAX_CASCADE_RULES,     ProceduralGenerationTableGroup::TOKEN_AND_RULE_CATALOG }
}};

consteval bool validate_procgen_buffer_specs() {
    for (size_t i = 0; i < PROCGEN_BUFFER_SPECS.size(); ++i) {
        if (static_cast<size_t>(PROCGEN_BUFFER_SPECS[i].slot) != i) return false;
        if (PROCGEN_BUFFER_SPECS[i].element_bytes == 0) return false;
    }
    return true;
}
static_assert(validate_procgen_buffer_specs(), "PROCGEN_BUFFER_SPECS must align 1:1 with ProceduralGenerationBufferSlot!");

using ProceduralGenerationBufferManifest = DomainBufferManifest<ProceduralGenerationBufferSlot>;

} // namespace ideam::core
