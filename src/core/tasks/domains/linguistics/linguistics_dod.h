#pragma once

/**
 * linguistics_dod.h
 * Lean, table-driven C++26 Data-Oriented Design (DOD) coordinator for `IdeamLinguistics`
 * (`src/core/tasks/domains/linguistics/`).
 *
 * Responsibilities:
 *   1. Loop-driven allocation, selection-range synchronization, semantic name lookup, and
 *      `TaskGraphResource` dictionary population across the 20 `LinguisticsDOD` buffer slots
 *      via `LINGUISTICS_BUFFER_SPECS`.
 *   2. Single-pass baking (`bake_from_semantic_lexicon_core`) of `SemanticLexiconCore` tables:
 *      - `SememeDescriptorPOD` (64B hybrid symbolic + 128-bit MRL binary embeddings) & SOA columns
 *      - `LexemePOD` + flattened `uint16_t` PanPhon/CMUdict articulatory feature CSR slices
 *      - `MorphemePOD`, `BPEVocabEntryPOD`, and `BPEMergeRulePOD` subword tables
 *      - `SynsetRelationPOD` WordNet hypernym/holonym edges & `SemanticFramePOD` FrameNet schemas
 *   3. Staging & readback for scene object `LinguisticProfilePOD` rows, active prompt query context
 *      (`LING_QUERY_CONTEXT_POD`), noisy `PhoneticUtteranceSpanPOD` DTW slices, and
 *      `FrameClauseInstancePOD` thematic role evaluations.
 *   4. Templated `add_linguistics_task_node<T_Logic>()` builder and `inject_pipeline_stages()`
 *      for all 7 `IdeamLinguistics` tasks (`LNG1–LNG7`).
 */

#include "linguistics_dod_common.h"
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

class LinguisticsDOD {
private:
    std::shared_ptr<MemoryManagerDOD> memory_manager_;
    std::shared_ptr<TaskGraphDOD> default_task_graph_;
    MemoryManagerDOD *raw_memory_override_ = nullptr;
    TaskGraphDOD *raw_graph_override_      = nullptr;

    LinguisticsBufferCapacities capacities_{};
    LinguisticsBufferManifest manifest_{};
    LinguisticsPipelineTopology active_topology_{};

    // --- O(1) Slot Allocators per Primary Table ---
    SlotIndexAllocator sememe_allocator_;
    SlotIndexAllocator lexeme_allocator_;
    SlotIndexAllocator morpheme_allocator_;
    SlotIndexAllocator bpe_vocab_allocator_;
    SlotIndexAllocator bpe_merge_allocator_;
    SlotIndexAllocator synset_relation_allocator_;
    SlotIndexAllocator semantic_frame_allocator_;
    SlotIndexAllocator frame_clause_allocator_;
    SlotIndexAllocator utterance_span_allocator_;

    // --- Contiguous CSR / Stream Bump Cursors ---
    uint32_t lexeme_phoneme_csr_cursor_    = 0u;
    uint32_t phoneme_stream_cursor_        = 0u;
    uint32_t decoded_sememe_stream_cursor_ = 0u;
    uint32_t slm_vocab_mask_cursor_        = 0u;

    uint32_t _active_count_for_group(LinguisticsTableGroup p_group) const noexcept;
    void _bind_default_logic_buffers(void *r_logic_bytes, size_t p_size, const void *p_default_bytes) const noexcept;

public:
    explicit LinguisticsDOD(
        std::shared_ptr<MemoryManagerDOD> p_memory_manager = nullptr,
        const LinguisticsBufferCapacities &p_capacities = LinguisticsBufferCapacities{}
    );
    ~LinguisticsDOD() = default;

    LinguisticsDOD(const LinguisticsDOD &) = delete;
    LinguisticsDOD &operator=(const LinguisticsDOD &) = delete;
    LinguisticsDOD(LinguisticsDOD &&) noexcept = default;
    LinguisticsDOD &operator=(LinguisticsDOD &&) noexcept = default;

