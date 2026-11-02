#include "game_property_gauge.h"

#include <cmath>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/theme_db.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include "../../game_entity.h"
#include "../../game_entities/game_piece.h"
#include "../../game_property.h"
#include "../../game_property_modifier.h"
#include "../../gameplay/gameplay_rule.h"

namespace ideam::godot_ext {

void GamePropertyGauge::_bind_methods() {
    // --- Signals ---
    ADD_SIGNAL(godot::MethodInfo(
        "telemetry_updated",
        godot::PropertyInfo(godot::Variant::INT, "base_value"),
        godot::PropertyInfo(godot::Variant::INT, "modded_value"),
        godot::PropertyInfo(godot::Variant::INT, "max_value")
    ));
    ADD_SIGNAL(godot::MethodInfo("property_exhausted"));
    ADD_SIGNAL(godot::MethodInfo("property_restored"));

    // --- Enums ---
    BIND_ENUM_CONSTANT(PropertyGaugeDisplayMode::GAUGE_DISPLAY_RADIAL);
    BIND_ENUM_CONSTANT(PropertyGaugeDisplayMode::GAUGE_DISPLAY_LINEAR_BAR);
    BIND_ENUM_CONSTANT(PropertyGaugeDisplayMode::GAUGE_DISPLAY_COMPACT_HYBRID);

    // --- Binding & Target Group ---
    ADD_GROUP("Target Binding", "");

    godot::ClassDB::bind_method(godot::D_METHOD("bind_to_entity", "entity", "property_title"), &GamePropertyGauge::bind_to_entity);
    godot::ClassDB::bind_method(godot::D_METHOD("set_target_entity", "entity"), &GamePropertyGauge::set_target_entity);
    godot::ClassDB::bind_method(godot::D_METHOD("get_target_entity"), &GamePropertyGauge::get_target_entity);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::OBJECT, "target_entity", godot::PROPERTY_HINT_NODE_TYPE, "GameEntity"),
        "set_target_entity",
        "get_target_entity"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_property_title", "property_title"), &GamePropertyGauge::set_property_title);
    godot::ClassDB::bind_method(godot::D_METHOD("get_property_title"), &GamePropertyGauge::get_property_title);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::STRING_NAME, "property_title"),
        "set_property_title",
        "get_property_title"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_custom_label", "custom_label"), &GamePropertyGauge::set_custom_label);
    godot::ClassDB::bind_method(godot::D_METHOD("get_custom_label"), &GamePropertyGauge::get_custom_label);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::STRING, "custom_label"),
        "set_custom_label",
        "get_custom_label"
    );

    // --- Display & Animation Group ---
    ADD_GROUP("Display & Telemetry", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_display_mode", "mode"), &GamePropertyGauge::set_display_mode);
    godot::ClassDB::bind_method(godot::D_METHOD("get_display_mode"), &GamePropertyGauge::get_display_mode);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::INT, "display_mode", godot::PROPERTY_HINT_ENUM, "Radial (CircularGauge),Linear Bar,Compact Hybrid"),
        "set_display_mode",
        "get_display_mode"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_poll_bound_entity", "poll"), &GamePropertyGauge::set_poll_bound_entity);
    godot::ClassDB::bind_method(godot::D_METHOD("get_poll_bound_entity"), &GamePropertyGauge::get_poll_bound_entity);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::BOOL, "poll_bound_entity"),
        "set_poll_bound_entity",
        "get_poll_bound_entity"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_smooth_transitions", "smooth"), &GamePropertyGauge::set_smooth_transitions);
    godot::ClassDB::bind_method(godot::D_METHOD("get_smooth_transitions"), &GamePropertyGauge::get_smooth_transitions);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::BOOL, "smooth_transitions"),
        "set_smooth_transitions",
        "get_smooth_transitions"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_smoothing_speed", "speed"), &GamePropertyGauge::set_smoothing_speed);
    godot::ClassDB::bind_method(godot::D_METHOD("get_smoothing_speed"), &GamePropertyGauge::get_smoothing_speed);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "smoothing_speed", godot::PROPERTY_HINT_RANGE, "1.0,60.0,0.5"),
        "set_smoothing_speed",
        "get_smoothing_speed"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_show_numeric_text", "show"), &GamePropertyGauge::set_show_numeric_text);
    godot::ClassDB::bind_method(godot::D_METHOD("get_show_numeric_text"), &GamePropertyGauge::get_show_numeric_text);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::BOOL, "show_numeric_text"),
        "set_show_numeric_text",
        "get_show_numeric_text"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_show_modifier_rings", "show"), &GamePropertyGauge::set_show_modifier_rings);
    godot::ClassDB::bind_method(godot::D_METHOD("get_show_modifier_rings"), &GamePropertyGauge::get_show_modifier_rings);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::BOOL, "show_modifier_rings"),
        "set_show_modifier_rings",
        "get_show_modifier_rings"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_segment_count", "segments"), &GamePropertyGauge::set_segment_count);
    godot::ClassDB::bind_method(godot::D_METHOD("get_segment_count"), &GamePropertyGauge::get_segment_count);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::INT, "segment_count", godot::PROPERTY_HINT_RANGE, "0,50,1"),
        "set_segment_count",
        "get_segment_count"
    );

    // --- Custom Color Overrides Group ---
    ADD_GROUP("Custom Colors", "custom_");

    godot::ClassDB::bind_method(godot::D_METHOD("set_custom_base_color", "color"), &GamePropertyGauge::set_custom_base_color);
    godot::ClassDB::bind_method(godot::D_METHOD("get_custom_base_color"), &GamePropertyGauge::get_custom_base_color);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::COLOR, "custom_base_color"), "set_custom_base_color", "get_custom_base_color");

    godot::ClassDB::bind_method(godot::D_METHOD("set_custom_buff_color", "color"), &GamePropertyGauge::set_custom_buff_color);
    godot::ClassDB::bind_method(godot::D_METHOD("get_custom_buff_color"), &GamePropertyGauge::get_custom_buff_color);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::COLOR, "custom_buff_color"), "set_custom_buff_color", "get_custom_buff_color");

    godot::ClassDB::bind_method(godot::D_METHOD("set_custom_debuff_color", "color"), &GamePropertyGauge::set_custom_debuff_color);
    godot::ClassDB::bind_method(godot::D_METHOD("get_custom_debuff_color"), &GamePropertyGauge::get_custom_debuff_color);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::COLOR, "custom_debuff_color"), "set_custom_debuff_color", "get_custom_debuff_color");

    godot::ClassDB::bind_method(godot::D_METHOD("set_custom_timed_ring_color", "color"), &GamePropertyGauge::set_custom_timed_ring_color);
    godot::ClassDB::bind_method(godot::D_METHOD("get_custom_timed_ring_color"), &GamePropertyGauge::get_custom_timed_ring_color);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::COLOR, "custom_timed_ring_color"), "set_custom_timed_ring_color", "get_custom_timed_ring_color");

    godot::ClassDB::bind_method(godot::D_METHOD("set_custom_bg_color", "color"), &GamePropertyGauge::set_custom_bg_color);
    godot::ClassDB::bind_method(godot::D_METHOD("get_custom_bg_color"), &GamePropertyGauge::get_custom_bg_color);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::COLOR, "custom_bg_color"), "set_custom_bg_color", "get_custom_bg_color");

    // --- Methods ---
    godot::ClassDB::bind_method(godot::D_METHOD("get_internal_circular_gauge"), &GamePropertyGauge::get_internal_circular_gauge);
    godot::ClassDB::bind_method(godot::D_METHOD("refresh_from_bound_entity"), &GamePropertyGauge::refresh_from_bound_entity);
    godot::ClassDB::bind_method(godot::D_METHOD("apply_property_snapshot", "snapshot"), &GamePropertyGauge::apply_property_snapshot);
    godot::ClassDB::bind_method(godot::D_METHOD("get_property_snapshot"), &GamePropertyGauge::get_property_snapshot);

    godot::ClassDB::bind_method(godot::D_METHOD("get_base_value"), &GamePropertyGauge::get_base_value);
    godot::ClassDB::bind_method(godot::D_METHOD("get_modded_value"), &GamePropertyGauge::get_modded_value);
    godot::ClassDB::bind_method(godot::D_METHOD("get_net_modifier_delta"), &GamePropertyGauge::get_net_modifier_delta);
    godot::ClassDB::bind_method(godot::D_METHOD("get_active_timed_modifier_count"), &GamePropertyGauge::get_active_timed_modifier_count);
    godot::ClassDB::bind_method(godot::D_METHOD("is_property_exhausted"), &GamePropertyGauge::is_property_exhausted);
    godot::ClassDB::bind_method(godot::D_METHOD("is_property_locked"), &GamePropertyGauge::is_property_locked);

    // Internal Signal Callbacks
    godot::ClassDB::bind_method(godot::D_METHOD("_on_property_value_changed", "old_value", "new_value"), &GamePropertyGauge::_on_property_value_changed);
    godot::ClassDB::bind_method(godot::D_METHOD("_on_property_exhausted"), &GamePropertyGauge::_on_property_exhausted);
    godot::ClassDB::bind_method(godot::D_METHOD("_on_property_restored"), &GamePropertyGauge::_on_property_restored);
    godot::ClassDB::bind_method(godot::D_METHOD("_on_property_modifier_changed", "mod"), &GamePropertyGauge::_on_property_modifier_changed);
}

