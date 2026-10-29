#include "gameplay_audio_option.h"

#include <cmath>
#include <godot_cpp/classes/audio_effect_compressor.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/array.hpp>

#include "gameplay_rule.h"

namespace ideam::godot_ext {

void GameplayAudioOption::_bind_methods() {
    // --- Enums ---
    BIND_ENUM_CONSTANT(AudioDynamicRangePreset::DYNAMIC_RANGE_FULL);
    BIND_ENUM_CONSTANT(AudioDynamicRangePreset::DYNAMIC_RANGE_STANDARD);
    BIND_ENUM_CONSTANT(AudioDynamicRangePreset::DYNAMIC_RANGE_MEDIUM_COMPRESSED);
    BIND_ENUM_CONSTANT(AudioDynamicRangePreset::DYNAMIC_RANGE_NIGHT_MODE);

    BIND_ENUM_CONSTANT(AudioSpeakerMode::SPEAKER_STEREO);
    BIND_ENUM_CONSTANT(AudioSpeakerMode::SPEAKER_HEADPHONES_BINAURAL);
    BIND_ENUM_CONSTANT(AudioSpeakerMode::SPEAKER_SURROUND_5_1);
    BIND_ENUM_CONSTANT(AudioSpeakerMode::SPEAKER_SURROUND_7_1);
    BIND_ENUM_CONSTANT(AudioSpeakerMode::SPEAKER_MONO);

    BIND_ENUM_CONSTANT(AcousticQualityPreset::ACOUSTIC_DISABLED);
    BIND_ENUM_CONSTANT(AcousticQualityPreset::ACOUSTIC_RAYCAST_BASIC);
    BIND_ENUM_CONSTANT(AcousticQualityPreset::ACOUSTIC_CYMATIC_WAVE_2D);
    BIND_ENUM_CONSTANT(AcousticQualityPreset::ACOUSTIC_CYMATIC_WAVE_3D);

    // --- 1. Bus Volumes & Mutes ---
    ADD_GROUP("Bus Volumes & Mutes", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_master_volume", "volume"), &GameplayAudioOption::set_master_volume);
    godot::ClassDB::bind_method(godot::D_METHOD("get_master_volume"), &GameplayAudioOption::get_master_volume);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "master_volume", godot::PROPERTY_HINT_RANGE, "0.0,1.0,0.01"),
        "set_master_volume",
        "get_master_volume"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_master_muted", "muted"), &GameplayAudioOption::set_master_muted);
    godot::ClassDB::bind_method(godot::D_METHOD("get_master_muted"), &GameplayAudioOption::get_master_muted);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "master_muted"), "set_master_muted", "get_master_muted");

    godot::ClassDB::bind_method(godot::D_METHOD("set_music_volume", "volume"), &GameplayAudioOption::set_music_volume);
    godot::ClassDB::bind_method(godot::D_METHOD("get_music_volume"), &GameplayAudioOption::get_music_volume);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "music_volume", godot::PROPERTY_HINT_RANGE, "0.0,1.0,0.01"),
        "set_music_volume",
        "get_music_volume"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_music_muted", "muted"), &GameplayAudioOption::set_music_muted);
    godot::ClassDB::bind_method(godot::D_METHOD("get_music_muted"), &GameplayAudioOption::get_music_muted);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "music_muted"), "set_music_muted", "get_music_muted");

    godot::ClassDB::bind_method(godot::D_METHOD("set_sfx_volume", "volume"), &GameplayAudioOption::set_sfx_volume);
    godot::ClassDB::bind_method(godot::D_METHOD("get_sfx_volume"), &GameplayAudioOption::get_sfx_volume);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "sfx_volume", godot::PROPERTY_HINT_RANGE, "0.0,1.0,0.01"),
        "set_sfx_volume",
        "get_sfx_volume"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_sfx_muted", "muted"), &GameplayAudioOption::set_sfx_muted);
    godot::ClassDB::bind_method(godot::D_METHOD("get_sfx_muted"), &GameplayAudioOption::get_sfx_muted);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "sfx_muted"), "set_sfx_muted", "get_sfx_muted");

    godot::ClassDB::bind_method(godot::D_METHOD("set_voice_volume", "volume"), &GameplayAudioOption::set_voice_volume);
    godot::ClassDB::bind_method(godot::D_METHOD("get_voice_volume"), &GameplayAudioOption::get_voice_volume);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "voice_volume", godot::PROPERTY_HINT_RANGE, "0.0,1.0,0.01"),
        "set_voice_volume",
        "get_voice_volume"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_voice_muted", "muted"), &GameplayAudioOption::set_voice_muted);
    godot::ClassDB::bind_method(godot::D_METHOD("get_voice_muted"), &GameplayAudioOption::get_voice_muted);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "voice_muted"), "set_voice_muted", "get_voice_muted");

    godot::ClassDB::bind_method(godot::D_METHOD("set_ambient_volume", "volume"), &GameplayAudioOption::set_ambient_volume);
    godot::ClassDB::bind_method(godot::D_METHOD("get_ambient_volume"), &GameplayAudioOption::get_ambient_volume);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "ambient_volume", godot::PROPERTY_HINT_RANGE, "0.0,1.0,0.01"),
        "set_ambient_volume",
        "get_ambient_volume"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_ambient_muted", "muted"), &GameplayAudioOption::set_ambient_muted);
    godot::ClassDB::bind_method(godot::D_METHOD("get_ambient_muted"), &GameplayAudioOption::get_ambient_muted);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "ambient_muted"), "set_ambient_muted", "get_ambient_muted");

    godot::ClassDB::bind_method(godot::D_METHOD("set_ui_volume", "volume"), &GameplayAudioOption::set_ui_volume);
    godot::ClassDB::bind_method(godot::D_METHOD("get_ui_volume"), &GameplayAudioOption::get_ui_volume);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "ui_volume", godot::PROPERTY_HINT_RANGE, "0.0,1.0,0.01"),
        "set_ui_volume",
        "get_ui_volume"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_ui_muted", "muted"), &GameplayAudioOption::set_ui_muted);
    godot::ClassDB::bind_method(godot::D_METHOD("get_ui_muted"), &GameplayAudioOption::get_ui_muted);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "ui_muted"), "set_ui_muted", "get_ui_muted");

    godot::ClassDB::bind_method(godot::D_METHOD("set_custom_bus_volumes", "volumes"), &GameplayAudioOption::set_custom_bus_volumes);
    godot::ClassDB::bind_method(godot::D_METHOD("get_custom_bus_volumes"), &GameplayAudioOption::get_custom_bus_volumes);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::DICTIONARY, "custom_bus_volumes"), "set_custom_bus_volumes", "get_custom_bus_volumes");

    godot::ClassDB::bind_method(godot::D_METHOD("set_custom_bus_mutes", "mutes"), &GameplayAudioOption::set_custom_bus_mutes);
    godot::ClassDB::bind_method(godot::D_METHOD("get_custom_bus_mutes"), &GameplayAudioOption::get_custom_bus_mutes);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::DICTIONARY, "custom_bus_mutes"), "set_custom_bus_mutes", "get_custom_bus_mutes");

    // --- 2. Bus Name Mappings ---
    ADD_GROUP("Bus Name Mappings", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_master_bus_name", "name"), &GameplayAudioOption::set_master_bus_name);
    godot::ClassDB::bind_method(godot::D_METHOD("get_master_bus_name"), &GameplayAudioOption::get_master_bus_name);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING_NAME, "master_bus_name"), "set_master_bus_name", "get_master_bus_name");

    godot::ClassDB::bind_method(godot::D_METHOD("set_music_bus_name", "name"), &GameplayAudioOption::set_music_bus_name);
    godot::ClassDB::bind_method(godot::D_METHOD("get_music_bus_name"), &GameplayAudioOption::get_music_bus_name);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING_NAME, "music_bus_name"), "set_music_bus_name", "get_music_bus_name");

    godot::ClassDB::bind_method(godot::D_METHOD("set_sfx_bus_name", "name"), &GameplayAudioOption::set_sfx_bus_name);
    godot::ClassDB::bind_method(godot::D_METHOD("get_sfx_bus_name"), &GameplayAudioOption::get_sfx_bus_name);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING_NAME, "sfx_bus_name"), "set_sfx_bus_name", "get_sfx_bus_name");

    godot::ClassDB::bind_method(godot::D_METHOD("set_voice_bus_name", "name"), &GameplayAudioOption::set_voice_bus_name);
    godot::ClassDB::bind_method(godot::D_METHOD("get_voice_bus_name"), &GameplayAudioOption::get_voice_bus_name);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING_NAME, "voice_bus_name"), "set_voice_bus_name", "get_voice_bus_name");

    godot::ClassDB::bind_method(godot::D_METHOD("set_ambient_bus_name", "name"), &GameplayAudioOption::set_ambient_bus_name);
    godot::ClassDB::bind_method(godot::D_METHOD("get_ambient_bus_name"), &GameplayAudioOption::get_ambient_bus_name);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING_NAME, "ambient_bus_name"), "set_ambient_bus_name", "get_ambient_bus_name");

    godot::ClassDB::bind_method(godot::D_METHOD("set_ui_bus_name", "name"), &GameplayAudioOption::set_ui_bus_name);
    godot::ClassDB::bind_method(godot::D_METHOD("get_ui_bus_name"), &GameplayAudioOption::get_ui_bus_name);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING_NAME, "ui_bus_name"), "set_ui_bus_name", "get_ui_bus_name");

    // --- 3. Output, Mixing & Dynamic Range ---
    ADD_GROUP("Output, Mixing & Dynamic Range", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_speaker_mode", "mode"), &GameplayAudioOption::set_speaker_mode);
    godot::ClassDB::bind_method(godot::D_METHOD("get_speaker_mode"), &GameplayAudioOption::get_speaker_mode);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::INT, "speaker_mode", godot::PROPERTY_HINT_ENUM, "Stereo,Headphones Binaural,Surround 5.1,Surround 7.1,Mono"),
        "set_speaker_mode",
        "get_speaker_mode"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_dynamic_range_preset", "preset"), &GameplayAudioOption::set_dynamic_range_preset);
    godot::ClassDB::bind_method(godot::D_METHOD("get_dynamic_range_preset"), &GameplayAudioOption::get_dynamic_range_preset);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::INT, "dynamic_range_preset", godot::PROPERTY_HINT_ENUM, "Full Range,Standard,Medium Compressed,Night Mode"),
        "set_dynamic_range_preset",
        "get_dynamic_range_preset"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_mute_when_unfocused", "mute"), &GameplayAudioOption::set_mute_when_unfocused);
    godot::ClassDB::bind_method(godot::D_METHOD("get_mute_when_unfocused"), &GameplayAudioOption::get_mute_when_unfocused);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "mute_when_unfocused"), "set_mute_when_unfocused", "get_mute_when_unfocused");

    godot::ClassDB::bind_method(godot::D_METHOD("set_playback_speed_scales_with_time", "scales"), &GameplayAudioOption::set_playback_speed_scales_with_time);
    godot::ClassDB::bind_method(godot::D_METHOD("get_playback_speed_scales_with_time"), &GameplayAudioOption::get_playback_speed_scales_with_time);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "playback_speed_scales_with_time"), "set_playback_speed_scales_with_time", "get_playback_speed_scales_with_time");

    godot::ClassDB::bind_method(godot::D_METHOD("set_playback_speed_scale", "scale"), &GameplayAudioOption::set_playback_speed_scale);
    godot::ClassDB::bind_method(godot::D_METHOD("get_playback_speed_scale"), &GameplayAudioOption::get_playback_speed_scale);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "playback_speed_scale", godot::PROPERTY_HINT_RANGE, "0.1,4.0,0.05"),
        "set_playback_speed_scale",
        "get_playback_speed_scale"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_voice_ducking_enabled", "enabled"), &GameplayAudioOption::set_voice_ducking_enabled);
    godot::ClassDB::bind_method(godot::D_METHOD("get_voice_ducking_enabled"), &GameplayAudioOption::get_voice_ducking_enabled);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "voice_ducking_enabled"), "set_voice_ducking_enabled", "get_voice_ducking_enabled");

    godot::ClassDB::bind_method(godot::D_METHOD("set_voice_ducking_attenuation_db", "db"), &GameplayAudioOption::set_voice_ducking_attenuation_db);
    godot::ClassDB::bind_method(godot::D_METHOD("get_voice_ducking_attenuation_db"), &GameplayAudioOption::get_voice_ducking_attenuation_db);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "voice_ducking_attenuation_db", godot::PROPERTY_HINT_RANGE, "-24.0,0.0,0.5"),
        "set_voice_ducking_attenuation_db",
        "get_voice_ducking_attenuation_db"
    );

    // --- 4. Spatial Acoustics & Cymatics ---
    ADD_GROUP("Spatial Acoustics & Cymatics", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_acoustic_quality", "quality"), &GameplayAudioOption::set_acoustic_quality);
    godot::ClassDB::bind_method(godot::D_METHOD("get_acoustic_quality"), &GameplayAudioOption::get_acoustic_quality);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::INT, "acoustic_quality", godot::PROPERTY_HINT_ENUM, "Disabled,Raycast Basic,Cymatic Wave 2D,Cymatic Wave 3D"),
        "set_acoustic_quality",
        "get_acoustic_quality"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_acoustic_occlusion_enabled", "enabled"), &GameplayAudioOption::set_acoustic_occlusion_enabled);
    godot::ClassDB::bind_method(godot::D_METHOD("get_acoustic_occlusion_enabled"), &GameplayAudioOption::get_acoustic_occlusion_enabled);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "acoustic_occlusion_enabled"), "set_acoustic_occlusion_enabled", "get_acoustic_occlusion_enabled");

    godot::ClassDB::bind_method(godot::D_METHOD("set_dynamic_reverb_enabled", "enabled"), &GameplayAudioOption::set_dynamic_reverb_enabled);
    godot::ClassDB::bind_method(godot::D_METHOD("get_dynamic_reverb_enabled"), &GameplayAudioOption::get_dynamic_reverb_enabled);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "dynamic_reverb_enabled"), "set_dynamic_reverb_enabled", "get_dynamic_reverb_enabled");

    godot::ClassDB::bind_method(godot::D_METHOD("set_cymatic_simulation_frequency_hz", "hz"), &GameplayAudioOption::set_cymatic_simulation_frequency_hz);
    godot::ClassDB::bind_method(godot::D_METHOD("get_cymatic_simulation_frequency_hz"), &GameplayAudioOption::get_cymatic_simulation_frequency_hz);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::INT, "cymatic_simulation_frequency_hz", godot::PROPERTY_HINT_RANGE, "15,240,15"),
        "set_cymatic_simulation_frequency_hz",
        "get_cymatic_simulation_frequency_hz"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_ai_hearing_sensitivity_multiplier", "multiplier"), &GameplayAudioOption::set_ai_hearing_sensitivity_multiplier);
    godot::ClassDB::bind_method(godot::D_METHOD("get_ai_hearing_sensitivity_multiplier"), &GameplayAudioOption::get_ai_hearing_sensitivity_multiplier);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "ai_hearing_sensitivity_multiplier", godot::PROPERTY_HINT_RANGE, "0.0,3.0,0.05"),
        "set_ai_hearing_sensitivity_multiplier",
        "get_ai_hearing_sensitivity_multiplier"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_enemy_group_tag", "tag"), &GameplayAudioOption::set_enemy_group_tag);
    godot::ClassDB::bind_method(godot::D_METHOD("get_enemy_group_tag"), &GameplayAudioOption::get_enemy_group_tag);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING_NAME, "enemy_group_tag"), "set_enemy_group_tag", "get_enemy_group_tag");

    // --- Methods ---
    godot::ClassDB::bind_static_method("GameplayAudioOption", godot::D_METHOD("linear_to_db_safe", "linear"), &GameplayAudioOption::linear_to_db_safe);
    godot::ClassDB::bind_static_method("GameplayAudioOption", godot::D_METHOD("db_to_linear_safe", "db"), &GameplayAudioOption::db_to_linear_safe);

    godot::ClassDB::bind_method(godot::D_METHOD("set_bus_linear_volume", "bus_name", "linear"), &GameplayAudioOption::set_bus_linear_volume);
    godot::ClassDB::bind_method(godot::D_METHOD("get_bus_linear_volume", "bus_name"), &GameplayAudioOption::get_bus_linear_volume);
    godot::ClassDB::bind_method(godot::D_METHOD("set_bus_muted", "bus_name", "muted"), &GameplayAudioOption::set_bus_muted);
    godot::ClassDB::bind_method(godot::D_METHOD("is_bus_muted", "bus_name"), &GameplayAudioOption::is_bus_muted);

    godot::ClassDB::bind_method(godot::D_METHOD("apply_audio_option", "other"), &GameplayAudioOption::apply_audio_option);
    godot::ClassDB::bind_method(godot::D_METHOD("apply_to_audio_server"), &GameplayAudioOption::apply_to_audio_server);
}

