#include "data_oriented_linguistics.h"

#include <algorithm>
#include <cstring>
#include <vector>

namespace ideam::godot_ext {

DataOrientedLinguistics::DataOrientedLinguistics()
    : dod_engine_(std::make_unique<core::LinguisticsDOD>()) {
    active_task_graph_ = dod_engine_->get_default_task_graph();
}

void DataOrientedLinguistics::_bind_methods() {
    using namespace godot;

    BIND_ENUM_CONSTANT(LING_DOD_EXEC_DISABLED);
    BIND_ENUM_CONSTANT(LING_DOD_EXEC_COMPOSABLE_STAGES);
    BIND_ENUM_CONSTANT(LING_DOD_EXEC_CUSTOM_GRAPH_RESOURCE);
    BIND_ENUM_CONSTANT(LING_DOD_EXEC_MANUAL_GRAPH);

    ADD_GROUP("Linguistics Asset & Object Profiler", "");

    ClassDB::bind_method(D_METHOD("set_language_asset", "language_asset"), &DataOrientedLinguistics::set_language_asset);
    ClassDB::bind_method(D_METHOD("get_language_asset"), &DataOrientedLinguistics::get_language_asset);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "language_asset", PROPERTY_HINT_RESOURCE_TYPE, "LanguageAssetResource"),
        "set_language_asset",
        "get_language_asset"
    );

    ClassDB::bind_method(D_METHOD("set_object_profiler", "profiler"), &DataOrientedLinguistics::set_object_profiler);
    ClassDB::bind_method(D_METHOD("get_object_profiler"), &DataOrientedLinguistics::get_object_profiler);

    ADD_GROUP("DOD Pipeline Configuration", "");

    ClassDB::bind_method(D_METHOD("set_dod_execution_mode", "mode"), &DataOrientedLinguistics::set_dod_execution_mode);
    ClassDB::bind_method(D_METHOD("get_dod_execution_mode"), &DataOrientedLinguistics::get_dod_execution_mode);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "dod_execution_mode", PROPERTY_HINT_ENUM, "Disabled:0,Composable Stages:1,Custom TaskGraphResource:2,Manual Graph:3"),
        "set_dod_execution_mode",
        "get_dod_execution_mode"
    );

    ClassDB::bind_method(D_METHOD("set_pipeline_stage_mask", "mask"), &DataOrientedLinguistics::set_pipeline_stage_mask);
    ClassDB::bind_method(D_METHOD("get_pipeline_stage_mask"), &DataOrientedLinguistics::get_pipeline_stage_mask);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "pipeline_stage_mask"), "set_pipeline_stage_mask", "get_pipeline_stage_mask");

    ClassDB::bind_method(D_METHOD("set_auto_connect_pipeline_stages", "auto_connect"), &DataOrientedLinguistics::set_auto_connect_pipeline_stages);
    ClassDB::bind_method(D_METHOD("get_auto_connect_pipeline_stages"), &DataOrientedLinguistics::get_auto_connect_pipeline_stages);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_connect_pipeline_stages"), "set_auto_connect_pipeline_stages", "get_auto_connect_pipeline_stages");

    ClassDB::bind_method(D_METHOD("set_auto_sync_resources_from_dod", "auto_sync"), &DataOrientedLinguistics::set_auto_sync_resources_from_dod);
    ClassDB::bind_method(D_METHOD("get_auto_sync_resources_from_dod"), &DataOrientedLinguistics::get_auto_sync_resources_from_dod);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_sync_resources_from_dod"), "set_auto_sync_resources_from_dod", "get_auto_sync_resources_from_dod");

    ClassDB::bind_method(D_METHOD("set_custom_task_graph_resource", "resource"), &DataOrientedLinguistics::set_custom_task_graph_resource);
    ClassDB::bind_method(D_METHOD("get_custom_task_graph_resource"), &DataOrientedLinguistics::get_custom_task_graph_resource);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "custom_task_graph_resource", PROPERTY_HINT_RESOURCE_TYPE, "TaskGraphResource"),
        "set_custom_task_graph_resource",
        "get_custom_task_graph_resource"
    );

    ADD_GROUP("DOD Buffer Capacities", "");

    ClassDB::bind_method(D_METHOD("set_max_dod_sememes", "max"), &DataOrientedLinguistics::set_max_dod_sememes);
    ClassDB::bind_method(D_METHOD("get_max_dod_sememes"), &DataOrientedLinguistics::get_max_dod_sememes);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_dod_sememes"), "set_max_dod_sememes", "get_max_dod_sememes");

    ClassDB::bind_method(D_METHOD("set_max_dod_lexemes", "max"), &DataOrientedLinguistics::set_max_dod_lexemes);
    ClassDB::bind_method(D_METHOD("get_max_dod_lexemes"), &DataOrientedLinguistics::get_max_dod_lexemes);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_dod_lexemes"), "set_max_dod_lexemes", "get_max_dod_lexemes");

    ClassDB::bind_method(D_METHOD("set_max_dod_phoneme_csr", "max"), &DataOrientedLinguistics::set_max_dod_phoneme_csr);
    ClassDB::bind_method(D_METHOD("get_max_dod_phoneme_csr"), &DataOrientedLinguistics::get_max_dod_phoneme_csr);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_dod_phoneme_csr"), "set_max_dod_phoneme_csr", "get_max_dod_phoneme_csr");

    ClassDB::bind_method(D_METHOD("set_max_dod_bpe_vocab", "max"), &DataOrientedLinguistics::set_max_dod_bpe_vocab);
    ClassDB::bind_method(D_METHOD("get_max_dod_bpe_vocab"), &DataOrientedLinguistics::get_max_dod_bpe_vocab);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_dod_bpe_vocab"), "set_max_dod_bpe_vocab", "get_max_dod_bpe_vocab");

    ClassDB::bind_method(D_METHOD("set_max_dod_synset_relations", "max"), &DataOrientedLinguistics::set_max_dod_synset_relations);
    ClassDB::bind_method(D_METHOD("get_max_dod_synset_relations"), &DataOrientedLinguistics::get_max_dod_synset_relations);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_dod_synset_relations"), "set_max_dod_synset_relations", "get_max_dod_synset_relations");

    ClassDB::bind_method(D_METHOD("set_max_dod_semantic_frames", "max"), &DataOrientedLinguistics::set_max_dod_semantic_frames);
    ClassDB::bind_method(D_METHOD("get_max_dod_semantic_frames"), &DataOrientedLinguistics::get_max_dod_semantic_frames);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_dod_semantic_frames"), "set_max_dod_semantic_frames", "get_max_dod_semantic_frames");

    // --- Pipeline Lifecycle & Object Profiling Methods ---
    ClassDB::bind_method(D_METHOD("setup_dod_pipeline"), &DataOrientedLinguistics::setup_dod_pipeline);
    ClassDB::bind_method(D_METHOD("bake_to_dod"), &DataOrientedLinguistics::bake_to_dod);
    ClassDB::bind_method(D_METHOD("rebuild_dod_task_graph"), &DataOrientedLinguistics::rebuild_dod_task_graph);
    ClassDB::bind_method(D_METHOD("execute_dod_step", "delta"), &DataOrientedLinguistics::execute_dod_step);
    ClassDB::bind_method(D_METHOD("sync_oop_resources_from_dod"), &DataOrientedLinguistics::sync_oop_resources_from_dod);

    ClassDB::bind_method(
        D_METHOD("bake_objects_to_profile_buffer", "objects", "cache_in_metadata"),
        &DataOrientedLinguistics::bake_objects_to_profile_buffer,
        DEFVAL(true)
    );
    ClassDB::bind_method(D_METHOD("get_staged_profile_pods"), &DataOrientedLinguistics::get_staged_profile_pods);
    ClassDB::bind_method(D_METHOD("get_staged_profile_count"), &DataOrientedLinguistics::get_staged_profile_count);

    ClassDB::bind_method(D_METHOD("get_buffer_id", "semantic_name"), &DataOrientedLinguistics::get_buffer_id);
    ClassDB::bind_method(D_METHOD("get_buffer_manifest_dictionary"), &DataOrientedLinguistics::get_buffer_manifest_dictionary);

    // --- High-Level DOD Query, Phonetic Decode, FrameNet & SLM Methods ---
    ClassDB::bind_method(D_METHOD("set_query_prompt", "prompt"), &DataOrientedLinguistics::set_query_prompt);
    ClassDB::bind_method(D_METHOD("set_query_tag_set", "tag_set"), &DataOrientedLinguistics::set_query_tag_set);
    ClassDB::bind_method(
        D_METHOD("get_matched_sememe_indices", "min_affinity"),
        &DataOrientedLinguistics::get_matched_sememe_indices,
        DEFVAL(0.25f)
    );
    ClassDB::bind_method(D_METHOD("get_sememe_affinity_scores"), &DataOrientedLinguistics::get_sememe_affinity_scores);

    ClassDB::bind_method(
        D_METHOD("queue_noisy_phonetic_utterance", "noisy_masks", "expected_sememe_id", "preferred_lect_id"),
        &DataOrientedLinguistics::queue_noisy_phonetic_utterance,
        DEFVAL(-1),
        DEFVAL(0)
    );
    ClassDB::bind_method(D_METHOD("get_decoded_utterance_result", "utterance_slot"), &DataOrientedLinguistics::get_decoded_utterance_result);

    ClassDB::bind_method(
        D_METHOD("queue_frame_clause_evaluation", "frame_name", "agent_token", "patient_token", "instrument_token"),
        &DataOrientedLinguistics::queue_frame_clause_evaluation,
        DEFVAL("")
    );
    ClassDB::bind_method(D_METHOD("get_frame_clause_result", "clause_slot"), &DataOrientedLinguistics::get_frame_clause_result);
    ClassDB::bind_method(D_METHOD("get_compiled_slm_allowed_bpe_tokens"), &DataOrientedLinguistics::get_compiled_slm_allowed_bpe_tokens);

    // --- Dynamic Task Node & Wave Co-Scheduling ---
    ClassDB::bind_method(
        D_METHOD("add_linguistics_transform_node", "logic_id", "properties", "priority", "use_registry_factory"),
        &DataOrientedLinguistics::add_linguistics_transform_node,
        DEFVAL(Dictionary()),
        DEFVAL(0),
        DEFVAL(false)
    );
    ClassDB::bind_method(
        D_METHOD("add_linguistics_query_node", "logic_id", "query_op", "properties", "priority", "use_registry_factory"),
        &DataOrientedLinguistics::add_linguistics_query_node,
        DEFVAL(0),
        DEFVAL(Dictionary()),
        DEFVAL(0),
        DEFVAL(false)
    );
    ClassDB::bind_method(
        D_METHOD("add_linguistics_metadata_node", "logic_id", "properties", "priority", "use_registry_factory"),
        &DataOrientedLinguistics::add_linguistics_metadata_node,
        DEFVAL(Dictionary()),
        DEFVAL(0),
        DEFVAL(false)
    );

    ClassDB::bind_method(
        D_METHOD("schedule_node_with_stage", "stage_flag", "custom_node_id", "join_downstream"),
        &DataOrientedLinguistics::schedule_node_with_stage,
        DEFVAL(false)
    );
    ClassDB::bind_method(
        D_METHOD("schedule_node_between_stages", "upstream_stage_flag", "downstream_stage_flag", "custom_node_id"),
        &DataOrientedLinguistics::schedule_node_between_stages
    );
}