GamePropertyGauge::GamePropertyGauge() {
    set_clip_contents(false);
    set_mouse_filter(MOUSE_FILTER_PASS);
    set_process(true);

    // Default sample telemetry for Editor preview when unbound
    _snapshot.base_value = 75;
    _snapshot.modded_value = 90;
    _snapshot.base_max_value = 100;
    _snapshot.modded_max_value = 100;
    _snapshot.base_min_value = 0;
    _snapshot.modded_min_value = 0;
    _snapshot.valid = true;

    _visual_base_value = 75.0f;
    _visual_modded_value = 90.0f;
    _visual_max_value = 100.0f;
    _visual_min_value = 0.0f;
}

GamePropertyGauge::~GamePropertyGauge() {
    _disconnect_property_signals();
}

void GamePropertyGauge::_ready() {
    _ensure_circular_gauge();
    refresh_from_bound_entity();
    _sync_circular_gauge_layout();
    queue_redraw();
}

void GamePropertyGauge::_process(double p_delta) {
    if (poll_bound_entity && _target_entity_id.is_valid()) {
        refresh_from_bound_entity();
    }

    float dt = static_cast<float>(p_delta);
    bool needs_redraw = false;

    if (smooth_transitions) {
        float alpha = godot::Math::clamp(dt * smoothing_speed, 0.0f, 1.0f);
        float next_base = godot::Math::lerp(_visual_base_value, static_cast<float>(_snapshot.base_value), alpha);
        float next_mod = godot::Math::lerp(_visual_modded_value, static_cast<float>(_snapshot.modded_value), alpha);
        float next_max = godot::Math::lerp(_visual_max_value, static_cast<float>(godot::Math::max(1, _snapshot.modded_max_value)), alpha);
        float next_min = godot::Math::lerp(_visual_min_value, static_cast<float>(_snapshot.modded_min_value), alpha);

        if (!godot::Math::is_equal_approx(next_base, _visual_base_value) ||
            !godot::Math::is_equal_approx(next_mod, _visual_modded_value) ||
            !godot::Math::is_equal_approx(next_max, _visual_max_value)) {
            _visual_base_value = next_base;
            _visual_modded_value = next_mod;
            _visual_max_value = next_max;
            _visual_min_value = next_min;
            needs_redraw = true;
        }
    } else {
        _visual_base_value = static_cast<float>(_snapshot.base_value);
        _visual_modded_value = static_cast<float>(_snapshot.modded_value);
        _visual_max_value = static_cast<float>(godot::Math::max(1, _snapshot.modded_max_value));
        _visual_min_value = static_cast<float>(_snapshot.modded_min_value);
    }

    if (_snapshot.is_exhausted) {
        _exhausted_pulse_phase = std::fmod(_exhausted_pulse_phase + dt * 4.5f, static_cast<float>(godot::Math::TAU));
        needs_redraw = true;
    }

    if (!_timed_modifiers.empty()) {
        needs_redraw = true;
    }

    if (needs_redraw) {
        _sync_circular_gauge_layout();
        queue_redraw();
    }
}

