#pragma once

/**
 * spline_tweener.h
 * General-Purpose Multi-Track Spline Animation & BlendShape Tweening System
 * (`SplineTweenTrack`, `SplineTweenResource`, `SplineTweener`, `SplineTweenerControl`,
 * and `SplineTweenResourceInspector`) in `src/godot/splines/ui/`.
 *
 * Designed from the user's specification in Section 3.3:
 *   1. Each `SplineTweenTrack` wraps a `Spline1DResource` (edited via an embedded `Spline1DControl`)
 *      where the spline points map to that track's weight/value and the section weights between
 *      each spline point map to the `0.0 - 1.0` completion percentage of the whole tween animation.
 *   2. Supports 5 general-purpose tween track modes (`SplineTweenTrackMode`):
 *      - `TWEEN_TRACK_BLEND_SHAPE`: Drives a `MeshInstance3D` blend shape by name (`find_blend_shape_by_name`
 *        / `set_blend_shape_value`), allowing complex multi-blendshape facial/creature animations.
 *      - `TWEEN_TRACK_PROPERTY_LERP`: Tween-interpolates any Godot `Object` property (`float`, `Vector2`,
 *        `Vector3`, `Vector4`, `Color`, `Quaternion`, `Transform3D`) between `from_value` and `to_value`
 *        using the sampled 1D spline weight (supporting overshoot when `w < 0` or `w > 1`).
 *      - `TWEEN_TRACK_SHADER_PARAMETER`: Drives a `ShaderMaterial` uniform parameter via
 *        `set_shader_parameter(target_key, value)` on a `GeometryInstance3D` or `CanvasItem`.
 *      - `TWEEN_TRACK_VECTOR3_OFFSET`: Evaluates a `Spline3DResource` trajectory modulated by the
 *        track's `Spline1DResource` timing curve and applies the 3D offset to a target `Node3D`.
 *      - `TWEEN_TRACK_COLOR_GRADIENT`: Samples a `Spline4DResource` (`ColorSplineControl` gradient)
 *        at the `Spline1DResource`-remapped time and writes the `Color` to a target property/shader.
 *   3. `SplineTweenerControl` renders a master `[0.0 .. 1.0]` timeline scrubber, preview transport,
 *      track management bar (`+ BlendShape Track`, `+ Property Track`, `+ Shader Track`, `Auto-Import
 *      Mesh BlendShapes`), and a stack of per-track `Spline1DControl` editors wired to `IdeamUndoRedo`.
 */

#include "../../geometry/editor/geometry_editor_inspector_base.h"
#include "../../geometry/geometry_resource.h"
#include "../spline_1d/spline_1d_control.h"
#include "../spline_1d/spline_1d_resource.h"
#include "../spline_1d/spline_node.h"
#include "../spline_3d/spline_3d_resource.h"
#include "../spline_4d/spline_4d_resource.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/h_slider.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/spin_box.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/variant.hpp>

#include <vector>

namespace ideam::godot_ext {

enum SplineTweenTrackMode : int32_t {
    TWEEN_TRACK_BLEND_SHAPE      = 0,
    TWEEN_TRACK_PROPERTY_LERP    = 1,
    TWEEN_TRACK_SHADER_PARAMETER = 2,
    TWEEN_TRACK_VECTOR3_OFFSET   = 3,
    TWEEN_TRACK_COLOR_GRADIENT   = 4
};

/**
 * SplineTweenTrack (`GeometryResource`)
 * Single spline-driven tween track (BlendShape, Property Lerp, Shader Uniform, 3D Offset, or Color Gradient).
 */
class SplineTweenTrack : public GeometryResource {
    GDCLASS(SplineTweenTrack, GeometryResource)

private:
    godot::String track_name_             = "Track";
    bool enabled_                         = true;
    SplineTweenTrackMode track_mode_      = TWEEN_TRACK_BLEND_SHAPE;
    godot::NodePath target_subpath_       = godot::NodePath(".");
    godot::StringName target_key_         = godot::StringName("");
    real_t weight_scale_                  = static_cast<real_t>(1.0);
    real_t weight_bias_                   = static_cast<real_t>(0.0);
    godot::Variant from_value_            = 0.0f;
    godot::Variant to_value_              = 1.0f;

