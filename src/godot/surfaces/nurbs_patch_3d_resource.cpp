#include "nurbs_patch_3d_resource.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

#include <algorithm>
#include <vector>

namespace ideam::godot_ext {

void NurbsPatch3DResource::_bind_methods() {
    using namespace godot;

    ADD_GROUP("NURBS Grid", "");

    ClassDB::bind_method(D_METHOD("set_count_u", "count_u"), &NurbsPatch3DResource::set_count_u);
    ClassDB::bind_method(D_METHOD("get_count_u"), &NurbsPatch3DResource::get_count_u);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "count_u", PROPERTY_HINT_RANGE, "2,64,1"), "set_count_u", "get_count_u");

    ClassDB::bind_method(D_METHOD("set_count_v", "count_v"), &NurbsPatch3DResource::set_count_v);
    ClassDB::bind_method(D_METHOD("get_count_v"), &NurbsPatch3DResource::get_count_v);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "count_v", PROPERTY_HINT_RANGE, "2,64,1"), "set_count_v", "get_count_v");

    ClassDB::bind_method(D_METHOD("set_degree_u", "degree"), &NurbsPatch3DResource::set_degree_u);
    ClassDB::bind_method(D_METHOD("get_degree_u"), &NurbsPatch3DResource::get_degree_u);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "degree_u", PROPERTY_HINT_RANGE, "1,7,1"), "set_degree_u", "get_degree_u");

    ClassDB::bind_method(D_METHOD("set_degree_v", "degree"), &NurbsPatch3DResource::set_degree_v);
    ClassDB::bind_method(D_METHOD("get_degree_v"), &NurbsPatch3DResource::get_degree_v);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "degree_v", PROPERTY_HINT_RANGE, "1,7,1"), "set_degree_v", "get_degree_v");

    ClassDB::bind_method(D_METHOD("set_wrap_u", "wrap"), &NurbsPatch3DResource::set_wrap_u);
    ClassDB::bind_method(D_METHOD("get_wrap_u"), &NurbsPatch3DResource::get_wrap_u);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "wrap_u"), "set_wrap_u", "get_wrap_u");

    ClassDB::bind_method(D_METHOD("set_wrap_v", "wrap"), &NurbsPatch3DResource::set_wrap_v);
    ClassDB::bind_method(D_METHOD("get_wrap_v"), &NurbsPatch3DResource::get_wrap_v);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "wrap_v"), "set_wrap_v", "get_wrap_v");

    ClassDB::bind_method(D_METHOD("set_control_points_array", "points"), &NurbsPatch3DResource::set_control_points_array);
    ClassDB::bind_method(D_METHOD("get_control_points_array"), &NurbsPatch3DResource::get_control_points_array);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_VECTOR3_ARRAY, "control_points"), "set_control_points_array", "get_control_points_array");

    ClassDB::bind_method(D_METHOD("set_weights_array", "weights"), &NurbsPatch3DResource::set_weights_array);
    ClassDB::bind_method(D_METHOD("get_weights_array"), &NurbsPatch3DResource::get_weights_array);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "weights"), "set_weights_array", "get_weights_array");

    ADD_GROUP("Spline3D Lofting", "");

    ClassDB::bind_method(D_METHOD("set_loft_ribs", "ribs"), &NurbsPatch3DResource::set_loft_ribs);
    ClassDB::bind_method(D_METHOD("get_loft_ribs"), &NurbsPatch3DResource::get_loft_ribs);
    ADD_PROPERTY(
        PropertyInfo(Variant::ARRAY, "loft_ribs", PROPERTY_HINT_ARRAY_TYPE, "Spline3DResource"),
        "set_loft_ribs",
        "get_loft_ribs"
    );

    ClassDB::bind_method(D_METHOD("set_loft_samples_u", "samples"), &NurbsPatch3DResource::set_loft_samples_u);
    ClassDB::bind_method(D_METHOD("get_loft_samples_u"), &NurbsPatch3DResource::get_loft_samples_u);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "loft_samples_u", PROPERTY_HINT_RANGE, "2,64,1"), "set_loft_samples_u", "get_loft_samples_u");

    ClassDB::bind_method(D_METHOD("set_loft_arc_length", "arc_length"), &NurbsPatch3DResource::set_loft_arc_length);
    ClassDB::bind_method(D_METHOD("get_loft_arc_length"), &NurbsPatch3DResource::get_loft_arc_length);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "loft_arc_length"), "set_loft_arc_length", "get_loft_arc_length");

    ClassDB::bind_method(D_METHOD("set_control_point", "u", "v", "position", "weight"), &NurbsPatch3DResource::set_control_point, DEFVAL(1.0));
    ClassDB::bind_method(D_METHOD("get_control_point", "u", "v"), &NurbsPatch3DResource::get_control_point);
    ClassDB::bind_method(D_METHOD("set_control_weight", "u", "v", "weight"), &NurbsPatch3DResource::set_control_weight);
    ClassDB::bind_method(D_METHOD("get_control_weight", "u", "v"), &NurbsPatch3DResource::get_control_weight);
    ClassDB::bind_method(D_METHOD("commit_set_control_point", "u", "v", "position"), &NurbsPatch3DResource::commit_set_control_point);
    ClassDB::bind_method(D_METHOD("commit_apply_snapshot", "before", "action_name"), &NurbsPatch3DResource::commit_apply_snapshot, DEFVAL("Edit NURBS Patch"));

    ClassDB::bind_method(D_METHOD("rebuild_from_loft_ribs"), &NurbsPatch3DResource::rebuild_from_loft_ribs);
    ClassDB::bind_method(D_METHOD("setup_flat_grid", "width", "depth", "count_u", "count_v"), &NurbsPatch3DResource::setup_flat_grid, DEFVAL(2.0), DEFVAL(2.0), DEFVAL(4), DEFVAL(4));
    ClassDB::bind_method(D_METHOD("setup_cylinder_patch", "radius", "height", "arc_radians", "count_u", "count_v"), &NurbsPatch3DResource::setup_cylinder_patch, DEFVAL(1.0), DEFVAL(2.0), DEFVAL(6.2831853), DEFVAL(9), DEFVAL(4));
    ClassDB::bind_method(D_METHOD("setup_dome_patch", "radius", "height", "count_u", "count_v"), &NurbsPatch3DResource::setup_dome_patch, DEFVAL(1.0), DEFVAL(1.0), DEFVAL(5), DEFVAL(5));

    ClassDB::bind_method(D_METHOD("_on_rib_spline_changed"), &NurbsPatch3DResource::_on_rib_spline_changed);
}

