#include "gameplay_accessibility_option.h"

#include <cmath>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/geometry_instance3d.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/classes/theme_db.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/array.hpp>

#include "gameplay_rule.h"

namespace ideam::godot_ext {

void GameplayAccessibilityOption::_bind_methods() {
    // --- Enums ---
    BIND_ENUM_CONSTANT(ColorVisionMode::CVD_NONE);
    BIND_ENUM_CONSTANT(ColorVisionMode::CVD_PROTANOPIA);
    BIND_ENUM_CONSTANT(ColorVisionMode::CVD_DEUTERANOPIA);
    BIND_ENUM_CONSTANT(ColorVisionMode::CVD_TRITANOPIA);
    BIND_ENUM_CONSTANT(ColorVisionMode::CVD_ACHROMATOPSIA);
    BIND_ENUM_CONSTANT(ColorVisionMode::CVD_CUSTOM_LUT);

    BIND_ENUM_CONSTANT(HighContrastPreset::CONTRAST_DEFAULT);
    BIND_ENUM_CONSTANT(HighContrastPreset::CONTRAST_HIGH_DARK);
    BIND_ENUM_CONSTANT(HighContrastPreset::CONTRAST_HIGH_LIGHT);
    BIND_ENUM_CONSTANT(HighContrastPreset::CONTRAST_YELLOW_ON_BLACK);
    BIND_ENUM_CONSTANT(HighContrastPreset::CONTRAST_CUSTOM);

    BIND_ENUM_CONSTANT(AimAssistMode::AIM_ASSIST_NONE);
    BIND_ENUM_CONSTANT(AimAssistMode::AIM_ASSIST_FRICTION);
    BIND_ENUM_CONSTANT(AimAssistMode::AIM_ASSIST_MAGNETISM);
    BIND_ENUM_CONSTANT(AimAssistMode::AIM_ASSIST_SNAP_LOCK);

    // --- 1. UI Theme & Typography ---
    ADD_GROUP("UI Theme & Typography", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_ui_theme_override", "theme"), &GameplayAccessibilityOption::set_ui_theme_override);
    godot::ClassDB::bind_method(godot::D_METHOD("get_ui_theme_override"), &GameplayAccessibilityOption::get_ui_theme_override);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::OBJECT, "ui_theme_override", godot::PROPERTY_HINT_RESOURCE_TYPE, "Theme"),
        "set_ui_theme_override",
        "get_ui_theme_override"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_theme_registry", "registry"), &GameplayAccessibilityOption::set_theme_registry);
    godot::ClassDB::bind_method(godot::D_METHOD("get_theme_registry"), &GameplayAccessibilityOption::get_theme_registry);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::OBJECT, "theme_registry", godot::PROPERTY_HINT_RESOURCE_TYPE, "ThemeRegistry"),
        "set_theme_registry",
        "get_theme_registry"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_theme_registry_index", "index"), &GameplayAccessibilityOption::set_theme_registry_index);
    godot::ClassDB::bind_method(godot::D_METHOD("get_theme_registry_index"), &GameplayAccessibilityOption::get_theme_registry_index);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "theme_registry_index"), "set_theme_registry_index", "get_theme_registry_index");

    godot::ClassDB::bind_method(godot::D_METHOD("set_high_contrast_preset", "preset"), &GameplayAccessibilityOption::set_high_contrast_preset);
    godot::ClassDB::bind_method(godot::D_METHOD("get_high_contrast_preset"), &GameplayAccessibilityOption::get_high_contrast_preset);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::INT, "high_contrast_preset", godot::PROPERTY_HINT_ENUM, "Default,High Contrast Dark,High Contrast Light,Yellow on Black,Custom"),
        "set_high_contrast_preset",
        "get_high_contrast_preset"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_ui_scale", "scale"), &GameplayAccessibilityOption::set_ui_scale);
    godot::ClassDB::bind_method(godot::D_METHOD("get_ui_scale"), &GameplayAccessibilityOption::get_ui_scale);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "ui_scale", godot::PROPERTY_HINT_RANGE, "0.5,2.5,0.05"),
        "set_ui_scale",
        "get_ui_scale"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_font_scale", "scale"), &GameplayAccessibilityOption::set_font_scale);
    godot::ClassDB::bind_method(godot::D_METHOD("get_font_scale"), &GameplayAccessibilityOption::get_font_scale);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "font_scale", godot::PROPERTY_HINT_RANGE, "0.5,2.5,0.05"),
        "set_font_scale",
        "get_font_scale"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_dyslexia_friendly_font", "font"), &GameplayAccessibilityOption::set_dyslexia_friendly_font);
    godot::ClassDB::bind_method(godot::D_METHOD("get_dyslexia_friendly_font"), &GameplayAccessibilityOption::get_dyslexia_friendly_font);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::OBJECT, "dyslexia_friendly_font", godot::PROPERTY_HINT_RESOURCE_TYPE, "Font"),
        "set_dyslexia_friendly_font",
        "get_dyslexia_friendly_font"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_stylebox_border_width_scale", "scale"), &GameplayAccessibilityOption::set_stylebox_border_width_scale);
    godot::ClassDB::bind_method(godot::D_METHOD("get_stylebox_border_width_scale"), &GameplayAccessibilityOption::get_stylebox_border_width_scale);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "stylebox_border_width_scale", godot::PROPERTY_HINT_RANGE, "0.0,4.0,0.25"),
        "set_stylebox_border_width_scale",
        "get_stylebox_border_width_scale"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_ui_background_opacity", "opacity"), &GameplayAccessibilityOption::set_ui_background_opacity);
    godot::ClassDB::bind_method(godot::D_METHOD("get_ui_background_opacity"), &GameplayAccessibilityOption::get_ui_background_opacity);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "ui_background_opacity", godot::PROPERTY_HINT_RANGE, "0.0,1.0,0.05"),
        "set_ui_background_opacity",
        "get_ui_background_opacity"
    );

    // --- 2. Vision & Shader Overrides ---
    ADD_GROUP("Vision & Shader Overrides", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_color_vision_mode", "mode"), &GameplayAccessibilityOption::set_color_vision_mode);
    godot::ClassDB::bind_method(godot::D_METHOD("get_color_vision_mode"), &GameplayAccessibilityOption::get_color_vision_mode);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::INT, "color_vision_mode", godot::PROPERTY_HINT_ENUM, "None,Protanopia,Deuteranopia,Tritanopia,Achromatopsia,Custom LUT"),
        "set_color_vision_mode",
        "get_color_vision_mode"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_color_vision_strength", "strength"), &GameplayAccessibilityOption::set_color_vision_strength);
    godot::ClassDB::bind_method(godot::D_METHOD("get_color_vision_strength"), &GameplayAccessibilityOption::get_color_vision_strength);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "color_vision_strength", godot::PROPERTY_HINT_RANGE, "0.0,1.0,0.05"),
        "set_color_vision_strength",
        "get_color_vision_strength"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_color_correction_lut", "lut"), &GameplayAccessibilityOption::set_color_correction_lut);
    godot::ClassDB::bind_method(godot::D_METHOD("get_color_correction_lut"), &GameplayAccessibilityOption::get_color_correction_lut);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::OBJECT, "color_correction_lut", godot::PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"),
        "set_color_correction_lut",
        "get_color_correction_lut"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_screen_post_process_material", "material"), &GameplayAccessibilityOption::set_screen_post_process_material);
    godot::ClassDB::bind_method(godot::D_METHOD("get_screen_post_process_material"), &GameplayAccessibilityOption::get_screen_post_process_material);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::OBJECT, "screen_post_process_material", godot::PROPERTY_HINT_RESOURCE_TYPE, "ShaderMaterial"),
        "set_screen_post_process_material",
        "get_screen_post_process_material"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_high_contrast_entity_outlines", "enabled"), &GameplayAccessibilityOption::set_high_contrast_entity_outlines);
    godot::ClassDB::bind_method(godot::D_METHOD("get_high_contrast_entity_outlines"), &GameplayAccessibilityOption::get_high_contrast_entity_outlines);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "high_contrast_entity_outlines"), "set_high_contrast_entity_outlines", "get_high_contrast_entity_outlines");

    godot::ClassDB::bind_method(godot::D_METHOD("set_entity_highlight_material", "material"), &GameplayAccessibilityOption::set_entity_highlight_material);
    godot::ClassDB::bind_method(godot::D_METHOD("get_entity_highlight_material"), &GameplayAccessibilityOption::get_entity_highlight_material);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::OBJECT, "entity_highlight_material", godot::PROPERTY_HINT_RESOURCE_TYPE, "Material"),
        "set_entity_highlight_material",
        "get_entity_highlight_material"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_enemy_highlight_color", "color"), &GameplayAccessibilityOption::set_enemy_highlight_color);
    godot::ClassDB::bind_method(godot::D_METHOD("get_enemy_highlight_color"), &GameplayAccessibilityOption::get_enemy_highlight_color);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::COLOR, "enemy_highlight_color"), "set_enemy_highlight_color", "get_enemy_highlight_color");

    godot::ClassDB::bind_method(godot::D_METHOD("set_interactable_highlight_color", "color"), &GameplayAccessibilityOption::set_interactable_highlight_color);
    godot::ClassDB::bind_method(godot::D_METHOD("get_interactable_highlight_color"), &GameplayAccessibilityOption::get_interactable_highlight_color);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::COLOR, "interactable_highlight_color"), "set_interactable_highlight_color", "get_interactable_highlight_color");

    godot::ClassDB::bind_method(godot::D_METHOD("set_hazard_highlight_color", "color"), &GameplayAccessibilityOption::set_hazard_highlight_color);
    godot::ClassDB::bind_method(godot::D_METHOD("get_hazard_highlight_color"), &GameplayAccessibilityOption::get_hazard_highlight_color);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::COLOR, "hazard_highlight_color"), "set_hazard_highlight_color", "get_hazard_highlight_color");

    godot::ClassDB::bind_method(godot::D_METHOD("set_highlight_target_groups", "groups"), &GameplayAccessibilityOption::set_highlight_target_groups);
    godot::ClassDB::bind_method(godot::D_METHOD("get_highlight_target_groups"), &GameplayAccessibilityOption::get_highlight_target_groups);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::ARRAY, "highlight_target_groups", godot::PROPERTY_HINT_TYPE_STRING, "StringName"),
        "set_highlight_target_groups",
        "get_highlight_target_groups"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_brightness_adjustment", "brightness"), &GameplayAccessibilityOption::set_brightness_adjustment);
    godot::ClassDB::bind_method(godot::D_METHOD("get_brightness_adjustment"), &GameplayAccessibilityOption::get_brightness_adjustment);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "brightness_adjustment", godot::PROPERTY_HINT_RANGE, "0.2,3.0,0.05"),
        "set_brightness_adjustment",
        "get_brightness_adjustment"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_contrast_adjustment", "contrast"), &GameplayAccessibilityOption::set_contrast_adjustment);
    godot::ClassDB::bind_method(godot::D_METHOD("get_contrast_adjustment"), &GameplayAccessibilityOption::get_contrast_adjustment);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "contrast_adjustment", godot::PROPERTY_HINT_RANGE, "0.2,3.0,0.05"),
        "set_contrast_adjustment",
        "get_contrast_adjustment"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_saturation_adjustment", "saturation"), &GameplayAccessibilityOption::set_saturation_adjustment);
    godot::ClassDB::bind_method(godot::D_METHOD("get_saturation_adjustment"), &GameplayAccessibilityOption::get_saturation_adjustment);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "saturation_adjustment", godot::PROPERTY_HINT_RANGE, "0.0,3.0,0.05"),
        "set_saturation_adjustment",
        "get_saturation_adjustment"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_disable_volumetric_fog_for_clarity", "disable"), &GameplayAccessibilityOption::set_disable_volumetric_fog_for_clarity);
    godot::ClassDB::bind_method(godot::D_METHOD("get_disable_volumetric_fog_for_clarity"), &GameplayAccessibilityOption::get_disable_volumetric_fog_for_clarity);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "disable_volumetric_fog_for_clarity"), "set_disable_volumetric_fog_for_clarity", "get_disable_volumetric_fog_for_clarity");

    // --- 3. Motion & Photosensitivity ---
    ADD_GROUP("Motion & Photosensitivity", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_reduce_flashing_lights", "reduce"), &GameplayAccessibilityOption::set_reduce_flashing_lights);
    godot::ClassDB::bind_method(godot::D_METHOD("get_reduce_flashing_lights"), &GameplayAccessibilityOption::get_reduce_flashing_lights);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "reduce_flashing_lights"), "set_reduce_flashing_lights", "get_reduce_flashing_lights");

    godot::ClassDB::bind_method(godot::D_METHOD("set_flash_intensity_cap", "cap"), &GameplayAccessibilityOption::set_flash_intensity_cap);
    godot::ClassDB::bind_method(godot::D_METHOD("get_flash_intensity_cap"), &GameplayAccessibilityOption::get_flash_intensity_cap);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "flash_intensity_cap", godot::PROPERTY_HINT_RANGE, "0.0,2.0,0.05"),
        "set_flash_intensity_cap",
        "get_flash_intensity_cap"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_screen_shake_scale", "scale"), &GameplayAccessibilityOption::set_screen_shake_scale);
    godot::ClassDB::bind_method(godot::D_METHOD("get_screen_shake_scale"), &GameplayAccessibilityOption::get_screen_shake_scale);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "screen_shake_scale", godot::PROPERTY_HINT_RANGE, "0.0,1.0,0.05"),
        "set_screen_shake_scale",
        "get_screen_shake_scale"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_motion_blur_scale", "scale"), &GameplayAccessibilityOption::set_motion_blur_scale);
    godot::ClassDB::bind_method(godot::D_METHOD("get_motion_blur_scale"), &GameplayAccessibilityOption::get_motion_blur_scale);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "motion_blur_scale", godot::PROPERTY_HINT_RANGE, "0.0,1.0,0.05"),
        "set_motion_blur_scale",
        "get_motion_blur_scale"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_reduce_camera_bob", "reduce"), &GameplayAccessibilityOption::set_reduce_camera_bob);
    godot::ClassDB::bind_method(godot::D_METHOD("get_reduce_camera_bob"), &GameplayAccessibilityOption::get_reduce_camera_bob);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "reduce_camera_bob"), "set_reduce_camera_bob", "get_reduce_camera_bob");

    godot::ClassDB::bind_method(godot::D_METHOD("set_reticle_scale", "scale"), &GameplayAccessibilityOption::set_reticle_scale);
    godot::ClassDB::bind_method(godot::D_METHOD("get_reticle_scale"), &GameplayAccessibilityOption::get_reticle_scale);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "reticle_scale", godot::PROPERTY_HINT_RANGE, "0.5,3.0,0.1"),
        "set_reticle_scale",
        "get_reticle_scale"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_fov_override", "fov"), &GameplayAccessibilityOption::set_fov_override);
    godot::ClassDB::bind_method(godot::D_METHOD("get_fov_override"), &GameplayAccessibilityOption::get_fov_override);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "fov_override", godot::PROPERTY_HINT_RANGE, "0.0,130.0,1.0"),
        "set_fov_override",
        "get_fov_override"
    );

    // --- 4. Audio, Captions & Cymatics ---
    ADD_GROUP("Audio, Captions & Cymatics", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_closed_captions_enabled", "enabled"), &GameplayAccessibilityOption::set_closed_captions_enabled);
    godot::ClassDB::bind_method(godot::D_METHOD("get_closed_captions_enabled"), &GameplayAccessibilityOption::get_closed_captions_enabled);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "closed_captions_enabled"), "set_closed_captions_enabled", "get_closed_captions_enabled");

    godot::ClassDB::bind_method(godot::D_METHOD("set_caption_text_scale", "scale"), &GameplayAccessibilityOption::set_caption_text_scale);
    godot::ClassDB::bind_method(godot::D_METHOD("get_caption_text_scale"), &GameplayAccessibilityOption::get_caption_text_scale);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "caption_text_scale", godot::PROPERTY_HINT_RANGE, "0.5,2.5,0.05"),
        "set_caption_text_scale",
        "get_caption_text_scale"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_caption_background_opacity", "opacity"), &GameplayAccessibilityOption::set_caption_background_opacity);
    godot::ClassDB::bind_method(godot::D_METHOD("get_caption_background_opacity"), &GameplayAccessibilityOption::get_caption_background_opacity);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "caption_background_opacity", godot::PROPERTY_HINT_RANGE, "0.0,1.0,0.05"),
        "set_caption_background_opacity",
        "get_caption_background_opacity"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_caption_speaker_labels", "enabled"), &GameplayAccessibilityOption::set_caption_speaker_labels);
    godot::ClassDB::bind_method(godot::D_METHOD("get_caption_speaker_labels"), &GameplayAccessibilityOption::get_caption_speaker_labels);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "caption_speaker_labels"), "set_caption_speaker_labels", "get_caption_speaker_labels");

    godot::ClassDB::bind_method(godot::D_METHOD("set_mono_audio_downmix", "mono"), &GameplayAccessibilityOption::set_mono_audio_downmix);
    godot::ClassDB::bind_method(godot::D_METHOD("get_mono_audio_downmix"), &GameplayAccessibilityOption::get_mono_audio_downmix);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "mono_audio_downmix"), "set_mono_audio_downmix", "get_mono_audio_downmix");

    godot::ClassDB::bind_method(godot::D_METHOD("set_visual_sound_indicators", "enabled"), &GameplayAccessibilityOption::set_visual_sound_indicators);
    godot::ClassDB::bind_method(godot::D_METHOD("get_visual_sound_indicators"), &GameplayAccessibilityOption::get_visual_sound_indicators);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "visual_sound_indicators"), "set_visual_sound_indicators", "get_visual_sound_indicators");

    godot::ClassDB::bind_method(godot::D_METHOD("set_cymatic_visual_gain", "gain"), &GameplayAccessibilityOption::set_cymatic_visual_gain);
    godot::ClassDB::bind_method(godot::D_METHOD("get_cymatic_visual_gain"), &GameplayAccessibilityOption::get_cymatic_visual_gain);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "cymatic_visual_gain", godot::PROPERTY_HINT_RANGE, "0.0,4.0,0.1"),
        "set_cymatic_visual_gain",
        "get_cymatic_visual_gain"
    );

    // --- 5. Motor, Input & Cognitive Assists ---
    ADD_GROUP("Motor, Input & Cognitive Assists", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_game_speed_scale", "scale"), &GameplayAccessibilityOption::set_game_speed_scale);
    godot::ClassDB::bind_method(godot::D_METHOD("get_game_speed_scale"), &GameplayAccessibilityOption::get_game_speed_scale);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "game_speed_scale", godot::PROPERTY_HINT_RANGE, "0.1,2.0,0.05"),
        "set_game_speed_scale",
        "get_game_speed_scale"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_enemy_time_scale", "scale"), &GameplayAccessibilityOption::set_enemy_time_scale);
    godot::ClassDB::bind_method(godot::D_METHOD("get_enemy_time_scale"), &GameplayAccessibilityOption::get_enemy_time_scale);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "enemy_time_scale", godot::PROPERTY_HINT_RANGE, "0.1,2.0,0.05"),
        "set_enemy_time_scale",
        "get_enemy_time_scale"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_enemy_attack_charge_multiplier", "multiplier"), &GameplayAccessibilityOption::set_enemy_attack_charge_multiplier);
    godot::ClassDB::bind_method(godot::D_METHOD("get_enemy_attack_charge_multiplier"), &GameplayAccessibilityOption::get_enemy_attack_charge_multiplier);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "enemy_attack_charge_multiplier", godot::PROPERTY_HINT_RANGE, "0.25,3.0,0.05"),
        "set_enemy_attack_charge_multiplier",
        "get_enemy_attack_charge_multiplier"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_player_damage_taken_multiplier", "multiplier"), &GameplayAccessibilityOption::set_player_damage_taken_multiplier);
    godot::ClassDB::bind_method(godot::D_METHOD("get_player_damage_taken_multiplier"), &GameplayAccessibilityOption::get_player_damage_taken_multiplier);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "player_damage_taken_multiplier", godot::PROPERTY_HINT_RANGE, "0.0,2.0,0.05"),
        "set_player_damage_taken_multiplier",
        "get_player_damage_taken_multiplier"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_interaction_timeout_multiplier", "multiplier"), &GameplayAccessibilityOption::set_interaction_timeout_multiplier);
    godot::ClassDB::bind_method(godot::D_METHOD("get_interaction_timeout_multiplier"), &GameplayAccessibilityOption::get_interaction_timeout_multiplier);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "interaction_timeout_multiplier", godot::PROPERTY_HINT_RANGE, "0.5,5.0,0.1"),
        "set_interaction_timeout_multiplier",
        "get_interaction_timeout_multiplier"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_aim_assist_mode", "mode"), &GameplayAccessibilityOption::set_aim_assist_mode);
    godot::ClassDB::bind_method(godot::D_METHOD("get_aim_assist_mode"), &GameplayAccessibilityOption::get_aim_assist_mode);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::INT, "aim_assist_mode", godot::PROPERTY_HINT_ENUM, "None,Friction,Magnetism,Snap Lock"),
        "set_aim_assist_mode",
        "get_aim_assist_mode"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_aim_assist_strength", "strength"), &GameplayAccessibilityOption::set_aim_assist_strength);
    godot::ClassDB::bind_method(godot::D_METHOD("get_aim_assist_strength"), &GameplayAccessibilityOption::get_aim_assist_strength);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "aim_assist_strength", godot::PROPERTY_HINT_RANGE, "0.0,1.0,0.05"),
        "set_aim_assist_strength",
        "get_aim_assist_strength"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_aim_magnetism_radius", "radius"), &GameplayAccessibilityOption::set_aim_magnetism_radius);
    godot::ClassDB::bind_method(godot::D_METHOD("get_aim_magnetism_radius"), &GameplayAccessibilityOption::get_aim_magnetism_radius);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "aim_magnetism_radius", godot::PROPERTY_HINT_RANGE, "0.0,15.0,0.5"),
        "set_aim_magnetism_radius",
        "get_aim_magnetism_radius"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_hold_to_toggle_aim", "toggle"), &GameplayAccessibilityOption::set_hold_to_toggle_aim);
    godot::ClassDB::bind_method(godot::D_METHOD("get_hold_to_toggle_aim"), &GameplayAccessibilityOption::get_hold_to_toggle_aim);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "hold_to_toggle_aim"), "set_hold_to_toggle_aim", "get_hold_to_toggle_aim");

    godot::ClassDB::bind_method(godot::D_METHOD("set_hold_to_toggle_crouch", "toggle"), &GameplayAccessibilityOption::set_hold_to_toggle_crouch);
    godot::ClassDB::bind_method(godot::D_METHOD("get_hold_to_toggle_crouch"), &GameplayAccessibilityOption::get_hold_to_toggle_crouch);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "hold_to_toggle_crouch"), "set_hold_to_toggle_crouch", "get_hold_to_toggle_crouch");

    godot::ClassDB::bind_method(godot::D_METHOD("set_hold_to_toggle_sprint", "toggle"), &GameplayAccessibilityOption::set_hold_to_toggle_sprint);
    godot::ClassDB::bind_method(godot::D_METHOD("get_hold_to_toggle_sprint"), &GameplayAccessibilityOption::get_hold_to_toggle_sprint);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "hold_to_toggle_sprint"), "set_hold_to_toggle_sprint", "get_hold_to_toggle_sprint");

    godot::ClassDB::bind_method(godot::D_METHOD("set_auto_fire_on_hold", "auto_fire"), &GameplayAccessibilityOption::set_auto_fire_on_hold);
    godot::ClassDB::bind_method(godot::D_METHOD("get_auto_fire_on_hold"), &GameplayAccessibilityOption::get_auto_fire_on_hold);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "auto_fire_on_hold"), "set_auto_fire_on_hold", "get_auto_fire_on_hold");

    godot::ClassDB::bind_method(godot::D_METHOD("set_single_stick_camera_assist", "assist"), &GameplayAccessibilityOption::set_single_stick_camera_assist);
    godot::ClassDB::bind_method(godot::D_METHOD("get_single_stick_camera_assist"), &GameplayAccessibilityOption::get_single_stick_camera_assist);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "single_stick_camera_assist"), "set_single_stick_camera_assist", "get_single_stick_camera_assist");

    godot::ClassDB::bind_method(godot::D_METHOD("set_enemy_group_tag", "tag"), &GameplayAccessibilityOption::set_enemy_group_tag);
    godot::ClassDB::bind_method(godot::D_METHOD("get_enemy_group_tag"), &GameplayAccessibilityOption::get_enemy_group_tag);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING_NAME, "enemy_group_tag"), "set_enemy_group_tag", "get_enemy_group_tag");

    godot::ClassDB::bind_method(godot::D_METHOD("set_enemy_weapon_group_tag", "tag"), &GameplayAccessibilityOption::set_enemy_weapon_group_tag);
    godot::ClassDB::bind_method(godot::D_METHOD("get_enemy_weapon_group_tag"), &GameplayAccessibilityOption::get_enemy_weapon_group_tag);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING_NAME, "enemy_weapon_group_tag"), "set_enemy_weapon_group_tag", "get_enemy_weapon_group_tag");

    godot::ClassDB::bind_method(godot::D_METHOD("set_player_group_tag", "tag"), &GameplayAccessibilityOption::set_player_group_tag);
    godot::ClassDB::bind_method(godot::D_METHOD("get_player_group_tag"), &GameplayAccessibilityOption::get_player_group_tag);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING_NAME, "player_group_tag"), "set_player_group_tag", "get_player_group_tag");

    // --- Methods ---
    godot::ClassDB::bind_method(godot::D_METHOD("apply_accessibility_option", "other"), &GameplayAccessibilityOption::apply_accessibility_option);
    godot::ClassDB::bind_method(godot::D_METHOD("apply_to_scene", "window", "environment", "scene_root"), &GameplayAccessibilityOption::apply_to_scene);
}

