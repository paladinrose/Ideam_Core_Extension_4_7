#pragma once

/**
 * discourse_resource.h
 * `UtteranceResource`, `SSMLParser`, and `DiscourseResource` (`src/godot/communications/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES (Master Plan Section 1.2)
 * ============================================================================
 * 1. `UtteranceResource`:
 *    - Folds `Voiced_Text` (`old_gdscript_plugins/ideam_dialogue/scripts/voiced_text.gd`)
 *      and `Dialogue` (`dialogue.gd`) into a single C++26 `Resource`:
 *      * Surface `raw_text`, `Spline2DResource` prosody curve (`X = Volume, Y = Pitch`),
 *        `TurnConstructionUnitType` (`LEXICAL`, `PHRASAL`, `CLAUSAL`, `SENTENTIAL`),
 *        and `TurnResolutionType` (`UNRESOLVED`, `INTONATIONAL`, `GRAMMATICAL`, `PRAGMATICAL`).
 *      * Full `DisplayServer::tts_speak()` synthesis parameters (`voice_name`, `gender`,
 *        `age`, `variant`, `desired_languages`, `volume`, `pitch`, `rate`, `utterance_id`,
 *        `interrupt`).
 *      * Explicit `phoneme_masks` (`PackedInt32Array` of 16-bit PanPhon articulatory masks),
 *        `source_fact_id`, and `expected_sememe_id`.
 * 2. `SSMLParser`:
 *    - Replaces `ssml_parser.gd` stub with a C++26 `RefCounted` helper translating
 *      `<prosody pitch="..." rate="..." volume="...">` tags into `Spline2DResource`
 *      prosody envelopes and `<phoneme alphabet="ipa|arpabet" ph="...">` tags directly
 *      into `IdeamLinguistics::PhonemePOD` feature masks.
 * 3. `DiscourseResource`:
 *    - Implements the Sacks-Schegloff-Jefferson 3-rule turn allocation state machine
 *      from `Discourse.cs` (Rule 1a: Current speaker selects next speaker; Rule 1b:
 *      Listener self-selection by excitation/urgency; Rule 1c: Current speaker continues
 *      with next `TurnConstructionUnit`) and absorbs `conversation.gd` interlocutor roster
 *      management with `IdeamUndoRedo` support.
 */

#include "communication_medium_resource.h"
#include "../linguistics/lexicon_resource.h"
#include "../splines/spline_2d/spline_2d_resource.h"
#include "../utilities/ideam_undo_redo.h"
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <vector>

namespace ideam::godot_ext {

class UtteranceResource : public godot::Resource {
    GDCLASS(UtteranceResource, godot::Resource)

private:
    godot::String raw_text_;
    godot::Ref<Spline2DResource> prosody_spline_;
    int tcu_type_        = static_cast<int>(core::TurnConstructionUnitType::SENTENTIAL);
    int resolution_type_ = static_cast<int>(core::TurnResolutionType::UNRESOLVED);

    // --- Folded from Voiced_Text (`voiced_text.gd`) ---
    godot::String voice_name_;
    int gender_          = 0;
    int age_             = 0;
    int variant_         = 0;
    godot::PackedStringArray desired_languages_;
    int volume_          = 80;   ///< `[0..100]`
    float pitch_         = 1.0f; ///< `[0.0..2.0]`
    float rate_          = 1.0f; ///< `[0.1..10.0]`
    int utterance_id_    = 0;
    bool interrupt_      = false;

    // --- Linguistic & Epistemic Binding ---
    int source_fact_id_     = -1;
    int expected_sememe_id_ = -1;
    int active_lect_id_     = 0;
    godot::PackedInt32Array phoneme_masks_;
    godot::Ref<SemanticTagSetResource> semantic_tags_;
    godot::Ref<IdeamUndoRedo> undo_redo_;

protected:
    static void _bind_methods();

public:
    UtteranceResource() = default;
    ~UtteranceResource() override = default;

    void set_raw_text(const godot::String &p_text) { raw_text_ = p_text; emit_changed(); }
    [[nodiscard]] godot::String get_raw_text() const { return raw_text_; }

    void set_prosody_spline(const godot::Ref<Spline2DResource> &p_spline) { prosody_spline_ = p_spline; emit_changed(); }
    [[nodiscard]] godot::Ref<Spline2DResource> get_prosody_spline() const { return prosody_spline_; }

    void set_tcu_type(int p_type) { tcu_type_ = std::clamp(p_type, 0, 4); emit_changed(); }
    [[nodiscard]] int get_tcu_type() const noexcept { return tcu_type_; }

    void set_resolution_type(int p_type) { resolution_type_ = std::clamp(p_type, 0, 3); emit_changed(); }
    [[nodiscard]] int get_resolution_type() const noexcept { return resolution_type_; }