NurbsPatch3DResource::NurbsPatch3DResource() {
    core_patch_.setup_flat_grid(static_cast<real_t>(2.0), static_cast<real_t>(2.0), 4u, 4u);
    pending_count_u_ = 4;
    pending_count_v_ = 4;
}

NurbsPatch3DResource::~NurbsPatch3DResource() {
    _disconnect_rib_signals();
}

void NurbsPatch3DResource::_apply_pending_grid() {
    const int nu = pending_count_u_ > 1 ? pending_count_u_ : static_cast<int>(core_patch_.get_count_u());
    const int nv = pending_count_v_ > 1 ? pending_count_v_ : static_cast<int>(core_patch_.get_count_v());
    const int64_t expected = static_cast<int64_t>(nu) * nv;
    if (nu < 2 || nv < 2 || pending_control_points_.size() != expected) {
        return;
    }

    std::vector<Vec3> pts(static_cast<size_t>(expected));
    for (int64_t i = 0; i < expected; ++i) {
        pts[static_cast<size_t>(i)] = _to_glm(pending_control_points_[i]);
    }

    std::vector<real_t> wts;
    if (pending_weights_.size() == expected) {
        wts.resize(static_cast<size_t>(expected));
        for (int64_t i = 0; i < expected; ++i) {
            wts[static_cast<size_t>(i)] = static_cast<real_t>(pending_weights_[i]);
        }
    }

    core_patch_.set_control_points_bulk(static_cast<uint32_t>(nu), static_cast<uint32_t>(nv), pts, wts);
    emit_changed();
}

