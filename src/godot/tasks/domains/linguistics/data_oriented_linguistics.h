#pragma once

/**
 * data_oriented_linguistics.h
 * Godot `Resource` bridge for `LinguisticsDOD` (`src/godot/tasks/domains/linguistics/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES
 * ============================================================================
 * Unites the Godot-side `LanguageAssetResource`, `LexiconResource`, `SememeResource`,
 * `SemanticTagSetResource`, `SemanticFrameResource`, `SemanticPromptParser`,
 * `MetadataLinguisticsManager`, and `LinguisticObjectProfiler` with the pure C++26
 * Data-Oriented Design (DOD) memory tables and Kahn-sorted `TaskGraphDOD` pipeline
 * in `src/core/tasks/domains/linguistics/`.
 */

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

#include "../../../../core/tasks/domains/linguistics/linguistics_dod.h"
#include "../../../linguistics/lexicon_resource.h"
#include "../../../linguistics/linguistic_object_profiler.h"
#include "../../task_graph_resource.h"

#include <cstdint>
#include <memory>

namespace ideam::godot_ext {

enum LinguisticsDODExecutionMode : int32_t {
    LING_DOD_EXEC_DISABLED              = 0, // Pure OOP Resource evaluation fallback
    LING_DOD_EXEC_COMPOSABLE_STAGES     = 1, // Bitmask-configurable 4-wave linguistic pipeline + custom co-scheduling
    LING_DOD_EXEC_CUSTOM_GRAPH_RESOURCE = 2, // User-authored TaskGraphResource bound to LinguisticsDOD buffers
    LING_DOD_EXEC_MANUAL_GRAPH          = 3  // Buffers allocated; graph left open for manual node construction
};

class DataOrientedLinguistics : public godot::Resource {
    GDCLASS(DataOrientedLinguistics, godot::Resource)

private:
    std::unique_ptr<core::LinguisticsDOD> dod_engine_;
    std::shared_ptr<core::TaskGraphDOD> active_task_graph_;
    godot::HashMap<godot::StringName, core::NodeID> ui_to_dod_map_;

    godot::Ref<LanguageAssetResource> language_asset_;
    mutable godot::Ref<LinguisticObjectProfiler> object_profiler_;
    godot::PackedByteArray staged_profile_pods_;

    // --- Inspector Configuration ---
    LinguisticsDODExecutionMode dod_execution_mode_ = LING_DOD_EXEC_COMPOSABLE_STAGES;
    uint32_t pipeline_stage_mask_                   = core::LING_STAGE_DEFAULT_PIPELINE_MASK;
    bool auto_connect_pipeline_stages_              = true;
    bool auto_sync_resources_from_dod_              = true;
    godot::Ref<TaskGraphResource> custom_task_graph_resource_;

    // --- Configurable Buffer Capacities ---
    int max_dod_sememes_          = 4096;
    int max_dod_lexemes_          = 8192;
    int max_dod_phoneme_csr_      = 32768;
    int max_dod_bpe_vocab_        = 16384;
    int max_dod_synset_relations_ = 8192;
    int max_dod_semantic_frames_  = 512;

    core::LinguisticsBufferCapacities _build_capacities() const noexcept;
    void _ensure_dod_initialized(std::shared_ptr<core::MemoryManagerDOD> p_shared_manager = nullptr);

protected:
    static void _bind_methods();

public:
    DataOrientedLinguistics();
    ~DataOrientedLinguistics() override = default;

    // --- Direct C++ Accessors ---
    [[nodiscard]] core::LinguisticsDOD *get_dod_engine() const noexcept { return dod_engine_.get(); }
    [[nodiscard]] std::shared_ptr<core::TaskGraphDOD> get_active_task_graph_shared() const noexcept { return active_task_graph_; }
    [[nodiscard]] core::TaskGraphDOD *get_active_task_graph_dod() const noexcept { return active_task_graph_.get(); }
    [[nodiscard]] std::shared_ptr<core::MemoryManagerDOD> get_memory_manager_shared() const noexcept {
        return dod_engine_ ? dod_engine_->get_memory_manager() : nullptr;
    }
    [[nodiscard]] core::MemoryManagerDOD *get_memory_manager_dod() const noexcept {
        return dod_engine_ ? dod_engine_->get_memory_manager_raw() : nullptr;
    }

    // --- Language Asset & Object Profiler Properties ---
    void set_language_asset(const godot::Ref<LanguageAssetResource> &p_asset);
    [[nodiscard]] godot::Ref<LanguageAssetResource> get_language_asset() const { return language_asset_; }

    void set_object_profiler(const godot::Ref<LinguisticObjectProfiler> &p_profiler) { object_profiler_ = p_profiler; }
    [[nodiscard]] godot::Ref<LinguisticObjectProfiler> get_object_profiler() const;

    // --- Inspector Execution Setters / Getters ---
    void set_dod_execution_mode(LinguisticsDODExecutionMode p_mode);
    [[nodiscard]] LinguisticsDODExecutionMode get_dod_execution_mode() const noexcept { return dod_execution_mode_; }

    void set_pipeline_stage_mask(int p_mask);
    [[nodiscard]] int get_pipeline_stage_mask() const noexcept { return static_cast<int>(pipeline_stage_mask_); }