    // --- Voiced_Text TTS synthesis properties ---
    void set_voice_name(const godot::String &p_name) { voice_name_ = p_name; emit_changed(); }
    [[nodiscard]] godot::String get_voice_name() const { return voice_name_; }

    void set_gender(int p_gender) { gender_ = p_gender; emit_changed(); }
    [[nodiscard]] int get_gender() const noexcept { return gender_; }

    void set_age(int p_age) { age_ = std::max(0, p_age); emit_changed(); }
    [[nodiscard]] int get_age() const noexcept { return age_; }

    void set_variant(int p_variant) { variant_ = p_variant; emit_changed(); }
    [[nodiscard]] int get_variant() const noexcept { return variant_; }

    void set_desired_languages(const godot::PackedStringArray &p_langs) { desired_languages_ = p_langs; emit_changed(); }
    [[nodiscard]] godot::PackedStringArray get_desired_languages() const { return desired_languages_; }

    void set_volume(int p_vol) { volume_ = std::clamp(p_vol, 0, 100); emit_changed(); }
    [[nodiscard]] int get_volume() const noexcept { return volume_; }

    void set_pitch(float p_pitch) { pitch_ = std::clamp(p_pitch, 0.0f, 2.0f); emit_changed(); }
    [[nodiscard]] float get_pitch() const noexcept { return pitch_; }

    void set_rate(float p_rate) { rate_ = std::clamp(p_rate, 0.1f, 10.0f); emit_changed(); }
    [[nodiscard]] float get_rate() const noexcept { return rate_; }

    void set_utterance_id(int p_id) { utterance_id_ = p_id; emit_changed(); }
    [[nodiscard]] int get_utterance_id() const noexcept { return utterance_id_; }

    void set_interrupt(bool p_interrupt) { interrupt_ = p_interrupt; emit_changed(); }
    [[nodiscard]] bool get_interrupt() const noexcept { return interrupt_; }

    void set_source_fact_id(int p_fact_id) { source_fact_id_ = p_fact_id; emit_changed(); }
    [[nodiscard]] int get_source_fact_id() const noexcept { return source_fact_id_; }

    void set_expected_sememe_id(int p_sememe_id) { expected_sememe_id_ = p_sememe_id; emit_changed(); }
    [[nodiscard]] int get_expected_sememe_id() const noexcept { return expected_sememe_id_; }

    void set_active_lect_id(int p_lect_id) { active_lect_id_ = std::max(0, p_lect_id); emit_changed(); }
    [[nodiscard]] int get_active_lect_id() const noexcept { return active_lect_id_; }

    void set_phoneme_masks(const godot::PackedInt32Array &p_masks) { phoneme_masks_ = p_masks; emit_changed(); }
    [[nodiscard]] godot::PackedInt32Array get_phoneme_masks() const { return phoneme_masks_; }

    void set_semantic_tags(const godot::Ref<SemanticTagSetResource> &p_tags) { semantic_tags_ = p_tags; emit_changed(); }
    [[nodiscard]] godot::Ref<SemanticTagSetResource> get_semantic_tags() const { return semantic_tags_; }

    // --- TTS & Prosody Sampling ---
    void speak() const;
    void stop_speaking() const;

    /**
     * Evaluates `(Volume, Pitch)` at `p_normalized_time` (`[0.0 .. 1.0]`).
     * Uses `prosody_spline_` if bound, or falls back to `(volume_ / 100.0f, pitch_)`.
     */
    [[nodiscard]] godot::Vector2 sample_prosody_at(float p_normalized_time) const;
    [[nodiscard]] core::ProsodySamplePOD sample_prosody_pod_at(float p_normalized_time) const;

    // --- SSML Import / Export ---
    void parse_from_ssml(const godot::String &p_ssml);
    [[nodiscard]] godot::String export_to_ssml() const;

    // --- Snapshot & IdeamUndoRedo Commit API ---
    [[nodiscard]] godot::Dictionary get_snapshot_state() const;
    void apply_snapshot_state(const godot::Dictionary &p_state);
    void commit_snapshot_mutation(const godot::String &p_action_name, const godot::Dictionary &p_before_state);
    void commit_utterance_edit(
        const godot::String &p_new_text,
        float p_new_pitch,
        float p_new_rate,
        int p_new_volume,
        const godot::String &p_action_name = "Edit Utterance Resource"
    );
};

/**
 * SSMLParser (`RefCounted`)
 * C++26 replacement for `old_gdscript_plugins/ideam_dialogue/scripts/ssml_parser.gd`.
 */
class SSMLParser : public godot::RefCounted {
    GDCLASS(SSMLParser, godot::RefCounted)

protected:
    static void _bind_methods();

public:
    SSMLParser() = default;
    ~SSMLParser() override = default;

