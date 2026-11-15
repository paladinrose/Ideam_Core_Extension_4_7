#include "xr_eye_effect_resource.h"
#include "gameplay_rule.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/quad_mesh.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

#include <algorithm>
#include <cmath>

namespace ideam::godot_ext {

// ============================================================================
// XREyeEffectResource Implementation
// ============================================================================

void XREyeEffectResource::_bind_methods() {
    BIND_ENUM_CONSTANT(XREyeEffectTarget::EYE_TARGET_BOTH);
    BIND_ENUM_CONSTANT(XREyeEffectTarget::EYE_TARGET_LEFT);
    BIND_ENUM_CONSTANT(XREyeEffectTarget::EYE_TARGET_RIGHT);

    BIND_ENUM_CONSTANT(XREyeEffectResolutionMode::EYE_RESOLUTION_PRIORITY);
    BIND_ENUM_CONSTANT(XREyeEffectResolutionMode::EYE_RESOLUTION_ADD);
    BIND_ENUM_CONSTANT(XREyeEffectResolutionMode::EYE_RESOLUTION_AVERAGE);
    BIND_ENUM_CONSTANT(XREyeEffectResolutionMode::EYE_RESOLUTION_PRIORITY_AVERAGE);

    ADD_SIGNAL(godot::MethodInfo("effect_changed", godot::PropertyInfo(godot::Variant::FLOAT, "new_value")));
    ADD_SIGNAL(godot::MethodInfo(
        "request_tween",
        godot::PropertyInfo(godot::Variant::FLOAT, "to_value"),
        godot::PropertyInfo(godot::Variant::FLOAT, "tween_time")
    ));

    ADD_GROUP("Stereoscopic Eye Effect", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_effect_priority", "priority"), &XREyeEffectResource::set_effect_priority);
    godot::ClassDB::bind_method(godot::D_METHOD("get_effect_priority"), &XREyeEffectResource::get_effect_priority);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "effect_priority"), "set_effect_priority", "get_effect_priority");

    godot::ClassDB::bind_method(godot::D_METHOD("set_effect_target", "target"), &XREyeEffectResource::set_effect_target);
    godot::ClassDB::bind_method(godot::D_METHOD("get_effect_target"), &XREyeEffectResource::get_effect_target);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::INT, "effect_target", godot::PROPERTY_HINT_ENUM, "Both Eyes:0,Left Eye:1,Right Eye:2"),
        "set_effect_target",
        "get_effect_target"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_float_resolution_mode", "mode"), &XREyeEffectResource::set_float_resolution_mode);
    godot::ClassDB::bind_method(godot::D_METHOD("get_float_resolution_mode"), &XREyeEffectResource::get_float_resolution_mode);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::INT, "float_resolution_mode", godot::PROPERTY_HINT_ENUM, "Priority:0,Add:1,Average:2,Priority Average:3"),
        "set_float_resolution_mode",
        "get_float_resolution_mode"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_color_resolution_mode", "mode"), &XREyeEffectResource::set_color_resolution_mode);
    godot::ClassDB::bind_method(godot::D_METHOD("get_color_resolution_mode"), &XREyeEffectResource::get_color_resolution_mode);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::INT, "color_resolution_mode", godot::PROPERTY_HINT_ENUM, "Priority:0,Add:1,Average:2,Priority Average:3"),
        "set_color_resolution_mode",
        "get_color_resolution_mode"
    );

    ADD_GROUP("Spline4D Color Tracks", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_target_colors", "names"), &XREyeEffectResource::set_target_colors);
    godot::ClassDB::bind_method(godot::D_METHOD("get_target_colors"), &XREyeEffectResource::get_target_colors);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::ARRAY, "target_colors", godot::PROPERTY_HINT_ARRAY_TYPE, "StringName"),
        "set_target_colors",
        "get_target_colors"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_target_color_splines", "splines"), &XREyeEffectResource::set_target_color_splines);
    godot::ClassDB::bind_method(godot::D_METHOD("get_target_color_splines"), &XREyeEffectResource::get_target_color_splines);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::ARRAY, "target_color_splines", godot::PROPERTY_HINT_ARRAY_TYPE, "Spline4DResource"),
        "set_target_color_splines",
        "get_target_color_splines"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_use_hsva_color_splines", "use_hsva"), &XREyeEffectResource::set_use_hsva_color_splines);
    godot::ClassDB::bind_method(godot::D_METHOD("get_use_hsva_color_splines"), &XREyeEffectResource::get_use_hsva_color_splines);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "use_hsva_color_splines"), "set_use_hsva_color_splines", "get_use_hsva_color_splines");

    ADD_GROUP("Spline1D Float Tracks", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_target_floats", "names"), &XREyeEffectResource::set_target_floats);
    godot::ClassDB::bind_method(godot::D_METHOD("get_target_floats"), &XREyeEffectResource::get_target_floats);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::ARRAY, "target_floats", godot::PROPERTY_HINT_ARRAY_TYPE, "StringName"),
        "set_target_floats",
        "get_target_floats"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_target_float_splines", "splines"), &XREyeEffectResource::set_target_float_splines);
    godot::ClassDB::bind_method(godot::D_METHOD("get_target_float_splines"), &XREyeEffectResource::get_target_float_splines);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::ARRAY, "target_float_splines", godot::PROPERTY_HINT_ARRAY_TYPE, "Spline1DResource"),
        "set_target_float_splines",
        "get_target_float_splines"
    );

    ADD_GROUP("Evaluation & Tween Defaults", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_default_effect_value", "value"), &XREyeEffectResource::set_default_effect_value);
    godot::ClassDB::bind_method(godot::D_METHOD("get_default_effect_value"), &XREyeEffectResource::get_default_effect_value);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "default_effect_value", godot::PROPERTY_HINT_RANGE, "0.0,1.0,0.01"),
        "set_default_effect_value",
        "get_default_effect_value"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_default_tween_time", "time"), &XREyeEffectResource::set_default_tween_time);
    godot::ClassDB::bind_method(godot::D_METHOD("get_default_tween_time"), &XREyeEffectResource::get_default_tween_time);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "default_tween_time", godot::PROPERTY_HINT_RANGE, "0.0,10.0,0.01,suffix:s"),
        "set_default_tween_time",
        "get_default_tween_time"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_current_value", "value"), &XREyeEffectResource::set_current_value);
    godot::ClassDB::bind_method(godot::D_METHOD("get_current_value"), &XREyeEffectResource::get_current_value);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "current_value", godot::PROPERTY_HINT_RANGE, "0.0,1.0,0.01"),
        "set_current_value",
        "get_current_value"
    );

    // Methods
    godot::ClassDB::bind_method(godot::D_METHOD("add_float_track", "param_name", "spline"), &XREyeEffectResource::add_float_track);
    godot::ClassDB::bind_method(godot::D_METHOD("add_color_track", "param_name", "spline"), &XREyeEffectResource::add_color_track);
    godot::ClassDB::bind_method(godot::D_METHOD("sample_float_track", "track_index", "percent"), &XREyeEffectResource::sample_float_track);
    godot::ClassDB::bind_method(godot::D_METHOD("sample_color_track", "track_index", "percent"), &XREyeEffectResource::sample_color_track);
    godot::ClassDB::bind_method(godot::D_METHOD("evaluate_at", "percent"), &XREyeEffectResource::evaluate_at);

    godot::ClassDB::bind_static_method(
        "XREyeEffectResource",
        godot::D_METHOD("create_blink_effect", "default_tween_time", "closed_color", "target"),
        &XREyeEffectResource::create_blink_effect,
        DEFVAL(0.15f),
        DEFVAL(godot::Color(0.0f, 0.0f, 0.0f, 1.0f)),
        DEFVAL(XREyeEffectTarget::EYE_TARGET_BOTH)
    );
    godot::ClassDB::bind_static_method(
        "XREyeEffectResource",
        godot::D_METHOD("create_fade_effect", "default_tween_time", "fade_color", "target"),
        &XREyeEffectResource::create_fade_effect,
        DEFVAL(0.35f),
        DEFVAL(godot::Color(0.0f, 0.0f, 0.0f, 1.0f)),
        DEFVAL(XREyeEffectTarget::EYE_TARGET_BOTH)
    );
    godot::ClassDB::bind_static_method(
        "XREyeEffectResource",
        godot::D_METHOD("create_comfort_vignette_effect", "min_eye_open", "feather", "vignette_color"),
        &XREyeEffectResource::create_comfort_vignette_effect,
        DEFVAL(0.55f),
        DEFVAL(0.35f),
        DEFVAL(godot::Color(0.0f, 0.0f, 0.0f, 1.0f))
    );
}

