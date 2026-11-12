#include "spline_tweener.h"

#include <godot_cpp/classes/canvas_item.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/geometry_instance3d.hpp>
#include <godot_cpp/classes/h_separator.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

#include <algorithm>
#include <cmath>

namespace ideam::godot_ext {

// ============================================================================
// SplineTweenTrack Implementation
// ============================================================================

void SplineTweenTrack::_bind_methods() {
    using namespace godot;

    BIND_ENUM_CONSTANT(TWEEN_TRACK_BLEND_SHAPE);
    BIND_ENUM_CONSTANT(TWEEN_TRACK_PROPERTY_LERP);
    BIND_ENUM_CONSTANT(TWEEN_TRACK_SHADER_PARAMETER);
    BIND_ENUM_CONSTANT(TWEEN_TRACK_VECTOR3_OFFSET);
    BIND_ENUM_CONSTANT(TWEEN_TRACK_COLOR_GRADIENT);

    ClassDB::bind_method(D_METHOD("set_track_name", "name"), &SplineTweenTrack::set_track_name);
    ClassDB::bind_method(D_METHOD("get_track_name"), &SplineTweenTrack::get_track_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "track_name"), "set_track_name", "get_track_name");

    ClassDB::bind_method(D_METHOD("set_enabled", "enabled"), &SplineTweenTrack::set_enabled);
    ClassDB::bind_method(D_METHOD("is_enabled"), &SplineTweenTrack::is_enabled);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enabled"), "set_enabled", "is_enabled");

    ClassDB::bind_method(D_METHOD("set_track_mode", "mode"), &SplineTweenTrack::set_track_mode);
    ClassDB::bind_method(D_METHOD("get_track_mode"), &SplineTweenTrack::get_track_mode);
    ADD_PROPERTY(
        PropertyInfo(
            Variant::INT,
            "track_mode",
            PROPERTY_HINT_ENUM,
            "BlendShape Weight:0,Property Lerp:1,Shader Uniform:2,Spline3D Vector Offset:3,Spline4D Color Gradient:4"
        ),
        "set_track_mode",
        "get_track_mode"
    );

    ClassDB::bind_method(D_METHOD("set_target_subpath", "path"), &SplineTweenTrack::set_target_subpath);
    ClassDB::bind_method(D_METHOD("get_target_subpath"), &SplineTweenTrack::get_target_subpath);
    ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "target_subpath"), "set_target_subpath", "get_target_subpath");

    ClassDB::bind_method(D_METHOD("set_target_key", "key"), &SplineTweenTrack::set_target_key);
    ClassDB::bind_method(D_METHOD("get_target_key"), &SplineTweenTrack::get_target_key);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "target_key"), "set_target_key", "get_target_key");

    ClassDB::bind_method(D_METHOD("set_weight_scale", "scale"), &SplineTweenTrack::set_weight_scale);
    ClassDB::bind_method(D_METHOD("get_weight_scale"), &SplineTweenTrack::get_weight_scale);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "weight_scale"), "set_weight_scale", "get_weight_scale");

    ClassDB::bind_method(D_METHOD("set_weight_bias", "bias"), &SplineTweenTrack::set_weight_bias);
    ClassDB::bind_method(D_METHOD("get_weight_bias"), &SplineTweenTrack::get_weight_bias);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "weight_bias"), "set_weight_bias", "get_weight_bias");

    ClassDB::bind_method(D_METHOD("set_from_value", "val"), &SplineTweenTrack::set_from_value);
    ClassDB::bind_method(D_METHOD("get_from_value"), &SplineTweenTrack::get_from_value);
    ADD_PROPERTY(PropertyInfo(Variant::NIL, "from_value", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT), "set_from_value", "get_from_value");

    ClassDB::bind_method(D_METHOD("set_to_value", "val"), &SplineTweenTrack::set_to_value);
    ClassDB::bind_method(D_METHOD("get_to_value"), &SplineTweenTrack::get_to_value);
    ADD_PROPERTY(PropertyInfo(Variant::NIL, "to_value", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT), "set_to_value", "get_to_value");

    ClassDB::bind_method(D_METHOD("set_curve_1d", "curve"), &SplineTweenTrack::set_curve_1d);
    ClassDB::bind_method(D_METHOD("get_curve_1d"), &SplineTweenTrack::get_curve_1d);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "curve_1d", PROPERTY_HINT_RESOURCE_TYPE, "Spline1DResource"),
        "set_curve_1d",
        "get_curve_1d"
    );

    ClassDB::bind_method(D_METHOD("set_offset_spline_3d", "spline"), &SplineTweenTrack::set_offset_spline_3d);
    ClassDB::bind_method(D_METHOD("get_offset_spline_3d"), &SplineTweenTrack::get_offset_spline_3d);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "offset_spline_3d", PROPERTY_HINT_RESOURCE_TYPE, "Spline3DResource"),
        "set_offset_spline_3d",
        "get_offset_spline_3d"
    );

    ClassDB::bind_method(D_METHOD("set_color_spline_4d", "spline"), &SplineTweenTrack::set_color_spline_4d);
    ClassDB::bind_method(D_METHOD("get_color_spline_4d"), &SplineTweenTrack::get_color_spline_4d);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "color_spline_4d", PROPERTY_HINT_RESOURCE_TYPE, "Spline4DResource"),
        "set_color_spline_4d",
        "get_color_spline_4d"
    );

    ClassDB::bind_method(
        D_METHOD("sample_weight", "percent", "weighted"),
        &SplineTweenTrack::sample_weight,
        DEFVAL(true)
    );
    ClassDB::bind_method(
        D_METHOD("sample_evaluated_variant", "percent", "weighted"),
        &SplineTweenTrack::sample_evaluated_variant,
        DEFVAL(true)
    );
    ClassDB::bind_method(
        D_METHOD("apply_to_root", "root", "percent", "weighted"),
        &SplineTweenTrack::apply_to_root,
        DEFVAL(true)
    );

    ClassDB::bind_method(D_METHOD("_on_sub_spline_changed"), &SplineTweenTrack::_on_sub_spline_changed);
}

SplineTweenTrack::SplineTweenTrack() {
    set_curve_1d(Spline1DResource::create_zero_to_one());
}

SplineTweenTrack::~SplineTweenTrack() {
    const godot::Callable cb(this, "_on_sub_spline_changed");
    if (curve_1d_.is_valid() && curve_1d_->is_connected("changed", cb)) {
        curve_1d_->disconnect("changed", cb);
    }
    if (offset_spline_3d_.is_valid() && offset_spline_3d_->is_connected("changed", cb)) {
        offset_spline_3d_->disconnect("changed", cb);
    }
    if (color_spline_4d_.is_valid() && color_spline_4d_->is_connected("changed", cb)) {
        color_spline_4d_->disconnect("changed", cb);
    }
}

void SplineTweenTrack::_on_sub_spline_changed() {
    emit_signal("geometry_changed");
    emit_changed();
}

godot::Dictionary SplineTweenTrack::get_snapshot_state() const {
    godot::Dictionary d;
    d["track_name"]     = track_name_;
    d["enabled"]        = enabled_;
    d["track_mode"]     = static_cast<int32_t>(track_mode_);
    d["target_subpath"] = target_subpath_;
    d["target_key"]     = target_key_;
    d["weight_scale"]   = weight_scale_;
    d["weight_bias"]    = weight_bias_;
    d["from_value"]     = from_value_;
    d["to_value"]       = to_value_;
    if (curve_1d_.is_valid()) {
        d["curve_1d_state"] = curve_1d_->get_snapshot_state();
    }
    return d;
}