    [[nodiscard]] static godot::Ref<UtteranceResource> parse_ssml_to_utterance(const godot::String &p_ssml);
    [[nodiscard]] static godot::String export_utterance_to_ssml(const godot::Ref<UtteranceResource> &p_utterance);
    [[nodiscard]] static godot::PackedInt32Array extract_phoneme_masks_from_ssml(const godot::String &p_ssml);
};

class DiscourseResource : public godot::Resource {
    GDCLASS(DiscourseResource, godot::Resource)

private:
    godot::Ref<CommunicationMediumResource> medium_;
    godot::TypedArray<UtteranceResource> utterances_;
    godot::Array participants_; ///< Array of Dictionaries representing `DiscourseParticipantPOD`
    int current_turn_             = -1;
    int current_speaker_id_       = -1;
    int next_selected_speaker_id_ = -1;
    int active_tcu_index_         = 0;
    int last_applied_rule_        = static_cast<int>(core::DiscourseAllocationOutcome::OUTCOME_NONE);
    bool is_active_               = false;

    godot::Ref<SemanticTagSetResource> semantic_tags_;
    godot::Ref<IdeamUndoRedo> undo_redo_;

protected:
    static void _bind_methods();

public:
    DiscourseResource() = default;
    ~DiscourseResource() override = default;

    void set_medium(const godot::Ref<CommunicationMediumResource> &p_medium) { medium_ = p_medium; emit_changed(); }
    [[nodiscard]] godot::Ref<CommunicationMediumResource> get_medium() const { return medium_; }

    void set_utterances(const godot::TypedArray<UtteranceResource> &p_utterances) { utterances_ = p_utterances; emit_changed(); }
    [[nodiscard]] godot::TypedArray<UtteranceResource> get_utterances() const { return utterances_; }

    void set_participants(const godot::Array &p_participants) { participants_ = p_participants; emit_changed(); }
    [[nodiscard]] godot::Array get_participants() const { return participants_; }

    void set_current_turn(int p_turn) { current_turn_ = p_turn; emit_changed(); }
    [[nodiscard]] int get_current_turn() const noexcept { return current_turn_; }

    void set_current_speaker_id(int p_id) { current_speaker_id_ = p_id; emit_changed(); }
    [[nodiscard]] int get_current_speaker_id() const noexcept { return current_speaker_id_; }

    void set_next_selected_speaker_id(int p_id) { next_selected_speaker_id_ = p_id; emit_changed(); }
    [[nodiscard]] int get_next_selected_speaker_id() const noexcept { return next_selected_speaker_id_; }

    [[nodiscard]] int get_last_applied_rule() const noexcept { return last_applied_rule_; }
    [[nodiscard]] bool is_discourse_active() const noexcept { return is_active_; }

    void set_semantic_tags(const godot::Ref<SemanticTagSetResource> &p_tags) { semantic_tags_ = p_tags; emit_changed(); }
    [[nodiscard]] godot::Ref<SemanticTagSetResource> get_semantic_tags() const { return semantic_tags_; }

    // --- Conversation & Sacks-Schegloff-Jefferson Turn Allocation API ---
    void begin_discourse(int p_initial_speaker_id = -1);
    void conclude_discourse();

    void introduce_participant(
        int p_participant_id,
        int p_mind_agent_id = -1,
        float p_excitation_urgency = 0.0f,
        float p_turn_claim_priority = 0.0f,
        int p_addressed_target_id = -1,
        bool p_wants_floor = false
    );
    void dismiss_participant(int p_participant_id);

    /**
     * Evaluates the Sacks-Schegloff-Jefferson 3-rule state machine over `participants_`
     * and the current `UtteranceResource` prosody/resolution state. Returns the
     * `TurnAllocationRule` (`1 = Rule 1a`, `2 = Rule 1b`, `3 = Rule 1c`, `4 = Interruption`).
     */
    int evaluate_next_turn(float p_prosody_pitch_delta = -0.25f, float p_interruption_margin_db = 0.0f);

    // --- POD Conversion for DOD Staging ---
    [[nodiscard]] core::DiscourseTurnStatePOD to_turn_state_pod() const noexcept;
    void extract_participant_pods(std::vector<core::DiscourseParticipantPOD> &r_out) const;

    // --- Snapshot & IdeamUndoRedo Commit API ---
    [[nodiscard]] godot::Dictionary get_snapshot_state() const;
    void apply_snapshot_state(const godot::Dictionary &p_state);
    void commit_snapshot_mutation(const godot::String &p_action_name, const godot::Dictionary &p_before_state);
    void commit_discourse_turn(int p_next_speaker_id, const godot::String &p_action_name = "Advance Discourse Turn");
};

} // namespace ideam::godot_ext