godot::Vector2 GamePropertyGauge::_get_minimum_size() const {
    if (display_mode == PropertyGaugeDisplayMode::GAUGE_DISPLAY_LINEAR_BAR) {
        return godot::Vector2(200.0f, 44.0f);
    }
    if (display_mode == PropertyGaugeDisplayMode::GAUGE_DISPLAY_COMPACT_HYBRID) {
        return godot::Vector2(240.0f, 96.0f);
    }
    return godot::Vector2(110.0f, 110.0f);
}

// --- Declarative Binding API ---

void GamePropertyGauge::bind_to_entity(GameEntity *p_entity, const godot::StringName &p_property_title) {
    _disconnect_property_signals();
    _target_entity_id = p_entity ? godot::ObjectID(p_entity->get_instance_id()) : godot::ObjectID();
    if (!p_property_title.is_empty()) {
        property_title = p_property_title;
    }
    refresh_from_bound_entity();
    _sync_circular_gauge_layout();
    queue_redraw();
}

void GamePropertyGauge::set_target_entity(GameEntity *p_entity) {
    godot::ObjectID new_id = p_entity ? godot::ObjectID(p_entity->get_instance_id()) : godot::ObjectID();
    if (_target_entity_id == new_id) return;
    _disconnect_property_signals();
    _target_entity_id = new_id;
    refresh_from_bound_entity();
    _sync_circular_gauge_layout();
    queue_redraw();
}

GameEntity *GamePropertyGauge::get_target_entity() const {
    if (!_target_entity_id.is_valid()) {
        return nullptr;
    }
    return godot::Object::cast_to<GameEntity>(godot::ObjectDB::get_instance(_target_entity_id));
}

void GamePropertyGauge::set_property_title(const godot::StringName &p_title) {
    if (property_title == p_title) return;
    _disconnect_property_signals();
    property_title = p_title;
    refresh_from_bound_entity();
    _sync_circular_gauge_layout();
    queue_redraw();
}

godot::StringName GamePropertyGauge::get_property_title() const {
    return property_title;
}

void GamePropertyGauge::set_custom_label(const godot::String &p_label) {
    if (custom_label == p_label) return;
    custom_label = p_label;
    queue_redraw();
}

godot::String GamePropertyGauge::get_custom_label() const {
    return custom_label;
}

// --- Display & Behavior Setters / Getters ---

void GamePropertyGauge::set_display_mode(PropertyGaugeDisplayMode p_mode) {
    if (display_mode == p_mode) return;
    display_mode = p_mode;
    _ensure_circular_gauge();
    _sync_circular_gauge_layout();
    update_minimum_size();
    queue_redraw();
}

PropertyGaugeDisplayMode GamePropertyGauge::get_display_mode() const {
    return display_mode;
}

void GamePropertyGauge::set_poll_bound_entity(bool p_poll) {
    poll_bound_entity = p_poll;
}

bool GamePropertyGauge::get_poll_bound_entity() const {
    return poll_bound_entity;
}

void GamePropertyGauge::set_smooth_transitions(bool p_smooth) {
    smooth_transitions = p_smooth;
}

bool GamePropertyGauge::get_smooth_transitions() const {
    return smooth_transitions;
}

void GamePropertyGauge::set_smoothing_speed(float p_speed) {
    smoothing_speed = godot::Math::clamp(p_speed, 0.5f, 120.0f);
}

float GamePropertyGauge::get_smoothing_speed() const {
    return smoothing_speed;
}

void GamePropertyGauge::set_show_numeric_text(bool p_show) {
    if (show_numeric_text == p_show) return;
    show_numeric_text = p_show;
    queue_redraw();
}

bool GamePropertyGauge::get_show_numeric_text() const {
    return show_numeric_text;
}

void GamePropertyGauge::set_show_modifier_rings(bool p_show) {
    if (show_modifier_rings == p_show) return;
    show_modifier_rings = p_show;
    _sync_circular_gauge_layout();
    queue_redraw();
}

bool GamePropertyGauge::get_show_modifier_rings() const {
    return show_modifier_rings;
}

void GamePropertyGauge::set_segment_count(int p_segments) {
    int clamped = godot::Math::clamp(p_segments, 0, 100);
    if (segment_count == clamped) return;
    segment_count = clamped;
    _sync_circular_gauge_layout();
    queue_redraw();
}

int GamePropertyGauge::get_segment_count() const {
    return segment_count;
}

CircularGauge *GamePropertyGauge::get_internal_circular_gauge() const {
    return _circular_gauge;
}

// --- Custom Color Overrides ---

void GamePropertyGauge::set_custom_base_color(const godot::Color &p_color) {
    if (custom_base_color == p_color) return;
    custom_base_color = p_color;
    _sync_circular_gauge_layout();
    queue_redraw();
}

godot::Color GamePropertyGauge::get_custom_base_color() const {
    return custom_base_color;
}

void GamePropertyGauge::set_custom_buff_color(const godot::Color &p_color) {
    if (custom_buff_color == p_color) return;
    custom_buff_color = p_color;
    _sync_circular_gauge_layout();
    queue_redraw();
}

godot::Color GamePropertyGauge::get_custom_buff_color() const {
    return custom_buff_color;
}

void GamePropertyGauge::set_custom_debuff_color(const godot::Color &p_color) {
    if (custom_debuff_color == p_color) return;
    custom_debuff_color = p_color;
    _sync_circular_gauge_layout();
    queue_redraw();
}

godot::Color GamePropertyGauge::get_custom_debuff_color() const {
    return custom_debuff_color;
}

void GamePropertyGauge::set_custom_timed_ring_color(const godot::Color &p_color) {
    if (custom_timed_ring_color == p_color) return;
    custom_timed_ring_color = p_color;
    _sync_circular_gauge_layout();
    queue_redraw();
}

