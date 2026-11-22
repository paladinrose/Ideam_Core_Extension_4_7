#include "surface_network_resource.h"

#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>

#include <algorithm>

namespace ideam::godot_ext {

namespace {

inline godot::Vector3 glm_to_godot(const glm::vec<3, real_t> &p_v) noexcept {
    return godot::Vector3(static_cast<float>(p_v.x), static_cast<float>(p_v.y), static_cast<float>(p_v.z));
}

inline godot::Transform3D frame_to_transform(const core::SurfaceFrame3DPOD<real_t> &p_frame) noexcept {
    godot::Basis basis;
    basis.set_column(0, glm_to_godot(p_frame.tangent_u));
    basis.set_column(1, glm_to_godot(p_frame.normal));
    basis.set_column(2, glm_to_godot(p_frame.tangent_v));
    return godot::Transform3D(basis, glm_to_godot(p_frame.position));
}

} // namespace

void SurfaceNetworkResource::_bind_methods() {
    using namespace godot;

    ADD_GROUP("Surface Network", "");

    ClassDB::bind_method(D_METHOD("set_network_name", "name"), &SurfaceNetworkResource::set_network_name);
    ClassDB::bind_method(D_METHOD("get_network_name"), &SurfaceNetworkResource::get_network_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "network_name"), "set_network_name", "get_network_name");