GameplayAccessibilityOption::GameplayAccessibilityOption() {
    set_category(godot::StringName("Accessibility"));
    set_config_section("accessibility");

    // Default target groups for TPS demo & general 3D action games
    highlight_target_groups.append(godot::StringName("RedRobot"));
    highlight_target_groups.append(godot::StringName("Enemy"));
    highlight_target_groups.append(godot::StringName("Door"));
    highlight_target_groups.append(godot::StringName("Interactable"));
    highlight_target_groups.append(godot::StringName("Hazard"));
}

GameplayAccessibilityOption::~GameplayAccessibilityOption() {
}

// --- 1. UI Theme & Typography Setters / Getters ---

void GameplayAccessibilityOption::set_ui_theme_override(const godot::Ref<godot::Theme> &p_theme) {
    if (ui_theme_override == p_theme) return;
    ui_theme_override = p_theme;
    emit_changed();
}
godot::Ref<godot::Theme> GameplayAccessibilityOption::get_ui_theme_override() const { return ui_theme_override; }

void GameplayAccessibilityOption::set_theme_registry(const godot::Ref<ThemeRegistry> &p_registry) {
    if (theme_registry == p_registry) return;
    theme_registry = p_registry;
    emit_changed();
}
godot::Ref<ThemeRegistry> GameplayAccessibilityOption::get_theme_registry() const { return theme_registry; }