XREyeEffectResource::XREyeEffectResource() {
    set_category(godot::StringName("XR_EyeFX"));
    set_config_section("xr_eye_fx");
}

XREyeEffectResource::~XREyeEffectResource() = default;

void XREyeEffectResource::set_effect_priority(int p_priority) {
    int clamped = std::max(0, p_priority);
    if (effect_priority == clamped) return;
    effect_priority = clamped;
    emit_changed();
}
int XREyeEffectResource::get_effect_priority() const { return effect_priority; }

void XREyeEffectResource::set_effect_target(XREyeEffectTarget p_target) {
    if (effect_target == p_target) return;
    effect_target = p_target;
    emit_changed();
}
XREyeEffectTarget XREyeEffectResource::get_effect_target() const { return effect_target; }

void XREyeEffectResource::set_float_resolution_mode(XREyeEffectResolutionMode p_mode) {
    if (float_resolution_mode == p_mode) return;
    float_resolution_mode = p_mode;
    emit_changed();
}
XREyeEffectResolutionMode XREyeEffectResource::get_float_resolution_mode() const { return float_resolution_mode; }

void XREyeEffectResource::set_color_resolution_mode(XREyeEffectResolutionMode p_mode) {
    if (color_resolution_mode == p_mode) return;
    color_resolution_mode = p_mode;
    emit_changed();
}
XREyeEffectResolutionMode XREyeEffectResource::get_color_resolution_mode() const { return color_resolution_mode; }

void XREyeEffectResource::set_target_colors(const godot::TypedArray<godot::StringName> &p_names) {
    target_colors = p_names;
    emit_changed();
}
godot::TypedArray<godot::StringName> XREyeEffectResource::get_target_colors() const { return target_colors; }

void XREyeEffectResource::set_target_color_splines(const godot::TypedArray<Spline4DResource> &p_splines) {
    target_color_splines = p_splines;
    emit_changed();
}
godot::TypedArray<Spline4DResource> XREyeEffectResource::get_target_color_splines() const { return target_color_splines; }

void XREyeEffectResource::set_use_hsva_color_splines(bool p_use_hsva) {
    if (use_hsva_color_splines == p_use_hsva) return;
    use_hsva_color_splines = p_use_hsva;
    emit_changed();
}
bool XREyeEffectResource::get_use_hsva_color_splines() const { return use_hsva_color_splines; }

void XREyeEffectResource::set_target_floats(const godot::TypedArray<godot::StringName> &p_names) {
    target_floats = p_names;
    emit_changed();
}
godot::TypedArray<godot::StringName> XREyeEffectResource::get_target_floats() const { return target_floats; }

void XREyeEffectResource::set_target_float_splines(const godot::TypedArray<Spline1DResource> &p_splines) {
    target_float_splines = p_splines;
    emit_changed();
}
godot::TypedArray<Spline1DResource> XREyeEffectResource::get_target_float_splines() const { return target_float_splines; }

void XREyeEffectResource::set_default_effect_value(float p_val) {
    float clamped = std::clamp(p_val, 0.0f, 1.0f);
    if (godot::Math::is_equal_approx(default_effect_value, clamped)) return;
    default_effect_value = clamped;
    current_value = clamped;
    emit_changed();
}
float XREyeEffectResource::get_default_effect_value() const { return default_effect_value; }

void XREyeEffectResource::set_default_tween_time(float p_time) {
    float clamped = std::max(0.0f, p_time);
    if (godot::Math::is_equal_approx(default_tween_time, clamped)) return;
    default_tween_time = clamped;
    emit_changed();
}
float XREyeEffectResource::get_default_tween_time() const { return default_tween_time; }

void XREyeEffectResource::set_current_value(float p_val) {
    float clamped = std::clamp(p_val, 0.0f, 1.0f);
    if (godot::Math::is_equal_approx(current_value, clamped)) return;
    current_value = clamped;
    emit_signal("effect_changed", current_value);
}
float XREyeEffectResource::get_current_value() const { return current_value; }

