#include "transceiver_nodes.h"
#include <algorithm>
#include <cmath>

namespace ideam::godot_ext {

// ============================================================================
// CommunicationTransmitterNode
// ============================================================================

void CommunicationTransmitterNode::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_transmitter_id", "id"), &CommunicationTransmitterNode::set_transmitter_id);
    ClassDB::bind_method(D_METHOD("get_transmitter_id"), &CommunicationTransmitterNode::get_transmitter_id);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "transmitter_id"), "set_transmitter_id", "get_transmitter_id");

    ClassDB::bind_method(D_METHOD("set_mind_agent_id", "id"), &CommunicationTransmitterNode::set_mind_agent_id);
    ClassDB::bind_method(D_METHOD("get_mind_agent_id"), &CommunicationTransmitterNode::get_mind_agent_id);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "mind_agent_id"), "set_mind_agent_id", "get_mind_agent_id");

    ClassDB::bind_method(D_METHOD("set_active_lect_id", "lect_id"), &CommunicationTransmitterNode::set_active_lect_id);
    ClassDB::bind_method(D_METHOD("get_active_lect_id"), &CommunicationTransmitterNode::get_active_lect_id);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "active_lect_id"), "set_active_lect_id", "get_active_lect_id");

    ClassDB::bind_method(D_METHOD("set_elocution_skill", "skill"), &CommunicationTransmitterNode::set_elocution_skill);
    ClassDB::bind_method(D_METHOD("get_elocution_skill"), &CommunicationTransmitterNode::get_elocution_skill);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "elocution_skill", PROPERTY_HINT_RANGE, "0.05,5.0,0.05"), "set_elocution_skill", "get_elocution_skill");

    ClassDB::bind_method(D_METHOD("set_projection_power_db", "power_db"), &CommunicationTransmitterNode::set_projection_power_db);
    ClassDB::bind_method(D_METHOD("get_projection_power_db"), &CommunicationTransmitterNode::get_projection_power_db);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "projection_power_db", PROPERTY_HINT_RANGE, "20.0,130.0,0.5"), "set_projection_power_db", "get_projection_power_db");

    ClassDB::bind_method(D_METHOD("set_world_position", "position"), &CommunicationTransmitterNode::set_world_position);
    ClassDB::bind_method(D_METHOD("get_world_position"), &CommunicationTransmitterNode::get_world_position);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "world_position"), "set_world_position", "get_world_position");

    ClassDB::bind_method(D_METHOD("set_speaker_display_name", "name"), &CommunicationTransmitterNode::set_speaker_display_name);
    ClassDB::bind_method(D_METHOD("get_speaker_display_name"), &CommunicationTransmitterNode::get_speaker_display_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "speaker_display_name"), "set_speaker_display_name", "get_speaker_display_name");

    ClassDB::bind_method(D_METHOD("set_language_asset", "language_asset"), &CommunicationTransmitterNode::set_language_asset);
    ClassDB::bind_method(D_METHOD("get_language_asset"), &CommunicationTransmitterNode::get_language_asset);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "language_asset", PROPERTY_HINT_RESOURCE_TYPE, "LanguageAssetResource"),
        "set_language_asset",
        "get_language_asset"
    );

    ClassDB::bind_method(D_METHOD("set_speaker_mind", "speaker_mind"), &CommunicationTransmitterNode::set_speaker_mind);
    ClassDB::bind_method(D_METHOD("get_speaker_mind"), &CommunicationTransmitterNode::get_speaker_mind);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "speaker_mind", PROPERTY_HINT_RESOURCE_TYPE, "AgentMindResource"),
        "set_speaker_mind",
        "get_speaker_mind"
    );

    ClassDB::bind_method(D_METHOD("set_default_prosody_spline", "prosody_spline"), &CommunicationTransmitterNode::set_default_prosody_spline);
    ClassDB::bind_method(D_METHOD("get_default_prosody_spline"), &CommunicationTransmitterNode::get_default_prosody_spline);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "default_prosody_spline", PROPERTY_HINT_RESOURCE_TYPE, "Spline2DResource"),
        "set_default_prosody_spline",
        "get_default_prosody_spline"
    );

    ClassDB::bind_method(
        D_METHOD("transmit_utterance", "utterance", "medium", "receiver", "random_seed"),
        &CommunicationTransmitterNode::transmit_utterance,
        DEFVAL(1337)
    );

    ADD_SIGNAL(MethodInfo("transmission_started", PropertyInfo(Variant::STRING, "raw_text")));
    ADD_SIGNAL(MethodInfo("signal_emitted", PropertyInfo(Variant::DICTIONARY, "packet_summary")));
}

