#include "cymatic_audio_compass.h"

#include <cmath>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/gradient.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/theme_db.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include "../../game_entities/game_agent.h"
#include "../../game_entities/game_board.h"
#include "../../game_entities/game_piece.h"
#include "../../game_property.h"

namespace ideam::godot_ext {

void CymaticAudioCompass::_bind_methods() {
    // --- Signals ---
    ADD_SIGNAL(godot::MethodInfo(
        "ai_hearing_alert_changed",
        godot::PropertyInfo(godot::Variant::BOOL, "alert_active"),
        godot::PropertyInfo(godot::Variant::FLOAT, "alert_ratio")
    ));
    ADD_SIGNAL(godot::MethodInfo(
        "acoustic_impulse_registered",
        godot::PropertyInfo(godot::Variant::INT, "sector_index"),
        godot::PropertyInfo(godot::Variant::FLOAT, "energy"),
        godot::PropertyInfo(godot::Variant::INT, "category")
    ));

    // --- Enums ---
    BIND_ENUM_CONSTANT(AudioCompassDisplayMode::COMPASS_RADIAL_ARCS);
    BIND_ENUM_CONSTANT(AudioCompassDisplayMode::COMPASS_HEATMAP_OVERLAY);
    BIND_ENUM_CONSTANT(AudioCompassDisplayMode::COMPASS_COMBINED_RING_AND_GRID);

    BIND_ENUM_CONSTANT(AcousticCueCategory::ACOUSTIC_CUE_AMBIENT);
    BIND_ENUM_CONSTANT(AcousticCueCategory::ACOUSTIC_CUE_ENEMY);
    BIND_ENUM_CONSTANT(AcousticCueCategory::ACOUSTIC_CUE_HAZARD);
    BIND_ENUM_CONSTANT(AcousticCueCategory::ACOUSTIC_CUE_INTERACTABLE);
    BIND_ENUM_CONSTANT(AcousticCueCategory::ACOUSTIC_CUE_VOICE);

    // --- Context & Resource Group ---
    ADD_GROUP("Context & Options", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_controlled_agent", "agent"), &CymaticAudioCompass::set_controlled_agent);
    godot::ClassDB::bind_method(godot::D_METHOD("get_controlled_agent"), &CymaticAudioCompass::get_controlled_agent);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::OBJECT, "controlled_agent", godot::PROPERTY_HINT_NODE_TYPE, "GameAgent"),
        "set_controlled_agent",
        "get_controlled_agent"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_active_board", "board"), &CymaticAudioCompass::set_active_board);
    godot::ClassDB::bind_method(godot::D_METHOD("get_active_board"), &CymaticAudioCompass::get_active_board);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::OBJECT, "active_board", godot::PROPERTY_HINT_NODE_TYPE, "GameBoard"),
        "set_active_board",
        "get_active_board"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_cymatic_map_node", "map_node"), &CymaticAudioCompass::set_cymatic_map_node);
    godot::ClassDB::bind_method(godot::D_METHOD("get_cymatic_map_node"), &CymaticAudioCompass::get_cymatic_map_node);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::OBJECT, "cymatic_map_node", godot::PROPERTY_HINT_NODE_TYPE, "Node"),
        "set_cymatic_map_node",
        "get_cymatic_map_node"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_audio_option", "audio_option"), &CymaticAudioCompass::set_audio_option);
    godot::ClassDB::bind_method(godot::D_METHOD("get_audio_option"), &CymaticAudioCompass::get_audio_option);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::OBJECT, "audio_option", godot::PROPERTY_HINT_RESOURCE_TYPE, "GameplayAudioOption"),
        "set_audio_option",
        "get_audio_option"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_accessibility_option", "accessibility_option"), &CymaticAudioCompass::set_accessibility_option);
    godot::ClassDB::bind_method(godot::D_METHOD("get_accessibility_option"), &CymaticAudioCompass::get_accessibility_option);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::OBJECT, "accessibility_option", godot::PROPERTY_HINT_RESOURCE_TYPE, "GameplayAccessibilityOption"),
        "set_accessibility_option",
        "get_accessibility_option"
    );

    // --- Compass & Cymatic Display Group ---
    ADD_GROUP("Compass & Cymatics", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_display_mode", "mode"), &CymaticAudioCompass::set_display_mode);
    godot::ClassDB::bind_method(godot::D_METHOD("get_display_mode"), &CymaticAudioCompass::get_display_mode);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::INT, "display_mode", godot::PROPERTY_HINT_ENUM, "Radial Arcs,Heatmap Overlay,Combined Ring & Grid"),
        "set_display_mode",
        "get_display_mode"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_respect_accessibility_toggle", "respect"), &CymaticAudioCompass::set_respect_accessibility_toggle);
    godot::ClassDB::bind_method(godot::D_METHOD("get_respect_accessibility_toggle"), &CymaticAudioCompass::get_respect_accessibility_toggle);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::BOOL, "respect_accessibility_toggle"),
        "set_respect_accessibility_toggle",
        "get_respect_accessibility_toggle"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_sector_count", "count"), &CymaticAudioCompass::set_sector_count);
    godot::ClassDB::bind_method(godot::D_METHOD("get_sector_count"), &CymaticAudioCompass::get_sector_count);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::INT, "sector_count", godot::PROPERTY_HINT_RANGE, "8,64,4"),
        "set_sector_count",
        "get_sector_count"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_heatmap_columns", "columns"), &CymaticAudioCompass::set_heatmap_columns);
    godot::ClassDB::bind_method(godot::D_METHOD("get_heatmap_columns"), &CymaticAudioCompass::get_heatmap_columns);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::INT, "heatmap_columns", godot::PROPERTY_HINT_RANGE, "4,32,2"),
        "set_heatmap_columns",
        "get_heatmap_columns"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_decay_rate", "rate"), &CymaticAudioCompass::set_decay_rate);
    godot::ClassDB::bind_method(godot::D_METHOD("get_decay_rate"), &CymaticAudioCompass::get_decay_rate);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "decay_rate", godot::PROPERTY_HINT_RANGE, "0.2,15.0,0.1"),
        "set_decay_rate",
        "get_decay_rate"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_max_detection_radius", "radius"), &CymaticAudioCompass::set_max_detection_radius);
    godot::ClassDB::bind_method(godot::D_METHOD("get_max_detection_radius"), &CymaticAudioCompass::get_max_detection_radius);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "max_detection_radius", godot::PROPERTY_HINT_RANGE, "1.0,250.0,1.0"),
        "set_max_detection_radius",
        "get_max_detection_radius"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_baseline_ai_hearing_range", "range"), &CymaticAudioCompass::set_baseline_ai_hearing_range);
    godot::ClassDB::bind_method(godot::D_METHOD("get_baseline_ai_hearing_range"), &CymaticAudioCompass::get_baseline_ai_hearing_range);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "baseline_ai_hearing_range", godot::PROPERTY_HINT_RANGE, "1.0,200.0,0.5"),
        "set_baseline_ai_hearing_range",
        "get_baseline_ai_hearing_range"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_player_acoustic_footprint", "footprint"), &CymaticAudioCompass::set_player_acoustic_footprint);
    godot::ClassDB::bind_method(godot::D_METHOD("get_player_acoustic_footprint"), &CymaticAudioCompass::get_player_acoustic_footprint);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "player_acoustic_footprint", godot::PROPERTY_HINT_RANGE, "0.0,2.0,0.05"),
        "set_player_acoustic_footprint",
        "get_player_acoustic_footprint"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_reference_yaw_degrees", "yaw"), &CymaticAudioCompass::set_reference_yaw_degrees);
    godot::ClassDB::bind_method(godot::D_METHOD("get_reference_yaw_degrees"), &CymaticAudioCompass::get_reference_yaw_degrees);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "reference_yaw_degrees", godot::PROPERTY_HINT_RANGE, "-360.0,360.0,1.0"),
        "set_reference_yaw_degrees",
        "get_reference_yaw_degrees"
    );

    // --- Methods ---
    godot::ClassDB::bind_method(godot::D_METHOD("get_compass_ring"), &CymaticAudioCompass::get_compass_ring);
    godot::ClassDB::bind_method(godot::D_METHOD("get_heatmap_grid"), &CymaticAudioCompass::get_heatmap_grid);

    godot::ClassDB::bind_method(
        godot::D_METHOD("push_directional_impulse", "angle_degrees", "energy", "category"),
        &CymaticAudioCompass::push_directional_impulse,
        DEFVAL(AcousticCueCategory::ACOUSTIC_CUE_AMBIENT)
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("push_world_acoustic_event", "source_pos", "listener_pos", "listener_yaw_deg", "energy", "category"),
        &CymaticAudioCompass::push_world_acoustic_event,
        DEFVAL(AcousticCueCategory::ACOUSTIC_CUE_AMBIENT)
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("set_sector_energies", "energies", "category"),
        &CymaticAudioCompass::set_sector_energies,
        DEFVAL(0)
    );
    godot::ClassDB::bind_method(godot::D_METHOD("get_sector_energies"), &CymaticAudioCompass::get_sector_energies);
    godot::ClassDB::bind_method(
        godot::D_METHOD("set_cymatic_grid_bytes", "bytes", "columns"),
        &CymaticAudioCompass::set_cymatic_grid_bytes,
        DEFVAL(12)
    );
    godot::ClassDB::bind_method(godot::D_METHOD("clear_telemetry"), &CymaticAudioCompass::clear_telemetry);

    godot::ClassDB::bind_method(godot::D_METHOD("get_effective_ai_hearing_range"), &CymaticAudioCompass::get_effective_ai_hearing_range);
    godot::ClassDB::bind_method(godot::D_METHOD("get_ai_alert_ratio"), &CymaticAudioCompass::get_ai_alert_ratio);
    godot::ClassDB::bind_method(godot::D_METHOD("is_ai_hearing_alert_active"), &CymaticAudioCompass::is_ai_hearing_alert_active);
}