GameplayAudioOption::GameplayAudioOption() {
    set_category(godot::StringName("Audio"));
    set_config_section("audio");
}

GameplayAudioOption::~GameplayAudioOption() {
}

// --- Static dB / Linear Conversion Utilities ---

float GameplayAudioOption::linear_to_db_safe(float p_linear) {
    if (p_linear <= 0.0001f) {
        return -80.0f;
    }
    return godot::Math::clamp(static_cast<float>(godot::Math::linear_to_db(p_linear)), -80.0f, 6.0f);
}

float GameplayAudioOption::db_to_linear_safe(float p_db) {
    if (p_db <= -79.9f) {
        return 0.0f;
    }
    return godot::Math::clamp(static_cast<float>(godot::Math::db_to_linear(p_db)), 0.0f, 2.0f);
}

// --- 1. Bus Volumes & Mutes Setters / Getters ---

void GameplayAudioOption::set_master_volume(float p_volume) {
    float clamped = godot::Math::clamp(p_volume, 0.0f, 1.0f);
    if (godot::Math::is_equal_approx(master_volume, clamped)) return;
    master_volume = clamped;
    emit_changed();
}
float GameplayAudioOption::get_master_volume() const { return master_volume; }

void GameplayAudioOption::set_master_muted(bool p_muted) {
    if (master_muted == p_muted) return;
    master_muted = p_muted;
    emit_changed();
}
bool GameplayAudioOption::get_master_muted() const { return master_muted; }