    void set_auto_connect_pipeline_stages(bool p_auto_connect);
    [[nodiscard]] bool get_auto_connect_pipeline_stages() const noexcept { return auto_connect_pipeline_stages_; }

    void set_auto_sync_resources_from_dod(bool p_auto_sync);
    [[nodiscard]] bool get_auto_sync_resources_from_dod() const noexcept { return auto_sync_resources_from_dod_; }

    void set_custom_task_graph_resource(const godot::Ref<TaskGraphResource> &p_resource);
    [[nodiscard]] godot::Ref<TaskGraphResource> get_custom_task_graph_resource() const { return custom_task_graph_resource_; }

    void set_max_dod_sememes(int p_max);
    [[nodiscard]] int get_max_dod_sememes() const noexcept { return max_dod_sememes_; }

    void set_max_dod_lexemes(int p_max);
    [[nodiscard]] int get_max_dod_lexemes() const noexcept { return max_dod_lexemes_; }

    void set_max_dod_phoneme_csr(int p_max);
    [[nodiscard]] int get_max_dod_phoneme_csr() const noexcept { return max_dod_phoneme_csr_; }

    void set_max_dod_bpe_vocab(int p_max);
    [[nodiscard]] int get_max_dod_bpe_vocab() const noexcept { return max_dod_bpe_vocab_; }

    void set_max_dod_synset_relations(int p_max);
    [[nodiscard]] int get_max_dod_synset_relations() const noexcept { return max_dod_synset_relations_; }

    void set_max_dod_semantic_frames(int p_max);
    [[nodiscard]] int get_max_dod_semantic_frames() const noexcept { return max_dod_semantic_frames_; }

    // --- Pipeline Setup, Baking & Execution API ---
    void setup_dod_pipeline_with_manager(
        std::shared_ptr<core::MemoryManagerDOD> p_shared_manager,
        std::shared_ptr<core::TaskGraphDOD> p_shared_graph = nullptr
    );
    void setup_dod_pipeline();
    void bake_to_dod();
    void rebuild_dod_task_graph();
    void execute_dod_step(double p_delta);
    void sync_oop_resources_from_dod();

    // --- Object Profiling & Buffer Manifest API ---
    [[nodiscard]] godot::PackedByteArray bake_objects_to_profile_buffer(
        const godot::Array &p_objects,
        bool p_cache_in_metadata = true
    );
    [[nodiscard]] godot::PackedByteArray get_staged_profile_pods() const { return staged_profile_pods_; }
    [[nodiscard]] int get_staged_profile_count() const {
        return static_cast<int>(staged_profile_pods_.size() / static_cast<int64_t>(sizeof(core::LinguisticProfilePOD)));
    }

    [[nodiscard]] int get_buffer_id(const godot::StringName &p_semantic_name) const;
    [[nodiscard]] godot::Dictionary get_buffer_manifest_dictionary() const;

    // --- High-Level DOD Query, Phonetic Decode, FrameNet & SLM Helpers ---
    void set_query_prompt(const godot::String &p_prompt);
    void set_query_tag_set(const godot::Ref<SemanticTagSetResource> &p_tag_set);
    [[nodiscard]] godot::PackedInt32Array get_matched_sememe_indices(float p_min_affinity = 0.25f) const;
    [[nodiscard]] godot::PackedFloat32Array get_sememe_affinity_scores() const;

    int queue_noisy_phonetic_utterance(
        const godot::PackedInt32Array &p_noisy_masks,
        int p_expected_sememe_id = -1,
        int p_preferred_lect_id = 0
    );
    [[nodiscard]] godot::Dictionary get_decoded_utterance_result(int p_utterance_slot) const;

    int queue_frame_clause_evaluation(
        const godot::String &p_frame_name,
        const godot::String &p_agent_token,
        const godot::String &p_patient_token,
        const godot::String &p_instrument_token = ""
    );
    [[nodiscard]] godot::Dictionary get_frame_clause_result(int p_clause_slot) const;

    [[nodiscard]] godot::PackedInt32Array get_compiled_slm_allowed_bpe_tokens() const;

    // --- Dynamic Task Node & Wave Co-Scheduling API ---
    int add_linguistics_transform_node(
        int p_logic_id,
        const godot::Dictionary &p_properties = godot::Dictionary(),
        int p_priority = 0,
        bool p_use_registry_factory = false
    );
    int add_linguistics_query_node(
        int p_logic_id,
        int p_query_op = 0,
        const godot::Dictionary &p_properties = godot::Dictionary(),
        int p_priority = 0,
        bool p_use_registry_factory = false
    );
    int add_linguistics_metadata_node(
        int p_logic_id,
        const godot::Dictionary &p_properties = godot::Dictionary(),
        int p_priority = 0,
        bool p_use_registry_factory = false
    );

    bool schedule_node_with_stage(int p_stage_flag, int p_custom_node_id, bool p_join_downstream = false);
    bool schedule_node_between_stages(int p_upstream_stage_flag, int p_downstream_stage_flag, int p_custom_node_id);
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::LinguisticsDODExecutionMode);
