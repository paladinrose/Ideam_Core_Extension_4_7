#pragma once

#include "gameplay_resource.h"

#include <godot_cpp/classes/environment.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/classes/theme.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include "../../controls/theme_registry.h"

namespace ideam::godot_ext {

/**
 * DOD NOTE: Extracting accessibility enums at namespace scope with explicit int32_t
 * backing allows them to be stored directly in contiguous Structure of Arrays (SoA)
 * configuration columns or packed into bitfields without pulling in the full
 * GameplayAccessibilityOption class definition.
 */
enum ColorVisionMode : int32_t {
    CVD_NONE = 0,
    CVD_PROTANOPIA = 1,    // Red-blind Daltonization / filter
    CVD_DEUTERANOPIA = 2,  // Green-blind Daltonization / filter
    CVD_TRITANOPIA = 3,    // Blue-blind Daltonization / filter
    CVD_ACHROMATOPSIA = 4, // Monochromacy / high-contrast grayscale
    CVD_CUSTOM_LUT = 5     // Uses custom 1D/3D LUT Texture2D in Environment
};

enum HighContrastPreset : int32_t {
    CONTRAST_DEFAULT = 0,
    CONTRAST_HIGH_DARK = 1,
    CONTRAST_HIGH_LIGHT = 2,
    CONTRAST_YELLOW_ON_BLACK = 3,
    CONTRAST_CUSTOM = 4
};

enum AimAssistMode : int32_t {
    AIM_ASSIST_NONE = 0,
    AIM_ASSIST_FRICTION = 1,  // Slows camera/reticle sensitivity over valid targets
    AIM_ASSIST_MAGNETISM = 2, // Subtle projectile/reticle pull within angular radius
    AIM_ASSIST_SNAP_LOCK = 3  // Snaps aim onto nearest target on aim-input press
};

/**
 * DOD NOTE: GameplayAccessibilityOption encapsulates a comprehensive suite of visual,
 * UI/Theme, shader, motion/photosensitivity, audio/cymatic, and motor/cognitive
 * accessibility settings.
 *
 * Beyond direct Godot Server/Scene application (`apply_to_scene`), this class bridges
 * into the `GameplayStyle` cascade by overriding `collect_rules()`. When enabled,
 * motor and cognitive assists (such as slowing enemy `RedRobot` movement/charge speed
 * or boosting player survivability) automatically synthesize `!important` `GameplayRule`
 * declarations that outweigh standard `GameplayDifficulty` rules.
 */
class GameplayAccessibilityOption : public GameplayResource {
    GDCLASS(GameplayAccessibilityOption, GameplayResource)

protected:
    static void _bind_methods();

private:
    // --- 1. UI Theme & Typography ---
    godot::Ref<godot::Theme> ui_theme_override;
    godot::Ref<ThemeRegistry> theme_registry;
    int theme_registry_index = -1;
    HighContrastPreset high_contrast_preset = HighContrastPreset::CONTRAST_DEFAULT;
    float ui_scale = 1.0f;
    float font_scale = 1.0f;
    godot::Ref<godot::Font> dyslexia_friendly_font;
    float stylebox_border_width_scale = 1.0f;
    float ui_background_opacity = 1.0f;

    // --- 2. Vision & Shader Overrides ---
    ColorVisionMode color_vision_mode = ColorVisionMode::CVD_NONE;
    float color_vision_strength = 1.0f;
    godot::Ref<godot::Texture2D> color_correction_lut;
    godot::Ref<godot::ShaderMaterial> screen_post_process_material;

    bool high_contrast_entity_outlines = false;
    godot::Ref<godot::Material> entity_highlight_material;
    godot::Color enemy_highlight_color = godot::Color(1.0f, 0.2f, 0.2f, 1.0f);
    godot::Color interactable_highlight_color = godot::Color(0.2f, 0.9f, 1.0f, 1.0f);
    godot::Color hazard_highlight_color = godot::Color(1.0f, 0.85f, 0.1f, 1.0f);
    godot::TypedArray<godot::StringName> highlight_target_groups;

    float brightness_adjustment = 1.0f;
    float contrast_adjustment = 1.0f;
    float saturation_adjustment = 1.0f;
    bool disable_volumetric_fog_for_clarity = false;

    // --- 3. Motion & Photosensitivity ---
    bool reduce_flashing_lights = false;
    float flash_intensity_cap = 0.35f;
    float screen_shake_scale = 1.0f;
    float motion_blur_scale = 1.0f;
    bool reduce_camera_bob = false;
    float reticle_scale = 1.0f;
    float fov_override = 0.0f;