void GameplayAudioOption::set_music_volume(float p_volume) {
    float clamped = godot::Math::clamp(p_volume, 0.0f, 1.0f);
    if (godot::Math::is_equal_approx(music_volume, clamped)) return;
    music_volume = clamped;
    emit_changed();
}
float GameplayAudioOption::get_music_volume() const { return music_volume; }

void GameplayAudioOption::set_music_muted(bool p_muted) {
    if (music_muted == p_muted) return;
    music_muted = p_muted;
    emit_changed();
}
bool GameplayAudioOption::get_music_muted() const { return music_muted; }

void GameplayAudioOption::set_sfx_volume(float p_volume) {
    float clamped = godot::Math::clamp(p_volume, 0.0f, 1.0f);
    if (godot::Math::is_equal_approx(sfx_volume, clamped)) return;
    sfx_volume = clamped;
    emit_changed();
}
float GameplayAudioOption::get_sfx_volume() const { return sfx_volume; }

void GameplayAudioOption::set_sfx_muted(bool p_muted) {
    if (sfx_muted == p_muted) return;
    sfx_muted = p_muted;
    emit_changed();
}
bool GameplayAudioOption::get_sfx_muted() const { return sfx_muted; }

void GameplayAudioOption::set_voice_volume(float p_volume) {
    float clamped = godot::Math::clamp(p_volume, 0.0f, 1.0f);
    if (godot::Math::is_equal_approx(voice_volume, clamped)) return;
    voice_volume = clamped;
    emit_changed();
}
float GameplayAudioOption::get_voice_volume() const { return voice_volume; }

