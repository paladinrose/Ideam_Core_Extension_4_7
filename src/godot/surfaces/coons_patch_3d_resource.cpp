#include "coons_patch_3d_resource.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

#include <algorithm>

namespace ideam::godot_ext {

void CoonsPatch3DResource::_bind_methods() {
    using namespace godot;

    BIND_ENUM_CONSTANT(COONS_BLEND_LINEAR);
    BIND_ENUM_CONSTANT(COONS_BLEND_CUBIC_HERMITE);

    ADD_GROUP("Boundary Curves", "");

    ClassDB::bind_method(D_METHOD("set_north_v0", "spline"), &CoonsPatch3DResource::set_north_v0);
    ClassDB::bind_method(D_METHOD("get_north_v0"), &CoonsPatch3DResource::get_north_v0);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "north_v0", PROPERTY_HINT_RESOURCE_TYPE, "Spline3DResource"), "set_north_v0", "get_north_v0");

    ClassDB::bind_method(D_METHOD("set_south_v1", "spline"), &CoonsPatch3DResource::set_south_v1);
    ClassDB::bind_method(D_METHOD("get_south_v1"), &CoonsPatch3DResource::get_south_v1);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "south_v1", PROPERTY_HINT_RESOURCE_TYPE, "Spline3DResource"), "set_south_v1", "get_south_v1");

    ClassDB::bind_method(D_METHOD("set_west_u0", "spline"), &CoonsPatch3DResource::set_west_u0);
    ClassDB::bind_method(D_METHOD("get_west_u0"), &CoonsPatch3DResource::get_west_u0);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "west_u0", PROPERTY_HINT_RESOURCE_TYPE, "Spline3DResource"), "set_west_u0", "get_west_u0");

    ClassDB::bind_method(D_METHOD("set_east_u1", "spline"), &CoonsPatch3DResource::set_east_u1);
    ClassDB::bind_method(D_METHOD("get_east_u1"), &CoonsPatch3DResource::get_east_u1);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "east_u1", PROPERTY_HINT_RESOURCE_TYPE, "Spline3DResource"), "set_east_u1", "get_east_u1");

    ClassDB::bind_method(D_METHOD("set_reverse_north", "reverse"), &CoonsPatch3DResource::set_reverse_north);
    ClassDB::bind_method(D_METHOD("get_reverse_north"), &CoonsPatch3DResource::get_reverse_north);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "reverse_north"), "set_reverse_north", "get_reverse_north");

    ClassDB::bind_method(D_METHOD("set_reverse_south", "reverse"), &CoonsPatch3DResource::set_reverse_south);
    ClassDB::bind_method(D_METHOD("get_reverse_south"), &CoonsPatch3DResource::get_reverse_south);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "reverse_south"), "set_reverse_south", "get_reverse_south");

    ClassDB::bind_method(D_METHOD("set_reverse_west", "reverse"), &CoonsPatch3DResource::set_reverse_west);
    ClassDB::bind_method(D_METHOD("get_reverse_west"), &CoonsPatch3DResource::get_reverse_west);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "reverse_west"), "set_reverse_west", "get_reverse_west");

    ClassDB::bind_method(D_METHOD("set_reverse_east", "reverse"), &CoonsPatch3DResource::set_reverse_east);
    ClassDB::bind_method(D_METHOD("get_reverse_east"), &CoonsPatch3DResource::get_reverse_east);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "reverse_east"), "set_reverse_east", "get_reverse_east");

    ADD_GROUP("Gordon Interior Guides", "");

    ClassDB::bind_method(D_METHOD("set_interior_u_guides", "guides"), &CoonsPatch3DResource::set_interior_u_guides);
    ClassDB::bind_method(D_METHOD("get_interior_u_guides"), &CoonsPatch3DResource::get_interior_u_guides);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "interior_u_guides", PROPERTY_HINT_ARRAY_TYPE, "Spline3DResource"), "set_interior_u_guides", "get_interior_u_guides");

    ClassDB::bind_method(D_METHOD("set_interior_u_params", "params"), &CoonsPatch3DResource::set_interior_u_params);
    ClassDB::bind_method(D_METHOD("get_interior_u_params"), &CoonsPatch3DResource::get_interior_u_params);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "interior_u_params"), "set_interior_u_params", "get_interior_u_params");

    ClassDB::bind_method(D_METHOD("set_interior_v_guides", "guides"), &CoonsPatch3DResource::set_interior_v_guides);
    ClassDB::bind_method(D_METHOD("get_interior_v_guides"), &CoonsPatch3DResource::get_interior_v_guides);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "interior_v_guides", PROPERTY_HINT_ARRAY_TYPE, "Spline3DResource"), "set_interior_v_guides", "get_interior_v_guides");

    ClassDB::bind_method(D_METHOD("set_interior_v_params", "params"), &CoonsPatch3DResource::set_interior_v_params);
    ClassDB::bind_method(D_METHOD("get_interior_v_params"), &CoonsPatch3DResource::get_interior_v_params);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "interior_v_params"), "set_interior_v_params", "get_interior_v_params");

    ADD_GROUP("Interpolation", "");

    ClassDB::bind_method(D_METHOD("set_blend_mode", "mode"), &CoonsPatch3DResource::set_blend_mode);
    ClassDB::bind_method(D_METHOD("get_blend_mode"), &CoonsPatch3DResource::get_blend_mode);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "blend_mode", PROPERTY_HINT_ENUM, "Linear:0,Cubic Hermite:1"), "set_blend_mode", "get_blend_mode");

    ClassDB::bind_method(D_METHOD("set_use_arc_length", "arc_length"), &CoonsPatch3DResource::set_use_arc_length);
    ClassDB::bind_method(D_METHOD("get_use_arc_length"), &CoonsPatch3DResource::get_use_arc_length);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_arc_length"), "set_use_arc_length", "get_use_arc_length");

    ClassDB::bind_method(
        D_METHOD("setup_default_pillow_patch", "width", "depth", "arch_height"),
        &CoonsPatch3DResource::setup_default_pillow_patch,
        DEFVAL(2.0),
        DEFVAL(2.0),
        DEFVAL(0.45)
    );
    ClassDB::bind_method(D_METHOD("_on_spline_changed"), &CoonsPatch3DResource::_on_spline_changed);
}