    // --- Backwards-Compatible Raw Context Binding ---
    void bind_execution_context(MemoryManagerDOD *p_mem, TaskGraphDOD *p_graph) noexcept {
        raw_memory_override_ = p_mem;
        raw_graph_override_  = p_graph;
    }

    [[nodiscard]] MemoryManagerDOD *memory_manager() const noexcept {
        return raw_memory_override_ ? raw_memory_override_ : memory_manager_.get();
    }
    [[nodiscard]] TaskGraphDOD *task_graph() const noexcept {
        return raw_graph_override_ ? raw_graph_override_ : default_task_graph_.get();
    }

    // --- Core Manager & Table-Driven Manifest Lifecycle ---
    void initialize_buffers(const LinguisticsBufferCapacities &p_capacities = LinguisticsBufferCapacities{});
    void sync_selection_ranges() noexcept;
    void reset_all_slots() noexcept;

    [[nodiscard]] std::shared_ptr<MemoryManagerDOD> get_memory_manager() const noexcept { return memory_manager_; }
    [[nodiscard]] MemoryManagerDOD *get_memory_manager_raw() const noexcept { return memory_manager(); }

    [[nodiscard]] std::shared_ptr<TaskGraphDOD> get_default_task_graph() const noexcept { return default_task_graph_; }
    void set_default_task_graph(std::shared_ptr<TaskGraphDOD> p_graph) noexcept { default_task_graph_ = std::move(p_graph); }

    [[nodiscard]] const LinguisticsBufferCapacities &get_capacities() const noexcept { return capacities_; }
    [[nodiscard]] const LinguisticsBufferManifest &get_buffer_manifest() const noexcept { return manifest_; }
    [[nodiscard]] const LinguisticsPipelineTopology &get_active_topology() const noexcept { return active_topology_; }

    [[nodiscard]] constexpr uint32_t get_buffer_id(LinguisticsBufferSlot p_slot) const noexcept { return manifest_.get(p_slot); }
    [[nodiscard]] constexpr uint32_t get_buffer_id_by_name(std::string_view p_name) const noexcept { return manifest_.get_by_name(p_name); }

    void populate_task_buffer_dictionary(godot::Dictionary &r_props, bool p_overwrite_existing = false) const;
    [[nodiscard]] GrantPartPOD make_buffer_requirement(uint32_t p_buffer_id, BufferAccessMode p_access) const noexcept;
    [[nodiscard]] GrantPartPOD make_slot_requirement(LinguisticsBufferSlot p_slot, BufferAccessMode p_access) const noexcept {
        return make_buffer_requirement(manifest_.get(p_slot), p_access);
    }

    // --- Direct Typed Column Accessors ---
    template <typename T>
    [[nodiscard]] T *get_column_ptr(LinguisticsBufferSlot p_slot) const noexcept {
        const uint32_t buf_id = manifest_.get(p_slot);
        MemoryManagerDOD *mgr = memory_manager();
        if (!mgr || buf_id == INVALID_ID) return nullptr;
        MemoryBufferPOD *buf = mgr->get_buffer(buf_id);
        if (!buf || !buf->master_block_ptr) return nullptr;
        return reinterpret_cast<T *>(buf->master_block_ptr + buf->memory_offset);
    }

    template <typename T>
    [[nodiscard]] std::span<T> get_column_span(LinguisticsBufferSlot p_slot) const noexcept {
        const uint32_t buf_id = manifest_.get(p_slot);
        MemoryManagerDOD *mgr = memory_manager();
        if (!mgr || buf_id == INVALID_ID) return {};
        MemoryBufferPOD *buf = mgr->get_buffer(buf_id);
        if (!buf || !buf->master_block_ptr || buf->max_elements <= 0) return {};
        return std::span<T>(
            reinterpret_cast<T *>(buf->master_block_ptr + buf->memory_offset),
            static_cast<size_t>(buf->max_elements)
        );
    }

    // ========================================================================
    // Table 1: Sememe & Object Profile Staging + Query Context
    // ========================================================================

