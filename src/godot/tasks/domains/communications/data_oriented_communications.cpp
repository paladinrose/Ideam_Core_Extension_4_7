#include "data_oriented_communications.h"
#include <algorithm>
#include <vector>

namespace ideam::godot_ext {

DataOrientedCommunications::DataOrientedCommunications()
    : dod_engine_(std::make_unique<core::CommunicationsDOD>()) {}

void DataOrientedCommunications::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_memory_manager_resource", "memory_manager"), &DataOrientedCommunications::set_memory_manager_resource);
    ClassDB::bind_method(D_METHOD("get_memory_manager_resource"), &DataOrientedCommunications::get_memory_manager_resource);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "memory_manager_resource", PROPERTY_HINT_RESOURCE_TYPE, "MemoryManagerResource"),
        "set_memory_manager_resource",
        "get_memory_manager_resource"
    );

    ClassDB::bind_method(D_METHOD("set_task_graph_resource", "task_graph"), &DataOrientedCommunications::set_task_graph_resource);
    ClassDB::bind_method(D_METHOD("get_task_graph_resource"), &DataOrientedCommunications::get_task_graph_resource);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "task_graph_resource", PROPERTY_HINT_RESOURCE_TYPE, "TaskGraphResource"),
        "set_task_graph_resource",
        "get_task_graph_resource"
    );

    ClassDB::bind_method(D_METHOD("set_default_medium", "medium"), &DataOrientedCommunications::set_default_medium);
    ClassDB::bind_method(D_METHOD("get_default_medium"), &DataOrientedCommunications::get_default_medium);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "default_medium", PROPERTY_HINT_RESOURCE_TYPE, "CommunicationMediumResource"),
        "set_default_medium",
        "get_default_medium"
    );

    ClassDB::bind_method(D_METHOD("set_active_discourse", "discourse"), &DataOrientedCommunications::set_active_discourse);
    ClassDB::bind_method(D_METHOD("get_active_discourse"), &DataOrientedCommunications::get_active_discourse);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "active_discourse", PROPERTY_HINT_RESOURCE_TYPE, "DiscourseResource"),
        "set_active_discourse",
        "get_active_discourse"
    );

    ClassDB::bind_method(D_METHOD("set_language_asset", "language_asset"), &DataOrientedCommunications::set_language_asset);
    ClassDB::bind_method(D_METHOD("get_language_asset"), &DataOrientedCommunications::get_language_asset);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "language_asset", PROPERTY_HINT_RESOURCE_TYPE, "LanguageAssetResource"),
        "set_language_asset",
        "get_language_asset"
    );

    ClassDB::bind_method(D_METHOD("set_max_signal_packets", "max_packets"), &DataOrientedCommunications::set_max_signal_packets);
    ClassDB::bind_method(D_METHOD("get_max_signal_packets"), &DataOrientedCommunications::get_max_signal_packets);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_signal_packets", PROPERTY_HINT_RANGE, "16,65536,16"), "set_max_signal_packets", "get_max_signal_packets");

    ClassDB::bind_method(D_METHOD("set_max_phoneme_stream_len", "max_len"), &DataOrientedCommunications::set_max_phoneme_stream_len);
    ClassDB::bind_method(D_METHOD("get_max_phoneme_stream_len"), &DataOrientedCommunications::get_max_phoneme_stream_len);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_phoneme_stream_len", PROPERTY_HINT_RANGE, "256,262144,256"), "set_max_phoneme_stream_len", "get_max_phoneme_stream_len");

    ClassDB::bind_method(D_METHOD("set_min_audible_snr_db", "snr_db"), &DataOrientedCommunications::set_min_audible_snr_db);
    ClassDB::bind_method(D_METHOD("get_min_audible_snr_db"), &DataOrientedCommunications::get_min_audible_snr_db);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "min_audible_snr_db", PROPERTY_HINT_RANGE, "-40.0,40.0,0.5"), "set_min_audible_snr_db", "get_min_audible_snr_db");

    ClassDB::bind_method(D_METHOD("set_unintelligible_ceiling", "ceiling"), &DataOrientedCommunications::set_unintelligible_ceiling);
    ClassDB::bind_method(D_METHOD("get_unintelligible_ceiling"), &DataOrientedCommunications::get_unintelligible_ceiling);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "unintelligible_ceiling", PROPERTY_HINT_RANGE, "0.05,1.0,0.01"), "set_unintelligible_ceiling", "get_unintelligible_ceiling");

    ClassDB::bind_method(D_METHOD("initialize_dod_buffers"), &DataOrientedCommunications::initialize_dod_buffers);
    ClassDB::bind_method(D_METHOD("stage_resources_to_dod"), &DataOrientedCommunications::stage_resources_to_dod);
    ClassDB::bind_method(
        D_METHOD("execute_pipeline_direct", "stage_mask"),
        &DataOrientedCommunications::execute_pipeline_direct,
        DEFVAL(static_cast<int>(core::COMM_STAGE_DEFAULT_PIPELINE_MASK))
    );
    ClassDB::bind_method(D_METHOD("read_epistemic_handoffs_as_dicts"), &DataOrientedCommunications::read_epistemic_handoffs_as_dicts);
    ClassDB::bind_method(D_METHOD("read_signal_packets_as_dicts"), &DataOrientedCommunications::read_signal_packets_as_dicts);
}