godot::Color GamePropertyGauge::get_custom_timed_ring_color() const {
    return custom_timed_ring_color;
}

void GamePropertyGauge::set_custom_bg_color(const godot::Color &p_color) {
    if (custom_bg_color == p_color) return;
    custom_bg_color = p_color;
    _sync_circular_gauge_layout();
    queue_redraw();
}

godot::Color GamePropertyGauge::get_custom_bg_color() const {
    return custom_bg_color;
}

// --- Internal Resolution & Signal Wiring ---

void GamePropertyGauge::_ensure_circular_gauge() {
    if (!_circular_gauge) {
        _circular_gauge = memnew(CircularGauge);
        _circular_gauge->set_name("InternalCircularGauge");
        _circular_gauge->set_mouse_filter(MOUSE_FILTER_IGNORE);
        _circular_gauge->set_marker_count(1);
        _circular_gauge->set_start_angle(135.0f);
        _circular_gauge->set_end_angle(405.0f);
        _circular_gauge->set_draw_head_tail_markers(false);
        add_child(_circular_gauge);
    }
    _circular_gauge->set_visible(display_mode != PropertyGaugeDisplayMode::GAUGE_DISPLAY_LINEAR_BAR);
}

void GamePropertyGauge::_sync_circular_gauge_layout() {
    if (!_circular_gauge) {
        return;
    }

    bool use_radial = (display_mode != PropertyGaugeDisplayMode::GAUGE_DISPLAY_LINEAR_BAR);
    _circular_gauge->set_visible(use_radial);
    if (!use_radial) {
        return;
    }

    godot::Vector2 sz = get_size();
    if (display_mode == PropertyGaugeDisplayMode::GAUGE_DISPLAY_COMPACT_HYBRID) {
        float dial_side = godot::Math::min(sz.y, sz.x * 0.45f);
        _circular_gauge->set_position(godot::Vector2(0.0f, (sz.y - dial_side) * 0.5f));
        _circular_gauge->set_size(godot::Vector2(dial_side, dial_side));
    } else {
        _circular_gauge->set_position(godot::Vector2(0.0f, 0.0f));
        _circular_gauge->set_size(sz);
    }

    godot::Color base_col = (custom_base_color.a > 0.0f) ? custom_base_color : godot::Color(0.18f, 0.68f, 0.92f, 1.0f);
    godot::Color bg_col = (custom_bg_color.a > 0.0f) ? custom_bg_color : godot::Color(0.11f, 0.13f, 0.16f, 0.88f);
    godot::Color overflow_col = (custom_debuff_color.a > 0.0f) ? custom_debuff_color : godot::Color(0.92f, 0.25f, 0.25f, 0.95f);

    _circular_gauge->set_custom_bg_color(bg_col);
    _circular_gauge->set_custom_fill_color(base_col);
    _circular_gauge->set_custom_overflow_color(overflow_col);
    _circular_gauge->set_min_value(_visual_min_value);
    _circular_gauge->set_max_value(godot::Math::max(_visual_min_value + 1.0f, _visual_max_value));
    _circular_gauge->set_value_1(godot::Math::clamp(_visual_base_value, _visual_min_value, _visual_max_value));
    _circular_gauge->set_slot_count(segment_count);
    _circular_gauge->set_draw_slot_ticks(segment_count > 0);
    _circular_gauge->set_is_overflowing(_snapshot.is_exhausted);

    godot::PackedColorArray layers;
    layers.push_back(base_col);
    if (show_modifier_rings && !_timed_modifiers.empty()) {
        godot::Color timed_col = (custom_timed_ring_color.a > 0.0f) ? custom_timed_ring_color : godot::Color(0.98f, 0.78f, 0.20f, 0.90f);
        layers.push_back(timed_col.darkened(0.15f));
    }
    _circular_gauge->set_layer_colors(layers);
    _circular_gauge->queue_redraw();
}

GameProperty *GamePropertyGauge::_resolve_live_game_property() const {
    GameEntity *entity = get_target_entity();
    if (!entity || property_title.is_empty()) {
        return nullptr;
    }

    godot::String title_str(property_title);

    // 1. Fast path for GamePiece
    if (GamePiece *piece = godot::Object::cast_to<GamePiece>(entity)) {
        if (GameProperty *prop = piece->get_property(title_str)) {
            return prop;
        }
    }

    // 2. Direct child GameProperty lookup on GameEntity
    int child_count = entity->get_child_count();
    for (int i = 0; i < child_count; ++i) {
        if (GameProperty *prop = godot::Object::cast_to<GameProperty>(entity->get_child(i))) {
            if (prop->get_title() == title_str || prop->get_name() == property_title) {
                return prop;
            }
        }
    }

    // 3. Fallback: search owned GamePieces if entity is a GameAgent or GameBoard
    if (entity->has_method("get_game_pieces")) {
        godot::Array pieces = entity->call("get_game_pieces");
        for (int i = 0; i < pieces.size(); ++i) {
            if (GamePiece *piece = godot::Object::cast_to<GamePiece>(pieces[i])) {
                if (GameProperty *prop = piece->get_property(title_str)) {
                    return prop;
                }
            }
        }
    }

    return nullptr;
}

void GamePropertyGauge::_connect_property_signals(GameProperty *p_prop) {
    if (!p_prop) return;
    godot::ObjectID prop_id(p_prop->get_instance_id());
    if (_bound_property_id == prop_id) {
        return;
    }

    _disconnect_property_signals();
    _bound_property_id = prop_id;

    godot::Callable val_cb(this, "_on_property_value_changed");
    if (!p_prop->is_connected("value_changed", val_cb)) {
        p_prop->connect("value_changed", val_cb);
    }
    if (!p_prop->is_connected("max_value_changed", val_cb)) {
        p_prop->connect("max_value_changed", val_cb);
    }
    if (!p_prop->is_connected("min_value_changed", val_cb)) {
        p_prop->connect("min_value_changed", val_cb);
    }

    godot::Callable exh_cb(this, "_on_property_exhausted");
    if (!p_prop->is_connected("value_exhausted", exh_cb)) {
        p_prop->connect("value_exhausted", exh_cb);
    }

    godot::Callable rest_cb(this, "_on_property_restored");
    if (!p_prop->is_connected("value_no_longer_exhausted", rest_cb)) {
        p_prop->connect("value_no_longer_exhausted", rest_cb);
    }

    godot::Callable mod_cb(this, "_on_property_modifier_changed");
    if (!p_prop->is_connected("modifier_added", mod_cb)) {
        p_prop->connect("modifier_added", mod_cb);
    }
    if (!p_prop->is_connected("modifier_removed", mod_cb)) {
        p_prop->connect("modifier_removed", mod_cb);
    }
}