core::TransmitterStatePOD CommunicationTransmitterNode::to_pod() const noexcept {
    core::TransmitterStatePOD tx{};
    tx.transmitter_id         = static_cast<uint32_t>(std::max(0, transmitter_id_));
    if (speaker_mind_.is_valid()) {
        tx.mind_agent_id = static_cast<uint32_t>( std::max(0, speaker_mind_->get_agent_id()) );
    } else {
        tx.mind_agent_id = (mind_agent_id_ >= 0) ? static_cast<uint32_t>(mind_agent_id_) : 0xFFFFFFFFu;
    }
    tx.elocution_encode_skill = elocution_skill_;
    tx.projection_power_db    = projection_power_db_;
    tx.pos_x                  = world_position_.x;
    tx.pos_y                  = world_position_.y;
    tx.pos_z                  = world_position_.z;
    tx.active_lect_id         = static_cast<uint16_t>(std::max(0, active_lect_id_));
    return tx;
}

godot::Dictionary CommunicationTransmitterNode::transmit_utterance(
    const godot::Ref<UtteranceResource> &p_utterance,
    const godot::Ref<CommunicationMediumResource> &p_medium,
    CommunicationReceiverNode *p_receiver,
    int64_t p_random_seed
) {
    godot::Dictionary empty_summary;
    if (p_utterance.is_null()) return empty_summary;

    emit_signal("transmission_started", p_utterance->get_raw_text());

    const core::TransmitterStatePOD tx = to_pod();
    const core::TransmissionMediumPOD med = p_medium.is_valid()
        ? p_medium->to_pod()
        : core::TransmissionMediumPOD{};
    const core::ReceiverStatePOD rx = (p_receiver != nullptr)
        ? p_receiver->to_pod()
        : core::ReceiverStatePOD{};

    core::SemanticLexiconCore fallback_lex;
    const core::SemanticLexiconCore *lex_ptr = nullptr;
    if (language_asset_.is_valid() && language_asset_->get_lexicon().is_valid()) {
        lex_ptr = &language_asset_->get_lexicon()->get_core_lexicon();
    } else if (p_receiver != nullptr &&
               p_receiver->get_language_asset().is_valid() &&
               p_receiver->get_language_asset()->get_lexicon().is_valid()) {
        lex_ptr = &p_receiver->get_language_asset()->get_lexicon()->get_core_lexicon();
    } else {
        fallback_lex.populate_default_core_semes_and_morphemes();
        lex_ptr = &fallback_lex;
    }

    const core::ProsodySamplePOD prosody = p_utterance->sample_prosody_pod_at(0.5f);
    const uint32_t src_fact_id = (p_utterance->get_source_fact_id() >= 0)
        ? static_cast<uint32_t>(p_utterance->get_source_fact_id())
        : 0xFFFFFFFFu;
    const uint32_t intended_sem = (p_utterance->get_expected_sememe_id() >= 0)
        ? static_cast<uint32_t>(p_utterance->get_expected_sememe_id())
        : 0xFFFFFFFFu;

    const core::DecodedUtteranceResult res = core::NoisyChannelTransceiver::transmit_and_decode_utterance(
        p_utterance->get_raw_text().utf8().get_data(),
        src_fact_id,
        intended_sem,
        tx,
        med,
        rx,
        *lex_ptr,
        static_cast<uint64_t>(p_random_seed),
        prosody
    );

    godot::Dictionary summary;
    if (p_receiver != nullptr) {
        summary = p_receiver->receive_decoded_result(res, speaker_display_name_, p_utterance);
    } else {
        summary["original_text"]          = p_utterance->get_raw_text();
        summary["decoded_text"]           = godot::String::utf8(res.reconstructed_text.c_str());
        summary["effective_snr_db"]       = res.packet.effective_snr_db;
        summary["noise_tier"]             = static_cast<int>(res.packet.noise_tier);
        summary["garbled_syllable_count"] = static_cast<int>(res.packet.garbled_syllable_count);
        summary["honest_misreference"]    = (res.packet.honest_misreference != 0u);
    }

    emit_signal("signal_emitted", summary);
    return summary;
}

// ============================================================================
// CommunicationReceiverNode
// ============================================================================