    godot::Ref<Spline1DResource> curve_1d_;
    godot::Ref<Spline3DResource> offset_spline_3d_;
    godot::Ref<Spline4DResource> color_spline_4d_;

    void _on_sub_spline_changed();

protected:
    static void _bind_methods();

public:
    SplineTweenTrack();
    ~SplineTweenTrack() override;

    [[nodiscard]] godot::Dictionary get_snapshot_state() const override;
    void apply_snapshot_state(const godot::Dictionary &p_state) override;

    void set_track_name(const godot::String &p_name);
    [[nodiscard]] godot::String get_track_name() const { return track_name_; }

    void set_enabled(bool p_enabled);
    [[nodiscard]] bool is_enabled() const noexcept { return enabled_; }

    void set_track_mode(SplineTweenTrackMode p_mode);
    [[nodiscard]] SplineTweenTrackMode get_track_mode() const noexcept { return track_mode_; }

    void set_target_subpath(const godot::NodePath &p_path);
    [[nodiscard]] godot::NodePath get_target_subpath() const { return target_subpath_; }

    void set_target_key(const godot::StringName &p_key);
    [[nodiscard]] godot::StringName get_target_key() const { return target_key_; }

    void set_weight_scale(real_t p_scale);
    [[nodiscard]] real_t get_weight_scale() const noexcept { return weight_scale_; }

    void set_weight_bias(real_t p_bias);
    [[nodiscard]] real_t get_weight_bias() const noexcept { return weight_bias_; }

    void set_from_value(const godot::Variant &p_val);
    [[nodiscard]] godot::Variant get_from_value() const { return from_value_; }

    void set_to_value(const godot::Variant &p_val);
    [[nodiscard]] godot::Variant get_to_value() const { return to_value_; }

    void set_curve_1d(const godot::Ref<Spline1DResource> &p_curve);
    [[nodiscard]] godot::Ref<Spline1DResource> get_curve_1d() const { return curve_1d_; }

    void set_offset_spline_3d(const godot::Ref<Spline3DResource> &p_spline);
    [[nodiscard]] godot::Ref<Spline3DResource> get_offset_spline_3d() const { return offset_spline_3d_; }

    void set_color_spline_4d(const godot::Ref<Spline4DResource> &p_spline);
    [[nodiscard]] godot::Ref<Spline4DResource> get_color_spline_4d() const { return color_spline_4d_; }

    // --- Evaluation & Target Application ---
    [[nodiscard]] real_t sample_weight(real_t p_percent, bool p_weighted = true) const;
    [[nodiscard]] godot::Variant sample_evaluated_variant(real_t p_percent, bool p_weighted = true) const;
    void apply_to_root(godot::Node *p_root, real_t p_percent, bool p_weighted = true) const;
};

/**
 * SplineTweenResource (`GeometryResource`)
 * Multi-track spline tween asset storing an ordered list of `SplineTweenTrack` tracks,
 * duration, loop mode, and Undo/Redo `commit_*` track authoring methods.
 */
class SplineTweenResource : public GeometryResource {
    GDCLASS(SplineTweenResource, GeometryResource)

private:
    std::vector<godot::Ref<SplineTweenTrack>> tracks_;
    real_t duration_          = static_cast<real_t>(1.0);
    SplineLoopMode loop_mode_ = SPLINE_LOOP_WRAP;
    bool use_weighted_time_   = true;

    void _on_track_changed();
    void _connect_track(const godot::Ref<SplineTweenTrack> &p_track);
    void _disconnect_track(const godot::Ref<SplineTweenTrack> &p_track);

protected:
    static void _bind_methods();

public:
    SplineTweenResource() = default;
    ~SplineTweenResource() override;

    [[nodiscard]] godot::Dictionary get_snapshot_state() const override;
    void apply_snapshot_state(const godot::Dictionary &p_state) override;

