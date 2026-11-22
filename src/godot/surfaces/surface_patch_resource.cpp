#include "surface_patch_resource.h"

#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>

#include <algorithm>

namespace ideam::godot_ext {

void SurfacePatchResource::_bind_methods() {
    using namespace godot;

    BIND_ENUM_CONSTANT(SURFACE_EDGE_NORTH_V0);
    BIND_ENUM_CONSTANT(SURFACE_EDGE_SOUTH_V1);
    BIND_ENUM_CONSTANT(SURFACE_EDGE_WEST_U0);
    BIND_ENUM_CONSTANT(SURFACE_EDGE_EAST_U1);

    ADD_GROUP("Surface Patch", "");

    ClassDB::bind_method(D_METHOD("set_patch_name", "name"), &SurfacePatchResource::set_patch_name);
    ClassDB::bind_method(D_METHOD("get_patch_name"), &SurfacePatchResource::get_patch_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "patch_name"), "set_patch_name", "get_patch_name");

    ClassDB::bind_method(D_METHOD("set_base_u_divs", "divs"), &SurfacePatchResource::set_base_u_divs);
    ClassDB::bind_method(D_METHOD("get_base_u_divs"), &SurfacePatchResource::get_base_u_divs);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "base_u_divs", PROPERTY_HINT_RANGE, "2,128,2"), "set_base_u_divs", "get_base_u_divs");

    ClassDB::bind_method(D_METHOD("set_base_v_divs", "divs"), &SurfacePatchResource::set_base_v_divs);
    ClassDB::bind_method(D_METHOD("get_base_v_divs"), &SurfacePatchResource::get_base_v_divs);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "base_v_divs", PROPERTY_HINT_RANGE, "2,128,2"), "set_base_v_divs", "get_base_v_divs");

    ClassDB::bind_method(D_METHOD("set_uv_scale", "scale"), &SurfacePatchResource::set_uv_scale);
    ClassDB::bind_method(D_METHOD("get_uv_scale"), &SurfacePatchResource::get_uv_scale);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "uv_scale"), "set_uv_scale", "get_uv_scale");

    ClassDB::bind_method(D_METHOD("set_base_color", "color"), &SurfacePatchResource::set_base_color);
    ClassDB::bind_method(D_METHOD("get_base_color"), &SurfacePatchResource::get_base_color);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "base_color"), "set_base_color", "get_base_color");

    ADD_GROUP("Tools & Tutorials", "");

    ClassDB::bind_method(D_METHOD("set_help_topic_id", "topic_id"), &SurfacePatchResource::set_help_topic_id);
    ClassDB::bind_method(D_METHOD("get_help_topic_id"), &SurfacePatchResource::get_help_topic_id);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "help_topic_id"), "set_help_topic_id", "get_help_topic_id");

    ClassDB::bind_method(D_METHOD("set_semantic_tags", "tags"), &SurfacePatchResource::set_semantic_tags);
    ClassDB::bind_method(D_METHOD("get_semantic_tags"), &SurfacePatchResource::get_semantic_tags);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "semantic_tags"), "set_semantic_tags", "get_semantic_tags");

    ClassDB::bind_method(D_METHOD("set_undo_redo", "undo_redo"), &SurfacePatchResource::set_undo_redo);
    ClassDB::bind_method(D_METHOD("get_undo_redo"), &SurfacePatchResource::get_undo_redo);

    ClassDB::bind_method(D_METHOD("is_patch_valid"), &SurfacePatchResource::is_patch_valid);
    ClassDB::bind_method(D_METHOD("get_patch_type_label"), &SurfacePatchResource::get_patch_type_label);
    ClassDB::bind_method(D_METHOD("sample_position", "u", "v"), &SurfacePatchResource::sample_position);
    ClassDB::bind_method(D_METHOD("sample_normal", "u", "v"), &SurfacePatchResource::sample_normal);
    ClassDB::bind_method(D_METHOD("sample_frame", "u", "v"), &SurfacePatchResource::sample_frame);
    ClassDB::bind_method(
        D_METHOD("offset_from_frame", "u", "v", "d_normal", "d_tangent_u", "d_tangent_v"),
        &SurfacePatchResource::offset_from_frame,
        DEFVAL(0.0),
        DEFVAL(0.0)
    );
    ClassDB::bind_method(D_METHOD("build_array_mesh", "lod_level"), &SurfacePatchResource::build_array_mesh, DEFVAL(0));
    ClassDB::bind_method(D_METHOD("export_quad_mesh_data", "lod_level"), &SurfacePatchResource::export_quad_mesh_data, DEFVAL(0));
    ClassDB::bind_method(D_METHOD("get_resolved_lod_divs", "lod_level"), &SurfacePatchResource::get_resolved_lod_divs, DEFVAL(0));
    ClassDB::bind_method(D_METHOD("get_snapshot_state"), &SurfacePatchResource::get_snapshot_state);
    ClassDB::bind_method(D_METHOD("apply_snapshot_state", "state"), &SurfacePatchResource::apply_snapshot_state);
}