void GameplayAudioOption::set_voice_muted(bool p_muted) {
    if (voice_muted == p_muted) return;
    voice_muted = p_muted;
    emit_changed();
}
bool GameplayAudioOption::get_voice_muted() const { return voice_muted; }

void GameplayAudioOption::set_ambient_volume(float p_volume) {
    float clamped = godot::Math::clamp(p_volume, 0.0f, 1.0f);
    if (godot::Math::is_equal_approx(ambient_volume, clamped)) return;
    ambient_volume = clamped;
    emit_changed();
}
float GameplayAudioOption::get_ambient_volume() const { return ambient_volume; }

void GameplayAudioOption::set_ambient_muted(bool p_muted) {
    if (ambient_muted == p_muted) return;
    ambient_muted = p_muted;
    emit_changed();
}
bool GameplayAudioOption::get_ambient_muted() const { return ambient_muted; }

void GameplayAudioOption::set_ui_volume(float p_volume) {
    float clamped = godot::Math::clamp(p_volume, 0.0f, 1.0f);
    if (godot::Math::is_equal_approx(ui_volume, clamped)) return;
    ui_volume = clamped;
    emit_changed();
}
float GameplayAudioOption::get_ui_volume() const { return ui_volume; }

void GameplayAudioOption::set_ui_muted(bool p_muted) {
    if (ui_muted == p_muted) return;
    ui_muted = p_muted;
    emit_changed();
}
bool GameplayAudioOption::get_ui_muted() const { return ui_muted; }

void GameplayAudioOption::set_custom_bus_volumes(const godot::Dictionary &p_volumes) {
    if (custom_bus_volumes == p_volumes) return;
    custom_bus_volumes = p_volumes;
    emit_changed();
}
godot::Dictionary GameplayAudioOption::get_custom_bus_volumes() const { return custom_bus_volumes; }

void GameplayAudioOption::set_custom_bus_mutes(const godot::Dictionary &p_mutes) {
    if (custom_bus_mutes == p_mutes) return;
    custom_bus_mutes = p_mutes;
    emit_changed();
}
godot::Dictionary GameplayAudioOption::get_custom_bus_mutes() const { return custom_bus_mutes; }

void GameplayAudioOption::set_bus_linear_volume(const godot::StringName &p_bus_name, float p_linear) {
    if (p_bus_name == master_bus_name) { set_master_volume(p_linear); return; }
    if (p_bus_name == music_bus_name) { set_music_volume(p_linear); return; }
    if (p_bus_name == sfx_bus_name) { set_sfx_volume(p_linear); return; }
    if (p_bus_name == voice_bus_name) { set_voice_volume(p_linear); return; }
    if (p_bus_name == ambient_bus_name) { set_ambient_volume(p_linear); return; }
    if (p_bus_name == ui_bus_name) { set_ui_volume(p_linear); return; }

    float clamped = godot::Math::clamp(p_linear, 0.0f, 1.0f);
    custom_bus_volumes[godot::String(p_bus_name)] = clamped;
    emit_changed();
}

