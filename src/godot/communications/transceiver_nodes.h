#pragma once

/**
 * transceiver_nodes.h
 * `CommunicationTransmitterNode`, `CommunicationReceiverNode`, `CommunicationSpeechBox2D`,
 * and `CommunicationSpeechBubble3D` (`src/godot/communications/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES (Master Plan Section 1.2)
 * ============================================================================
 * 1. `CommunicationTransmitterNode`:
 *    - Succeeds `Speaker.cs` and `Interlocutor` (`interlocutor.gd`):
 *    - Reads speaker RPG skills (`elocution_skill` / "Speak Clearly", `projection_power_db` /
 *      "Project Voice") and `AgentMindResource` emotional state to encode intended
 *      `FactPod` / `Sememe` propositions into `LexemePOD` -> `PhonemePOD` feature streams
 *      plus `Spline2DResource` prosody (`X = Volume, Y = Pitch`).
 * 2. `CommunicationReceiverNode`:
 *    - Succeeds `Listener.cs`:
 *    - Reads listener RPG skills (`listen_skill`, `perception_bonus_db`), receives
 *      noise-degraded `PhonemePOD` streams from a `CommunicationMediumResource`, decodes
 *      them against `LanguageAssetResource` and `AgentMindResource` priors, flags
 *      `HONEST_MISREFERENCE` (`FACT_FLAG_MISREFERENCED`) when phonetic bit-flips shift
 *      a decoded word, and routes the resulting text + prosody to bound
 *      `CommunicationSpeechBox2D` / `CommunicationSpeechBubble3D` presenters.
 * 3. `CommunicationSpeechBox2D` (`Control`):
 *    - Ports and upgrades `Dialogue_Box` (`old_gdscript_plugins/ideam_dialogue/scripts/dialogue_box.gd`):
 *      * Fixes the `main_text_type` setter node-free bug from `dialogue_box.gd` (line 55).
 *      * Replaces legacy `Tween.EaseType` / `TransitionType` reveal progression with
 *        `IdeamSplines::Spline1DResource` reveal curves.
 *      * Integrates `GameplayAccessibilityOption` closed-caption scaling (`caption_text_scale`,
 *        `caption_background_opacity`, `caption_speaker_labels`) plus `Spline2DResource`
 *        prosody font modulation.
 * 4. `CommunicationSpeechBubble3D` (`Label3D`):
 *    - Ports and upgrades `ExpandedLabel3D` (`old_gdscript_plugins/ideam_dialogue/scripts/expandedlabel3d.gd`)
 *      with real `visible_ratio` substring progression, `Spline1DResource` reveal timing,
 *      `Spline2DResource` prosody scale modulation, and `GameplayAccessibilityOption` support.
 */