CymaticAudioCompass::CymaticAudioCompass() {
    set_clip_contents(false);
    set_mouse_filter(MOUSE_FILTER_PASS);
    set_process(true);
    _resize_sectors(sector_count);
}

CymaticAudioCompass::~CymaticAudioCompass() {
}

void CymaticAudioCompass::_ready() {
    _ensure_child_controls();
    _sync_child_controls_layout();
    queue_redraw();
}

void CymaticAudioCompass::_process(double p_delta) {
    if (respect_accessibility_toggle && accessibility_option.is_valid()) {
        bool should_show = accessibility_option->get_visual_sound_indicators();
        if (is_visible() != should_show) {
            set_visible(should_show);
        }
        if (!should_show) {
            return;
        }
    }

    float dt = static_cast<float>(p_delta);
    bool any_active = false;

    for (AcousticSectorPOD &sec : _sectors) {
        if (sec.energy > 0.001f || sec.peak_hold > 0.001f) {
            sec.energy = godot::Math::max(0.0f, sec.energy - decay_rate * dt);
            sec.peak_hold = godot::Math::max(sec.energy, sec.peak_hold - (decay_rate * 0.45f) * dt);
            any_active = true;
        }
    }

    _poll_board_and_cymatic_telemetry();

    if (any_active) {
        queue_redraw();
    }
}