void XREyeEffectResource::add_float_track(const godot::StringName &p_param_name, const godot::Ref<Spline1DResource> &p_spline) {
    if (p_param_name.is_empty() || !p_spline.is_valid()) return;
    target_floats.push_back(p_param_name);
    target_float_splines.push_back(p_spline);
    emit_changed();
}

void XREyeEffectResource::add_color_track(const godot::StringName &p_param_name, const godot::Ref<Spline4DResource> &p_spline) {
    if (p_param_name.is_empty() || !p_spline.is_valid()) return;
    target_colors.push_back(p_param_name);
    target_color_splines.push_back(p_spline);
    emit_changed();
}

float XREyeEffectResource::sample_float_track(int p_track_index, float p_percent) const {
    if (p_track_index < 0 || p_track_index >= target_float_splines.size()) {
        return 0.0f;
    }
    godot::Ref<Spline1DResource> spline = target_float_splines[p_track_index];
    if (!spline.is_valid() || spline->get_point_count() == 0) {
        return std::clamp(p_percent, 0.0f, 1.0f);
    }
    return static_cast<float>(spline->sample(static_cast<real_t>(std::clamp(p_percent, 0.0f, 1.0f)), true));
}

godot::Color XREyeEffectResource::sample_color_track(int p_track_index, float p_percent) const {
    if (p_track_index < 0 || p_track_index >= target_color_splines.size()) {
        return godot::Color(0.0f, 0.0f, 0.0f, 0.0f);
    }
    godot::Ref<Spline4DResource> spline = target_color_splines[p_track_index];
    if (!spline.is_valid() || spline->get_point_count() == 0) {
        return godot::Color(0.0f, 0.0f, 0.0f, 0.0f);
    }
    return spline->sample_color(static_cast<real_t>(std::clamp(p_percent, 0.0f, 1.0f)), true, use_hsva_color_splines);
}

godot::Dictionary XREyeEffectResource::evaluate_at(float p_percent) const {
    godot::Dictionary result;
    godot::Dictionary floats_dict;
    godot::Dictionary colors_dict;

    const float t = std::clamp(p_percent, 0.0f, 1.0f);

    const int float_count = std::min(target_floats.size(), target_float_splines.size());
    for (int i = 0; i < float_count; ++i) {
        godot::StringName name = target_floats[i];
        if (!name.is_empty()) {
            floats_dict[name] = sample_float_track(i, t);
        }
    }

    const int color_count = std::min(target_colors.size(), target_color_splines.size());
    for (int i = 0; i < color_count; ++i) {
        godot::StringName name = target_colors[i];
        if (!name.is_empty()) {
            colors_dict[name] = sample_color_track(i, t);
        }
    }

    result["floats"] = floats_dict;
    result["colors"] = colors_dict;
    return result;
}

godot::Ref<XREyeEffectResource> XREyeEffectResource::create_blink_effect(
    float p_default_tween_time,
    const godot::Color &p_closed_color,
    XREyeEffectTarget p_target
) {
    godot::Ref<XREyeEffectResource> fx;
    fx.instantiate();
    fx->set_title("XR_Blink_Effect");
    fx->set_effect_priority(100);
    fx->set_effect_target(p_target);
    fx->set_default_tween_time(p_default_tween_time);
    fx->set_default_effect_value(0.0f);

    // eye_open: 1.0 at t=0.0 -> 0.0 at t=1.0
    godot::Ref<Spline1DResource> open_spline = Spline1DResource::create_one_to_zero();
    fx->add_float_track(godot::StringName("eye_open"), open_spline);

    // eye_feather: 0.0 at t=0.0 -> 0.15 at t=0.5 -> 0.0 at t=1.0
    godot::Ref<Spline1DResource> feather_spline;
    feather_spline.instantiate();
    feather_spline->add_point(0.0f, SplineCurveType::CURVE_CATMULL_ROM);
    feather_spline->add_point(0.15f, SplineCurveType::CURVE_CATMULL_ROM);
    feather_spline->add_point(0.0f, SplineCurveType::CURVE_CATMULL_ROM);
    fx->add_float_track(godot::StringName("eye_feather"), feather_spline);

    // eye_closed_color: constant closed eyelid/blackout color
    godot::PackedColorArray cols;
    cols.push_back(p_closed_color);
    cols.push_back(p_closed_color);
    godot::Ref<Spline4DResource> color_spline = Spline4DResource::create_color_gradient(cols, false);
    fx->add_color_track(godot::StringName("eye_closed_color"), color_spline);

    return fx;
}

godot::Ref<XREyeEffectResource> XREyeEffectResource::create_fade_effect(
    float p_default_tween_time,
    const godot::Color &p_fade_color,
    XREyeEffectTarget p_target
) {
    godot::Ref<XREyeEffectResource> fx;
    fx.instantiate();
    fx->set_title("XR_Fade_Effect");
    fx->set_effect_priority(90);
    fx->set_effect_target(p_target);
    fx->set_default_tween_time(p_default_tween_time);
    fx->set_default_effect_value(0.0f);

    godot::Ref<Spline1DResource> override_spline = Spline1DResource::create_zero_to_one();
    fx->add_float_track(godot::StringName("override"), override_spline);

    godot::PackedColorArray cols;
    godot::Color start_col = p_fade_color;
    start_col.a = 0.0f;
    cols.push_back(start_col);
    cols.push_back(p_fade_color);
    godot::Ref<Spline4DResource> color_spline = Spline4DResource::create_color_gradient(cols, false);
    fx->add_color_track(godot::StringName("override_color"), color_spline);

    return fx;
}