void GamePropertyGauge::_disconnect_property_signals() {
    if (!_bound_property_id.is_valid()) {
        return;
    }
    if (GameProperty *prop = godot::Object::cast_to<GameProperty>(godot::ObjectDB::get_instance(_bound_property_id))) {
        godot::Callable val_cb(this, "_on_property_value_changed");
        if (prop->is_connected("value_changed", val_cb)) prop->disconnect("value_changed", val_cb);
        if (prop->is_connected("max_value_changed", val_cb)) prop->disconnect("max_value_changed", val_cb);
        if (prop->is_connected("min_value_changed", val_cb)) prop->disconnect("min_value_changed", val_cb);

        godot::Callable exh_cb(this, "_on_property_exhausted");
        if (prop->is_connected("value_exhausted", exh_cb)) prop->disconnect("value_exhausted", exh_cb);

        godot::Callable rest_cb(this, "_on_property_restored");
        if (prop->is_connected("value_no_longer_exhausted", rest_cb)) prop->disconnect("value_no_longer_exhausted", rest_cb);

        godot::Callable mod_cb(this, "_on_property_modifier_changed");
        if (prop->is_connected("modifier_added", mod_cb)) prop->disconnect("modifier_added", mod_cb);
        if (prop->is_connected("modifier_removed", mod_cb)) prop->disconnect("modifier_removed", mod_cb);
    }
    _bound_property_id = godot::ObjectID();
}

void GamePropertyGauge::_pull_snapshot_from_property(GameProperty *p_prop) {
    if (!p_prop) {
        return;
    }

    _connect_property_signals(p_prop);

    int raw_prop_val = p_prop->get_value();
    godot::Variant cached_base = GameplayRule::get_cached_base_value(p_prop, godot::StringName("value"), raw_prop_val);
    int authoritative_base = static_cast<int>(cached_base);

    int modded_val = p_prop->get_modded_value();
    if (modded_val == raw_prop_val && authoritative_base != raw_prop_val) {
        _snapshot.base_value = authoritative_base;
        _snapshot.modded_value = raw_prop_val;
    } else {
        _snapshot.base_value = raw_prop_val;
        _snapshot.modded_value = modded_val;
    }

    _snapshot.base_max_value = p_prop->get_max_value();
    _snapshot.modded_max_value = p_prop->get_modded_max_value();
    _snapshot.base_min_value = p_prop->get_min_value();
    _snapshot.modded_min_value = p_prop->get_modded_min_value();
    _snapshot.is_exhausted = p_prop->get_is_exhausted();
    _snapshot.is_locked = p_prop->get_locked();
    _snapshot.valid = true;

    _timed_modifiers.clear();
    auto harvest_timed = [this](const godot::TypedArray<GamePropertyModifier> &p_mods, int32_t p_channel) {
        for (int i = 0; i < p_mods.size(); ++i) {
            GamePropertyModifier *mod = godot::Object::cast_to<GamePropertyModifier>(p_mods[i]);
            if (!mod) continue;

            ModifierType mtype = mod->get_type();
            if (mtype != ModifierType::TIMED && mtype != ModifierType::TIMED_DIMINISHING) {
                continue;
            }

            float limit = mod->get_time_limit();
            if (limit <= 0.0001f) continue;

            godot::Dictionary mdata = mod->save_data();
            float elapsed = mdata.has("current_time") ? static_cast<float>(mdata["current_time"]) : 0.0f;
            float rem_ratio = godot::Math::clamp(1.0f - (elapsed / limit), 0.0f, 1.0f);

            TimedModifierSlicePOD slice;
            slice.value = mod->get_value();
            slice.time_limit = limit;
            slice.current_time = elapsed;
            slice.remaining_ratio = rem_ratio;
            slice.is_diminishing = (mtype == ModifierType::TIMED_DIMINISHING);
            slice.target_channel = p_channel;
            _timed_modifiers.push_back(slice);
        }
    };

    harvest_timed(p_prop->get_modifiers(), 0);
    harvest_timed(p_prop->get_max_modifiers(), 1);
    harvest_timed(p_prop->get_min_modifiers(), 2);
}

void GamePropertyGauge::_on_property_value_changed(int p_old_value, int p_new_value) {
    refresh_from_bound_entity();
    emit_signal("telemetry_updated", _snapshot.base_value, _snapshot.modded_value, _snapshot.modded_max_value);
}

void GamePropertyGauge::_on_property_exhausted() {
    refresh_from_bound_entity();
    emit_signal("property_exhausted");
}

void GamePropertyGauge::_on_property_restored() {
    refresh_from_bound_entity();
    emit_signal("property_restored");
}

void GamePropertyGauge::_on_property_modifier_changed(godot::Object *p_mod) {
    refresh_from_bound_entity();
}

// --- Public Telemetry & Snapshot Methods ---

void GamePropertyGauge::refresh_from_bound_entity() {
    if (GameProperty *prop = _resolve_live_game_property()) {
        _pull_snapshot_from_property(prop);
        _sync_circular_gauge_layout();
        queue_redraw();
    }
}