godot::Vector2 CymaticAudioCompass::_get_minimum_size() const {
    return godot::Vector2(140.0f, 140.0f);
}

// --- Target & Resource Setters / Getters ---

void CymaticAudioCompass::set_controlled_agent(GameAgent *p_agent) {
    _controlled_agent_id = p_agent ? godot::ObjectID(p_agent->get_instance_id()) : godot::ObjectID();
    queue_redraw();
}

GameAgent *CymaticAudioCompass::get_controlled_agent() const {
    if (!_controlled_agent_id.is_valid()) return nullptr;
    return godot::Object::cast_to<GameAgent>(godot::ObjectDB::get_instance(_controlled_agent_id));
}

void CymaticAudioCompass::set_active_board(GameBoard *p_board) {
    _active_board_id = p_board ? godot::ObjectID(p_board->get_instance_id()) : godot::ObjectID();
    queue_redraw();
}

GameBoard *CymaticAudioCompass::get_active_board() const {
    if (!_active_board_id.is_valid()) return nullptr;
    return godot::Object::cast_to<GameBoard>(godot::ObjectDB::get_instance(_active_board_id));
}

void CymaticAudioCompass::set_cymatic_map_node(godot::Node *p_map_node) {
    _cymatic_map_id = p_map_node ? godot::ObjectID(p_map_node->get_instance_id()) : godot::ObjectID();
    queue_redraw();
}

godot::Node *CymaticAudioCompass::get_cymatic_map_node() const {
    if (!_cymatic_map_id.is_valid()) return nullptr;
    return godot::Object::cast_to<godot::Node>(godot::ObjectDB::get_instance(_cymatic_map_id));
}

void CymaticAudioCompass::set_audio_option(const godot::Ref<GameplayAudioOption> &p_opt) {
    if (audio_option == p_opt) return;
    audio_option = p_opt;
    queue_redraw();
}

godot::Ref<GameplayAudioOption> CymaticAudioCompass::get_audio_option() const {
    return audio_option;
}

void CymaticAudioCompass::set_accessibility_option(const godot::Ref<GameplayAccessibilityOption> &p_opt) {
    if (accessibility_option == p_opt) return;
    accessibility_option = p_opt;
    queue_redraw();
}

godot::Ref<GameplayAccessibilityOption> CymaticAudioCompass::get_accessibility_option() const {
    return accessibility_option;
}

// --- Display & Acoustic Parameters Setters / Getters ---

void CymaticAudioCompass::set_display_mode(AudioCompassDisplayMode p_mode) {
    if (display_mode == p_mode) return;
    display_mode = p_mode;
    _ensure_child_controls();
    _sync_child_controls_layout();
    queue_redraw();
}

AudioCompassDisplayMode CymaticAudioCompass::get_display_mode() const {
    return display_mode;
}

void CymaticAudioCompass::set_respect_accessibility_toggle(bool p_respect) {
    respect_accessibility_toggle = p_respect;
}

bool CymaticAudioCompass::get_respect_accessibility_toggle() const {
    return respect_accessibility_toggle;
}

void CymaticAudioCompass::set_sector_count(int p_count) {
    int clamped = godot::Math::clamp(p_count, 8, 64);
    if (sector_count == clamped) return;
    sector_count = clamped;
    _resize_sectors(sector_count);
    _sync_child_controls_layout();
    queue_redraw();
}