core::LinguisticsBufferCapacities DataOrientedLinguistics::_build_capacities() const noexcept {
    core::LinguisticsBufferCapacities caps{};
    caps.max_sememes          = static_cast<uint32_t>(std::max(64, max_dod_sememes_));
    caps.max_lexemes          = static_cast<uint32_t>(std::max(64, max_dod_lexemes_));
    caps.max_phoneme_csr_masks= static_cast<uint32_t>(std::max(256, max_dod_phoneme_csr_));
    caps.max_bpe_vocab        = static_cast<uint32_t>(std::max(64, max_dod_bpe_vocab_));
    caps.max_bpe_merge_rules  = static_cast<uint32_t>(std::max(64, max_dod_bpe_vocab_));
    caps.max_synset_relations = static_cast<uint32_t>(std::max(64, max_dod_synset_relations_));
    caps.max_semantic_frames  = static_cast<uint32_t>(std::max(16, max_dod_semantic_frames_));
    return caps;
}

void DataOrientedLinguistics::_ensure_dod_initialized(std::shared_ptr<core::MemoryManagerDOD> p_shared_manager) {
    const core::LinguisticsBufferCapacities caps = _build_capacities();
    if (!dod_engine_) {
        dod_engine_ = std::make_unique<core::LinguisticsDOD>(std::move(p_shared_manager), caps);
    } else if (p_shared_manager && dod_engine_->get_memory_manager() != p_shared_manager) {
        dod_engine_ = std::make_unique<core::LinguisticsDOD>(std::move(p_shared_manager), caps);
    }
    if (!active_task_graph_) {
        active_task_graph_ = dod_engine_->get_default_task_graph();
    }
}