void GamePropertyGauge::apply_property_snapshot(const godot::Dictionary &p_snapshot) {
    if (p_snapshot.has("base_value")) _snapshot.base_value = p_snapshot["base_value"];
    if (p_snapshot.has("modded_value")) _snapshot.modded_value = p_snapshot["modded_value"];
    if (p_snapshot.has("base_max_value")) _snapshot.base_max_value = p_snapshot["base_max_value"];
    if (p_snapshot.has("modded_max_value")) _snapshot.modded_max_value = p_snapshot["modded_max_value"];
    if (p_snapshot.has("base_min_value")) _snapshot.base_min_value = p_snapshot["base_min_value"];
    if (p_snapshot.has("modded_min_value")) _snapshot.modded_min_value = p_snapshot["modded_min_value"];
    if (p_snapshot.has("is_exhausted")) _snapshot.is_exhausted = p_snapshot["is_exhausted"];
    if (p_snapshot.has("is_locked")) _snapshot.is_locked = p_snapshot["is_locked"];
    _snapshot.valid = true;

    _timed_modifiers.clear();
    if (p_snapshot.has("timed_modifiers") && p_snapshot["timed_modifiers"].get_type() == godot::Variant::ARRAY) {
        godot::Array arr = p_snapshot["timed_modifiers"];
        for (int i = 0; i < arr.size(); ++i) {
            if (arr[i].get_type() != godot::Variant::DICTIONARY) continue;
            godot::Dictionary d = arr[i];
            TimedModifierSlicePOD slice;
            slice.value = d.get("value", 0);
            slice.time_limit = d.get("time_limit", 1.0f);
            slice.current_time = d.get("current_time", 0.0f);
            slice.remaining_ratio = d.get("remaining_ratio", 1.0f);
            slice.is_diminishing = d.get("is_diminishing", false);
            slice.target_channel = d.get("target_channel", 0);
            _timed_modifiers.push_back(slice);
        }
    }

    if (!smooth_transitions) {
        _visual_base_value = static_cast<float>(_snapshot.base_value);
        _visual_modded_value = static_cast<float>(_snapshot.modded_value);
        _visual_max_value = static_cast<float>(godot::Math::max(1, _snapshot.modded_max_value));
        _visual_min_value = static_cast<float>(_snapshot.modded_min_value);
    }

    _sync_circular_gauge_layout();
    queue_redraw();
}

godot::Dictionary GamePropertyGauge::get_property_snapshot() const {
    godot::Dictionary d;
    d["property_title"] = godot::String(property_title);
    d["base_value"] = _snapshot.base_value;
    d["modded_value"] = _snapshot.modded_value;
    d["base_max_value"] = _snapshot.base_max_value;
    d["modded_max_value"] = _snapshot.modded_max_value;
    d["base_min_value"] = _snapshot.base_min_value;
    d["modded_min_value"] = _snapshot.modded_min_value;
    d["is_exhausted"] = _snapshot.is_exhausted;
    d["is_locked"] = _snapshot.is_locked;

    godot::Array mods;
    for (const TimedModifierSlicePOD &slice : _timed_modifiers) {
        godot::Dictionary m;
        m["value"] = slice.value;
        m["time_limit"] = slice.time_limit;
        m["current_time"] = slice.current_time;
        m["remaining_ratio"] = slice.remaining_ratio;
        m["is_diminishing"] = slice.is_diminishing;
        m["target_channel"] = slice.target_channel;
        mods.append(m);
    }
    d["timed_modifiers"] = mods;
    return d;
}

int GamePropertyGauge::get_base_value() const { return _snapshot.base_value; }
int GamePropertyGauge::get_modded_value() const { return _snapshot.modded_value; }
int GamePropertyGauge::get_net_modifier_delta() const { return _snapshot.modded_value - _snapshot.base_value; }
int GamePropertyGauge::get_active_timed_modifier_count() const { return static_cast<int>(_timed_modifiers.size()); }
bool GamePropertyGauge::is_property_exhausted() const { return _snapshot.is_exhausted; }
bool GamePropertyGauge::is_property_locked() const { return _snapshot.is_locked; }

// --- Custom Drawing ---

void GamePropertyGauge::_notification(int p_what) {
    if (p_what == NOTIFICATION_RESIZED || p_what == NOTIFICATION_THEME_CHANGED) {
        _sync_circular_gauge_layout();
        queue_redraw();
    } else if (p_what == NOTIFICATION_DRAW) {
        godot::Vector2 sz = get_size();
        if (sz.x <= 2.0f || sz.y <= 2.0f) {
            return;
        }

        if (display_mode == PropertyGaugeDisplayMode::GAUGE_DISPLAY_RADIAL) {
            _draw_radial_overlay(godot::Rect2(godot::Vector2(), sz));
        } else if (display_mode == PropertyGaugeDisplayMode::GAUGE_DISPLAY_LINEAR_BAR) {
            _draw_linear_bar(godot::Rect2(godot::Vector2(), sz), false);
        } else {
            float dial_side = godot::Math::min(sz.y, sz.x * 0.45f);
            godot::Rect2 dial_rect(godot::Vector2(0.0f, (sz.y - dial_side) * 0.5f), godot::Vector2(dial_side, dial_side));
            godot::Rect2 bar_rect(godot::Vector2(dial_side + 8.0f, 4.0f), godot::Vector2(godot::Math::max(20.0f, sz.x - dial_side - 12.0f), sz.y - 8.0f));
            _draw_radial_overlay(dial_rect);
            _draw_linear_bar(bar_rect, true);
        }
    }
}

void GamePropertyGauge::_draw_arc_band(
    const godot::Vector2 &p_center,
    float p_inner_radius,
    float p_outer_radius,
    float p_start_deg,
    float p_end_deg,
    const godot::Color &p_color
) {
    if (p_outer_radius <= p_inner_radius || godot::Math::is_equal_approx(p_start_deg, p_end_deg)) {
        return;
    }

    float start_rad = godot::Math::deg_to_rad(p_start_deg);
    float end_rad = godot::Math::deg_to_rad(p_end_deg);
    int steps = godot::Math::clamp(static_cast<int>(std::fabs(p_end_deg - p_start_deg) * 0.35f), 6, 48);

    godot::PackedVector2Array poly;
    poly.resize((steps + 1) * 2);

    for (int i = 0; i <= steps; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(steps);
        float ang = godot::Math::lerp(start_rad, end_rad, t);
        godot::Vector2 dir(std::cos(ang), std::sin(ang));
        poly.set(i, p_center + dir * p_outer_radius);
        poly.set((steps * 2) + 1 - i, p_center + dir * p_inner_radius);
    }

    godot::PackedColorArray colors;
    colors.push_back(p_color);
    draw_polygon(poly, colors);
}