    ClassDB::bind_method(D_METHOD("set_patches", "patches"), &SurfaceNetworkResource::set_patches);
    ClassDB::bind_method(D_METHOD("get_patches"), &SurfaceNetworkResource::get_patches);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "patches", PROPERTY_HINT_ARRAY_TYPE, "SurfacePatchResource"), "set_patches", "get_patches");

    ClassDB::bind_method(D_METHOD("set_quad_bridges", "bridges"), &SurfaceNetworkResource::set_quad_bridges);
    ClassDB::bind_method(D_METHOD("get_quad_bridges"), &SurfaceNetworkResource::get_quad_bridges);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "quad_bridges"), "set_quad_bridges", "get_quad_bridges");

    ClassDB::bind_method(D_METHOD("set_pole_stitches", "poles"), &SurfaceNetworkResource::set_pole_stitches);
    ClassDB::bind_method(D_METHOD("get_pole_stitches"), &SurfaceNetworkResource::get_pole_stitches);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "pole_stitches"), "set_pole_stitches", "get_pole_stitches");

    ClassDB::bind_method(D_METHOD("set_weld_epsilon", "epsilon"), &SurfaceNetworkResource::set_weld_epsilon);
    ClassDB::bind_method(D_METHOD("get_weld_epsilon"), &SurfaceNetworkResource::get_weld_epsilon);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "weld_epsilon", PROPERTY_HINT_RANGE, "0.0,0.05,0.0001"), "set_weld_epsilon", "get_weld_epsilon");

    ClassDB::bind_method(D_METHOD("set_recompute_smooth_normals", "smooth"), &SurfaceNetworkResource::set_recompute_smooth_normals);
    ClassDB::bind_method(D_METHOD("get_recompute_smooth_normals"), &SurfaceNetworkResource::get_recompute_smooth_normals);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "recompute_smooth_normals"), "set_recompute_smooth_normals", "get_recompute_smooth_normals");

    ClassDB::bind_method(D_METHOD("set_uv_scale", "scale"), &SurfaceNetworkResource::set_uv_scale);
    ClassDB::bind_method(D_METHOD("get_uv_scale"), &SurfaceNetworkResource::get_uv_scale);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "uv_scale"), "set_uv_scale", "get_uv_scale");

    ClassDB::bind_method(D_METHOD("set_base_color", "color"), &SurfaceNetworkResource::set_base_color);
    ClassDB::bind_method(D_METHOD("get_base_color"), &SurfaceNetworkResource::get_base_color);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "base_color"), "set_base_color", "get_base_color");

    ADD_GROUP("Tools & Tutorials", "");

    ClassDB::bind_method(D_METHOD("set_help_topic_id", "topic_id"), &SurfaceNetworkResource::set_help_topic_id);
    ClassDB::bind_method(D_METHOD("get_help_topic_id"), &SurfaceNetworkResource::get_help_topic_id);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "help_topic_id"), "set_help_topic_id", "get_help_topic_id");

    ClassDB::bind_method(D_METHOD("set_semantic_tags", "tags"), &SurfaceNetworkResource::set_semantic_tags);
    ClassDB::bind_method(D_METHOD("get_semantic_tags"), &SurfaceNetworkResource::get_semantic_tags);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "semantic_tags"), "set_semantic_tags", "get_semantic_tags");

    ClassDB::bind_method(D_METHOD("set_undo_redo", "undo_redo"), &SurfaceNetworkResource::set_undo_redo);
    ClassDB::bind_method(D_METHOD("get_undo_redo"), &SurfaceNetworkResource::get_undo_redo);

    ClassDB::bind_method(D_METHOD("add_patch", "patch"), &SurfaceNetworkResource::add_patch);
    ClassDB::bind_method(D_METHOD("remove_patch", "index"), &SurfaceNetworkResource::remove_patch);
    ClassDB::bind_method(D_METHOD("get_patch_count"), &SurfaceNetworkResource::get_patch_count);
    ClassDB::bind_method(D_METHOD("get_patch", "index"), &SurfaceNetworkResource::get_patch);

    ClassDB::bind_method(
        D_METHOD("add_quad_bridge", "patch_a", "edge_a", "patch_b", "edge_b", "reverse_b", "bridge_rings", "tangent_strength"),
        &SurfaceNetworkResource::add_quad_bridge,
        DEFVAL(false),
        DEFVAL(2),
        DEFVAL(0.35)
    );
    ClassDB::bind_method(D_METHOD("add_pole_stitch", "edges", "center_normal_bias"), &SurfaceNetworkResource::add_pole_stitch, DEFVAL(0.0));

    ClassDB::bind_method(D_METHOD("sample_frame", "patch_id", "u", "v"), &SurfaceNetworkResource::sample_frame);
    ClassDB::bind_method(D_METHOD("sample_position", "patch_id", "u", "v"), &SurfaceNetworkResource::sample_position);
    ClassDB::bind_method(D_METHOD("sample_normal", "patch_id", "u", "v"), &SurfaceNetworkResource::sample_normal);
    ClassDB::bind_method(D_METHOD("build_array_mesh", "lod_level"), &SurfaceNetworkResource::build_array_mesh, DEFVAL(0));
    ClassDB::bind_method(D_METHOD("export_quad_mesh_data", "lod_level"), &SurfaceNetworkResource::export_quad_mesh_data, DEFVAL(0));
    ClassDB::bind_method(D_METHOD("get_manifold_stats", "lod_level"), &SurfaceNetworkResource::get_manifold_stats, DEFVAL(0));

    ClassDB::bind_method(D_METHOD("_on_patch_changed"), &SurfaceNetworkResource::_on_patch_changed);
}

SurfaceNetworkResource::~SurfaceNetworkResource() {
    _disconnect_patches();
}

void SurfaceNetworkResource::_connect_patches() {
    const godot::Callable cb(this, "_on_patch_changed");
    for (int64_t i = 0; i < patches_.size(); ++i) {
        godot::Ref<SurfacePatchResource> p = patches_[i];
        if (p.is_valid() && !p->is_connected("changed", cb)) {
            p->connect("changed", cb);
        }
    }
}

void SurfaceNetworkResource::_disconnect_patches() {
    const godot::Callable cb(this, "_on_patch_changed");
    for (int64_t i = 0; i < patches_.size(); ++i) {
        godot::Ref<SurfacePatchResource> p = patches_[i];
        if (p.is_valid() && p->is_connected("changed", cb)) {
            p->disconnect("changed", cb);
        }
    }
}

void SurfaceNetworkResource::_on_patch_changed() {
    core_dirty_ = true;
    emit_changed();
}