void DataOrientedLinguistics::set_language_asset(const godot::Ref<LanguageAssetResource> &p_asset) {
    language_asset_ = p_asset;
    if (object_profiler_.is_valid() && object_profiler_->get_prompt_parser().is_valid()) {
        object_profiler_->get_prompt_parser()->set_language_asset(language_asset_);
    }
    emit_changed();
}

godot::Ref<LinguisticObjectProfiler> DataOrientedLinguistics::get_object_profiler() const {
    if (object_profiler_.is_null()) {
        object_profiler_.instantiate();
        if (language_asset_.is_valid() && object_profiler_->get_prompt_parser().is_valid()) {
            object_profiler_->get_prompt_parser()->set_language_asset(language_asset_);
        }
    }
    return object_profiler_;
}

void DataOrientedLinguistics::set_dod_execution_mode(LinguisticsDODExecutionMode p_mode) {
    dod_execution_mode_ = p_mode;
}

void DataOrientedLinguistics::set_pipeline_stage_mask(int p_mask) {
    pipeline_stage_mask_ = static_cast<uint32_t>(p_mask) & core::LING_STAGE_ALL_MASK;
}

void DataOrientedLinguistics::set_auto_connect_pipeline_stages(bool p_auto_connect) {
    auto_connect_pipeline_stages_ = p_auto_connect;
}