void GameplayAccessibilityOption::set_theme_registry_index(int p_index) {
    if (theme_registry_index == p_index) return;
    theme_registry_index = p_index;
    emit_changed();
}
int GameplayAccessibilityOption::get_theme_registry_index() const { return theme_registry_index; }

void GameplayAccessibilityOption::set_high_contrast_preset(HighContrastPreset p_preset) {
    if (high_contrast_preset == p_preset) return;
    high_contrast_preset = p_preset;
    emit_changed();
}
HighContrastPreset GameplayAccessibilityOption::get_high_contrast_preset() const { return high_contrast_preset; }

void GameplayAccessibilityOption::set_ui_scale(float p_scale) {
    float clamped = godot::Math::clamp(p_scale, 0.5f, 2.5f);
    if (godot::Math::is_equal_approx(ui_scale, clamped)) return;
    ui_scale = clamped;
    emit_changed();
}
float GameplayAccessibilityOption::get_ui_scale() const { return ui_scale; }

void GameplayAccessibilityOption::set_font_scale(float p_scale) {
    float clamped = godot::Math::clamp(p_scale, 0.5f, 2.5f);
    if (godot::Math::is_equal_approx(font_scale, clamped)) return;
    font_scale = clamped;
    emit_changed();
}
float GameplayAccessibilityOption::get_font_scale() const { return font_scale; }

