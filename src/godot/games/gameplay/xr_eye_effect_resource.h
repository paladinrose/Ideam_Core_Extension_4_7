#pragma once

/**
 * xr_eye_effect_resource.h
 * Section 2.4 Merger of `old_gdscript_plugins/ideam_xr` (`xr_eye_effect.gd`, `xr_eye_fx.gd`, `xr_fx.gd`,
 * `scenes/xr_eye_fx.tscn`, and `shaders/xr_eye_overlay.gdshader`) into `IdeamGames` + `IdeamSplines`.
 *
 * Key Architectural Upgrades:
 *   1. `XREyeEffectResource` (inherits `GameplayResource`):
 *      - Replaces legacy `XR_Eye_Effect` Node so eye effects can be shared across `GameAgent`s,
 *        `SceneTransition`s (`Game_In`/`Game_Out`, `Board_In`/`Board_Out`, `Blink`), and `GameplayStyle` cascades.
 *      - Replaces `godot::Curve` (`target_float_curves`) and `godot::Gradient` (`target_color_gradients`)
 *        with `IdeamSplines::Spline1DResource` and `IdeamSplines::Spline4DResource`.
 *   2. `XREyeFXNode3D` (inherits `godot::Node3D`):
 *      - Native C++26 stereoscopic overlay rig managing `both_eyes_overlay` (`view_id = -1`),
 *        `left_eye_overlay` (`view_id = 0`), and `right_eye_overlay` (`view_id = 1`) quad meshes.
 *      - Evaluates multi-effect conflict resolution across `EYE_RESOLUTION_PRIORITY`,
 *        `EYE_RESOLUTION_ADD`, `EYE_RESOLUTION_AVERAGE`, and `EYE_RESOLUTION_PRIORITY_AVERAGE`
 *        (weighted by `priority + 1`) and synchronizes uniforms with `xr_eye_overlay.gdshader`.
 */

#include "gameplay_resource.h"
#include "../../splines/spline_1d/spline_1d_resource.h"
#include "../../splines/spline_4d/spline_4d_resource.h"

#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include <vector>

namespace ideam::godot_ext {

/**
 * Target eye overlay selection (mirrors `XR_FX.EyeEffectTarget`).
 */
enum XREyeEffectTarget : int32_t {
    EYE_TARGET_BOTH  = 0, // Applied to the shared both-eyes quad (view_id = -1)
    EYE_TARGET_LEFT  = 1, // Applied exclusively to the left-eye quad (VIEW_INDEX == 0)
    EYE_TARGET_RIGHT = 2  // Applied exclusively to the right-eye quad (VIEW_INDEX == 1)
};

/**
 * Multi-effect parameter conflict resolution strategy (mirrors `XR_FX.EffectedFloatResolutions`
 * and `XR_FX.EffectedColorResolutions`).
 */
enum XREyeEffectResolutionMode : int32_t {
    EYE_RESOLUTION_PRIORITY         = 0, // Highest-priority active effect wins
    EYE_RESOLUTION_ADD              = 1, // Sum of all active effect contributions
    EYE_RESOLUTION_AVERAGE          = 2, // Unweighted arithmetic mean across active effects
    EYE_RESOLUTION_PRIORITY_AVERAGE = 3  // Priority-weighted mean (weight = priority + 1)
};

/**
 * XREyeEffectResource
 * Declarative `GameplayResource` driving stereoscopic per-eye shader parameters via
 * `Spline1DResource` (float tracks such as `eye_open`, `eye_feather`, `override`) and
 * `Spline4DResource` (4-channel RGBA/HSVA color tracks such as `color`, `override_color`, `eye_closed_color`).
 */
class XREyeEffectResource : public GameplayResource {
    GDCLASS(XREyeEffectResource, GameplayResource)

protected:
    static void _bind_methods();

private:
    int effect_priority = 0;
    XREyeEffectTarget effect_target = XREyeEffectTarget::EYE_TARGET_BOTH;
    XREyeEffectResolutionMode float_resolution_mode = XREyeEffectResolutionMode::EYE_RESOLUTION_PRIORITY;
    XREyeEffectResolutionMode color_resolution_mode = XREyeEffectResolutionMode::EYE_RESOLUTION_PRIORITY;