    // --- 4. Audio, Captions & Cymatics ---
    bool closed_captions_enabled = false;
    float caption_text_scale = 1.0f;
    float caption_background_opacity = 0.75f;
    bool caption_speaker_labels = true;
    bool mono_audio_downmix = false;
    bool visual_sound_indicators = false;
    float cymatic_visual_gain = 1.0f;

    // --- 5. Motor, Input & Cognitive Assists ---
    float game_speed_scale = 1.0f;
    float enemy_time_scale = 1.0f;
    float enemy_attack_charge_multiplier = 1.0f;
    float player_damage_taken_multiplier = 1.0f;
    float interaction_timeout_multiplier = 1.0f;

    AimAssistMode aim_assist_mode = AimAssistMode::AIM_ASSIST_NONE;
    float aim_assist_strength = 0.0f;
    float aim_magnetism_radius = 0.0f;

    bool hold_to_toggle_aim = false;
    bool hold_to_toggle_crouch = false;
    bool hold_to_toggle_sprint = false;
    bool auto_fire_on_hold = false;
    bool single_stick_camera_assist = false;

    godot::StringName enemy_group_tag = godot::StringName("RedRobot");
    godot::StringName enemy_weapon_group_tag = godot::StringName("HeavyCannon");
    godot::StringName player_group_tag = godot::StringName("Player");

    // Internal scene-traversal helpers
    godot::Ref<godot::Theme> _resolve_effective_theme() const;
    void _apply_entity_highlights_recursive(godot::Node *p_node) const;

public:
    GameplayAccessibilityOption();
    ~GameplayAccessibilityOption() override;

    // --- 1. UI Theme & Typography Setters / Getters ---
    void set_ui_theme_override(const godot::Ref<godot::Theme> &p_theme);
    godot::Ref<godot::Theme> get_ui_theme_override() const;

    void set_theme_registry(const godot::Ref<ThemeRegistry> &p_registry);
    godot::Ref<ThemeRegistry> get_theme_registry() const;

    void set_theme_registry_index(int p_index);
    int get_theme_registry_index() const;

    void set_high_contrast_preset(HighContrastPreset p_preset);
    HighContrastPreset get_high_contrast_preset() const;

    void set_ui_scale(float p_scale);
    float get_ui_scale() const;

    void set_font_scale(float p_scale);
    float get_font_scale() const;

    void set_dyslexia_friendly_font(const godot::Ref<godot::Font> &p_font);
    godot::Ref<godot::Font> get_dyslexia_friendly_font() const;

    void set_stylebox_border_width_scale(float p_scale);
    float get_stylebox_border_width_scale() const;

    void set_ui_background_opacity(float p_opacity);
    float get_ui_background_opacity() const;

    // --- 2. Vision & Shader Overrides Setters / Getters ---
    void set_color_vision_mode(ColorVisionMode p_mode);
    ColorVisionMode get_color_vision_mode() const;

    void set_color_vision_strength(float p_strength);
    float get_color_vision_strength() const;

    void set_color_correction_lut(const godot::Ref<godot::Texture2D> &p_lut);
    godot::Ref<godot::Texture2D> get_color_correction_lut() const;

    void set_screen_post_process_material(const godot::Ref<godot::ShaderMaterial> &p_material);
    godot::Ref<godot::ShaderMaterial> get_screen_post_process_material() const;

    void set_high_contrast_entity_outlines(bool p_enabled);
    bool get_high_contrast_entity_outlines() const;

    void set_entity_highlight_material(const godot::Ref<godot::Material> &p_material);
    godot::Ref<godot::Material> get_entity_highlight_material() const;

    void set_enemy_highlight_color(const godot::Color &p_color);
    godot::Color get_enemy_highlight_color() const;

    void set_interactable_highlight_color(const godot::Color &p_color);
    godot::Color get_interactable_highlight_color() const;

    void set_hazard_highlight_color(const godot::Color &p_color);
    godot::Color get_hazard_highlight_color() const;

    void set_highlight_target_groups(const godot::TypedArray<godot::StringName> &p_groups);
    godot::TypedArray<godot::StringName> get_highlight_target_groups() const;

    void set_brightness_adjustment(float p_brightness);
    float get_brightness_adjustment() const;

    void set_contrast_adjustment(float p_contrast);
    float get_contrast_adjustment() const;

    void set_saturation_adjustment(float p_saturation);
    float get_saturation_adjustment() const;

    void set_disable_volumetric_fog_for_clarity(bool p_disable);
    bool get_disable_volumetric_fog_for_clarity() const;