void CommunicationReceiverNode::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_receiver_id", "id"), &CommunicationReceiverNode::set_receiver_id);
    ClassDB::bind_method(D_METHOD("get_receiver_id"), &CommunicationReceiverNode::get_receiver_id);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "receiver_id"), "set_receiver_id", "get_receiver_id");

    ClassDB::bind_method(D_METHOD("set_mind_agent_id", "id"), &CommunicationReceiverNode::set_mind_agent_id);
    ClassDB::bind_method(D_METHOD("get_mind_agent_id"), &CommunicationReceiverNode::get_mind_agent_id);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "mind_agent_id"), "set_mind_agent_id", "get_mind_agent_id");

    ClassDB::bind_method(D_METHOD("set_expected_sememe_id", "sememe_id"), &CommunicationReceiverNode::set_expected_sememe_id);
    ClassDB::bind_method(D_METHOD("get_expected_sememe_id"), &CommunicationReceiverNode::get_expected_sememe_id);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "expected_sememe_id"), "set_expected_sememe_id", "get_expected_sememe_id");

    ClassDB::bind_method(D_METHOD("set_listen_skill", "skill"), &CommunicationReceiverNode::set_listen_skill);
    ClassDB::bind_method(D_METHOD("get_listen_skill"), &CommunicationReceiverNode::get_listen_skill);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "listen_skill", PROPERTY_HINT_RANGE, "0.05,5.0,0.05"), "set_listen_skill", "get_listen_skill");

    ClassDB::bind_method(D_METHOD("set_perception_bonus_db", "bonus_db"), &CommunicationReceiverNode::set_perception_bonus_db);
    ClassDB::bind_method(D_METHOD("get_perception_bonus_db"), &CommunicationReceiverNode::get_perception_bonus_db);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "perception_bonus_db", PROPERTY_HINT_RANGE, "-40.0,60.0,0.5"), "set_perception_bonus_db", "get_perception_bonus_db");

    ClassDB::bind_method(D_METHOD("set_world_position", "position"), &CommunicationReceiverNode::set_world_position);
    ClassDB::bind_method(D_METHOD("get_world_position"), &CommunicationReceiverNode::get_world_position);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "world_position"), "set_world_position", "get_world_position");

    ClassDB::bind_method(D_METHOD("set_auto_speak_tts", "enable"), &CommunicationReceiverNode::set_auto_speak_tts);
    ClassDB::bind_method(D_METHOD("get_auto_speak_tts"), &CommunicationReceiverNode::get_auto_speak_tts);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_speak_tts"), "set_auto_speak_tts", "get_auto_speak_tts");

    ClassDB::bind_method(D_METHOD("set_language_asset", "language_asset"), &CommunicationReceiverNode::set_language_asset);
    ClassDB::bind_method(D_METHOD("get_language_asset"), &CommunicationReceiverNode::get_language_asset);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "language_asset", PROPERTY_HINT_RESOURCE_TYPE, "LanguageAssetResource"),
        "set_language_asset",
        "get_language_asset"
    );

    ClassDB::bind_method(D_METHOD("set_listener_mind", "listener_mind"), &CommunicationReceiverNode::set_listener_mind);
    ClassDB::bind_method(D_METHOD("get_listener_mind"), &CommunicationReceiverNode::get_listener_mind);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "listener_mind", PROPERTY_HINT_RESOURCE_TYPE, "AgentMindResource"),
        "set_listener_mind",
        "get_listener_mind"
    );

    ClassDB::bind_method(D_METHOD("set_akashic_tome", "akashic_tome"), &CommunicationReceiverNode::set_akashic_tome);
    ClassDB::bind_method(D_METHOD("get_akashic_tome"), &CommunicationReceiverNode::get_akashic_tome);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "akashic_tome", PROPERTY_HINT_RESOURCE_TYPE, "InformationMatrixResource"),
        "set_akashic_tome",
        "get_akashic_tome"
    );

    ClassDB::bind_method(D_METHOD("set_speech_box_2d_path", "path"), &CommunicationReceiverNode::set_speech_box_2d_path);
    ClassDB::bind_method(D_METHOD("get_speech_box_2d_path"), &CommunicationReceiverNode::get_speech_box_2d_path);
    ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "speech_box_2d_path"), "set_speech_box_2d_path", "get_speech_box_2d_path");

    ClassDB::bind_method(D_METHOD("set_speech_bubble_3d_path", "path"), &CommunicationReceiverNode::set_speech_bubble_3d_path);
    ClassDB::bind_method(D_METHOD("get_speech_bubble_3d_path"), &CommunicationReceiverNode::get_speech_bubble_3d_path);
    ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "speech_bubble_3d_path"), "set_speech_bubble_3d_path", "get_speech_bubble_3d_path");

    ClassDB::bind_method(D_METHOD("get_last_received_packet"), &CommunicationReceiverNode::get_last_received_packet);

    ADD_SIGNAL(MethodInfo("signal_received", PropertyInfo(Variant::DICTIONARY, "packet_summary")));
    ADD_SIGNAL(MethodInfo(
        "utterance_decoded",
        PropertyInfo(Variant::STRING, "decoded_text"),
        PropertyInfo(Variant::FLOAT, "effective_snr_db"),
        PropertyInfo(Variant::BOOL, "honest_misreference")
    ));
    ADD_SIGNAL(MethodInfo(
        "honest_misreference_recorded",
        PropertyInfo(Variant::INT, "source_fact_id"),
        PropertyInfo(Variant::INT, "decoded_fact_id")
    ));
}

core::ReceiverStatePOD CommunicationReceiverNode::to_pod() const noexcept {
    core::ReceiverStatePOD rx{};
    rx.receiver_id = static_cast<uint32_t>(std::max(0, receiver_id_));
    if (listener_mind_.is_valid()) {
        rx.mind_agent_id = static_cast<uint32_t>(std::max(0, listener_mind_->get_agent_id()));
    } else {
        rx.mind_agent_id = (mind_agent_id_ >= 0) ? static_cast<uint32_t>(mind_agent_id_) : 0xFFFFFFFFu;
    }
    rx.listen_decode_skill = listen_skill_;
    rx.perception_bonus_db = perception_bonus_db_;
    rx.pos_x               = world_position_.x;
    rx.pos_y               = world_position_.y;
    rx.pos_z               = world_position_.z;
    rx.expected_sememe_id  = (expected_sememe_id_ >= 0) ? static_cast<uint32_t>(expected_sememe_id_) : 0xFFFFFFFFu;
    return rx;
}