    godot::TypedArray<godot::StringName> target_colors;
    godot::TypedArray<Spline4DResource> target_color_splines;
    bool use_hsva_color_splines = false;

    godot::TypedArray<godot::StringName> target_floats;
    godot::TypedArray<Spline1DResource> target_float_splines;

    float default_effect_value = 0.0f;
    float default_tween_time = 0.1f;
    float current_value = 0.0f;

public:
    XREyeEffectResource();
    ~XREyeEffectResource() override;

    // --- Priority & Target Setters / Getters ---
    void set_effect_priority(int p_priority);
    int get_effect_priority() const;

    void set_effect_target(XREyeEffectTarget p_target);
    XREyeEffectTarget get_effect_target() const;

    void set_float_resolution_mode(XREyeEffectResolutionMode p_mode);
    XREyeEffectResolutionMode get_float_resolution_mode() const;

    void set_color_resolution_mode(XREyeEffectResolutionMode p_mode);
    XREyeEffectResolutionMode get_color_resolution_mode() const;

    // --- Color Track Setters / Getters (Spline4DResource) ---
    void set_target_colors(const godot::TypedArray<godot::StringName> &p_names);
    godot::TypedArray<godot::StringName> get_target_colors() const;

    void set_target_color_splines(const godot::TypedArray<Spline4DResource> &p_splines);
    godot::TypedArray<Spline4DResource> get_target_color_splines() const;

    void set_use_hsva_color_splines(bool p_use_hsva);
    bool get_use_hsva_color_splines() const;

    // --- Float Track Setters / Getters (Spline1DResource) ---
    void set_target_floats(const godot::TypedArray<godot::StringName> &p_names);
    godot::TypedArray<godot::StringName> get_target_floats() const;

    void set_target_float_splines(const godot::TypedArray<Spline1DResource> &p_splines);
    godot::TypedArray<Spline1DResource> get_target_float_splines() const;

    // --- Effect Value & Tween Timing ---
    void set_default_effect_value(float p_val);
    float get_default_effect_value() const;

    void set_default_tween_time(float p_time);
    float get_default_tween_time() const;

    void set_current_value(float p_val);
    float get_current_value() const;

    // --- Track Registration & Sampling ---
    void add_float_track(const godot::StringName &p_param_name, const godot::Ref<Spline1DResource> &p_spline);
    void add_color_track(const godot::StringName &p_param_name, const godot::Ref<Spline4DResource> &p_spline);

    float sample_float_track(int p_track_index, float p_percent) const;
    godot::Color sample_color_track(int p_track_index, float p_percent) const;

    /**
     * Evaluates all float and color tracks at `p_percent` (clamped to [0..1]) and returns
     * `{"floats": Dictionary(StringName -> float), "colors": Dictionary(StringName -> Color)}`.
     */
    godot::Dictionary evaluate_at(float p_percent) const;

    // --- Static Preset Factories (For SceneTransition & Comfort Vignettes) ---
    static godot::Ref<XREyeEffectResource> create_blink_effect(
        float p_default_tween_time = 0.15f,
        const godot::Color &p_closed_color = godot::Color(0.0f, 0.0f, 0.0f, 1.0f),
        XREyeEffectTarget p_target = XREyeEffectTarget::EYE_TARGET_BOTH
    );
    static godot::Ref<XREyeEffectResource> create_fade_effect(
        float p_default_tween_time = 0.35f,
        const godot::Color &p_fade_color = godot::Color(0.0f, 0.0f, 0.0f, 1.0f),
        XREyeEffectTarget p_target = XREyeEffectTarget::EYE_TARGET_BOTH
    );
    static godot::Ref<XREyeEffectResource> create_comfort_vignette_effect(
        float p_min_eye_open = 0.55f,
        float p_feather = 0.35f,
        const godot::Color &p_vignette_color = godot::Color(0.0f, 0.0f, 0.0f, 1.0f)
    );