    // --- 3. Motion & Photosensitivity Setters / Getters ---
    void set_reduce_flashing_lights(bool p_reduce);
    bool get_reduce_flashing_lights() const;

    void set_flash_intensity_cap(float p_cap);
    float get_flash_intensity_cap() const;

    void set_screen_shake_scale(float p_scale);
    float get_screen_shake_scale() const;

    void set_motion_blur_scale(float p_scale);
    float get_motion_blur_scale() const;

    void set_reduce_camera_bob(bool p_reduce);
    bool get_reduce_camera_bob() const;

    void set_reticle_scale(float p_scale);
    float get_reticle_scale() const;

    void set_fov_override(float p_fov);
    float get_fov_override() const;

    // --- 4. Audio, Captions & Cymatics Setters / Getters ---
    void set_closed_captions_enabled(bool p_enabled);
    bool get_closed_captions_enabled() const;

    void set_caption_text_scale(float p_scale);
    float get_caption_text_scale() const;

    void set_caption_background_opacity(float p_opacity);
    float get_caption_background_opacity() const;

    void set_caption_speaker_labels(bool p_enabled);
    bool get_caption_speaker_labels() const;

    void set_mono_audio_downmix(bool p_mono);
    bool get_mono_audio_downmix() const;

    void set_visual_sound_indicators(bool p_enabled);
    bool get_visual_sound_indicators() const;

    void set_cymatic_visual_gain(float p_gain);
    float get_cymatic_visual_gain() const;

    // --- 5. Motor, Input & Cognitive Assists Setters / Getters ---
    void set_game_speed_scale(float p_scale);
    float get_game_speed_scale() const;

    void set_enemy_time_scale(float p_scale);
    float get_enemy_time_scale() const;

    void set_enemy_attack_charge_multiplier(float p_mult);
    float get_enemy_attack_charge_multiplier() const;

    void set_player_damage_taken_multiplier(float p_mult);
    float get_player_damage_taken_multiplier() const;

    void set_interaction_timeout_multiplier(float p_mult);
    float get_interaction_timeout_multiplier() const;

    void set_aim_assist_mode(AimAssistMode p_mode);
    AimAssistMode get_aim_assist_mode() const;

    void set_aim_assist_strength(float p_strength);
    float get_aim_assist_strength() const;

    void set_aim_magnetism_radius(float p_radius);
    float get_aim_magnetism_radius() const;

    void set_hold_to_toggle_aim(bool p_toggle);
    bool get_hold_to_toggle_aim() const;

    void set_hold_to_toggle_crouch(bool p_toggle);
    bool get_hold_to_toggle_crouch() const;

    void set_hold_to_toggle_sprint(bool p_toggle);
    bool get_hold_to_toggle_sprint() const;

    void set_auto_fire_on_hold(bool p_auto_fire);
    bool get_auto_fire_on_hold() const;

    void set_single_stick_camera_assist(bool p_assist);
    bool get_single_stick_camera_assist() const;

    void set_enemy_group_tag(const godot::StringName &p_tag);
    godot::StringName get_enemy_group_tag() const;

    void set_enemy_weapon_group_tag(const godot::StringName &p_tag);
    godot::StringName get_enemy_weapon_group_tag() const;

    void set_player_group_tag(const godot::StringName &p_tag);
    godot::StringName get_player_group_tag() const;

    // --- Application, Rule Synthesis & Persistence ---
    void apply_accessibility_option(const godot::Ref<GameplayAccessibilityOption> &p_other);

    /**
     * Applies UI scaling, Theme/Font/StyleBox overrides, Environment color/fog/flash
     * adjustments, Post-Process shader uniforms, and 3D entity highlight material
     * overlays directly to the active Godot Window, Environment, and Scene root.
     */
    void apply_to_scene(godot::Window *p_window, godot::Environment *p_environment, godot::Node *p_scene_root);

    /**
     * Synthesizes `!important` GameplayRule objects for active motor/cognitive assists
     * (enemy time_scale, cannon charge time, player health/shield durability, and
     * interaction timeouts) and combines them with `associated_rules`.
     */
    virtual godot::TypedArray<GameplayRule> collect_rules() const override;

    virtual void apply_resource(const godot::Ref<GameplayResource> &p_other) override;
    virtual godot::Dictionary save_resource() const override;
    virtual void load_resource(const godot::Dictionary &p_data) override;
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::ColorVisionMode);
VARIANT_ENUM_CAST(ideam::godot_ext::HighContrastPreset);
VARIANT_ENUM_CAST(ideam::godot_ext::AimAssistMode);