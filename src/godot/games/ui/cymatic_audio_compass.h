#pragma once

#include <cstdint>
#include <vector>

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include "../../../controls/circular_gauge.h"
#include "../../../controls/heatmap_grid.h"
#include "../../gameplay/gameplay_accessibility_option.h"
#include "../../gameplay/gameplay_audio_option.h"

namespace ideam::godot_ext {

// Forward Declarations
class GameBoard;
class GameAgent;

/**
 * DOD NOTE: Extracting CymaticAudioCompass enums at namespace scope with explicit
 * int32_t backing allows acoustic HUD state to be stored in flat Structure of Arrays
 * (SoA) tables without pulling in the full Control definition.
 */
enum AudioCompassDisplayMode : int32_t {
    COMPASS_RADIAL_ARCS = 0,            // Circular directional energy sectors + AI HearingRange alert ring
    COMPASS_HEATMAP_OVERLAY = 1,        // 2D HeatmapGrid cymatic wave field with threat vectors
    COMPASS_COMBINED_RING_AND_GRID = 2  // 2D HeatmapGrid core framed by directional acoustic arcs
};

enum AcousticCueCategory : int32_t {
    ACOUSTIC_CUE_AMBIENT = 0,
    ACOUSTIC_CUE_ENEMY = 1,
    ACOUSTIC_CUE_HAZARD = 2,
    ACOUSTIC_CUE_INTERACTABLE = 3,
    ACOUSTIC_CUE_VOICE = 4
};

/**
 * DOD NOTE: Flat POD acoustic sector state to avoid heap-allocated event nodes.
 */
struct AcousticSectorPOD {
    float energy = 0.0f;
    float peak_hold = 0.0f;
    int32_t dominant_category = 0;
};

/**
 * DOD NOTE: CymaticAudioCompass bridges `GameplayAudioOption` (`acoustic_quality`,
 * `ai_hearing_sensitivity_multiplier`, `CymaticMap`) and `GameplayAccessibilityOption`
 * (`visual_sound_indicators`, `cymatic_visual_gain`, `enemy_highlight_color`,
 * `hazard_highlight_color`, `interactable_highlight_color`).
 *
 * It composes `CircularGauge` and `HeatmapGrid` to visualize directional acoustic
 * energy sectors, 2D cymatic field propagation, and nearby AI `HearingRange` alert
 * thresholds around the controlled `GameAgent` without holding raw entity pointers
 * across frames.
 */
class CymaticAudioCompass : public godot::Control {
    GDCLASS(CymaticAudioCompass, godot::Control)

protected:
    static void _bind_methods();
    void _notification(int p_what);

private:
    // --- Context & Option Resource Bindings (DOD-safe ObjectIDs + Refs) ---
    godot::ObjectID _controlled_agent_id;
    godot::ObjectID _active_board_id;
    godot::ObjectID _cymatic_map_id;

    godot::Ref<GameplayAudioOption> audio_option;
    godot::Ref<GameplayAccessibilityOption> accessibility_option;

    AudioCompassDisplayMode display_mode = AudioCompassDisplayMode::COMPASS_COMBINED_RING_AND_GRID;
    bool respect_accessibility_toggle = false; // When true, auto-hides unless visual_sound_indicators == true
    int sector_count = 16;
    int heatmap_columns = 12;
    float decay_rate = 2.2f;
    float max_detection_radius = 35.0f;
    float baseline_ai_hearing_range = 18.0f;
    float player_acoustic_footprint = 0.25f;
    float reference_yaw_degrees = 0.0f;

    // --- Composed Controls ---
    CircularGauge *_compass_ring = nullptr;
    HeatmapGrid *_heatmap_grid = nullptr;

    // --- Flat Acoustic Telemetry State ---
    std::vector<AcousticSectorPOD> _sectors;
    float _nearest_enemy_distance = -1.0f;
    float _effective_ai_hearing_range = 18.0f;
    float _ai_alert_ratio = 0.0f; // 0.0 = undetected, >= 1.0 = within AI HearingRange