int CymaticAudioCompass::get_sector_count() const {
    return sector_count;
}

void CymaticAudioCompass::set_heatmap_columns(int p_columns) {
    int clamped = godot::Math::clamp(p_columns, 4, 32);
    if (heatmap_columns == clamped) return;
    heatmap_columns = clamped;
    _sync_child_controls_layout();
    queue_redraw();
}

int CymaticAudioCompass::get_heatmap_columns() const {
    return heatmap_columns;
}

void CymaticAudioCompass::set_decay_rate(float p_rate) {
    decay_rate = godot::Math::clamp(p_rate, 0.1f, 30.0f);
}

float CymaticAudioCompass::get_decay_rate() const {
    return decay_rate;
}

void CymaticAudioCompass::set_max_detection_radius(float p_radius) {
    max_detection_radius = godot::Math::max(1.0f, p_radius);
    queue_redraw();
}

float CymaticAudioCompass::get_max_detection_radius() const {
    return max_detection_radius;
}

void CymaticAudioCompass::set_baseline_ai_hearing_range(float p_range) {
    baseline_ai_hearing_range = godot::Math::max(0.5f, p_range);
    queue_redraw();
}

float CymaticAudioCompass::get_baseline_ai_hearing_range() const {
    return baseline_ai_hearing_range;
}

void CymaticAudioCompass::set_player_acoustic_footprint(float p_footprint) {
    player_acoustic_footprint = godot::Math::clamp(p_footprint, 0.0f, 5.0f);
    queue_redraw();
}

float CymaticAudioCompass::get_player_acoustic_footprint() const {
    return player_acoustic_footprint;
}

void CymaticAudioCompass::set_reference_yaw_degrees(float p_yaw) {
    reference_yaw_degrees = p_yaw;
    queue_redraw();
}

float CymaticAudioCompass::get_reference_yaw_degrees() const {
    return reference_yaw_degrees;
}

CircularGauge *CymaticAudioCompass::get_compass_ring() const {
    return _compass_ring;
}

HeatmapGrid *CymaticAudioCompass::get_heatmap_grid() const {
    return _heatmap_grid;
}

// --- Internal Composition & Telemetry Polling ---

void CymaticAudioCompass::_resize_sectors(int p_count) {
    _sectors.assign(p_count, AcousticSectorPOD{});
}

void CymaticAudioCompass::_ensure_child_controls() {
    if (!_compass_ring) {
        _compass_ring = memnew(CircularGauge);
        _compass_ring->set_name("CompassRingGauge");
        _compass_ring->set_mouse_filter(MOUSE_FILTER_IGNORE);
        _compass_ring->set_start_angle(0.0f);
        _compass_ring->set_end_angle(360.0f);
        _compass_ring->set_min_value(0.0f);
        _compass_ring->set_max_value(1.0f);
        _compass_ring->set_marker_count(1);
        _compass_ring->set_draw_head_tail_markers(false);
        add_child(_compass_ring);
    }

    if (!_heatmap_grid) {
        _heatmap_grid = memnew(HeatmapGrid);
        _heatmap_grid->set_name("CymaticHeatmapGrid");
        _heatmap_grid->set_mouse_filter(MOUSE_FILTER_IGNORE);
        _heatmap_grid->set_columns(heatmap_columns);
        _heatmap_grid->set_square_cells(true);
        _heatmap_grid->resize_data(heatmap_columns * heatmap_columns);

        godot::Ref<godot::Gradient> grad;
        grad.instantiate();
        grad->set_color(0, godot::Color(0.06f, 0.08f, 0.12f, 0.35f));
        grad->set_color(1, godot::Color(0.15f, 0.85f, 0.98f, 0.90f));
        _heatmap_grid->set_gradient(grad);
        add_child(_heatmap_grid);
    }
}

