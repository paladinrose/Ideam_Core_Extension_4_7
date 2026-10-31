#pragma once

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include "../gameplay/gameplay_accessibility_option.h"

namespace ideam::godot_ext {

/**
 * DOD NOTE: AccessibilityLivePreviewCard renders an immediate-mode split preview card
 * for `GameplayAccessibilityOption` so players can verify visual, color-vision,
 * entity-outline, high-contrast UI, and closed-caption settings before applying them.
 *
 * It simulates on the CPU:
 * 1. Daltonization / Color Vision Deficiency transforms (`CVD_PROTANOPIA`,
 *    `CVD_DEUTERANOPIA`, `CVD_TRITANOPIA`, `CVD_ACHROMATOPSIA`) blended by
 *    `color_vision_strength`, plus `brightness_adjustment`, `contrast_adjustment`,
 *    and `saturation_adjustment`.
 * 2. 3D Entity Silhouette Badges (Enemy `RedRobot`, Interactable, and Hazard) with
 *    `high_contrast_entity_outlines` and `enemy_highlight_color` /
 *    `interactable_highlight_color` / `hazard_highlight_color`.
 * 3. Live Closed-Caption Overlay Box previewing `closed_captions_enabled`,
 *    `caption_text_scale`, `caption_background_opacity`, and `caption_speaker_labels`.
 */
class AccessibilityLivePreviewCard : public godot::Control {
    GDCLASS(AccessibilityLivePreviewCard, godot::Control)

protected:
    static void _bind_methods();
    void _notification(int p_what);

private:
    godot::Ref<GameplayAccessibilityOption> accessibility_option;

    godot::String sample_speaker_name = "Commander Vance";
    godot::String sample_caption_text = "Watch the Red Robot charging its heavy cannon on the left flank!";
    bool show_cymatic_sound_ring = true;

    void _on_accessibility_changed();

public:
    AccessibilityLivePreviewCard();
    ~AccessibilityLivePreviewCard() override;

    virtual godot::Vector2 _get_minimum_size() const override;

    /**
     * Pure static color transform helper applying Brightness/Contrast/Saturation and
     * Daltonization CVD simulation (`ColorVisionMode`) to any input `godot::Color`.
     */
    static godot::Color simulate_accessible_color(
        const godot::Color &p_input,
        ColorVisionMode p_mode,
        float p_cvd_strength,
        float p_brightness = 1.0f,
        float p_contrast = 1.0f,
        float p_saturation = 1.0f
    );

    // --- Setters / Getters ---
    void set_accessibility_option(const godot::Ref<GameplayAccessibilityOption> &p_option);
    godot::Ref<GameplayAccessibilityOption> get_accessibility_option() const;

    void set_sample_speaker_name(const godot::String &p_speaker);
    godot::String get_sample_speaker_name() const;

    void set_sample_caption_text(const godot::String &p_text);
    godot::String get_sample_caption_text() const;

    void set_show_cymatic_sound_ring(bool p_show);
    bool get_show_cymatic_sound_ring() const;
};

} // namespace ideam::godot_ext