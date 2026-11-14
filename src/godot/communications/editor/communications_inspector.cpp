#include "communications_inspector.h"
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

namespace ideam::godot_ext {

void CommunicationsInspector::_bind_methods() {}

bool CommunicationsInspector::_can_handle(godot::Object *p_object) const {
    if (!p_object) return false;
    return godot::Object::cast_to<CommunicationMediumResource>(p_object) != nullptr ||
           godot::Object::cast_to<DiscourseResource>(p_object) != nullptr ||
           godot::Object::cast_to<UtteranceResource>(p_object) != nullptr ||
           godot::Object::cast_to<CommunicationTransmitterNode>(p_object) != nullptr ||
           godot::Object::cast_to<CommunicationReceiverNode>(p_object) != nullptr ||
           godot::Object::cast_to<CommunicationSpeechBox2D>(p_object) != nullptr ||
           godot::Object::cast_to<CommunicationSpeechBubble3D>(p_object) != nullptr;
}

void CommunicationsInspector::_parse_begin(godot::Object *p_object) {
    if (!p_object) return;

    auto *panel = memnew(godot::PanelContainer);
    auto *vbox  = memnew(godot::VBoxContainer);
    panel->add_child(vbox);

    auto *title = memnew(godot::Label);
    title->set_text("IdeamCommunications — Shannon-Weaver Diagnostics");
    vbox->add_child(title);

    if (auto *med = godot::Object::cast_to<CommunicationMediumResource>(p_object)) {
        const float snr_5m  = med->compute_effective_snr_db_preview(5.0f);
        const float snr_15m = med->compute_effective_snr_db_preview(15.0f);
        const int tier_5m   = med->classify_noise_tier_preview(snr_5m);
        const int tier_15m  = med->classify_noise_tier_preview(snr_15m);

        auto *info = memnew(godot::Label);
        info->set_text(
            "SNR @ 5m: " + godot::String::num(snr_5m, 1) + " dB (Tier " + godot::String::num_int64(tier_5m) + ") | " +
            "SNR @ 15m: " + godot::String::num(snr_15m, 1) + " dB (Tier " + godot::String::num_int64(tier_15m) + ")"
        );
        vbox->add_child(info);
    } else if (auto *utt = godot::Object::cast_to<UtteranceResource>(p_object)) {
        auto *info = memnew(godot::Label);
        info->set_text(
            "TCU Type: " + godot::String::num_int64(utt->get_tcu_type()) +
            " | Phoneme Masks: " + godot::String::num_int64(utt->get_phoneme_masks().size()) +
            " | Rate: " + godot::String::num(utt->get_rate(), 2) + "x"
        );
        vbox->add_child(info);
    } else if (auto *disc = godot::Object::cast_to<DiscourseResource>(p_object)) {
        auto *info = memnew(godot::Label);
        info->set_text(
            "Participants: " + godot::String::num_int64(disc->get_participants().size()) +
            " | Turn: " + godot::String::num_int64(disc->get_current_turn()) +
            " | Speaker ID: " + godot::String::num_int64(disc->get_current_speaker_id())
        );
        vbox->add_child(info);
    }

    add_custom_control(panel);
}

} // namespace ideam::godot_ext