godot::Dictionary CommunicationReceiverNode::receive_decoded_result(
    const core::DecodedUtteranceResult &p_result,
    const godot::String &p_speaker_name,
    const godot::Ref<UtteranceResource> &p_utterance
) {
    const godot::String decoded_str = godot::String::utf8(p_result.reconstructed_text.c_str());
    const bool is_misref = (p_result.packet.honest_misreference != 0u);

    godot::Dictionary summary;
    summary["speaker_name"]           = p_speaker_name;
    summary["original_text"]          = godot::String::utf8(p_result.original_text.c_str());
    summary["decoded_text"]           = decoded_str;
    summary["effective_snr_db"]       = p_result.packet.effective_snr_db;
    summary["noise_tier"]             = static_cast<int>(p_result.packet.noise_tier);
    summary["garbled_syllable_count"] = static_cast<int>(p_result.packet.garbled_syllable_count);
    summary["honest_misreference"]    = is_misref;
    summary["source_fact_id"]         = static_cast<int>(p_result.packet.source_fact_id);
    summary["decoded_fact_id"]        = static_cast<int>(p_result.packet.decoded_fact_id);
    summary["decoded_sememe_id"]      = static_cast<int>(p_result.packet.decoded_sememe_id);
    summary["decoded_confidence"]     = p_result.epistemic_handoff.decoded_confidence;

    last_received_packet_ = summary;

    if (is_misref) {
        emit_signal(
            "honest_misreference_recorded",
            static_cast<int>(p_result.packet.source_fact_id),
            static_cast<int>(p_result.packet.decoded_fact_id)
        );
    }

    if (auto_speak_tts_ && p_utterance.is_valid()) {
        p_utterance->speak();
    }

    const float rate_cps = p_utterance.is_valid()
        ? std::max(4.0f, p_utterance->get_rate() * 24.0f)
        : 24.0f;
    const godot::Ref<Spline2DResource> prosody = p_utterance.is_valid()
        ? p_utterance->get_prosody_spline()
        : godot::Ref<Spline2DResource>();

    if (!speech_box_2d_path_.is_empty() && is_inside_tree()) {
        if (auto *box = godot::Object::cast_to<CommunicationSpeechBox2D>(get_node_or_null(speech_box_2d_path_))) {
            box->display_utterance(p_speaker_name, decoded_str, rate_cps, prosody);
        }
    }
    if (!speech_bubble_3d_path_.is_empty() && is_inside_tree()) {
        if (auto *bubble = godot::Object::cast_to<CommunicationSpeechBubble3D>(get_node_or_null(speech_bubble_3d_path_))) {
            bubble->display_utterance(p_speaker_name, decoded_str, rate_cps, prosody);
        }
    }

    emit_signal("utterance_decoded", decoded_str, p_result.packet.effective_snr_db, is_misref);
    emit_signal("signal_received", summary);
    return summary;
}

// ============================================================================
// CommunicationSpeechBox2D (`dialogue_box.gd` Port & Upgrade)
// ============================================================================