void SplineTweenTrack::apply_snapshot_state(const godot::Dictionary &p_state) {
    if (p_state.has("track_name")) track_name_ = p_state["track_name"];
    if (p_state.has("enabled")) enabled_ = static_cast<bool>(p_state["enabled"]);
    if (p_state.has("track_mode")) track_mode_ = static_cast<SplineTweenTrackMode>(static_cast<int32_t>(p_state["track_mode"]));
    if (p_state.has("target_subpath")) target_subpath_ = p_state["target_subpath"];
    if (p_state.has("target_key")) target_key_ = p_state["target_key"];
    if (p_state.has("weight_scale")) weight_scale_ = static_cast<real_t>(p_state["weight_scale"]);
    if (p_state.has("weight_bias")) weight_bias_ = static_cast<real_t>(p_state["weight_bias"]);
    if (p_state.has("from_value")) from_value_ = p_state["from_value"];
    if (p_state.has("to_value")) to_value_ = p_state["to_value"];
    if (p_state.has("curve_1d_state") && curve_1d_.is_valid()) {
        curve_1d_->apply_snapshot_state(p_state["curve_1d_state"]);
    }
    _on_sub_spline_changed();
}

void SplineTweenTrack::set_track_name(const godot::String &p_name) {
    track_name_ = p_name;
    _on_sub_spline_changed();
}

void SplineTweenTrack::set_enabled(bool p_enabled) {
    enabled_ = p_enabled;
    _on_sub_spline_changed();
}

void SplineTweenTrack::set_track_mode(SplineTweenTrackMode p_mode) {
    track_mode_ = p_mode;
    _on_sub_spline_changed();
}

void SplineTweenTrack::set_target_subpath(const godot::NodePath &p_path) {
    target_subpath_ = p_path;
    _on_sub_spline_changed();
}

void SplineTweenTrack::set_target_key(const godot::StringName &p_key) {
    target_key_ = p_key;
    if (track_name_ == "Track" && !godot::String(p_key).is_empty()) {
        track_name_ = godot::String(p_key);
    }
    _on_sub_spline_changed();
}

void SplineTweenTrack::set_weight_scale(real_t p_scale) {
    weight_scale_ = p_scale;
    _on_sub_spline_changed();
}

void SplineTweenTrack::set_weight_bias(real_t p_bias) {
    weight_bias_ = p_bias;
    _on_sub_spline_changed();
}

void SplineTweenTrack::set_from_value(const godot::Variant &p_val) {
    from_value_ = p_val;
    _on_sub_spline_changed();
}

void SplineTweenTrack::set_to_value(const godot::Variant &p_val) {
    to_value_ = p_val;
    _on_sub_spline_changed();
}

void SplineTweenTrack::set_curve_1d(const godot::Ref<Spline1DResource> &p_curve) {
    if (curve_1d_ == p_curve) return;
    const godot::Callable cb(this, "_on_sub_spline_changed");
    if (curve_1d_.is_valid() && curve_1d_->is_connected("changed", cb)) {
        curve_1d_->disconnect("changed", cb);
    }
    curve_1d_ = p_curve;
    if (curve_1d_.is_valid() && !curve_1d_->is_connected("changed", cb)) {
        curve_1d_->connect("changed", cb);
    }
    _on_sub_spline_changed();
}

void SplineTweenTrack::set_offset_spline_3d(const godot::Ref<Spline3DResource> &p_spline) {
    if (offset_spline_3d_ == p_spline) return;
    const godot::Callable cb(this, "_on_sub_spline_changed");
    if (offset_spline_3d_.is_valid() && offset_spline_3d_->is_connected("changed", cb)) {
        offset_spline_3d_->disconnect("changed", cb);
    }
    offset_spline_3d_ = p_spline;
    if (offset_spline_3d_.is_valid() && !offset_spline_3d_->is_connected("changed", cb)) {
        offset_spline_3d_->connect("changed", cb);
    }
    _on_sub_spline_changed();
}

void SplineTweenTrack::set_color_spline_4d(const godot::Ref<Spline4DResource> &p_spline) {
    if (color_spline_4d_ == p_spline) return;
    const godot::Callable cb(this, "_on_sub_spline_changed");
    if (color_spline_4d_.is_valid() && color_spline_4d_->is_connected("changed", cb)) {
        color_spline_4d_->disconnect("changed", cb);
    }
    color_spline_4d_ = p_spline;
    if (color_spline_4d_.is_valid() && !color_spline_4d_->is_connected("changed", cb)) {
        color_spline_4d_->connect("changed", cb);
    }
    _on_sub_spline_changed();
}

real_t SplineTweenTrack::sample_weight(real_t p_percent, bool p_weighted) const {
    const real_t raw = (curve_1d_.is_valid() && curve_1d_->get_point_count() > 0)
        ? curve_1d_->sample(p_percent, p_weighted)
        : p_percent;
    return raw * weight_scale_ + weight_bias_;
}

godot::Variant SplineTweenTrack::sample_evaluated_variant(real_t p_percent, bool p_weighted) const {
    using namespace godot;
    const real_t w = sample_weight(p_percent, p_weighted);

    if (track_mode_ == TWEEN_TRACK_VECTOR3_OFFSET) {
        if (offset_spline_3d_.is_valid() && offset_spline_3d_->get_point_count() > 0) {
            const real_t clamped_t = std::clamp(w, static_cast<real_t>(0.0), static_cast<real_t>(1.0));
            return offset_spline_3d_->sample(clamped_t, p_weighted) * static_cast<float>(weight_scale_);
        }
        return Vector3(0.0f, static_cast<float>(w), 0.0f);
    }

    if (track_mode_ == TWEEN_TRACK_COLOR_GRADIENT) {
        if (color_spline_4d_.is_valid() && color_spline_4d_->get_point_count() > 0) {
            const real_t clamped_t = std::clamp(w, static_cast<real_t>(0.0), static_cast<real_t>(1.0));
            return color_spline_4d_->sample_color(clamped_t, p_weighted);
        }
        return Color(static_cast<float>(w), static_cast<float>(w), static_cast<float>(w), 1.0f);
    }

    if (track_mode_ == TWEEN_TRACK_BLEND_SHAPE) {
        return static_cast<float>(w);
    }

    // Interpolate between from_value_ and to_value_ when types match
    if (from_value_.get_type() == to_value_.get_type()) {
        const float wf = static_cast<float>(w);
        switch (from_value_.get_type()) {
            case Variant::FLOAT:
            case Variant::INT: {
                const double a = static_cast<double>(from_value_);
                const double b = static_cast<double>(to_value_);
                return a + (b - a) * static_cast<double>(w);
            }
            case Variant::VECTOR2: {
                const Vector2 a = from_value_;
                const Vector2 b = to_value_;
                return a + (b - a) * wf;
            }
            case Variant::VECTOR3: {
                const Vector3 a = from_value_;
                const Vector3 b = to_value_;
                return a + (b - a) * wf;
            }
            case Variant::VECTOR4: {
                const Vector4 a = from_value_;
                const Vector4 b = to_value_;
                return a + (b - a) * wf;
            }
            case Variant::COLOR: {
                const Color a = from_value_;
                const Color b = to_value_;
                return Color(
                    a.r + (b.r - a.r) * wf,
                    a.g + (b.g - a.g) * wf,
                    a.b + (b.b - a.b) * wf,
                    a.a + (b.a - a.a) * wf
                );
            }
            default:
                break;
        }
    }
    return static_cast<float>(w);
}