void GameplayAccessibilityOption::set_dyslexia_friendly_font(const godot::Ref<godot::Font> &p_font) {
    if (dyslexia_friendly_font == p_font) return;
    dyslexia_friendly_font = p_font;
    emit_changed();
}
godot::Ref<godot::Font> GameplayAccessibilityOption::get_dyslexia_friendly_font() const { return dyslexia_friendly_font; }

void GameplayAccessibilityOption::set_stylebox_border_width_scale(float p_scale) {
    float clamped = godot::Math::clamp(p_scale, 0.0f, 4.0f);
    if (godot::Math::is_equal_approx(stylebox_border_width_scale, clamped)) return;
    stylebox_border_width_scale = clamped;
    emit_changed();
}
float GameplayAccessibilityOption::get_stylebox_border_width_scale() const { return stylebox_border_width_scale; }

void GameplayAccessibilityOption::set_ui_background_opacity(float p_opacity) {
    float clamped = godot::Math::clamp(p_opacity, 0.0f, 1.0f);
    if (godot::Math::is_equal_approx(ui_background_opacity, clamped)) return;
    ui_background_opacity = clamped;
    emit_changed();
}
float GameplayAccessibilityOption::get_ui_background_opacity() const { return ui_background_opacity; }

// --- 2. Vision & Shader Overrides Setters / Getters ---

void GameplayAccessibilityOption::set_color_vision_mode(ColorVisionMode p_mode) {
    if (color_vision_mode == p_mode) return;
    color_vision_mode = p_mode;
    emit_changed();
}
ColorVisionMode GameplayAccessibilityOption::get_color_vision_mode() const { return color_vision_mode; }

void GameplayAccessibilityOption::set_color_vision_strength(float p_strength) {
    float clamped = godot::Math::clamp(p_strength, 0.0f, 1.0f);
    if (godot::Math::is_equal_approx(color_vision_strength, clamped)) return;
    color_vision_strength = clamped;
    emit_changed();
}
float GameplayAccessibilityOption::get_color_vision_strength() const { return color_vision_strength; }

void GameplayAccessibilityOption::set_color_correction_lut(const godot::Ref<godot::Texture2D> &p_lut) {
    if (color_correction_lut == p_lut) return;
    color_correction_lut = p_lut;
    emit_changed();
}
godot::Ref<godot::Texture2D> GameplayAccessibilityOption::get_color_correction_lut() const { return color_correction_lut; }

void GameplayAccessibilityOption::set_screen_post_process_material(const godot::Ref<godot::ShaderMaterial> &p_material) {
    if (screen_post_process_material == p_material) return;
    screen_post_process_material = p_material;
    emit_changed();
}
godot::Ref<godot::ShaderMaterial> GameplayAccessibilityOption::get_screen_post_process_material() const { return screen_post_process_material; }

void GameplayAccessibilityOption::set_high_contrast_entity_outlines(bool p_enabled) {
    if (high_contrast_entity_outlines == p_enabled) return;
    high_contrast_entity_outlines = p_enabled;
    emit_changed();
}
bool GameplayAccessibilityOption::get_high_contrast_entity_outlines() const { return high_contrast_entity_outlines; }

void GameplayAccessibilityOption::set_entity_highlight_material(const godot::Ref<godot::Material> &p_material) {
    if (entity_highlight_material == p_material) return;
    entity_highlight_material = p_material;
    emit_changed();
}
godot::Ref<godot::Material> GameplayAccessibilityOption::get_entity_highlight_material() const { return entity_highlight_material; }

void GameplayAccessibilityOption::set_enemy_highlight_color(const godot::Color &p_color) {
    if (enemy_highlight_color == p_color) return;
    enemy_highlight_color = p_color;
    emit_changed();
}
godot::Color GameplayAccessibilityOption::get_enemy_highlight_color() const { return enemy_highlight_color; }

void GameplayAccessibilityOption::set_interactable_highlight_color(const godot::Color &p_color) {
    if (interactable_highlight_color == p_color) return;
    interactable_highlight_color = p_color;
    emit_changed();
}
godot::Color GameplayAccessibilityOption::get_interactable_highlight_color() const { return interactable_highlight_color; }

void GameplayAccessibilityOption::set_hazard_highlight_color(const godot::Color &p_color) {
    if (hazard_highlight_color == p_color) return;
    hazard_highlight_color = p_color;
    emit_changed();
}
godot::Color GameplayAccessibilityOption::get_hazard_highlight_color() const { return hazard_highlight_color; }

void GameplayAccessibilityOption::set_highlight_target_groups(const godot::TypedArray<godot::StringName> &p_groups) {
    if (highlight_target_groups == p_groups) return;
    highlight_target_groups = p_groups;
    emit_changed();
}
godot::TypedArray<godot::StringName> GameplayAccessibilityOption::get_highlight_target_groups() const { return highlight_target_groups; }

void GameplayAccessibilityOption::set_brightness_adjustment(float p_brightness) {
    float clamped = godot::Math::clamp(p_brightness, 0.2f, 3.0f);
    if (godot::Math::is_equal_approx(brightness_adjustment, clamped)) return;
    brightness_adjustment = clamped;
    emit_changed();
}
float GameplayAccessibilityOption::get_brightness_adjustment() const { return brightness_adjustment; }

void GameplayAccessibilityOption::set_contrast_adjustment(float p_contrast) {
    float clamped = godot::Math::clamp(p_contrast, 0.2f, 3.0f);
    if (godot::Math::is_equal_approx(contrast_adjustment, clamped)) return;
    contrast_adjustment = clamped;
    emit_changed();
}
float GameplayAccessibilityOption::get_contrast_adjustment() const { return contrast_adjustment; }

void GameplayAccessibilityOption::set_saturation_adjustment(float p_saturation) {
    float clamped = godot::Math::clamp(p_saturation, 0.0f, 3.0f);
    if (godot::Math::is_equal_approx(saturation_adjustment, clamped)) return;
    saturation_adjustment = clamped;
    emit_changed();
}
float GameplayAccessibilityOption::get_saturation_adjustment() const { return saturation_adjustment; }

void GameplayAccessibilityOption::set_disable_volumetric_fog_for_clarity(bool p_disable) {
    if (disable_volumetric_fog_for_clarity == p_disable) return;
    disable_volumetric_fog_for_clarity = p_disable;
    emit_changed();
}
bool GameplayAccessibilityOption::get_disable_volumetric_fog_for_clarity() const { return disable_volumetric_fog_for_clarity; }

// --- 3. Motion & Photosensitivity Setters / Getters ---

void GameplayAccessibilityOption::set_reduce_flashing_lights(bool p_reduce) {
    if (reduce_flashing_lights == p_reduce) return;
    reduce_flashing_lights = p_reduce;
    emit_changed();
}
bool GameplayAccessibilityOption::get_reduce_flashing_lights() const { return reduce_flashing_lights; }

void GameplayAccessibilityOption::set_flash_intensity_cap(float p_cap) {
    float clamped = godot::Math::clamp(p_cap, 0.0f, 2.0f);
    if (godot::Math::is_equal_approx(flash_intensity_cap, clamped)) return;
    flash_intensity_cap = clamped;
    emit_changed();
}
float GameplayAccessibilityOption::get_flash_intensity_cap() const { return flash_intensity_cap; }

void GameplayAccessibilityOption::set_screen_shake_scale(float p_scale) {
    float clamped = godot::Math::clamp(p_scale, 0.0f, 1.0f);
    if (godot::Math::is_equal_approx(screen_shake_scale, clamped)) return;
    screen_shake_scale = clamped;
    emit_changed();
}
float GameplayAccessibilityOption::get_screen_shake_scale() const { return screen_shake_scale; }

void GameplayAccessibilityOption::set_motion_blur_scale(float p_scale) {
    float clamped = godot::Math::clamp(p_scale, 0.0f, 1.0f);
    if (godot::Math::is_equal_approx(motion_blur_scale, clamped)) return;
    motion_blur_scale = clamped;
    emit_changed();
}
float GameplayAccessibilityOption::get_motion_blur_scale() const { return motion_blur_scale; }

void GameplayAccessibilityOption::set_reduce_camera_bob(bool p_reduce) {
    if (reduce_camera_bob == p_reduce) return;
    reduce_camera_bob = p_reduce;
    emit_changed();
}
bool GameplayAccessibilityOption::get_reduce_camera_bob() const { return reduce_camera_bob; }

void GameplayAccessibilityOption::set_reticle_scale(float p_scale) {
    float clamped = godot::Math::clamp(p_scale, 0.5f, 3.0f);
    if (godot::Math::is_equal_approx(reticle_scale, clamped)) return;
    reticle_scale = clamped;
    emit_changed();
}
float GameplayAccessibilityOption::get_reticle_scale() const { return reticle_scale; }

void GameplayAccessibilityOption::set_fov_override(float p_fov) {
    float clamped = godot::Math::clamp(p_fov, 0.0f, 130.0f);
    if (godot::Math::is_equal_approx(fov_override, clamped)) return;
    fov_override = clamped;
    emit_changed();
}
float GameplayAccessibilityOption::get_fov_override() const { return fov_override; }

// --- 4. Audio, Captions & Cymatics Setters / Getters ---

void GameplayAccessibilityOption::set_closed_captions_enabled(bool p_enabled) {
    if (closed_captions_enabled == p_enabled) return;
    closed_captions_enabled = p_enabled;
    emit_changed();
}
bool GameplayAccessibilityOption::get_closed_captions_enabled() const { return closed_captions_enabled; }

void GameplayAccessibilityOption::set_caption_text_scale(float p_scale) {
    float clamped = godot::Math::clamp(p_scale, 0.5f, 2.5f);
    if (godot::Math::is_equal_approx(caption_text_scale, clamped)) return;
    caption_text_scale = clamped;
    emit_changed();
}
float GameplayAccessibilityOption::get_caption_text_scale() const { return caption_text_scale; }