godot::Ref<XREyeEffectResource> XREyeEffectResource::create_comfort_vignette_effect(
    float p_min_eye_open,
    float p_feather,
    const godot::Color &p_vignette_color
) {
    godot::Ref<XREyeEffectResource> fx;
    fx.instantiate();
    fx->set_title("XR_Comfort_Vignette");
    fx->set_effect_priority(50);
    fx->set_effect_target(XREyeEffectTarget::EYE_TARGET_BOTH);
    fx->set_default_tween_time(0.12f);
    fx->set_default_effect_value(0.0f);

    godot::Ref<Spline1DResource> open_spline;
    open_spline.instantiate();
    open_spline->add_point(1.0f, SplineCurveType::CURVE_LINEAR);
    open_spline->add_point(std::clamp(p_min_eye_open, 0.1f, 1.0f), SplineCurveType::CURVE_LINEAR);
    fx->add_float_track(godot::StringName("eye_open"), open_spline);

    godot::Ref<Spline1DResource> feather_spline;
    feather_spline.instantiate();
    feather_spline->add_point(0.0f, SplineCurveType::CURVE_LINEAR);
    feather_spline->add_point(std::clamp(p_feather, 0.0f, 1.0f), SplineCurveType::CURVE_LINEAR);
    fx->add_float_track(godot::StringName("eye_feather"), feather_spline);

    godot::PackedColorArray cols;
    cols.push_back(p_vignette_color);
    cols.push_back(p_vignette_color);
    godot::Ref<Spline4DResource> color_spline = Spline4DResource::create_color_gradient(cols, false);
    fx->add_color_track(godot::StringName("eye_closed_color"), color_spline);

    return fx;
}

godot::TypedArray<GameplayRule> XREyeEffectResource::collect_rules() const {
    return GameplayResource::collect_rules();
}

void XREyeEffectResource::apply_resource(const godot::Ref<GameplayResource> &p_other) {
    GameplayResource::apply_resource(p_other);
    godot::Ref<XREyeEffectResource> other = p_other;
    if (!other.is_valid()) return;

    effect_priority = other->get_effect_priority();
    effect_target = other->get_effect_target();
    float_resolution_mode = other->get_float_resolution_mode();
    color_resolution_mode = other->get_color_resolution_mode();
    target_colors = other->get_target_colors().duplicate();
    target_color_splines = other->get_target_color_splines().duplicate();
    use_hsva_color_splines = other->get_use_hsva_color_splines();
    target_floats = other->get_target_floats().duplicate();
    target_float_splines = other->get_target_float_splines().duplicate();
    default_effect_value = other->get_default_effect_value();
    default_tween_time = other->get_default_tween_time();
    current_value = other->get_current_value();
    emit_changed();
}

godot::Dictionary XREyeEffectResource::save_resource() const {
    godot::Dictionary data = GameplayResource::save_resource();
    data["effect_priority"] = effect_priority;
    data["effect_target"] = static_cast<int>(effect_target);
    data["float_resolution_mode"] = static_cast<int>(float_resolution_mode);
    data["color_resolution_mode"] = static_cast<int>(color_resolution_mode);
    data["target_colors"] = target_colors;
    data["use_hsva_color_splines"] = use_hsva_color_splines;
    data["target_floats"] = target_floats;
    data["default_effect_value"] = default_effect_value;
    data["default_tween_time"] = default_tween_time;
    data["current_value"] = current_value;
    return data;
}

void XREyeEffectResource::load_resource(const godot::Dictionary &p_data) {
    GameplayResource::load_resource(p_data);
    if (p_data.has("effect_priority")) effect_priority = static_cast<int>(p_data["effect_priority"]);
    if (p_data.has("effect_target")) effect_target = static_cast<XREyeEffectTarget>(static_cast<int>(p_data["effect_target"]));
    if (p_data.has("float_resolution_mode")) float_resolution_mode = static_cast<XREyeEffectResolutionMode>(static_cast<int>(p_data["float_resolution_mode"]));
    if (p_data.has("color_resolution_mode")) color_resolution_mode = static_cast<XREyeEffectResolutionMode>(static_cast<int>(p_data["color_resolution_mode"]));
    if (p_data.has("target_colors")) target_colors = p_data["target_colors"];
    if (p_data.has("use_hsva_color_splines")) use_hsva_color_splines = static_cast<bool>(p_data["use_hsva_color_splines"]);
    if (p_data.has("target_floats")) target_floats = p_data["target_floats"];
    if (p_data.has("default_effect_value")) default_effect_value = static_cast<float>(p_data["default_effect_value"]);
    if (p_data.has("default_tween_time")) default_tween_time = static_cast<float>(p_data["default_tween_time"]);
    if (p_data.has("current_value")) current_value = static_cast<float>(p_data["current_value"]);
}

// ============================================================================
// XREyeFXNode3D Implementation
// ============================================================================