void SplineTweenTrack::apply_to_root(godot::Node *p_root, real_t p_percent, bool p_weighted) const {
    using namespace godot;
    if (!enabled_ || !p_root) return;

    Node *target = (target_subpath_.is_empty() || String(target_subpath_) == ".")
        ? p_root
        : p_root->get_node_or_null(target_subpath_);
    if (!target) return;

    const real_t w = sample_weight(p_percent, p_weighted);

    switch (track_mode_) {
        case TWEEN_TRACK_BLEND_SHAPE: {
            MeshInstance3D *mi = Object::cast_to<MeshInstance3D>(target);
            if (!mi || target_key_.is_empty()) break;
            const int32_t bs_idx = mi->find_blend_shape_by_name(target_key_);
            if (bs_idx >= 0) {
                mi->set_blend_shape_value(bs_idx, static_cast<float>(w));
            }
            break;
        }
        case TWEEN_TRACK_PROPERTY_LERP:
        case TWEEN_TRACK_VECTOR3_OFFSET:
        case TWEEN_TRACK_COLOR_GRADIENT: {
            if (target_key_.is_empty()) {
                if (track_mode_ == TWEEN_TRACK_VECTOR3_OFFSET) {
                    if (Node3D *n3d = Object::cast_to<Node3D>(target)) {
                        n3d->set_position(sample_evaluated_variant(p_percent, p_weighted));
                    }
                }
                break;
            }
            const Variant val = sample_evaluated_variant(p_percent, p_weighted);
            target->set_indexed(NodePath(String(target_key_)), val);
            break;
        }
        case TWEEN_TRACK_SHADER_PARAMETER: {
            if (target_key_.is_empty()) break;
            const Variant val = sample_evaluated_variant(p_percent, p_weighted);
            Ref<ShaderMaterial> smat;
            if (GeometryInstance3D *gi = Object::cast_to<GeometryInstance3D>(target)) {
                smat = gi->get_material_override();
                if (smat.is_null()) {
                    if (MeshInstance3D *mi = Object::cast_to<MeshInstance3D>(target)) {
                        smat = mi->get_active_material(0);
                    }
                }
            } else if (CanvasItem *ci = Object::cast_to<CanvasItem>(target)) {
                smat = ci->get_material();
            }
            if (smat.is_valid()) {
                smat->set_shader_parameter(target_key_, val);
            }
            break;
        }
        default:
            break;
    }
}

// ============================================================================
// SplineTweenResource Implementation
// ============================================================================

void SplineTweenResource::_bind_methods() {
    using namespace godot;

    ADD_GROUP("Spline Tween", "");

    ClassDB::bind_method(D_METHOD("set_duration", "duration"), &SplineTweenResource::set_duration);
    ClassDB::bind_method(D_METHOD("get_duration"), &SplineTweenResource::get_duration);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "duration"), "set_duration", "get_duration");

    ClassDB::bind_method(D_METHOD("set_loop_mode", "mode"), &SplineTweenResource::set_loop_mode);
    ClassDB::bind_method(D_METHOD("get_loop_mode"), &SplineTweenResource::get_loop_mode);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "loop_mode", PROPERTY_HINT_ENUM, "Once:0,Wrap:1,Ping-Pong:2"),
        "set_loop_mode",
        "get_loop_mode"
    );

    ClassDB::bind_method(D_METHOD("set_use_weighted_time", "weighted"), &SplineTweenResource::set_use_weighted_time);
    ClassDB::bind_method(D_METHOD("get_use_weighted_time"), &SplineTweenResource::get_use_weighted_time);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_weighted_time"), "set_use_weighted_time", "get_use_weighted_time");

    ClassDB::bind_method(D_METHOD("set_tracks", "tracks"), &SplineTweenResource::set_tracks);
    ClassDB::bind_method(D_METHOD("get_tracks"), &SplineTweenResource::get_tracks);
    ADD_PROPERTY(
        PropertyInfo(Variant::ARRAY, "tracks", PROPERTY_HINT_ARRAY_TYPE, "SplineTweenTrack"),
        "set_tracks",
        "get_tracks"
    );

    ClassDB::bind_method(D_METHOD("get_track_count"), &SplineTweenResource::get_track_count);
    ClassDB::bind_method(D_METHOD("get_track", "index"), &SplineTweenResource::get_track);
    ClassDB::bind_method(D_METHOD("add_track", "track"), &SplineTweenResource::add_track);
    ClassDB::bind_method(
        D_METHOD("create_and_add_track", "name", "mode", "target_key"),
        &SplineTweenResource::create_and_add_track,
        DEFVAL(TWEEN_TRACK_BLEND_SHAPE),
        DEFVAL(StringName(""))
    );
    ClassDB::bind_method(D_METHOD("remove_track", "index"), &SplineTweenResource::remove_track);
    ClassDB::bind_method(D_METHOD("clear_tracks"), &SplineTweenResource::clear_tracks);
    ClassDB::bind_method(D_METHOD("evaluate_and_apply", "root", "percent"), &SplineTweenResource::evaluate_and_apply);

    // --- Undo/Redo Commit Wrappers ---
    ClassDB::bind_method(
        D_METHOD("commit_create_and_add_track", "name", "mode", "target_key"),
        &SplineTweenResource::commit_create_and_add_track,
        DEFVAL(TWEEN_TRACK_BLEND_SHAPE),
        DEFVAL(StringName(""))
    );
    ClassDB::bind_method(D_METHOD("commit_remove_track", "index"), &SplineTweenResource::commit_remove_track);
    ClassDB::bind_method(D_METHOD("commit_clear_tracks"), &SplineTweenResource::commit_clear_tracks);
    ClassDB::bind_method(
        D_METHOD("commit_populate_blend_shapes_from_mesh", "mesh_instance"),
        &SplineTweenResource::commit_populate_blend_shapes_from_mesh
    );

    ClassDB::bind_method(D_METHOD("_on_track_changed"), &SplineTweenResource::_on_track_changed);
}

SplineTweenResource::~SplineTweenResource() {
    for (const auto &tr : tracks_) {
        _disconnect_track(tr);
    }
}

void SplineTweenResource::_connect_track(const godot::Ref<SplineTweenTrack> &p_track) {
    if (p_track.is_null()) return;
    const godot::Callable cb(this, "_on_track_changed");
    if (!p_track->is_connected("changed", cb)) {
        p_track->connect("changed", cb);
    }
}

void SplineTweenResource::_disconnect_track(const godot::Ref<SplineTweenTrack> &p_track) {
    if (p_track.is_null()) return;
    const godot::Callable cb(this, "_on_track_changed");
    if (p_track->is_connected("changed", cb)) {
        p_track->disconnect("changed", cb);
    }
}

void SplineTweenResource::_on_track_changed() {
    emit_signal("geometry_changed");
    emit_changed();
}

godot::Dictionary SplineTweenResource::get_snapshot_state() const {
    godot::Dictionary d;
    d["duration"]          = duration_;
    d["loop_mode"]         = static_cast<int32_t>(loop_mode_);
    d["use_weighted_time"] = use_weighted_time_;

    godot::Array tr_states;
    for (const auto &tr : tracks_) {
        if (tr.is_valid()) {
            tr_states.push_back(tr->get_snapshot_state());
        }
    }
    d["track_states"] = tr_states;
    return d;
}

void SplineTweenResource::apply_snapshot_state(const godot::Dictionary &p_state) {
    if (p_state.has("duration")) duration_ = static_cast<real_t>(p_state["duration"]);
    if (p_state.has("loop_mode")) loop_mode_ = static_cast<SplineLoopMode>(static_cast<int32_t>(p_state["loop_mode"]));
    if (p_state.has("use_weighted_time")) use_weighted_time_ = static_cast<bool>(p_state["use_weighted_time"]);

    if (p_state.has("track_states")) {
        for (const auto &tr : tracks_) {
            _disconnect_track(tr);
        }
        tracks_.clear();
        const godot::Array arr = p_state["track_states"];
        for (int64_t i = 0; i < arr.size(); ++i) {
            godot::Ref<SplineTweenTrack> tr;
            tr.instantiate();
            tr->apply_snapshot_state(arr[i]);
            _connect_track(tr);
            tracks_.push_back(tr);
        }
    }
    _on_track_changed();
}