void NurbsPatch3DResource::_connect_rib_signals() {
    const godot::Callable cb(this, "_on_rib_spline_changed");
    for (int64_t i = 0; i < loft_ribs_.size(); ++i) {
        godot::Ref<Spline3DResource> rib = loft_ribs_[i];
        if (rib.is_valid() && !rib->is_connected("changed", cb)) {
            rib->connect("changed", cb);
        }
    }
}

void NurbsPatch3DResource::_disconnect_rib_signals() {
    const godot::Callable cb(this, "_on_rib_spline_changed");
    for (int64_t i = 0; i < loft_ribs_.size(); ++i) {
        godot::Ref<Spline3DResource> rib = loft_ribs_[i];
        if (rib.is_valid() && rib->is_connected("changed", cb)) {
            rib->disconnect("changed", cb);
        }
    }
}

void NurbsPatch3DResource::_on_rib_spline_changed() {
    rebuild_from_loft_ribs();
}

godot::Dictionary NurbsPatch3DResource::get_snapshot_state() const {
    godot::Dictionary d;
    d["count_u"]        = get_count_u();
    d["count_v"]        = get_count_v();
    d["degree_u"]       = get_degree_u();
    d["degree_v"]       = get_degree_v();
    d["wrap_u"]         = get_wrap_u();
    d["wrap_v"]         = get_wrap_v();
    d["control_points"] = get_control_points_array();
    d["weights"]        = get_weights_array();
    return d;
}

void NurbsPatch3DResource::apply_snapshot_state(const godot::Dictionary &p_state) {
    if (p_state.has("degree_u")) core_patch_.set_degree_u(static_cast<uint32_t>(int(p_state["degree_u"])));
    if (p_state.has("degree_v")) core_patch_.set_degree_v(static_cast<uint32_t>(int(p_state["degree_v"])));
    if (p_state.has("wrap_u"))   core_patch_.set_wrap_u(bool(p_state["wrap_u"]));
    if (p_state.has("wrap_v"))   core_patch_.set_wrap_v(bool(p_state["wrap_v"]));
    if (p_state.has("count_u"))  pending_count_u_ = int(p_state["count_u"]);
    if (p_state.has("count_v"))  pending_count_v_ = int(p_state["count_v"]);
    if (p_state.has("control_points")) pending_control_points_ = p_state["control_points"];
    if (p_state.has("weights"))        pending_weights_ = p_state["weights"];
    _apply_pending_grid();
}

void NurbsPatch3DResource::set_count_u(int p_count_u) {
    pending_count_u_ = std::clamp(p_count_u, 2, 64);
    if (pending_control_points_.size() == static_cast<int64_t>(pending_count_u_) * std::max(2, pending_count_v_)) {
        _apply_pending_grid();
    } else if (static_cast<int>(core_patch_.get_count_u()) != pending_count_u_) {
        core_patch_.setup_flat_grid(static_cast<real_t>(2.0), static_cast<real_t>(2.0),
                                    static_cast<uint32_t>(pending_count_u_),
                                    std::max<uint32_t>(2u, core_patch_.get_count_v()));
        emit_changed();
    }
}

void NurbsPatch3DResource::set_count_v(int p_count_v) {
    pending_count_v_ = std::clamp(p_count_v, 2, 64);
    if (pending_control_points_.size() == static_cast<int64_t>(std::max(2, pending_count_u_)) * pending_count_v_) {
        _apply_pending_grid();
    } else if (static_cast<int>(core_patch_.get_count_v()) != pending_count_v_) {
        core_patch_.setup_flat_grid(static_cast<real_t>(2.0), static_cast<real_t>(2.0),
                                    std::max<uint32_t>(2u, core_patch_.get_count_u()),
                                    static_cast<uint32_t>(pending_count_v_));
        emit_changed();
    }
}