void DataOrientedCommunications::set_memory_manager_resource(const godot::Ref<MemoryManagerResource> &p_mem) {
    memory_manager_resource_ = p_mem;
    if (dod_engine_ && memory_manager_resource_.is_valid()) {
        dod_engine_->bind_execution_context(
            memory_manager_resource_->get_dod_manager(),
            task_graph_resource_.is_valid() ? task_graph_resource_->get_dod_graph() : nullptr
        );
    }
    emit_changed();
}

void DataOrientedCommunications::set_task_graph_resource(const godot::Ref<TaskGraphResource> &p_graph) {
    task_graph_resource_ = p_graph;
    if (dod_engine_ && task_graph_resource_.is_valid()) {
        dod_engine_->bind_execution_context(
            memory_manager_resource_.is_valid() ? memory_manager_resource_->get_dod_manager() : nullptr,
            task_graph_resource_->get_dod_graph()
        );
    }
    emit_changed();
}

void DataOrientedCommunications::set_max_signal_packets(int p_max) {
    max_signal_packets_ = std::max(16, p_max);
    if (dod_engine_) {
        auto caps = dod_engine_->get_capacities();
        caps.max_signal_packets = max_signal_packets_;
        dod_engine_->set_capacities(caps);
    }
    emit_changed();
}

void DataOrientedCommunications::set_max_phoneme_stream_len(int p_max) {
    max_phoneme_stream_len_ = std::max(256, p_max);
    if (dod_engine_) {
        auto caps = dod_engine_->get_capacities();
        caps.max_phoneme_stream_len = max_phoneme_stream_len_;
        dod_engine_->set_capacities(caps);
    }
    emit_changed();
}

void DataOrientedCommunications::set_min_audible_snr_db(float p_db) {
    min_audible_snr_db_ = p_db;
    if (dod_engine_) dod_engine_->set_min_audible_snr_db(p_db);
    emit_changed();
}

void DataOrientedCommunications::set_unintelligible_ceiling(float p_val) {
    unintelligible_ceiling_ = std::clamp(p_val, 0.05f, 1.0f);
    if (dod_engine_) dod_engine_->set_unintelligible_distance_ceiling(unintelligible_ceiling_);
    emit_changed();
}

bool DataOrientedCommunications::initialize_dod_buffers() {
    if (!dod_engine_ || memory_manager_resource_.is_null()) return false;
    return dod_engine_->initialize_communications_memory(memory_manager_resource_->get_dod_manager());
}