void GameplayAccessibilityOption::set_caption_background_opacity(float p_opacity) {
    float clamped = godot::Math::clamp(p_opacity, 0.0f, 1.0f);
    if (godot::Math::is_equal_approx(caption_background_opacity, clamped)) return;
    caption_background_opacity = clamped;
    emit_changed();
}
float GameplayAccessibilityOption::get_caption_background_opacity() const { return caption_background_opacity; }

void GameplayAccessibilityOption::set_caption_speaker_labels(bool p_enabled) {
    if (caption_speaker_labels == p_enabled) return;
    caption_speaker_labels = p_enabled;
    emit_changed();
}
bool GameplayAccessibilityOption::get_caption_speaker_labels() const { return caption_speaker_labels; }

void GameplayAccessibilityOption::set_mono_audio_downmix(bool p_mono) {
    if (mono_audio_downmix == p_mono) return;
    mono_audio_downmix = p_mono;
    emit_changed();
}
bool GameplayAccessibilityOption::get_mono_audio_downmix() const { return mono_audio_downmix; }

void GameplayAccessibilityOption::set_visual_sound_indicators(bool p_enabled) {
    if (visual_sound_indicators == p_enabled) return;
    visual_sound_indicators = p_enabled;
    emit_changed();
}
bool GameplayAccessibilityOption::get_visual_sound_indicators() const { return visual_sound_indicators; }

void GameplayAccessibilityOption::set_cymatic_visual_gain(float p_gain) {
    float clamped = godot::Math::clamp(p_gain, 0.0f, 4.0f);
    if (godot::Math::is_equal_approx(cymatic_visual_gain, clamped)) return;
    cymatic_visual_gain = clamped;
    emit_changed();
}
float GameplayAccessibilityOption::get_cymatic_visual_gain() const { return cymatic_visual_gain; }

// --- 5. Motor, Input & Cognitive Assists Setters / Getters ---

void GameplayAccessibilityOption::set_game_speed_scale(float p_scale) {
    float clamped = godot::Math::clamp(p_scale, 0.1f, 2.0f);
    if (godot::Math::is_equal_approx(game_speed_scale, clamped)) return;
    game_speed_scale = clamped;
    emit_changed();
}
float GameplayAccessibilityOption::get_game_speed_scale() const { return game_speed_scale; }

void GameplayAccessibilityOption::set_enemy_time_scale(float p_scale) {
    float clamped = godot::Math::clamp(p_scale, 0.1f, 2.0f);
    if (godot::Math::is_equal_approx(enemy_time_scale, clamped)) return;
    enemy_time_scale = clamped;
    emit_changed();
}
float GameplayAccessibilityOption::get_enemy_time_scale() const { return enemy_time_scale; }

void GameplayAccessibilityOption::set_enemy_attack_charge_multiplier(float p_mult) {
    float clamped = godot::Math::clamp(p_mult, 0.25f, 3.0f);
    if (godot::Math::is_equal_approx(enemy_attack_charge_multiplier, clamped)) return;
    enemy_attack_charge_multiplier = clamped;
    emit_changed();
}
float GameplayAccessibilityOption::get_enemy_attack_charge_multiplier() const { return enemy_attack_charge_multiplier; }

void GameplayAccessibilityOption::set_player_damage_taken_multiplier(float p_mult) {
    float clamped = godot::Math::clamp(p_mult, 0.0f, 2.0f);
    if (godot::Math::is_equal_approx(player_damage_taken_multiplier, clamped)) return;
    player_damage_taken_multiplier = clamped;
    emit_changed();
}
float GameplayAccessibilityOption::get_player_damage_taken_multiplier() const { return player_damage_taken_multiplier; }

void GameplayAccessibilityOption::set_interaction_timeout_multiplier(float p_mult) {
    float clamped = godot::Math::clamp(p_mult, 0.5f, 5.0f);
    if (godot::Math::is_equal_approx(interaction_timeout_multiplier, clamped)) return;
    interaction_timeout_multiplier = clamped;
    emit_changed();
}
float GameplayAccessibilityOption::get_interaction_timeout_multiplier() const { return interaction_timeout_multiplier; }

void GameplayAccessibilityOption::set_aim_assist_mode(AimAssistMode p_mode) {
    if (aim_assist_mode == p_mode) return;
    aim_assist_mode = p_mode;
    emit_changed();
}
AimAssistMode GameplayAccessibilityOption::get_aim_assist_mode() const { return aim_assist_mode; }

void GameplayAccessibilityOption::set_aim_assist_strength(float p_strength) {
    float clamped = godot::Math::clamp(p_strength, 0.0f, 1.0f);
    if (godot::Math::is_equal_approx(aim_assist_strength, clamped)) return;
    aim_assist_strength = clamped;
    emit_changed();
}
float GameplayAccessibilityOption::get_aim_assist_strength() const { return aim_assist_strength; }

void GameplayAccessibilityOption::set_aim_magnetism_radius(float p_radius) {
    float clamped = godot::Math::clamp(p_radius, 0.0f, 15.0f);
    if (godot::Math::is_equal_approx(aim_magnetism_radius, clamped)) return;
    aim_magnetism_radius = clamped;
    emit_changed();
}
float GameplayAccessibilityOption::get_aim_magnetism_radius() const { return aim_magnetism_radius; }

void GameplayAccessibilityOption::set_hold_to_toggle_aim(bool p_toggle) {
    if (hold_to_toggle_aim == p_toggle) return;
    hold_to_toggle_aim = p_toggle;
    emit_changed();
}
bool GameplayAccessibilityOption::get_hold_to_toggle_aim() const { return hold_to_toggle_aim; }

void GameplayAccessibilityOption::set_hold_to_toggle_crouch(bool p_toggle) {
    if (hold_to_toggle_crouch == p_toggle) return;
    hold_to_toggle_crouch = p_toggle;
    emit_changed();
}
bool GameplayAccessibilityOption::get_hold_to_toggle_crouch() const { return hold_to_toggle_crouch; }

void GameplayAccessibilityOption::set_hold_to_toggle_sprint(bool p_toggle) {
    if (hold_to_toggle_sprint == p_toggle) return;
    hold_to_toggle_sprint = p_toggle;
    emit_changed();
}
bool GameplayAccessibilityOption::get_hold_to_toggle_sprint() const { return hold_to_toggle_sprint; }

void GameplayAccessibilityOption::set_auto_fire_on_hold(bool p_auto_fire) {
    if (auto_fire_on_hold == p_auto_fire) return;
    auto_fire_on_hold = p_auto_fire;
    emit_changed();
}
bool GameplayAccessibilityOption::get_auto_fire_on_hold() const { return auto_fire_on_hold; }

void GameplayAccessibilityOption::set_single_stick_camera_assist(bool p_assist) {
    if (single_stick_camera_assist == p_assist) return;
    single_stick_camera_assist = p_assist;
    emit_changed();
}
bool GameplayAccessibilityOption::get_single_stick_camera_assist() const { return single_stick_camera_assist; }

void GameplayAccessibilityOption::set_enemy_group_tag(const godot::StringName &p_tag) {
    if (enemy_group_tag == p_tag) return;
    enemy_group_tag = p_tag;
    emit_changed();
}
godot::StringName GameplayAccessibilityOption::get_enemy_group_tag() const { return enemy_group_tag; }

void GameplayAccessibilityOption::set_enemy_weapon_group_tag(const godot::StringName &p_tag) {
    if (enemy_weapon_group_tag == p_tag) return;
    enemy_weapon_group_tag = p_tag;
    emit_changed();
}
godot::StringName GameplayAccessibilityOption::get_enemy_weapon_group_tag() const { return enemy_weapon_group_tag; }

void GameplayAccessibilityOption::set_player_group_tag(const godot::StringName &p_tag) {
    if (player_group_tag == p_tag) return;
    player_group_tag = p_tag;
    emit_changed();
}
godot::StringName GameplayAccessibilityOption::get_player_group_tag() const { return player_group_tag; }

// --- Internal Helpers ---