void XREyeFXNode3D::_bind_methods() {
    ADD_SIGNAL(godot::MethodInfo(
        "effect_tween_completed",
        godot::PropertyInfo(godot::Variant::INT, "effect_index"),
        godot::PropertyInfo(godot::Variant::FLOAT, "final_value")
    ));

    godot::ClassDB::bind_method(godot::D_METHOD("set_both_eyes_overlay", "overlay"), &XREyeFXNode3D::set_both_eyes_overlay);
    godot::ClassDB::bind_method(godot::D_METHOD("get_both_eyes_overlay"), &XREyeFXNode3D::get_both_eyes_overlay);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::OBJECT, "both_eyes_overlay", godot::PROPERTY_HINT_NODE_TYPE, "MeshInstance3D"),
        "set_both_eyes_overlay",
        "get_both_eyes_overlay"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_left_eye_overlay", "overlay"), &XREyeFXNode3D::set_left_eye_overlay);
    godot::ClassDB::bind_method(godot::D_METHOD("get_left_eye_overlay"), &XREyeFXNode3D::get_left_eye_overlay);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::OBJECT, "left_eye_overlay", godot::PROPERTY_HINT_NODE_TYPE, "MeshInstance3D"),
        "set_left_eye_overlay",
        "get_left_eye_overlay"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_right_eye_overlay", "overlay"), &XREyeFXNode3D::set_right_eye_overlay);
    godot::ClassDB::bind_method(godot::D_METHOD("get_right_eye_overlay"), &XREyeFXNode3D::get_right_eye_overlay);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::OBJECT, "right_eye_overlay", godot::PROPERTY_HINT_NODE_TYPE, "MeshInstance3D"),
        "set_right_eye_overlay",
        "get_right_eye_overlay"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_auto_create_overlays_if_missing", "auto_create"), &XREyeFXNode3D::set_auto_create_overlays_if_missing);
    godot::ClassDB::bind_method(godot::D_METHOD("get_auto_create_overlays_if_missing"), &XREyeFXNode3D::get_auto_create_overlays_if_missing);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "auto_create_overlays_if_missing"), "set_auto_create_overlays_if_missing", "get_auto_create_overlays_if_missing");

    godot::ClassDB::bind_method(godot::D_METHOD("set_effects", "effects"), &XREyeFXNode3D::set_effects);
    godot::ClassDB::bind_method(godot::D_METHOD("get_effects"), &XREyeFXNode3D::get_effects);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::ARRAY, "effects", godot::PROPERTY_HINT_ARRAY_TYPE, "XREyeEffectResource"),
        "set_effects",
        "get_effects"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_float_resolutions", "resolutions"), &XREyeFXNode3D::set_float_resolutions);
    godot::ClassDB::bind_method(godot::D_METHOD("get_float_resolutions"), &XREyeFXNode3D::get_float_resolutions);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::DICTIONARY, "float_resolutions"), "set_float_resolutions", "get_float_resolutions");

    godot::ClassDB::bind_method(godot::D_METHOD("set_color_resolutions", "resolutions"), &XREyeFXNode3D::set_color_resolutions);
    godot::ClassDB::bind_method(godot::D_METHOD("get_color_resolutions"), &XREyeFXNode3D::get_color_resolutions);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::DICTIONARY, "color_resolutions"), "set_color_resolutions", "get_color_resolutions");

    // Methods
    godot::ClassDB::bind_method(godot::D_METHOD("add_effect", "effect"), &XREyeFXNode3D::add_effect);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_effect", "effect"), &XREyeFXNode3D::remove_effect);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_effect_at", "index"), &XREyeFXNode3D::remove_effect_at);
    godot::ClassDB::bind_method(godot::D_METHOD("clear_effects"), &XREyeFXNode3D::clear_effects);
    godot::ClassDB::bind_method(godot::D_METHOD("get_effect_count"), &XREyeFXNode3D::get_effect_count);

    godot::ClassDB::bind_method(godot::D_METHOD("set_effect_value", "effect_index", "value"), &XREyeFXNode3D::set_effect_value);
    godot::ClassDB::bind_method(godot::D_METHOD("get_effect_value", "effect_index"), &XREyeFXNode3D::get_effect_value);
    godot::ClassDB::bind_method(godot::D_METHOD("tween_effect", "effect_index", "to_value", "tween_time"), &XREyeFXNode3D::tween_effect, DEFVAL(-1.0f));
    godot::ClassDB::bind_method(godot::D_METHOD("apply_all_effects"), &XREyeFXNode3D::apply_all_effects);

    godot::ClassDB::bind_method(godot::D_METHOD("blink", "duration"), &XREyeFXNode3D::blink, DEFVAL(0.2f));
    godot::ClassDB::bind_method(godot::D_METHOD("fade_out", "duration"), &XREyeFXNode3D::fade_out, DEFVAL(0.35f));
    godot::ClassDB::bind_method(godot::D_METHOD("fade_in", "duration"), &XREyeFXNode3D::fade_in, DEFVAL(0.35f));
}

XREyeFXNode3D::XREyeFXNode3D() {
    float_resolutions[godot::StringName("override")]    = static_cast<int>(XREyeEffectResolutionMode::EYE_RESOLUTION_PRIORITY);
    float_resolutions[godot::StringName("eye_open")]    = static_cast<int>(XREyeEffectResolutionMode::EYE_RESOLUTION_PRIORITY);
    float_resolutions[godot::StringName("eye_feather")] = static_cast<int>(XREyeEffectResolutionMode::EYE_RESOLUTION_PRIORITY);

    color_resolutions[godot::StringName("color")]            = static_cast<int>(XREyeEffectResolutionMode::EYE_RESOLUTION_PRIORITY);
    color_resolutions[godot::StringName("override_color")]   = static_cast<int>(XREyeEffectResolutionMode::EYE_RESOLUTION_PRIORITY);
    color_resolutions[godot::StringName("eye_closed_color")] = static_cast<int>(XREyeEffectResolutionMode::EYE_RESOLUTION_PRIORITY);

    default_both_floats[godot::StringName("override")]    = 0.0f;
    default_both_floats[godot::StringName("eye_open")]    = 1.0f;
    default_both_floats[godot::StringName("eye_feather")] = 0.0f;
    default_both_colors[godot::StringName("color")]            = godot::Color(0.0f, 0.0f, 0.0f, 0.0f);
    default_both_colors[godot::StringName("override_color")]   = godot::Color(0.0f, 0.0f, 0.0f, 0.0f);
    default_both_colors[godot::StringName("eye_closed_color")] = godot::Color(0.0f, 0.0f, 0.0f, 1.0f);

    default_left_floats  = default_both_floats.duplicate();
    default_left_colors  = default_both_colors.duplicate();
    default_right_floats = default_both_floats.duplicate();
    default_right_colors = default_both_colors.duplicate();
}

XREyeFXNode3D::~XREyeFXNode3D() = default;

void XREyeFXNode3D::_ready() {
    if (auto_create_overlays_if_missing) {
        _ensure_overlay_meshes();
    }
    _cache_default_overlay_parameters();
    apply_all_effects();
}

void XREyeFXNode3D::_process(double p_delta) {
    if (_active_tweens.empty() || p_delta <= 0.0) {
        return;
    }

    const float dt = static_cast<float>(p_delta);
    bool any_updated = false;

    for (auto &tw : _active_tweens) {
        if (!tw.active) continue;
        tw.elapsed += dt;
        float alpha = (tw.duration <= 0.0001f) ? 1.0f : std::clamp(tw.elapsed / tw.duration, 0.0f, 1.0f);
        float val = tw.from_value + (tw.to_value - tw.from_value) * alpha;
        if (tw.effect_index >= 0 && tw.effect_index < effect_values.size()) {
            effect_values.set(tw.effect_index, val);
            godot::Ref<XREyeEffectResource> fx = effects[tw.effect_index];
            if (fx.is_valid()) {
                fx->set_current_value(val);
            }
            any_updated = true;
        }
        if (alpha >= 0.9999f) {
            tw.active = false;
            emit_signal("effect_tween_completed", tw.effect_index, tw.to_value);
        }
    }

    _active_tweens.erase(
        std::remove_if(_active_tweens.begin(), _active_tweens.end(), [](const ActiveTweenState &s) { return !s.active; }),
        _active_tweens.end()
    );

    if (any_updated) {
        apply_all_effects();
    }
}

void XREyeFXNode3D::set_both_eyes_overlay(godot::MeshInstance3D *p_overlay) { both_eyes_overlay = p_overlay; }
godot::MeshInstance3D *XREyeFXNode3D::get_both_eyes_overlay() const { return both_eyes_overlay; }

