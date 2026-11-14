#include "discourse_resource.h"
#include "ideam_communications_plugin.h"
#include <godot_cpp/classes/display_server.hpp>
#include <algorithm>
#include <cmath>

namespace ideam::godot_ext {

// ============================================================================
// UtteranceResource
// ============================================================================

void UtteranceResource::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_raw_text", "raw_text"), &UtteranceResource::set_raw_text);
    ClassDB::bind_method(D_METHOD("get_raw_text"), &UtteranceResource::get_raw_text);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "raw_text", PROPERTY_HINT_MULTILINE_TEXT), "set_raw_text", "get_raw_text");

    ClassDB::bind_method(D_METHOD("set_prosody_spline", "prosody_spline"), &UtteranceResource::set_prosody_spline);
    ClassDB::bind_method(D_METHOD("get_prosody_spline"), &UtteranceResource::get_prosody_spline);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "prosody_spline", PROPERTY_HINT_RESOURCE_TYPE, "Spline2DResource"),
        "set_prosody_spline",
        "get_prosody_spline"
    );

    ClassDB::bind_method(D_METHOD("set_tcu_type", "tcu_type"), &UtteranceResource::set_tcu_type);
    ClassDB::bind_method(D_METHOD("get_tcu_type"), &UtteranceResource::get_tcu_type);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "tcu_type", PROPERTY_HINT_ENUM, "Undefined:0,Lexical:1,Phrasal:2,Clausal:3,Sentential:4"),
        "set_tcu_type",
        "get_tcu_type"
    );

    ClassDB::bind_method(D_METHOD("set_resolution_type", "resolution_type"), &UtteranceResource::set_resolution_type);
    ClassDB::bind_method(D_METHOD("get_resolution_type"), &UtteranceResource::get_resolution_type);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "resolution_type", PROPERTY_HINT_ENUM, "Unresolved:0,Intonational:1,Grammatical:2,Pragmatical:3"),
        "set_resolution_type",
        "get_resolution_type"
    );

    ADD_GROUP("Voice & TTS Synthesis", "");
    ClassDB::bind_method(D_METHOD("set_voice_name", "voice_name"), &UtteranceResource::set_voice_name);
    ClassDB::bind_method(D_METHOD("get_voice_name"), &UtteranceResource::get_voice_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "voice_name"), "set_voice_name", "get_voice_name");

    ClassDB::bind_method(D_METHOD("set_gender", "gender"), &UtteranceResource::set_gender);
    ClassDB::bind_method(D_METHOD("get_gender"), &UtteranceResource::get_gender);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "gender", PROPERTY_HINT_ENUM, "Unspecified:0,Female:1,Male:2,Neutral:3"), "set_gender", "get_gender");

    ClassDB::bind_method(D_METHOD("set_age", "age"), &UtteranceResource::set_age);
    ClassDB::bind_method(D_METHOD("get_age"), &UtteranceResource::get_age);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "age", PROPERTY_HINT_RANGE, "0,120,1"), "set_age", "get_age");

    ClassDB::bind_method(D_METHOD("set_variant", "variant"), &UtteranceResource::set_variant);
    ClassDB::bind_method(D_METHOD("get_variant"), &UtteranceResource::get_variant);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "variant"), "set_variant", "get_variant");

    ClassDB::bind_method(D_METHOD("set_desired_languages", "languages"), &UtteranceResource::set_desired_languages);
    ClassDB::bind_method(D_METHOD("get_desired_languages"), &UtteranceResource::get_desired_languages);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "desired_languages"), "set_desired_languages", "get_desired_languages");

    ClassDB::bind_method(D_METHOD("set_volume", "volume"), &UtteranceResource::set_volume);
    ClassDB::bind_method(D_METHOD("get_volume"), &UtteranceResource::get_volume);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "volume", PROPERTY_HINT_RANGE, "0,100,1"), "set_volume", "get_volume");

    ClassDB::bind_method(D_METHOD("set_pitch", "pitch"), &UtteranceResource::set_pitch);
    ClassDB::bind_method(D_METHOD("get_pitch"), &UtteranceResource::get_pitch);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "pitch", PROPERTY_HINT_RANGE, "0.0,2.0,0.05"), "set_pitch", "get_pitch");

    ClassDB::bind_method(D_METHOD("set_rate", "rate"), &UtteranceResource::set_rate);
    ClassDB::bind_method(D_METHOD("get_rate"), &UtteranceResource::get_rate);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "rate", PROPERTY_HINT_RANGE, "0.1,10.0,0.05"), "set_rate", "get_rate");

    ClassDB::bind_method(D_METHOD("set_utterance_id", "utterance_id"), &UtteranceResource::set_utterance_id);
    ClassDB::bind_method(D_METHOD("get_utterance_id"), &UtteranceResource::get_utterance_id);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "utterance_id"), "set_utterance_id", "get_utterance_id");

    ClassDB::bind_method(D_METHOD("set_interrupt", "interrupt"), &UtteranceResource::set_interrupt);
    ClassDB::bind_method(D_METHOD("get_interrupt"), &UtteranceResource::get_interrupt);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "interrupt"), "set_interrupt", "get_interrupt");

    ADD_GROUP("Linguistic & Epistemic Binding", "");
    ClassDB::bind_method(D_METHOD("set_source_fact_id", "fact_id"), &UtteranceResource::set_source_fact_id);
    ClassDB::bind_method(D_METHOD("get_source_fact_id"), &UtteranceResource::get_source_fact_id);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "source_fact_id"), "set_source_fact_id", "get_source_fact_id");

    ClassDB::bind_method(D_METHOD("set_expected_sememe_id", "sememe_id"), &UtteranceResource::set_expected_sememe_id);
    ClassDB::bind_method(D_METHOD("get_expected_sememe_id"), &UtteranceResource::get_expected_sememe_id);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "expected_sememe_id"), "set_expected_sememe_id", "get_expected_sememe_id");

    ClassDB::bind_method(D_METHOD("set_active_lect_id", "lect_id"), &UtteranceResource::set_active_lect_id);
    ClassDB::bind_method(D_METHOD("get_active_lect_id"), &UtteranceResource::get_active_lect_id);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "active_lect_id"), "set_active_lect_id", "get_active_lect_id");

    ClassDB::bind_method(D_METHOD("set_phoneme_masks", "masks"), &UtteranceResource::set_phoneme_masks);
    ClassDB::bind_method(D_METHOD("get_phoneme_masks"), &UtteranceResource::get_phoneme_masks);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT32_ARRAY, "phoneme_masks"), "set_phoneme_masks", "get_phoneme_masks");

    ClassDB::bind_method(D_METHOD("set_semantic_tags", "tags"), &UtteranceResource::set_semantic_tags);
    ClassDB::bind_method(D_METHOD("get_semantic_tags"), &UtteranceResource::get_semantic_tags);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "semantic_tags", PROPERTY_HINT_RESOURCE_TYPE, "SemanticTagSetResource"),
        "set_semantic_tags",
        "get_semantic_tags"
    );

    ClassDB::bind_method(D_METHOD("speak"), &UtteranceResource::speak);
    ClassDB::bind_method(D_METHOD("stop_speaking"), &UtteranceResource::stop_speaking);
    ClassDB::bind_method(D_METHOD("sample_prosody_at", "normalized_time"), &UtteranceResource::sample_prosody_at);
    ClassDB::bind_method(D_METHOD("parse_from_ssml", "ssml"), &UtteranceResource::parse_from_ssml);
    ClassDB::bind_method(D_METHOD("export_to_ssml"), &UtteranceResource::export_to_ssml);

    ClassDB::bind_method(D_METHOD("get_snapshot_state"), &UtteranceResource::get_snapshot_state);
    ClassDB::bind_method(D_METHOD("apply_snapshot_state", "state"), &UtteranceResource::apply_snapshot_state);
    ClassDB::bind_method(
        D_METHOD("commit_utterance_edit", "new_text", "new_pitch", "new_rate", "new_volume", "action_name"),
        &UtteranceResource::commit_utterance_edit,
        DEFVAL("Edit Utterance Resource")
    );
}