void DataOrientedLinguistics::set_auto_sync_resources_from_dod(bool p_auto_sync) {
    auto_sync_resources_from_dod_ = p_auto_sync;
}

void DataOrientedLinguistics::set_custom_task_graph_resource(const godot::Ref<TaskGraphResource> &p_resource) {
    custom_task_graph_resource_ = p_resource;
}

void DataOrientedLinguistics::set_max_dod_sememes(int p_max) {
    max_dod_sememes_ = std::max(64, p_max);
}

void DataOrientedLinguistics::set_max_dod_lexemes(int p_max) {
    max_dod_lexemes_ = std::max(64, p_max);
}

void DataOrientedLinguistics::set_max_dod_phoneme_csr(int p_max) {
    max_dod_phoneme_csr_ = std::max(256, p_max);
}

void DataOrientedLinguistics::set_max_dod_bpe_vocab(int p_max) {
    max_dod_bpe_vocab_ = std::max(64, p_max);
}

void DataOrientedLinguistics::set_max_dod_synset_relations(int p_max) {
    max_dod_synset_relations_ = std::max(64, p_max);
}

void DataOrientedLinguistics::set_max_dod_semantic_frames(int p_max) {
    max_dod_semantic_frames_ = std::max(16, p_max);
}

void DataOrientedLinguistics::setup_dod_pipeline_with_manager(
    std::shared_ptr<core::MemoryManagerDOD> p_shared_manager,
    std::shared_ptr<core::TaskGraphDOD> p_shared_graph
) {
    _ensure_dod_initialized(std::move(p_shared_manager));
    if (p_shared_graph) {
        active_task_graph_ = std::move(p_shared_graph);
        dod_engine_->set_default_task_graph(active_task_graph_);
    }
    bake_to_dod();
    rebuild_dod_task_graph();
}