void CymaticAudioCompass::_sync_child_controls_layout() {
    if (!_compass_ring || !_heatmap_grid) {
        return;
    }

    godot::Vector2 sz = get_size();
    godot::Vector2 center = sz * 0.5f;
    float min_side = godot::Math::min(sz.x, sz.y);

    bool show_ring = (display_mode != AudioCompassDisplayMode::COMPASS_HEATMAP_OVERLAY);
    _compass_ring->set_visible(show_ring);
    _compass_ring->set_position(godot::Vector2(0.0f, 0.0f));
    _compass_ring->set_size(sz);
    _compass_ring->set_slot_count(sector_count);
    _compass_ring->set_draw_slot_ticks(true);
    _compass_ring->set_value_1(godot::Math::clamp(_ai_alert_ratio, 0.0f, 1.0f));
    _compass_ring->set_is_overflowing(_ai_alert_ratio >= 1.0f);
    _compass_ring->set_custom_bg_color(godot::Color(0.08f, 0.10f, 0.14f, 0.65f));
    _compass_ring->set_custom_fill_color(
        (_ai_alert_ratio >= 1.0f)
            ? _resolve_category_color(ACOUSTIC_CUE_ENEMY)
            : godot::Color(0.16f, 0.72f, 0.92f, 0.45f)
    );

    bool show_grid = (display_mode == AudioCompassDisplayMode::COMPASS_HEATMAP_OVERLAY ||
                      display_mode == AudioCompassDisplayMode::COMPASS_COMBINED_RING_AND_GRID);
    _heatmap_grid->set_visible(show_grid);
    _heatmap_grid->set_columns(heatmap_columns);
    if (_heatmap_grid->get_data().size() != heatmap_columns * heatmap_columns) {
        _heatmap_grid->resize_data(heatmap_columns * heatmap_columns);
    }

    if (display_mode == AudioCompassDisplayMode::COMPASS_HEATMAP_OVERLAY) {
        float grid_side = godot::Math::max(24.0f, min_side - 16.0f);
        _heatmap_grid->set_position(center - godot::Vector2(grid_side * 0.5f, grid_side * 0.5f));
        _heatmap_grid->set_size(godot::Vector2(grid_side, grid_side));
    } else {
        float inner_grid_side = godot::Math::max(20.0f, (min_side - 68.0f) * 0.70f);
        _heatmap_grid->set_position(center - godot::Vector2(inner_grid_side * 0.5f, inner_grid_side * 0.5f));
        _heatmap_grid->set_size(godot::Vector2(inner_grid_side, inner_grid_side));
    }
}

float CymaticAudioCompass::_get_effective_visual_gain() const {
    float gain = 1.0f;
    if (accessibility_option.is_valid()) {
        gain *= accessibility_option->get_cymatic_visual_gain();
    }
    if (audio_option.is_valid()) {
        if (audio_option->get_master_muted()) {
            gain *= 0.25f;
        }
    }
    return godot::Math::clamp(gain, 0.0f, 5.0f);
}

godot::Color CymaticAudioCompass::_resolve_category_color(int32_t p_category) const {
    if (accessibility_option.is_valid()) {
        switch (p_category) {
            case ACOUSTIC_CUE_ENEMY:
                return accessibility_option->get_enemy_highlight_color();
            case ACOUSTIC_CUE_HAZARD:
                return accessibility_option->get_hazard_highlight_color();
            case ACOUSTIC_CUE_INTERACTABLE:
                return accessibility_option->get_interactable_highlight_color();
            default:
                break;
        }
    }

    switch (p_category) {
        case ACOUSTIC_CUE_ENEMY:
            return godot::Color(0.96f, 0.26f, 0.26f, 0.95f);
        case ACOUSTIC_CUE_HAZARD:
            return godot::Color(0.98f, 0.82f, 0.16f, 0.95f);
        case ACOUSTIC_CUE_INTERACTABLE:
            return godot::Color(0.20f, 0.90f, 0.98f, 0.95f);
        case ACOUSTIC_CUE_VOICE:
            return godot::Color(0.62f, 0.42f, 0.98f, 0.95f);
        default:
            return godot::Color(0.24f, 0.82f, 0.62f, 0.90f);
    }
}

