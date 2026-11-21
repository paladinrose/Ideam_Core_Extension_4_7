#pragma once

/**
 * quantum_darwinism_common.h
 * Pure C++26 POD types for `IdeamQuantumDarwinism` (`src/core/quantum_darwinism/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES (FULL GODOT-CANVAS PROCGEN & WFC REPAIR)
 * ============================================================================
 * 1. THEORETICAL GROUNDING (ZUREK'S QUANTUM DARWINISM):
 *    - Interaction with the environment E causes *einselection* (environment-induced
 *      superselection): fragile superpositions decohere while **Pointer States**
 *      survive by imprinting redundant copies (R_delta) across many independent
 *      fragments of the environment.
 *
 * 2. DUAL-ROLE ENGINE ON THE GODOT ENVIRONMENT CANVAS:
 *    - **Conflict Stabilizer Mode (`MODE_CONFLICT_STABILIZER`)**: Draws a bounded
 *      Environment Horizon Box (E_box) around WFC/ProcGen conflicts, promotes high-
 *      redundancy settled cells to Pointer States, and relaxes lower-precedence CSS
 *      cascade layers inward along coherence bridges.
 *    - **Outward Generative Mode (`MODE_OUTWARD_GENERATIVE`)**: Elevates Quantum
 *      Darwinism into a standalone ProcGen solver whose canvas is the Godot
 *      environment itself (SceneTrees, Node/Resource hierarchies, ClassDB archetypes,
 *      reflected properties/methods/signals, IdeamLinguistics semes, and optional
 *      spatial/port graphs):
 *        a) **Factored Reflection & Semantic Channels (`PropertyChannelSpecPOD`,
 *           `SlotChannelStatePOD`)**: Decomposes each slot into K independent
 *           channels with per-channel redundancy R_delta_k, threshold tau_k,
 *           permeability gamma_k, and lock state.
 *        b) **Multi-Topology Outward Wavefronts**: Propagates outward from pinned
 *           landmarks across parent->child, child->parent, sibling<->sibling, and
 *           arbitrary directed/spatial edges (lifting the 512-slot conflict cap).
 *        c) **Cross-Channel Threshold Coupling (`ChannelCouplingRulePOD`)**: When
 *           channel k locks in slot i, shifts tau_m, gamma_m, and state/value biases
 *           for channels m != k across scoped relatives (SELF, CHILDREN, DESCENDANTS,
 *           SIBLINGS, ALL).
 *        d) **Symmetry Breaking & Spontaneous Nucleation**: Combines deterministic
 *           graph/slot-hash perturbation (plus optional FastNoiseLite/DOD field
 *           modulation) with frontier nucleation on wavefront stall, then projects
 *           converged channels back onto `EnvironmentSlotPOD::domain`.
 */

#include "../procedural_generation/procedural_environment.h"
#include "../wave_function_collapse/wfc_common.h"
#include <cstdint>