void XREyeFXNode3D::set_left_eye_overlay(godot::MeshInstance3D *p_overlay) { left_eye_overlay = p_overlay; }
godot::MeshInstance3D *XREyeFXNode3D::get_left_eye_overlay() const { return left_eye_overlay; }

void XREyeFXNode3D::set_right_eye_overlay(godot::MeshInstance3D *p_overlay) { right_eye_overlay = p_overlay; }
godot::MeshInstance3D *XREyeFXNode3D::get_right_eye_overlay() const { return right_eye_overlay; }

void XREyeFXNode3D::set_auto_create_overlays_if_missing(bool p_auto) { auto_create_overlays_if_missing = p_auto; }
bool XREyeFXNode3D::get_auto_create_overlays_if_missing() const { return auto_create_overlays_if_missing; }

void XREyeFXNode3D::set_effects(const godot::TypedArray<XREyeEffectResource> &p_effects) {
    effects = p_effects;
    effect_values.resize(effects.size());
    for (int i = 0; i < effects.size(); ++i) {
        godot::Ref<XREyeEffectResource> fx = effects[i];
        effect_values.set(i, fx.is_valid() ? fx->get_default_effect_value() : 0.0f);
    }
    apply_all_effects();
}
godot::TypedArray<XREyeEffectResource> XREyeFXNode3D::get_effects() const { return effects; }

void XREyeFXNode3D::set_float_resolutions(const godot::Dictionary &p_resolutions) { float_resolutions = p_resolutions; }
godot::Dictionary XREyeFXNode3D::get_float_resolutions() const { return float_resolutions; }

void XREyeFXNode3D::set_color_resolutions(const godot::Dictionary &p_resolutions) { color_resolutions = p_resolutions; }
godot::Dictionary XREyeFXNode3D::get_color_resolutions() const { return color_resolutions; }

int XREyeFXNode3D::add_effect(const godot::Ref<XREyeEffectResource> &p_effect) {
    if (!p_effect.is_valid()) return -1;
    int existing = effects.find(p_effect);
    if (existing >= 0) return existing;

    int idx = effects.size();
    effects.push_back(p_effect);
    effect_values.push_back(p_effect->get_default_effect_value());
    apply_all_effects();
    return idx;
}

bool XREyeFXNode3D::remove_effect(const godot::Ref<XREyeEffectResource> &p_effect) {
    int idx = effects.find(p_effect);
    if (idx < 0) return false;
    return remove_effect_at(idx);
}

bool XREyeFXNode3D::remove_effect_at(int p_index) {
    if (p_index < 0 || p_index >= effects.size()) return false;
    effects.remove_at(p_index);
    effect_values.remove_at(p_index);
    apply_all_effects();
    return true;
}

void XREyeFXNode3D::clear_effects() {
    effects.clear();
    effect_values.clear();
    _active_tweens.clear();
    apply_all_effects();
}

int XREyeFXNode3D::get_effect_count() const {
    return effects.size();
}

void XREyeFXNode3D::set_effect_value(int p_effect_index, float p_value) {
    if (p_effect_index < 0 || p_effect_index >= effects.size()) return;
    if (effect_values.size() != effects.size()) {
        effect_values.resize(effects.size());
    }
    float clamped = std::clamp(p_value, 0.0f, 1.0f);
    effect_values.set(p_effect_index, clamped);
    godot::Ref<XREyeEffectResource> fx = effects[p_effect_index];
    if (fx.is_valid()) {
        fx->set_current_value(clamped);
    }
    apply_all_effects();
}

float XREyeFXNode3D::get_effect_value(int p_effect_index) const {
    if (p_effect_index < 0 || p_effect_index >= effect_values.size()) return 0.0f;
    return effect_values[p_effect_index];
}

void XREyeFXNode3D::tween_effect(int p_effect_index, float p_to_value, float p_tween_time) {
    if (p_effect_index < 0 || p_effect_index >= effects.size()) return;
    if (effect_values.size() != effects.size()) {
        effect_values.resize(effects.size());
    }

    godot::Ref<XREyeEffectResource> fx = effects[p_effect_index];
    float duration = p_tween_time;
    if (duration < 0.0f) {
        duration = fx.is_valid() ? fx->get_default_tween_time() : 0.1f;
    }

    if (duration <= 0.0001f) {
        set_effect_value(p_effect_index, p_to_value);
        emit_signal("effect_tween_completed", p_effect_index, std::clamp(p_to_value, 0.0f, 1.0f));
        return;
    }

    for (auto &tw : _active_tweens) {
        if (tw.effect_index == p_effect_index && tw.active) {
            tw.from_value = effect_values[p_effect_index];
            tw.to_value = std::clamp(p_to_value, 0.0f, 1.0f);
            tw.duration = duration;
            tw.elapsed = 0.0f;
            return;
        }
    }

    ActiveTweenState state;
    state.effect_index = p_effect_index;
    state.from_value = effect_values[p_effect_index];
    state.to_value = std::clamp(p_to_value, 0.0f, 1.0f);
    state.duration = duration;
    state.elapsed = 0.0f;
    state.active = true;
    _active_tweens.push_back(state);
}

godot::ShaderMaterial *XREyeFXNode3D::_get_overlay_shader_material(godot::MeshInstance3D *p_overlay) const {
    if (!p_overlay) return nullptr;
    godot::Ref<godot::Material> mat = p_overlay->get_surface_override_material(0);
    if (!mat.is_valid() && p_overlay->get_mesh().is_valid()) {
        mat = p_overlay->get_mesh()->surface_get_material(0);
    }
    return godot::Object::cast_to<godot::ShaderMaterial>(mat.ptr());
}