void DataOrientedLinguistics::setup_dod_pipeline() {
    setup_dod_pipeline_with_manager(nullptr, nullptr);
}

void DataOrientedLinguistics::bake_to_dod() {
    _ensure_dod_initialized();
    dod_engine_->reset_all_slots();

    if (language_asset_.is_null()) {
        language_asset_.instantiate();
        language_asset_->populate_builtin_godot_and_wizard_vocabulary();
    }

    dod_engine_->bake_from_semantic_lexicon_core(language_asset_->get_unified_core_lexicon());

    // Also append any staged scene object LinguisticProfilePODs
    const int64_t prof_count = staged_profile_pods_.size() / static_cast<int64_t>(sizeof(core::LinguisticProfilePOD));
    if (prof_count > 0) {
        const auto *profs = reinterpret_cast<const core::LinguisticProfilePOD *>(staged_profile_pods_.ptr());
        dod_engine_->stage_object_profiles(std::span<const core::LinguisticProfilePOD>(profs, static_cast<size_t>(prof_count)));
    }
}

void DataOrientedLinguistics::rebuild_dod_task_graph() {
    _ensure_dod_initialized();
    if (!active_task_graph_) return;

    if (dod_execution_mode_ == LING_DOD_EXEC_DISABLED ||
        dod_execution_mode_ == LING_DOD_EXEC_MANUAL_GRAPH) {
        return;
    }

    if (dod_execution_mode_ == LING_DOD_EXEC_CUSTOM_GRAPH_RESOURCE &&
        custom_task_graph_resource_.is_valid()) {
        ui_to_dod_map_.clear();
        custom_task_graph_resource_->compile_to_task_graph(*active_task_graph_, ui_to_dod_map_);
        return;
    }

    dod_engine_->inject_pipeline_stages(
        *active_task_graph_,
        pipeline_stage_mask_,
        auto_connect_pipeline_stages_
    );
}

void DataOrientedLinguistics::execute_dod_step(double p_delta) {
    if (dod_execution_mode_ == LING_DOD_EXEC_DISABLED || !dod_engine_) return;
    dod_engine_->execute_step(p_delta, active_task_graph_.get());
    if (auto_sync_resources_from_dod_) {
        sync_oop_resources_from_dod();
    }
}

void DataOrientedLinguistics::sync_oop_resources_from_dod() {
    if (!dod_engine_ || language_asset_.is_null()) return;
    const godot::TypedArray<LexiconResource> lexicons = language_asset_->get_lexicons();
    if (!lexicons.is_empty()) {
        godot::Ref<LexiconResource> primary_lex = lexicons[0];
        if (primary_lex.is_valid()) {
            dod_engine_->sync_to_semantic_lexicon_core(primary_lex->get_core_lexicon_mut());
        }
    }
}

godot::PackedByteArray DataOrientedLinguistics::bake_objects_to_profile_buffer(
    const godot::Array &p_objects,
    bool p_cache_in_metadata
) {
    staged_profile_pods_ = get_object_profiler()->profile_objects_to_dod_buffer(
        p_objects,
        p_cache_in_metadata
    );
    if (dod_engine_) {
        const int64_t prof_count = staged_profile_pods_.size() / static_cast<int64_t>(sizeof(core::LinguisticProfilePOD));
        if (prof_count > 0) {
            const auto *profs = reinterpret_cast<const core::LinguisticProfilePOD *>(staged_profile_pods_.ptr());
            dod_engine_->stage_object_profiles(std::span<const core::LinguisticProfilePOD>(profs, static_cast<size_t>(prof_count)));
        }
    }
    emit_changed();
    return staged_profile_pods_;
}

