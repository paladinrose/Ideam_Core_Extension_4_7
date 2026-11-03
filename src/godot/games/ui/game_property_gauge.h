#pragma once

#include <cstdint>
#include <vector>

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include "../../controls/circular_gauge.h"
#include "../game_entity.h"
#include "../game_entities/game_piece.h"
#include "../game_property.h"
#include "../game_property_modifier.h"
#include "../gameplay/gameplay_rule.h"

namespace ideam::godot_ext {

/**
 * DOD NOTE: Extracting PropertyGaugeDisplayMode at namespace scope with explicit
 * int32_t backing allows HUD layout managers to store widget modes in flat POD tables.
 */
enum PropertyGaugeDisplayMode : int32_t {
    GAUGE_DISPLAY_RADIAL = 0,         // CircularGauge concentric rings + buff/debuff arc & center readout
    GAUGE_DISPLAY_LINEAR_BAR = 1,     // Horizontal/vertical segmented bar + timed modifier strips
    GAUGE_DISPLAY_COMPACT_HYBRID = 2  // CircularGauge dial on the left + linear modifier breakdown on the right
};

/**
 * DOD NOTE: Flat, pointer-free telemetry structs used to decouple HUD rendering
 * from live scene-tree Node pointers across frames.
 */
struct TimedModifierSlicePOD {
    int32_t value = 0;
    float time_limit = 0.0f;
    float current_time = 0.0f;
    float remaining_ratio = 0.0f;
    bool is_diminishing = false;
    int32_t target_channel = 0; // 0 = value, 1 = max_value, 2 = min_value
};

struct PropertyTelemetrySnapshotPOD {
    int32_t base_value = 0;
    int32_t modded_value = 0;
    int32_t base_max_value = 100;
    int32_t modded_max_value = 100;
    int32_t base_min_value = 0;
    int32_t modded_min_value = 0;
    bool is_exhausted = false;
    bool is_locked = false;
    bool valid = false;
};

/**
 * DOD NOTE: GamePropertyGauge (which also serves as GamePropertyBar when
 * `display_mode == GAUGE_DISPLAY_LINEAR_BAR`) binds declaratively to a `GameEntity`
 * by `ObjectID` and a `StringName` property title (e.g., `&"Health"`, `&"Stamina"`,
 * `&"Shield"`), or ingests flat `Dictionary` telemetry snapshots directly.
 *
 * It visualizes three layers simultaneously:
 * 1. Authoritative Base Value (`_value` / cached rule base value).
 * 2. Effective Modded Value (`get_modded_value()`), highlighting positive buffs vs.
 *    negative debuffs from `GameplayRule` or `GamePropertyModifier`.
 * 3. Active timed modifiers (`ModifierType::TIMED` / `TIMED_DIMINISHING`) via
 *    `CircularGauge` concentric rings and/or linear countdown bars.
 */
class GamePropertyGauge : public godot::Control {
    GDCLASS(GamePropertyGauge, godot::Control)

protected:
    static void _bind_methods();
    void _notification(int p_what);

private:
    // --- Declarative Binding (DOD-safe ObjectID + StringName) ---
    godot::ObjectID _target_entity_id;
    godot::ObjectID _bound_property_id;
    godot::StringName property_title = godot::StringName("Health");
    godot::String custom_label;

    PropertyGaugeDisplayMode display_mode = PropertyGaugeDisplayMode::GAUGE_DISPLAY_RADIAL;
    bool poll_bound_entity = true;
    bool smooth_transitions = true;
    float smoothing_speed = 10.0f;
    bool show_numeric_text = true;
    bool show_modifier_rings = true;
    int segment_count = 10;

    // --- Composed CircularGauge Child ---
    CircularGauge *_circular_gauge = nullptr;

    // --- Flat Telemetry State (Zero cross-frame Node pointer retention) ---
    PropertyTelemetrySnapshotPOD _snapshot;
    std::vector<TimedModifierSlicePOD> _timed_modifiers;

    // Smoothed visual values for fluid HUD animation
    float _visual_base_value = 0.0f;
    float _visual_modded_value = 0.0f;
    float _visual_max_value = 100.0f;
    float _visual_min_value = 0.0f;
    float _exhausted_pulse_phase = 0.0f;