    uint32_t allocate_sememe_descriptor(const SememeDescriptorPOD &p_desc);
    [[nodiscard]] uint32_t get_active_sememe_count() const noexcept { return sememe_allocator_.get_active_count(); }
    [[nodiscard]] SememeDescriptorPOD get_sememe_descriptor(uint32_t p_slot) const noexcept;
    void set_sememe_descriptor(uint32_t p_slot, const SememeDescriptorPOD &p_desc) noexcept;

    uint32_t stage_object_profiles(std::span<const LinguisticProfilePOD> p_profiles);
    void set_query_context(
        const SememeDescriptorPOD &p_required_query,
        const SememeDescriptorPOD &p_excluded_query = SememeDescriptorPOD{}
    ) noexcept;
    [[nodiscard]] float get_sememe_affinity_score(uint32_t p_slot) const noexcept;

    // ========================================================================
    // Table 2 & 3: SemanticLexiconCore Single-Pass Bake & Readback
    // ========================================================================

    void bake_from_semantic_lexicon_core(const SemanticLexiconCore &p_lexicon);
    void sync_to_semantic_lexicon_core(SemanticLexiconCore &r_lexicon) const;

    uint32_t allocate_lexeme(
        const LexemePOD &p_lexeme,
        std::span<const uint16_t> p_phoneme_masks = {}
    );
    [[nodiscard]] uint32_t get_active_lexeme_count() const noexcept { return lexeme_allocator_.get_active_count(); }
    [[nodiscard]] uint32_t get_active_synset_relation_count() const noexcept { return synset_relation_allocator_.get_active_count(); }
    [[nodiscard]] uint32_t get_active_semantic_frame_count() const noexcept { return semantic_frame_allocator_.get_active_count(); }
    [[nodiscard]] uint32_t get_active_bpe_vocab_count() const noexcept { return bpe_vocab_allocator_.get_active_count(); }
    [[nodiscard]] uint32_t get_active_bpe_merge_count() const noexcept { return bpe_merge_allocator_.get_active_count(); }

    // ========================================================================
    // Table 4: Noisy Phonetic Utterances, BPE Token Stream & Frame Clauses
    // ========================================================================

    uint32_t stage_phonetic_utterance(
        std::span<const uint16_t> p_noisy_phoneme_masks,
        uint32_t p_expected_sememe_id = 0xFFFFFFFFu,
        uint16_t p_preferred_lect_id = 0u
    );
    [[nodiscard]] uint32_t get_active_utterance_span_count() const noexcept { return utterance_span_allocator_.get_active_count(); }
    [[nodiscard]] PhoneticUtteranceSpanPOD get_utterance_span(uint32_t p_span_slot) const noexcept;

    uint32_t stage_bpe_token_stream(std::span<const uint32_t> p_token_ids) noexcept;
    [[nodiscard]] std::vector<uint32_t> read_decoded_sememe_or_bpe_stream() const;

    uint32_t stage_frame_clause_instance(
        uint32_t p_frame_id,
        uint32_t p_agent_sememe_id,
        uint32_t p_patient_sememe_id,
        uint32_t p_instrument_sememe_id = 0xFFFFFFFFu,
        uint32_t p_location_sememe_id = 0xFFFFFFFFu
    );
    [[nodiscard]] uint32_t get_active_frame_clause_count() const noexcept { return frame_clause_allocator_.get_active_count(); }
    [[nodiscard]] FrameClauseInstancePOD get_frame_clause_instance(uint32_t p_clause_slot) const noexcept;

    [[nodiscard]] std::vector<uint32_t> read_slm_allowed_bpe_tokens() const;

    // ========================================================================
    // TEMPLATED TASK NODE BUILDER (LNG1 - LNG7) & PIPELINE STAGE INJECTION
    // ========================================================================

    template <typename T_Logic>
    NodeID add_linguistics_task_node(
        TaskGraphDOD &r_graph,
        const T_Logic &p_logic_override = T_Logic{},
        const LinguisticsNodeExecutionConfig &p_config = {}
    ) const;