int DataOrientedLinguistics::get_buffer_id(const godot::StringName &p_semantic_name) const {
    if (!dod_engine_) return -1;
    const godot::String s(p_semantic_name);
    const uint32_t id = dod_engine_->get_buffer_id_by_name(s.utf8().get_data());
    return (id != core::INVALID_ID) ? static_cast<int>(id) : -1;
}

godot::Dictionary DataOrientedLinguistics::get_buffer_manifest_dictionary() const {
    godot::Dictionary d;
    if (dod_engine_) {
        dod_engine_->populate_task_buffer_dictionary(d, true);
    }
    return d;
}

void DataOrientedLinguistics::set_query_prompt(const godot::String &p_prompt) {
    _ensure_dod_initialized();
    godot::Ref<SemanticPromptParser> parser = get_object_profiler()->get_prompt_parser();
    if (parser.is_null()) {
        parser.instantiate();
    }
    if (language_asset_.is_valid()) {
        parser->set_language_asset(language_asset_);
    }
    set_query_tag_set(parser->parse_prompt_to_tag_set(p_prompt));
}

void DataOrientedLinguistics::set_query_tag_set(const godot::Ref<SemanticTagSetResource> &p_tag_set) {
    _ensure_dod_initialized();
    if (p_tag_set.is_null()) {
        dod_engine_->set_query_context(core::SememeDescriptorPOD{}, core::SememeDescriptorPOD{});
        return;
    }
    core::SememeDescriptorPOD excluded_desc{};
    const godot::PackedStringArray excl_tags = p_tag_set->get_excluded_seme_tags();
    for (int i = 0; i < excl_tags.size(); ++i) {
        excluded_desc.set_nuclear_bit(
            core::SemanticLexiconCore::resolve_canonical_seme_bit(excl_tags[i].utf8().get_data())
        );
    }
    dod_engine_->set_query_context(p_tag_set->get_compiled_descriptor(), excluded_desc);
}

godot::PackedInt32Array DataOrientedLinguistics::get_matched_sememe_indices(float p_min_affinity) const {
    godot::PackedInt32Array out;
    if (!dod_engine_) return out;
    const uint32_t count = dod_engine_->get_active_sememe_count();
    for (uint32_t i = 0; i < count; ++i) {
        if (dod_engine_->get_sememe_affinity_score(i) >= p_min_affinity) {
            out.push_back(static_cast<int32_t>(i));
        }
    }
    return out;
}

godot::PackedFloat32Array DataOrientedLinguistics::get_sememe_affinity_scores() const {
    godot::PackedFloat32Array out;
    if (!dod_engine_) return out;
    const uint32_t count = dod_engine_->get_active_sememe_count();
    out.resize(static_cast<int64_t>(count));
    for (uint32_t i = 0; i < count; ++i) {
        out.set(static_cast<int64_t>(i), dod_engine_->get_sememe_affinity_score(i));
    }
    return out;
}

int DataOrientedLinguistics::queue_noisy_phonetic_utterance(
    const godot::PackedInt32Array &p_noisy_masks,
    int p_expected_sememe_id,
    int p_preferred_lect_id
) {
    _ensure_dod_initialized();
    std::vector<uint16_t> masks(static_cast<size_t>(p_noisy_masks.size()));
    for (int64_t i = 0; i < p_noisy_masks.size(); ++i) {
        masks[static_cast<size_t>(i)] = static_cast<uint16_t>(p_noisy_masks[i] & 0xFFFF);
    }
    const uint32_t slot = dod_engine_->stage_phonetic_utterance(
        masks,
        (p_expected_sememe_id >= 0) ? static_cast<uint32_t>(p_expected_sememe_id) : 0xFFFFFFFFu,
        static_cast<uint16_t>(std::max(0, p_preferred_lect_id))
    );
    return (slot != core::INVALID_ID) ? static_cast<int>(slot) : -1;
}

