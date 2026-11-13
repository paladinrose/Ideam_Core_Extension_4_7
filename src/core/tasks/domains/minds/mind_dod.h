#pragma once

/**
 * mind_dod.h
 * Lean, table-driven C++26 Data-Oriented Design (DOD) coordinator for the `IdeamMinds`
 * epistemic AI, reasoning, and discourse substrate (`src/core/tasks/domains/minds/`).
 *
 * Responsibilities:
 *   1. Loop-driven allocation, selection-range synchronization, semantic name lookup, and
 *      `TaskGraphResource` dictionary population across the 51 `MindDOD` buffer slots via
 *      `MIND_BUFFER_SPECS`.
 *   2. Multi-Source **Akashic Tome** staging (`allocate_akashic_tome`, `allocate_ground_truth_fact`,
 *      `bake_from_information_matrix`) and Per-Agent **Epistemic Reference Catalogue** staging
 *      (`allocate_agent`, `allocate_belief_catalogue_row`), keeping both AoS (`FactPod`, `BeliefPod`,
 *      `BeliefCatalogueEntryPod`) and SOA columns synchronized.
 *   3. Staging & readback for Reasoning (`ObservationPod`, `HypothesisPod`, `EvidencePod`,
 *      `SyllogisticPropositionPod`, `ExplanationPod`), Spoken Dialogue (`DiscourseTurnPod`),
 *      and the 3 Event Rings (`SensoryObservationEventRing`, `SpeechClaimEventRing`,
 *      `MindIntuitionAndContradictionRing`).
 *   4. Templated `add_mind_task_node<T_Logic>()` builder and `inject_pipeline_stages()`
 *      for all 12 `IdeamMinds` tasks (`MND1–MND12`, Section 2.3).
 */

#include "minds_dod_common.h"
#include "../../task_graph_dod.h"
#include "../../transform_task.h"
#include "../../query_task.h"
#include "../../metadata_task.h"