void SplineTweenResource::set_tracks(const godot::TypedArray<SplineTweenTrack> &p_tracks) {
    for (const auto &tr : tracks_) {
        _disconnect_track(tr);
    }
    tracks_.clear();
    tracks_.reserve(static_cast<size_t>(p_tracks.size()));
    for (int64_t i = 0; i < p_tracks.size(); ++i) {
        godot::Ref<SplineTweenTrack> tr = p_tracks[i];
        if (tr.is_valid()) {
            _connect_track(tr);
            tracks_.push_back(tr);
        }
    }
    _on_track_changed();
}

godot::TypedArray<SplineTweenTrack> SplineTweenResource::get_tracks() const {
    godot::TypedArray<SplineTweenTrack> out;
    out.resize(static_cast<int64_t>(tracks_.size()));
    for (size_t i = 0; i < tracks_.size(); ++i) {
        out[static_cast<int64_t>(i)] = tracks_[i];
    }
    return out;
}

godot::Ref<SplineTweenTrack> SplineTweenResource::get_track(int p_index) const {
    if (p_index < 0 || p_index >= static_cast<int>(tracks_.size())) return godot::Ref<SplineTweenTrack>();
    return tracks_[static_cast<size_t>(p_index)];
}

void SplineTweenResource::add_track(const godot::Ref<SplineTweenTrack> &p_track) {
    if (p_track.is_null()) return;
    _connect_track(p_track);
    tracks_.push_back(p_track);
    _on_track_changed();
}

godot::Ref<SplineTweenTrack> SplineTweenResource::create_and_add_track(
    const godot::String &p_name,
    SplineTweenTrackMode p_mode,
    const godot::StringName &p_target_key
) {
    godot::Ref<SplineTweenTrack> tr;
    tr.instantiate();
    tr->set_track_name(p_name);
    tr->set_track_mode(p_mode);
    tr->set_target_key(p_target_key);
    add_track(tr);
    return tr;
}

void SplineTweenResource::remove_track(int p_index) {
    if (p_index < 0 || p_index >= static_cast<int>(tracks_.size())) return;
    _disconnect_track(tracks_[static_cast<size_t>(p_index)]);
    tracks_.erase(tracks_.begin() + p_index);
    _on_track_changed();
}

void SplineTweenResource::clear_tracks() {
    if (tracks_.empty()) return;
    for (const auto &tr : tracks_) {
        _disconnect_track(tr);
    }
    tracks_.clear();
    _on_track_changed();
}

void SplineTweenResource::set_duration(real_t p_duration) {
    duration_ = std::max(static_cast<real_t>(0.01), p_duration);
    _on_track_changed();
}

void SplineTweenResource::set_loop_mode(SplineLoopMode p_mode) {
    loop_mode_ = p_mode;
    _on_track_changed();
}

void SplineTweenResource::set_use_weighted_time(bool p_weighted) {
    use_weighted_time_ = p_weighted;
    _on_track_changed();
}

void SplineTweenResource::evaluate_and_apply(godot::Node *p_root, real_t p_percent) const {
    if (!p_root) return;
    const real_t clamped_t = std::clamp(p_percent, static_cast<real_t>(0.0), static_cast<real_t>(1.0));
    for (const auto &tr : tracks_) {
        if (tr.is_valid() && tr->is_enabled()) {
            tr->apply_to_root(p_root, clamped_t, use_weighted_time_);
        }
    }
}

godot::Ref<SplineTweenTrack> SplineTweenResource::commit_create_and_add_track(
    const godot::String &p_name,
    SplineTweenTrackMode p_mode,
    const godot::StringName &p_target_key
) {
    const godot::Dictionary before = get_snapshot_state();
    godot::Ref<SplineTweenTrack> tr = create_and_add_track(p_name, p_mode, p_target_key);
    commit_snapshot_action("Add Spline Tween Track", before, get_snapshot_state());
    return tr;
}

void SplineTweenResource::commit_remove_track(int p_index) {
    if (p_index < 0 || p_index >= get_track_count()) return;
    const godot::Dictionary before = get_snapshot_state();
    remove_track(p_index);
    commit_snapshot_action("Remove Spline Tween Track", before, get_snapshot_state());
}

void SplineTweenResource::commit_clear_tracks() {
    if (tracks_.empty()) return;
    const godot::Dictionary before = get_snapshot_state();
    clear_tracks();
    commit_snapshot_action("Clear Spline Tween Tracks", before, get_snapshot_state());
}

int SplineTweenResource::commit_populate_blend_shapes_from_mesh(godot::MeshInstance3D *p_mesh_instance) {
    if (!p_mesh_instance || p_mesh_instance->get_mesh().is_null()) return 0;
    godot::Ref<godot::Mesh> mesh = p_mesh_instance->get_mesh();
    const int bs_count = mesh->get_blend_shape_count();
    if (bs_count <= 0) return 0;

    const godot::Dictionary before = get_snapshot_state();
    int added = 0;
    for (int i = 0; i < bs_count; ++i) {
        const godot::StringName bs_name = mesh->get_blend_shape_name(i);
        bool exists = false;
        for (const auto &tr : tracks_) {
            if (tr.is_valid() && tr->get_track_mode() == TWEEN_TRACK_BLEND_SHAPE && tr->get_target_key() == bs_name) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            create_and_add_track(godot::String(bs_name), TWEEN_TRACK_BLEND_SHAPE, bs_name);
            ++added;
        }
    }
    if (added > 0) {
        commit_snapshot_action("Populate BlendShape Spline Tracks", before, get_snapshot_state());
    }
    return added;
}

// ============================================================================
// SplineTweener (SceneTree Node) Implementation
// ============================================================================