godot::Ref<godot::Theme> GameplayAccessibilityOption::_resolve_effective_theme() const {
    godot::Ref<godot::Theme> base_theme = ui_theme_override;

    // Fallback to ThemeRegistry if no explicit Theme override was assigned
    if (base_theme.is_null() && theme_registry.is_valid() && theme_registry_index >= 0) {
        godot::TypedArray<godot::String> paths = theme_registry->get_theme_paths();
        if (theme_registry_index < paths.size()) {
            godot::String theme_path = paths[theme_registry_index];
            if (!theme_path.is_empty() && godot::ResourceLoader::get_singleton()->exists(theme_path)) {
                base_theme = godot::ResourceLoader::get_singleton()->load(theme_path);
            }
        }
    }

    bool needs_runtime_customization =
        dyslexia_friendly_font.is_valid() ||
        !godot::Math::is_equal_approx(font_scale, 1.0f) ||
        high_contrast_preset != HighContrastPreset::CONTRAST_DEFAULT ||
        !godot::Math::is_equal_approx(stylebox_border_width_scale, 1.0f) ||
        !godot::Math::is_equal_approx(ui_background_opacity, 1.0f);

    if (!needs_runtime_customization) {
        return base_theme;
    }

    godot::Ref<godot::Theme> working_theme;
    if (base_theme.is_valid()) {
        working_theme = godot::Ref<godot::Theme>(godot::Object::cast_to<godot::Theme>(base_theme->duplicate(true).ptr()));
    } else {
        working_theme.instantiate();
    }

    // 1. Apply dyslexia-friendly / high-legibility font & scaled default font size
    if (dyslexia_friendly_font.is_valid()) {
        working_theme->set_default_font(dyslexia_friendly_font);
    }
    if (!godot::Math::is_equal_approx(font_scale, 1.0f)) {
        int base_font_size = working_theme->has_default_font_size() ? working_theme->get_default_font_size() : 16;
        working_theme->set_default_font_size(godot::Math::max(8, static_cast<int>(std::round(base_font_size * font_scale))));
    }

    // 2. Apply High-Contrast Preset StyleBoxes for PanelContainer, Button, and Focus Rings
    if (high_contrast_preset != HighContrastPreset::CONTRAST_DEFAULT ||
        !godot::Math::is_equal_approx(stylebox_border_width_scale, 1.0f) ||
        !godot::Math::is_equal_approx(ui_background_opacity, 1.0f)) {

        godot::Color bg_color(0.08f, 0.09f, 0.11f, ui_background_opacity);
        godot::Color border_color(0.85f, 0.85f, 0.85f, 1.0f);
        godot::Color font_color(0.95f, 0.95f, 0.95f, 1.0f);

        switch (high_contrast_preset) {
            case HighContrastPreset::CONTRAST_HIGH_DARK:
                bg_color = godot::Color(0.02f, 0.02f, 0.02f, godot::Math::max(ui_background_opacity, 0.95f));
                border_color = godot::Color(1.0f, 1.0f, 1.0f, 1.0f);
                font_color = godot::Color(1.0f, 1.0f, 1.0f, 1.0f);
                break;
            case HighContrastPreset::CONTRAST_HIGH_LIGHT:
                bg_color = godot::Color(0.98f, 0.98f, 0.98f, godot::Math::max(ui_background_opacity, 0.95f));
                border_color = godot::Color(0.0f, 0.0f, 0.0f, 1.0f);
                font_color = godot::Color(0.0f, 0.0f, 0.0f, 1.0f);
                break;
            case HighContrastPreset::CONTRAST_YELLOW_ON_BLACK:
                bg_color = godot::Color(0.0f, 0.0f, 0.0f, 0.98f);
                border_color = godot::Color(1.0f, 0.92f, 0.0f, 1.0f);
                font_color = godot::Color(1.0f, 0.92f, 0.0f, 1.0f);
                break;
            default:
                break;
        }

        int border_px = godot::Math::max(1, static_cast<int>(std::round(2.0f * stylebox_border_width_scale)));

        godot::Ref<godot::StyleBoxFlat> panel_style;
        panel_style.instantiate();
        panel_style->set_bg_color(bg_color);
        panel_style->set_border_color(border_color);
        panel_style->set_border_width_all(border_px);
        panel_style->set_corner_radius_all(4);

        godot::Ref<godot::StyleBoxFlat> focus_style;
        focus_style.instantiate();
        focus_style->set_draw_center(false);
        focus_style->set_border_color(border_color);
        focus_style->set_border_width_all(border_px + 2);
        focus_style->set_corner_radius_all(4);

        working_theme->set_stylebox("panel", "PanelContainer", panel_style);
        working_theme->set_stylebox("normal", "Button", panel_style);
        working_theme->set_stylebox("focus", "Button", focus_style);
        working_theme->set_color("font_color", "Label", font_color);
        working_theme->set_color("font_color", "Button", font_color);
    }

    return working_theme;
}

void GameplayAccessibilityOption::_apply_entity_highlights_recursive(godot::Node *p_node) const {
    if (!p_node) {
        return;
    }

    if (godot::GeometryInstance3D *geom = godot::Object::cast_to<godot::GeometryInstance3D>(p_node)) {
        bool matches_group = false;
        godot::StringName matched_group;

        // Check both the GeometryInstance3D itself and its ancestor nodes for group membership
        godot::Node *cursor = p_node;
        while (cursor && !matches_group) {
            for (int i = 0; i < highlight_target_groups.size(); ++i) {
                godot::StringName grp = highlight_target_groups[i];
                if (!grp.is_empty() && cursor->is_in_group(grp)) {
                    matches_group = true;
                    matched_group = grp;
                    break;
                }
            }
            cursor = cursor->get_parent();
        }

        if (matches_group) {
            if (get_enabled() && high_contrast_entity_outlines && entity_highlight_material.is_valid()) {
                // If the highlight material is a ShaderMaterial, pass per-category highlight color
                if (godot::ShaderMaterial *shader_mat = godot::Object::cast_to<godot::ShaderMaterial>(entity_highlight_material.ptr())) {
                    godot::Ref<godot::ShaderMaterial> inst_mat(godot::Object::cast_to<godot::ShaderMaterial>(shader_mat->duplicate().ptr()));
                    if (inst_mat.is_valid()) {
                        godot::Color chosen_color = interactable_highlight_color;
                        if (matched_group == enemy_group_tag || matched_group == godot::StringName("Enemy")) {
                            chosen_color = enemy_highlight_color;
                        } else if (matched_group == godot::StringName("Hazard")) {
                            chosen_color = hazard_highlight_color;
                        }
                        inst_mat->set_shader_parameter("highlight_color", chosen_color);
                        geom->set_material_overlay(inst_mat);
                    }
                } else {
                    geom->set_material_overlay(entity_highlight_material);
                }
            } else {
                geom->set_material_overlay(godot::Ref<godot::Material>());
            }
        }
    }

    int child_count = p_node->get_child_count();
    for (int i = 0; i < child_count; ++i) {
        _apply_entity_highlights_recursive(p_node->get_child(i));
    }
}

// --- Application, Rule Synthesis & Persistence ---

void GameplayAccessibilityOption::apply_accessibility_option(const godot::Ref<GameplayAccessibilityOption> &p_other) {
    if (!p_other.is_valid()) {
        return;
    }

    GameplayResource::apply_resource(p_other);

    // 1. UI Theme & Typography
    ui_theme_override = p_other->get_ui_theme_override();
    theme_registry = p_other->get_theme_registry();
    theme_registry_index = p_other->get_theme_registry_index();
    high_contrast_preset = p_other->get_high_contrast_preset();
    ui_scale = p_other->get_ui_scale();
    font_scale = p_other->get_font_scale();
    dyslexia_friendly_font = p_other->get_dyslexia_friendly_font();
    stylebox_border_width_scale = p_other->get_stylebox_border_width_scale();
    ui_background_opacity = p_other->get_ui_background_opacity();

    // 2. Vision & Shader Overrides
    color_vision_mode = p_other->get_color_vision_mode();
    color_vision_strength = p_other->get_color_vision_strength();
    color_correction_lut = p_other->get_color_correction_lut();
    screen_post_process_material = p_other->get_screen_post_process_material();
    high_contrast_entity_outlines = p_other->get_high_contrast_entity_outlines();
    entity_highlight_material = p_other->get_entity_highlight_material();
    enemy_highlight_color = p_other->get_enemy_highlight_color();
    interactable_highlight_color = p_other->get_interactable_highlight_color();
    hazard_highlight_color = p_other->get_hazard_highlight_color();
    highlight_target_groups = p_other->get_highlight_target_groups().duplicate();
    brightness_adjustment = p_other->get_brightness_adjustment();
    contrast_adjustment = p_other->get_contrast_adjustment();
    saturation_adjustment = p_other->get_saturation_adjustment();
    disable_volumetric_fog_for_clarity = p_other->get_disable_volumetric_fog_for_clarity();

    // 3. Motion & Photosensitivity
    reduce_flashing_lights = p_other->get_reduce_flashing_lights();
    flash_intensity_cap = p_other->get_flash_intensity_cap();
    screen_shake_scale = p_other->get_screen_shake_scale();
    motion_blur_scale = p_other->get_motion_blur_scale();
    reduce_camera_bob = p_other->get_reduce_camera_bob();
    reticle_scale = p_other->get_reticle_scale();
    fov_override = p_other->get_fov_override();

    // 4. Audio, Captions & Cymatics
    closed_captions_enabled = p_other->get_closed_captions_enabled();
    caption_text_scale = p_other->get_caption_text_scale();
    caption_background_opacity = p_other->get_caption_background_opacity();
    caption_speaker_labels = p_other->get_caption_speaker_labels();
    mono_audio_downmix = p_other->get_mono_audio_downmix();
    visual_sound_indicators = p_other->get_visual_sound_indicators();
    cymatic_visual_gain = p_other->get_cymatic_visual_gain();

    // 5. Motor, Input & Cognitive Assists
    game_speed_scale = p_other->get_game_speed_scale();
    enemy_time_scale = p_other->get_enemy_time_scale();
    enemy_attack_charge_multiplier = p_other->get_enemy_attack_charge_multiplier();
    player_damage_taken_multiplier = p_other->get_player_damage_taken_multiplier();
    interaction_timeout_multiplier = p_other->get_interaction_timeout_multiplier();
    aim_assist_mode = p_other->get_aim_assist_mode();
    aim_assist_strength = p_other->get_aim_assist_strength();
    aim_magnetism_radius = p_other->get_aim_magnetism_radius();
    hold_to_toggle_aim = p_other->get_hold_to_toggle_aim();
    hold_to_toggle_crouch = p_other->get_hold_to_toggle_crouch();
    hold_to_toggle_sprint = p_other->get_hold_to_toggle_sprint();
    auto_fire_on_hold = p_other->get_auto_fire_on_hold();
    single_stick_camera_assist = p_other->get_single_stick_camera_assist();
    enemy_group_tag = p_other->get_enemy_group_tag();
    enemy_weapon_group_tag = p_other->get_enemy_weapon_group_tag();
    player_group_tag = p_other->get_player_group_tag();

    emit_changed();
}

void GameplayAccessibilityOption::apply_resource(const godot::Ref<GameplayResource> &p_other) {
    godot::Ref<GameplayAccessibilityOption> acc_other = p_other;
    if (acc_other.is_valid()) {
        apply_accessibility_option(acc_other);
    } else {
        GameplayResource::apply_resource(p_other);
    }
}