godot::Transform3D SurfacePatchResource::_frame_to_transform(const core::SurfaceFrame3DPOD<real_t> &p_frame) noexcept {
    const godot::Vector3 tx = _to_godot(p_frame.tangent_u);
    const godot::Vector3 ty = _to_godot(p_frame.normal);
    const godot::Vector3 tz = _to_godot(p_frame.tangent_v);
    godot::Basis basis;
    basis.set_column(0, tx);
    basis.set_column(1, ty);
    basis.set_column(2, tz);
    return godot::Transform3D(basis, _to_godot(p_frame.position));
}

godot::Ref<godot::ArrayMesh> SurfacePatchResource::_quad_mesh_to_array_mesh(
    const core::SurfaceQuadMesh3D<real_t> &p_mesh,
    const godot::Vector2 &p_uv_scale,
    const godot::Color &p_color
) {
    godot::Ref<godot::ArrayMesh> mesh;
    mesh.instantiate();

    const size_t vcount = p_mesh.positions.size();
    const std::vector<uint32_t> tri_indices = p_mesh.build_triangle_indices();
    if (vcount == 0u || tri_indices.empty()) {
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
        verts.set(static_cast<int64_t>(i), _to_godot(p_mesh.positions[i]));
        norms.set(static_cast<int64_t>(i), _to_godot(p_mesh.normals[i]));
        tans.set(static_cast<int64_t>(i * 4u + 0u), static_cast<float>(p_mesh.tangents[i].x));
        tans.set(static_cast<int64_t>(i * 4u + 1u), static_cast<float>(p_mesh.tangents[i].y));
        tans.set(static_cast<int64_t>(i * 4u + 2u), static_cast<float>(p_mesh.tangents[i].z));
        tans.set(static_cast<int64_t>(i * 4u + 3u), static_cast<float>(p_mesh.tangents[i].w));
        uvs.set(
            static_cast<int64_t>(i),
            godot::Vector2(
                static_cast<float>(p_mesh.uvs[i].x) * p_uv_scale.x,
                static_cast<float>(p_mesh.uvs[i].y) * p_uv_scale.y
            )
        );
        cols.set(static_cast<int64_t>(i), p_color);
    }

    indices.resize(static_cast<int64_t>(tri_indices.size()));
    for (size_t i = 0; i < tri_indices.size(); ++i) {
        indices.set(static_cast<int64_t>(i), static_cast<int32_t>(tri_indices[i]));
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

godot::Dictionary SurfacePatchResource::_quad_mesh_to_dictionary(
    const core::SurfaceQuadMesh3D<real_t> &p_mesh
) {
    godot::Dictionary d;
    const size_t vcount = p_mesh.positions.size();
    const size_t qcount = p_mesh.quads.size();

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
        verts.set(static_cast<int64_t>(i), _to_godot(p_mesh.positions[i]));
        norms.set(static_cast<int64_t>(i), _to_godot(p_mesh.normals[i]));
        uvs.set(static_cast<int64_t>(i), godot::Vector2(static_cast<float>(p_mesh.uvs[i].x), static_cast<float>(p_mesh.uvs[i].y)));
        patch_ids.set(static_cast<int64_t>(i), i < p_mesh.vertex_patch_ids.size() ? static_cast<int32_t>(p_mesh.vertex_patch_ids[i]) : 0);
        ring_indices.set(static_cast<int64_t>(i), i < p_mesh.vertex_ring_indices.size() ? static_cast<int32_t>(p_mesh.vertex_ring_indices[i]) : 0);
    }

    quad_indices.resize(static_cast<int64_t>(qcount * 4u));
    for (size_t q = 0; q < qcount; ++q) {
        quad_indices.set(static_cast<int64_t>(q * 4u + 0u), static_cast<int32_t>(p_mesh.quads[q][0]));
        quad_indices.set(static_cast<int64_t>(q * 4u + 1u), static_cast<int32_t>(p_mesh.quads[q][1]));
        quad_indices.set(static_cast<int64_t>(q * 4u + 2u), static_cast<int32_t>(p_mesh.quads[q][2]));
        quad_indices.set(static_cast<int64_t>(q * 4u + 3u), static_cast<int32_t>(p_mesh.quads[q][3]));
    }

    d["positions"]    = verts;
    d["normals"]      = norms;
    d["uvs"]          = uvs;
    d["quad_indices"] = quad_indices;
    d["patch_ids"]    = patch_ids;
    d["ring_indices"] = ring_indices;
    d["vertex_count"] = static_cast<int64_t>(vcount);
    d["quad_count"]   = static_cast<int64_t>(qcount);
    return d;
}

void SurfacePatchResource::set_patch_name(const godot::String &p_name) {
    patch_name_ = p_name;
    emit_changed();
}

void SurfacePatchResource::set_base_u_divs(int p_divs) {
    base_u_divs_ = std::clamp(p_divs, 2, 128);
    emit_changed();
}

void SurfacePatchResource::set_base_v_divs(int p_divs) {
    base_v_divs_ = std::clamp(p_divs, 2, 128);
    emit_changed();
}

void SurfacePatchResource::set_uv_scale(const godot::Vector2 &p_scale) {
    uv_scale_ = p_scale;
    emit_changed();
}

void SurfacePatchResource::set_base_color(const godot::Color &p_color) {
    base_color_ = p_color;
    emit_changed();
}

godot::StringName SurfacePatchResource::get_help_topic_id() const {
    if (help_topic_id_ != godot::StringName()) {
        return help_topic_id_;
    }
    return godot::StringName(get_class());
}

void SurfacePatchResource::set_semantic_tags(const godot::PackedStringArray &p_tags) {
    semantic_tags_ = p_tags;
    emit_changed();
}

godot::Vector3 SurfacePatchResource::sample_position(real_t p_u, real_t p_v) const {
    return _to_godot(sample_core_frame(p_u, p_v, 0u).position);
}

godot::Vector3 SurfacePatchResource::sample_normal(real_t p_u, real_t p_v) const {
    return _to_godot(sample_core_frame(p_u, p_v, 0u).normal);
}

godot::Transform3D SurfacePatchResource::sample_frame(real_t p_u, real_t p_v) const {
    return _frame_to_transform(sample_core_frame(p_u, p_v, 0u));
}

godot::Vector3 SurfacePatchResource::offset_from_frame(
    real_t p_u,
    real_t p_v,
    real_t p_d_normal,
    real_t p_d_tangent_u,
    real_t p_d_tangent_v
) const {
    const auto f = sample_core_frame(p_u, p_v, 0u);
    return _to_godot(f.offset_point(p_d_normal, p_d_tangent_u, p_d_tangent_v));
}

godot::Ref<godot::ArrayMesh> SurfacePatchResource::build_array_mesh(int p_lod_level) const {
    const auto quad_mesh = tessellate_core_quad_mesh(p_lod_level, 0u);
    return _quad_mesh_to_array_mesh(quad_mesh, uv_scale_, base_color_);
}

godot::Dictionary SurfacePatchResource::export_quad_mesh_data(int p_lod_level) const {
    const auto quad_mesh = tessellate_core_quad_mesh(p_lod_level, 0u);
    return _quad_mesh_to_dictionary(quad_mesh);
}

godot::Vector2i SurfacePatchResource::get_resolved_lod_divs(int p_lod_level) const {
    const uint8_t lod = static_cast<uint8_t>(std::clamp(p_lod_level, 0, 3));
    return godot::Vector2i(
        static_cast<int32_t>(core::resolve_surface_lod_divs(static_cast<uint32_t>(base_u_divs_), lod)),
        static_cast<int32_t>(core::resolve_surface_lod_divs(static_cast<uint32_t>(base_v_divs_), lod))
    );
}

} // namespace ideam::godot_ext