void GamePropertyGauge::_draw_radial_overlay(const godot::Rect2 &p_dial_rect) {
    godot::Color buff_col = (custom_buff_color.a > 0.0f) ? custom_buff_color : godot::Color(0.22f, 0.88f, 0.48f, 0.92f);
    godot::Color debuff_col = (custom_debuff_color.a > 0.0f) ? custom_debuff_color : godot::Color(0.95f, 0.30f, 0.30f, 0.92f);
    godot::Color timed_col = (custom_timed_ring_color.a > 0.0f) ? custom_timed_ring_color : godot::Color(0.98f, 0.80f, 0.22f, 0.92f);

    godot::Vector2 center = p_dial_rect.position + p_dial_rect.size * 0.5f;
    float max_radius = godot::Math::min(p_dial_rect.size.x, p_dial_rect.size.y) * 0.5f;
    float outer_r = godot::Math::max(max_radius - 16.0f, 12.0f);
    float inner_r = godot::Math::max(outer_r - 22.0f, 8.0f);

    float span_val = godot::Math::max(1.0f, _visual_max_value - _visual_min_value);
    float base_ratio = godot::Math::clamp((_visual_base_value - _visual_min_value) / span_val, 0.0f, 1.0f);
    float mod_ratio = godot::Math::clamp((_visual_modded_value - _visual_min_value) / span_val, 0.0f, 1.0f);

    float start_deg = 135.0f;
    float total_sweep = 270.0f;
    float base_deg = start_deg + base_ratio * total_sweep;
    float mod_deg = start_deg + mod_ratio * total_sweep;

    // 1. Highlight positive buff or negative debuff delta arc over the outer band
    if (!godot::Math::is_equal_approx(base_deg, mod_deg)) {
        if (mod_deg > base_deg) {
            _draw_arc_band(center, inner_r + 2.0f, outer_r - 1.0f, base_deg, mod_deg, buff_col);
        } else {
            _draw_arc_band(center, inner_r + 2.0f, outer_r - 1.0f, mod_deg, base_deg, debuff_col);
        }
    }

    // 2. Draw concentric inner countdown rings for each active TIMED / TIMED_DIMINISHING modifier
    if (show_modifier_rings && !_timed_modifiers.empty()) {
        int ring_count = godot::Math::min(static_cast<int>(_timed_modifiers.size()), 3);
        float ring_thick = 4.0f;
        float ring_gap = 2.0f;
        for (int i = 0; i < ring_count; ++i) {
            const TimedModifierSlicePOD &slice = _timed_modifiers[i];
            float r_out = inner_r - 3.0f - i * (ring_thick + ring_gap);
            float r_in = r_out - ring_thick;
            if (r_in <= 4.0f) break;

            godot::Color ring_c = (slice.value >= 0) ? timed_col : debuff_col.lightened(0.15f);
            float sweep_end = start_deg + godot::Math::clamp(slice.remaining_ratio, 0.0f, 1.0f) * total_sweep;
            _draw_arc_band(center, r_in, r_out, start_deg, start_deg + total_sweep, godot::Color(0.08f, 0.09f, 0.12f, 0.65f));
            _draw_arc_band(center, r_in, r_out, start_deg, sweep_end, ring_c);
        }
    }

    // 3. Center Text Readout (Property Title, Modded Value, Net Delta / Status)
    if (show_numeric_text) {
        godot::Ref<godot::Font> font = godot::ThemeDB::get_singleton()->get_fallback_font();
        if (font.is_valid()) {
            godot::String label_str = custom_label.is_empty() ? godot::String(property_title) : custom_label;
            int delta = get_net_modifier_delta();
            godot::String val_str = godot::String::num_int64(_snapshot.modded_value);
            if (delta > 0) {
                val_str += " (+" + godot::String::num_int64(delta) + ")";
            } else if (delta < 0) {
                val_str += " (" + godot::String::num_int64(delta) + ")";
            }

            godot::Color text_col(0.94f, 0.96f, 0.98f, 1.0f);
            if (_snapshot.is_exhausted) {
                float pulse = 0.5f + 0.5f * std::sin(_exhausted_pulse_phase);
                text_col = debuff_col.lerp(godot::Color(1.0f, 0.9f, 0.9f, 1.0f), pulse);
            } else if (delta > 0) {
                text_col = buff_col.lightened(0.25f);
            } else if (delta < 0) {
                text_col = debuff_col.lightened(0.25f);
            }

            draw_string(
                font,
                godot::Vector2(p_dial_rect.position.x, center.y - 6.0f),
                label_str,
                godot::HORIZONTAL_ALIGNMENT_CENTER,
                p_dial_rect.size.x,
                11,
                godot::Color(0.75f, 0.80f, 0.86f, 0.90f)
            );
            draw_string(
                font,
                godot::Vector2(p_dial_rect.position.x, center.y + 10.0f),
                val_str,
                godot::HORIZONTAL_ALIGNMENT_CENTER,
                p_dial_rect.size.x,
                13,
                text_col
            );

            if (_snapshot.is_locked || _snapshot.is_exhausted) {
                godot::String badge = _snapshot.is_locked ? "LOCKED" : "EXHAUSTED";
                draw_string(
                    font,
                    godot::Vector2(p_dial_rect.position.x, center.y + 24.0f),
                    badge,
                    godot::HORIZONTAL_ALIGNMENT_CENTER,
                    p_dial_rect.size.x,
                    9,
                    _snapshot.is_locked ? timed_col : debuff_col
                );
            }
        }
    }
}