void CymaticAudioCompass::_poll_board_and_cymatic_telemetry() {
    float sensitivity_mult = 1.0f;
    godot::StringName enemy_tag("RedRobot");
    if (audio_option.is_valid()) {
        sensitivity_mult = audio_option->get_ai_hearing_sensitivity_multiplier();
        if (!audio_option->get_enemy_group_tag().is_empty()) {
            enemy_tag = audio_option->get_enemy_group_tag();
        }
    }

    _effective_ai_hearing_range = baseline_ai_hearing_range * sensitivity_mult;
    _nearest_enemy_distance = -1.0f;

    // 1. Poll optional CymaticMap node if bound and exposing packed field bytes
    if (godot::Node *cmap = get_cymatic_map_node()) {
        if (cmap->has_method("get_cymatic_field_bytes")) {
            godot::Variant v = cmap->call("get_cymatic_field_bytes");
            if (v.get_type() == godot::Variant::PACKED_BYTE_ARRAY && _heatmap_grid) {
                _heatmap_grid->set_data(v);
            }
        }
    }

    // 2. Poll active GameBoard for enemy GameAgents & their HearingRange GameProperty
    GameBoard *board = get_active_board();
    GameAgent *player_agent = get_controlled_agent();
    if (board) {
        godot::Vector3 listener_pos(0.0f, 0.0f, 0.0f);
        bool has_listener_3d = false;
        if (player_agent) {
            godot::Node *p_root = player_agent->get_root_node() ? player_agent->get_root_node() : player_agent;
            if (godot::Node3D *n3d = godot::Object::cast_to<godot::Node3D>(p_root)) {
                listener_pos = n3d->get_global_position();
                reference_yaw_degrees = n3d->get_global_rotation_degrees().y;
                has_listener_3d = true;
            }
        }

        godot::TypedArray<GameAgent> agents = board->get_game_agents();
        for (int i = 0; i < agents.size(); ++i) {
            GameAgent *other = godot::Object::cast_to<GameAgent>(agents[i]);
            if (!other || other == player_agent || !other->get_enabled()) continue;

            bool is_enemy = other->is_in_group(enemy_tag) || (other->get_title() == godot::String(enemy_tag));
            float agent_hearing = _effective_ai_hearing_range;

            int child_count = other->get_child_count();
            for (int c = 0; c < child_count; ++c) {
                if (GameProperty *gp = godot::Object::cast_to<GameProperty>(other->get_child(c))) {
                    if (gp->get_title() == "HearingRange" || gp->get_name() == godot::StringName("HearingRange")) {
                        agent_hearing = static_cast<float>(gp->get_modded_value());
                        is_enemy = true;
                        break;
                    }
                }
            }

            if (!is_enemy) continue;
            _effective_ai_hearing_range = godot::Math::max(_effective_ai_hearing_range, agent_hearing);

            if (has_listener_3d) {
                godot::Node *o_root = other->get_root_node() ? other->get_root_node() : other;
                if (godot::Node3D *o3d = godot::Object::cast_to<godot::Node3D>(o_root)) {
                    godot::Vector3 o_pos = o3d->get_global_position();
                    float dist = listener_pos.distance_to(o_pos);
                    if (_nearest_enemy_distance < 0.0f || dist < _nearest_enemy_distance) {
                        _nearest_enemy_distance = dist;
                    }
                    if (dist <= max_detection_radius) {
                        float atten = godot::Math::clamp(1.0f - (dist / max_detection_radius), 0.1f, 1.0f);
                        push_world_acoustic_event(o_pos, listener_pos, reference_yaw_degrees, atten * 0.65f, ACOUSTIC_CUE_ENEMY);
                    }
                }
            }
        }
    }

    // 3. Compute AI HearingRange alert ratio
    float prev_alert = _ai_alert_ratio;
    if (_nearest_enemy_distance >= 0.0f && _effective_ai_hearing_range > 0.01f) {
        float proximity_factor = godot::Math::clamp(1.0f - (_nearest_enemy_distance / _effective_ai_hearing_range), 0.0f, 1.5f);
        _ai_alert_ratio = proximity_factor * (0.5f + player_acoustic_footprint);
    } else {
        _ai_alert_ratio = player_acoustic_footprint * 0.5f;
    }

    bool was_alert = (prev_alert >= 1.0f);
    bool is_alert = (_ai_alert_ratio >= 1.0f);
    if (was_alert != is_alert) {
        emit_signal("ai_hearing_alert_changed", is_alert, _ai_alert_ratio);
    }
}

// --- Acoustic Event & DOD Snapshot Ingestion ---

void CymaticAudioCompass::push_directional_impulse(float p_angle_degrees, float p_energy, AcousticCueCategory p_category) {
    if (_sectors.empty()) {
        _resize_sectors(godot::Math::max(8, sector_count));
    }

    float norm_deg = std::fmod(p_angle_degrees - reference_yaw_degrees, 360.0f);
    if (norm_deg < 0.0f) {
        norm_deg += 360.0f;
    }

    int count = static_cast<int>(_sectors.size());
    int idx = static_cast<int>((norm_deg / 360.0f) * static_cast<float>(count)) % count;
    float scaled_energy = godot::Math::clamp(p_energy * _get_effective_visual_gain(), 0.0f, 1.0f);

    AcousticSectorPOD &sec = _sectors[idx];
    if (scaled_energy >= sec.energy) {
        sec.energy = scaled_energy;
        sec.dominant_category = static_cast<int32_t>(p_category);
    }
    if (scaled_energy > sec.peak_hold) {
        sec.peak_hold = scaled_energy;
    }

    if (_heatmap_grid && heatmap_columns >= 4) {
        float rad = godot::Math::deg_to_rad(norm_deg - 90.0f);
        float half = static_cast<float>(heatmap_columns - 1) * 0.5f;
        float r_cell = half * godot::Math::clamp(1.0f - scaled_energy * 0.35f, 0.25f, 0.95f);
        int cx = godot::Math::clamp(static_cast<int>(std::round(half + std::cos(rad) * r_cell)), 0, heatmap_columns - 1);
        int cy = godot::Math::clamp(static_cast<int>(std::round(half + std::sin(rad) * r_cell)), 0, heatmap_columns - 1);
        int cell_idx = cy * heatmap_columns + cx;
        uint8_t byte_val = static_cast<uint8_t>(godot::Math::clamp(scaled_energy * 255.0f, 0.0f, 255.0f));
        if (byte_val > _heatmap_grid->get_value(cell_idx)) {
            _heatmap_grid->set_value(cell_idx, byte_val);
        }
    }

    emit_signal("acoustic_impulse_registered", idx, scaled_energy, static_cast<int>(p_category));
    queue_redraw();
}

