#include "communication_medium_resource.h"
#include "ideam_communications_plugin.h"
#include <algorithm>

namespace ideam::godot_ext {

void CommunicationMediumResource::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_medium_id", "medium_id"), &CommunicationMediumResource::set_medium_id);
    ClassDB::bind_method(D_METHOD("get_medium_id"), &CommunicationMediumResource::get_medium_id);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "medium_id"), "set_medium_id", "get_medium_id");

    ClassDB::bind_method(D_METHOD("set_ambient_noise_db", "noise_db"), &CommunicationMediumResource::set_ambient_noise_db);
    ClassDB::bind_method(D_METHOD("get_ambient_noise_db"), &CommunicationMediumResource::get_ambient_noise_db);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "ambient_noise_db", PROPERTY_HINT_RANGE, "0.0,140.0,0.5"), "set_ambient_noise_db", "get_ambient_noise_db");

    ClassDB::bind_method(D_METHOD("set_attenuation_db_per_m", "attenuation"), &CommunicationMediumResource::set_attenuation_db_per_m);
    ClassDB::bind_method(D_METHOD("get_attenuation_db_per_m"), &CommunicationMediumResource::get_attenuation_db_per_m);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "attenuation_db_per_m", PROPERTY_HINT_RANGE, "0.0,20.0,0.05"), "set_attenuation_db_per_m", "get_attenuation_db_per_m");

    ClassDB::bind_method(D_METHOD("set_occlusion_damping_db", "occlusion_db"), &CommunicationMediumResource::set_occlusion_damping_db);
    ClassDB::bind_method(D_METHOD("get_occlusion_damping_db"), &CommunicationMediumResource::get_occlusion_damping_db);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "occlusion_damping_db", PROPERTY_HINT_RANGE, "0.0,80.0,0.5"), "set_occlusion_damping_db", "get_occlusion_damping_db");

    ClassDB::bind_method(D_METHOD("set_place_flip_sensitivity", "sensitivity"), &CommunicationMediumResource::set_place_flip_sensitivity);
    ClassDB::bind_method(D_METHOD("get_place_flip_sensitivity"), &CommunicationMediumResource::get_place_flip_sensitivity);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "place_flip_sensitivity", PROPERTY_HINT_RANGE, "0.0,4.0,0.05"), "set_place_flip_sensitivity", "get_place_flip_sensitivity");

    ClassDB::bind_method(D_METHOD("set_manner_flip_sensitivity", "sensitivity"), &CommunicationMediumResource::set_manner_flip_sensitivity);
    ClassDB::bind_method(D_METHOD("get_manner_flip_sensitivity"), &CommunicationMediumResource::get_manner_flip_sensitivity);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "manner_flip_sensitivity", PROPERTY_HINT_RANGE, "0.0,4.0,0.05"), "set_manner_flip_sensitivity", "get_manner_flip_sensitivity");

    ClassDB::bind_method(D_METHOD("set_voicing_flip_sensitivity", "sensitivity"), &CommunicationMediumResource::set_voicing_flip_sensitivity);
    ClassDB::bind_method(D_METHOD("get_voicing_flip_sensitivity"), &CommunicationMediumResource::get_voicing_flip_sensitivity);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "voicing_flip_sensitivity", PROPERTY_HINT_RANGE, "0.0,4.0,0.05"), "set_voicing_flip_sensitivity", "get_voicing_flip_sensitivity");

    ClassDB::bind_method(D_METHOD("set_modality", "modality"), &CommunicationMediumResource::set_modality);
    ClassDB::bind_method(D_METHOD("get_modality"), &CommunicationMediumResource::get_modality);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "modality", PROPERTY_HINT_ENUM, "SpokenAcoustic:0,WrittenInscription:1,RadioElectromagnetic:2,NonVerbalGesture:3,CymaticResonance:4"),
        "set_modality",
        "get_modality"
    );

    ClassDB::bind_method(D_METHOD("set_reverb_intelligibility_penalty", "penalty"), &CommunicationMediumResource::set_reverb_intelligibility_penalty);
    ClassDB::bind_method(D_METHOD("get_reverb_intelligibility_penalty"), &CommunicationMediumResource::get_reverb_intelligibility_penalty);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "reverb_intelligibility_penalty", PROPERTY_HINT_RANGE, "0,255,1"), "set_reverb_intelligibility_penalty", "get_reverb_intelligibility_penalty");

    ClassDB::bind_method(D_METHOD("set_semantic_tags", "tags"), &CommunicationMediumResource::set_semantic_tags);
    ClassDB::bind_method(D_METHOD("get_semantic_tags"), &CommunicationMediumResource::get_semantic_tags);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "semantic_tags", PROPERTY_HINT_RESOURCE_TYPE, "SemanticTagSetResource"),
        "set_semantic_tags",
        "get_semantic_tags"
    );

    ClassDB::bind_method(
        D_METHOD("compute_effective_snr_db_preview", "distance_m", "projection_power_db", "elocution_skill", "listen_skill", "perception_bonus_db"),
        &CommunicationMediumResource::compute_effective_snr_db_preview,
        DEFVAL(65.0f), DEFVAL(1.0f), DEFVAL(1.0f), DEFVAL(0.0f)
    );
    ClassDB::bind_method(D_METHOD("classify_noise_tier_preview", "effective_snr_db"), &CommunicationMediumResource::classify_noise_tier_preview);

    ClassDB::bind_method(D_METHOD("get_snapshot_state"), &CommunicationMediumResource::get_snapshot_state);
    ClassDB::bind_method(D_METHOD("apply_snapshot_state", "state"), &CommunicationMediumResource::apply_snapshot_state);
    ClassDB::bind_method(
        D_METHOD("commit_noise_profile", "ambient_db", "attenuation_per_m", "occlusion_db", "action_name"),
        &CommunicationMediumResource::commit_noise_profile,
        DEFVAL("Edit Communication Medium Noise Profile")
    );
}