void UtteranceResource::speak() const {
    if (raw_text_.is_empty()) return;
    if (godot::DisplayServer *ds = godot::DisplayServer::get_singleton()) {
        ds->tts_speak(raw_text_, voice_name_, volume_, pitch_, rate_, utterance_id_, interrupt_);
    }
}

void UtteranceResource::stop_speaking() const {
    if (godot::DisplayServer *ds = godot::DisplayServer::get_singleton()) {
        ds->tts_stop();
    }
}

godot::Vector2 UtteranceResource::sample_prosody_at(float p_normalized_time) const {
    const float t = std::clamp(p_normalized_time, 0.0f, 1.0f);
    if (prosody_spline_.is_valid() && prosody_spline_->get_point_count() > 0) {
        const godot::Vector2 pt = prosody_spline_->sample_normalized(t);
        return godot::Vector2(std::max(0.05f, pt.x), std::clamp(pt.y, 0.25f, 2.5f));
    }
    return godot::Vector2(static_cast<float>(volume_) / 100.0f, pitch_);
}

core::ProsodySamplePOD UtteranceResource::sample_prosody_pod_at(float p_normalized_time) const {
    const godot::Vector2 vp = sample_prosody_at(p_normalized_time);
    core::ProsodySamplePOD pod{};
    pod.volume_gain      = vp.x;
    pod.pitch_multiplier = vp.y;
    pod.speaking_rate    = rate_;
    pod.normalized_time  = std::clamp(p_normalized_time, 0.0f, 1.0f);
    return pod;
}

