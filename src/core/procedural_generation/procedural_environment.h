#pragma once

/**
 * procedural_environment.h
 * Unified Godot-Reflection-Bounded Procedural Environment (`src/core/procedural_generation/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES (SECTIONS 1.3, 1.4 & 1.5)
 * ============================================================================
 * All procedural generation subsystems (`IdeamProceduralGeneration`, `IdeamWaveFunctionCollapse`,
 * `IdeamQuantumDarwinism`, and future pattern solvers) read from and write to this shared
 * `ProceduralEnvironment` substrate.
 *
 * 1. BOUNDED BY GODOT REFLECTION (`TokenDescriptorPOD`, 64B):
 *    - Every candidate archetype ("token") in the environment is described by its
 *      `ReflectionSignaturePOD` (Godot class inheritance, node/resource name, prefix/suffix,
 *      groups, properties, methods, signals) plus `IdeamLinguistics` nuclear & peripheral
 *      seme masks (`nuclear_seme_mask_lo32`, `peripheral_seme_mask_lo32`) and CSS
 *      `coherence_trait_mask` for `IdeamQuantumDarwinism`.
 * 2. HIERARCHY, CARDINALITY, SIBLING & SPATIAL SLOTS (`EnvironmentSlotPOD`, 64B):
 *    - Every slot stores its 256-state superposition domain (`BitmaskDomain256POD`),
 *      its parent slot index and child CSR slice (`CastleGrounds -> CastleTower -> children`),
 *      its spatial / port neighbor CSR slice, its Quantum Darwinism Einselection Redundancy
 *      score (`einselection_score`, $R_\delta$), and its `is_pointer_state` / `has_conflict` flags.
 */

