#pragma once

/**
 * data_oriented_communications.h
 * Godot `Resource` bridge for `CommunicationsDOD` (`src/godot/tasks/domains/communications/`).
 *
 * Bridges Godot resources (`CommunicationMediumResource`, `UtteranceResource`,
 * `DiscourseResource`, `LanguageAssetResource`) into the 15-slot `CommunicationsDOD`
 * buffer arena and executes the 4-wave Shannon-Weaver + Sacks-Schegloff-Jefferson
 * DOD pipeline (`COM1`–`COM4`).
 */

#include "../../../../core/tasks/domains/communications/communications_dod.h"
#include "../../../communications/transceiver_nodes.h"
#include "../../../memory/memory_manager_resource.h"
#include "../../../tasks/task_graph_resource.h"
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <memory>

namespace ideam::godot_ext {

class DataOrientedCommunications : public godot::Resource {
    GDCLASS(DataOrientedCommunications, godot::Resource)

private:
    std::unique_ptr<core::CommunicationsDOD> dod_engine_;

    godot::Ref<MemoryManagerResource> memory_manager_resource_;
    godot::Ref<TaskGraphResource> task_graph_resource_;
    godot::Ref<CommunicationMediumResource> default_medium_;
    godot::Ref<DiscourseResource> active_discourse_;
    godot::Ref<LanguageAssetResource> language_asset_;

    int max_signal_packets_         = 512;
    int max_phoneme_stream_len_     = 16384;
    int max_transmitters_           = 256;
    int max_receivers_              = 256;
    float min_audible_snr_db_       = -5.0f;
    float unintelligible_ceiling_   = 0.55f;

protected:
    static void _bind_methods();

public:
    DataOrientedCommunications();
    ~DataOrientedCommunications() override = default;

    [[nodiscard]] core::CommunicationsDOD *get_dod_engine() const noexcept { return dod_engine_.get(); }

    void set_memory_manager_resource(const godot::Ref<MemoryManagerResource> &p_mem);
    [[nodiscard]] godot::Ref<MemoryManagerResource> get_memory_manager_resource() const { return memory_manager_resource_; }

    void set_task_graph_resource(const godot::Ref<TaskGraphResource> &p_graph);
    [[nodiscard]] godot::Ref<TaskGraphResource> get_task_graph_resource() const { return task_graph_resource_; }

    void set_default_medium(const godot::Ref<CommunicationMediumResource> &p_med) { default_medium_ = p_med; emit_changed(); }
    [[nodiscard]] godot::Ref<CommunicationMediumResource> get_default_medium() const { return default_medium_; }

    void set_active_discourse(const godot::Ref<DiscourseResource> &p_disc) { active_discourse_ = p_disc; emit_changed(); }
    [[nodiscard]] godot::Ref<DiscourseResource> get_active_discourse() const { return active_discourse_; }

    void set_language_asset(const godot::Ref<LanguageAssetResource> &p_asset) { language_asset_ = p_asset; emit_changed(); }
    [[nodiscard]] godot::Ref<LanguageAssetResource> get_language_asset() const { return language_asset_; }

    void set_max_signal_packets(int p_max);
    [[nodiscard]] int get_max_signal_packets() const noexcept { return max_signal_packets_; }

    void set_max_phoneme_stream_len(int p_max);
    [[nodiscard]] int get_max_phoneme_stream_len() const noexcept { return max_phoneme_stream_len_; }

    void set_min_audible_snr_db(float p_db);
    [[nodiscard]] float get_min_audible_snr_db() const noexcept { return min_audible_snr_db_; }

    void set_unintelligible_ceiling(float p_val);
    [[nodiscard]] float get_unintelligible_ceiling() const noexcept { return unintelligible_ceiling_; }

    // --- DOD Arena Setup, Staging & Execution ---
    bool initialize_dod_buffers();
    bool stage_resources_to_dod();
    bool execute_pipeline_direct(int p_stage_mask = static_cast<int>(core::COMM_STAGE_DEFAULT_PIPELINE_MASK));

    [[nodiscard]] godot::Array read_epistemic_handoffs_as_dicts() const;
    [[nodiscard]] godot::Array read_signal_packets_as_dicts() const;
};

} // namespace ideam::godot_ext
