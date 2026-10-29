#pragma once

#include "gameplay_resource.h"

#include <godot_cpp/classes/audio_server.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace ideam::godot_ext {

/**
 * DOD NOTE: Extracting audio enums at namespace scope with explicit int32_t backing
 * allows them to be stored in flat Structure of Arrays (SoA) configuration columns
 * and evaluated without pulling in the full GameplayAudioOption class definition.
 */
enum AudioDynamicRangePreset : int32_t {
    DYNAMIC_RANGE_FULL = 0,              // Uncompressed cinema/studio dynamic range
    DYNAMIC_RANGE_STANDARD = 1,          // Balanced default dynamic range
    DYNAMIC_RANGE_MEDIUM_COMPRESSED = 2, // Moderate compression for TV/desk speakers
    DYNAMIC_RANGE_NIGHT_MODE = 3         // Heavy compression & limiting so quiet cues stay audible
};

enum AudioSpeakerMode : int32_t {
    SPEAKER_STEREO = 0,
    SPEAKER_HEADPHONES_BINAURAL = 1,
    SPEAKER_SURROUND_5_1 = 2,
    SPEAKER_SURROUND_7_1 = 3,
    SPEAKER_MONO = 4
};

enum AcousticQualityPreset : int32_t {
    ACOUSTIC_DISABLED = 0,
    ACOUSTIC_RAYCAST_BASIC = 1,
    ACOUSTIC_CYMATIC_WAVE_2D = 2,
    ACOUSTIC_CYMATIC_WAVE_3D = 3
};

/**
 * DOD NOTE: GameplayAudioOption manages standard and advanced audio settings across
 * Godot's `AudioServer` buses (`Master`, `Music`, `SFX`, `Voice`, `Ambient`, `UI`, plus
 * arbitrary custom buses), dynamic range compression, dialogue voice ducking, and
 * spatial/cymatic acoustic propagation parameters (`CymaticMap`).
 *
 * It also integrates with the `GameplayStyle` cascade via `collect_rules()`, emitting
 * `GameplayRule` modifiers that scale AI acoustic perception (`HearingRange`) on
 * `GameAgent` entities such as the TPS Demo's Red Robots.
 */
class GameplayAudioOption : public GameplayResource {
    GDCLASS(GameplayAudioOption, GameplayResource)

protected:
    static void _bind_methods();

private:
    // --- 1. Bus Volumes (Linear 0.0 .. 1.0) & Mute Toggles ---
    float master_volume = 1.0f;
    bool master_muted = false;

    float music_volume = 0.85f;
    bool music_muted = false;

    float sfx_volume = 1.0f;
    bool sfx_muted = false;

    float voice_volume = 1.0f;
    bool voice_muted = false;

    float ambient_volume = 0.85f;
    bool ambient_muted = false;

    float ui_volume = 1.0f;
    bool ui_muted = false;

    // Arbitrary project-specific bus overrides (StringName -> float linear volume / bool mute)
    godot::Dictionary custom_bus_volumes;
    godot::Dictionary custom_bus_mutes;

    // --- 2. Bus Name Mappings ---
    godot::StringName master_bus_name = godot::StringName("Master");
    godot::StringName music_bus_name = godot::StringName("Music");
    godot::StringName sfx_bus_name = godot::StringName("SFX");
    godot::StringName voice_bus_name = godot::StringName("Voice");
    godot::StringName ambient_bus_name = godot::StringName("Ambient");
    godot::StringName ui_bus_name = godot::StringName("UI");

    // --- 3. Output, Mixing & Dynamic Range ---
    AudioSpeakerMode speaker_mode = AudioSpeakerMode::SPEAKER_STEREO;
    AudioDynamicRangePreset dynamic_range_preset = AudioDynamicRangePreset::DYNAMIC_RANGE_STANDARD;
    bool mute_when_unfocused = false;
    bool playback_speed_scales_with_time = true;
    float playback_speed_scale = 1.0f;
    bool voice_ducking_enabled = true;
    float voice_ducking_attenuation_db = -6.0f;

    // --- 4. Spatial Acoustics & Cymatics (`CymaticMap` Integration) ---
    AcousticQualityPreset acoustic_quality = AcousticQualityPreset::ACOUSTIC_CYMATIC_WAVE_2D;
    bool acoustic_occlusion_enabled = true;
    bool dynamic_reverb_enabled = true;
    int cymatic_simulation_frequency_hz = 60;
    float ai_hearing_sensitivity_multiplier = 1.0f;
    godot::StringName enemy_group_tag = godot::StringName("RedRobot");

    // Internal helper to apply volume and mute state to a named AudioServer bus
    void _apply_bus_state(godot::AudioServer *p_audio_server, const godot::StringName &p_bus_name, float p_linear_volume, bool p_muted) const;
    void _apply_dynamic_range_compressor(godot::AudioServer *p_audio_server) const;

public:
    GameplayAudioOption();
    ~GameplayAudioOption() override;

