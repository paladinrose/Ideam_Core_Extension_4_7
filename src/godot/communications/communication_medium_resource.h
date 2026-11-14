#pragma once

/**
 * communication_medium_resource.h
 * `CommunicationMediumResource` (`src/godot/communications/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES
 * ============================================================================
 * Godot `Resource` representing the physical or logical transmission medium and
 * its psycholinguistic noise profile (succeeding `DiscourseMedium.cs` and
 * `AudioEnvironment.cs`). Supports `SemanticTagSetResource` tagging and
 * `IdeamUndoRedo` snapshot transactions.
 */

#include "../../core/communications/noisy_channel_transceiver.h"
#include "../linguistics/sememe_resource.h"
#include "../utilities/ideam_undo_redo.h"
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

namespace ideam::godot_ext {

class CommunicationMediumResource : public godot::Resource {
    GDCLASS(CommunicationMediumResource, godot::Resource)

private:
    int   medium_id_                      = 0;
    float ambient_noise_db_               = 50.0f;
    float attenuation_db_per_m_           = 0.6f;
    float occlusion_damping_db_           = 0.0f;
    float place_flip_sensitivity_         = 1.0f;
    float manner_flip_sensitivity_        = 0.55f;
    float voicing_flip_sensitivity_       = 0.20f;
    int   modality_                       = static_cast<int>(core::CommunicationModality::SPOKEN_ACOUSTIC);
    int   reverb_intelligibility_penalty_ = 0;

    godot::Ref<SemanticTagSetResource> semantic_tags_;
    godot::Ref<IdeamUndoRedo> undo_redo_;

protected:
    static void _bind_methods();

public:
    CommunicationMediumResource() = default;
    ~CommunicationMediumResource() override = default;

    void set_medium_id(int p_id) { medium_id_ = p_id; emit_changed(); }
    [[nodiscard]] int get_medium_id() const noexcept { return medium_id_; }

    void set_ambient_noise_db(float p_db) { ambient_noise_db_ = p_db; emit_changed(); }
    [[nodiscard]] float get_ambient_noise_db() const noexcept { return ambient_noise_db_; }

    void set_attenuation_db_per_m(float p_att) { attenuation_db_per_m_ = p_att; emit_changed(); }
    [[nodiscard]] float get_attenuation_db_per_m() const noexcept { return attenuation_db_per_m_; }

    void set_occlusion_damping_db(float p_occ) { occlusion_damping_db_ = p_occ; emit_changed(); }
    [[nodiscard]] float get_occlusion_damping_db() const noexcept { return occlusion_damping_db_; }

    void set_place_flip_sensitivity(float p_val) { place_flip_sensitivity_ = p_val; emit_changed(); }
    [[nodiscard]] float get_place_flip_sensitivity() const noexcept { return place_flip_sensitivity_; }

    void set_manner_flip_sensitivity(float p_val) { manner_flip_sensitivity_ = p_val; emit_changed(); }
    [[nodiscard]] float get_manner_flip_sensitivity() const noexcept { return manner_flip_sensitivity_; }

    void set_voicing_flip_sensitivity(float p_val) { voicing_flip_sensitivity_ = p_val; emit_changed(); }
    [[nodiscard]] float get_voicing_flip_sensitivity() const noexcept { return voicing_flip_sensitivity_; }

    void set_modality(int p_modality) { modality_ = p_modality; emit_changed(); }
    [[nodiscard]] int get_modality() const noexcept { return modality_; }

    void set_reverb_intelligibility_penalty(int p_penalty) {
        reverb_intelligibility_penalty_ = std::clamp(p_penalty, 0, 255);
        emit_changed();
    }
    [[nodiscard]] int get_reverb_intelligibility_penalty() const noexcept { return reverb_intelligibility_penalty_; }

    void set_semantic_tags(const godot::Ref<SemanticTagSetResource> &p_tags) { semantic_tags_ = p_tags; emit_changed(); }
    [[nodiscard]] godot::Ref<SemanticTagSetResource> get_semantic_tags() const { return semantic_tags_; }

    void set_undo_redo(const godot::Ref<IdeamUndoRedo> &p_ur) { undo_redo_ = p_ur; }
    [[nodiscard]] godot::Ref<IdeamUndoRedo> get_undo_redo() const { return undo_redo_; }

    // --- POD Conversion & SNR Preview Helpers ---
    [[nodiscard]] core::TransmissionMediumPOD to_pod() const noexcept;
    void from_pod(const core::TransmissionMediumPOD &p_pod);

    [[nodiscard]] float compute_effective_snr_db_preview(
        float p_distance_m,
        float p_projection_power_db = 65.0f,
        float p_elocution_skill = 1.0f,
        float p_listen_skill = 1.0f,
        float p_perception_bonus_db = 0.0f
    ) const noexcept;

    [[nodiscard]] int classify_noise_tier_preview(float p_effective_snr_db) const noexcept;

    // --- Snapshot & IdeamUndoRedo Commit API ---
    [[nodiscard]] godot::Dictionary get_snapshot_state() const;
    void apply_snapshot_state(const godot::Dictionary &p_state);
    void commit_snapshot_mutation(const godot::String &p_action_name, const godot::Dictionary &p_before_state);
    void commit_noise_profile(
        float p_ambient_db,
        float p_attenuation_per_m,
        float p_occlusion_db,
        const godot::String &p_action_name = "Edit Communication Medium Noise Profile"
    );
};

} // namespace ideam::godot_ext