void SurfaceNetworkResource::_sync_core() const {
    if (!core_dirty_) return;
    core_dirty_ = false;

    core_network_.clear();
    for (int64_t i = 0; i < patches_.size(); ++i) {
        godot::Ref<SurfacePatchResource> p = patches_[i];
        if (!p.is_valid()) continue;

        const std::string name = p->get_patch_name().utf8().get_data();
        const uint32_t u_divs = static_cast<uint32_t>(std::max(2, p->get_base_u_divs()));
        const uint32_t v_divs = static_cast<uint32_t>(std::max(2, p->get_base_v_divs()));

        if (const auto *nurbs = godot::Object::cast_to<NurbsPatch3DResource>(p.ptr())) {
            core_network_.add_nurbs_patch(nurbs->get_core_patch(), name, u_divs, v_divs);
        } else if (const auto *coons = godot::Object::cast_to<CoonsPatch3DResource>(p.ptr())) {
            core_network_.add_coons_patch(coons->get_core_patch(), name, u_divs, v_divs);
        }
    }

    for (int64_t i = 0; i < quad_bridges_.size(); ++i) {
        const godot::Dictionary b = quad_bridges_[i];
        core::QuadBridgeSpec spec{};
        spec.patch_a          = static_cast<uint32_t>(int(b.get("patch_a", 0)));
        spec.edge_a           = static_cast<core::SurfaceBoundaryEdge>(std::clamp(int(b.get("edge_a", 3)), 0, 3));
        spec.patch_b          = static_cast<uint32_t>(int(b.get("patch_b", 0)));
        spec.edge_b           = static_cast<core::SurfaceBoundaryEdge>(std::clamp(int(b.get("edge_b", 2)), 0, 3));
        spec.reverse_b        = bool(b.get("reverse_b", false));
        spec.bridge_rings     = static_cast<uint32_t>(std::max(1, int(b.get("bridge_rings", 2))));
        spec.tangent_strength = static_cast<float>(double(b.get("tangent_strength", 0.35)));
        core_network_.add_quad_bridge(spec);
    }

    for (int64_t i = 0; i < pole_stitches_.size(); ++i) {
        const godot::Dictionary p = pole_stitches_[i];
        core::PoleStitchSpec spec{};
        spec.center_normal_bias = static_cast<float>(double(p.get("center_normal_bias", 0.0)));
        const godot::Array edges = p.get("edges", godot::Array());
        for (int64_t e = 0; e < edges.size(); ++e) {
            const godot::Dictionary ed = edges[e];
            core::PoleEdgeRef eref{};
            eref.patch_id = static_cast<uint32_t>(int(ed.get("patch_id", 0)));
            eref.edge     = static_cast<core::SurfaceBoundaryEdge>(std::clamp(int(ed.get("edge", 0)), 0, 3));
            eref.reverse  = bool(ed.get("reverse", false));
            spec.edges.push_back(eref);
        }
        if (spec.edges.size() >= 3u) {
            core_network_.add_pole_stitch(spec);
        }
    }
}

const core::SurfaceNetwork3D<real_t> &SurfaceNetworkResource::get_core_network() const {
    _sync_core();
    return core_network_;
}

void SurfaceNetworkResource::set_network_name(const godot::String &p_name) { network_name_ = p_name; emit_changed(); }

void SurfaceNetworkResource::set_patches(const godot::TypedArray<SurfacePatchResource> &p_patches) {
    _disconnect_patches();
    patches_ = p_patches;
    _connect_patches();
    core_dirty_ = true;
    emit_changed();
}

void SurfaceNetworkResource::set_quad_bridges(const godot::TypedArray<godot::Dictionary> &p_bridges) {
    quad_bridges_ = p_bridges;
    core_dirty_ = true;
    emit_changed();
}

void SurfaceNetworkResource::set_pole_stitches(const godot::TypedArray<godot::Dictionary> &p_poles) {
    pole_stitches_ = p_poles;
    core_dirty_ = true;
    emit_changed();
}

void SurfaceNetworkResource::set_weld_epsilon(real_t p_eps) {
    weld_epsilon_ = std::max(static_cast<real_t>(0.0), p_eps);
    emit_changed();
}

void SurfaceNetworkResource::set_recompute_smooth_normals(bool p_smooth) {
    recompute_smooth_normals_ = p_smooth;
    emit_changed();
}