void CymaticAudioCompass::push_world_acoustic_event(
    const godot::Vector3 &p_source_pos,
    const godot::Vector3 &p_listener_pos,
    float p_listener_yaw_deg,
    float p_energy,
    AcousticCueCategory p_category
) {
    godot::Vector3 delta = p_source_pos - p_listener_pos;
    float dist_xz = std::sqrt(delta.x * delta.x + delta.z * delta.z);
    if (dist_xz > max_detection_radius) {
        return;
    }

    float world_angle_deg = godot::Math::rad_to_deg(std::atan2(delta.x, -delta.z));
    float dist_atten = godot::Math::clamp(1.0f - (dist_xz / godot::Math::max(1.0f, max_detection_radius)), 0.05f, 1.0f);
    reference_yaw_degrees = p_listener_yaw_deg;
    push_directional_impulse(world_angle_deg, p_energy * dist_atten, p_category);
}

void CymaticAudioCompass::set_sector_energies(const godot::PackedFloat32Array &p_energies, int p_category) {
    if (p_energies.is_empty()) {
        return;
    }
    if (static_cast<int>(_sectors.size()) != p_energies.size()) {
        sector_count = godot::Math::clamp(static_cast<int>(p_energies.size()), 8, 64);
        _resize_sectors(sector_count);
    }
    float gain = _get_effective_visual_gain();
    for (int i = 0; i < static_cast<int>(_sectors.size()) && i < p_energies.size(); ++i) {
        float e = godot::Math::clamp(p_energies[i] * gain, 0.0f, 1.0f);
        _sectors[i].energy = e;
        _sectors[i].peak_hold = godot::Math::max(_sectors[i].peak_hold, e);
        _sectors[i].dominant_category = p_category;
    }
    queue_redraw();
}

godot::PackedFloat32Array CymaticAudioCompass::get_sector_energies() const {
    godot::PackedFloat32Array out;
    out.resize(static_cast<int>(_sectors.size()));
    for (int i = 0; i < static_cast<int>(_sectors.size()); ++i) {
        out.set(i, _sectors[i].energy);
    }
    return out;
}

void CymaticAudioCompass::set_cymatic_grid_bytes(const godot::PackedByteArray &p_bytes, int p_columns) {
    heatmap_columns = godot::Math::clamp(p_columns, 4, 32);
    _ensure_child_controls();
    if (_heatmap_grid) {
        _heatmap_grid->set_columns(heatmap_columns);
        _heatmap_grid->set_data(p_bytes);
    }
    _sync_child_controls_layout();
    queue_redraw();
}

void CymaticAudioCompass::clear_telemetry() {
    for (AcousticSectorPOD &sec : _sectors) {
        sec.energy = 0.0f;
        sec.peak_hold = 0.0f;
        sec.dominant_category = ACOUSTIC_CUE_AMBIENT;
    }
    if (_heatmap_grid) {
        _heatmap_grid->clear_data();
    }
    queue_redraw();
}

float CymaticAudioCompass::get_effective_ai_hearing_range() const {
    return _effective_ai_hearing_range;
}

float CymaticAudioCompass::get_ai_alert_ratio() const {
    return _ai_alert_ratio;
}

bool CymaticAudioCompass::is_ai_hearing_alert_active() const {
    return _ai_alert_ratio >= 1.0f;
}

// --- Custom Drawing ---

void CymaticAudioCompass::_notification(int p_what) {
    if (p_what == NOTIFICATION_RESIZED || p_what == NOTIFICATION_THEME_CHANGED) {
        _sync_child_controls_layout();
        queue_redraw();
    } else if (p_what == NOTIFICATION_DRAW) {
        godot::Vector2 sz = get_size();
        if (sz.x <= 4.0f || sz.y <= 4.0f) {
            return;
        }

        godot::Vector2 center = sz * 0.5f;
        float max_r = godot::Math::min(sz.x, sz.y) * 0.5f;
        float outer_r = godot::Math::max(max_r - 14.0f, 16.0f);
        float inner_r = godot::Math::max(outer_r - 24.0f, 10.0f);

        if (display_mode != AudioCompassDisplayMode::COMPASS_HEATMAP_OVERLAY) {
            _draw_directional_sectors(center, inner_r, outer_r);
            _draw_hearing_threshold_ring(center, inner_r - 3.0f);
        } else {
            _draw_hearing_threshold_ring(center, outer_r);
        }

        _draw_cardinal_markers(center, outer_r + 2.0f);
    }
}

