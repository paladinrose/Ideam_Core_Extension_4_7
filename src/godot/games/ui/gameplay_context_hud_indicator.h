#pragma once

#include <cstdint>
#include <vector>

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include "../../gameplay/gameplay_control_map.h"
#include "../../gameplay/gameplay_mode.h"
#include "../../gameplay/gameplay_style.h"

namespace ideam::godot_ext {

// Forward Declarations
class Game;
class GameBoard;
class GameAgent;

/**
 * DOD NOTE: Flat POD action prompt state used by GameplayContextHUDIndicator
 * so action glyph rendering and pulse animations never dereference Node pointers
 * during the draw pass.
 */
struct HUDActionPromptPOD {
    godot::StringName semantic_action;
    godot::String display_label;
    godot::String binding_glyph;
    float active_strength = 0.0f;
    float pulse_timer = 0.0f;
};

/**
 * DOD NOTE: GameplayContextHUDIndicator subscribes to `GameplayControlMap::device_family_changed`,
 * `action_triggered`, `action_released`, `vr_gesture_detected`, and `GameAgent` possession /
 * `GameplayStyle` / `GameplayMode` transitions.
 *
 * It provides three synchronized HUD layers:
 * 1. Device Family & Context Pill + Dynamic Action Prompt Bar (automatically swapping prompt
 *    glyphs between Keyboard/Mouse, Gamepad, Custom USB/Arduino, VR/OpenXR, and Touch).
 * 2. Transition Toast Notification Banner when a `GameBoard` or `GameAgent` switches
 *    `GameplayMode`, `GameplayStyle`, or possession target.
 * 3. VR / Motion Comfort Vignette (`vr_comfort_vignette_on_move`) rendered around the
 *    viewport perimeter when moving/turning in VR or during rapid context transitions.
 */
class GameplayContextHUDIndicator : public godot::Control {
    GDCLASS(GameplayContextHUDIndicator, godot::Control)

protected:
    static void _bind_methods();
    void _notification(int p_what);

private:
    // --- Context & Resource Bindings (DOD-safe ObjectIDs + Resource Refs) ---
    godot::ObjectID _game_id;
    godot::ObjectID _active_board_id;
    godot::ObjectID _possessed_agent_id;

    godot::Ref<GameplayControlMap> control_map;
    godot::Ref<GameplayStyle> active_style;
    godot::Ref<GameplayMode> active_mode;

    // --- Display & Behavior Settings ---
    bool show_device_badge = true;
    bool show_action_prompts = true;
    bool show_transition_toast = true;
    bool enable_comfort_vignette = true;
    float toast_duration = 2.8f;
    float vignette_max_intensity = 0.55f;
    float vignette_smoothing = 8.0f;

    godot::TypedArray<godot::StringName> tracked_actions;

    // --- Flat Runtime Telemetry State ---
    ControlDeviceFamily _active_device = ControlDeviceFamily::DEVICE_KEYBOARD_MOUSE;
    ControlContextMode _active_context_mode = ControlContextMode::CONTROL_CONTEXT_EXPLORATION;
    godot::String _possessed_agent_title;
    godot::String _active_mode_title;

    std::vector<HUDActionPromptPOD> _prompts;

    // Toast Notification State
    godot::String _toast_headline;
    godot::String _toast_subtext;
    float _toast_timer = 0.0f;

    // Comfort Vignette State (0.0 = transparent, 1.0 = max peripheral darkening)
    float _current_vignette_weight = 0.0f;
    float _target_vignette_weight = 0.0f;

    // --- Internal Helpers ---
    void _connect_control_map_signals();
    void _disconnect_control_map_signals();
    void _connect_agent_signals(GameAgent *p_agent);
    void _disconnect_agent_signals();

    void _rebuild_action_prompts();
    godot::String _resolve_binding_glyph(const godot::StringName &p_action, ControlDeviceFamily p_family) const;
    godot::String _resolve_action_display_label(const godot::StringName &p_action) const;
    static godot::String _device_family_short_name(ControlDeviceFamily p_family);
    static godot::String _context_mode_short_name(ControlContextMode p_mode);

    // Signal Callbacks
    void _on_device_family_changed(int p_new_family, int p_prev_family);
    void _on_action_triggered(const godot::StringName &p_action, float p_strength);
    void _on_action_released(const godot::StringName &p_action);
    void _on_vr_gesture_detected(const godot::StringName &p_gesture, int p_hand, float p_speed);
    void _on_agent_style_changed(const godot::Ref<GameplayStyle> &p_style);

    // Drawing Helpers
    void _draw_comfort_vignette(const godot::Vector2 &p_size);
    void _draw_transition_toast(const godot::Vector2 &p_size);
    void _draw_device_and_prompt_bar(const godot::Vector2 &p_size);
    void _draw_device_icon(const godot::Vector2 &p_center, ControlDeviceFamily p_family, const godot::Color &p_color);

public:
    GameplayContextHUDIndicator();
    ~GameplayContextHUDIndicator() override;

    virtual void _ready() override;
    virtual void _process(double p_delta) override;
    virtual godot::Vector2 _get_minimum_size() const override;

    // --- Context & Resource Setters / Getters ---
    void set_game(Game *p_game);
    Game *get_game() const;

    void set_active_board(GameBoard *p_board);
    GameBoard *get_active_board() const;

    void set_possessed_agent(GameAgent *p_agent);
    GameAgent *get_possessed_agent() const;

    void set_control_map(const godot::Ref<GameplayControlMap> &p_map);
    godot::Ref<GameplayControlMap> get_control_map() const;

    void set_active_style(const godot::Ref<GameplayStyle> &p_style);
    godot::Ref<GameplayStyle> get_active_style() const;

    void set_active_mode(const godot::Ref<GameplayMode> &p_mode);
    godot::Ref<GameplayMode> get_active_mode() const;

    // --- Display & Behavior Setters / Getters ---
    void set_show_device_badge(bool p_show);
    bool get_show_device_badge() const;

    void set_show_action_prompts(bool p_show);
    bool get_show_action_prompts() const;

    void set_show_transition_toast(bool p_show);
    bool get_show_transition_toast() const;

    void set_enable_comfort_vignette(bool p_enable);
    bool get_enable_comfort_vignette() const;

    void set_toast_duration(float p_duration);
    float get_toast_duration() const;

    void set_vignette_max_intensity(float p_intensity);
    float get_vignette_max_intensity() const;

    void set_vignette_smoothing(float p_smoothing);
    float get_vignette_smoothing() const;

    void set_tracked_actions(const godot::TypedArray<godot::StringName> &p_actions);
    godot::TypedArray<godot::StringName> get_tracked_actions() const;

    // --- Imperative HUD Actions & Telemetry Queries ---
    void show_mode_transition_toast(const godot::String &p_headline, const godot::String &p_subtext = "");
    void pulse_action_prompt(const godot::StringName &p_action, float p_strength = 1.0f);
    void set_manual_vignette_intensity(float p_weight);
    float get_current_vignette_weight() const;

    ControlDeviceFamily get_displayed_device_family() const;
    godot::String get_binding_glyph_for_action(const godot::StringName &p_action) const;
};

} // namespace ideam::godot_ext