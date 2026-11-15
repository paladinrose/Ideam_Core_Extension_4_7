#pragma once

/**
 * character_morph_slider_panel.h
 * Tier 2 Specific UI (`CharacterMorphSliderPanel`) for `IdeamCharacters`
 * (`src/godot/characters/ui/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES (SECTION 3.2 TIER 2 — `CharacterMorphSliderPanel`)
 * ============================================================================
 * Mirrors a `CharacterBlueprintResource` in real time across:
 *   1. Body Archetype Selector (`Biped`, `Quadruped`, `Avian`, `FlyingEye`, `Serpentine`, `Custom`).
 *   2. Subdivision LOD Selector (`0..3`).
 *   3. 12 Anatomical Sparse Morph Sliders (`Stature Height`, `Torso Bulk`, `Head Scale`,
 *      `Upper Limb Reach`, `Lower Limb Stride`, `Digit 0..4 Curl`, `Tail Length`, `Wing Span`),
 *      each wired through `CharacterBlueprintResource::commit_set_morph_channel_weight()`
 *      for reversible `IdeamUndoRedo` support.
 */

#include "../character_blueprint_resource.h"
#include "../../utilities/ideam_undo_redo.h"

#include <godot_cpp/classes/h_slider.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/spin_box.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <vector>

namespace ideam::godot_ext {

class CharacterMorphSliderPanel : public godot::VBoxContainer {
    GDCLASS(CharacterMorphSliderPanel, godot::VBoxContainer)

private:
    godot::Ref<CharacterBlueprintResource> blueprint_;
    godot::Ref<IdeamUndoRedo> undo_redo_;

    godot::OptionButton *archetype_selector_ = nullptr;
    godot::SpinBox *lod_spin_box_            = nullptr;
    godot::Label *telemetry_label_           = nullptr;
    std::vector<godot::HSlider *> channel_sliders_;
    std::vector<godot::Label *> channel_value_labels_;
    bool updating_ui_                        = false;

    void _rebuild_ui();
    void _sync_from_blueprint();
    void _on_archetype_selected(int p_index);
    void _on_lod_value_changed(double p_value);
    void _on_morph_slider_changed(double p_value, int p_channel);
    void _on_reset_morphs_pressed();

protected:
    static void _bind_methods();
    void _notification(int p_what);

public:
    CharacterMorphSliderPanel() = default;
    ~CharacterMorphSliderPanel() override = default;

    void set_blueprint(const godot::Ref<CharacterBlueprintResource> &p_blueprint);
    [[nodiscard]] godot::Ref<CharacterBlueprintResource> get_blueprint() const { return blueprint_; }

    void set_undo_redo(const godot::Ref<IdeamUndoRedo> &p_undo_redo) { undo_redo_ = p_undo_redo; }
    [[nodiscard]] godot::Ref<IdeamUndoRedo> get_undo_redo() const { return undo_redo_; }

    void refresh_from_blueprint();
};

} // namespace ideam::godot_ext