    // --- Theme / Custom Color Overrides (alpha == 0 falls back to Theme/defaults) ---
    godot::Color custom_base_color = godot::Color(0.0f, 0.0f, 0.0f, 0.0f);
    godot::Color custom_buff_color = godot::Color(0.0f, 0.0f, 0.0f, 0.0f);
    godot::Color custom_debuff_color = godot::Color(0.0f, 0.0f, 0.0f, 0.0f);
    godot::Color custom_timed_ring_color = godot::Color(0.0f, 0.0f, 0.0f, 0.0f);
    godot::Color custom_bg_color = godot::Color(0.0f, 0.0f, 0.0f, 0.0f);

    // --- Internal Helpers ---
    void _ensure_circular_gauge();
    void _sync_circular_gauge_layout();
    GameProperty *_resolve_live_game_property() const;
    void _connect_property_signals(GameProperty *p_prop);
    void _disconnect_property_signals();
    void _pull_snapshot_from_property(GameProperty *p_prop);

    // Signal Callbacks from GameProperty
    void _on_property_value_changed(int p_old_value, int p_new_value);
    void _on_property_exhausted();
    void _on_property_restored();
    void _on_property_modifier_changed(godot::Object *p_mod);

    // Drawing Helpers
    void _draw_radial_overlay(const godot::Rect2 &p_dial_rect);
    void _draw_linear_bar(const godot::Rect2 &p_bar_rect, bool p_compact_labels);
    void _draw_arc_band(
        const godot::Vector2 &p_center,
        float p_inner_radius,
        float p_outer_radius,
        float p_start_deg,
        float p_end_deg,
        const godot::Color &p_color
    );

public:
    GamePropertyGauge();
    ~GamePropertyGauge() override;

    virtual void _ready() override;
    virtual void _process(double p_delta) override;
    virtual godot::Vector2 _get_minimum_size() const override;

    // --- Declarative Binding API ---
    void bind_to_entity(GameEntity *p_entity, const godot::StringName &p_property_title);
    void set_target_entity(GameEntity *p_entity);
    GameEntity *get_target_entity() const;

    void set_property_title(const godot::StringName &p_title);
    godot::StringName get_property_title() const;

    void set_custom_label(const godot::String &p_label);
    godot::String get_custom_label() const;

    // --- Display & Behavior Setters / Getters ---
    void set_display_mode(PropertyGaugeDisplayMode p_mode);
    PropertyGaugeDisplayMode get_display_mode() const;

    void set_poll_bound_entity(bool p_poll);
    bool get_poll_bound_entity() const;

    void set_smooth_transitions(bool p_smooth);
    bool get_smooth_transitions() const;

    void set_smoothing_speed(float p_speed);
    float get_smoothing_speed() const;

    void set_show_numeric_text(bool p_show);
    bool get_show_numeric_text() const;

    void set_show_modifier_rings(bool p_show);
    bool get_show_modifier_rings() const;

    void set_segment_count(int p_segments);
    int get_segment_count() const;

    CircularGauge *get_internal_circular_gauge() const;

    // --- Custom Color Overrides ---
    void set_custom_base_color(const godot::Color &p_color);
    godot::Color get_custom_base_color() const;

    void set_custom_buff_color(const godot::Color &p_color);
    godot::Color get_custom_buff_color() const;

    void set_custom_debuff_color(const godot::Color &p_color);
    godot::Color get_custom_debuff_color() const;

    void set_custom_timed_ring_color(const godot::Color &p_color);
    godot::Color get_custom_timed_ring_color() const;

    void set_custom_bg_color(const godot::Color &p_color);
    godot::Color get_custom_bg_color() const;

    // --- DOD Snapshot Ingestion & Telemetry Queries ---
    void refresh_from_bound_entity();
    void apply_property_snapshot(const godot::Dictionary &p_snapshot);
    godot::Dictionary get_property_snapshot() const;

    int get_base_value() const;
    int get_modded_value() const;
    int get_net_modifier_delta() const;
    int get_active_timed_modifier_count() const;
    bool is_property_exhausted() const;
    bool is_property_locked() const;
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::PropertyGaugeDisplayMode);