void NurbsPatch3DResource::set_degree_u(int p_degree) {
    core_patch_.set_degree_u(static_cast<uint32_t>(std::clamp(p_degree, 1, 7)));
    emit_changed();
}

void NurbsPatch3DResource::set_degree_v(int p_degree) {
    core_patch_.set_degree_v(static_cast<uint32_t>(std::clamp(p_degree, 1, 7)));
    emit_changed();
}

void NurbsPatch3DResource::set_wrap_u(bool p_wrap) {
    core_patch_.set_wrap_u(p_wrap);
    emit_changed();
}

void NurbsPatch3DResource::set_wrap_v(bool p_wrap) {
    core_patch_.set_wrap_v(p_wrap);
    emit_changed();
}

void NurbsPatch3DResource::set_control_points_array(const godot::PackedVector3Array &p_points) {
    pending_control_points_ = p_points;
    _apply_pending_grid();
}

godot::PackedVector3Array NurbsPatch3DResource::get_control_points_array() const {
    const auto pts = core_patch_.get_control_points();
    godot::PackedVector3Array out;
    out.resize(static_cast<int64_t>(pts.size()));
    for (size_t i = 0; i < pts.size(); ++i) {
        out.set(static_cast<int64_t>(i), _to_godot(pts[i]));
    }
    return out;
}

void NurbsPatch3DResource::set_weights_array(const godot::PackedFloat32Array &p_weights) {
    pending_weights_ = p_weights;
    _apply_pending_grid();
}

godot::PackedFloat32Array NurbsPatch3DResource::get_weights_array() const {
    const auto wts = core_patch_.get_weights();
    godot::PackedFloat32Array out;
    out.resize(static_cast<int64_t>(wts.size()));
    for (size_t i = 0; i < wts.size(); ++i) {
        out.set(static_cast<int64_t>(i), static_cast<float>(wts[i]));
    }
    return out;
}

void NurbsPatch3DResource::set_control_point(int p_u, int p_v, const godot::Vector3 &p_pos, real_t p_weight) {
    if (p_u < 0 || p_v < 0) return;
    core_patch_.set_control_point(static_cast<uint32_t>(p_u), static_cast<uint32_t>(p_v), _to_glm(p_pos), p_weight);
    emit_changed();
}

godot::Vector3 NurbsPatch3DResource::get_control_point(int p_u, int p_v) const {
    if (p_u < 0 || p_v < 0) return godot::Vector3();
    return _to_godot(core_patch_.get_control_point(static_cast<uint32_t>(p_u), static_cast<uint32_t>(p_v)));
}

void NurbsPatch3DResource::set_control_weight(int p_u, int p_v, real_t p_weight) {
    if (p_u < 0 || p_v < 0) return;
    core_patch_.set_weight(static_cast<uint32_t>(p_u), static_cast<uint32_t>(p_v), p_weight);
    emit_changed();
}

real_t NurbsPatch3DResource::get_control_weight(int p_u, int p_v) const {
    if (p_u < 0 || p_v < 0) return static_cast<real_t>(1.0);
    return core_patch_.get_weight(static_cast<uint32_t>(p_u), static_cast<uint32_t>(p_v));
}

void NurbsPatch3DResource::commit_set_control_point(int p_u, int p_v, const godot::Vector3 &p_pos) {
    const godot::Vector3 old_pos = get_control_point(p_u, p_v);
    const real_t w = get_control_weight(p_u, p_v);
    if (undo_redo_.is_valid()) {
        undo_redo_->create_action("Move NURBS Control Point");
        undo_redo_->add_do_method(this, "set_control_point", {p_u, p_v, p_pos, w});
        undo_redo_->add_undo_method(this, "set_control_point", {p_u, p_v, old_pos, w});
        undo_redo_->commit_action();
    } else {
        set_control_point(p_u, p_v, p_pos, w);
    }
}