namespace ideam::core {

enum class EinselectionStatus : uint8_t {
    IDLE                   = 0,
    HORIZON_ISOLATED       = 1,
    POINTERS_ESTABLISHED   = 2,
    COHERENCE_RESTORED     = 3,
    UNRESOLVABLE_HORIZON   = 4,
    WAVEFRONT_PROPAGATING  = 5,
    GENERATION_CONVERGED   = 6
};

enum class QuantumDarwinismMode : uint8_t {
    MODE_CONFLICT_STABILIZER           = 0, ///< Reactive inward conflict repair around WFC/ProcGen conflicts
    MODE_OUTWARD_GENERATIVE            = 1, ///< Standalone outward multi-channel wavefront ProcGen solver
    MODE_HYBRID_GENERATE_AND_STABILIZE = 2  ///< Outward generative einselection + inward conflict stabilization
};

/**
 * Reflection or semantic aspect represented by a Quantum Darwinism property channel
 * on the Godot environment canvas.
 */
enum class QDChannelAspect : uint8_t {
    ASPECT_REFLECTED_PROPERTY = 0, ///< Reflected Godot property name/range (e.g. `light_energy`, `difficulty_tier`)
    ASPECT_CLASS_ARCHETYPE    = 1, ///< `ClassDB` class family or archetype token family
    ASPECT_GROUP_OR_TAG       = 2, ///< Godot Node group or metadata tag bit
    ASPECT_NUCLEAR_SEME       = 3, ///< Locked `IdeamLinguistics` nuclear seme bit
    ASPECT_PERIPHERAL_SEME    = 4, ///< Relaxable `IdeamLinguistics` peripheral seme bit
    ASPECT_METHOD_TRAIT       = 5, ///< Reflected `has_method` capability bit
    ASPECT_SIGNAL_TRAIT       = 6, ///< Reflected `has_signal` capability bit
    ASPECT_CUSTOM_TRAIT       = 7  ///< Domain-defined custom trait (physical, narrative, gameplay, acoustic)
};

/**
 * Bitmask controlling which relational topologies carry a channel's wavefront
 * or a cross-channel coupling rule across `ProceduralEnvironment`.
 */
enum QDTopologyMask : uint8_t {
    TOPOLOGY_NONE             = 0u,
    TOPOLOGY_HIERARCHY_DOWN   = 1u << 0, ///< Parent -> Child (`child_slot_csr_`)
    TOPOLOGY_HIERARCHY_UP     = 1u << 1, ///< Child -> Parent (`parent_slot_id`)
    TOPOLOGY_SIBLINGS         = 1u << 2, ///< Lateral Sibling <-> Sibling under same parent
    TOPOLOGY_GRAPH_OR_SPATIAL = 1u << 3, ///< Directed/port/reference/spatial edges (`spatial_neighbor_csr_`)
    TOPOLOGY_ALL              = 0x0Fu
};

/**
 * Classification of a slot inside or around an Environment Horizon Box (`E_box`)
 * or outward generative wavefront.
 */
enum HorizonSlotRoleFlags : uint8_t {
    HORIZON_ROLE_OUTSIDE         = 0u,
    HORIZON_ROLE_CONFLICT_CENTER = 1u << 0, ///< Distance 0: conflicted slot or seed center
    HORIZON_ROLE_INTERIOR        = 1u << 1, ///< Distance `1 .. radius-1` inside `E_box` / wavefront
    HORIZON_ROLE_PERIMETER       = 1u << 2, ///< Distance `== radius` boundary anchor
    HORIZON_ROLE_POINTER_STATE   = 1u << 3, ///< Selected as immutable Pointer State ($R_\delta \ge \tau$)
    HORIZON_ROLE_BRIDGE_RELAXED  = 1u << 4, ///< Intermediate bridge slot whose leaf layer was relaxed
    HORIZON_ROLE_NUCLEATED_SEED  = 1u << 5  ///< Secondary Pointer State minted by spontaneous nucleation
};

/**
 * HorizonBoxConfigPOD (32 Bytes, alignas(16))
 * Parameters governing how Quantum Darwinism bounds the local environment around a conflict.
 */
struct alignas(16) HorizonBoxConfigPOD {
    uint32_t max_graph_hop_radius        = 4;     ///< Max spatial/hierarchy hops from conflict slot
    uint32_t max_horizon_slots           = 128;   ///< Hard upper bound on slots inside E_box (conflict mode)
    float    pointer_redundancy_thresh   = 0.75f; ///< R_delta threshold to qualify as a Pointer State
    float    fragile_redundancy_thresh   = 0.30f; ///< R_delta below which a slot fully re-opens
    PatternCascadeLayer min_locked_layer = PatternCascadeLayer::LAYER_HIERARCHY_PARENT;
    uint8_t  allow_sibling_rebalancing   = 1;     ///< Allow adjusting [min..max] child/sibling counts
    uint8_t  lock_nuclear_semes          = 1;     ///< Preserve `IdeamLinguistics` nuclear seme bits
    uint8_t  promote_perimeter_anchors   = 1;     ///< Promote settled perimeter slots on E_box boundary
    uint8_t  reserved[8]                 = {0};
};
static_assert(sizeof(HorizonBoxConfigPOD) == 32, "HorizonBoxConfigPOD must be 32 bytes.");

/**
 * PointerStateRecordPOD (32 Bytes, alignas(16))
 * Represents a high-redundancy settled cell selected as an immutable anchor.
 */
struct alignas(16) PointerStateRecordPOD {
    uint32_t slot_id                    = 0;
    uint32_t settled_token_id           = 0;
    float    einselection_redundancy    = 1.0f;  ///< R_delta mutual-information witness score
    uint32_t highest_satisfied_spec     = 0;     ///< Highest CSS GenerationRule specificity satisfied
    uint32_t preserved_trait_mask       = 0;     ///< Structural trait & nuclear seme bits propagated
    uint32_t hop_distance_to_conflict   = 0;
    uint8_t  reserved[8]                = {0};
};
static_assert(sizeof(PointerStateRecordPOD) == 32, "PointerStateRecordPOD must be 32 bytes.");

/**
 * CoherenceBridgeSlotPOD (32 Bytes, alignas(16))
 * Represents an intermediate slot lying between a Pointer State and a conflicted slot.
 */
struct alignas(16) CoherenceBridgeSlotPOD {
    uint32_t slot_id                    = 0;
    uint32_t source_pointer_slot_id     = 0;
    uint32_t target_conflict_slot_id    = 0;
    uint32_t hops_from_pointer          = 0;
    uint32_t hops_to_conflict           = 0;
    uint32_t mutable_trait_mask         = 0;     ///< Properties/siblings alterable without losing coherence
    PatternCascadeLayer relax_down_to   = PatternCascadeLayer::LAYER_CHILD_CARDINALITY;
    uint8_t  reserved[7]                = {0};
};
static_assert(sizeof(CoherenceBridgeSlotPOD) == 32, "CoherenceBridgeSlotPOD must be 32 bytes.");

/**
 * HorizonSlotPartitionPOD (16 Bytes, alignas(16))
 * Per-slot horizon/wavefront metadata computed by `ConflictHorizonBoxPartitionMetadataLogic` (`QD1`).
 */
struct alignas(16) HorizonSlotPartitionPOD {
    uint32_t slot_id                 = 0;
    uint32_t nearest_conflict_slot   = 0xFFFFFFFFu; ///< Nearest conflict (stabilizer) or seed landmark (generative)
    uint32_t predecessor_toward_conf = 0xFFFFFFFFu; ///< Next hop along shortest path toward conflict/seed
    uint16_t hop_distance            = 0xFFFFu;     ///< `0` = center/seed, `0xFFFF` = unreached
    uint8_t  role_flags              = HORIZON_ROLE_OUTSIDE;
    uint8_t  locked_cascade_floor    = static_cast<uint8_t>(PatternCascadeLayer::LAYER_HIERARCHY_PARENT);
};
static_assert(sizeof(HorizonSlotPartitionPOD) == 16, "HorizonSlotPartitionPOD must be 16 bytes.");

/**
 * PropertyChannelSpecPOD (64 Bytes, alignas(64))
 * Defines one factored Godot-reflection or semantic channel propagated by Quantum Darwinism.
 */
struct alignas(64) PropertyChannelSpecPOD {
    uint32_t        channel_id                 = 0;
    uint32_t        symbol_hash                = 0;      ///< `hash_reflection_symbol` of property/group/class/trait
    uint32_t        trait_bit_mask             = 0;      ///< Bitmask projected into `ReflectionSignaturePOD` or seme masks
    float           default_threshold_tau      = 0.65f;  ///< Baseline einselection crystallization threshold $\tau_k$
    float           default_permeability_gamma = 0.78f;  ///< Baseline propagation permeability $\gamma_k \in [0..1]$
    float           decay_per_hop              = 0.14f;  ///< Redundancy attenuation per graph/hierarchy hop
    float           value_min                  = 0.0f;   ///< Min value for continuous channels
    float           value_max                  = 1.0f;   ///< Max value for continuous channels
    float           default_value              = 0.0f;   ///< Neutral starting scalar value
    uint32_t        state_count                = 4;      ///< Number of discrete states `[0..32]` if discrete
    QDChannelAspect aspect                     = QDChannelAspect::ASPECT_REFLECTED_PROPERTY;
    uint8_t         topology_mask              = TOPOLOGY_ALL;
    uint8_t         is_discrete_mask           = 0;      ///< `1` = discrete state bitmask, `0` = continuous float
    uint8_t         is_nuclear_invariant       = 0;      ///< `1` = once locked, immune to relaxation
    uint8_t         reserved[20]               = {0};
};
static_assert(sizeof(PropertyChannelSpecPOD) == 64, "PropertyChannelSpecPOD must be 64 bytes.");

/**
 * SlotChannelStatePOD (32 Bytes, alignas(32))
 * Per-(slot, channel) state stored in flat DOD buffers (`slot_id * channel_count + channel_id`).
 */
struct alignas(32) SlotChannelStatePOD {
    uint32_t slot_id                = 0;
    uint32_t channel_id             = 0;
    uint32_t candidate_state_mask   = 0xFFFFFFFFu; ///< Active discrete state bits or trait mask
    float    value                  = 0.0f;        ///< Continuous scalar value or resolved discrete index
    float    redundancy_r_delta     = 0.0f;        ///< Accumulated einselection redundancy $R_{\delta,k} \in [0..1]$
    float    effective_threshold_tau = 0.65f;      ///< Effective $\tau_k$ after cross-channel coupling + perturbation
    float    effective_permeability = 0.78f;       ///< Effective $\gamma_k$ after cross-channel coupling
    uint8_t  is_locked              = 0;           ///< `1` once $R_{\delta,k} \ge \tau_k$ or pinned/nucleated
    uint8_t  is_pinned              = 0;           ///< `1` if explicitly seeded by designer (`LAYER_PINNED_OVERRIDE`)
    uint8_t  is_nucleated           = 0;           ///< `1` if minted by spontaneous nucleation on wavefront stall
    uint8_t  locked_wave_index      = 0;           ///< Wave iteration when this channel crystallized
};
static_assert(sizeof(SlotChannelStatePOD) == 32, "SlotChannelStatePOD must be 32 bytes.");

/**
 * ChannelCouplingRulePOD (64 Bytes, alignas(64))
 * Cross-channel threshold & permeability coupling rule:
 * When `source_channel_id` locks (matching `required_source_state_mask` or `[source_value_min, source_value_max]`),
 * shifts `effective_threshold_tau`, `effective_permeability`, and value/state bias of `target_channel_id`
 * across slots matching `scope` and `topology_mask`.
 */
struct alignas(64) ChannelCouplingRulePOD {
    uint32_t         rule_id                    = 0;
    uint32_t         source_channel_id          = 0;
    uint32_t         target_channel_id          = 0;
    uint32_t         required_source_state_mask = 0;       ///< `0` = trigger on any locked state
    float            source_value_min           = -1e30f;  ///< Min continuous trigger value
    float            source_value_max           =  1e30f;  ///< Max continuous trigger value
    float            delta_threshold_tau        = 0.0f;    ///< Negative = easier einselection, Positive = inhibits
    float            delta_permeability_gamma   = 0.0f;    ///< Shift to propagation permeability $\gamma_m$
    float            target_value_bias          = 0.0f;    ///< Continuous value attractor
    float            value_bias_weight          = 0.0f;    ///< `[0..1]` blend weight toward `target_value_bias`
    uint32_t         target_state_bias_mask     = 0;       ///< Preferred discrete states for target channel
    uint32_t         target_forbid_state_mask   = 0;       ///< Forbidden discrete states for target channel
    PatternRuleScope scope                      = PatternRuleScope::SELF;
    uint8_t          topology_mask              = TOPOLOGY_ALL;
    uint8_t          max_coupling_hops          = 1;       ///< `0` = self only, `1` = 1-ring relatives/neighbors
    uint8_t          priority                   = 0;
    uint8_t          reserved[12]               = {0};
};
static_assert(sizeof(ChannelCouplingRulePOD) == 64, "ChannelCouplingRulePOD must be 64 bytes.");

/**
 * GenerativeWavefrontConfigPOD (32 Bytes, alignas(16))
 * Parameters controlling standalone outward generative einselection and spontaneous nucleation.
 */
struct alignas(16) GenerativeWavefrontConfigPOD {
    uint64_t             master_seed                     = 1337ULL;
    uint32_t             max_wavefront_iterations        = 64;
    float                perturbation_strength           = 0.12f; ///< Stochastic / noise warp amplitude on $\tau$ & weights
    float                nucleation_temperature          = 0.15f; ///< Tie-breaking temperature on wavefront stall
    float                min_frontier_redundancy         = 0.05f; ///< Minimum $R_\delta$ to prefer frontier over cold slot
    QuantumDarwinismMode mode                            = QuantumDarwinismMode::MODE_OUTWARD_GENERATIVE;
    uint8_t              enable_spontaneous_nucleation   = 1;
    uint8_t              enable_cross_channel_coupling   = 1;
    uint8_t              auto_stabilize_after_projection = 1;
    uint8_t              reserved[4]                     = {0};
};
static_assert(sizeof(GenerativeWavefrontConfigPOD) == 32, "GenerativeWavefrontConfigPOD must be 32 bytes.");

/**
 * WavefrontNucleationEventPOD (16 Bytes, alignas(16))
 * Telemetry record of a spontaneous nucleation event when an outward wave stalled.
 */
struct alignas(16) WavefrontNucleationEventPOD {
    uint32_t slot_id                   = 0;
    uint32_t channel_id                = 0;
    uint32_t wave_index                = 0;
    float    pre_nucleation_redundancy = 0.0f;
};
static_assert(sizeof(WavefrontNucleationEventPOD) == 16, "WavefrontNucleationEventPOD must be 16 bytes.");

} // namespace ideam::core