CoonsPatch3DResource::CoonsPatch3DResource() {
    setup_default_pillow_patch();
}

CoonsPatch3DResource::~CoonsPatch3DResource() {
    _disconnect_spline(north_v0_);
    _disconnect_spline(south_v1_);
    _disconnect_spline(west_u0_);
    _disconnect_spline(east_u1_);
    for (int64_t i = 0; i < interior_u_guides_.size(); ++i) _disconnect_spline(interior_u_guides_[i]);
    for (int64_t i = 0; i < interior_v_guides_.size(); ++i) _disconnect_spline(interior_v_guides_[i]);
}

void CoonsPatch3DResource::_connect_spline(const godot::Ref<Spline3DResource> &p_spline) {
    if (!p_spline.is_valid()) return;
    const godot::Callable cb(this, "_on_spline_changed");
    if (!p_spline->is_connected("changed", cb)) {
        p_spline->connect("changed", cb);
    }
}

void CoonsPatch3DResource::_disconnect_spline(const godot::Ref<Spline3DResource> &p_spline) {
    if (!p_spline.is_valid()) return;
    const godot::Callable cb(this, "_on_spline_changed");
    if (p_spline->is_connected("changed", cb)) {
        p_spline->disconnect("changed", cb);
    }
}

void CoonsPatch3DResource::_on_spline_changed() {
    core_dirty_ = true;
    emit_changed();
}