void UtteranceResource::parse_from_ssml(const godot::String &p_ssml) {
    const std::string utf8 = p_ssml.utf8().get_data();
    core::SemanticLexiconCore default_lex;
    default_lex.populate_default_core_semes_and_morphemes();

    const core::SSMLParsedUtterance parsed =
        core::NoisyChannelTransceiver::parse_ssml_document(utf8, &default_lex);

    raw_text_   = godot::String::utf8(parsed.plain_text.c_str());
    voice_name_ = godot::String::utf8(parsed.voice_name.c_str());
    gender_     = parsed.gender;
    age_        = parsed.age;
    variant_    = parsed.variant;
    volume_     = parsed.volume;
    pitch_      = parsed.pitch;
    rate_       = parsed.rate;

    desired_languages_.clear();
    for (const std::string &lang : parsed.desired_languages) {
        desired_languages_.push_back(godot::String::utf8(lang.c_str()));
    }

    phoneme_masks_.clear();
    for (uint16_t mask : parsed.phoneme_masks) {
        phoneme_masks_.push_back(static_cast<int32_t>(mask));
    }

    if (!parsed.prosody_contour.empty()) {
        if (prosody_spline_.is_null()) {
            prosody_spline_.instantiate();
        }
        prosody_spline_->clear_points();
        for (const core::ProsodySamplePOD &pt : parsed.prosody_contour) {
            prosody_spline_->add_point(godot::Vector2(pt.volume_gain, pt.pitch_multiplier));
        }
    }
    emit_changed();
}

godot::String UtteranceResource::export_to_ssml() const {
    core::SSMLParsedUtterance pod{};
    pod.plain_text   = raw_text_.utf8().get_data();
    pod.voice_name   = voice_name_.utf8().get_data();
    pod.gender       = gender_;
    pod.age          = age_;
    pod.variant      = variant_;
    pod.volume       = volume_;
    pod.pitch        = pitch_;
    pod.rate         = rate_;
    pod.utterance_id = utterance_id_;
    pod.interrupt    = interrupt_;
    for (int i = 0; i < desired_languages_.size(); ++i) {
        pod.desired_languages.push_back(desired_languages_[i].utf8().get_data());
    }
    const std::string ssml = core::NoisyChannelTransceiver::export_to_ssml_document(pod);
    return godot::String::utf8(ssml.c_str());
}