float GameplayAudioOption::get_bus_linear_volume(const godot::StringName &p_bus_name) const {
    if (p_bus_name == master_bus_name) return master_volume;
    if (p_bus_name == music_bus_name) return music_volume;
    if (p_bus_name == sfx_bus_name) return sfx_volume;
    if (p_bus_name == voice_bus_name) return voice_volume;
    if (p_bus_name == ambient_bus_name) return ambient_volume;
    if (p_bus_name == ui_bus_name) return ui_volume;

    godot::String key(p_bus_name);
    if (custom_bus_volumes.has(key)) {
        return static_cast<float>(custom_bus_volumes[key]);
    }
    return 1.0f;
}

void GameplayAudioOption::set_bus_muted(const godot::StringName &p_bus_name, bool p_muted) {
    if (p_bus_name == master_bus_name) { set_master_muted(p_muted); return; }
    if (p_bus_name == music_bus_name) { set_music_muted(p_muted); return; }
    if (p_bus_name == sfx_bus_name) { set_sfx_muted(p_muted); return; }
    if (p_bus_name == voice_bus_name) { set_voice_muted(p_muted); return; }
    if (p_bus_name == ambient_bus_name) { set_ambient_muted(p_muted); return; }
    if (p_bus_name == ui_bus_name) { set_ui_muted(p_muted); return; }

    custom_bus_mutes[godot::String(p_bus_name)] = p_muted;
    emit_changed();
}

bool GameplayAudioOption::is_bus_muted(const godot::StringName &p_bus_name) const {
    if (p_bus_name == master_bus_name) return master_muted;
    if (p_bus_name == music_bus_name) return music_muted;
    if (p_bus_name == sfx_bus_name) return sfx_muted;
    if (p_bus_name == voice_bus_name) return voice_muted;
    if (p_bus_name == ambient_bus_name) return ambient_muted;
    if (p_bus_name == ui_bus_name) return ui_muted;

    godot::String key(p_bus_name);
    if (custom_bus_mutes.has(key)) {
        return static_cast<bool>(custom_bus_mutes[key]);
    }
    return false;
}

// --- 2. Bus Name Mappings Setters / Getters ---

void GameplayAudioOption::set_master_bus_name(const godot::StringName &p_name) {
    if (master_bus_name == p_name) return;
    master_bus_name = p_name;
    emit_changed();
}
godot::StringName GameplayAudioOption::get_master_bus_name() const { return master_bus_name; }

void GameplayAudioOption::set_music_bus_name(const godot::StringName &p_name) {
    if (music_bus_name == p_name) return;
    music_bus_name = p_name;
    emit_changed();
}
godot::StringName GameplayAudioOption::get_music_bus_name() const { return music_bus_name; }

void GameplayAudioOption::set_sfx_bus_name(const godot::StringName &p_name) {
    if (sfx_bus_name == p_name) return;
    sfx_bus_name = p_name;
    emit_changed();
}
godot::StringName GameplayAudioOption::get_sfx_bus_name() const { return sfx_bus_name; }

void GameplayAudioOption::set_voice_bus_name(const godot::StringName &p_name) {
    if (voice_bus_name == p_name) return;
    voice_bus_name = p_name;
    emit_changed();
}
godot::StringName GameplayAudioOption::get_voice_bus_name() const { return voice_bus_name; }

void GameplayAudioOption::set_ambient_bus_name(const godot::StringName &p_name) {
    if (ambient_bus_name == p_name) return;
    ambient_bus_name = p_name;
    emit_changed();
}
godot::StringName GameplayAudioOption::get_ambient_bus_name() const { return ambient_bus_name; }

void GameplayAudioOption::set_ui_bus_name(const godot::StringName &p_name) {
    if (ui_bus_name == p_name) return;
    ui_bus_name = p_name;
    emit_changed();
}
godot::StringName GameplayAudioOption::get_ui_bus_name() const { return ui_bus_name; }

// --- 3. Output, Mixing & Dynamic Range Setters / Getters ---

void GameplayAudioOption::set_speaker_mode(AudioSpeakerMode p_mode) {
    if (speaker_mode == p_mode) return;
    speaker_mode = p_mode;
    emit_changed();
}
AudioSpeakerMode GameplayAudioOption::get_speaker_mode() const { return speaker_mode; }

void GameplayAudioOption::set_dynamic_range_preset(AudioDynamicRangePreset p_preset) {
    if (dynamic_range_preset == p_preset) return;
    dynamic_range_preset = p_preset;
    emit_changed();
}
AudioDynamicRangePreset GameplayAudioOption::get_dynamic_range_preset() const { return dynamic_range_preset; }

void GameplayAudioOption::set_mute_when_unfocused(bool p_mute) {
    if (mute_when_unfocused == p_mute) return;
    mute_when_unfocused = p_mute;
    emit_changed();
}
bool GameplayAudioOption::get_mute_when_unfocused() const { return mute_when_unfocused; }

void GameplayAudioOption::set_playback_speed_scales_with_time(bool p_scales) {
    if (playback_speed_scales_with_time == p_scales) return;
    playback_speed_scales_with_time = p_scales;
    emit_changed();
}
bool GameplayAudioOption::get_playback_speed_scales_with_time() const { return playback_speed_scales_with_time; }

void GameplayAudioOption::set_playback_speed_scale(float p_scale) {
    float clamped = godot::Math::clamp(p_scale, 0.1f, 4.0f);
    if (godot::Math::is_equal_approx(playback_speed_scale, clamped)) return;
    playback_speed_scale = clamped;
    emit_changed();
}
float GameplayAudioOption::get_playback_speed_scale() const { return playback_speed_scale; }

void GameplayAudioOption::set_voice_ducking_enabled(bool p_enabled) {
    if (voice_ducking_enabled == p_enabled) return;
    voice_ducking_enabled = p_enabled;
    emit_changed();
}
bool GameplayAudioOption::get_voice_ducking_enabled() const { return voice_ducking_enabled; }

void GameplayAudioOption::set_voice_ducking_attenuation_db(float p_db) {
    float clamped = godot::Math::clamp(p_db, -24.0f, 0.0f);
    if (godot::Math::is_equal_approx(voice_ducking_attenuation_db, clamped)) return;
    voice_ducking_attenuation_db = clamped;
    emit_changed();
}
float GameplayAudioOption::get_voice_ducking_attenuation_db() const { return voice_ducking_attenuation_db; }