void SplineTweener::_bind_methods() {
    using namespace godot;

    ADD_SIGNAL(MethodInfo("tween_stepped", PropertyInfo(Variant::FLOAT, "percent")));
    ADD_SIGNAL(MethodInfo("tween_finished"));

    ClassDB::bind_method(D_METHOD("set_tween_resource", "tween"), &SplineTweener::set_tween_resource);
    ClassDB::bind_method(D_METHOD("get_tween_resource"), &SplineTweener::get_tween_resource);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "tween_resource", PROPERTY_HINT_RESOURCE_TYPE, "SplineTweenResource"),
        "set_tween_resource",
        "get_tween_resource"
    );

    ClassDB::bind_method(D_METHOD("set_target_root_path", "path"), &SplineTweener::set_target_root_path);
    ClassDB::bind_method(D_METHOD("get_target_root_path"), &SplineTweener::get_target_root_path);
    ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "target_root_path"), "set_target_root_path", "get_target_root_path");

    ClassDB::bind_method(D_METHOD("set_process_mode_type", "mode"), &SplineTweener::set_process_mode_type);
    ClassDB::bind_method(D_METHOD("get_process_mode_type"), &SplineTweener::get_process_mode_type);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "process_mode_type", PROPERTY_HINT_ENUM, "Idle:0,Physics:1,Manual:2"),
        "set_process_mode_type",
        "get_process_mode_type"
    );

    ClassDB::bind_method(D_METHOD("set_playing", "playing"), &SplineTweener::set_playing);
    ClassDB::bind_method(D_METHOD("is_playing"), &SplineTweener::is_playing);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "playing"), "set_playing", "is_playing");

    ClassDB::bind_method(D_METHOD("set_autoplay", "autoplay"), &SplineTweener::set_autoplay);
    ClassDB::bind_method(D_METHOD("get_autoplay"), &SplineTweener::get_autoplay);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "autoplay"), "set_autoplay", "get_autoplay");

    ClassDB::bind_method(D_METHOD("set_speed_scale", "speed"), &SplineTweener::set_speed_scale);
    ClassDB::bind_method(D_METHOD("get_speed_scale"), &SplineTweener::get_speed_scale);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "speed_scale"), "set_speed_scale", "get_speed_scale");

    ClassDB::bind_method(D_METHOD("set_current_percent", "percent"), &SplineTweener::set_current_percent);
    ClassDB::bind_method(D_METHOD("get_current_percent"), &SplineTweener::get_current_percent);
    ADD_PROPERTY(
        PropertyInfo(Variant::FLOAT, "current_percent", PROPERTY_HINT_RANGE, "0.0,1.0,0.001"),
        "set_current_percent",
        "get_current_percent"
    );

    ClassDB::bind_method(D_METHOD("play", "from_start"), &SplineTweener::play, DEFVAL(false));
    ClassDB::bind_method(D_METHOD("pause"), &SplineTweener::pause);
    ClassDB::bind_method(D_METHOD("stop", "reset"), &SplineTweener::stop, DEFVAL(true));
    ClassDB::bind_method(D_METHOD("step", "delta_seconds"), &SplineTweener::step);
    ClassDB::bind_method(
        D_METHOD("populate_blend_shape_tracks_from_target"),
        &SplineTweener::populate_blend_shape_tracks_from_target
    );

    ClassDB::bind_method(D_METHOD("_on_tween_resource_changed"), &SplineTweener::_on_tween_resource_changed);
}

SplineTweener::SplineTweener() {
    tween_resource_.instantiate();
    tween_resource_->connect("changed", godot::Callable(this, "_on_tween_resource_changed"));
}

SplineTweener::~SplineTweener() {
    if (tween_resource_.is_valid()) {
        const godot::Callable cb(this, "_on_tween_resource_changed");
        if (tween_resource_->is_connected("changed", cb)) {
            tween_resource_->disconnect("changed", cb);
        }
    }
}

void SplineTweener::_ready() {
    if (autoplay_ && !godot::Engine::get_singleton()->is_editor_hint()) {
        play(true);
    } else {
        _update_process_callbacks();
        _apply_current_percent();
    }
}

void SplineTweener::_process(double p_delta) {
    if (playing_ && process_mode_type_ == SPLINE_PROCESS_IDLE) {
        step(static_cast<real_t>(p_delta));
    }
}

void SplineTweener::_physics_process(double p_delta) {
    if (playing_ && process_mode_type_ == SPLINE_PROCESS_PHYSICS) {
        step(static_cast<real_t>(p_delta));
    }
}

void SplineTweener::_on_tween_resource_changed() {
    _apply_current_percent();
}

void SplineTweener::_update_process_callbacks() {
    set_process(playing_ && process_mode_type_ == SPLINE_PROCESS_IDLE);
    set_physics_process(playing_ && process_mode_type_ == SPLINE_PROCESS_PHYSICS);
}

godot::Node *SplineTweener::resolve_target_root() const {
    if (!is_inside_tree()) return nullptr;
    if (target_root_path_.is_empty()) return get_parent();
    return get_node_or_null(target_root_path_);
}

void SplineTweener::_apply_current_percent() {
    if (tween_resource_.is_null()) return;
    if (godot::Node *root = resolve_target_root()) {
        tween_resource_->evaluate_and_apply(root, current_percent_);
    }
    emit_signal("tween_stepped", current_percent_);
}

void SplineTweener::set_tween_resource(const godot::Ref<SplineTweenResource> &p_tween) {
    if (tween_resource_ == p_tween) return;
    const godot::Callable cb(this, "_on_tween_resource_changed");
    if (tween_resource_.is_valid() && tween_resource_->is_connected("changed", cb)) {
        tween_resource_->disconnect("changed", cb);
    }
    tween_resource_ = p_tween;
    if (tween_resource_.is_valid() && !tween_resource_->is_connected("changed", cb)) {
        tween_resource_->connect("changed", cb);
    }
    _apply_current_percent();
}

void SplineTweener::set_target_root_path(const godot::NodePath &p_path) {
    target_root_path_ = p_path;
    _apply_current_percent();
}

void SplineTweener::set_process_mode_type(SplineProcessMode p_mode) {
    process_mode_type_ = p_mode;
    _update_process_callbacks();
}

void SplineTweener::set_playing(bool p_playing) {
    playing_ = p_playing;
    _update_process_callbacks();
}

void SplineTweener::set_autoplay(bool p_autoplay) {
    autoplay_ = p_autoplay;
}

void SplineTweener::set_speed_scale(real_t p_speed) {
    speed_scale_ = p_speed;
}

void SplineTweener::set_current_percent(real_t p_percent) {
    current_percent_ = std::clamp(p_percent, static_cast<real_t>(0.0), static_cast<real_t>(1.0));
    _apply_current_percent();
}

void SplineTweener::play(bool p_from_start) {
    if (p_from_start) {
        current_percent_ = static_cast<real_t>(0.0);
        ping_pong_dir_   = static_cast<real_t>(1.0);
    }
    set_playing(true);
    _apply_current_percent();
}

void SplineTweener::pause() {
    set_playing(false);
}

void SplineTweener::stop(bool p_reset) {
    set_playing(false);
    if (p_reset) {
        current_percent_ = static_cast<real_t>(0.0);
        ping_pong_dir_   = static_cast<real_t>(1.0);
        _apply_current_percent();
    }
}

void SplineTweener::step(real_t p_delta_seconds) {
    if (tween_resource_.is_null()) return;
    const real_t dur  = std::max(static_cast<real_t>(0.01), tween_resource_->get_duration());
    const real_t step_p = (p_delta_seconds * speed_scale_) / dur;
    const SplineLoopMode lmode = tween_resource_->get_loop_mode();

    real_t next_p = current_percent_ + step_p * ping_pong_dir_;
    if (lmode == SPLINE_LOOP_ONCE) {
        if (next_p >= static_cast<real_t>(1.0)) {
            current_percent_ = static_cast<real_t>(1.0);
            _apply_current_percent();
            set_playing(false);
            emit_signal("tween_finished");
            return;
        } else if (next_p <= static_cast<real_t>(0.0)) {
            current_percent_ = static_cast<real_t>(0.0);
        } else {
            current_percent_ = next_p;
        }
    } else if (lmode == SPLINE_LOOP_WRAP) {
        next_p = std::fmod(next_p, static_cast<real_t>(1.0));
        if (next_p < static_cast<real_t>(0.0)) next_p += static_cast<real_t>(1.0);
        current_percent_ = next_p;
    } else {
        // Ping-Pong
        if (next_p >= static_cast<real_t>(1.0)) {
            next_p         = static_cast<real_t>(2.0) - next_p;
            ping_pong_dir_ = static_cast<real_t>(-1.0);
        } else if (next_p <= static_cast<real_t>(0.0)) {
            next_p         = -next_p;
            ping_pong_dir_ = static_cast<real_t>(1.0);
        }
        current_percent_ = std::clamp(next_p, static_cast<real_t>(0.0), static_cast<real_t>(1.0));
    }

    _apply_current_percent();
}

int SplineTweener::populate_blend_shape_tracks_from_target() {
    if (tween_resource_.is_null()) return 0;
    godot::MeshInstance3D *mi = godot::Object::cast_to<godot::MeshInstance3D>(resolve_target_root());
    if (!mi) return 0;
    const int added = tween_resource_->commit_populate_blend_shapes_from_mesh(mi);
    _apply_current_percent();
    return added;
}