void CommunicationSpeechBox2D::_bind_methods() {
    using namespace godot;

    BIND_ENUM_CONSTANT(TEXT_CLASS_LABEL);
    BIND_ENUM_CONSTANT(TEXT_CLASS_RICH_TEXT_LABEL);

    ClassDB::bind_method(D_METHOD("set_label_type", "type"), &CommunicationSpeechBox2D::set_label_type);
    ClassDB::bind_method(D_METHOD("get_label_type"), &CommunicationSpeechBox2D::get_label_type);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "label_type", PROPERTY_HINT_ENUM, "Label:0,RichTextLabel:1"), "set_label_type", "get_label_type");

    ClassDB::bind_method(D_METHOD("set_main_text_type", "type"), &CommunicationSpeechBox2D::set_main_text_type);
    ClassDB::bind_method(D_METHOD("get_main_text_type"), &CommunicationSpeechBox2D::get_main_text_type);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "main_text_type", PROPERTY_HINT_ENUM, "Label:0,RichTextLabel:1"), "set_main_text_type", "get_main_text_type");

    ClassDB::bind_method(D_METHOD("set_interlocutor_id", "id"), &CommunicationSpeechBox2D::set_interlocutor_id);
    ClassDB::bind_method(D_METHOD("get_interlocutor_id"), &CommunicationSpeechBox2D::get_interlocutor_id);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "interlocutor_id"), "set_interlocutor_id", "get_interlocutor_id");

    ClassDB::bind_method(D_METHOD("set_reveal_rate_cps", "rate_cps"), &CommunicationSpeechBox2D::set_reveal_rate_cps);
    ClassDB::bind_method(D_METHOD("get_reveal_rate_cps"), &CommunicationSpeechBox2D::get_reveal_rate_cps);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "reveal_rate_cps", PROPERTY_HINT_RANGE, "1.0,200.0,1.0"), "set_reveal_rate_cps", "get_reveal_rate_cps");

    ClassDB::bind_method(D_METHOD("set_visible_ratio", "ratio"), &CommunicationSpeechBox2D::set_visible_ratio);
    ClassDB::bind_method(D_METHOD("get_visible_ratio"), &CommunicationSpeechBox2D::get_visible_ratio);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "visible_ratio", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_visible_ratio", "get_visible_ratio");

    ClassDB::bind_method(D_METHOD("set_speaker_name", "name"), &CommunicationSpeechBox2D::set_speaker_name);
    ClassDB::bind_method(D_METHOD("get_speaker_name"), &CommunicationSpeechBox2D::get_speaker_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "speaker_name"), "set_speaker_name", "get_speaker_name");

    ClassDB::bind_method(D_METHOD("set_full_text", "text"), &CommunicationSpeechBox2D::set_full_text);
    ClassDB::bind_method(D_METHOD("get_full_text"), &CommunicationSpeechBox2D::get_full_text);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "full_text", PROPERTY_HINT_MULTILINE_TEXT), "set_full_text", "get_full_text");

    ClassDB::bind_method(D_METHOD("set_reveal_curve", "curve"), &CommunicationSpeechBox2D::set_reveal_curve);
    ClassDB::bind_method(D_METHOD("get_reveal_curve"), &CommunicationSpeechBox2D::get_reveal_curve);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "reveal_curve", PROPERTY_HINT_RESOURCE_TYPE, "Spline1DResource"),
        "set_reveal_curve",
        "get_reveal_curve"
    );

    ClassDB::bind_method(D_METHOD("set_active_prosody_spline", "spline"), &CommunicationSpeechBox2D::set_active_prosody_spline);
    ClassDB::bind_method(D_METHOD("get_active_prosody_spline"), &CommunicationSpeechBox2D::get_active_prosody_spline);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "active_prosody_spline", PROPERTY_HINT_RESOURCE_TYPE, "Spline2DResource"),
        "set_active_prosody_spline",
        "get_active_prosody_spline"
    );

    ClassDB::bind_method(D_METHOD("set_accessibility_option", "option"), &CommunicationSpeechBox2D::set_accessibility_option);
    ClassDB::bind_method(D_METHOD("get_accessibility_option"), &CommunicationSpeechBox2D::get_accessibility_option);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "accessibility_option", PROPERTY_HINT_RESOURCE_TYPE, "GameplayAccessibilityOption"),
        "set_accessibility_option",
        "get_accessibility_option"
    );

    ClassDB::bind_method(D_METHOD("open_dialogue_box"), &CommunicationSpeechBox2D::open_dialogue_box);
    ClassDB::bind_method(D_METHOD("close_dialogue_box"), &CommunicationSpeechBox2D::close_dialogue_box);
    ClassDB::bind_method(
        D_METHOD("display_utterance", "speaker", "decoded_text", "rate_cps", "prosody"),
        &CommunicationSpeechBox2D::display_utterance,
        DEFVAL(24.0f), DEFVAL(Variant())
    );
    ClassDB::bind_method(D_METHOD("update_dialogue", "normalized_progress"), &CommunicationSpeechBox2D::update_dialogue);
    ClassDB::bind_method(D_METHOD("stop_dialogue"), &CommunicationSpeechBox2D::stop_dialogue);
    ClassDB::bind_method(D_METHOD("complete_dialogue"), &CommunicationSpeechBox2D::complete_dialogue);
    ClassDB::bind_method(D_METHOD("fast_track_dialogue"), &CommunicationSpeechBox2D::fast_track_dialogue);

    ADD_SIGNAL(MethodInfo("box_opened"));
    ADD_SIGNAL(MethodInfo("box_closed"));
    ADD_SIGNAL(MethodInfo("dialogue_started", PropertyInfo(Variant::INT, "interlocutor_id")));
    ADD_SIGNAL(MethodInfo("dialogue_updated", PropertyInfo(Variant::INT, "interlocutor_id"), PropertyInfo(Variant::FLOAT, "visible_ratio")));
    ADD_SIGNAL(MethodInfo("dialogue_completed", PropertyInfo(Variant::INT, "interlocutor_id")));
}

void CommunicationSpeechBox2D::_ready() {
    _rebuild_speaker_label_node();
    _rebuild_main_text_node();
    _apply_text_and_ratio();
}

void CommunicationSpeechBox2D::_process(double p_delta) {
    if (!is_revealing_) return;
    elapsed_seconds_ += static_cast<float>(p_delta);
    const float raw_t = (total_duration_seconds_ > 0.0001f)
        ? std::clamp(elapsed_seconds_ / total_duration_seconds_, 0.0f, 1.0f)
        : 1.0f;
    update_dialogue(raw_t);
}

void CommunicationSpeechBox2D::_rebuild_speaker_label_node() {
    if (speaker_label_node_ != nullptr) {
        speaker_label_node_->queue_free();
        speaker_label_node_ = nullptr;
    }
    if (label_type_ == TEXT_CLASS_RICH_TEXT_LABEL) {
        auto *rtl = memnew(godot::RichTextLabel);
        rtl->set_name("Label");
        rtl->set_fit_content(true);
        speaker_label_node_ = rtl;
    } else {
        auto *lbl = memnew(godot::Label);
        lbl->set_name("Label");
        speaker_label_node_ = lbl;
    }
    add_child(speaker_label_node_);
}