// --- 4. Spatial Acoustics & Cymatics Setters / Getters ---

void GameplayAudioOption::set_acoustic_quality(AcousticQualityPreset p_quality) {
    if (acoustic_quality == p_quality) return;
    acoustic_quality = p_quality;
    emit_changed();
}
AcousticQualityPreset GameplayAudioOption::get_acoustic_quality() const { return acoustic_quality; }

void GameplayAudioOption::set_acoustic_occlusion_enabled(bool p_enabled) {
    if (acoustic_occlusion_enabled == p_enabled) return;
    acoustic_occlusion_enabled = p_enabled;
    emit_changed();
}
bool GameplayAudioOption::get_acoustic_occlusion_enabled() const { return acoustic_occlusion_enabled; }

void GameplayAudioOption::set_dynamic_reverb_enabled(bool p_enabled) {
    if (dynamic_reverb_enabled == p_enabled) return;
    dynamic_reverb_enabled = p_enabled;
    emit_changed();
}
bool GameplayAudioOption::get_dynamic_reverb_enabled() const { return dynamic_reverb_enabled; }

void GameplayAudioOption::set_cymatic_simulation_frequency_hz(int p_hz) {
    int clamped = godot::Math::clamp(p_hz, 15, 240);
    if (cymatic_simulation_frequency_hz == clamped) return;
    cymatic_simulation_frequency_hz = clamped;
    emit_changed();
}
int GameplayAudioOption::get_cymatic_simulation_frequency_hz() const { return cymatic_simulation_frequency_hz; }

void GameplayAudioOption::set_ai_hearing_sensitivity_multiplier(float p_multiplier) {
    float clamped = godot::Math::clamp(p_multiplier, 0.0f, 3.0f);
    if (godot::Math::is_equal_approx(ai_hearing_sensitivity_multiplier, clamped)) return;
    ai_hearing_sensitivity_multiplier = clamped;
    emit_changed();
}
float GameplayAudioOption::get_ai_hearing_sensitivity_multiplier() const { return ai_hearing_sensitivity_multiplier; }

void GameplayAudioOption::set_enemy_group_tag(const godot::StringName &p_tag) {
    if (enemy_group_tag == p_tag) return;
    enemy_group_tag = p_tag;
    emit_changed();
}
godot::StringName GameplayAudioOption::get_enemy_group_tag() const { return enemy_group_tag; }

// --- Internal AudioServer Helpers ---

void GameplayAudioOption::_apply_bus_state(godot::AudioServer *p_audio_server, const godot::StringName &p_bus_name, float p_linear_volume, bool p_muted) const {
    if (!p_audio_server || p_bus_name.is_empty()) {
        return;
    }

    int bus_idx = p_audio_server->get_bus_index(p_bus_name);
    if (bus_idx < 0) {
        return;
    }

    p_audio_server->set_bus_volume_db(bus_idx, linear_to_db_safe(p_linear_volume));
    p_audio_server->set_bus_mute(bus_idx, p_muted);
}

void GameplayAudioOption::_apply_dynamic_range_compressor(godot::AudioServer *p_audio_server) const {
    if (!p_audio_server) {
        return;
    }

    int master_idx = p_audio_server->get_bus_index(master_bus_name);
    if (master_idx < 0) {
        master_idx = 0;
    }
    if (master_idx >= p_audio_server->get_bus_count()) {
        return;
    }

    // Locate an existing AudioEffectCompressor on the Master bus if present
    int effect_count = p_audio_server->get_bus_effect_count(master_idx);
    godot::Ref<godot::AudioEffectCompressor> compressor;
    int compressor_idx = -1;

    for (int i = 0; i < effect_count; ++i) {
        godot::Ref<godot::AudioEffect> eff = p_audio_server->get_bus_effect(master_idx, i);
        godot::Ref<godot::AudioEffectCompressor> comp = eff;
        if (comp.is_valid()) {
            compressor = comp;
            compressor_idx = i;
            break;
        }
    }

    if (dynamic_range_preset == AudioDynamicRangePreset::DYNAMIC_RANGE_FULL) {
        if (compressor_idx >= 0) {
            p_audio_server->set_bus_effect_enabled(master_idx, compressor_idx, false);
        }
        return;
    }

    // Create a master compressor effect if none exists yet and compression is requested
    if (compressor.is_null()) {
        compressor.instantiate();
        p_audio_server->add_bus_effect(master_idx, compressor);
        compressor_idx = p_audio_server->get_bus_effect_count(master_idx) - 1;
    }

    p_audio_server->set_bus_effect_enabled(master_idx, compressor_idx, true);

    switch (dynamic_range_preset) {
        case AudioDynamicRangePreset::DYNAMIC_RANGE_STANDARD:
            compressor->set_threshold(-6.0f);
            compressor->set_ratio(2.0f);
            compressor->set_gain(0.0f);
            break;
        case AudioDynamicRangePreset::DYNAMIC_RANGE_MEDIUM_COMPRESSED:
            compressor->set_threshold(-14.0f);
            compressor->set_ratio(4.0f);
            compressor->set_gain(2.5f);
            break;
        case AudioDynamicRangePreset::DYNAMIC_RANGE_NIGHT_MODE:
            compressor->set_threshold(-24.0f);
            compressor->set_ratio(8.0f);
            compressor->set_gain(5.0f);
            break;
        default:
            break;
    }
}

// --- Application, Rule Synthesis & Persistence ---