    // --- Static dB / Linear Conversion Utilities ---
    static float linear_to_db_safe(float p_linear);
    static float db_to_linear_safe(float p_db);

    // --- 1. Bus Volumes & Mutes Setters / Getters ---
    void set_master_volume(float p_volume);
    float get_master_volume() const;
    void set_master_muted(bool p_muted);
    bool get_master_muted() const;

    void set_music_volume(float p_volume);
    float get_music_volume() const;
    void set_music_muted(bool p_muted);
    bool get_music_muted() const;

    void set_sfx_volume(float p_volume);
    float get_sfx_volume() const;
    void set_sfx_muted(bool p_muted);
    bool get_sfx_muted() const;

    void set_voice_volume(float p_volume);
    float get_voice_volume() const;
    void set_voice_muted(bool p_muted);
    bool get_voice_muted() const;

    void set_ambient_volume(float p_volume);
    float get_ambient_volume() const;
    void set_ambient_muted(bool p_muted);
    bool get_ambient_muted() const;

    void set_ui_volume(float p_volume);
    float get_ui_volume() const;
    void set_ui_muted(bool p_muted);
    bool get_ui_muted() const;

    void set_custom_bus_volumes(const godot::Dictionary &p_volumes);
    godot::Dictionary get_custom_bus_volumes() const;
    void set_custom_bus_mutes(const godot::Dictionary &p_mutes);
    godot::Dictionary get_custom_bus_mutes() const;

    void set_bus_linear_volume(const godot::StringName &p_bus_name, float p_linear);
    float get_bus_linear_volume(const godot::StringName &p_bus_name) const;
    void set_bus_muted(const godot::StringName &p_bus_name, bool p_muted);
    bool is_bus_muted(const godot::StringName &p_bus_name) const;

    // --- 2. Bus Name Mappings Setters / Getters ---
    void set_master_bus_name(const godot::StringName &p_name);
    godot::StringName get_master_bus_name() const;

    void set_music_bus_name(const godot::StringName &p_name);
    godot::StringName get_music_bus_name() const;

    void set_sfx_bus_name(const godot::StringName &p_name);
    godot::StringName get_sfx_bus_name() const;

    void set_voice_bus_name(const godot::StringName &p_name);
    godot::StringName get_voice_bus_name() const;

    void set_ambient_bus_name(const godot::StringName &p_name);
    godot::StringName get_ambient_bus_name() const;

    void set_ui_bus_name(const godot::StringName &p_name);
    godot::StringName get_ui_bus_name() const;

    // --- 3. Output, Mixing & Dynamic Range Setters / Getters ---
    void set_speaker_mode(AudioSpeakerMode p_mode);
    AudioSpeakerMode get_speaker_mode() const;

    void set_dynamic_range_preset(AudioDynamicRangePreset p_preset);
    AudioDynamicRangePreset get_dynamic_range_preset() const;

    void set_mute_when_unfocused(bool p_mute);
    bool get_mute_when_unfocused() const;

    void set_playback_speed_scales_with_time(bool p_scales);
    bool get_playback_speed_scales_with_time() const;

    void set_playback_speed_scale(float p_scale);
    float get_playback_speed_scale() const;

    void set_voice_ducking_enabled(bool p_enabled);
    bool get_voice_ducking_enabled() const;

    void set_voice_ducking_attenuation_db(float p_db);
    float get_voice_ducking_attenuation_db() const;

    // --- 4. Spatial Acoustics & Cymatics Setters / Getters ---
    void set_acoustic_quality(AcousticQualityPreset p_quality);
    AcousticQualityPreset get_acoustic_quality() const;

    void set_acoustic_occlusion_enabled(bool p_enabled);
    bool get_acoustic_occlusion_enabled() const;

    void set_dynamic_reverb_enabled(bool p_enabled);
    bool get_dynamic_reverb_enabled() const;

    void set_cymatic_simulation_frequency_hz(int p_hz);
    int get_cymatic_simulation_frequency_hz() const;

    void set_ai_hearing_sensitivity_multiplier(float p_multiplier);
    float get_ai_hearing_sensitivity_multiplier() const;

    void set_enemy_group_tag(const godot::StringName &p_tag);
    godot::StringName get_enemy_group_tag() const;

    // --- Application, Rule Synthesis & Persistence ---
    void apply_audio_option(const godot::Ref<GameplayAudioOption> &p_other);

    /**
     * Applies all configured bus volumes (converted to dB), mute states, playback speed
     * scale, and dynamic range compression directly to `godot::AudioServer`.
     */
    void apply_to_audio_server() const;

    virtual godot::TypedArray<GameplayRule> collect_rules() const override;
    virtual void apply_resource(const godot::Ref<GameplayResource> &p_other) override;
    virtual godot::Dictionary save_resource() const override;
    virtual void load_resource(const godot::Dictionary &p_data) override;
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::AudioDynamicRangePreset);
VARIANT_ENUM_CAST(ideam::godot_ext::AudioSpeakerMode);
VARIANT_ENUM_CAST(ideam::godot_ext::AcousticQualityPreset);