#include "discourse_resource.h"
#include "../games/gameplay/gameplay_accessibility_option.h"
#include "../linguistics/lexicon_resource.h"
#include "../minds/agent_mind_resource.h"
#include "../minds/information_matrix_resource.h"
#include "../splines/spline_1d/spline_1d_resource.h"
#include "../splines/spline_2d/spline_2d_resource.h"
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/label3d.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/rich_text_label.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace ideam::godot_ext {

class CommunicationReceiverNode;
class CommunicationSpeechBox2D;
class CommunicationSpeechBubble3D;

class CommunicationTransmitterNode : public godot::Node {
    GDCLASS(CommunicationTransmitterNode, godot::Node)

private:
    int   transmitter_id_      = 0;
    int   mind_agent_id_       = -1;
    int   active_lect_id_      = 0;
    float elocution_skill_     = 1.0f;  ///< RPG "Speak Clearly" skill multiplier
    float projection_power_db_ = 65.0f; ///< RPG "Project Voice" source dB
    godot::Vector3 world_position_ = godot::Vector3(0.0f, 0.0f, 0.0f);
    godot::String speaker_display_name_ = "Speaker";

    godot::Ref<LanguageAssetResource> language_asset_;
    godot::Ref<AgentMindResource> speaker_mind_;
    godot::Ref<Spline2DResource> default_prosody_spline_;

protected:
    static void _bind_methods();

public:
    CommunicationTransmitterNode() = default;
    ~CommunicationTransmitterNode() override = default;

    void set_transmitter_id(int p_id) { transmitter_id_ = p_id; }
    [[nodiscard]] int get_transmitter_id() const noexcept { return transmitter_id_; }

    void set_mind_agent_id(int p_id) { mind_agent_id_ = p_id; }
    [[nodiscard]] int get_mind_agent_id() const noexcept { return mind_agent_id_; }

    void set_active_lect_id(int p_id) { active_lect_id_ = std::max(0, p_id); }
    [[nodiscard]] int get_active_lect_id() const noexcept { return active_lect_id_; }

    void set_elocution_skill(float p_skill) { elocution_skill_ = std::max(0.05f, p_skill); }
    [[nodiscard]] float get_elocution_skill() const noexcept { return elocution_skill_; }

    void set_projection_power_db(float p_db) { projection_power_db_ = p_db; }
    [[nodiscard]] float get_projection_power_db() const noexcept { return projection_power_db_; }

    void set_world_position(const godot::Vector3 &p_pos) { world_position_ = p_pos; }
    [[nodiscard]] godot::Vector3 get_world_position() const noexcept { return world_position_; }

    void set_speaker_display_name(const godot::String &p_name) { speaker_display_name_ = p_name; }
    [[nodiscard]] godot::String get_speaker_display_name() const { return speaker_display_name_; }

    void set_language_asset(const godot::Ref<LanguageAssetResource> &p_asset) { language_asset_ = p_asset; }
    [[nodiscard]] godot::Ref<LanguageAssetResource> get_language_asset() const { return language_asset_; }

    void set_speaker_mind(const godot::Ref<AgentMindResource> &p_mind) { speaker_mind_ = p_mind; }
    [[nodiscard]] godot::Ref<AgentMindResource> get_speaker_mind() const { return speaker_mind_; }

    void set_default_prosody_spline(const godot::Ref<Spline2DResource> &p_spline) { default_prosody_spline_ = p_spline; }
    [[nodiscard]] godot::Ref<Spline2DResource> get_default_prosody_spline() const { return default_prosody_spline_; }

    [[nodiscard]] core::TransmitterStatePOD to_pod() const noexcept;

    /**
     * Executes the 6-stage Shannon-Weaver transmission of `p_utterance` through `p_medium`
     * to `p_receiver`, returning a Dictionary summary of the transmitted & decoded packet.
     */
    godot::Dictionary transmit_utterance(
        const godot::Ref<UtteranceResource> &p_utterance,
        const godot::Ref<CommunicationMediumResource> &p_medium,
        CommunicationReceiverNode *p_receiver,
        int64_t p_random_seed = 1337
    );
};

class CommunicationReceiverNode : public godot::Node {
    GDCLASS(CommunicationReceiverNode, godot::Node)

private:
    int   receiver_id_         = 0;
    int   mind_agent_id_       = -1;
    int   expected_sememe_id_  = -1;
    float listen_skill_        = 1.0f; ///< RPG "Listen" skill multiplier
    float perception_bonus_db_ = 0.0f; ///< RPG "Perception" attribute SNR bonus in dB
    godot::Vector3 world_position_ = godot::Vector3(0.0f, 0.0f, 0.0f);
    bool  auto_speak_tts_      = false;

    godot::Ref<LanguageAssetResource> language_asset_;
    godot::Ref<AgentMindResource> listener_mind_;
    godot::Ref<InformationMatrixResource> akashic_tome_;
    godot::NodePath speech_box_2d_path_;
    godot::NodePath speech_bubble_3d_path_;

    godot::Dictionary last_received_packet_;

protected:
    static void _bind_methods();

public:
    CommunicationReceiverNode() = default;
    ~CommunicationReceiverNode() override = default;

    void set_receiver_id(int p_id) { receiver_id_ = p_id; }
    [[nodiscard]] int get_receiver_id() const noexcept { return receiver_id_; }

    void set_mind_agent_id(int p_id) { mind_agent_id_ = p_id; }
    [[nodiscard]] int get_mind_agent_id() const noexcept { return mind_agent_id_; }

    void set_expected_sememe_id(int p_id) { expected_sememe_id_ = p_id; }
    [[nodiscard]] int get_expected_sememe_id() const noexcept { return expected_sememe_id_; }

    void set_listen_skill(float p_skill) { listen_skill_ = std::max(0.05f, p_skill); }
    [[nodiscard]] float get_listen_skill() const noexcept { return listen_skill_; }

    void set_perception_bonus_db(float p_db) { perception_bonus_db_ = p_db; }
    [[nodiscard]] float get_perception_bonus_db() const noexcept { return perception_bonus_db_; }

    void set_world_position(const godot::Vector3 &p_pos) { world_position_ = p_pos; }
    [[nodiscard]] godot::Vector3 get_world_position() const noexcept { return world_position_; }

    void set_auto_speak_tts(bool p_enable) { auto_speak_tts_ = p_enable; }
    [[nodiscard]] bool get_auto_speak_tts() const noexcept { return auto_speak_tts_; }

    void set_language_asset(const godot::Ref<LanguageAssetResource> &p_asset) { language_asset_ = p_asset; }
    [[nodiscard]] godot::Ref<LanguageAssetResource> get_language_asset() const { return language_asset_; }

    void set_listener_mind(const godot::Ref<AgentMindResource> &p_mind) { listener_mind_ = p_mind; }
    [[nodiscard]] godot::Ref<AgentMindResource> get_listener_mind() const { return listener_mind_; }

    void set_akashic_tome(const godot::Ref<InformationMatrixResource> &p_tome) { akashic_tome_ = p_tome; }
    [[nodiscard]] godot::Ref<InformationMatrixResource> get_akashic_tome() const { return akashic_tome_; }

    void set_speech_box_2d_path(const godot::NodePath &p_path) { speech_box_2d_path_ = p_path; }
    [[nodiscard]] godot::NodePath get_speech_box_2d_path() const { return speech_box_2d_path_; }

    void set_speech_bubble_3d_path(const godot::NodePath &p_path) { speech_bubble_3d_path_ = p_path; }
    [[nodiscard]] godot::NodePath get_speech_bubble_3d_path() const { return speech_bubble_3d_path_; }

    [[nodiscard]] godot::Dictionary get_last_received_packet() const { return last_received_packet_; }
    [[nodiscard]] core::ReceiverStatePOD to_pod() const noexcept;

    /**
     * Receives and processes a decoded utterance result from `CommunicationTransmitterNode`
     * or `DataOrientedCommunications`, routing the reconstructed text + prosody to any bound
     * `CommunicationSpeechBox2D` or `CommunicationSpeechBubble3D` presenter.
     */
    godot::Dictionary receive_decoded_result(
        const core::DecodedUtteranceResult &p_result,
        const godot::String &p_speaker_name = "Speaker",
        const godot::Ref<UtteranceResource> &p_utterance = godot::Ref<UtteranceResource>()
    );
};

/**
 * CommunicationSpeechBox2D (`Control`)
 * Runtime 2D speech-box presenter replacing `old_gdscript_plugins/ideam_dialogue/scripts/dialogue_box.gd`.
 * Driven by `Spline1DResource` reveal curves, `Spline2DResource` prosody font modulation,
 * and `GameplayAccessibilityOption` closed-caption scaling.
 */
class CommunicationSpeechBox2D : public godot::Control {
    GDCLASS(CommunicationSpeechBox2D, godot::Control)

public:
    enum TextClass : int32_t {
        TEXT_CLASS_LABEL           = 0,
        TEXT_CLASS_RICH_TEXT_LABEL = 1
    };

private:
    int label_type_              = TEXT_CLASS_LABEL;
    int main_text_type_          = TEXT_CLASS_RICH_TEXT_LABEL;
    int interlocutor_id_         = 0;
    float reveal_rate_cps_       = 24.0f; ///< Characters per second
    float visible_ratio_         = 0.0f;
    float elapsed_seconds_       = 0.0f;
    float total_duration_seconds_= 1.0f;
    bool  is_revealing_          = false;

    godot::String speaker_name_  = "Speaker";
    godot::String full_text_;
    float base_font_size_        = 18.0f;
    float caption_text_scale_    = 1.0f;
    float caption_bg_opacity_    = 0.80f;
    bool  show_speaker_label_    = true;

    godot::Ref<Spline1DResource> reveal_curve_;
    godot::Ref<Spline2DResource> active_prosody_spline_;
    godot::Ref<GameplayAccessibilityOption> accessibility_option_;

    godot::Control *speaker_label_node_ = nullptr;
    godot::Control *main_text_node_     = nullptr;

    void _rebuild_speaker_label_node();
    void _rebuild_main_text_node();
    void _apply_text_and_ratio();

protected:
    static void _bind_methods();

public:
    CommunicationSpeechBox2D() = default;
    ~CommunicationSpeechBox2D() override = default;

    void _ready() override;
    void _process(double p_delta) override;

    void set_label_type(int p_type);
    [[nodiscard]] int get_label_type() const noexcept { return label_type_; }

    /**
     * Fixes the node-free bug from `dialogue_box.gd` line 55: frees `main_text_node_`,
     * never `speaker_label_node_`.
     */
    void set_main_text_type(int p_type);
    [[nodiscard]] int get_main_text_type() const noexcept { return main_text_type_; }

    void set_interlocutor_id(int p_id) { interlocutor_id_ = p_id; }
    [[nodiscard]] int get_interlocutor_id() const noexcept { return interlocutor_id_; }

    void set_reveal_rate_cps(float p_cps) { reveal_rate_cps_ = std::max(1.0f, p_cps); }
    [[nodiscard]] float get_reveal_rate_cps() const noexcept { return reveal_rate_cps_; }

    void set_visible_ratio(float p_ratio);
    [[nodiscard]] float get_visible_ratio() const noexcept { return visible_ratio_; }

    void set_speaker_name(const godot::String &p_name);
    [[nodiscard]] godot::String get_speaker_name() const { return speaker_name_; }

    void set_full_text(const godot::String &p_text);
    [[nodiscard]] godot::String get_full_text() const { return full_text_; }

    void set_reveal_curve(const godot::Ref<Spline1DResource> &p_curve) { reveal_curve_ = p_curve; }
    [[nodiscard]] godot::Ref<Spline1DResource> get_reveal_curve() const { return reveal_curve_; }

    void set_active_prosody_spline(const godot::Ref<Spline2DResource> &p_spline) { active_prosody_spline_ = p_spline; }
    [[nodiscard]] godot::Ref<Spline2DResource> get_active_prosody_spline() const { return active_prosody_spline_; }

    void set_accessibility_option(const godot::Ref<GameplayAccessibilityOption> &p_opt);
    [[nodiscard]] godot::Ref<GameplayAccessibilityOption> get_accessibility_option() const { return accessibility_option_; }

    // --- Presentation Lifecycle (`dialogue_box.gd` upgraded API) ---
    void open_dialogue_box();
    void close_dialogue_box();
    void display_utterance(
        const godot::String &p_speaker,
        const godot::String &p_decoded_text,
        float p_rate_cps = 24.0f,
        const godot::Ref<Spline2DResource> &p_prosody = godot::Ref<Spline2DResource>()
    );
    void update_dialogue(float p_normalized_progress);
    void stop_dialogue();
    void complete_dialogue();
    void fast_track_dialogue();
};

/**
 * CommunicationSpeechBubble3D (`Label3D`)
 * Runtime 3D speech-bubble presenter replacing `old_gdscript_plugins/ideam_dialogue/scripts/expandedlabel3d.gd`.
 */
class CommunicationSpeechBubble3D : public godot::Label3D {
    GDCLASS(CommunicationSpeechBubble3D, godot::Label3D)

private:
    godot::String speaker_name_;
    godot::String full_text_;
    float visible_ratio_          = 1.0f;
    float reveal_rate_cps_        = 24.0f;
    float elapsed_seconds_        = 0.0f;
    float total_duration_seconds_ = 1.0f;
    float base_pixel_size_        = 0.005f;
    float caption_text_scale_     = 1.0f;
    bool  show_speaker_label_     = true;
    bool  is_revealing_           = false;
    int   interlocutor_id_        = 0;

    godot::Ref<Spline1DResource> reveal_curve_;
    godot::Ref<Spline2DResource> active_prosody_spline_;
    godot::Ref<GameplayAccessibilityOption> accessibility_option_;

protected:
    static void _bind_methods();

public:
    CommunicationSpeechBubble3D() = default;
    ~CommunicationSpeechBubble3D() override = default;

    void _process(double p_delta) override;

    void set_speaker_name(const godot::String &p_name);
    [[nodiscard]] godot::String get_speaker_name() const { return speaker_name_; }

    void set_full_text(const godot::String &p_text);
    [[nodiscard]] godot::String get_full_text() const { return full_text_; }

    void set_visible_ratio(float p_ratio);
    [[nodiscard]] float get_visible_ratio() const noexcept { return visible_ratio_; }

    void set_reveal_rate_cps(float p_cps) { reveal_rate_cps_ = std::max(1.0f, p_cps); }
    [[nodiscard]] float get_reveal_rate_cps() const noexcept { return reveal_rate_cps_; }

    void set_reveal_curve(const godot::Ref<Spline1DResource> &p_curve) { reveal_curve_ = p_curve; }
    [[nodiscard]] godot::Ref<Spline1DResource> get_reveal_curve() const { return reveal_curve_; }

    void set_active_prosody_spline(const godot::Ref<Spline2DResource> &p_spline) { active_prosody_spline_ = p_spline; }
    [[nodiscard]] godot::Ref<Spline2DResource> get_active_prosody_spline() const { return active_prosody_spline_; }

    void set_accessibility_option(const godot::Ref<GameplayAccessibilityOption> &p_opt);
    [[nodiscard]] godot::Ref<GameplayAccessibilityOption> get_accessibility_option() const { return accessibility_option_; }

    void update_visible_ratio();
    void display_utterance(
        const godot::String &p_speaker,
        const godot::String &p_decoded_text,
        float p_rate_cps = 24.0f,
        const godot::Ref<Spline2DResource> &p_prosody = godot::Ref<Spline2DResource>()
    );
    void fast_track_dialogue();
};

} // namespace ideam::godot_ext
VARIANT_ENUM_CAST(ideam::godot_ext::CommunicationSpeechBox2D::TextClass);