// ============================================================================
// SplineTweenerControl Implementation
// ============================================================================

void SplineTweenerControl::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_tween_resource", "tween"), &SplineTweenerControl::set_tween_resource);
    ClassDB::bind_method(D_METHOD("get_tween_resource"), &SplineTweenerControl::get_tween_resource);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "tween_resource", PROPERTY_HINT_RESOURCE_TYPE, "SplineTweenResource"),
        "set_tween_resource",
        "get_tween_resource"
    );

    ClassDB::bind_method(D_METHOD("refresh_ui"), &SplineTweenerControl::refresh_ui);

    ClassDB::bind_method(D_METHOD("_on_tween_changed"), &SplineTweenerControl::_on_tween_changed);
    ClassDB::bind_method(D_METHOD("_on_scrub_changed", "val"), &SplineTweenerControl::_on_scrub_changed);
    ClassDB::bind_method(D_METHOD("_on_duration_changed", "val"), &SplineTweenerControl::_on_duration_changed);
    ClassDB::bind_method(D_METHOD("_on_loop_mode_selected", "idx"), &SplineTweenerControl::_on_loop_mode_selected);
    ClassDB::bind_method(D_METHOD("_on_weighted_toggled", "weighted"), &SplineTweenerControl::_on_weighted_toggled);

    ClassDB::bind_method(D_METHOD("_on_play_pressed"), &SplineTweenerControl::_on_play_pressed);
    ClassDB::bind_method(D_METHOD("_on_pause_pressed"), &SplineTweenerControl::_on_pause_pressed);
    ClassDB::bind_method(D_METHOD("_on_stop_pressed"), &SplineTweenerControl::_on_stop_pressed);

    ClassDB::bind_method(D_METHOD("_on_add_blend_shape_track_pressed"), &SplineTweenerControl::_on_add_blend_shape_track_pressed);
    ClassDB::bind_method(D_METHOD("_on_add_property_track_pressed"), &SplineTweenerControl::_on_add_property_track_pressed);
    ClassDB::bind_method(D_METHOD("_on_add_shader_track_pressed"), &SplineTweenerControl::_on_add_shader_track_pressed);
    ClassDB::bind_method(D_METHOD("_on_add_offset_3d_track_pressed"), &SplineTweenerControl::_on_add_offset_3d_track_pressed);
    ClassDB::bind_method(D_METHOD("_on_import_target_blend_shapes_pressed"), &SplineTweenerControl::_on_import_target_blend_shapes_pressed);
    ClassDB::bind_method(D_METHOD("_on_clear_tracks_pressed"), &SplineTweenerControl::_on_clear_tracks_pressed);

    ClassDB::bind_method(D_METHOD("_on_track_enabled_toggled", "enabled", "track_idx"), &SplineTweenerControl::_on_track_enabled_toggled);
    ClassDB::bind_method(D_METHOD("_on_track_mode_selected", "mode_idx", "track_idx"), &SplineTweenerControl::_on_track_mode_selected);
    ClassDB::bind_method(D_METHOD("_on_track_key_submitted", "key", "track_idx"), &SplineTweenerControl::_on_track_key_submitted);
    ClassDB::bind_method(D_METHOD("_on_track_scale_changed", "scale", "track_idx"), &SplineTweenerControl::_on_track_scale_changed);
    ClassDB::bind_method(D_METHOD("_on_track_curve_toggle_pressed", "track_idx"), &SplineTweenerControl::_on_track_curve_toggle_pressed);
    ClassDB::bind_method(D_METHOD("_on_remove_track_pressed", "track_idx"), &SplineTweenerControl::_on_remove_track_pressed);
}

SplineTweenerControl::SplineTweenerControl() {
    set_h_size_flags(Control::SIZE_EXPAND_FILL);
    _build_ui();
}

SplineTweenerControl::~SplineTweenerControl() {
    if (tween_resource_.is_valid()) {
        const godot::Callable cb(this, "_on_tween_changed");
        if (tween_resource_->is_connected("changed", cb)) {
            tween_resource_->disconnect("changed", cb);
        }
    }
}

void SplineTweenerControl::_build_ui() {
    using namespace godot;

    // 1. Master Timeline Scrubber & Preview Transport Bar
    HBoxContainer *transport_bar = memnew(HBoxContainer);
    transport_bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    add_child(transport_bar);

    Button *play_btn = memnew(Button);
    play_btn->set_text("▶ Play");
    play_btn->connect("pressed", Callable(this, "_on_play_pressed"));
    transport_bar->add_child(play_btn);

    Button *pause_btn = memnew(Button);
    pause_btn->set_text("⏸");
    pause_btn->connect("pressed", Callable(this, "_on_pause_pressed"));
    transport_bar->add_child(pause_btn);

    Button *stop_btn = memnew(Button);
    stop_btn->set_text("⏹");
    stop_btn->connect("pressed", Callable(this, "_on_stop_pressed"));
    transport_bar->add_child(stop_btn);

    Label *t_lbl = memnew(Label);
    t_lbl->set_text("  t:");
    transport_bar->add_child(t_lbl);

    scrub_slider_ = memnew(HSlider);
    scrub_slider_->set_min(0.0);
    scrub_slider_->set_max(1.0);
    scrub_slider_->set_step(0.005);
    scrub_slider_->set_value(0.0);
    scrub_slider_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    scrub_slider_->connect("value_changed", Callable(this, "_on_scrub_changed"));
    transport_bar->add_child(scrub_slider_);

    scrub_spin_ = memnew(SpinBox);
    scrub_spin_->set_min(0.0);
    scrub_spin_->set_max(1.0);
    scrub_spin_->set_step(0.01);
    scrub_spin_->set_value(0.0);
    scrub_spin_->connect("value_changed", Callable(this, "_on_scrub_changed"));
    transport_bar->add_child(scrub_spin_);

    duration_spin_ = memnew(SpinBox);
    duration_spin_->set_min(0.05);
    duration_spin_->set_max(120.0);
    duration_spin_->set_step(0.05);
    duration_spin_->set_value(1.0);
    duration_spin_->set_suffix("s");
    duration_spin_->connect("value_changed", Callable(this, "_on_duration_changed"));
    transport_bar->add_child(duration_spin_);

    loop_mode_opt_ = memnew(OptionButton);
    loop_mode_opt_->add_item("Once", SPLINE_LOOP_ONCE);
    loop_mode_opt_->add_item("Wrap", SPLINE_LOOP_WRAP);
    loop_mode_opt_->add_item("Ping-Pong", SPLINE_LOOP_PING_PONG);
    loop_mode_opt_->select(1);
    loop_mode_opt_->connect("item_selected", Callable(this, "_on_loop_mode_selected"));
    transport_bar->add_child(loop_mode_opt_);

    weighted_check_ = memnew(CheckBox);
    weighted_check_->set_text("Weighted %");
    weighted_check_->set_pressed(true);
    weighted_check_->connect("toggled", Callable(this, "_on_weighted_toggled"));
    transport_bar->add_child(weighted_check_);

    // 2. Track Management & BlendShape Discovery Toolbar
    HBoxContainer *add_bar = memnew(HBoxContainer);
    add_bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    add_child(add_bar);

    Button *add_bs_btn = memnew(Button);
    add_bs_btn->set_text("+ BlendShape");
    add_bs_btn->connect("pressed", Callable(this, "_on_add_blend_shape_track_pressed"));
    add_bar->add_child(add_bs_btn);

    Button *add_prop_btn = memnew(Button);
    add_prop_btn->set_text("+ Property");
    add_prop_btn->connect("pressed", Callable(this, "_on_add_property_track_pressed"));
    add_bar->add_child(add_prop_btn);

    Button *add_shd_btn = memnew(Button);
    add_shd_btn->set_text("+ Shader Param");
    add_shd_btn->connect("pressed", Callable(this, "_on_add_shader_track_pressed"));
    add_bar->add_child(add_shd_btn);

    Button *add_off_btn = memnew(Button);
    add_off_btn->set_text("+ 3D Offset");
    add_off_btn->connect("pressed", Callable(this, "_on_add_offset_3d_track_pressed"));
    add_bar->add_child(add_off_btn);

    Button *import_bs_btn = memnew(Button);
    import_bs_btn->set_text("Auto-Import Target BlendShapes");
    import_bs_btn->connect("pressed", Callable(this, "_on_import_target_blend_shapes_pressed"));
    add_bar->add_child(import_bs_btn);

    Button *clr_btn = memnew(Button);
    clr_btn->set_text("Clear All");
    clr_btn->connect("pressed", Callable(this, "_on_clear_tracks_pressed"));
    add_bar->add_child(clr_btn);

    // 3. Dynamic Multi-Track Stack
    tracks_vbox_ = memnew(VBoxContainer);
    tracks_vbox_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    add_child(tracks_vbox_);

    // 4. Summary Footer
    summary_label_ = memnew(Label);
    add_child(summary_label_);
}