    void set_tracks(const godot::TypedArray<SplineTweenTrack> &p_tracks);
    [[nodiscard]] godot::TypedArray<SplineTweenTrack> get_tracks() const;

    [[nodiscard]] int get_track_count() const noexcept { return static_cast<int>(tracks_.size()); }
    [[nodiscard]] godot::Ref<SplineTweenTrack> get_track(int p_index) const;

    void add_track(const godot::Ref<SplineTweenTrack> &p_track);
    godot::Ref<SplineTweenTrack> create_and_add_track(
        const godot::String &p_name,
        SplineTweenTrackMode p_mode = TWEEN_TRACK_BLEND_SHAPE,
        const godot::StringName &p_target_key = godot::StringName("")
    );
    void remove_track(int p_index);
    void clear_tracks();

    void set_duration(real_t p_duration);
    [[nodiscard]] real_t get_duration() const noexcept { return duration_; }

    void set_loop_mode(SplineLoopMode p_mode);
    [[nodiscard]] SplineLoopMode get_loop_mode() const noexcept { return loop_mode_; }

    void set_use_weighted_time(bool p_weighted);
    [[nodiscard]] bool get_use_weighted_time() const noexcept { return use_weighted_time_; }

    void evaluate_and_apply(godot::Node *p_root, real_t p_percent) const;

    // --- Undo/Redo Commit Wrappers ---
    godot::Ref<SplineTweenTrack> commit_create_and_add_track(
        const godot::String &p_name,
        SplineTweenTrackMode p_mode = TWEEN_TRACK_BLEND_SHAPE,
        const godot::StringName &p_target_key = godot::StringName("")
    );
    void commit_remove_track(int p_index);
    void commit_clear_tracks();
    int commit_populate_blend_shapes_from_mesh(godot::MeshInstance3D *p_mesh_instance);
};

/**
 * SplineTweener (`godot::Node`)
 * SceneTree node that plays, loops, or scrubs a `SplineTweenResource` against a target
 * `Node` or `MeshInstance3D` (driving blend shapes, properties, shader uniforms, and 3D offsets).
 */
class SplineTweener : public godot::Node {
    GDCLASS(SplineTweener, godot::Node)

private:
    godot::Ref<SplineTweenResource> tween_resource_;
    godot::NodePath target_root_path_    = godot::NodePath("..");
    SplineProcessMode process_mode_type_ = SPLINE_PROCESS_IDLE;
    bool playing_                        = false;
    bool autoplay_                       = false;
    real_t speed_scale_                  = static_cast<real_t>(1.0);
    real_t current_percent_              = static_cast<real_t>(0.0);
    real_t ping_pong_dir_                = static_cast<real_t>(1.0);

    void _on_tween_resource_changed();
    void _update_process_callbacks();
    void _apply_current_percent();

protected:
    static void _bind_methods();

public:
    SplineTweener();
    ~SplineTweener() override;

    void _ready() override;
    void _process(double p_delta) override;
    void _physics_process(double p_delta) override;

    void set_tween_resource(const godot::Ref<SplineTweenResource> &p_tween);
    [[nodiscard]] godot::Ref<SplineTweenResource> get_tween_resource() const { return tween_resource_; }

    void set_target_root_path(const godot::NodePath &p_path);
    [[nodiscard]] godot::NodePath get_target_root_path() const { return target_root_path_; }

    void set_process_mode_type(SplineProcessMode p_mode);
    [[nodiscard]] SplineProcessMode get_process_mode_type() const noexcept { return process_mode_type_; }

    void set_playing(bool p_playing);
    [[nodiscard]] bool is_playing() const noexcept { return playing_; }

    void set_autoplay(bool p_autoplay);
    [[nodiscard]] bool get_autoplay() const noexcept { return autoplay_; }

    void set_speed_scale(real_t p_speed);
    [[nodiscard]] real_t get_speed_scale() const noexcept { return speed_scale_; }

    void set_current_percent(real_t p_percent);
    [[nodiscard]] real_t get_current_percent() const noexcept { return current_percent_; }