godot::Dictionary UtteranceResource::get_snapshot_state() const {
    godot::Dictionary d;
    d["raw_text"]           = raw_text_;
    d["tcu_type"]           = tcu_type_;
    d["resolution_type"]    = resolution_type_;
    d["voice_name"]         = voice_name_;
    d["gender"]             = gender_;
    d["age"]                = age_;
    d["variant"]            = variant_;
    d["desired_languages"]  = desired_languages_;
    d["volume"]             = volume_;
    d["pitch"]              = pitch_;
    d["rate"]               = rate_;
    d["utterance_id"]       = utterance_id_;
    d["interrupt"]          = interrupt_;
    d["source_fact_id"]     = source_fact_id_;
    d["expected_sememe_id"] = expected_sememe_id_;
    d["active_lect_id"]     = active_lect_id_;
    d["phoneme_masks"]      = phoneme_masks_;
    return d;
}

void UtteranceResource::apply_snapshot_state(const godot::Dictionary &p_state) {
    if (p_state.has("raw_text"))           raw_text_           = p_state["raw_text"];
    if (p_state.has("tcu_type"))           tcu_type_           = static_cast<int>(p_state["tcu_type"]);
    if (p_state.has("resolution_type"))    resolution_type_    = static_cast<int>(p_state["resolution_type"]);
    if (p_state.has("voice_name"))         voice_name_         = p_state["voice_name"];
    if (p_state.has("gender"))             gender_             = static_cast<int>(p_state["gender"]);
    if (p_state.has("age"))                age_                = static_cast<int>(p_state["age"]);
    if (p_state.has("variant"))            variant_            = static_cast<int>(p_state["variant"]);
    if (p_state.has("desired_languages"))  desired_languages_  = p_state["desired_languages"];
    if (p_state.has("volume"))             volume_             = static_cast<int>(p_state["volume"]);
    if (p_state.has("pitch"))              pitch_              = static_cast<float>(p_state["pitch"]);
    if (p_state.has("rate"))               rate_               = static_cast<float>(p_state["rate"]);
    if (p_state.has("utterance_id"))       utterance_id_       = static_cast<int>(p_state["utterance_id"]);
    if (p_state.has("interrupt"))          interrupt_          = static_cast<bool>(p_state["interrupt"]);
    if (p_state.has("source_fact_id"))     source_fact_id_     = static_cast<int>(p_state["source_fact_id"]);
    if (p_state.has("expected_sememe_id")) expected_sememe_id_ = static_cast<int>(p_state["expected_sememe_id"]);
    if (p_state.has("active_lect_id"))     active_lect_id_     = static_cast<int>(p_state["active_lect_id"]);
    if (p_state.has("phoneme_masks"))      phoneme_masks_      = p_state["phoneme_masks"];
    emit_changed();
}

void UtteranceResource::commit_snapshot_mutation(
    const godot::String &p_action_name,
    const godot::Dictionary &p_before_state
) {
    if (undo_redo_.is_null()) {
        undo_redo_ = IdeamCommunicationsPlugin::create_undo_redo_wrapper();
    }
    if (undo_redo_.is_valid()) {
        const godot::Dictionary after_state = get_snapshot_state();
        undo_redo_->create_action(p_action_name);
        undo_redo_->add_do_method(this, "apply_snapshot_state", after_state);
        undo_redo_->add_undo_method(this, "apply_snapshot_state", p_before_state);
        undo_redo_->commit_action(false);
    }
}