void SplineTweenerControl::set_tween_resource(const godot::Ref<SplineTweenResource> &p_tween) {
    if (tween_resource_ == p_tween) return;
    const godot::Callable cb(this, "_on_tween_changed");
    if (tween_resource_.is_valid() && tween_resource_->is_connected("changed", cb)) {
        tween_resource_->disconnect("changed", cb);
    }
    tween_resource_ = p_tween;
    if (tween_resource_.is_valid() && !tween_resource_->is_connected("changed", cb)) {
        tween_resource_->connect("changed", cb);
    }
    refresh_ui();
}

void SplineTweenerControl::set_bound_tweener_node(SplineTweener *p_node) {
    bound_tweener_node_ = p_node;
    if (bound_tweener_node_ && bound_tweener_node_->get_tween_resource().is_valid()) {
        set_tween_resource(bound_tweener_node_->get_tween_resource());
    }
}

void SplineTweenerControl::_on_tween_changed() {
    if (!is_updating_ui_) {
        _update_summary_label();
    }
}

void SplineTweenerControl::refresh_ui() {
    is_updating_ui_ = true;

    if (tween_resource_.is_valid()) {
        if (duration_spin_) duration_spin_->set_value(tween_resource_->get_duration());
        if (loop_mode_opt_) loop_mode_opt_->select(static_cast<int>(tween_resource_->get_loop_mode()));
        if (weighted_check_) weighted_check_->set_pressed(tween_resource_->get_use_weighted_time());
    }
    if (bound_tweener_node_) {
        const double p = static_cast<double>(bound_tweener_node_->get_current_percent());
        if (scrub_slider_) scrub_slider_->set_value(p);
        if (scrub_spin_) scrub_spin_->set_value(p);
    }

    _rebuild_track_cards();
    _update_summary_label();

    is_updating_ui_ = false;
}

void SplineTweenerControl::_rebuild_track_cards() {
    using namespace godot;
    if (!tracks_vbox_) return;

    while (tracks_vbox_->get_child_count() > 0) {
        Node *child = tracks_vbox_->get_child(0);
        tracks_vbox_->remove_child(child);
        memdelete(child);
    }

    if (tween_resource_.is_null()) return;

    const int n_tracks = tween_resource_->get_track_count();
    const real_t cur_t = scrub_slider_ ? static_cast<real_t>(scrub_slider_->get_value()) : static_cast<real_t>(0.0);

    for (int i = 0; i < n_tracks; ++i) {
        Ref<SplineTweenTrack> tr = tween_resource_->get_track(i);
        if (tr.is_null()) continue;

        VBoxContainer *card = memnew(VBoxContainer);
        card->set_h_size_flags(Control::SIZE_EXPAND_FILL);
        tracks_vbox_->add_child(card);

        HBoxContainer *hdr = memnew(HBoxContainer);
        hdr->set_h_size_flags(Control::SIZE_EXPAND_FILL);
        card->add_child(hdr);

        CheckBox *en_chk = memnew(CheckBox);
        en_chk->set_text("#" + String::num_int64(i));
        en_chk->set_pressed(tr->is_enabled());
        en_chk->connect("toggled", Callable(this, "_on_track_enabled_toggled").bind(i));
        hdr->add_child(en_chk);

        OptionButton *mode_opt = memnew(OptionButton);
        mode_opt->add_item("BlendShape", TWEEN_TRACK_BLEND_SHAPE);
        mode_opt->add_item("Property", TWEEN_TRACK_PROPERTY_LERP);
        mode_opt->add_item("Shader Param", TWEEN_TRACK_SHADER_PARAMETER);
        mode_opt->add_item("3D Offset", TWEEN_TRACK_VECTOR3_OFFSET);
        mode_opt->add_item("Color 4D", TWEEN_TRACK_COLOR_GRADIENT);
        mode_opt->select(static_cast<int>(tr->get_track_mode()));
        mode_opt->connect("item_selected", Callable(this, "_on_track_mode_selected").bind(i));
        hdr->add_child(mode_opt);

        LineEdit *key_edit = memnew(LineEdit);
        key_edit->set_placeholder("BlendShape / Property / Uniform Name");
        key_edit->set_text(String(tr->get_target_key()));
        key_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
        key_edit->connect("text_changed", Callable(this, "_on_track_key_submitted").bind(i));
        hdr->add_child(key_edit);

        SpinBox *scale_spin = memnew(SpinBox);
        scale_spin->set_min(-100.0);
        scale_spin->set_max(100.0);
        scale_spin->set_step(0.05);
        scale_spin->set_value(tr->get_weight_scale());
        scale_spin->set_prefix("×");
        scale_spin->connect("value_changed", Callable(this, "_on_track_scale_changed").bind(i));
        hdr->add_child(scale_spin);

        Label *val_lbl = memnew(Label);
        val_lbl->set_text(" w=" + String::num(static_cast<double>(tr->sample_weight(cur_t)), 2) + " ");
        hdr->add_child(val_lbl);

        Button *toggle_curve_btn = memnew(Button);
        toggle_curve_btn->set_text("Curve ▾");
        toggle_curve_btn->connect("pressed", Callable(this, "_on_track_curve_toggle_pressed").bind(i));
        hdr->add_child(toggle_curve_btn);

        Button *rem_btn = memnew(Button);
        rem_btn->set_text("✕");
        rem_btn->connect("pressed", Callable(this, "_on_remove_track_pressed").bind(i));
        hdr->add_child(rem_btn);

        // Embedded Spline1DControl for sculpting this track's weight curve and section completion weights
        Spline1DControl *s1d_ctrl = memnew(Spline1DControl);
        s1d_ctrl->set_name("TrackSpline1DControl");
        s1d_ctrl->set_spline(tr->get_curve_1d());
        card->add_child(s1d_ctrl);

        HSeparator *sep = memnew(HSeparator);
        card->add_child(sep);
    }
}