#include <godot_cpp/variant/dictionary.hpp>

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace ideam::core {

class MindDOD {
private:
    std::shared_ptr<MemoryManagerDOD> memory_manager_;
    std::shared_ptr<TaskGraphDOD> default_task_graph_;

    MindsBufferCapacities capacities_{};
    MindsBufferManifest manifest_{};
    MindsPipelineTopology active_topology_{};

    // --- O(1) Slot Allocators per Primary Table ---
    SlotIndexAllocator tome_allocator_;
    SlotIndexAllocator fact_allocator_;
    SlotIndexAllocator agent_allocator_;
    SlotIndexAllocator belief_allocator_;
    SlotIndexAllocator observation_allocator_;
    SlotIndexAllocator hypothesis_allocator_;
    SlotIndexAllocator evidence_allocator_;
    SlotIndexAllocator syllogism_allocator_;
    SlotIndexAllocator explanation_allocator_;
    SlotIndexAllocator discourse_turn_allocator_;

    // --- Contiguous CSR / Token Bump Cursors ---
    uint32_t explanation_fact_csr_cursor_  = 0u;
    uint32_t explanation_cause_csr_cursor_ = 0u;
    uint32_t sememe_token_cursor_          = 0u;

    // --- Generic Ring Push / Drain Helpers (Shared by All 3 Event Rings) ---
    template <typename T_Event>
    bool _push_ring_event(
        MindBufferSlot p_ring_slot,
        MindBufferSlot p_cursor_slot,
        uint32_t p_max_capacity,
        const T_Event &p_event
    ) noexcept {
        auto *ring    = get_column_ptr<T_Event>(p_ring_slot);
        auto *cursors = get_column_ptr<uint32_t>(p_cursor_slot);
        if (!ring || !cursors || p_max_capacity == 0u) return false;

        const uint32_t write_idx = cursors[1];
        if (write_idx >= p_max_capacity) return false;

        ring[write_idx] = p_event;
        cursors[1]      = write_idx + 1u;
        return true;
    }

    template <typename T_Event>
    uint32_t _drain_ring_events(
        MindBufferSlot p_ring_slot,
        MindBufferSlot p_cursor_slot,
        uint32_t p_max_capacity,
        std::vector<T_Event> &r_out_events,
        uint32_t p_max_drain
    ) noexcept {
        const auto *ring = get_column_ptr<T_Event>(p_ring_slot);
        auto *cursors    = get_column_ptr<uint32_t>(p_cursor_slot);
        if (!ring || !cursors) return 0u;

        const uint32_t write_idx = std::min(cursors[1], p_max_capacity);
        uint32_t read_idx        = std::min(cursors[0], write_idx);
        uint32_t drained         = 0u;

        while (read_idx < write_idx && drained < p_max_drain) {
            r_out_events.push_back(ring[read_idx++]);
            ++drained;
        }

        if (read_idx >= write_idx) {
            cursors[0] = 0u;
            cursors[1] = 0u;
        } else {
            cursors[0] = read_idx;
        }
        return drained;
    }

    uint32_t _active_count_for_group(MindTableGroup p_group) const noexcept;
    void _bind_default_logic_buffers(void *r_logic_bytes, size_t p_size, const void *p_default_bytes) const noexcept;

public:
    explicit MindDOD(
        std::shared_ptr<MemoryManagerDOD> p_memory_manager = nullptr,
        const MindsBufferCapacities &p_capacities = MindsBufferCapacities{}
    );
    ~MindDOD() = default;

    MindDOD(const MindDOD &) = delete;
    MindDOD &operator=(const MindDOD &) = delete;
    MindDOD(MindDOD &&) noexcept = default;
    MindDOD &operator=(MindDOD &&) noexcept = default;

    // --- Core Manager & Table-Driven Manifest Lifecycle ---
    void initialize_buffers(const MindsBufferCapacities &p_capacities = MindsBufferCapacities{});
    void sync_selection_ranges() noexcept;
    void reset_all_slots() noexcept;

    [[nodiscard]] std::shared_ptr<MemoryManagerDOD> get_memory_manager() const noexcept { return memory_manager_; }
    [[nodiscard]] MemoryManagerDOD *get_memory_manager_raw() const noexcept { return memory_manager_.get(); }

    [[nodiscard]] std::shared_ptr<TaskGraphDOD> get_default_task_graph() const noexcept { return default_task_graph_; }
    void set_default_task_graph(std::shared_ptr<TaskGraphDOD> p_graph) noexcept { default_task_graph_ = std::move(p_graph); }

    [[nodiscard]] const MindsBufferCapacities &get_capacities() const noexcept { return capacities_; }
    [[nodiscard]] const MindsBufferManifest &get_buffer_manifest() const noexcept { return manifest_; }
    [[nodiscard]] const MindsPipelineTopology &get_active_topology() const noexcept { return active_topology_; }

    [[nodiscard]] constexpr uint32_t get_buffer_id(MindBufferSlot p_slot) const noexcept { return manifest_.get(p_slot); }
    [[nodiscard]] constexpr uint32_t get_buffer_id_by_name(std::string_view p_name) const noexcept { return manifest_.get_by_name(p_name); }

    void populate_task_buffer_dictionary(godot::Dictionary &r_props, bool p_overwrite_existing = false) const;
    [[nodiscard]] GrantPartPOD make_buffer_requirement(uint32_t p_buffer_id, BufferAccessMode p_access) const noexcept;
    [[nodiscard]] GrantPartPOD make_slot_requirement(MindBufferSlot p_slot, BufferAccessMode p_access) const noexcept {
        return make_buffer_requirement(manifest_.get(p_slot), p_access);
    }

    // --- Direct Typed Column Accessors ---
    template <typename T>
    [[nodiscard]] T *get_column_ptr(MindBufferSlot p_slot) const noexcept {
        const uint32_t buf_id = manifest_.get(p_slot);
        if (!memory_manager_ || buf_id == INVALID_ID) return nullptr;
        MemoryBufferPOD *buf = memory_manager_->get_buffer(buf_id);
        if (!buf || !buf->master_block_ptr) return nullptr;
        return reinterpret_cast<T *>(buf->master_block_ptr + buf->memory_offset);
    }

    template <typename T>
    [[nodiscard]] std::span<T> get_column_span(MindBufferSlot p_slot) const noexcept {
        const uint32_t buf_id = manifest_.get(p_slot);
        if (!memory_manager_ || buf_id == INVALID_ID) return {};
        MemoryBufferPOD *buf = memory_manager_->get_buffer(buf_id);
        if (!buf || !buf->master_block_ptr || buf->max_elements <= 0) return {};
        return std::span<T>(
            reinterpret_cast<T *>(buf->master_block_ptr + buf->memory_offset),
            static_cast<size_t>(buf->max_elements)
        );
    }

    // ========================================================================
    // Table 1: WorldGroundTruthFactTable (Multi-Source Akashic Tomes & Facts)
    // ========================================================================

    uint32_t allocate_akashic_tome(const AkashicTomeHeaderPod &p_header);
    [[nodiscard]] uint32_t get_active_tome_count() const noexcept { return tome_allocator_.get_active_count(); }
    [[nodiscard]] AkashicTomeHeaderPod get_akashic_tome_header(uint32_t p_tome_slot) const noexcept;

    uint32_t allocate_ground_truth_fact(const FactPod &p_fact, uint16_t p_tome_id = TOME_PHYSICAL_REALITY);
    [[nodiscard]] bool is_fact_slot_valid(uint32_t p_slot) const noexcept { return fact_allocator_.is_valid(p_slot); }
    [[nodiscard]] uint32_t get_active_fact_count() const noexcept { return fact_allocator_.get_active_count(); }
    [[nodiscard]] FactPod get_ground_truth_fact(uint32_t p_fact_slot) const noexcept;
    void set_ground_truth_fact(uint32_t p_fact_slot, const FactPod &p_fact, uint16_t p_tome_id = TOME_PHYSICAL_REALITY) noexcept;

    // ========================================================================
    // Table 2: AgentTraitAndMentalStateTable
    // ========================================================================

    uint32_t allocate_agent(
        const InstinctiveTraitWeightsPod &p_traits = {},
        float p_attention_threshold = 0.25f,
        float p_knowledge_threshold = 1.0f,
        float p_excitation_level = 0.0f,
        uint32_t p_active_emotion_mask = 0u,
        uint16_t p_primary_tome_id = TOME_PHYSICAL_REALITY
    );
    [[nodiscard]] bool is_agent_slot_valid(uint32_t p_slot) const noexcept { return agent_allocator_.is_valid(p_slot); }
    [[nodiscard]] uint32_t get_active_agent_count() const noexcept { return agent_allocator_.get_active_count(); }
    [[nodiscard]] InstinctiveTraitWeightsPod get_agent_traits(uint32_t p_agent_slot) const noexcept;
    void set_agent_traits(uint32_t p_agent_slot, const InstinctiveTraitWeightsPod &p_traits) noexcept;

    // ========================================================================
    // Table 3: AgentBeliefSparseTable (Epistemic Reference Catalogue)
    // ========================================================================

    uint32_t allocate_belief_catalogue_row(
        const BeliefPod &p_belief,
        const BeliefCatalogueEntryPod &p_catalogue_entry
    );
    [[nodiscard]] uint32_t get_active_belief_count() const noexcept { return belief_allocator_.get_active_count(); }
    [[nodiscard]] BeliefPod get_belief_pod(uint32_t p_belief_slot) const noexcept;
    [[nodiscard]] BeliefCatalogueEntryPod get_belief_catalogue_entry(uint32_t p_belief_slot) const noexcept;

    /**
     * Bakes an entire `InformationMatrixDOD` (all registered Akashic Tomes, Ground-Truth Facts,
     * Hypotheses, Syllogisms, Explanations, and Per-Agent Reference Catalogues) into contiguous
     * `MindDOD` buffers in a single pass.
     */
    void bake_from_information_matrix(
        const InformationMatrixDOD &p_matrix,
        std::span<const uint32_t> p_agent_ids = {}
    );

    /**
     * Synchronizes updated `BeliefPod` and `BeliefCatalogueEntryPod` rows from `MindDOD` back
     * into `InformationMatrixDOD` for OOP inspection or persistence.
     */
    void sync_to_information_matrix(
        InformationMatrixDOD &r_matrix,
        std::span<const uint32_t> p_agent_ids = {}
    ) const;

    // ========================================================================
    // Table 4 & Table 5: Reasoning, Theories & Spoken Dialogue Staging
    // ========================================================================

    uint32_t allocate_observation(const ObservationPod &p_obs);
    uint32_t allocate_hypothesis(const HypothesisPod &p_hyp);
    uint32_t allocate_evidence(const EvidencePod &p_ev);
    uint32_t allocate_syllogism(const SyllogisticPropositionPod &p_syl);
    uint32_t allocate_explanation(
        const ExplanationPod &p_exp,
        std::span<const uint32_t> p_fact_ids = {},
        std::span<const uint32_t> p_cause_ids = {}
    );
    uint32_t allocate_discourse_turn(
        const DiscourseTurnPod &p_turn,
        uint64_t p_speaker_mask = 0ULL,
        uint64_t p_listener_mask = 0ULL
    );

    // ========================================================================
    // 3 Event Ring Push / Drain Wrappers
    // ========================================================================

    bool push_sensory_observation_event(const SensoryEventPod &p_ev) noexcept {
        return _push_ring_event(
            MindBufferSlot::MIND_SENSORY_OBSERVATION_EVENT_RING,
            MindBufferSlot::MIND_SENSORY_RING_CURSOR,
            capacities_.max_sensory_events,
            p_ev
        );
    }
    uint32_t drain_sensory_observation_events(std::vector<SensoryEventPod> &r_out, uint32_t p_max_drain = 2048u) noexcept {
        return _drain_ring_events(
            MindBufferSlot::MIND_SENSORY_OBSERVATION_EVENT_RING,
            MindBufferSlot::MIND_SENSORY_RING_CURSOR,
            capacities_.max_sensory_events,
            r_out,
            p_max_drain
        );
    }

    bool push_speech_claim_event(const ClaimPod &p_claim) noexcept {
        return _push_ring_event(
            MindBufferSlot::MIND_SPEECH_CLAIM_EVENT_RING,
            MindBufferSlot::MIND_SPEECH_CLAIM_RING_CURSOR,
            capacities_.max_speech_claims,
            p_claim
        );
    }
    uint32_t drain_speech_claim_events(std::vector<ClaimPod> &r_out, uint32_t p_max_drain = 1024u) noexcept {
        return _drain_ring_events(
            MindBufferSlot::MIND_SPEECH_CLAIM_EVENT_RING,
            MindBufferSlot::MIND_SPEECH_CLAIM_RING_CURSOR,
            capacities_.max_speech_claims,
            r_out,
            p_max_drain
        );
    }

    bool push_epistemic_event(const MindEpistemicEventPod &p_ev) noexcept {
        return _push_ring_event(
            MindBufferSlot::MIND_INTUITION_CONTRADICTION_RING,
            MindBufferSlot::MIND_INTUITION_RING_CURSOR,
            capacities_.max_epistemic_events,
            p_ev
        );
    }
    uint32_t drain_epistemic_events(std::vector<MindEpistemicEventPod> &r_out, uint32_t p_max_drain = 1024u) noexcept {
        return _drain_ring_events(
            MindBufferSlot::MIND_INTUITION_CONTRADICTION_RING,
            MindBufferSlot::MIND_INTUITION_RING_CURSOR,
            capacities_.max_epistemic_events,
            r_out,
            p_max_drain
        );
    }

    // ========================================================================
    // TEMPLATED TASK NODE BUILDER (MND1 - MND12) & PIPELINE STAGE INJECTION
    // ========================================================================

    template <typename T_Logic>
    NodeID add_mind_task_node(
        TaskGraphDOD &r_graph,
        const T_Logic &p_logic_override = T_Logic{},
        const MindsNodeExecutionConfig &p_config = {}
    ) const;

    // Single-line convenience aliases forwarding to add_mind_task_node<T_Logic>
    NodeID add_sensory_signal_decode_node(TaskGraphDOD &g, const SensorySignalDecodeTransformLogic<> &l = {}, const MindsNodeExecutionConfig &c = {}) const { return add_mind_task_node(g, l, c); }
    NodeID add_sensory_observation_wake_node(TaskGraphDOD &g, const SensoryObservationWakeQueryLogic<> &l = {}, const MindsNodeExecutionConfig &c = {}) const { return add_mind_task_node(g, l, c); }
    NodeID add_instinctive_affect_appraisal_node(TaskGraphDOD &g, const InstinctiveAffectAppraisalTransformLogic<> &l = {}, const MindsNodeExecutionConfig &c = {}) const { return add_mind_task_node(g, l, c); }
    NodeID add_stimulus_latency_triage_node(TaskGraphDOD &g, const StimulusLatencyTriageMetadataLogic<> &l = {}, const MindsNodeExecutionConfig &c = {}) const { return add_mind_task_node(g, l, c); }
    NodeID add_abductive_hypothesis_node(TaskGraphDOD &g, const AbductiveHypothesisTransformLogic<> &l = {}, const MindsNodeExecutionConfig &c = {}) const { return add_mind_task_node(g, l, c); }
    NodeID add_intuitive_leap_stochastic_node(TaskGraphDOD &g, const IntuitiveLeapStochasticQueryLogic<> &l = {}, const MindsNodeExecutionConfig &c = {}) const { return add_mind_task_node(g, l, c); }
    NodeID add_inductive_evidence_accumulate_node(TaskGraphDOD &g, const InductiveEvidenceAccumulateTransformLogic<> &l = {}, const MindsNodeExecutionConfig &c = {}) const { return add_mind_task_node(g, l, c); }
    NodeID add_deductive_syllogism_propagate_node(TaskGraphDOD &g, const DeductiveSyllogismPropagateTransformLogic<> &l = {}, const MindsNodeExecutionConfig &c = {}) const { return add_mind_task_node(g, l, c); }
    NodeID add_deliberation_and_deception_node(TaskGraphDOD &g, const DeliberationAndDeceptionTransformLogic<> &l = {}, const MindsNodeExecutionConfig &c = {}) const { return add_mind_task_node(g, l, c); }
    NodeID add_discourse_turn_and_prosody_node(TaskGraphDOD &g, const DiscourseTurnAndProsodyTransformLogic<> &l = {}, const MindsNodeExecutionConfig &c = {}) const { return add_mind_task_node(g, l, c); }
    NodeID add_contradiction_and_trust_reconcile_node(TaskGraphDOD &g, const ContradictionAndTrustReconcileTransformLogic<> &l = {}, const MindsNodeExecutionConfig &c = {}) const { return add_mind_task_node(g, l, c); }
    NodeID add_sememe_and_lexicon_morphology_node(TaskGraphDOD &g, const SememeAndLexiconMorphologyTransformLogic<> &l = {}, const MindsNodeExecutionConfig &c = {}) const { return add_mind_task_node(g, l, c); }

    NodeID add_transform_node_by_id(
        TaskGraphDOD &r_graph,
        MindsTransformLogicID p_logic_id,
        const godot::Dictionary &p_custom_props = godot::Dictionary(),
        const MindsNodeExecutionConfig &p_config = {}
    ) const;

    NodeID add_query_node_by_id(
        TaskGraphDOD &r_graph,
        MindsQueryLogicID p_logic_id,
        const godot::Dictionary &p_custom_props = godot::Dictionary(),
        const MindsNodeExecutionConfig &p_config = {}
    ) const;

    NodeID add_metadata_node_by_id(
        TaskGraphDOD &r_graph,
        MindsMetadataLogicID p_logic_id,
        const godot::Dictionary &p_custom_props = godot::Dictionary(),
        const MindsNodeExecutionConfig &p_config = {}
    ) const;

    MindsPipelineTopology inject_pipeline_stages(
        TaskGraphDOD &r_graph,
        uint32_t p_stage_mask = MINDS_STAGE_DEFAULT_PIPELINE_MASK,
        bool p_auto_connect_stages = true
    );

    static bool schedule_node_with_stage(
        TaskGraphDOD &r_graph,
        const MindsPipelineTopology &p_topology,
        MindsPipelineStageFlags p_target_stage,
        NodeID p_custom_node,
        bool p_join_downstream = false
    );

    static bool schedule_node_between_stages(
        TaskGraphDOD &r_graph,
        const MindsPipelineTopology &p_topology,
        MindsPipelineStageFlags p_upstream_stage,
        MindsPipelineStageFlags p_downstream_stage,
        NodeID p_custom_node
    );

    void execute_step(double p_delta, TaskGraphDOD *p_override_graph = nullptr);
};

} // namespace ideam::core