void CoonsPatch3DResource::_sync_core() const {
    if (!core_dirty_) return;
    core_dirty_ = false;

    core_patch_.set_blend_mode(blend_mode_ == COONS_BLEND_LINEAR ? core::CoonsBlendMode::LINEAR : core::CoonsBlendMode::CUBIC_HERMITE);
    core_patch_.set_use_arc_length(use_arc_length_);

    const core::Spline3D<real_t> empty_spline;
    core_patch_.set_boundary_curves(
        north_v0_.is_valid() ? north_v0_->get_core_spline() : empty_spline,
        south_v1_.is_valid() ? south_v1_->get_core_spline() : empty_spline,
        west_u0_.is_valid()  ? west_u0_->get_core_spline()  : empty_spline,
        east_u1_.is_valid()  ? east_u1_->get_core_spline()  : empty_spline,
        reverse_north_,
        reverse_south_,
        reverse_west_,
        reverse_east_
    );

    core_patch_.clear_interior_guides();
    const int64_t nu = interior_u_guides_.size();
    for (int64_t i = 0; i < nu; ++i) {
        godot::Ref<Spline3DResource> g = interior_u_guides_[i];
        if (!g.is_valid() || g->get_point_count() < 2) continue;
        const real_t param = (i < interior_u_params_.size())
            ? static_cast<real_t>(interior_u_params_[i])
            : static_cast<real_t>(i + 1) / static_cast<real_t>(nu + 1);
        core_patch_.add_interior_u_guide(param, g->get_core_spline(), false);
    }

    const int64_t nv = interior_v_guides_.size();
    for (int64_t i = 0; i < nv; ++i) {
        godot::Ref<Spline3DResource> g = interior_v_guides_[i];
        if (!g.is_valid() || g->get_point_count() < 2) continue;
        const real_t param = (i < interior_v_params_.size())
            ? static_cast<real_t>(interior_v_params_[i])
            : static_cast<real_t>(i + 1) / static_cast<real_t>(nv + 1);
        core_patch_.add_interior_v_guide(param, g->get_core_spline(), false);
    }
}

const CoonsPatch3DResource::CorePatch &CoonsPatch3DResource::get_core_patch() const {
    _sync_core();
    return core_patch_;
}

bool CoonsPatch3DResource::is_patch_valid() const {
    _sync_core();
    return core_patch_.is_valid();
}

core::SurfaceFrame3DPOD<real_t> CoonsPatch3DResource::sample_core_frame(real_t p_u, real_t p_v, uint32_t p_patch_id) const {
    _sync_core();
    return core_patch_.sample_frame(p_u, p_v, p_patch_id);
}

core::SurfaceQuadMesh3D<real_t> CoonsPatch3DResource::tessellate_core_quad_mesh(int p_lod_level, uint32_t p_patch_id) const {
    _sync_core();
    const uint8_t lod = static_cast<uint8_t>(std::clamp(p_lod_level, 0, 3));
    return core_patch_.tessellate_quad_mesh(
        static_cast<uint32_t>(base_u_divs_),
        static_cast<uint32_t>(base_v_divs_),
        lod,
        p_patch_id
    );
}

void CoonsPatch3DResource::set_north_v0(const godot::Ref<Spline3DResource> &p_spline) {
    _disconnect_spline(north_v0_);
    north_v0_ = p_spline;
    _connect_spline(north_v0_);
    core_dirty_ = true;
    emit_changed();
}

void CoonsPatch3DResource::set_south_v1(const godot::Ref<Spline3DResource> &p_spline) {
    _disconnect_spline(south_v1_);
    south_v1_ = p_spline;
    _connect_spline(south_v1_);
    core_dirty_ = true;
    emit_changed();
}

void CoonsPatch3DResource::set_west_u0(const godot::Ref<Spline3DResource> &p_spline) {
    _disconnect_spline(west_u0_);
    west_u0_ = p_spline;
    _connect_spline(west_u0_);
    core_dirty_ = true;
    emit_changed();
}

void CoonsPatch3DResource::set_east_u1(const godot::Ref<Spline3DResource> &p_spline) {
    _disconnect_spline(east_u1_);
    east_u1_ = p_spline;
    _connect_spline(east_u1_);
    core_dirty_ = true;
    emit_changed();
}

void CoonsPatch3DResource::set_reverse_north(bool p_rev) { reverse_north_ = p_rev; core_dirty_ = true; emit_changed(); }
void CoonsPatch3DResource::set_reverse_south(bool p_rev) { reverse_south_ = p_rev; core_dirty_ = true; emit_changed(); }
void CoonsPatch3DResource::set_reverse_west(bool p_rev)  { reverse_west_  = p_rev; core_dirty_ = true; emit_changed(); }
void CoonsPatch3DResource::set_reverse_east(bool p_rev)  { reverse_east_  = p_rev; core_dirty_ = true; emit_changed(); }