void UtteranceResource::commit_utterance_edit(
    const godot::String &p_new_text,
    float p_new_pitch,
    float p_new_rate,
    int p_new_volume,
    const godot::String &p_action_name
) {
    const godot::Dictionary before = get_snapshot_state();
    raw_text_ = p_new_text;
    pitch_    = std::clamp(p_new_pitch, 0.0f, 2.0f);
    rate_     = std::clamp(p_new_rate, 0.1f, 10.0f);
    volume_   = std::clamp(p_new_volume, 0, 100);
    emit_changed();
    commit_snapshot_mutation(p_action_name, before);
}

// ============================================================================
// SSMLParser
// ============================================================================

void SSMLParser::_bind_methods() {
    using namespace godot;
    ClassDB::bind_static_method("SSMLParser", D_METHOD("parse_ssml_to_utterance", "ssml"), &SSMLParser::parse_ssml_to_utterance);
    ClassDB::bind_static_method("SSMLParser", D_METHOD("export_utterance_to_ssml", "utterance"), &SSMLParser::export_utterance_to_ssml);
    ClassDB::bind_static_method("SSMLParser", D_METHOD("extract_phoneme_masks_from_ssml", "ssml"), &SSMLParser::extract_phoneme_masks_from_ssml);
}

godot::Ref<UtteranceResource> SSMLParser::parse_ssml_to_utterance(const godot::String &p_ssml) {
    godot::Ref<UtteranceResource> utt;
    utt.instantiate();
    utt->parse_from_ssml(p_ssml);
    return utt;
}

godot::String SSMLParser::export_utterance_to_ssml(const godot::Ref<UtteranceResource> &p_utterance) {
    if (p_utterance.is_null()) return godot::String();
    return p_utterance->export_to_ssml();
}

godot::PackedInt32Array SSMLParser::extract_phoneme_masks_from_ssml(const godot::String &p_ssml) {
    const godot::Ref<UtteranceResource> utt = parse_ssml_to_utterance(p_ssml);
    return utt.is_valid() ? utt->get_phoneme_masks() : godot::PackedInt32Array();
}

// ============================================================================
// DiscourseResource
// ============================================================================