void GameplayAudioOption::apply_audio_option(const godot::Ref<GameplayAudioOption> &p_other) {
    if (!p_other.is_valid()) {
        return;
    }

    GameplayResource::apply_resource(p_other);

    master_volume = p_other->get_master_volume();
    master_muted = p_other->get_master_muted();
    music_volume = p_other->get_music_volume();
    music_muted = p_other->get_music_muted();
    sfx_volume = p_other->get_sfx_volume();
    sfx_muted = p_other->get_sfx_muted();
    voice_volume = p_other->get_voice_volume();
    voice_muted = p_other->get_voice_muted();
    ambient_volume = p_other->get_ambient_volume();
    ambient_muted = p_other->get_ambient_muted();
    ui_volume = p_other->get_ui_volume();
    ui_muted = p_other->get_ui_muted();

    custom_bus_volumes = p_other->get_custom_bus_volumes().duplicate();
    custom_bus_mutes = p_other->get_custom_bus_mutes().duplicate();

    master_bus_name = p_other->get_master_bus_name();
    music_bus_name = p_other->get_music_bus_name();
    sfx_bus_name = p_other->get_sfx_bus_name();
    voice_bus_name = p_other->get_voice_bus_name();
    ambient_bus_name = p_other->get_ambient_bus_name();
    ui_bus_name = p_other->get_ui_bus_name();

    speaker_mode = p_other->get_speaker_mode();
    dynamic_range_preset = p_other->get_dynamic_range_preset();
    mute_when_unfocused = p_other->get_mute_when_unfocused();
    playback_speed_scales_with_time = p_other->get_playback_speed_scales_with_time();
    playback_speed_scale = p_other->get_playback_speed_scale();
    voice_ducking_enabled = p_other->get_voice_ducking_enabled();
    voice_ducking_attenuation_db = p_other->get_voice_ducking_attenuation_db();

    acoustic_quality = p_other->get_acoustic_quality();
    acoustic_occlusion_enabled = p_other->get_acoustic_occlusion_enabled();
    dynamic_reverb_enabled = p_other->get_dynamic_reverb_enabled();
    cymatic_simulation_frequency_hz = p_other->get_cymatic_simulation_frequency_hz();
    ai_hearing_sensitivity_multiplier = p_other->get_ai_hearing_sensitivity_multiplier();
    enemy_group_tag = p_other->get_enemy_group_tag();

    emit_changed();
}

void GameplayAudioOption::apply_resource(const godot::Ref<GameplayResource> &p_other) {
    godot::Ref<GameplayAudioOption> aud_other = p_other;
    if (aud_other.is_valid()) {
        apply_audio_option(aud_other);
    } else {
        GameplayResource::apply_resource(p_other);
    }
}

void GameplayAudioOption::apply_to_audio_server() const {
    if (!get_enabled()) {
        return;
    }

    godot::AudioServer *as = godot::AudioServer::get_singleton();
    if (!as) {
        return;
    }

    // 1. Apply Standard Named Buses
    _apply_bus_state(as, master_bus_name, master_volume, master_muted);
    _apply_bus_state(as, music_bus_name, music_volume, music_muted);
    _apply_bus_state(as, sfx_bus_name, sfx_volume, sfx_muted);
    _apply_bus_state(as, voice_bus_name, voice_volume, voice_muted);
    _apply_bus_state(as, ambient_bus_name, ambient_volume, ambient_muted);
    _apply_bus_state(as, ui_bus_name, ui_volume, ui_muted);

    // 2. Apply Custom Bus Volumes & Mutes
    godot::Array custom_keys = custom_bus_volumes.keys();
    for (int i = 0; i < custom_keys.size(); ++i) {
        godot::String bus_str = custom_keys[i];
        godot::StringName bus_name(bus_str);
        float vol = static_cast<float>(custom_bus_volumes[bus_str]);
        bool muted = custom_bus_mutes.has(bus_str) ? static_cast<bool>(custom_bus_mutes[bus_str]) : false;
        _apply_bus_state(as, bus_name, vol, muted);
    }

    // 3. Apply Global Playback Speed & Dynamic Range Preset
    as->set_playback_speed_scale(playback_speed_scale);
    _apply_dynamic_range_compressor(as);
}

godot::TypedArray<GameplayRule> GameplayAudioOption::collect_rules() const {
    godot::TypedArray<GameplayRule> collected = GameplayResource::collect_rules();
    if (!get_enabled()) {
        return collected;
    }

    // Synthesize a cascading rule for AI acoustic perception / HearingRange on enemy GameAgents
    if (!godot::Math::is_equal_approx(ai_hearing_sensitivity_multiplier, 1.0f) && !enemy_group_tag.is_empty()) {
        godot::Ref<GameplayRule> hearing_rule;
        hearing_rule.instantiate();
        hearing_rule->set_title("Audio_AIHearingSensitivity");
        hearing_rule->set_scope(RuleScope::DESCENDANTS);
        hearing_rule->set_target_class(godot::StringName("GameAgent"));
        hearing_rule->set_target_title_or_group(enemy_group_tag);
        hearing_rule->set_target_property(godot::StringName("HearingRange"));
        hearing_rule->set_operation(RuleOp::MULTIPLY);
        hearing_rule->set_value(ai_hearing_sensitivity_multiplier);
        hearing_rule->set_priority(50);
        collected.append(hearing_rule);
    }

    return collected;
}

