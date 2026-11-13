#pragma once

/**
 * procedural_environment.h
 * Unified Godot-Reflection-Bounded Procedural Environment (`src/core/procedural_generation/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES
 * ============================================================================
 * All procedural generation subsystems (`IdeamProceduralGeneration`, `IdeamWaveFunctionCollapse`,
 * `IdeamQuantumDarwinism`, and future pattern solvers) read from and write to this shared
 * `ProceduralEnvironment` substrate.
 *
 * 1. BOUNDED BY GODOT REFLECTION:
 *    - Every candidate archetype ("token") in the environment is described by a
 *      `TokenDescriptorPOD` containing its `ReflectionSignaturePOD` (Godot class inheritance,
 *      node/resource name, prefix/suffix, groups, properties, methods, signals).
 *    - Every position or hierarchy node to be solved is an `EnvironmentSlotPOD` storing:
 *        * Its superposition domain (`BitmaskDomain256POD`)
 *        * Its parent slot index and child CSR slice (`CastleGrounds -> CastleTower -> children`)
 *        * Its spatial / port neighbor CSR slice
 *        * Its Quantum Darwinism Einselection Redundancy score (`einselection_redundancy`)
 *        * Its minimum locked `PatternCascadeLayer` (used during QD coherence relaxation)
 */

#include "cascade_rules.h"
#include <vector>

namespace ideam::core {

struct alignas(64) TokenDescriptorPOD {
    ReflectionSignaturePOD signature{};      // 32B
    uint32_t token_id             = 0;       // 4B
    float    base_frequency_weight= 1.0f;    // 4B
    uint32_t source_scene_or_res  = 0;       // 4B
    uint32_t coherence_trait_mask = 0;       // 4B: Derived from matching CSS GenerationRule tags
    uint8_t  reserved[16]         = {0};     // 16B -> 64B
};
static_assert(sizeof(TokenDescriptorPOD) == 64, "TokenDescriptorPOD must be 64 bytes.");

struct alignas(64) EnvironmentSlotPOD {
    BitmaskDomain256POD domain{};            // 32B: Active superposition of up to 256 tokens
    uint32_t slot_id              = 0;       // 4B
    uint32_t parent_slot_id       = 0xFFFFFFFFu; // 4B: Hierarchy parent slot
    uint32_t children_csr_offset  = 0;       // 4B: Offset into child slot CSR array
    uint32_t children_count       = 0;       // 4B: Number of child slots
    int32_t  collapsed_token_id   = -1;      // 4B: -1 if uncollapsed
    float    einselection_score   = 0.0f;    // 4B: Quantum Darwinism R_delta score [0..1]
    PatternCascadeLayer locked_layer = PatternCascadeLayer::LAYER_DOMAIN; // 1B
    uint8_t  is_pointer_state     = 0;       // 1B: 1 if selected as a QD Pointer State
    uint8_t  has_conflict         = 0;       // 1B: 1 if domain is empty or cardinality violated
    uint8_t  reserved[5]          = {0};     // 5B -> 64B
};
static_assert(sizeof(EnvironmentSlotPOD) == 64, "EnvironmentSlotPOD must be 64 bytes.");

class ProceduralEnvironment {
private:
    std::vector<TokenDescriptorPOD> tokens_;
    std::vector<EnvironmentSlotPOD> slots_;
    std::vector<uint32_t> child_slot_csr_;
    std::vector<GenerationRulePOD> active_rules_;
    std::vector<BitmaskDomain256POD> rule_token_masks_;

public:
    ProceduralEnvironment() = default;
    ~ProceduralEnvironment() = default;

    void clear() noexcept;
    void configure_slots(uint32_t p_slot_count, uint32_t p_token_count);

    [[nodiscard]] std::vector<TokenDescriptorPOD> &tokens() noexcept { return tokens_; }
    [[nodiscard]] const std::vector<TokenDescriptorPOD> &tokens() const noexcept { return tokens_; }

    [[nodiscard]] std::vector<EnvironmentSlotPOD> &slots() noexcept { return slots_; }
    [[nodiscard]] const std::vector<EnvironmentSlotPOD> &slots() const noexcept { return slots_; }

    [[nodiscard]] std::vector<GenerationRulePOD> &active_rules() noexcept { return active_rules_; }
    [[nodiscard]] const std::vector<GenerationRulePOD> &active_rules() const noexcept { return active_rules_; }
};

} // namespace ideam::core