void CommunicationSpeechBox2D::_rebuild_main_text_node() {
    // Fixes line 55 bug from dialogue_box.gd: free main_text_node_, NOT speaker_label_node_!
    if (main_text_node_ != nullptr) {
        main_text_node_->queue_free();
        main_text_node_ = nullptr;
    }
    if (main_text_type_ == TEXT_CLASS_RICH_TEXT_LABEL) {
        auto *rtl = memnew(godot::RichTextLabel);
        rtl->set_name("Main_Text");
        rtl->set_fit_content(true);
        main_text_node_ = rtl;
    } else {
        auto *lbl = memnew(godot::Label);
        lbl->set_name("Main_Text");
        main_text_node_ = lbl;
    }
    add_child(main_text_node_);
}

void CommunicationSpeechBox2D::set_label_type(int p_type) {
    const int clamped = std::clamp(p_type, 0, 1);
    if (clamped == label_type_) return;
    label_type_ = clamped;
    if (is_inside_tree()) {
        _rebuild_speaker_label_node();
        _apply_text_and_ratio();
    }
}

void CommunicationSpeechBox2D::set_main_text_type(int p_type) {
    const int clamped = std::clamp(p_type, 0, 1);
    if (clamped == main_text_type_) return;
    main_text_type_ = clamped;
    if (is_inside_tree()) {
        _rebuild_main_text_node();
        _apply_text_and_ratio();
    }
}

void CommunicationSpeechBox2D::set_visible_ratio(float p_ratio) {
    visible_ratio_ = std::clamp(p_ratio, 0.0f, 1.0f);
    _apply_text_and_ratio();
}

void CommunicationSpeechBox2D::set_speaker_name(const godot::String &p_name) {
    speaker_name_ = p_name;
    _apply_text_and_ratio();
}

void CommunicationSpeechBox2D::set_full_text(const godot::String &p_text) {
    full_text_ = p_text;
    _apply_text_and_ratio();
}

void CommunicationSpeechBox2D::set_accessibility_option(const godot::Ref<GameplayAccessibilityOption> &p_opt) {
    accessibility_option_ = p_opt;
    if (accessibility_option_.is_valid()) {
        caption_text_scale_ = accessibility_option_->get_caption_text_scale();
        caption_bg_opacity_ = accessibility_option_->get_caption_background_opacity();
        show_speaker_label_ = accessibility_option_->is_caption_speaker_labels_enabled();
    }
    _apply_text_and_ratio();
}

void CommunicationSpeechBox2D::_apply_text_and_ratio() {
    float prosody_vol_scale = 1.0f;
    if (active_prosody_spline_.is_valid() && active_prosody_spline_->get_point_count() > 0) {
        const godot::Vector2 vp = active_prosody_spline_->sample_normalized(visible_ratio_);
        prosody_vol_scale = std::clamp(vp.x, 0.65f, 1.50f);
    }
    const int32_t effective_font_px = static_cast<int32_t>(
        std::round(base_font_size_ * caption_text_scale_ * prosody_vol_scale)
    );

    if (speaker_label_node_ != nullptr) {
        speaker_label_node_->set_visible(show_speaker_label_ && !speaker_name_.is_empty());
        speaker_label_node_->add_theme_font_size_override("font_size", effective_font_px);
        if (auto *lbl = godot::Object::cast_to<godot::Label>(speaker_label_node_)) {
            lbl->set_text(speaker_name_);
        } else if (auto *rtl = godot::Object::cast_to<godot::RichTextLabel>(speaker_label_node_)) {
            rtl->set_text(speaker_name_);
        }
    }

    if (main_text_node_ != nullptr) {
        main_text_node_->add_theme_font_size_override("font_size", effective_font_px);
        if (auto *lbl = godot::Object::cast_to<godot::Label>(main_text_node_)) {
            lbl->set_text(full_text_);
            lbl->set_visible_ratio(visible_ratio_);
        } else if (auto *rtl = godot::Object::cast_to<godot::RichTextLabel>(main_text_node_)) {
            rtl->set_text(full_text_);
            rtl->set_visible_ratio(visible_ratio_);
        }
    }
}

void CommunicationSpeechBox2D::open_dialogue_box() {
    if (speaker_label_node_ == nullptr) _rebuild_speaker_label_node();
    if (main_text_node_ == nullptr) _rebuild_main_text_node();
    set_visible(true);
    emit_signal("box_opened");
}

void CommunicationSpeechBox2D::close_dialogue_box() {
    is_revealing_  = false;
    visible_ratio_ = 0.0f;
    full_text_     = "";
    _apply_text_and_ratio();
    set_visible(false);
    emit_signal("box_closed");
}