void DiscourseResource::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_medium", "medium"), &DiscourseResource::set_medium);
    ClassDB::bind_method(D_METHOD("get_medium"), &DiscourseResource::get_medium);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "medium", PROPERTY_HINT_RESOURCE_TYPE, "CommunicationMediumResource"),
        "set_medium",
        "get_medium"
    );

    ClassDB::bind_method(D_METHOD("set_utterances", "utterances"), &DiscourseResource::set_utterances);
    ClassDB::bind_method(D_METHOD("get_utterances"), &DiscourseResource::get_utterances);
    ADD_PROPERTY(
        PropertyInfo(Variant::ARRAY, "utterances", PROPERTY_HINT_TYPE_STRING, String::num(Variant::OBJECT) + "/" + String::num(PROPERTY_HINT_RESOURCE_TYPE) + ":UtteranceResource"),
        "set_utterances",
        "get_utterances"
    );

    ClassDB::bind_method(D_METHOD("set_participants", "participants"), &DiscourseResource::set_participants);
    ClassDB::bind_method(D_METHOD("get_participants"), &DiscourseResource::get_participants);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "participants"), "set_participants", "get_participants");

    ClassDB::bind_method(D_METHOD("set_current_turn", "current_turn"), &DiscourseResource::set_current_turn);
    ClassDB::bind_method(D_METHOD("get_current_turn"), &DiscourseResource::get_current_turn);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "current_turn"), "set_current_turn", "get_current_turn");

    ClassDB::bind_method(D_METHOD("set_current_speaker_id", "speaker_id"), &DiscourseResource::set_current_speaker_id);
    ClassDB::bind_method(D_METHOD("get_current_speaker_id"), &DiscourseResource::get_current_speaker_id);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "current_speaker_id"), "set_current_speaker_id", "get_current_speaker_id");

    ClassDB::bind_method(D_METHOD("set_next_selected_speaker_id", "speaker_id"), &DiscourseResource::set_next_selected_speaker_id);
    ClassDB::bind_method(D_METHOD("get_next_selected_speaker_id"), &DiscourseResource::get_next_selected_speaker_id);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "next_selected_speaker_id"), "set_next_selected_speaker_id", "get_next_selected_speaker_id");

    ClassDB::bind_method(D_METHOD("get_last_applied_rule"), &DiscourseResource::get_last_applied_rule);
    ClassDB::bind_method(D_METHOD("is_discourse_active"), &DiscourseResource::is_discourse_active);

    ClassDB::bind_method(D_METHOD("set_semantic_tags", "tags"), &DiscourseResource::set_semantic_tags);
    ClassDB::bind_method(D_METHOD("get_semantic_tags"), &DiscourseResource::get_semantic_tags);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "semantic_tags", PROPERTY_HINT_RESOURCE_TYPE, "SemanticTagSetResource"),
        "set_semantic_tags",
        "get_semantic_tags"
    );

    ClassDB::bind_method(D_METHOD("begin_discourse", "initial_speaker_id"), &DiscourseResource::begin_discourse, DEFVAL(-1));
    ClassDB::bind_method(D_METHOD("conclude_discourse"), &DiscourseResource::conclude_discourse);
    ClassDB::bind_method(
        D_METHOD("introduce_participant", "participant_id", "mind_agent_id", "excitation_urgency", "turn_claim_priority", "addressed_target_id", "wants_floor"),
        &DiscourseResource::introduce_participant,
        DEFVAL(-1), DEFVAL(0.0f), DEFVAL(0.0f), DEFVAL(-1), DEFVAL(false)
    );
    ClassDB::bind_method(D_METHOD("dismiss_participant", "participant_id"), &DiscourseResource::dismiss_participant);
    ClassDB::bind_method(
        D_METHOD("evaluate_next_turn", "prosody_pitch_delta", "interruption_margin_db"),
        &DiscourseResource::evaluate_next_turn,
        DEFVAL(-0.25f), DEFVAL(0.0f)
    );

    ClassDB::bind_method(D_METHOD("get_snapshot_state"), &DiscourseResource::get_snapshot_state);
    ClassDB::bind_method(D_METHOD("apply_snapshot_state", "state"), &DiscourseResource::apply_snapshot_state);
    ClassDB::bind_method(
        D_METHOD("commit_discourse_turn", "next_speaker_id", "action_name"),
        &DiscourseResource::commit_discourse_turn,
        DEFVAL("Advance Discourse Turn")
    );

    ADD_SIGNAL(MethodInfo("discourse_begun", PropertyInfo(Variant::INT, "initial_speaker_id")));
    ADD_SIGNAL(MethodInfo("discourse_concluded"));
    ADD_SIGNAL(MethodInfo("participant_joined", PropertyInfo(Variant::INT, "participant_id")));
    ADD_SIGNAL(MethodInfo("participant_left", PropertyInfo(Variant::INT, "participant_id")));
    ADD_SIGNAL(MethodInfo("turn_allocated", PropertyInfo(Variant::INT, "turn_index"), PropertyInfo(Variant::INT, "speaker_id"), PropertyInfo(Variant::INT, "rule_id")));
}

void DiscourseResource::begin_discourse(int p_initial_speaker_id) {
    is_active_         = true;
    current_turn_      = 0;
    active_tcu_index_  = 0;
    if (p_initial_speaker_id >= 0) {
        current_speaker_id_ = p_initial_speaker_id;
    } else if (!participants_.is_empty()) {
        const godot::Dictionary first = participants_[0];
        current_speaker_id_ = first.get("participant_id", 0);
    }
    emit_signal("discourse_begun", current_speaker_id_);
    emit_changed();
}