#include "cascade_rules.h"
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace ideam::core {

enum PointerOrConflictSlotFlags : uint32_t {
	SLOT_STATUS_CLEAN = 0,
	SLOT_STATUS_POINTER_STATE = 1u << 0,
	SLOT_STATUS_HAS_CONFLICT = 1u << 1,
	SLOT_STATUS_COLLAPSED = 1u << 2,
	SLOT_STATUS_PINNED = 1u << 3
};

struct alignas(64) TokenDescriptorPOD {
    ReflectionSignaturePOD signature{};          ///< 32B: Class, name, prefix/suffix, groups, props, methods, signals
    uint32_t token_id                  = 0;      ///< 4B: Index `[0..255]` in `ProceduralEnvironment::tokens()`
    float    base_frequency_weight     = 1.0f;   ///< 4B: Prior corpus frequency weight
    uint32_t source_scene_or_res       = 0;      ///< 4B: Source `PackedScene` / `Resource` handle
    uint32_t coherence_trait_mask      = 0;      ///< 4B: Structural coherence mask for Quantum Darwinism Pointer States
    uint32_t nuclear_seme_mask_lo32    = 0;      ///< 4B: Locked `IdeamLinguistics` nuclear seme bits (invariant in QD)
    uint32_t peripheral_seme_mask_lo32 = 0;      ///< 4B: Relaxable `IdeamLinguistics` peripheral seme bits
    uint16_t default_min_children      = 0;      ///< 2B
    uint16_t default_max_children      = 16;     ///< 2B
    uint8_t  reserved[4]               = {0};    ///< 4B -> 64B total
};
static_assert(sizeof(TokenDescriptorPOD) == 64, "TokenDescriptorPOD must be 64 bytes.");

/**
 * SpatialNeighborEdgePOD (16 Bytes, alignas(16))
 * Directed spatial or socket adjacency link stored in `ProceduralEnvironment::spatial_neighbor_csr_`.
 */
struct alignas(16) SpatialNeighborEdgePOD {
    uint32_t neighbor_slot_id     = 0xFFFFFFFFu;
    uint32_t port_or_direction_id = 0;
    float    distance_weight      = 1.0f;
    uint32_t edge_flags           = 0;
};
static_assert(sizeof(SpatialNeighborEdgePOD) == 16, "SpatialNeighborEdgePOD must be 16 bytes.");

struct alignas(64) EnvironmentSlotPOD {
    BitmaskDomain256POD domain{};                ///< 32B: Active superposition of up to 256 tokens
    uint32_t slot_id                   = 0;      ///< 4B
    uint32_t parent_slot_id            = 0xFFFFFFFFu; ///< 4B: Hierarchy parent slot
    uint32_t children_csr_offset       = 0;      ///< 4B: Offset into child slot CSR array
    uint32_t children_count            = 0;      ///< 4B: Number of child slots
    int32_t  collapsed_token_id        = -1;     ///< 4B: -1 if uncollapsed
    float    einselection_score        = 0.0f;   ///< 4B: Quantum Darwinism R_delta score `[0..1]`
    PatternCascadeLayer locked_layer   = PatternCascadeLayer::LAYER_DOMAIN; ///< 1B
    uint8_t  is_pointer_state          = 0;      ///< 1B: 1 if selected as a QD Pointer State
    uint8_t  has_conflict              = 0;      ///< 1B: 1 if domain is empty or cardinality violated
    uint8_t  entropy_bucket            = 0;      ///< 1B: Quantized Shannon/popcount entropy bucket `[0..15]`
    uint16_t spatial_csr_offset        = 0;      ///< 2B: Offset into `spatial_neighbor_csr_`
    uint16_t spatial_neighbor_count    = 0;      ///< 2B -> 64B total
};
static_assert(sizeof(EnvironmentSlotPOD) == 64, "EnvironmentSlotPOD must be 64 bytes.");

/**
 * HarvestedSceneNodeDescriptor
 * Intermediate reflection record harvested by `ProceduralPatterner` from a Godot
 * `PackedScene` (`SceneState`) or live `Object` hierarchy to synthesize cascading
 * `GenerationRulePOD`s (`ADMIT_ARCHETYPE`, `ACCEPT_PARENTS`, `ACCEPT_CHILDREN_CARDINALITY`,
 * `REQUIRE_SIBLING`, `FORBID_SIBLING`).
 */
struct HarvestedSceneNodeDescriptor {
    std::string class_name;
    std::string node_name;
    int32_t     parent_index          = -1;
    uint32_t    group_or_tag_mask     = 0u;
    uint32_t    property_trait_mask   = 0u;
    uint32_t    method_trait_mask     = 0u;
    uint32_t    signal_trait_mask     = 0u;
    uint32_t    nuclear_seme_mask     = 0u;
    uint32_t    peripheral_seme_mask  = 0u;
};

class ProceduralEnvironment {
private:
    std::vector<TokenDescriptorPOD> tokens_;
    std::vector<std::string> token_names_;
    std::vector<EnvironmentSlotPOD> slots_;
    std::vector<uint32_t> child_slot_csr_;
    std::vector<SpatialNeighborEdgePOD> spatial_neighbor_csr_;
    std::vector<GenerationRulePOD> active_rules_;
    std::vector<BitmaskDomain256POD> rule_token_masks_;

public:
    ProceduralEnvironment() = default;
    ~ProceduralEnvironment() = default;

    void clear() noexcept;
    void configure_slots(uint32_t p_slot_count, uint32_t p_token_count);

    // --- Token & Slot Accessors ---
    [[nodiscard]] std::vector<TokenDescriptorPOD> &tokens() noexcept { return tokens_; }
    [[nodiscard]] const std::vector<TokenDescriptorPOD> &tokens() const noexcept { return tokens_; }
    [[nodiscard]] const std::vector<std::string> &token_names() const noexcept { return token_names_; }

    [[nodiscard]] std::vector<EnvironmentSlotPOD> &slots() noexcept { return slots_; }
    [[nodiscard]] const std::vector<EnvironmentSlotPOD> &slots() const noexcept { return slots_; }

    [[nodiscard]] std::vector<uint32_t> &child_slot_csr() noexcept { return child_slot_csr_; }
    [[nodiscard]] const std::vector<uint32_t> &child_slot_csr() const noexcept { return child_slot_csr_; }

    [[nodiscard]] std::vector<SpatialNeighborEdgePOD> &spatial_neighbor_csr() noexcept { return spatial_neighbor_csr_; }
    [[nodiscard]] const std::vector<SpatialNeighborEdgePOD> &spatial_neighbor_csr() const noexcept { return spatial_neighbor_csr_; }

    [[nodiscard]] std::vector<GenerationRulePOD> &active_rules() noexcept { return active_rules_; }
    [[nodiscard]] const std::vector<GenerationRulePOD> &active_rules() const noexcept { return active_rules_; }

    [[nodiscard]] std::vector<BitmaskDomain256POD> &rule_token_masks() noexcept { return rule_token_masks_; }
    [[nodiscard]] const std::vector<BitmaskDomain256POD> &rule_token_masks() const noexcept { return rule_token_masks_; }

    // --- Token Registration & Lookup ---
    uint32_t register_token(std::string_view p_token_name, const TokenDescriptorPOD &p_descriptor);
    [[nodiscard]] int32_t find_token_by_name(std::string_view p_token_name) const noexcept;
    [[nodiscard]] std::string_view get_token_name(uint32_t p_token_id) const noexcept;

    // --- Slot Hierarchy & Spatial Topology ---
    void set_slot_children(uint32_t p_parent_slot_id, std::span<const uint32_t> p_child_slot_ids);
    void set_slot_spatial_neighbors(uint32_t p_slot_id, std::span<const SpatialNeighborEdgePOD> p_neighbors);

    // --- Rule Registration & Cascade Compilation ---
    uint32_t add_generation_rule(
        const GenerationRulePOD &p_rule,
        const BitmaskDomain256POD &p_target_token_mask = BitmaskDomain256POD{}
    );

    /**
     * Synthesizes tokens and cascading `GenerationRulePOD`s (`ADMIT_ARCHETYPE`,
     * `ACCEPT_PARENTS`, `ACCEPT_CHILDREN_CARDINALITY`, `REQUIRE_SIBLING`) from a
     * harvested scene/object hierarchy filtered by `p_analysis_rules`.
     */
    void synthesize_pattern_from_harvested_nodes(
        std::span<const HarvestedSceneNodeDescriptor> p_nodes,
        std::span<const AnalysisRulePOD> p_analysis_rules,
        std::string_view p_required_class_filter = {},
        std::string_view p_required_suffix_filter = {}
    );

    /**
     * Sorts `active_rules_` by CSS cascade precedence, binds `coherence_trait_mask`
     * on `tokens_`, and initializes `slots_` domains from `ADMIT_ARCHETYPE` rules.
     */
    void compile_cascade_and_initialize_domains();

    /**
     * Propagates `ACCEPT_PARENTS`, `ACCEPT_CHILDREN_CARDINALITY` (`[min..max]`),
     * `REQUIRE_SIBLING`, and `FORBID_SIBLING` constraints across all parent/child/sibling
     * slots. Returns `false` and flags `EnvironmentSlotPOD::has_conflict = 1` if a
     * contradiction is detected.
     */
    bool propagate_hierarchy_and_cardinality_constraints();

    /**
     * Computes Quantum Darwinism Einselection Redundancy $R_\delta(i)$ across all slots
     * and marks settled slots with $R_\delta(i) \ge \tau_{\text{pointer}}$ as Pointer States.
     */
    void compute_einselection_redundancy_scores(float p_pointer_threshold = 0.75f) noexcept;
};

} // namespace ideam::core