void GameplayAccessibilityOption::apply_to_scene(godot::Window *p_window, godot::Environment *p_environment, godot::Node *p_scene_root) {
    // 1. Apply Window UI Scale & Theme Overrides
    if (p_window) {
        p_window->set_content_scale_factor(get_enabled() ? ui_scale : 1.0f);
        godot::Ref<godot::Theme> effective_theme = get_enabled() ? _resolve_effective_theme() : godot::Ref<godot::Theme>();
        p_window->set_theme(effective_theme);
    }

    // 2. Apply Fullscreen Post-Process Shader Uniforms if configured
    if (screen_post_process_material.is_valid()) {
        screen_post_process_material->set_shader_parameter("accessibility_enabled", get_enabled());
        screen_post_process_material->set_shader_parameter("cvd_mode", static_cast<int>(color_vision_mode));
        screen_post_process_material->set_shader_parameter("cvd_strength", color_vision_strength);
        screen_post_process_material->set_shader_parameter("high_contrast_preset", static_cast<int>(high_contrast_preset));
    }

    // 3. Apply Environment Color Adjustments, LUT, Volumetric Fog, and Photosensitivity Caps
    if (p_environment) {
        if (get_enabled()) {
            bool needs_adjustments =
                !godot::Math::is_equal_approx(brightness_adjustment, 1.0f) ||
                !godot::Math::is_equal_approx(contrast_adjustment, 1.0f) ||
                !godot::Math::is_equal_approx(saturation_adjustment, 1.0f) ||
                color_vision_mode == ColorVisionMode::CVD_ACHROMATOPSIA ||
                color_correction_lut.is_valid();

            p_environment->set_adjustment_enabled(needs_adjustments);
            if (needs_adjustments) {
                p_environment->set_adjustment_brightness(brightness_adjustment);
                p_environment->set_adjustment_contrast(contrast_adjustment);
                float effective_saturation = (color_vision_mode == ColorVisionMode::CVD_ACHROMATOPSIA)
                    ? (1.0f - color_vision_strength) * saturation_adjustment
                    : saturation_adjustment;
                p_environment->set_adjustment_saturation(effective_saturation);
                if (color_correction_lut.is_valid()) {
                    p_environment->set_adjustment_color_correction(color_correction_lut);
                }
            }

            if (reduce_flashing_lights && p_environment->get_glow_intensity() > flash_intensity_cap) {
                p_environment->set_glow_intensity(flash_intensity_cap);
            }

            if (disable_volumetric_fog_for_clarity) {
                p_environment->set_volumetric_fog_enabled(false);
            }
        }
    }

    // 4. Apply 3D Entity Highlight Material Overlays across Scene Root
    if (p_scene_root) {
        _apply_entity_highlights_recursive(p_scene_root);
    }
}

godot::TypedArray<GameplayRule> GameplayAccessibilityOption::collect_rules() const {
    godot::TypedArray<GameplayRule> collected = GameplayResource::collect_rules();
    if (!get_enabled()) {
        return collected;
    }

    // 1. Global Game Speed Scale -> cascades down to descendant GameEntities
    if (!godot::Math::is_equal_approx(game_speed_scale, 1.0f)) {
        godot::Ref<GameplayRule> speed_rule;
        speed_rule.instantiate();
        speed_rule->set_title("Accessibility_GameSpeedScale");
        speed_rule->set_scope(RuleScope::ALL);
        speed_rule->set_target_class(godot::StringName("GameEntity"));
        speed_rule->set_target_property(godot::StringName("time_scale"));
        speed_rule->set_operation(RuleOp::MULTIPLY);
        speed_rule->set_value(game_speed_scale);
        speed_rule->set_priority(90);
        speed_rule->set_is_important(true);
        collected.append(speed_rule);
    }

    // 2. Enemy Time Scale Assist (e.g., slows Red Robots while Player remains at normal speed)
    if (!godot::Math::is_equal_approx(enemy_time_scale, 1.0f) && !enemy_group_tag.is_empty()) {
        godot::Ref<GameplayRule> enemy_speed_rule;
        enemy_speed_rule.instantiate();
        enemy_speed_rule->set_title("Accessibility_EnemyTimeScale");
        enemy_speed_rule->set_scope(RuleScope::DESCENDANTS);
        enemy_speed_rule->set_target_class(godot::StringName("GameAgent"));
        enemy_speed_rule->set_target_title_or_group(enemy_group_tag);
        enemy_speed_rule->set_target_property(godot::StringName("time_scale"));
        enemy_speed_rule->set_operation(RuleOp::MULTIPLY);
        enemy_speed_rule->set_value(enemy_time_scale);
        enemy_speed_rule->set_priority(100);
        enemy_speed_rule->set_is_important(true);
        collected.append(enemy_speed_rule);
    }

    // 3. Enemy Attack Telegraph / Cannon Charge Multiplier
    if (!godot::Math::is_equal_approx(enemy_attack_charge_multiplier, 1.0f) && !enemy_weapon_group_tag.is_empty()) {
        godot::Ref<GameplayRule> charge_rule;
        charge_rule.instantiate();
        charge_rule->set_title("Accessibility_EnemyChargeTime");
        charge_rule->set_scope(RuleScope::DESCENDANTS);
        charge_rule->set_target_class(godot::StringName("GamePiece"));
        charge_rule->set_target_title_or_group(enemy_weapon_group_tag);
        charge_rule->set_target_property(godot::StringName("ChargeTime"));
        charge_rule->set_operation(RuleOp::MULTIPLY);
        charge_rule->set_value(enemy_attack_charge_multiplier);
        charge_rule->set_priority(100);
        charge_rule->set_is_important(true);
        collected.append(charge_rule);
    }

    // 4. Player Damage Taken / Survivability Assist (scales Player Health inversely to damage multiplier)
    if (!godot::Math::is_equal_approx(player_damage_taken_multiplier, 1.0f) && !player_group_tag.is_empty()) {
        float health_scale = (player_damage_taken_multiplier > 0.05f)
            ? (1.0f / player_damage_taken_multiplier)
            : 20.0f;
        godot::Ref<GameplayRule> health_rule;
        health_rule.instantiate();
        health_rule->set_title("Accessibility_PlayerHealthScale");
        health_rule->set_scope(RuleScope::DESCENDANTS);
        health_rule->set_target_class(godot::StringName("GameEntity"));
        health_rule->set_target_title_or_group(player_group_tag);
        health_rule->set_target_property(godot::StringName("Health"));
        health_rule->set_operation(RuleOp::MULTIPLY);
        health_rule->set_value(health_scale);
        health_rule->set_priority(100);
        health_rule->set_is_important(true);
        collected.append(health_rule);
    }

    return collected;
}

godot::Dictionary GameplayAccessibilityOption::save_resource() const {
    godot::Dictionary data = GameplayResource::save_resource();

    // 1. UI Theme & Typography
    if (ui_theme_override.is_valid() && !ui_theme_override->get_path().is_empty()) {
        data["ui_theme_override_path"] = ui_theme_override->get_path();
    }
    if (theme_registry.is_valid() && !theme_registry->get_path().is_empty()) {
        data["theme_registry_path"] = theme_registry->get_path();
    }
    data["theme_registry_index"] = theme_registry_index;
    data["high_contrast_preset"] = static_cast<int>(high_contrast_preset);
    data["ui_scale"] = ui_scale;
    data["font_scale"] = font_scale;
    if (dyslexia_friendly_font.is_valid() && !dyslexia_friendly_font->get_path().is_empty()) {
        data["dyslexia_friendly_font_path"] = dyslexia_friendly_font->get_path();
    }
    data["stylebox_border_width_scale"] = stylebox_border_width_scale;
    data["ui_background_opacity"] = ui_background_opacity;

    // 2. Vision & Shader Overrides
    data["color_vision_mode"] = static_cast<int>(color_vision_mode);
    data["color_vision_strength"] = color_vision_strength;
    if (color_correction_lut.is_valid() && !color_correction_lut->get_path().is_empty()) {
        data["color_correction_lut_path"] = color_correction_lut->get_path();
    }
    if (screen_post_process_material.is_valid() && !screen_post_process_material->get_path().is_empty()) {
        data["screen_post_process_material_path"] = screen_post_process_material->get_path();
    }
    data["high_contrast_entity_outlines"] = high_contrast_entity_outlines;
    if (entity_highlight_material.is_valid() && !entity_highlight_material->get_path().is_empty()) {
        data["entity_highlight_material_path"] = entity_highlight_material->get_path();
    }
    data["enemy_highlight_color"] = enemy_highlight_color;
    data["interactable_highlight_color"] = interactable_highlight_color;
    data["hazard_highlight_color"] = hazard_highlight_color;

    godot::Array groups_arr;
    for (int i = 0; i < highlight_target_groups.size(); ++i) {
        groups_arr.append(godot::String(godot::StringName(highlight_target_groups[i])));
    }
    data["highlight_target_groups"] = groups_arr;

    data["brightness_adjustment"] = brightness_adjustment;
    data["contrast_adjustment"] = contrast_adjustment;
    data["saturation_adjustment"] = saturation_adjustment;
    data["disable_volumetric_fog_for_clarity"] = disable_volumetric_fog_for_clarity;

    // 3. Motion & Photosensitivity
    data["reduce_flashing_lights"] = reduce_flashing_lights;
    data["flash_intensity_cap"] = flash_intensity_cap;
    data["screen_shake_scale"] = screen_shake_scale;
    data["motion_blur_scale"] = motion_blur_scale;
    data["reduce_camera_bob"] = reduce_camera_bob;
    data["reticle_scale"] = reticle_scale;
    data["fov_override"] = fov_override;

    // 4. Audio, Captions & Cymatics
    data["closed_captions_enabled"] = closed_captions_enabled;
    data["caption_text_scale"] = caption_text_scale;
    data["caption_background_opacity"] = caption_background_opacity;
    data["caption_speaker_labels"] = caption_speaker_labels;
    data["mono_audio_downmix"] = mono_audio_downmix;
    data["visual_sound_indicators"] = visual_sound_indicators;
    data["cymatic_visual_gain"] = cymatic_visual_gain;

    // 5. Motor, Input & Cognitive Assists
    data["game_speed_scale"] = game_speed_scale;
    data["enemy_time_scale"] = enemy_time_scale;
    data["enemy_attack_charge_multiplier"] = enemy_attack_charge_multiplier;
    data["player_damage_taken_multiplier"] = player_damage_taken_multiplier;
    data["interaction_timeout_multiplier"] = interaction_timeout_multiplier;
    data["aim_assist_mode"] = static_cast<int>(aim_assist_mode);
    data["aim_assist_strength"] = aim_assist_strength;
    data["aim_magnetism_radius"] = aim_magnetism_radius;
    data["hold_to_toggle_aim"] = hold_to_toggle_aim;
    data["hold_to_toggle_crouch"] = hold_to_toggle_crouch;
    data["hold_to_toggle_sprint"] = hold_to_toggle_sprint;
    data["auto_fire_on_hold"] = auto_fire_on_hold;
    data["single_stick_camera_assist"] = single_stick_camera_assist;
    data["enemy_group_tag"] = godot::String(enemy_group_tag);
    data["enemy_weapon_group_tag"] = godot::String(enemy_weapon_group_tag);
    data["player_group_tag"] = godot::String(player_group_tag);

    return data;
}