    // --- GameplayResource Overrides ---
    virtual godot::TypedArray<GameplayRule> collect_rules() const override;
    virtual void apply_resource(const godot::Ref<GameplayResource> &p_other) override;
    virtual godot::Dictionary save_resource() const override;
    virtual void load_resource(const godot::Dictionary &p_data) override;
};

/**
 * XREyeFXNode3D
 * Stereoscopic per-eye shader overlay controller node (ported from `xr_eye_fx.gd` and `xr_eye_fx.tscn`).
 * Can be attached as a child of `XRCamera3D` / `Camera3D` on `GamePlayer`.
 */
class XREyeFXNode3D : public godot::Node3D {
    GDCLASS(XREyeFXNode3D, godot::Node3D)

protected:
    static void _bind_methods();

private:
    struct ActiveTweenState {
        int effect_index = -1;
        float from_value = 0.0f;
        float to_value = 0.0f;
        float duration = 0.1f;
        float elapsed = 0.0f;
        bool active = false;
    };

    godot::MeshInstance3D *both_eyes_overlay = nullptr;
    godot::MeshInstance3D *left_eye_overlay = nullptr;
    godot::MeshInstance3D *right_eye_overlay = nullptr;

    godot::TypedArray<XREyeEffectResource> effects;
    godot::PackedFloat32Array effect_values;

    godot::Dictionary float_resolutions; // StringName -> int (XREyeEffectResolutionMode)
    godot::Dictionary color_resolutions; // StringName -> int (XREyeEffectResolutionMode)

    godot::Dictionary default_both_floats;
    godot::Dictionary default_both_colors;
    godot::Dictionary default_left_floats;
    godot::Dictionary default_left_colors;
    godot::Dictionary default_right_floats;
    godot::Dictionary default_right_colors;

    bool auto_create_overlays_if_missing = true;
    std::vector<ActiveTweenState> _active_tweens;

    void _ensure_overlay_meshes();
    void _cache_default_overlay_parameters();
    godot::ShaderMaterial *_get_overlay_shader_material(godot::MeshInstance3D *p_overlay) const;
    void _resolve_and_apply_target(XREyeEffectTarget p_target, godot::MeshInstance3D *p_overlay, const godot::Dictionary &p_default_floats, const godot::Dictionary &p_default_colors);

public:
    XREyeFXNode3D();
    ~XREyeFXNode3D() override;

    virtual void _ready() override;
    virtual void _process(double p_delta) override;

    // --- Overlay Node Setters / Getters ---
    void set_both_eyes_overlay(godot::MeshInstance3D *p_overlay);
    godot::MeshInstance3D *get_both_eyes_overlay() const;

    void set_left_eye_overlay(godot::MeshInstance3D *p_overlay);
    godot::MeshInstance3D *get_left_eye_overlay() const;

    void set_right_eye_overlay(godot::MeshInstance3D *p_overlay);
    godot::MeshInstance3D *get_right_eye_overlay() const;

    void set_auto_create_overlays_if_missing(bool p_auto);
    bool get_auto_create_overlays_if_missing() const;

    // --- Effects & Conflict Resolution ---
    void set_effects(const godot::TypedArray<XREyeEffectResource> &p_effects);
    godot::TypedArray<XREyeEffectResource> get_effects() const;

    void set_float_resolutions(const godot::Dictionary &p_resolutions);
    godot::Dictionary get_float_resolutions() const;

    void set_color_resolutions(const godot::Dictionary &p_resolutions);
    godot::Dictionary get_color_resolutions() const;

    int add_effect(const godot::Ref<XREyeEffectResource> &p_effect);
    bool remove_effect(const godot::Ref<XREyeEffectResource> &p_effect);
    bool remove_effect_at(int p_index);
    void clear_effects();
    int get_effect_count() const;

    void set_effect_value(int p_effect_index, float p_value);
    float get_effect_value(int p_effect_index) const;

    void tween_effect(int p_effect_index, float p_to_value, float p_tween_time = -1.0f);
    void apply_all_effects();

    // --- High-Level Stereoscopic Transition Helpers ---
    void blink(float p_duration = 0.2f);
    void fade_out(float p_duration = 0.35f);
    void fade_in(float p_duration = 0.35f);
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::XREyeEffectTarget);
VARIANT_ENUM_CAST(ideam::godot_ext::XREyeEffectResolutionMode);