void GamePropertyGauge::_draw_linear_bar(const godot::Rect2 &p_bar_rect, bool p_compact_labels) {
    godot::Color bg_col = (custom_bg_color.a > 0.0f) ? custom_bg_color : godot::Color(0.11f, 0.13f, 0.16f, 0.90f);
    godot::Color base_col = (custom_base_color.a > 0.0f) ? custom_base_color : godot::Color(0.18f, 0.68f, 0.92f, 1.0f);
    godot::Color buff_col = (custom_buff_color.a > 0.0f) ? custom_buff_color : godot::Color(0.22f, 0.88f, 0.48f, 0.95f);
    godot::Color debuff_col = (custom_debuff_color.a > 0.0f) ? custom_debuff_color : godot::Color(0.95f, 0.30f, 0.30f, 0.95f);
    godot::Color timed_col = (custom_timed_ring_color.a > 0.0f) ? custom_timed_ring_color : godot::Color(0.98f, 0.80f, 0.22f, 0.95f);

    float header_h = show_numeric_text ? 18.0f : 2.0f;
    float timed_area_h = (show_modifier_rings && !_timed_modifiers.empty()) ? 10.0f : 0.0f;
    float bar_h = godot::Math::clamp(p_bar_rect.size.y - header_h - timed_area_h - 4.0f, 8.0f, 24.0f);
    godot::Rect2 track_rect(
        godot::Vector2(p_bar_rect.position.x, p_bar_rect.position.y + header_h),
        godot::Vector2(p_bar_rect.size.x, bar_h)
    );

    // 1. Background Track
    draw_rect(track_rect, bg_col, true);

    float span_val = godot::Math::max(1.0f, _visual_max_value - _visual_min_value);
    float base_ratio = godot::Math::clamp((_visual_base_value - _visual_min_value) / span_val, 0.0f, 1.0f);
    float mod_ratio = godot::Math::clamp((_visual_modded_value - _visual_min_value) / span_val, 0.0f, 1.0f);

    float min_r = godot::Math::min(base_ratio, mod_ratio);
    float max_r = godot::Math::max(base_ratio, mod_ratio);

    // 2. Authoritative Base Fill
    if (min_r > 0.0f) {
        draw_rect(
            godot::Rect2(track_rect.position, godot::Vector2(track_rect.size.x * min_r, track_rect.size.y)),
            base_col,
            true
        );
    }

    // 3. Positive Buff / Negative Debuff Delta Fill
    if (max_r > min_r) {
        godot::Color delta_col = (mod_ratio >= base_ratio) ? buff_col : debuff_col;
        draw_rect(
            godot::Rect2(
                godot::Vector2(track_rect.position.x + track_rect.size.x * min_r, track_rect.position.y),
                godot::Vector2(track_rect.size.x * (max_r - min_r), track_rect.size.y)
            ),
            delta_col,
            true
        );
    }

    // 4. Segment Ticks & Border
    if (segment_count > 1) {
        for (int i = 1; i < segment_count; ++i) {
            float tx = track_rect.position.x + (track_rect.size.x * static_cast<float>(i) / static_cast<float>(segment_count));
            draw_line(
                godot::Vector2(tx, track_rect.position.y),
                godot::Vector2(tx, track_rect.position.y + track_rect.size.y),
                godot::Color(0.05f, 0.06f, 0.08f, 0.55f),
                1.0f
            );
        }
    }

    godot::Color border_col = _snapshot.is_exhausted ? debuff_col : godot::Color(0.28f, 0.32f, 0.38f, 0.90f);
    draw_rect(track_rect, border_col, false, 1.5f);

    // 5. Timed Modifier Countdown Strips Below Main Bar
    if (show_modifier_rings && !_timed_modifiers.empty()) {
        int count = godot::Math::min(static_cast<int>(_timed_modifiers.size()), 2);
        float strip_y = track_rect.position.y + track_rect.size.y + 3.0f;
        for (int i = 0; i < count; ++i) {
            const TimedModifierSlicePOD &slice = _timed_modifiers[i];
            godot::Rect2 strip_bg(godot::Vector2(track_rect.position.x, strip_y + i * 5.0f), godot::Vector2(track_rect.size.x, 3.5f));
            draw_rect(strip_bg, godot::Color(0.08f, 0.09f, 0.11f, 0.75f), true);
            godot::Color sc = (slice.value >= 0) ? timed_col : debuff_col;
            draw_rect(
                godot::Rect2(strip_bg.position, godot::Vector2(strip_bg.size.x * godot::Math::clamp(slice.remaining_ratio, 0.0f, 1.0f), strip_bg.size.y)),
                sc,
                true
            );
        }
    }

    // 6. Header Text (Title on Left, Modded / Max + Delta on Right)
    if (show_numeric_text) {
        godot::Ref<godot::Font> font = godot::ThemeDB::get_singleton()->get_fallback_font();
        if (font.is_valid()) {
            godot::String label_str = custom_label.is_empty() ? godot::String(property_title) : custom_label;
            int delta = get_net_modifier_delta();
            godot::String num_str = godot::String::num_int64(_snapshot.modded_value) + " / " + godot::String::num_int64(_snapshot.modded_max_value);
            if (delta > 0) {
                num_str += " (+" + godot::String::num_int64(delta) + ")";
            } else if (delta < 0) {
                num_str += " (" + godot::String::num_int64(delta) + ")";
            }

            draw_string(
                font,
                godot::Vector2(p_bar_rect.position.x, p_bar_rect.position.y + 13.0f),
                label_str,
                godot::HORIZONTAL_ALIGNMENT_LEFT,
                p_bar_rect.size.x * 0.5f,
                12,
                godot::Color(0.88f, 0.92f, 0.96f, 0.95f)
            );
            draw_string(
                font,
                godot::Vector2(p_bar_rect.position.x, p_bar_rect.position.y + 13.0f),
                num_str,
                godot::HORIZONTAL_ALIGNMENT_RIGHT,
                p_bar_rect.size.x,
                12,
                (delta > 0) ? buff_col : ((delta < 0) ? debuff_col : godot::Color(0.92f, 0.95f, 0.98f, 1.0f))
            );
        }
    }
}

} // namespace ideam::godot_ext