void GameplayAccessibilityOption::load_resource(const godot::Dictionary &p_data) {
    GameplayResource::load_resource(p_data);

    godot::ResourceLoader *loader = godot::ResourceLoader::get_singleton();

    // 1. UI Theme & Typography
    if (p_data.has("ui_theme_override_path")) {
        godot::String path = p_data["ui_theme_override_path"];
        if (!path.is_empty() && loader->exists(path)) {
            ui_theme_override = loader->load(path);
        }
    }
    if (p_data.has("theme_registry_path")) {
        godot::String path = p_data["theme_registry_path"];
        if (!path.is_empty() && loader->exists(path)) {
            theme_registry = loader->load(path);
        }
    }
    if (p_data.has("theme_registry_index")) theme_registry_index = p_data["theme_registry_index"];
    if (p_data.has("high_contrast_preset")) high_contrast_preset = static_cast<HighContrastPreset>(static_cast<int>(p_data["high_contrast_preset"]));
    if (p_data.has("ui_scale")) set_ui_scale(p_data["ui_scale"]);
    if (p_data.has("font_scale")) set_font_scale(p_data["font_scale"]);
    if (p_data.has("dyslexia_friendly_font_path")) {
        godot::String path = p_data["dyslexia_friendly_font_path"];
        if (!path.is_empty() && loader->exists(path)) {
            dyslexia_friendly_font = loader->load(path);
        }
    }
    if (p_data.has("stylebox_border_width_scale")) set_stylebox_border_width_scale(p_data["stylebox_border_width_scale"]);
    if (p_data.has("ui_background_opacity")) set_ui_background_opacity(p_data["ui_background_opacity"]);

    // 2. Vision & Shader Overrides
    if (p_data.has("color_vision_mode")) color_vision_mode = static_cast<ColorVisionMode>(static_cast<int>(p_data["color_vision_mode"]));
    if (p_data.has("color_vision_strength")) set_color_vision_strength(p_data["color_vision_strength"]);
    if (p_data.has("color_correction_lut_path")) {
        godot::String path = p_data["color_correction_lut_path"];
        if (!path.is_empty() && loader->exists(path)) {
            color_correction_lut = loader->load(path);
        }
    }
    if (p_data.has("screen_post_process_material_path")) {
        godot::String path = p_data["screen_post_process_material_path"];
        if (!path.is_empty() && loader->exists(path)) {
            screen_post_process_material = loader->load(path);
        }
    }
    if (p_data.has("high_contrast_entity_outlines")) high_contrast_entity_outlines = p_data["high_contrast_entity_outlines"];
    if (p_data.has("entity_highlight_material_path")) {
        godot::String path = p_data["entity_highlight_material_path"];
        if (!path.is_empty() && loader->exists(path)) {
            entity_highlight_material = loader->load(path);
        }
    }
    if (p_data.has("enemy_highlight_color")) enemy_highlight_color = p_data["enemy_highlight_color"];
    if (p_data.has("interactable_highlight_color")) interactable_highlight_color = p_data["interactable_highlight_color"];
    if (p_data.has("hazard_highlight_color")) hazard_highlight_color = p_data["hazard_highlight_color"];

    if (p_data.has("highlight_target_groups") && p_data["highlight_target_groups"].get_type() == godot::Variant::ARRAY) {
        highlight_target_groups.clear();
        godot::Array arr = p_data["highlight_target_groups"];
        for (int i = 0; i < arr.size(); ++i) {
            highlight_target_groups.append(godot::StringName(godot::String(arr[i])));
        }
    }

    if (p_data.has("brightness_adjustment")) set_brightness_adjustment(p_data["brightness_adjustment"]);
    if (p_data.has("contrast_adjustment")) set_contrast_adjustment(p_data["contrast_adjustment"]);
    if (p_data.has("saturation_adjustment")) set_saturation_adjustment(p_data["saturation_adjustment"]);
    if (p_data.has("disable_volumetric_fog_for_clarity")) disable_volumetric_fog_for_clarity = p_data["disable_volumetric_fog_for_clarity"];

    // 3. Motion & Photosensitivity
    if (p_data.has("reduce_flashing_lights")) reduce_flashing_lights = p_data["reduce_flashing_lights"];
    if (p_data.has("flash_intensity_cap")) set_flash_intensity_cap(p_data["flash_intensity_cap"]);
    if (p_data.has("screen_shake_scale")) set_screen_shake_scale(p_data["screen_shake_scale"]);
    if (p_data.has("motion_blur_scale")) set_motion_blur_scale(p_data["motion_blur_scale"]);
    if (p_data.has("reduce_camera_bob")) reduce_camera_bob = p_data["reduce_camera_bob"];
    if (p_data.has("reticle_scale")) set_reticle_scale(p_data["reticle_scale"]);
    if (p_data.has("fov_override")) set_fov_override(p_data["fov_override"]);

    // 4. Audio, Captions & Cymatics
    if (p_data.has("closed_captions_enabled")) closed_captions_enabled = p_data["closed_captions_enabled"];
    if (p_data.has("caption_text_scale")) set_caption_text_scale(p_data["caption_text_scale"]);
    if (p_data.has("caption_background_opacity")) set_caption_background_opacity(p_data["caption_background_opacity"]);
    if (p_data.has("caption_speaker_labels")) caption_speaker_labels = p_data["caption_speaker_labels"];
    if (p_data.has("mono_audio_downmix")) mono_audio_downmix = p_data["mono_audio_downmix"];
    if (p_data.has("visual_sound_indicators")) visual_sound_indicators = p_data["visual_sound_indicators"];
    if (p_data.has("cymatic_visual_gain")) set_cymatic_visual_gain(p_data["cymatic_visual_gain"]);

    // 5. Motor, Input & Cognitive Assists
    if (p_data.has("game_speed_scale")) set_game_speed_scale(p_data["game_speed_scale"]);
    if (p_data.has("enemy_time_scale")) set_enemy_time_scale(p_data["enemy_time_scale"]);
    if (p_data.has("enemy_attack_charge_multiplier")) set_enemy_attack_charge_multiplier(p_data["enemy_attack_charge_multiplier"]);
    if (p_data.has("player_damage_taken_multiplier")) set_player_damage_taken_multiplier(p_data["player_damage_taken_multiplier"]);
    if (p_data.has("interaction_timeout_multiplier")) set_interaction_timeout_multiplier(p_data["interaction_timeout_multiplier"]);
    if (p_data.has("aim_assist_mode")) aim_assist_mode = static_cast<AimAssistMode>(static_cast<int>(p_data["aim_assist_mode"]));
    if (p_data.has("aim_assist_strength")) set_aim_assist_strength(p_data["aim_assist_strength"]);
    if (p_data.has("aim_magnetism_radius")) set_aim_magnetism_radius(p_data["aim_magnetism_radius"]);
    if (p_data.has("hold_to_toggle_aim")) hold_to_toggle_aim = p_data["hold_to_toggle_aim"];
    if (p_data.has("hold_to_toggle_crouch")) hold_to_toggle_crouch = p_data["hold_to_toggle_crouch"];
    if (p_data.has("hold_to_toggle_sprint")) hold_to_toggle_sprint = p_data["hold_to_toggle_sprint"];
    if (p_data.has("auto_fire_on_hold")) auto_fire_on_hold = p_data["auto_fire_on_hold"];
    if (p_data.has("single_stick_camera_assist")) single_stick_camera_assist = p_data["single_stick_camera_assist"];
    if (p_data.has("enemy_group_tag")) enemy_group_tag = godot::StringName(godot::String(p_data["enemy_group_tag"]));
    if (p_data.has("enemy_weapon_group_tag")) enemy_weapon_group_tag = godot::StringName(godot::String(p_data["enemy_weapon_group_tag"]));
    if (p_data.has("player_group_tag")) player_group_tag = godot::StringName(godot::String(p_data["player_group_tag"]));

    emit_changed();
}

} // namespace ideam::godot_ext