godot::Dictionary DataOrientedLinguistics::get_decoded_utterance_result(int p_utterance_slot) const {
    godot::Dictionary d;
    if (!dod_engine_ || p_utterance_slot < 0) return d;
    const core::PhoneticUtteranceSpanPOD span = dod_engine_->get_utterance_span(static_cast<uint32_t>(p_utterance_slot));
    d["decoded_lexeme_id"]   = (span.decoded_lexeme_id == 0xFFFFFFFFu) ? -1 : static_cast<int>(span.decoded_lexeme_id);
    d["decoded_sememe_id"]   = (span.decoded_sememe_id == 0xFFFFFFFFu) ? -1 : static_cast<int>(span.decoded_sememe_id);
    d["phonetic_distance"]   = span.phonetic_distance;
    d["honest_misreference"] = (span.honest_misreference != 0u);
    if (language_asset_.is_valid() && span.decoded_lexeme_id != 0xFFFFFFFFu) {
        std::string_view surf = language_asset_->get_unified_core_lexicon().get_lexeme_surface(span.decoded_lexeme_id);
        d["decoded_surface"] = godot::String::utf8(surf.data(), static_cast<int>(surf.size()));
    } else {
        d["decoded_surface"] = godot::String("");
    }
    return d;
}

int DataOrientedLinguistics::queue_frame_clause_evaluation(
    const godot::String &p_frame_name,
    const godot::String &p_agent_token,
    const godot::String &p_patient_token,
    const godot::String &p_instrument_token
) {
    _ensure_dod_initialized();
    if (language_asset_.is_null()) {
        language_asset_.instantiate();
        language_asset_->populate_builtin_godot_and_wizard_vocabulary();
    }
    const auto &core_lex = language_asset_->get_unified_core_lexicon();
    const int32_t fid = core_lex.find_semantic_frame_by_name(p_frame_name.utf8().get_data());
    if (fid < 0) return -1;

    const int32_t ag_id = core_lex.find_sememe_by_label(p_agent_token.utf8().get_data());
    const int32_t pt_id = core_lex.find_sememe_by_label(p_patient_token.utf8().get_data());
    const int32_t in_id = p_instrument_token.is_empty()
        ? -1
        : core_lex.find_sememe_by_label(p_instrument_token.utf8().get_data());

    const uint32_t slot = dod_engine_->stage_frame_clause_instance(
        static_cast<uint32_t>(fid),
        (ag_id >= 0) ? static_cast<uint32_t>(ag_id) : 0xFFFFFFFFu,
        (pt_id >= 0) ? static_cast<uint32_t>(pt_id) : 0xFFFFFFFFu,
        (in_id >= 0) ? static_cast<uint32_t>(in_id) : 0xFFFFFFFFu
    );
    return (slot != core::INVALID_ID) ? static_cast<int>(slot) : -1;
}

godot::Dictionary DataOrientedLinguistics::get_frame_clause_result(int p_clause_slot) const {
    godot::Dictionary d;
    if (!dod_engine_ || p_clause_slot < 0) return d;
    const core::FrameClauseInstancePOD cl = dod_engine_->get_frame_clause_instance(static_cast<uint32_t>(p_clause_slot));
    d["frame_id"]             = static_cast<int>(cl.frame_id);
    d["agent_sememe_id"]      = (cl.agent_sememe_id == 0xFFFFFFFFu) ? -1 : static_cast<int>(cl.agent_sememe_id);
    d["patient_sememe_id"]    = (cl.patient_sememe_id == 0xFFFFFFFFu) ? -1 : static_cast<int>(cl.patient_sememe_id);
    d["instrument_sememe_id"] = (cl.instrument_sememe_id == 0xFFFFFFFFu) ? -1 : static_cast<int>(cl.instrument_sememe_id);
    d["role_fit_score"]       = cl.role_fit_score;
    d["consequent_applied"]   = (cl.consequent_applied != 0u);
    return d;
}