core::TransmissionMediumPOD CommunicationMediumResource::to_pod() const noexcept {
    core::TransmissionMediumPOD pod{};
    pod.medium_id                      = static_cast<uint32_t>(std::max(0, medium_id_));
    pod.ambient_noise_db               = ambient_noise_db_;
    pod.attenuation_db_per_m           = attenuation_db_per_m_;
    pod.occlusion_damping_db           = occlusion_damping_db_;
    pod.place_flip_sensitivity         = place_flip_sensitivity_;
    pod.manner_flip_sensitivity        = manner_flip_sensitivity_;
    pod.voicing_flip_sensitivity       = voicing_flip_sensitivity_;
    pod.modality                       = static_cast<core::CommunicationModality>(std::clamp(modality_, 0, 4));
    pod.reverb_intelligibility_penalty = static_cast<uint8_t>(std::clamp(reverb_intelligibility_penalty_, 0, 255));
    return pod;
}

void CommunicationMediumResource::from_pod(const core::TransmissionMediumPOD &p_pod) {
    medium_id_                      = static_cast<int>(p_pod.medium_id);
    ambient_noise_db_               = p_pod.ambient_noise_db;
    attenuation_db_per_m_           = p_pod.attenuation_db_per_m;
    occlusion_damping_db_           = p_pod.occlusion_damping_db;
    place_flip_sensitivity_         = p_pod.place_flip_sensitivity;
    manner_flip_sensitivity_        = p_pod.manner_flip_sensitivity;
    voicing_flip_sensitivity_       = p_pod.voicing_flip_sensitivity;
    modality_                       = static_cast<int>(p_pod.modality);
    reverb_intelligibility_penalty_ = static_cast<int>(p_pod.reverb_intelligibility_penalty);
    emit_changed();
}

float CommunicationMediumResource::compute_effective_snr_db_preview(
    float p_distance_m,
    float p_projection_power_db,
    float p_elocution_skill,
    float p_listen_skill,
    float p_perception_bonus_db
) const noexcept {
    core::TransmitterStatePOD tx{};
    tx.projection_power_db    = p_projection_power_db;
    tx.elocution_encode_skill = p_elocution_skill;
    tx.pos_x                  = 0.0f;

    core::ReceiverStatePOD rx{};
    rx.listen_decode_skill = p_listen_skill;
    rx.perception_bonus_db = p_perception_bonus_db;
    rx.pos_x               = std::max(0.0f, p_distance_m);

    return core::NoisyChannelTransceiver::compute_effective_snr_db(tx, to_pod(), rx, 1.0f);
}

int CommunicationMediumResource::classify_noise_tier_preview(float p_effective_snr_db) const noexcept {
    return static_cast<int>(core::NoisyChannelTransceiver::classify_noise_tier(p_effective_snr_db));
}

godot::Dictionary CommunicationMediumResource::get_snapshot_state() const {
    godot::Dictionary d;
    d["medium_id"]                      = medium_id_;
    d["ambient_noise_db"]               = ambient_noise_db_;
    d["attenuation_db_per_m"]           = attenuation_db_per_m_;
    d["occlusion_damping_db"]           = occlusion_damping_db_;
    d["place_flip_sensitivity"]         = place_flip_sensitivity_;
    d["manner_flip_sensitivity"]        = manner_flip_sensitivity_;
    d["voicing_flip_sensitivity"]       = voicing_flip_sensitivity_;
    d["modality"]                       = modality_;
    d["reverb_intelligibility_penalty"] = reverb_intelligibility_penalty_;
    return d;
}

void CommunicationMediumResource::apply_snapshot_state(const godot::Dictionary &p_state) {
    if (p_state.has("medium_id"))                      medium_id_                      = static_cast<int>(p_state["medium_id"]);
    if (p_state.has("ambient_noise_db"))               ambient_noise_db_               = static_cast<float>(p_state["ambient_noise_db"]);
    if (p_state.has("attenuation_db_per_m"))           attenuation_db_per_m_           = static_cast<float>(p_state["attenuation_db_per_m"]);
    if (p_state.has("occlusion_damping_db"))           occlusion_damping_db_           = static_cast<float>(p_state["occlusion_damping_db"]);
    if (p_state.has("place_flip_sensitivity"))         place_flip_sensitivity_         = static_cast<float>(p_state["place_flip_sensitivity"]);
    if (p_state.has("manner_flip_sensitivity"))        manner_flip_sensitivity_        = static_cast<float>(p_state["manner_flip_sensitivity"]);
    if (p_state.has("voicing_flip_sensitivity"))       voicing_flip_sensitivity_       = static_cast<float>(p_state["voicing_flip_sensitivity"]);
    if (p_state.has("modality"))                       modality_                       = static_cast<int>(p_state["modality"]);
    if (p_state.has("reverb_intelligibility_penalty")) reverb_intelligibility_penalty_ = static_cast<int>(p_state["reverb_intelligibility_penalty"]);
    emit_changed();
}

void CommunicationMediumResource::commit_snapshot_mutation(
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

void CommunicationMediumResource::commit_noise_profile(
    float p_ambient_db,
    float p_attenuation_per_m,
    float p_occlusion_db,
    const godot::String &p_action_name
) {
    const godot::Dictionary before = get_snapshot_state();
    ambient_noise_db_     = p_ambient_db;
    attenuation_db_per_m_ = p_attenuation_per_m;
    occlusion_damping_db_ = p_occlusion_db;
    emit_changed();
    commit_snapshot_mutation(p_action_name, before);
}

} // namespace ideam::godot_ext