void XREyeFXNode3D::_ensure_overlay_meshes() {
    auto create_quad_overlay = [this](const char *p_name, int p_view_id, float p_z_offset) -> godot::MeshInstance3D * {
        if (has_node(p_name)) {
            if (auto *existing = get_node<godot::MeshInstance3D>(p_name)) {
                return existing;
            }
        }
        godot::MeshInstance3D *mi = memnew(godot::MeshInstance3D);
        mi->set_name(p_name);

        godot::Ref<godot::QuadMesh> quad;
        quad.instantiate();
        quad->set_size(godot::Vector2(2.0f, 2.0f));
        mi->set_mesh(quad);
        mi->set_position(godot::Vector3(0.0f, 0.0f, p_z_offset));
        mi->set_cast_shadows_setting(godot::GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);

        godot::Ref<godot::ShaderMaterial> smat;
        smat.instantiate();
        if (godot::ResourceLoader *rl = godot::ResourceLoader::get_singleton()) {
            const char *shader_paths[] = {
                "res://src/godot/games/shaders/xr_eye_overlay.gdshader",
                "res://old_gdscript_plugins/ideam_xr/shaders/xr_eye_overlay.gdshader"
            };
            for (const char *sp : shader_paths) {
                if (rl->exists(sp)) {
                    godot::Ref<godot::Shader> sh = rl->load(sp);
                    if (sh.is_valid()) {
                        smat->set_shader(sh);
                        break;
                    }
                }
            }
        }
        smat->set_shader_parameter("view_id", p_view_id);
        smat->set_shader_parameter("override", 0.0f);
        smat->set_shader_parameter("eye_open", 1.0f);
        smat->set_shader_parameter("eye_feather", 0.0f);
        smat->set_shader_parameter("color", godot::Color(0.0f, 0.0f, 0.0f, 0.0f));
        smat->set_shader_parameter("override_color", godot::Color(0.0f, 0.0f, 0.0f, 0.0f));
        smat->set_shader_parameter("eye_closed_color", godot::Color(0.0f, 0.0f, 0.0f, 1.0f));
        mi->set_surface_override_material(0, smat);

        add_child(mi);
        return mi;
    };

    if (!both_eyes_overlay)  both_eyes_overlay  = create_quad_overlay("Both_Eyes", -1, -0.055f);
    if (!left_eye_overlay)   left_eye_overlay   = create_quad_overlay("Left_Eye",   0, -0.052f);
    if (!right_eye_overlay)  right_eye_overlay  = create_quad_overlay("Right_Eye",  1, -0.050f);
}

void XREyeFXNode3D::_cache_default_overlay_parameters() {
    auto cache_from = [this](godot::MeshInstance3D *p_overlay, godot::Dictionary &r_floats, godot::Dictionary &r_colors) {
        godot::ShaderMaterial *smat = _get_overlay_shader_material(p_overlay);
        if (!smat) return;

        godot::Array f_keys = float_resolutions.keys();
        for (int i = 0; i < f_keys.size(); ++i) {
            godot::StringName k = f_keys[i];
            godot::Variant v = smat->get_shader_parameter(k);
            if (v.get_type() == godot::Variant::FLOAT || v.get_type() == godot::Variant::INT) {
                r_floats[k] = static_cast<float>(v);
            }
        }

        godot::Array c_keys = color_resolutions.keys();
        for (int i = 0; i < c_keys.size(); ++i) {
            godot::StringName k = c_keys[i];
            godot::Variant v = smat->get_shader_parameter(k);
            if (v.get_type() == godot::Variant::COLOR) {
                r_colors[k] = static_cast<godot::Color>(v);
            }
        }
    };

    cache_from(both_eyes_overlay, default_both_floats, default_both_colors);
    cache_from(left_eye_overlay, default_left_floats, default_left_colors);
    cache_from(right_eye_overlay, default_right_floats, default_right_colors);
}