void NurbsPatch3DResource::commit_apply_snapshot(const godot::Dictionary &p_before, const godot::String &p_action_name) {
    const godot::Dictionary after = get_snapshot_state();
    if (undo_redo_.is_valid()) {
        undo_redo_->create_action(p_action_name);
        undo_redo_->add_do_method(this, "apply_snapshot_state", {after});
        undo_redo_->add_undo_method(this, "apply_snapshot_state", {p_before});
        undo_redo_->commit_action();
    } else {
        apply_snapshot_state(after);
    }
}

void NurbsPatch3DResource::set_loft_ribs(const godot::TypedArray<Spline3DResource> &p_ribs) {
    _disconnect_rib_signals();
    loft_ribs_ = p_ribs;
    _connect_rib_signals();
    if (loft_ribs_.size() >= 2) {
        rebuild_from_loft_ribs();
    }
}

void NurbsPatch3DResource::set_loft_samples_u(int p_samples) {
    loft_samples_u_ = std::clamp(p_samples, 2, 64);
    if (loft_ribs_.size() >= 2) {
        rebuild_from_loft_ribs();
    }
}

void NurbsPatch3DResource::set_loft_arc_length(bool p_arc_length) {
    loft_arc_length_ = p_arc_length;
    if (loft_ribs_.size() >= 2) {
        rebuild_from_loft_ribs();
    }
}

void NurbsPatch3DResource::rebuild_from_loft_ribs() {
    std::vector<core::Spline3D<real_t>> ribs;
    ribs.reserve(static_cast<size_t>(loft_ribs_.size()));
    for (int64_t i = 0; i < loft_ribs_.size(); ++i) {
        godot::Ref<Spline3DResource> rib = loft_ribs_[i];
        if (rib.is_valid() && rib->get_point_count() >= 2) {
            ribs.push_back(rib->get_core_spline());
        }
    }
    if (ribs.size() < 2u) {
        return;
    }

    core_patch_.loft_from_splines(
        ribs,
        static_cast<uint32_t>(loft_samples_u_),
        core_patch_.get_degree_u(),
        core_patch_.get_degree_v(),
        loft_arc_length_
    );
    pending_count_u_ = static_cast<int>(core_patch_.get_count_u());
    pending_count_v_ = static_cast<int>(core_patch_.get_count_v());
    emit_changed();
}

void NurbsPatch3DResource::setup_flat_grid(real_t p_width, real_t p_depth, int p_count_u, int p_count_v) {
    core_patch_.setup_flat_grid(p_width, p_depth, static_cast<uint32_t>(std::max(2, p_count_u)), static_cast<uint32_t>(std::max(2, p_count_v)));
    pending_count_u_ = static_cast<int>(core_patch_.get_count_u());
    pending_count_v_ = static_cast<int>(core_patch_.get_count_v());
    emit_changed();
}

void NurbsPatch3DResource::setup_cylinder_patch(real_t p_radius, real_t p_height, real_t p_arc_radians, int p_count_u, int p_count_v) {
    core_patch_.setup_cylinder_patch(p_radius, p_height, p_arc_radians, static_cast<uint32_t>(std::max(4, p_count_u)), static_cast<uint32_t>(std::max(2, p_count_v)));
    pending_count_u_ = static_cast<int>(core_patch_.get_count_u());
    pending_count_v_ = static_cast<int>(core_patch_.get_count_v());
    emit_changed();
}

void NurbsPatch3DResource::setup_dome_patch(real_t p_radius, real_t p_height, int p_count_u, int p_count_v) {
    core_patch_.setup_dome_patch(p_radius, p_height, static_cast<uint32_t>(std::max(3, p_count_u)), static_cast<uint32_t>(std::max(3, p_count_v)));
    pending_count_u_ = static_cast<int>(core_patch_.get_count_u());
    pending_count_v_ = static_cast<int>(core_patch_.get_count_v());
    emit_changed();
}

} // namespace ideam::godot_ext
