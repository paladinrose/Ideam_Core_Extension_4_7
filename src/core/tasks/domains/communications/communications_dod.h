#pragma once

/**
 * communications_dod.h
 * DOD Orchestrator for `IdeamCommunications` (`src/core/tasks/domains/communications/`).
 *
 * Manages the 15-slot `CommunicationsBufferManifest`, stages Transmitter/Medium/Receiver,
 * Discourse Turn, and SignalPacket/Phoneme streams into `MemoryManagerDOD`, and executes
 * the 4-wave `IdeamCommunications` pipeline (`COM1`–`COM4`).
 */

#include "communications_dod_common.h"
#include "metadata_logic/discourse_turn_allocation_metadata_logic.h"
#include "query_logic/channel_snr_audibility_query_logic.h"
#include "transform_logic/noisy_medium_phoneme_degrade_transform_logic.h"
#include "transform_logic/receiver_epistemic_handoff_transform_logic.h"
#include <span>
#include <vector>

namespace ideam::core {

class CommunicationsDOD {
private:
    MemoryManagerDOD *memory_manager_ = nullptr;
    TaskGraphDOD *task_graph_         = nullptr;

    CommunicationsBufferCapacities capacities_{};
    CommunicationsBufferManifest   manifest_{};
    CommunicationsPipelineTopology topology_{};
    CommunicationsNodeExecutionConfig exec_config_{};

    float    min_audible_snr_db_              = -5.0f;
    float    unintelligible_distance_ceiling_ = 0.55f;
    float    expectation_semantic_bias_       = 0.12f;
    uint64_t random_seed_                     = 0x9E3779B97F4A7C15ULL;

public:
    CommunicationsDOD() = default;
    ~CommunicationsDOD() = default;

    void bind_execution_context(MemoryManagerDOD *p_mem, TaskGraphDOD *p_graph) noexcept {
        memory_manager_ = p_mem;
        task_graph_     = p_graph;
    }

    [[nodiscard]] MemoryManagerDOD *memory_manager() const noexcept { return memory_manager_; }
    [[nodiscard]] TaskGraphDOD *task_graph() const noexcept { return task_graph_; }

    void set_capacities(const CommunicationsBufferCapacities &p_caps) noexcept { capacities_ = p_caps; }
    [[nodiscard]] const CommunicationsBufferCapacities &get_capacities() const noexcept { return capacities_; }

    [[nodiscard]] const CommunicationsBufferManifest &get_manifest() const noexcept { return manifest_; }
    [[nodiscard]] CommunicationsBufferManifest &get_manifest_mut() noexcept { return manifest_; }

    void set_execution_config(const CommunicationsNodeExecutionConfig &p_cfg) noexcept { exec_config_ = p_cfg; }
    [[nodiscard]] const CommunicationsNodeExecutionConfig &get_execution_config() const noexcept { return exec_config_; }

    void set_min_audible_snr_db(float p_db) noexcept { min_audible_snr_db_ = p_db; }
    [[nodiscard]] float get_min_audible_snr_db() const noexcept { return min_audible_snr_db_; }

    void set_unintelligible_distance_ceiling(float p_val) noexcept { unintelligible_distance_ceiling_ = p_val; }
    [[nodiscard]] float get_unintelligible_distance_ceiling() const noexcept { return unintelligible_distance_ceiling_; }

    void set_expectation_semantic_bias(float p_val) noexcept { expectation_semantic_bias_ = p_val; }
    [[nodiscard]] float get_expectation_semantic_bias() const noexcept { return expectation_semantic_bias_; }

    void set_random_seed(uint64_t p_seed) noexcept { random_seed_ = p_seed; }
    [[nodiscard]] uint64_t get_random_seed() const noexcept { return random_seed_; }

    // --- Buffer Initialization & Staging ---
    bool initialize_communications_memory(MemoryManagerDOD *p_mem);
    [[nodiscard]] uint32_t get_buffer_id(CommunicationsBufferSlot p_slot) const noexcept {
        return manifest_.get(p_slot);
    }

    bool stage_transmitters(std::span<const TransmitterStatePOD> p_transmitters);
    bool stage_mediums(std::span<const TransmissionMediumPOD> p_mediums);
    bool stage_receivers(std::span<const ReceiverStatePOD> p_receivers);
    bool stage_signal_packets(std::span<const SignalPacketPOD> p_packets);
    bool stage_phoneme_stream(std::span<const uint16_t> p_phoneme_masks);
    bool stage_discourse_participants(std::span<const DiscourseParticipantPOD> p_participants);
    bool stage_discourse_turns(std::span<const DiscourseTurnStatePOD> p_turns);
    bool stage_prosody_samples(std::span<const ProsodySamplePOD> p_samples);
    bool stage_lexicon_reference_tables(const SemanticLexiconCore &p_lexicon);

    // --- Direct Synchronous & TaskGraph Execution ---
    bool execute_full_pipeline_direct(uint32_t p_stage_mask = COMM_STAGE_DEFAULT_PIPELINE_MASK);

    // --- Readback Helpers ---
    void read_signal_packets(std::vector<SignalPacketPOD> &r_out) const;
    void read_degraded_phoneme_stream(std::vector<uint16_t> &r_out) const;
    void read_epistemic_handoffs(std::vector<EpistemicHandoffPOD> &r_out) const;
    void read_discourse_turns(std::vector<DiscourseTurnStatePOD> &r_out) const;
};

} // namespace ideam::core