void XREyeFXNode3D::_resolve_and_apply_target(
    XREyeEffectTarget p_target,
    godot::MeshInstance3D *p_overlay,
    const godot::Dictionary &p_default_floats,
    const godot::Dictionary &p_default_colors
) {
    godot::ShaderMaterial *smat = _get_overlay_shader_material(p_overlay);
    if (!smat) return;

    struct FloatAccum {
        float value = 0.0f;
        int highest_priority = -1;
        int count = 0;
        int weight_sum = 0;
    };
    struct ColorAccum {
        godot::Color value = godot::Color(0.0f, 0.0f, 0.0f, 0.0f);
        int highest_priority = -1;
        int count = 0;
        int weight_sum = 0;
    };

    godot::Dictionary float_accums;
    godot::Dictionary color_accums;

    // 1. Reset to defaults first
    godot::Array def_f_keys = p_default_floats.keys();
    for (int i = 0; i < def_f_keys.size(); ++i) {
        godot::StringName k = def_f_keys[i];
        smat->set_shader_parameter(k, p_default_floats[k]);
    }
    godot::Array def_c_keys = p_default_colors.keys();
    for (int i = 0; i < def_c_keys.size(); ++i) {
        godot::StringName k = def_c_keys[i];
        smat->set_shader_parameter(k, p_default_colors[k]);
    }

    // 2. Accumulate active effects targeting `p_target`
    const int fx_count = effects.size();
    for (int i = 0; i < fx_count; ++i) {
        godot::Ref<XREyeEffectResource> fx = effects[i];
        if (!fx.is_valid() || !fx->get_enabled() || fx->get_effect_target() != p_target) {
            continue;
        }

        const float eval_t = (i < effect_values.size()) ? effect_values[i] : fx->get_current_value();
        if (eval_t <= 0.0001f) {
            continue;
        }

        const int prio = fx->get_effect_priority();
        const int prio_weight = prio + 1;
        godot::Dictionary evaluated = fx->evaluate_at(eval_t);
        godot::Dictionary f_vals = evaluated["floats"];
        godot::Dictionary c_vals = evaluated["colors"];

        // Floats
        godot::Array f_keys = f_vals.keys();
        for (int k = 0; k < f_keys.size(); ++k) {
            godot::StringName param = f_keys[k];
            const float sample_v = static_cast<float>(f_vals[param]);
            XREyeEffectResolutionMode mode = fx->get_float_resolution_mode();
            if (float_resolutions.has(param)) {
                mode = static_cast<XREyeEffectResolutionMode>(static_cast<int>(float_resolutions[param]));
            }

            godot::Dictionary state = float_accums.has(param) ? godot::Dictionary(float_accums[param]) : godot::Dictionary();
            float cur_val = state.has("val") ? static_cast<float>(state["val"]) : 0.0f;
            int cur_prio  = state.has("prio") ? static_cast<int>(state["prio"]) : -1;
            int cur_cnt   = state.has("cnt") ? static_cast<int>(state["cnt"]) : 0;
            int cur_wsum  = state.has("wsum") ? static_cast<int>(state["wsum"]) : 0;

            switch (mode) {
                case XREyeEffectResolutionMode::EYE_RESOLUTION_PRIORITY:
                    if (prio >= cur_prio) {
                        cur_val = sample_v;
                        cur_prio = prio;
                        cur_cnt = 1;
                    }
                    break;
                case XREyeEffectResolutionMode::EYE_RESOLUTION_ADD:
                case XREyeEffectResolutionMode::EYE_RESOLUTION_AVERAGE:
                    cur_val += sample_v;
                    cur_cnt += 1;
                    break;
                case XREyeEffectResolutionMode::EYE_RESOLUTION_PRIORITY_AVERAGE:
                    cur_val += sample_v * static_cast<float>(prio_weight);
                    cur_wsum += prio_weight;
                    cur_cnt += 1;
                    break;
            }

            state["val"]  = cur_val;
            state["prio"] = cur_prio;
            state["cnt"]  = cur_cnt;
            state["wsum"] = cur_wsum;
            state["mode"] = static_cast<int>(mode);
            float_accums[param] = state;
        }

        // Colors
        godot::Array c_keys = c_vals.keys();
        for (int k = 0; k < c_keys.size(); ++k) {
            godot::StringName param = c_keys[k];
            const godot::Color sample_c = c_vals[param];
            XREyeEffectResolutionMode mode = fx->get_color_resolution_mode();
            if (color_resolutions.has(param)) {
                mode = static_cast<XREyeEffectResolutionMode>(static_cast<int>(color_resolutions[param]));
            }

            godot::Dictionary state = color_accums.has(param) ? godot::Dictionary(color_accums[param]) : godot::Dictionary();
            godot::Color cur_col = state.has("val") ? static_cast<godot::Color>(state["val"]) : godot::Color(0.0f, 0.0f, 0.0f, 0.0f);
            int cur_prio = state.has("prio") ? static_cast<int>(state["prio"]) : -1;
            int cur_cnt  = state.has("cnt") ? static_cast<int>(state["cnt"]) : 0;
            int cur_wsum = state.has("wsum") ? static_cast<int>(state["wsum"]) : 0;

            switch (mode) {
                case XREyeEffectResolutionMode::EYE_RESOLUTION_PRIORITY:
                    if (prio >= cur_prio) {
                        cur_col = sample_c;
                        cur_prio = prio;
                        cur_cnt = 1;
                    }
                    break;
                case XREyeEffectResolutionMode::EYE_RESOLUTION_ADD:
                case XREyeEffectResolutionMode::EYE_RESOLUTION_AVERAGE:
                    cur_col += sample_c;
                    cur_cnt += 1;
                    break;
                case XREyeEffectResolutionMode::EYE_RESOLUTION_PRIORITY_AVERAGE:
                    cur_col += sample_c * static_cast<float>(prio_weight);
                    cur_wsum += prio_weight;
                    cur_cnt += 1;
                    break;
            }

            state["val"]  = cur_col;
            state["prio"] = cur_prio;
            state["cnt"]  = cur_cnt;
            state["wsum"] = cur_wsum;
            state["mode"] = static_cast<int>(mode);
            color_accums[param] = state;
        }
    }

    // 3. Finalize averages and write to ShaderMaterial
    godot::Array res_f_keys = float_accums.keys();
    for (int i = 0; i < res_f_keys.size(); ++i) {
        godot::StringName param = res_f_keys[i];
        godot::Dictionary state = float_accums[param];
        float final_val = static_cast<float>(state["val"]);
        int cnt = static_cast<int>(state["cnt"]);
        int wsum = static_cast<int>(state["wsum"]);
        auto mode = static_cast<XREyeEffectResolutionMode>(static_cast<int>(state["mode"]));

        if (mode == XREyeEffectResolutionMode::EYE_RESOLUTION_AVERAGE && cnt > 1) {
            final_val /= static_cast<float>(cnt);
        } else if (mode == XREyeEffectResolutionMode::EYE_RESOLUTION_PRIORITY_AVERAGE && wsum > 0) {
            final_val /= static_cast<float>(wsum);
        }
        smat->set_shader_parameter(param, final_val);
    }

    godot::Array res_c_keys = color_accums.keys();
    for (int i = 0; i < res_c_keys.size(); ++i) {
        godot::StringName param = res_c_keys[i];
        godot::Dictionary state = color_accums[param];
        godot::Color final_col = state["val"];
        int cnt = static_cast<int>(state["cnt"]);
        int wsum = static_cast<int>(state["wsum"]);
        auto mode = static_cast<XREyeEffectResolutionMode>(static_cast<int>(state["mode"]));

        if (mode == XREyeEffectResolutionMode::EYE_RESOLUTION_AVERAGE && cnt > 1) {
            final_col /= static_cast<float>(cnt);
        } else if (mode == XREyeEffectResolutionMode::EYE_RESOLUTION_PRIORITY_AVERAGE && wsum > 0) {
            final_col /= static_cast<float>(wsum);
        }
        smat->set_shader_parameter(param, final_col);
    }
}

void XREyeFXNode3D::apply_all_effects() {
    _resolve_and_apply_target(XREyeEffectTarget::EYE_TARGET_BOTH, both_eyes_overlay, default_both_floats, default_both_colors);
    _resolve_and_apply_target(XREyeEffectTarget::EYE_TARGET_LEFT, left_eye_overlay, default_left_floats, default_left_colors);
    _resolve_and_apply_target(XREyeEffectTarget::EYE_TARGET_RIGHT, right_eye_overlay, default_right_floats, default_right_colors);
}

void XREyeFXNode3D::blink(float p_duration) {
    if (effects.is_empty()) {
        add_effect(XREyeEffectResource::create_blink_effect(p_duration * 0.5f));
    }
    tween_effect(0, 1.0f, std::max(0.02f, p_duration * 0.5f));
}

void XREyeFXNode3D::fade_out(float p_duration) {
    if (effects.is_empty()) {
        add_effect(XREyeEffectResource::create_fade_effect(p_duration));
    }
    tween_effect(0, 1.0f, p_duration);
}

void XREyeFXNode3D::fade_in(float p_duration) {
    if (effects.is_empty()) {
        add_effect(XREyeEffectResource::create_fade_effect(p_duration));
    }
    tween_effect(0, 0.0f, p_duration);
}

} // namespace ideam::godot_ext