godot::PackedInt32Array DataOrientedLinguistics::get_compiled_slm_allowed_bpe_tokens() const {
    godot::PackedInt32Array out;
    if (!dod_engine_) return out;
    const std::vector<uint32_t> tokens = dod_engine_->read_slm_allowed_bpe_tokens();
    out.resize(static_cast<int64_t>(tokens.size()));
    for (size_t i = 0; i < tokens.size(); ++i) {
        out.set(static_cast<int64_t>(i), static_cast<int32_t>(tokens[i]));
    }
    return out;
}

int DataOrientedLinguistics::add_linguistics_transform_node(
    int p_logic_id,
    const godot::Dictionary &p_properties,
    int p_priority,
    bool p_use_registry_factory
) {
    _ensure_dod_initialized();
    if (!active_task_graph_) return -1;
    core::LinguisticsNodeExecutionConfig cfg{};
    cfg.execution_priority   = p_priority;
    cfg.use_registry_factory = p_use_registry_factory;
    const core::NodeID nid = dod_engine_->add_transform_node_by_id(
        *active_task_graph_,
        static_cast<core::LinguisticsTransformLogicID>(p_logic_id),
        p_properties,
        cfg
    );
    return (nid != core::INVALID_ID) ? static_cast<int>(nid) : -1;
}

int DataOrientedLinguistics::add_linguistics_query_node(
    int p_logic_id,
    int p_query_op,
    const godot::Dictionary &p_properties,
    int p_priority,
    bool p_use_registry_factory
) {
    _ensure_dod_initialized();
    if (!active_task_graph_) return -1;
    core::LinguisticsNodeExecutionConfig cfg{};
    cfg.query_op             = (p_query_op == 1) ? core::QueryOp::ADD : core::QueryOp::CULL;
    cfg.execution_priority   = p_priority;
    cfg.use_registry_factory = p_use_registry_factory;
    const core::NodeID nid = dod_engine_->add_query_node_by_id(
        *active_task_graph_,
        static_cast<core::LinguisticsQueryLogicID>(p_logic_id),
        p_properties,
        cfg
    );
    return (nid != core::INVALID_ID) ? static_cast<int>(nid) : -1;
}

int DataOrientedLinguistics::add_linguistics_metadata_node(
    int p_logic_id,
    const godot::Dictionary &p_properties,
    int p_priority,
    bool p_use_registry_factory
) {
    _ensure_dod_initialized();
    if (!active_task_graph_) return -1;
    core::LinguisticsNodeExecutionConfig cfg{};
    cfg.execution_priority   = p_priority;
    cfg.use_registry_factory = p_use_registry_factory;
    const core::NodeID nid = dod_engine_->add_metadata_node_by_id(
        *active_task_graph_,
        static_cast<core::LinguisticsMetadataLogicID>(p_logic_id),
        p_properties,
        cfg
    );
    return (nid != core::INVALID_ID) ? static_cast<int>(nid) : -1;
}

bool DataOrientedLinguistics::schedule_node_with_stage(int p_stage_flag, int p_custom_node_id, bool p_join_downstream) {
    if (!dod_engine_ || !active_task_graph_ || p_custom_node_id < 0) return false;
    return core::LinguisticsDOD::schedule_node_with_stage(
        *active_task_graph_,
        dod_engine_->get_active_topology(),
        static_cast<core::LinguisticsPipelineStageFlags>(p_stage_flag),
        static_cast<core::NodeID>(p_custom_node_id),
        p_join_downstream
    );
}

bool DataOrientedLinguistics::schedule_node_between_stages(int p_upstream_stage_flag, int p_downstream_stage_flag, int p_custom_node_id) {
    if (!dod_engine_ || !active_task_graph_ || p_custom_node_id < 0) return false;
    return core::LinguisticsDOD::schedule_node_between_stages(
        *active_task_graph_,
        dod_engine_->get_active_topology(),
        static_cast<core::LinguisticsPipelineStageFlags>(p_upstream_stage_flag),
        static_cast<core::LinguisticsPipelineStageFlags>(p_downstream_stage_flag),
        static_cast<core::NodeID>(p_custom_node_id)
    );
}

} // namespace ideam::godot_ext