void CommunicationSpeechBox2D::display_utterance(
    const godot::String &p_speaker,
    const godot::String &p_decoded_text,
    float p_rate_cps,
    const godot::Ref<Spline2DResource> &p_prosody
) {
    speaker_name_           = p_speaker;
    full_text_              = p_decoded_text;
    reveal_rate_cps_        = std::max(1.0f, p_rate_cps);
    active_prosody_spline_  = p_prosody;
    elapsed_seconds_        = 0.0f;
    visible_ratio_          = 0.0f;
    total_duration_seconds_ = std::max(0.05f, static_cast<float>(full_text_.length()) / reveal_rate_cps_);
    is_revealing_           = true;

    open_dialogue_box();
    _apply_text_and_ratio();
    set_process(true);
    emit_signal("dialogue_started", interlocutor_id_);
}

void CommunicationSpeechBox2D::update_dialogue(float p_normalized_progress) {
    const float clamped_t = std::clamp(p_normalized_progress, 0.0f, 1.0f);
    float mapped_ratio = clamped_t;
    if (reveal_curve_.is_valid() && reveal_curve_->get_point_count() > 0) {
        mapped_ratio = std::clamp(static_cast<float>(reveal_curve_->sample_normalized(clamped_t)), 0.0f, 1.0f);
    }

    visible_ratio_ = mapped_ratio;
    _apply_text_and_ratio();

    if (clamped_t >= 1.0f) {
        complete_dialogue();
        return;
    }
    emit_signal("dialogue_updated", interlocutor_id_, visible_ratio_);
}

void CommunicationSpeechBox2D::stop_dialogue() {
    is_revealing_ = false;
    set_process(false);
}

void CommunicationSpeechBox2D::complete_dialogue() {
    is_revealing_  = false;
    visible_ratio_ = 1.0f;
    set_process(false);
    _apply_text_and_ratio();
    emit_signal("dialogue_completed", interlocutor_id_);
}

void CommunicationSpeechBox2D::fast_track_dialogue() {
    stop_dialogue();
    complete_dialogue();
}

// ============================================================================
// CommunicationSpeechBubble3D (`expandedlabel3d.gd` Port & Upgrade)
// ============================================================================

void CommunicationSpeechBubble3D::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_speaker_name", "name"), &CommunicationSpeechBubble3D::set_speaker_name);
    ClassDB::bind_method(D_METHOD("get_speaker_name"), &CommunicationSpeechBubble3D::get_speaker_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "speaker_name"), "set_speaker_name", "get_speaker_name");

    ClassDB::bind_method(D_METHOD("set_full_text", "text"), &CommunicationSpeechBubble3D::set_full_text);
    ClassDB::bind_method(D_METHOD("get_full_text"), &CommunicationSpeechBubble3D::get_full_text);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "full_text", PROPERTY_HINT_MULTILINE_TEXT), "set_full_text", "get_full_text");

    ClassDB::bind_method(D_METHOD("set_visible_ratio", "ratio"), &CommunicationSpeechBubble3D::set_visible_ratio);
    ClassDB::bind_method(D_METHOD("get_visible_ratio"), &CommunicationSpeechBubble3D::get_visible_ratio);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "visible_ratio", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_visible_ratio", "get_visible_ratio");

    ClassDB::bind_method(D_METHOD("set_reveal_rate_cps", "rate_cps"), &CommunicationSpeechBubble3D::set_reveal_rate_cps);
    ClassDB::bind_method(D_METHOD("get_reveal_rate_cps"), &CommunicationSpeechBubble3D::get_reveal_rate_cps);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "reveal_rate_cps", PROPERTY_HINT_RANGE, "1.0,200.0,1.0"), "set_reveal_rate_cps", "get_reveal_rate_cps");

    ClassDB::bind_method(D_METHOD("set_reveal_curve", "curve"), &CommunicationSpeechBubble3D::set_reveal_curve);
    ClassDB::bind_method(D_METHOD("get_reveal_curve"), &CommunicationSpeechBubble3D::get_reveal_curve);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "reveal_curve", PROPERTY_HINT_RESOURCE_TYPE, "Spline1DResource"),
        "set_reveal_curve",
        "get_reveal_curve"
    );

    ClassDB::bind_method(D_METHOD("set_active_prosody_spline", "spline"), &CommunicationSpeechBubble3D::set_active_prosody_spline);
    ClassDB::bind_method(D_METHOD("get_active_prosody_spline"), &CommunicationSpeechBubble3D::get_active_prosody_spline);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "active_prosody_spline", PROPERTY_HINT_RESOURCE_TYPE, "Spline2DResource"),
        "set_active_prosody_spline",
        "get_active_prosody_spline"
    );

    ClassDB::bind_method(D_METHOD("set_accessibility_option", "option"), &CommunicationSpeechBubble3D::set_accessibility_option);
    ClassDB::bind_method(D_METHOD("get_accessibility_option"), &CommunicationSpeechBubble3D::get_accessibility_option);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "accessibility_option", PROPERTY_HINT_RESOURCE_TYPE, "GameplayAccessibilityOption"),
        "set_accessibility_option",
        "get_accessibility_option"
    );

    ClassDB::bind_method(D_METHOD("update_visible_ratio"), &CommunicationSpeechBubble3D::update_visible_ratio);
    ClassDB::bind_method(
        D_METHOD("display_utterance", "speaker", "decoded_text", "rate_cps", "prosody"),
        &CommunicationSpeechBubble3D::display_utterance,
        DEFVAL(24.0f), DEFVAL(Variant())
    );
    ClassDB::bind_method(D_METHOD("fast_track_dialogue"), &CommunicationSpeechBubble3D::fast_track_dialogue);

    ADD_SIGNAL(MethodInfo("bubble_opened"));
    ADD_SIGNAL(MethodInfo("bubble_closed"));
    ADD_SIGNAL(MethodInfo("dialogue_started", PropertyInfo(Variant::INT, "interlocutor_id")));
    ADD_SIGNAL(MethodInfo("dialogue_updated", PropertyInfo(Variant::INT, "interlocutor_id"), PropertyInfo(Variant::FLOAT, "visible_ratio")));
    ADD_SIGNAL(MethodInfo("dialogue_completed", PropertyInfo(Variant::INT, "interlocutor_id")));
}