void SplineTweenerControl::_update_summary_label() {
    using namespace godot;
    if (!summary_label_) return;
    if (tween_resource_.is_null()) {
        summary_label_->set_text("No SplineTweenResource bound.");
        return;
    }
    const int n_tracks = tween_resource_->get_track_count();
    const double cur_t = scrub_slider_ ? scrub_slider_->get_value() : 0.0;
    summary_label_->set_text(
        "Tracks: " + String::num_int64(n_tracks) +
        "  |  Completion t = " + String::num(cur_t * 100.0, 1) + "%" +
        "  |  Duration: " + String::num(static_cast<double>(tween_resource_->get_duration()), 2) + "s"
    );
}

void SplineTweenerControl::_on_scrub_changed(double p_val) {
    if (is_updating_ui_) return;
    is_updating_ui_ = true;
    if (scrub_slider_) scrub_slider_->set_value(p_val);
    if (scrub_spin_) scrub_spin_->set_value(p_val);

    if (bound_tweener_node_) {
        bound_tweener_node_->set_current_percent(static_cast<real_t>(p_val));
    }
    _update_summary_label();
    is_updating_ui_ = false;
}

void SplineTweenerControl::_on_duration_changed(double p_val) {
    if (is_updating_ui_ || tween_resource_.is_null()) return;
    tween_resource_->set_duration(static_cast<real_t>(p_val));
}

void SplineTweenerControl::_on_loop_mode_selected(int p_idx) {
    if (is_updating_ui_ || tween_resource_.is_null() || !loop_mode_opt_) return;
    tween_resource_->set_loop_mode(static_cast<SplineLoopMode>(loop_mode_opt_->get_item_id(p_idx)));
}

void SplineTweenerControl::_on_weighted_toggled(bool p_weighted) {
    if (is_updating_ui_ || tween_resource_.is_null()) return;
    tween_resource_->set_use_weighted_time(p_weighted);
}

void SplineTweenerControl::_on_play_pressed() {
    if (bound_tweener_node_) bound_tweener_node_->play(false);
}

void SplineTweenerControl::_on_pause_pressed() {
    if (bound_tweener_node_) bound_tweener_node_->pause();
}

void SplineTweenerControl::_on_stop_pressed() {
    if (bound_tweener_node_) {
        bound_tweener_node_->stop(true);
        refresh_ui();
    }
}

void SplineTweenerControl::_on_add_blend_shape_track_pressed() {
    if (tween_resource_.is_null()) return;
    const int idx = tween_resource_->get_track_count();
    const godot::String name = "BlendShape_" + godot::String::num_int64(idx);
    tween_resource_->commit_create_and_add_track(name, TWEEN_TRACK_BLEND_SHAPE, godot::StringName(name));
    refresh_ui();
}

void SplineTweenerControl::_on_add_property_track_pressed() {
    if (tween_resource_.is_null()) return;
    tween_resource_->commit_create_and_add_track(
        "PropertyTrack",
        TWEEN_TRACK_PROPERTY_LERP,
        godot::StringName("scale")
    );
    refresh_ui();
}

void SplineTweenerControl::_on_add_shader_track_pressed() {
    if (tween_resource_.is_null()) return;
    tween_resource_->commit_create_and_add_track(
        "ShaderParamTrack",
        TWEEN_TRACK_SHADER_PARAMETER,
        godot::StringName("dissolve_amount")
    );
    refresh_ui();
}

void SplineTweenerControl::_on_add_offset_3d_track_pressed() {
    if (tween_resource_.is_null()) return;
    godot::Ref<SplineTweenTrack> tr = tween_resource_->commit_create_and_add_track(
        "Spline3DOffsetTrack",
        TWEEN_TRACK_VECTOR3_OFFSET,
        godot::StringName("position")
    );
    if (tr.is_valid() && tr->get_offset_spline_3d().is_null()) {
        tr->set_offset_spline_3d(Spline3DResource::create_line(
            godot::Vector3(0.0f, 0.0f, 0.0f),
            godot::Vector3(0.0f, 1.5f, 0.0f),
            3
        ));
    }
    refresh_ui();
}

void SplineTweenerControl::_on_import_target_blend_shapes_pressed() {
    if (bound_tweener_node_) {
        bound_tweener_node_->populate_blend_shape_tracks_from_target();
        refresh_ui();
    }
}

void SplineTweenerControl::_on_clear_tracks_pressed() {
    if (tween_resource_.is_null()) return;
    tween_resource_->commit_clear_tracks();
    refresh_ui();
}

void SplineTweenerControl::_on_track_enabled_toggled(bool p_enabled, int p_track_idx) {
    if (is_updating_ui_ || tween_resource_.is_null()) return;
    godot::Ref<SplineTweenTrack> tr = tween_resource_->get_track(p_track_idx);
    if (tr.is_valid()) tr->set_enabled(p_enabled);
}

void SplineTweenerControl::_on_track_mode_selected(int p_mode_idx, int p_track_idx) {
    if (is_updating_ui_ || tween_resource_.is_null()) return;
    godot::Ref<SplineTweenTrack> tr = tween_resource_->get_track(p_track_idx);
    if (tr.is_valid()) tr->set_track_mode(static_cast<SplineTweenTrackMode>(p_mode_idx));
}

void SplineTweenerControl::_on_track_key_submitted(const godot::String &p_key, int p_track_idx) {
    if (is_updating_ui_ || tween_resource_.is_null()) return;
    godot::Ref<SplineTweenTrack> tr = tween_resource_->get_track(p_track_idx);
    if (tr.is_valid()) tr->set_target_key(godot::StringName(p_key));
}

void SplineTweenerControl::_on_track_scale_changed(double p_scale, int p_track_idx) {
    if (is_updating_ui_ || tween_resource_.is_null()) return;
    godot::Ref<SplineTweenTrack> tr = tween_resource_->get_track(p_track_idx);
    if (tr.is_valid()) tr->set_weight_scale(static_cast<real_t>(p_scale));
}

void SplineTweenerControl::_on_track_curve_toggle_pressed(int p_track_idx) {
    if (!tracks_vbox_ || p_track_idx < 0 || p_track_idx >= tracks_vbox_->get_child_count()) return;
    godot::Node *card = tracks_vbox_->get_child(p_track_idx);
    if (!card) return;
    godot::Control *ctrl = godot::Object::cast_to<godot::Control>(card->get_node_or_null("TrackSpline1DControl"));
    if (ctrl) {
        ctrl->set_visible(!ctrl->is_visible());
    }
}

void SplineTweenerControl::_on_remove_track_pressed(int p_track_idx) {
    if (tween_resource_.is_null()) return;
    tween_resource_->commit_remove_track(p_track_idx);
    refresh_ui();
}

// ============================================================================
// SplineTweenResourceInspector Implementation
// ============================================================================

bool SplineTweenResourceInspector::_can_handle(godot::Object *p_object) {
    if (!p_object) return false;
    return godot::Object::cast_to<SplineTweenResource>(p_object) != nullptr ||
           godot::Object::cast_to<SplineTweener>(p_object) != nullptr;
}

godot::Control *SplineTweenResourceInspector::_create_inspector_control(godot::Object *p_object) {
    if (SplineTweener *tweener_node = godot::Object::cast_to<SplineTweener>(p_object)) {
        SplineTweenerControl *ctrl = memnew(SplineTweenerControl);
        ctrl->set_bound_tweener_node(tweener_node);
        return ctrl;
    }
    if (SplineTweenResource *tween_res = godot::Object::cast_to<SplineTweenResource>(p_object)) {
        SplineTweenerControl *ctrl = memnew(SplineTweenerControl);
        ctrl->set_tween_resource(godot::Ref<SplineTweenResource>(tween_res));
        return ctrl;
    }
    return nullptr;
}

bool SplineTweenResourceInspector::_is_managed_property(
    godot::Object *p_object,
    const godot::String &p_name
) const {
    (void)p_object;
    return p_name == "tracks";
}

} // namespace ideam::godot_ext