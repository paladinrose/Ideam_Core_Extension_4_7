#include "communications_dod.h"
#include <algorithm>
#include <cstring>

namespace ideam::core {

namespace {

template <typename T_Elem>
bool copy_span_to_buffer(MemoryManagerDOD *p_mem, uint32_t p_buf_id, std::span<const T_Elem> p_src) {
    if (!p_mem || p_buf_id == INVALID_ID) return false;
    MemoryBufferPOD *buf = p_mem->get_buffer(p_buf_id);
    if (!buf || !buf->master_block_ptr) return false;
    const size_t max_elems = static_cast<size_t>(std::max<int64_t>(0, buf->max_elements));
    const size_t count     = std::min<size_t>(p_src.size(), max_elems);
    if (count > 0) {
        std::memcpy(buf->master_block_ptr + buf->memory_offset, p_src.data(), count * sizeof(T_Elem));
    }
    buf->current_count = static_cast<int64_t>(count);
    return true;
}

template <typename T_Elem>
void read_buffer_to_vector(const MemoryManagerDOD *p_mem, uint32_t p_buf_id, std::vector<T_Elem> &r_out) {
    r_out.clear();
    if (!p_mem || p_buf_id == INVALID_ID) return;
    const MemoryBufferPOD *buf = const_cast<MemoryManagerDOD *>(p_mem)->get_buffer(p_buf_id);
    if (!buf || !buf->master_block_ptr || buf->current_count <= 0) return;
    const size_t count = static_cast<size_t>(std::min<int64_t>(buf->current_count, buf->max_elements));
    r_out.resize(count);
    std::memcpy(r_out.data(), buf->master_block_ptr + buf->memory_offset, count * sizeof(T_Elem));
}

} // namespace

bool CommunicationsDOD::initialize_communications_memory(MemoryManagerDOD *p_mem) {
    if (!p_mem) return false;
    memory_manager_ = p_mem;
    manifest_.reset();

    for (size_t i = 0; i < COMMUNICATIONS_BUFFER_SPECS.size(); ++i) {
        const CommunicationsBufferSpec &spec = COMMUNICATIONS_BUFFER_SPECS[i];
        const int64_t cap = capacities_.resolve(spec.capacity_kind);
        const size_t safe_cap = static_cast<size_t>(std::max<int64_t>(1, cap));
        const size_t total_bytes = safe_cap * static_cast<size_t>(spec.element_bytes);
        const uint32_t buf_id = p_mem->create_buffer(BufferLayoutType::FLAT, total_bytes, 64u);
        if (MemoryBufferPOD *buf = p_mem->get_buffer(buf_id)) {
            buf->max_elements   = static_cast<int64_t>(safe_cap);
            buf->current_count  = 0;
            buf->element_stride = static_cast<uint32_t>(spec.element_bytes);
        }
        manifest_.set(spec.slot, buf_id);
    }
    return manifest_.has(CommunicationsBufferSlot::COMM_SIGNAL_PACKET_POD);
}

bool CommunicationsDOD::stage_transmitters(std::span<const TransmitterStatePOD> p_transmitters) {
    return copy_span_to_buffer(memory_manager_, get_buffer_id(CommunicationsBufferSlot::COMM_TRANSMITTER_STATE_POD), p_transmitters);
}

bool CommunicationsDOD::stage_mediums(std::span<const TransmissionMediumPOD> p_mediums) {
    return copy_span_to_buffer(memory_manager_, get_buffer_id(CommunicationsBufferSlot::COMM_TRANSMISSION_MEDIUM_POD), p_mediums);
}

bool CommunicationsDOD::stage_receivers(std::span<const ReceiverStatePOD> p_receivers) {
    return copy_span_to_buffer(memory_manager_, get_buffer_id(CommunicationsBufferSlot::COMM_RECEIVER_STATE_POD), p_receivers);
}

bool CommunicationsDOD::stage_signal_packets(std::span<const SignalPacketPOD> p_packets) {
    const bool ok = copy_span_to_buffer(memory_manager_, get_buffer_id(CommunicationsBufferSlot::COMM_SIGNAL_PACKET_POD), p_packets);
    if (ok && memory_manager_) {
        if (MemoryBufferPOD *ho_buf = memory_manager_->get_buffer(get_buffer_id(CommunicationsBufferSlot::COMM_EPISTEMIC_HANDOFF_POD))) {
            ho_buf->current_count = static_cast<int64_t>(p_packets.size());
        }
        if (MemoryBufferPOD *snr_buf = memory_manager_->get_buffer(get_buffer_id(CommunicationsBufferSlot::COMM_EFFECTIVE_SNR_DB))) {
            snr_buf->current_count = static_cast<int64_t>(p_packets.size());
        }
        if (MemoryBufferPOD *tier_buf = memory_manager_->get_buffer(get_buffer_id(CommunicationsBufferSlot::COMM_NOISE_TIER_MASK))) {
            tier_buf->current_count = static_cast<int64_t>(p_packets.size());
        }
    }
    return ok;
}

bool CommunicationsDOD::stage_phoneme_stream(std::span<const uint16_t> p_phoneme_masks) {
    return copy_span_to_buffer(memory_manager_, get_buffer_id(CommunicationsBufferSlot::COMM_PHONEME_FEATURE_STREAM), p_phoneme_masks);
}

bool CommunicationsDOD::stage_discourse_participants(std::span<const DiscourseParticipantPOD> p_participants) {
    return copy_span_to_buffer(memory_manager_, get_buffer_id(CommunicationsBufferSlot::COMM_DISCOURSE_PARTICIPANT_POD), p_participants);
}

bool CommunicationsDOD::stage_discourse_turns(std::span<const DiscourseTurnStatePOD> p_turns) {
    return copy_span_to_buffer(memory_manager_, get_buffer_id(CommunicationsBufferSlot::COMM_DISCOURSE_TURN_STATE_POD), p_turns);
}

bool CommunicationsDOD::stage_prosody_samples(std::span<const ProsodySamplePOD> p_samples) {
    return copy_span_to_buffer(memory_manager_, get_buffer_id(CommunicationsBufferSlot::COMM_PROSODY_SAMPLE_POD), p_samples);
}

bool CommunicationsDOD::stage_lexicon_reference_tables(const SemanticLexiconCore &p_lexicon) {
    const auto &lexemes = p_lexicon.lexemes();
    const auto &masks   = p_lexicon.lexeme_phoneme_feature_csr();
    const bool ok1 = copy_span_to_buffer(memory_manager_, get_buffer_id(CommunicationsBufferSlot::COMM_LEXEME_POD_TABLE), std::span<const LexemePOD>(lexemes));
    const bool ok2 = copy_span_to_buffer(memory_manager_, get_buffer_id(CommunicationsBufferSlot::COMM_LEXEME_PHONEME_CSR_MASKS), std::span<const uint16_t>(masks));
    return ok1 && ok2;
}

bool CommunicationsDOD::execute_full_pipeline_direct(uint32_t p_stage_mask) {
    if (!memory_manager_) return false;

    TaskContextPOD ctx{};
    ctx.manager = memory_manager_;
    SingleElementView<uint32_t, FlatStrategy> dummy_view{};

    // Wave 1: COM1 DiscourseTurnAllocationMetadataLogic
    if ((p_stage_mask & COMM_STAGE_WAVE_1_DISCOURSE_TURN_ALLOCATION) != 0u) {
        DiscourseTurnAllocationMetadataLogic<> com1{};
        com1.target_buffer_id                = get_buffer_id(CommunicationsBufferSlot::COMM_DISCOURSE_TURN_STATE_POD);
        com1.discourse_turn_state_buffer_id  = get_buffer_id(CommunicationsBufferSlot::COMM_DISCOURSE_TURN_STATE_POD);
        com1.discourse_participant_buffer_id = get_buffer_id(CommunicationsBufferSlot::COMM_DISCOURSE_PARTICIPANT_POD);
        com1.signal_packet_buffer_id         = get_buffer_id(CommunicationsBufferSlot::COMM_SIGNAL_PACKET_POD);
        com1.execute_metadata<SingleElementView<uint32_t, FlatStrategy>, FlatStrategy>(ctx, dummy_view);
    }

    // Wave 2: COM2 ChannelSNRAudibilityQueryLogic
    if ((p_stage_mask & COMM_STAGE_WAVE_2_CHANNEL_SNR_AUDIBILITY_QUERY) != 0u) {
        ChannelSNRAudibilityQueryLogic<> com2{};
        com2.target_buffer_id              = get_buffer_id(CommunicationsBufferSlot::COMM_SIGNAL_PACKET_POD);
        com2.transmitter_state_buffer_id   = get_buffer_id(CommunicationsBufferSlot::COMM_TRANSMITTER_STATE_POD);
        com2.transmission_medium_buffer_id = get_buffer_id(CommunicationsBufferSlot::COMM_TRANSMISSION_MEDIUM_POD);
        com2.receiver_state_buffer_id      = get_buffer_id(CommunicationsBufferSlot::COMM_RECEIVER_STATE_POD);
        com2.effective_snr_out_buffer_id   = get_buffer_id(CommunicationsBufferSlot::COMM_EFFECTIVE_SNR_DB);
        com2.noise_tier_out_buffer_id      = get_buffer_id(CommunicationsBufferSlot::COMM_NOISE_TIER_MASK);
        com2.min_audible_snr_db            = min_audible_snr_db_;
        com2.execute_query<QueryOp::CULL, SingleElementView<uint32_t, FlatStrategy>, FlatStrategy>(ctx, dummy_view);
    }

    // Wave 3: COM3 NoisyMediumPhonemeDegradeTransformLogic
    if ((p_stage_mask & COMM_STAGE_WAVE_3_NOISY_MEDIUM_PHONEME_DEGRADE) != 0u) {
        NoisyMediumPhonemeDegradeTransformLogic<> com3{};
        com3.target_buffer_id              = get_buffer_id(CommunicationsBufferSlot::COMM_PHONEME_FEATURE_STREAM);
        com3.signal_packet_buffer_id       = get_buffer_id(CommunicationsBufferSlot::COMM_SIGNAL_PACKET_POD);
        com3.phoneme_feature_stream_id     = get_buffer_id(CommunicationsBufferSlot::COMM_PHONEME_FEATURE_STREAM);
        com3.transmission_medium_buffer_id = get_buffer_id(CommunicationsBufferSlot::COMM_TRANSMISSION_MEDIUM_POD);
        com3.prosody_sample_buffer_id      = get_buffer_id(CommunicationsBufferSlot::COMM_PROSODY_SAMPLE_POD);
        com3.random_seed                   = random_seed_;
        com3.execute_transform<SingleElementView<uint32_t, FlatStrategy>, FlatStrategy>(ctx, dummy_view);
    }

    // Wave 4: COM4 ReceiverEpistemicHandoffTransformLogic
    if ((p_stage_mask & COMM_STAGE_WAVE_4_RECEIVER_DECODE_AND_MIND_HANDOFF) != 0u) {
        ReceiverEpistemicHandoffTransformLogic<> com4{};
        com4.target_buffer_id                   = get_buffer_id(CommunicationsBufferSlot::COMM_EPISTEMIC_HANDOFF_POD);
        com4.signal_packet_buffer_id            = get_buffer_id(CommunicationsBufferSlot::COMM_SIGNAL_PACKET_POD);
        com4.phoneme_feature_stream_id          = get_buffer_id(CommunicationsBufferSlot::COMM_PHONEME_FEATURE_STREAM);
        com4.receiver_state_buffer_id           = get_buffer_id(CommunicationsBufferSlot::COMM_RECEIVER_STATE_POD);
        com4.lexeme_pod_buffer_id               = get_buffer_id(CommunicationsBufferSlot::COMM_LEXEME_POD_TABLE);
        com4.lexeme_phoneme_csr_masks_buffer_id = get_buffer_id(CommunicationsBufferSlot::COMM_LEXEME_PHONEME_CSR_MASKS);
        com4.decoded_sememe_stream_buffer_id    = get_buffer_id(CommunicationsBufferSlot::COMM_DECODED_SEMEME_STREAM);
        com4.unintelligible_distance_ceiling    = unintelligible_distance_ceiling_;
        com4.expectation_semantic_bias          = expectation_semantic_bias_;
        com4.execute_transform<SingleElementView<uint32_t, FlatStrategy>, FlatStrategy>(ctx, dummy_view);
    }

    return true;
}

void CommunicationsDOD::read_signal_packets(std::vector<SignalPacketPOD> &r_out) const {
    read_buffer_to_vector(memory_manager_, get_buffer_id(CommunicationsBufferSlot::COMM_SIGNAL_PACKET_POD), r_out);
}

void CommunicationsDOD::read_degraded_phoneme_stream(std::vector<uint16_t> &r_out) const {
    read_buffer_to_vector(memory_manager_, get_buffer_id(CommunicationsBufferSlot::COMM_PHONEME_FEATURE_STREAM), r_out);
}

void CommunicationsDOD::read_epistemic_handoffs(std::vector<EpistemicHandoffPOD> &r_out) const {
    read_buffer_to_vector(memory_manager_, get_buffer_id(CommunicationsBufferSlot::COMM_EPISTEMIC_HANDOFF_POD), r_out);
}

void CommunicationsDOD::read_discourse_turns(std::vector<DiscourseTurnStatePOD> &r_out) const {
    read_buffer_to_vector(memory_manager_, get_buffer_id(CommunicationsBufferSlot::COMM_DISCOURSE_TURN_STATE_POD), r_out);
}

} // namespace ideam::core