    void play(bool p_from_start = false);
    void pause();
    void stop(bool p_reset = true);
    void step(real_t p_delta_seconds);

    [[nodiscard]] godot::Node *resolve_target_root() const;
    int populate_blend_shape_tracks_from_target();
};

/**
 * SplineTweenerControl (`godot::VBoxContainer`, Tier 3 UI Tool)
 * Interactive Multi-Track Spline Tween & BlendShape Editor mounting a master `[0.0 .. 1.0]`
 * scrubber, track generator buttons, and a stack of per-track `Spline1DControl` editors.
 */
class SplineTweenerControl : public godot::VBoxContainer {
    GDCLASS(SplineTweenerControl, godot::VBoxContainer)

private:
    godot::Ref<SplineTweenResource> tween_resource_;
    SplineTweener *bound_tweener_node_ = nullptr;
    bool is_updating_ui_               = false;

    // 1. Master Timeline & Transport Bar
    godot::HSlider *scrub_slider_       = nullptr;
    godot::SpinBox *scrub_spin_         = nullptr;
    godot::SpinBox *duration_spin_      = nullptr;
    godot::OptionButton *loop_mode_opt_ = nullptr;
    godot::CheckBox *weighted_check_    = nullptr;

    // 2. Dynamic Track Cards Container
    godot::VBoxContainer *tracks_vbox_  = nullptr;
    godot::Label *summary_label_        = nullptr;

    void _build_ui();
    void _rebuild_track_cards();
    void _update_summary_label();

    // --- Callbacks ---
    void _on_tween_changed();
    void _on_scrub_changed(double p_val);
    void _on_duration_changed(double p_val);
    void _on_loop_mode_selected(int p_idx);
    void _on_weighted_toggled(bool p_weighted);

    void _on_play_pressed();
    void _on_pause_pressed();
    void _on_stop_pressed();

    void _on_add_blend_shape_track_pressed();
    void _on_add_property_track_pressed();
    void _on_add_shader_track_pressed();
    void _on_add_offset_3d_track_pressed();
    void _on_import_target_blend_shapes_pressed();
    void _on_clear_tracks_pressed();

    void _on_track_enabled_toggled(bool p_enabled, int p_track_idx);
    void _on_track_mode_selected(int p_mode_idx, int p_track_idx);
    void _on_track_key_submitted(const godot::String &p_key, int p_track_idx);
    void _on_track_scale_changed(double p_scale, int p_track_idx);
    void _on_track_curve_toggle_pressed(int p_track_idx);
    void _on_remove_track_pressed(int p_track_idx);

protected:
    static void _bind_methods();

public:
    SplineTweenerControl();
    ~SplineTweenerControl() override;

    void set_tween_resource(const godot::Ref<SplineTweenResource> &p_tween);
    [[nodiscard]] godot::Ref<SplineTweenResource> get_tween_resource() const { return tween_resource_; }

    void set_bound_tweener_node(SplineTweener *p_node);
    [[nodiscard]] SplineTweener *get_bound_tweener_node() const noexcept { return bound_tweener_node_; }

    void refresh_ui();
};

/**
 * SplineTweenResourceInspector (`GeometryEditorInspectorBase`)
 * Mounts `SplineTweenerControl` in the Godot Inspector whenever a `SplineTweenResource`
 * or `SplineTweener` node is inspected.
 */
class SplineTweenResourceInspector : public GeometryEditorInspectorBase {
    GDCLASS(SplineTweenResourceInspector, GeometryEditorInspectorBase)

protected:
    static void _bind_methods() {}

    [[nodiscard]] godot::Control *_create_inspector_control(godot::Object *p_object) override;
    [[nodiscard]] bool _is_managed_property(
        godot::Object *p_object,
        const godot::String &p_name
    ) const override;

public:
    SplineTweenResourceInspector() = default;
    ~SplineTweenResourceInspector() override = default;

    bool _can_handle(godot::Object *p_object) override;
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::SplineTweenTrackMode);