    // --- Internal Helpers ---
    void _ensure_child_controls();
    void _sync_child_controls_layout();
    void _resize_sectors(int p_count);
    void _poll_board_and_cymatic_telemetry();
    godot::Color _resolve_category_color(int32_t p_category) const;
    float _get_effective_visual_gain() const;

    void _draw_directional_sectors(const godot::Vector2 &p_center, float p_inner_r, float p_outer_r);
    void _draw_hearing_threshold_ring(const godot::Vector2 &p_center, float p_radius);
    void _draw_cardinal_markers(const godot::Vector2 &p_center, float p_outer_r);
    void _draw_arc_segment(
        const godot::Vector2 &p_center,
        float p_inner_r,
        float p_outer_r,
        float p_start_rad,
        float p_end_rad,
        const godot::Color &p_color
    );

public:
    CymaticAudioCompass();
    ~CymaticAudioCompass() override;

    virtual void _ready() override;
    virtual void _process(double p_delta) override;
    virtual godot::Vector2 _get_minimum_size() const override;

    // --- Target & Resource Setters / Getters ---
    void set_controlled_agent(GameAgent *p_agent);
    GameAgent *get_controlled_agent() const;

    void set_active_board(GameBoard *p_board);
    GameBoard *get_active_board() const;

    void set_cymatic_map_node(godot::Node *p_map_node);
    godot::Node *get_cymatic_map_node() const;

    void set_audio_option(const godot::Ref<GameplayAudioOption> &p_opt);
    godot::Ref<GameplayAudioOption> get_audio_option() const;

    void set_accessibility_option(const godot::Ref<GameplayAccessibilityOption> &p_opt);
    godot::Ref<GameplayAccessibilityOption> get_accessibility_option() const;

    // --- Display & Acoustic Parameters Setters / Getters ---
    void set_display_mode(AudioCompassDisplayMode p_mode);
    AudioCompassDisplayMode get_display_mode() const;

    void set_respect_accessibility_toggle(bool p_respect);
    bool get_respect_accessibility_toggle() const;

    void set_sector_count(int p_count);
    int get_sector_count() const;

    void set_heatmap_columns(int p_columns);
    int get_heatmap_columns() const;

    void set_decay_rate(float p_rate);
    float get_decay_rate() const;

    void set_max_detection_radius(float p_radius);
    float get_max_detection_radius() const;

    void set_baseline_ai_hearing_range(float p_range);
    float get_baseline_ai_hearing_range() const;

    void set_player_acoustic_footprint(float p_footprint);
    float get_player_acoustic_footprint() const;

    void set_reference_yaw_degrees(float p_yaw);
    float get_reference_yaw_degrees() const;

    CircularGauge *get_compass_ring() const;
    HeatmapGrid *get_heatmap_grid() const;

    // --- Acoustic Event & DOD Snapshot Ingestion ---
    void push_directional_impulse(float p_angle_degrees, float p_energy, AcousticCueCategory p_category = ACOUSTIC_CUE_AMBIENT);
    void push_world_acoustic_event(const godot::Vector3 &p_source_pos, const godot::Vector3 &p_listener_pos, float p_listener_yaw_deg, float p_energy, AcousticCueCategory p_category = ACOUSTIC_CUE_AMBIENT);
    void set_sector_energies(const godot::PackedFloat32Array &p_energies, int p_category = 0);
    godot::PackedFloat32Array get_sector_energies() const;

    void set_cymatic_grid_bytes(const godot::PackedByteArray &p_bytes, int p_columns = 12);
    void clear_telemetry();

    float get_effective_ai_hearing_range() const;
    float get_ai_alert_ratio() const;
    bool is_ai_hearing_alert_active() const;
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::AudioCompassDisplayMode);
VARIANT_ENUM_CAST(ideam::godot_ext::AcousticCueCategory);