    // Single-line convenience aliases forwarding to add_linguistics_task_node<T_Logic>
    NodeID add_lexicon_phoneme_feature_index_node(TaskGraphDOD &g, const LexiconPhonemeFeatureIndexMetadataLogic<> &l = {}, const LinguisticsNodeExecutionConfig &c = {}) const { return add_linguistics_task_node(g, l, c); }
    NodeID add_wordnet_synset_hypernym_propagate_node(TaskGraphDOD &g, const WordNetSynsetHypernymPropagateTransformLogic<> &l = {}, const LinguisticsNodeExecutionConfig &c = {}) const { return add_linguistics_task_node(g, l, c); }
    NodeID add_seme_componential_affinity_node(TaskGraphDOD &g, const SemeComponentialAffinityQueryLogic<> &l = {}, const LinguisticsNodeExecutionConfig &c = {}) const { return add_linguistics_task_node(g, l, c); }
    NodeID add_matryoshka_binary_hamming_node(TaskGraphDOD &g, const MatryoshkaBinaryHammingQueryLogic<> &l = {}, const LinguisticsNodeExecutionConfig &c = {}) const { return add_linguistics_task_node(g, l, c); }
    NodeID add_phonetic_hamming_lexeme_decode_node(TaskGraphDOD &g, const PhoneticHammingLexemeDecodeTransformLogic<> &l = {}, const LinguisticsNodeExecutionConfig &c = {}) const { return add_linguistics_task_node(g, l, c); }
    NodeID add_bpe_subword_and_morphology_node(TaskGraphDOD &g, const BPESubwordAndMorphologyTransformLogic<> &l = {}, const LinguisticsNodeExecutionConfig &c = {}) const { return add_linguistics_task_node(g, l, c); }
    NodeID add_semantic_frame_and_slm_mask_node(TaskGraphDOD &g, const SemanticFrameAndSLMMaskTransformLogic<> &l = {}, const LinguisticsNodeExecutionConfig &c = {}) const { return add_linguistics_task_node(g, l, c); }

    NodeID add_transform_node_by_id(
        TaskGraphDOD &r_graph,
        LinguisticsTransformLogicID p_logic_id,
        const godot::Dictionary &p_custom_props = godot::Dictionary(),
        const LinguisticsNodeExecutionConfig &p_config = {}
    ) const;

    NodeID add_query_node_by_id(
        TaskGraphDOD &r_graph,
        LinguisticsQueryLogicID p_logic_id,
        const godot::Dictionary &p_custom_props = godot::Dictionary(),
        const LinguisticsNodeExecutionConfig &p_config = {}
    ) const;

    NodeID add_metadata_node_by_id(
        TaskGraphDOD &r_graph,
        LinguisticsMetadataLogicID p_logic_id,
        const godot::Dictionary &p_custom_props = godot::Dictionary(),
        const LinguisticsNodeExecutionConfig &p_config = {}
    ) const;

    LinguisticsPipelineTopology inject_pipeline_stages(
        TaskGraphDOD &r_graph,
        uint32_t p_stage_mask = LING_STAGE_DEFAULT_PIPELINE_MASK,
        bool p_auto_connect_stages = true
    );

    static bool schedule_node_with_stage(
        TaskGraphDOD &r_graph,
        const LinguisticsPipelineTopology &p_topology,
        LinguisticsPipelineStageFlags p_target_stage,
        NodeID p_custom_node,
        bool p_join_downstream = false
    );

    static bool schedule_node_between_stages(
        TaskGraphDOD &r_graph,
        const LinguisticsPipelineTopology &p_topology,
        LinguisticsPipelineStageFlags p_upstream_stage,
        LinguisticsPipelineStageFlags p_downstream_stage,
        NodeID p_custom_node
    );

    void execute_step(double p_delta, TaskGraphDOD *p_override_graph = nullptr);
};

} // namespace ideam::core