void CoonsPatch3DResource::set_interior_u_guides(const godot::TypedArray<Spline3DResource> &p_guides) {
    for (int64_t i = 0; i < interior_u_guides_.size(); ++i) _disconnect_spline(interior_u_guides_[i]);
    interior_u_guides_ = p_guides;
    for (int64_t i = 0; i < interior_u_guides_.size(); ++i) _connect_spline(interior_u_guides_[i]);
    core_dirty_ = true;
    emit_changed();
}

void CoonsPatch3DResource::set_interior_u_params(const godot::PackedFloat32Array &p_params) {
    interior_u_params_ = p_params;
    core_dirty_ = true;
    emit_changed();
}

void CoonsPatch3DResource::set_interior_v_guides(const godot::TypedArray<Spline3DResource> &p_guides) {
    for (int64_t i = 0; i < interior_v_guides_.size(); ++i) _disconnect_spline(interior_v_guides_[i]);
    interior_v_guides_ = p_guides;
    for (int64_t i = 0; i < interior_v_guides_.size(); ++i) _connect_spline(interior_v_guides_[i]);
    core_dirty_ = true;
    emit_changed();
}

void CoonsPatch3DResource::set_interior_v_params(const godot::PackedFloat32Array &p_params) {
    interior_v_params_ = p_params;
    core_dirty_ = true;
    emit_changed();
}

void CoonsPatch3DResource::set_blend_mode(CoonsPatchBlendMode p_mode) {
    blend_mode_ = p_mode;
    core_dirty_ = true;
    emit_changed();
}

void CoonsPatch3DResource::set_use_arc_length(bool p_arc_length) {
    use_arc_length_ = p_arc_length;
    core_dirty_ = true;
    emit_changed();
}

void CoonsPatch3DResource::setup_default_pillow_patch(real_t p_width, real_t p_depth, real_t p_arch_height) {
    const float hw = static_cast<float>(p_width) * 0.5f;
    const float hd = static_cast<float>(p_depth) * 0.5f;
    const float h  = static_cast<float>(p_arch_height);

    auto make_arch_res = [](const godot::Vector3 &p0, const godot::Vector3 &p1, const godot::Vector3 &mid_off) {
        godot::Ref<Spline3DResource> s;
        s.instantiate();
        s->set_default_curve_type(CURVE_CATMULL_ROM);
        s->add_point(p0, CURVE_CATMULL_ROM);
        s->add_point((p0 + p1) * 0.5f + mid_off, CURVE_CATMULL_ROM);
        s->add_point(p1, CURVE_CATMULL_ROM);
        return s;
    };

    const godot::Vector3 c00(-hw, 0.0f, -hd);
    const godot::Vector3 c10( hw, 0.0f, -hd);
    const godot::Vector3 c01(-hw, 0.0f,  hd);
    const godot::Vector3 c11( hw, 0.0f,  hd);
    const godot::Vector3 lift(0.0f, h, 0.0f);

    set_north_v0(make_arch_res(c00, c10, lift * 0.5f));
    set_south_v1(make_arch_res(c01, c11, lift * 0.5f));
    set_west_u0(make_arch_res(c00, c01, lift * 0.5f));
    set_east_u1(make_arch_res(c10, c11, lift * 0.5f));

    reverse_north_ = false;
    reverse_south_ = false;
    reverse_west_  = false;
    reverse_east_  = false;

    godot::TypedArray<Spline3DResource> u_guides;
    u_guides.append(make_arch_res((c00 + c01) * 0.5f + lift * 0.5f, (c10 + c11) * 0.5f + lift * 0.5f, lift * 0.5f));
    set_interior_u_guides(u_guides);

    godot::PackedFloat32Array u_params;
    u_params.append(0.5f);
    set_interior_u_params(u_params);

    set_interior_v_guides(godot::TypedArray<Spline3DResource>());
    set_interior_v_params(godot::PackedFloat32Array());
}

} // namespace ideam::godot_ext