godot::Dictionary GameplayAudioOption::save_resource() const {
    godot::Dictionary data = GameplayResource::save_resource();

    data["master_volume"] = master_volume;
    data["master_muted"] = master_muted;
    data["music_volume"] = music_volume;
    data["music_muted"] = music_muted;
    data["sfx_volume"] = sfx_volume;
    data["sfx_muted"] = sfx_muted;
    data["voice_volume"] = voice_volume;
    data["voice_muted"] = voice_muted;
    data["ambient_volume"] = ambient_volume;
    data["ambient_muted"] = ambient_muted;
    data["ui_volume"] = ui_volume;
    data["ui_muted"] = ui_muted;

    data["custom_bus_volumes"] = custom_bus_volumes.duplicate();
    data["custom_bus_mutes"] = custom_bus_mutes.duplicate();

    data["master_bus_name"] = godot::String(master_bus_name);
    data["music_bus_name"] = godot::String(music_bus_name);
    data["sfx_bus_name"] = godot::String(sfx_bus_name);
    data["voice_bus_name"] = godot::String(voice_bus_name);
    data["ambient_bus_name"] = godot::String(ambient_bus_name);
    data["ui_bus_name"] = godot::String(ui_bus_name);

    data["speaker_mode"] = static_cast<int>(speaker_mode);
    data["dynamic_range_preset"] = static_cast<int>(dynamic_range_preset);
    data["mute_when_unfocused"] = mute_when_unfocused;
    data["playback_speed_scales_with_time"] = playback_speed_scales_with_time;
    data["playback_speed_scale"] = playback_speed_scale;
    data["voice_ducking_enabled"] = voice_ducking_enabled;
    data["voice_ducking_attenuation_db"] = voice_ducking_attenuation_db;

    data["acoustic_quality"] = static_cast<int>(acoustic_quality);
    data["acoustic_occlusion_enabled"] = acoustic_occlusion_enabled;
    data["dynamic_reverb_enabled"] = dynamic_reverb_enabled;
    data["cymatic_simulation_frequency_hz"] = cymatic_simulation_frequency_hz;
    data["ai_hearing_sensitivity_multiplier"] = ai_hearing_sensitivity_multiplier;
    data["enemy_group_tag"] = godot::String(enemy_group_tag);

    return data;
}

void GameplayAudioOption::load_resource(const godot::Dictionary &p_data) {
    GameplayResource::load_resource(p_data);

    if (p_data.has("master_volume")) set_master_volume(p_data["master_volume"]);
    if (p_data.has("master_muted")) master_muted = p_data["master_muted"];
    if (p_data.has("music_volume")) set_music_volume(p_data["music_volume"]);
    if (p_data.has("music_muted")) music_muted = p_data["music_muted"];
    if (p_data.has("sfx_volume")) set_sfx_volume(p_data["sfx_volume"]);
    if (p_data.has("sfx_muted")) sfx_muted = p_data["sfx_muted"];
    if (p_data.has("voice_volume")) set_voice_volume(p_data["voice_volume"]);
    if (p_data.has("voice_muted")) voice_muted = p_data["voice_muted"];
    if (p_data.has("ambient_volume")) set_ambient_volume(p_data["ambient_volume"]);
    if (p_data.has("ambient_muted")) ambient_muted = p_data["ambient_muted"];
    if (p_data.has("ui_volume")) set_ui_volume(p_data["ui_volume"]);
    if (p_data.has("ui_muted")) ui_muted = p_data["ui_muted"];

    if (p_data.has("custom_bus_volumes") && p_data["custom_bus_volumes"].get_type() == godot::Variant::DICTIONARY) {
        custom_bus_volumes = godot::Dictionary(p_data["custom_bus_volumes"]).duplicate();
    }
    if (p_data.has("custom_bus_mutes") && p_data["custom_bus_mutes"].get_type() == godot::Variant::DICTIONARY) {
        custom_bus_mutes = godot::Dictionary(p_data["custom_bus_mutes"]).duplicate();
    }

    if (p_data.has("master_bus_name")) master_bus_name = godot::StringName(godot::String(p_data["master_bus_name"]));
    if (p_data.has("music_bus_name")) music_bus_name = godot::StringName(godot::String(p_data["music_bus_name"]));
    if (p_data.has("sfx_bus_name")) sfx_bus_name = godot::StringName(godot::String(p_data["sfx_bus_name"]));
    if (p_data.has("voice_bus_name")) voice_bus_name = godot::StringName(godot::String(p_data["voice_bus_name"]));
    if (p_data.has("ambient_bus_name")) ambient_bus_name = godot::StringName(godot::String(p_data["ambient_bus_name"]));
    if (p_data.has("ui_bus_name")) ui_bus_name = godot::StringName(godot::String(p_data["ui_bus_name"]));

    if (p_data.has("speaker_mode")) speaker_mode = static_cast<AudioSpeakerMode>(static_cast<int>(p_data["speaker_mode"]));
    if (p_data.has("dynamic_range_preset")) dynamic_range_preset = static_cast<AudioDynamicRangePreset>(static_cast<int>(p_data["dynamic_range_preset"]));
    if (p_data.has("mute_when_unfocused")) mute_when_unfocused = p_data["mute_when_unfocused"];
    if (p_data.has("playback_speed_scales_with_time")) playback_speed_scales_with_time = p_data["playback_speed_scales_with_time"];
    if (p_data.has("playback_speed_scale")) set_playback_speed_scale(p_data["playback_speed_scale"]);
    if (p_data.has("voice_ducking_enabled")) voice_ducking_enabled = p_data["voice_ducking_enabled"];
    if (p_data.has("voice_ducking_attenuation_db")) set_voice_ducking_attenuation_db(p_data["voice_ducking_attenuation_db"]);

    if (p_data.has("acoustic_quality")) acoustic_quality = static_cast<AcousticQualityPreset>(static_cast<int>(p_data["acoustic_quality"]));
    if (p_data.has("acoustic_occlusion_enabled")) acoustic_occlusion_enabled = p_data["acoustic_occlusion_enabled"];
    if (p_data.has("dynamic_reverb_enabled")) dynamic_reverb_enabled = p_data["dynamic_reverb_enabled"];
    if (p_data.has("cymatic_simulation_frequency_hz")) set_cymatic_simulation_frequency_hz(p_data["cymatic_simulation_frequency_hz"]);
    if (p_data.has("ai_hearing_sensitivity_multiplier")) set_ai_hearing_sensitivity_multiplier(p_data["ai_hearing_sensitivity_multiplier"]);
    if (p_data.has("enemy_group_tag")) enemy_group_tag = godot::StringName(godot::String(p_data["enemy_group_tag"]));

    emit_changed();
}

} // namespace ideam::godot_ext