bool DataOrientedCommunications::stage_resources_to_dod() {
    if (!dod_engine_) return false;
    if (!dod_engine_->get_manifest().has(core::CommunicationsBufferSlot::COMM_SIGNAL_PACKET_POD)) {
        if (!initialize_dod_buffers()) return false;
    }

    if (default_medium_.is_valid()) {
        const core::TransmissionMediumPOD med = default_medium_->to_pod();
        dod_engine_->stage_mediums(std::span<const core::TransmissionMediumPOD>(&med, 1));
    }

    if (language_asset_.is_valid() && language_asset_->get_lexicon().is_valid()) {
        dod_engine_->stage_lexicon_reference_tables(language_asset_->get_lexicon()->get_core_lexicon());
    }

    if (active_discourse_.is_valid()) {
        if (active_discourse_->get_medium().is_valid()) {
            const core::TransmissionMediumPOD med = active_discourse_->get_medium()->to_pod();
            dod_engine_->stage_mediums(std::span<const core::TransmissionMediumPOD>(&med, 1));
        }
        const core::DiscourseTurnStatePOD turn = active_discourse_->to_turn_state_pod();
        dod_engine_->stage_discourse_turns(std::span<const core::DiscourseTurnStatePOD>(&turn, 1));

        std::vector<core::DiscourseParticipantPOD> parts;
        active_discourse_->extract_participant_pods(parts);
        if (!parts.empty()) {
            dod_engine_->stage_discourse_participants(parts);
        }
    }
    return true;
}

bool DataOrientedCommunications::execute_pipeline_direct(int p_stage_mask) {
    if (!dod_engine_) return false;
    return dod_engine_->execute_full_pipeline_direct(static_cast<uint32_t>(p_stage_mask));
}

godot::Array DataOrientedCommunications::read_epistemic_handoffs_as_dicts() const {
    godot::Array out;
    if (!dod_engine_) return out;
    std::vector<core::EpistemicHandoffPOD> handoffs;
    dod_engine_->read_epistemic_handoffs(handoffs);
    for (const core::EpistemicHandoffPOD &ho : handoffs) {
        godot::Dictionary d;
        d["packet_id"]              = static_cast<int>(ho.packet_id);
        d["receiver_mind_agent_id"] = static_cast<int>(ho.receiver_mind_agent_id);
        d["source_fact_id"]         = static_cast<int>(ho.source_fact_id);
        d["decoded_fact_id"]        = static_cast<int>(ho.decoded_fact_id);
        d["decoded_sememe_id"]      = static_cast<int>(ho.decoded_sememe_id);
        d["effective_snr_db"]       = ho.effective_snr_db;
        d["decoded_confidence"]     = ho.decoded_confidence;
        d["tome_id"]                = static_cast<int>(ho.tome_id);
        d["honest_misreference"]    = (ho.honest_misreference != 0u);
        d["garbled_syllable_count"] = static_cast<int>(ho.garbled_syllable_count);
        out.push_back(d);
    }
    return out;
}

godot::Array DataOrientedCommunications::read_signal_packets_as_dicts() const {
    godot::Array out;
    if (!dod_engine_) return out;
    std::vector<core::SignalPacketPOD> packets;
    dod_engine_->read_signal_packets(packets);
    for (const core::SignalPacketPOD &pkt : packets) {
        godot::Dictionary d;
        d["packet_id"]              = static_cast<int>(pkt.packet_id);
        d["transmitter_id"]         = static_cast<int>(pkt.transmitter_id);
        d["receiver_id"]            = static_cast<int>(pkt.receiver_id);
        d["medium_id"]              = static_cast<int>(pkt.medium_id);
        d["source_fact_id"]         = static_cast<int>(pkt.source_fact_id);
        d["decoded_fact_id"]        = static_cast<int>(pkt.decoded_fact_id);
        d["effective_snr_db"]       = pkt.effective_snr_db;
        d["noise_tier"]             = static_cast<int>(pkt.noise_tier);
        d["garbled_syllable_count"] = static_cast<int>(pkt.garbled_syllable_count);
        d["honest_misreference"]    = (pkt.honest_misreference != 0u);
        d["intended_sememe_id"]     = static_cast<int>(pkt.intended_sememe_id);
        d["decoded_sememe_id"]      = static_cast<int>(pkt.decoded_sememe_id);
        out.push_back(d);
    }
    return out;
}

} // namespace ideam::godot_ext