void SurfaceNetworkResource::set_uv_scale(const godot::Vector2 &p_scale) { uv_scale_ = p_scale; emit_changed(); }
void SurfaceNetworkResource::set_base_color(const godot::Color &p_color) { base_color_ = p_color; emit_changed(); }

void SurfaceNetworkResource::set_undo_redo(const godot::Ref<IdeamUndoRedo> &p_ur) {
    undo_redo_ = p_ur;
    for (int64_t i = 0; i < patches_.size(); ++i) {
        godot::Ref<SurfacePatchResource> p = patches_[i];
        if (p.is_valid()) {
            p->set_undo_redo(p_ur);
        }
    }
}

godot::StringName SurfaceNetworkResource::get_help_topic_id() const {
    if (help_topic_id_ != godot::StringName()) return help_topic_id_;
    return godot::StringName("SurfaceNetworkResource");
}

void SurfaceNetworkResource::set_semantic_tags(const godot::PackedStringArray &p_tags) {
    semantic_tags_ = p_tags;
    emit_changed();
}

int SurfaceNetworkResource::add_patch(const godot::Ref<SurfacePatchResource> &p_patch) {
    if (!p_patch.is_valid()) return -1;
    if (undo_redo_.is_valid()) {
        p_patch->set_undo_redo(undo_redo_);
    }
    const int idx = static_cast<int>(patches_.size());
    patches_.append(p_patch);
    const godot::Callable cb(this, "_on_patch_changed");
    if (!p_patch->is_connected("changed", cb)) {
        p_patch->connect("changed", cb);
    }
    core_dirty_ = true;
    emit_changed();
    return idx;
}

void SurfaceNetworkResource::remove_patch(int p_index) {
    if (p_index < 0 || p_index >= patches_.size()) return;
    godot::Ref<SurfacePatchResource> p = patches_[p_index];
    if (p.is_valid()) {
        const godot::Callable cb(this, "_on_patch_changed");
        if (p->is_connected("changed", cb)) {
            p->disconnect("changed", cb);
        }
    }
    patches_.remove_at(p_index);
    core_dirty_ = true;
    emit_changed();
}

godot::Ref<SurfacePatchResource> SurfaceNetworkResource::get_patch(int p_index) const {
    if (p_index < 0 || p_index >= patches_.size()) return godot::Ref<SurfacePatchResource>();
    return patches_[p_index];
}

void SurfaceNetworkResource::add_quad_bridge(
    int p_patch_a,
    SurfacePatchBoundaryEdge p_edge_a,
    int p_patch_b,
    SurfacePatchBoundaryEdge p_edge_b,
    bool p_reverse_b,
    int p_bridge_rings,
    real_t p_tangent_strength
) {
    godot::Dictionary d;
    d["patch_a"]          = p_patch_a;
    d["edge_a"]           = static_cast<int>(p_edge_a);
    d["patch_b"]          = p_patch_b;
    d["edge_b"]           = static_cast<int>(p_edge_b);
    d["reverse_b"]        = p_reverse_b;
    d["bridge_rings"]     = std::max(1, p_bridge_rings);
    d["tangent_strength"] = static_cast<double>(p_tangent_strength);
    quad_bridges_.append(d);
    core_dirty_ = true;
    emit_changed();
}

void SurfaceNetworkResource::add_pole_stitch(const godot::TypedArray<godot::Dictionary> &p_edges, real_t p_center_normal_bias) {
    godot::Dictionary d;
    d["edges"]              = p_edges;
    d["center_normal_bias"] = static_cast<double>(p_center_normal_bias);
    pole_stitches_.append(d);
    core_dirty_ = true;
    emit_changed();
}

godot::Transform3D SurfaceNetworkResource::sample_frame(int p_patch_id, real_t p_u, real_t p_v) const {
    _sync_core();
    if (p_patch_id < 0) return godot::Transform3D();
    return frame_to_transform(core_network_.sample_frame(static_cast<uint32_t>(p_patch_id), p_u, p_v));
}