void CommunicationSpeechBubble3D::_process(double p_delta) {
    if (!is_revealing_) return;
    elapsed_seconds_ += static_cast<float>(p_delta);
    const float raw_t = (total_duration_seconds_ > 0.0001f)
        ? std::clamp(elapsed_seconds_ / total_duration_seconds_, 0.0f, 1.0f)
        : 1.0f;

    float mapped_ratio = raw_t;
    if (reveal_curve_.is_valid() && reveal_curve_->get_point_count() > 0) {
        mapped_ratio = std::clamp(static_cast<float>(reveal_curve_->sample_normalized(raw_t)), 0.0f, 1.0f);
    }
    visible_ratio_ = mapped_ratio;
    update_visible_ratio();

    if (raw_t >= 1.0f) {
        is_revealing_  = false;
        visible_ratio_ = 1.0f;
        set_process(false);
        update_visible_ratio();
        emit_signal("dialogue_completed", interlocutor_id_);
        return;
    }
    emit_signal("dialogue_updated", interlocutor_id_, visible_ratio_);
}

void CommunicationSpeechBubble3D::set_speaker_name(const godot::String &p_name) {
    speaker_name_ = p_name;
    update_visible_ratio();
}

void CommunicationSpeechBubble3D::set_full_text(const godot::String &p_text) {
    full_text_ = p_text;
    update_visible_ratio();
}

void CommunicationSpeechBubble3D::set_visible_ratio(float p_ratio) {
    visible_ratio_ = std::clamp(p_ratio, 0.0f, 1.0f);
    update_visible_ratio();
}

void CommunicationSpeechBubble3D::set_accessibility_option(const godot::Ref<GameplayAccessibilityOption> &p_opt) {
    accessibility_option_ = p_opt;
    if (accessibility_option_.is_valid()) {
        caption_text_scale_ = accessibility_option_->get_caption_text_scale();
        show_speaker_label_ = accessibility_option_->is_caption_speaker_labels_enabled();
    }
    update_visible_ratio();
}

void CommunicationSpeechBubble3D::update_visible_ratio() {
    const int64_t total_chars = full_text_.length();
    const int64_t shown_chars = static_cast<int64_t>(std::round(static_cast<double>(total_chars) * visible_ratio_));
    const godot::String visible_slice = full_text_.substr(0, std::clamp<int64_t>(shown_chars, 0, total_chars));

    if (show_speaker_label_ && !speaker_name_.is_empty()) {
        set_text(speaker_name_ + ": " + visible_slice);
    } else {
        set_text(visible_slice);
    }

    float prosody_scale = 1.0f;
    if (active_prosody_spline_.is_valid() && active_prosody_spline_->get_point_count() > 0) {
        const godot::Vector2 vp = active_prosody_spline_->sample_normalized(visible_ratio_);
        prosody_scale = std::clamp(vp.x, 0.65f, 1.50f);
    }
    set_pixel_size(base_pixel_size_ * caption_text_scale_ * prosody_scale);
}

void CommunicationSpeechBubble3D::display_utterance(
    const godot::String &p_speaker,
    const godot::String &p_decoded_text,
    float p_rate_cps,
    const godot::Ref<Spline2DResource> &p_prosody
) {
    speaker_name_           = p_speaker;
    full_text_              = p_decoded_text;
    reveal_rate_cps_        = std::max(1.0f, p_rate_cps);
    active_prosody_spline_  = p_prosody;
    elapsed_seconds_        = 0.0f;
    visible_ratio_          = 0.0f;
    total_duration_seconds_ = std::max(0.05f, static_cast<float>(full_text_.length()) / reveal_rate_cps_);
    is_revealing_           = true;

    set_visible(true);
    update_visible_ratio();
    set_process(true);
    emit_signal("bubble_opened");
    emit_signal("dialogue_started", interlocutor_id_);
}

void CommunicationSpeechBubble3D::fast_track_dialogue() {
    is_revealing_  = false;
    visible_ratio_ = 1.0f;
    set_process(false);
    update_visible_ratio();
    emit_signal("dialogue_completed", interlocutor_id_);
}

} // namespace ideam::godot_ext