void DiscourseResource::conclude_discourse() {
    is_active_ = false;
    emit_signal("discourse_concluded");
    emit_changed();
}

void DiscourseResource::introduce_participant(
    int p_participant_id,
    int p_mind_agent_id,
    float p_excitation_urgency,
    float p_turn_claim_priority,
    int p_addressed_target_id,
    bool p_wants_floor
) {
    for (int i = 0; i < participants_.size(); ++i) {
        godot::Dictionary d = participants_[i];
        if (static_cast<int>(d.get("participant_id", -1)) == p_participant_id) {
            d["mind_agent_id"]       = p_mind_agent_id;
            d["excitation_urgency"]  = p_excitation_urgency;
            d["turn_claim_priority"] = p_turn_claim_priority;
            d["addressed_target_id"] = p_addressed_target_id;
            d["wants_floor"]         = p_wants_floor;
            participants_[i]         = d;
            emit_changed();
            return;
        }
    }

    godot::Dictionary d;
    d["participant_id"]      = p_participant_id;
    d["mind_agent_id"]       = p_mind_agent_id;
    d["excitation_urgency"]  = p_excitation_urgency;
    d["turn_claim_priority"] = p_turn_claim_priority;
    d["addressed_target_id"] = p_addressed_target_id;
    d["wants_floor"]         = p_wants_floor;
    d["is_speaking"]         = (p_participant_id == current_speaker_id_);
    participants_.push_back(d);
    emit_signal("participant_joined", p_participant_id);
    emit_changed();
}

void DiscourseResource::dismiss_participant(int p_participant_id) {
    for (int i = 0; i < participants_.size(); ++i) {
        const godot::Dictionary d = participants_[i];
        if (static_cast<int>(d.get("participant_id", -1)) == p_participant_id) {
            participants_.remove_at(i);
            emit_signal("participant_left", p_participant_id);
            emit_changed();
            return;
        }
    }
}

core::DiscourseTurnStatePOD DiscourseResource::to_turn_state_pod() const noexcept {
    core::DiscourseTurnStatePOD state{};
    state.current_speaker_id       = (current_speaker_id_ >= 0) ? static_cast<uint32_t>(current_speaker_id_) : 0xFFFFFFFFu;
    state.next_selected_speaker_id = (next_selected_speaker_id_ >= 0) ? static_cast<uint32_t>(next_selected_speaker_id_) : 0xFFFFFFFFu;
    state.current_turn_index       = static_cast<uint32_t>(std::max(0, current_turn_));
    state.active_tcu_index         = static_cast<uint16_t>(std::max(0, active_tcu_index_));
    state.tcu_count                = static_cast<uint16_t>(std::max<int64_t>(1, utterances_.size()));
    return state;
}

void DiscourseResource::extract_participant_pods(std::vector<core::DiscourseParticipantPOD> &r_out) const {
    r_out.clear();
    r_out.reserve(static_cast<size_t>(participants_.size()));
    for (int i = 0; i < participants_.size(); ++i) {
        const godot::Dictionary d = participants_[i];
        core::DiscourseParticipantPOD p{};
        p.participant_id      = static_cast<uint32_t>(std::max(0, static_cast<int>(d.get("participant_id", i))));
        const int mind_id     = static_cast<int>(d.get("mind_agent_id", -1));
        p.mind_agent_id       = (mind_id >= 0) ? static_cast<uint32_t>(mind_id) : 0xFFFFFFFFu;
        p.excitation_urgency  = static_cast<float>(d.get("excitation_urgency", 0.0f));
        p.turn_claim_priority = static_cast<float>(d.get("turn_claim_priority", 0.0f));
        const int addr_id     = static_cast<int>(d.get("addressed_target_id", -1));
        p.addressed_target_id = (addr_id >= 0) ? static_cast<uint32_t>(addr_id) : 0xFFFFFFFFu;
        p.wants_floor         = static_cast<bool>(d.get("wants_floor", false)) ? 1u : 0u;
        p.is_speaking         = (static_cast<int>(p.participant_id) == current_speaker_id_) ? 1u : 0u;
        r_out.push_back(p);
    }
}