void CymaticAudioCompass::_draw_arc_segment(
    const godot::Vector2 &p_center,
    float p_inner_r,
    float p_outer_r,
    float p_start_rad,
    float p_end_rad,
    const godot::Color &p_color
) {
    if (p_outer_r <= p_inner_r || godot::Math::is_equal_approx(p_start_rad, p_end_rad)) {
        return;
    }

    int steps = 8;
    godot::PackedVector2Array poly;
    poly.resize((steps + 1) * 2);

    for (int i = 0; i <= steps; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(steps);
        float ang = godot::Math::lerp(p_start_rad, p_end_rad, t);
        godot::Vector2 dir(std::cos(ang), std::sin(ang));
        poly.set(i, p_center + dir * p_outer_r);
        poly.set((steps * 2) + 1 - i, p_center + dir * p_inner_r);
    }

    godot::PackedColorArray colors;
    colors.push_back(p_color);
    draw_polygon(poly, colors);
}

void CymaticAudioCompass::_draw_directional_sectors(const godot::Vector2 &p_center, float p_inner_r, float p_outer_r) {
    int count = static_cast<int>(_sectors.size());
    if (count <= 0) {
        return;
    }

    float step_deg = 360.0f / static_cast<float>(count);
    float pad_deg = godot::Math::min(2.5f, step_deg * 0.12f);
    float radial_span = p_outer_r - p_inner_r;

    for (int i = 0; i < count; ++i) {
        const AcousticSectorPOD &sec = _sectors[i];
        if (sec.energy <= 0.01f && sec.peak_hold <= 0.01f) {
            continue;
        }

        float deg0 = (static_cast<float>(i) * step_deg) - 90.0f + pad_deg * 0.5f;
        float deg1 = ((static_cast<float>(i) + 1.0f) * step_deg) - 90.0f - pad_deg * 0.5f;
        float rad0 = godot::Math::deg_to_rad(deg0);
        float rad1 = godot::Math::deg_to_rad(deg1);

        godot::Color col = _resolve_category_color(sec.dominant_category);
        if (sec.energy > 0.01f) {
            float fill_r = p_inner_r + radial_span * godot::Math::clamp(sec.energy, 0.08f, 1.0f);
            godot::Color fill_col = col;
            fill_col.a = godot::Math::clamp(0.35f + 0.60f * sec.energy, 0.35f, 0.95f);
            _draw_arc_segment(p_center, p_inner_r, fill_r, rad0, rad1, fill_col);
        }

        if (sec.peak_hold > 0.05f) {
            float peak_r = p_inner_r + radial_span * godot::Math::clamp(sec.peak_hold, 0.08f, 1.0f);
            draw_arc(p_center, peak_r, rad0, rad1, 8, col.lightened(0.25f), 2.0f);
        }
    }
}

void CymaticAudioCompass::_draw_hearing_threshold_ring(const godot::Vector2 &p_center, float p_radius) {
    if (p_radius <= 6.0f) {
        return;
    }

    float ratio = godot::Math::clamp(_effective_ai_hearing_range / godot::Math::max(1.0f, max_detection_radius), 0.15f, 1.0f);
    float ring_r = p_radius * ratio;
    bool alert = is_ai_hearing_alert_active();

    godot::Color ring_col = alert
        ? _resolve_category_color(ACOUSTIC_CUE_ENEMY)
        : godot::Color(0.95f, 0.78f, 0.22f, 0.65f);

    draw_arc(p_center, ring_r, 0.0f, static_cast<float>(godot::Math::TAU), 48, ring_col, alert ? 2.2f : 1.25f);
}

void CymaticAudioCompass::_draw_cardinal_markers(const godot::Vector2 &p_center, float p_outer_r) {
    godot::Ref<godot::Font> font = godot::ThemeDB::get_singleton()->get_fallback_font();
    if (!font.is_valid()) {
        return;
    }

    godot::Vector2 top_pt(p_center.x, p_center.y - p_outer_r - 2.0f);
    godot::PackedVector2Array tri;
    tri.push_back(top_pt + godot::Vector2(0.0f, -6.0f));
    tri.push_back(top_pt + godot::Vector2(-5.0f, 2.0f));
    tri.push_back(top_pt + godot::Vector2(5.0f, 2.0f));
    godot::PackedColorArray colors;
    colors.push_back(godot::Color(0.95f, 0.97f, 1.0f, 0.90f));
    draw_polygon(tri, colors);

    godot::String footer = "AI Hear: " + godot::String::num(_effective_ai_hearing_range, 1) + "m";
    draw_string(
        font,
        godot::Vector2(0.0f, get_size().y - 3.0f),
        footer,
        godot::HORIZONTAL_ALIGNMENT_CENTER,
        get_size().x,
        10,
        is_ai_hearing_alert_active()
            ? _resolve_category_color(ACOUSTIC_CUE_ENEMY)
            : godot::Color(0.75f, 0.82f, 0.88f, 0.85f)
    );
}

} // namespace ideam::godot_ext