godot::Vector3 SurfaceNetworkResource::sample_position(int p_patch_id, real_t p_u, real_t p_v) const {
    _sync_core();
    if (p_patch_id < 0) return godot::Vector3();
    return glm_to_godot(core_network_.sample_frame(static_cast<uint32_t>(p_patch_id), p_u, p_v).position);
}

godot::Vector3 SurfaceNetworkResource::sample_normal(int p_patch_id, real_t p_u, real_t p_v) const {
    _sync_core();
    if (p_patch_id < 0) return godot::Vector3(0.0f, 1.0f, 0.0f);
    return glm_to_godot(core_network_.sample_frame(static_cast<uint32_t>(p_patch_id), p_u, p_v).normal);
}

godot::Ref<godot::ArrayMesh> SurfaceNetworkResource::build_array_mesh(int p_lod_level) const {
    _sync_core();
    const uint8_t lod = static_cast<uint8_t>(std::clamp(p_lod_level, 0, 3));
    const auto qmesh = core_network_.build_watertight_quad_mesh(lod, weld_epsilon_, recompute_smooth_normals_);

    godot::Ref<godot::ArrayMesh> mesh;
    mesh.instantiate();
    const size_t vcount = qmesh.positions.size();
    const std::vector<uint32_t> tris = qmesh.build_triangle_indices();
    if (vcount == 0u || tris.empty()) {
        return mesh;
    }

    godot::PackedVector3Array verts;
    godot::PackedVector3Array norms;
    godot::PackedFloat32Array tans;
    godot::PackedVector2Array uvs;
    godot::PackedColorArray cols;
    godot::PackedInt32Array indices;

    verts.resize(static_cast<int64_t>(vcount));
    norms.resize(static_cast<int64_t>(vcount));
    tans.resize(static_cast<int64_t>(vcount * 4u));
    uvs.resize(static_cast<int64_t>(vcount));
    cols.resize(static_cast<int64_t>(vcount));

    for (size_t i = 0; i < vcount; ++i) {
        verts.set(static_cast<int64_t>(i), glm_to_godot(qmesh.positions[i]));
        norms.set(static_cast<int64_t>(i), glm_to_godot(qmesh.normals[i]));
        tans.set(static_cast<int64_t>(i * 4u + 0u), static_cast<float>(qmesh.tangents[i].x));
        tans.set(static_cast<int64_t>(i * 4u + 1u), static_cast<float>(qmesh.tangents[i].y));
        tans.set(static_cast<int64_t>(i * 4u + 2u), static_cast<float>(qmesh.tangents[i].z));
        tans.set(static_cast<int64_t>(i * 4u + 3u), static_cast<float>(qmesh.tangents[i].w));
        uvs.set(static_cast<int64_t>(i), godot::Vector2(static_cast<float>(qmesh.uvs[i].x) * uv_scale_.x, static_cast<float>(qmesh.uvs[i].y) * uv_scale_.y));
        cols.set(static_cast<int64_t>(i), base_color_);
    }

    indices.resize(static_cast<int64_t>(tris.size()));
    for (size_t i = 0; i < tris.size(); ++i) {
        indices.set(static_cast<int64_t>(i), static_cast<int32_t>(tris[i]));
    }

    godot::Array arrays;
    arrays.resize(godot::Mesh::ARRAY_MAX);
    arrays[godot::Mesh::ARRAY_VERTEX]  = verts;
    arrays[godot::Mesh::ARRAY_NORMAL]  = norms;
    arrays[godot::Mesh::ARRAY_TANGENT] = tans;
    arrays[godot::Mesh::ARRAY_TEX_UV]  = uvs;
    arrays[godot::Mesh::ARRAY_COLOR]   = cols;
    arrays[godot::Mesh::ARRAY_INDEX]   = indices;

    mesh->add_surface_from_arrays(godot::Mesh::PRIMITIVE_TRIANGLES, arrays);
    return mesh;
}