int DiscourseResource::evaluate_next_turn(float p_prosody_pitch_delta, float p_interruption_margin_db) {
    core::DiscourseTurnStatePOD turn_state = to_turn_state_pod();
    turn_state.prosody_pitch_delta    = p_prosody_pitch_delta;
    turn_state.interruption_margin_db = p_interruption_margin_db;

    std::vector<core::DiscourseParticipantPOD> pods;
    extract_participant_pods(pods);

    const core::DiscourseAllocationOutcome rule =
        core::NoisyChannelTransceiver::evaluate_discourse_turn_allocation(turn_state, pods);

    current_turn_             = static_cast<int>(turn_state.current_turn_index);
    current_speaker_id_       = (turn_state.current_speaker_id != 0xFFFFFFFFu)
        ? static_cast<int>(turn_state.current_speaker_id)
        : -1;
    next_selected_speaker_id_ = (turn_state.next_selected_speaker_id != 0xFFFFFFFFu)
        ? static_cast<int>(turn_state.next_selected_speaker_id)
        : -1;
    active_tcu_index_         = static_cast<int>(turn_state.active_tcu_index);
    last_applied_rule_        = static_cast<int>(rule);

    emit_signal("turn_allocated", current_turn_, current_speaker_id_, last_applied_rule_);
    emit_changed();
    return last_applied_rule_;
}

godot::Dictionary DiscourseResource::get_snapshot_state() const {
    godot::Dictionary d;
    d["current_turn"]             = current_turn_;
    d["current_speaker_id"]       = current_speaker_id_;
    d["next_selected_speaker_id"] = next_selected_speaker_id_;
    d["active_tcu_index"]         = active_tcu_index_;
    d["last_applied_rule"]        = last_applied_rule_;
    d["participants"]             = participants_.duplicate(true);
    return d;
}

void DiscourseResource::apply_snapshot_state(const godot::Dictionary &p_state) {
    if (p_state.has("current_turn"))             current_turn_             = static_cast<int>(p_state["current_turn"]);
    if (p_state.has("current_speaker_id"))       current_speaker_id_       = static_cast<int>(p_state["current_speaker_id"]);
    if (p_state.has("next_selected_speaker_id")) next_selected_speaker_id_ = static_cast<int>(p_state["next_selected_speaker_id"]);
    if (p_state.has("active_tcu_index"))         active_tcu_index_         = static_cast<int>(p_state["active_tcu_index"]);
    if (p_state.has("last_applied_rule"))        last_applied_rule_        = static_cast<int>(p_state["last_applied_rule"]);
    if (p_state.has("participants"))             participants_             = p_state["participants"];
    emit_changed();
}

void DiscourseResource::commit_snapshot_mutation(
    const godot::String &p_action_name,
    const godot::Dictionary &p_before_state
) {
    if (undo_redo_.is_null()) {
        undo_redo_ = IdeamCommunicationsPlugin::create_undo_redo_wrapper();
    }
    if (undo_redo_.is_valid()) {
        const godot::Dictionary after_state = get_snapshot_state();
        undo_redo_->create_action(p_action_name);
        undo_redo_->add_do_method(this, "apply_snapshot_state", after_state);
        undo_redo_->add_undo_method(this, "apply_snapshot_state", p_before_state);
        undo_redo_->commit_action(false);
    }
}

void DiscourseResource::commit_discourse_turn(int p_next_speaker_id, const godot::String &p_action_name) {
    const godot::Dictionary before = get_snapshot_state();
    ++current_turn_;
    current_speaker_id_       = p_next_speaker_id;
    next_selected_speaker_id_ = p_next_speaker_id;
    emit_changed();
    commit_snapshot_mutation(p_action_name, before);
}

} // namespace ideam::godot_ext