godot::Dictionary SurfaceNetworkResource::export_quad_mesh_data(int p_lod_level) const {
    _sync_core();
    const uint8_t lod = static_cast<uint8_t>(std::clamp(p_lod_level, 0, 3));
    const auto qmesh = core_network_.build_watertight_quad_mesh(lod, weld_epsilon_, recompute_smooth_normals_);
    const auto stats = core::SurfaceNetwork3D<real_t>::compute_manifold_stats(qmesh);

    godot::Dictionary d;
    const size_t vcount = qmesh.positions.size();
    const size_t qcount = qmesh.quads.size();

    godot::PackedVector3Array verts;
    godot::PackedVector3Array norms;
    godot::PackedVector2Array uvs;
    godot::PackedInt32Array quad_indices;
    godot::PackedInt32Array patch_ids;
    godot::PackedInt32Array ring_indices;

    verts.resize(static_cast<int64_t>(vcount));
    norms.resize(static_cast<int64_t>(vcount));
    uvs.resize(static_cast<int64_t>(vcount));
    patch_ids.resize(static_cast<int64_t>(vcount));
    ring_indices.resize(static_cast<int64_t>(vcount));

    for (size_t i = 0; i < vcount; ++i) {
        verts.set(static_cast<int64_t>(i), glm_to_godot(qmesh.positions[i]));
        norms.set(static_cast<int64_t>(i), glm_to_godot(qmesh.normals[i]));
        uvs.set(static_cast<int64_t>(i), godot::Vector2(static_cast<float>(qmesh.uvs[i].x), static_cast<float>(qmesh.uvs[i].y)));
        patch_ids.set(static_cast<int64_t>(i), i < qmesh.vertex_patch_ids.size() ? static_cast<int32_t>(qmesh.vertex_patch_ids[i]) : 0);
        ring_indices.set(static_cast<int64_t>(i), i < qmesh.vertex_ring_indices.size() ? static_cast<int32_t>(qmesh.vertex_ring_indices[i]) : 0);
    }

    quad_indices.resize(static_cast<int64_t>(qcount * 4u));
    for (size_t q = 0; q < qcount; ++q) {
        quad_indices.set(static_cast<int64_t>(q * 4u + 0u), static_cast<int32_t>(qmesh.quads[q][0]));
        quad_indices.set(static_cast<int64_t>(q * 4u + 1u), static_cast<int32_t>(qmesh.quads[q][1]));
        quad_indices.set(static_cast<int64_t>(q * 4u + 2u), static_cast<int32_t>(qmesh.quads[q][2]));
        quad_indices.set(static_cast<int64_t>(q * 4u + 3u), static_cast<int32_t>(qmesh.quads[q][3]));
    }

    d["positions"]          = verts;
    d["normals"]            = norms;
    d["uvs"]                = uvs;
    d["quad_indices"]       = quad_indices;
    d["patch_ids"]          = patch_ids;
    d["ring_indices"]       = ring_indices;
    d["vertex_count"]       = static_cast<int64_t>(stats.vertex_count);
    d["quad_count"]         = static_cast<int64_t>(stats.quad_count);
    d["triangle_count"]     = static_cast<int64_t>(stats.triangle_count);
    d["is_manifold"]        = stats.is_manifold();
    d["is_watertight"]      = stats.is_watertight();
    return d;
}

godot::Dictionary SurfaceNetworkResource::get_manifold_stats(int p_lod_level) const {
    _sync_core();
    const uint8_t lod = static_cast<uint8_t>(std::clamp(p_lod_level, 0, 3));
    const auto qmesh = core_network_.build_watertight_quad_mesh(lod, weld_epsilon_, false);
    const auto stats = core::SurfaceNetwork3D<real_t>::compute_manifold_stats(qmesh);

    godot::Dictionary d;
    d["vertex_count"]       = static_cast<int64_t>(stats.vertex_count);
    d["quad_count"]         = static_cast<int64_t>(stats.quad_count);
    d["triangle_count"]     = static_cast<int64_t>(stats.triangle_count);
    d["boundary_edges"]     = static_cast<int64_t>(stats.boundary_edges);
    d["manifold_edges"]     = static_cast<int64_t>(stats.manifold_edges);
    d["non_manifold_edges"] = static_cast<int64_t>(stats.non_manifold_edges);
    d["is_manifold"]        = stats.is_manifold();
    d["is_watertight"]      = stats.is_watertight();
    return d;
}

} // namespace ideam::godot_ext
