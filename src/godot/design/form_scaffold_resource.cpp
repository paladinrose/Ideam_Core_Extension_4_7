#include "form_scaffold_resource.h"

#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
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

inline godot::Vector3 glm3_to_gd(const glm::vec3 &v) noexcept {
    return godot::Vector3(v.x, v.y, v.z);
}

inline godot::Transform3D glm4_to_gd(const glm::mat4 &m) noexcept {
    godot::Basis b;
    b.set_column(0, godot::Vector3(m[0][0], m[0][1], m[0][2]));
    b.set_column(1, godot::Vector3(m[1][0], m[1][1], m[1][2]));
    b.set_column(2, godot::Vector3(m[2][0], m[2][1], m[2][2]));
    return godot::Transform3D(b, godot::Vector3(m[3][0], m[3][1], m[3][2]));
}

} // namespace

void FormScaffoldResource::_bind_methods() {
    using namespace godot;

    BIND_ENUM_CONSTANT(SCAFFOLD_ARCHETYPE_BIPED_NORMAL);
    BIND_ENUM_CONSTANT(SCAFFOLD_ARCHETYPE_BIPED_IDEALISTIC);
    BIND_ENUM_CONSTANT(SCAFFOLD_ARCHETYPE_BIPED_HEROIC);
    BIND_ENUM_CONSTANT(SCAFFOLD_ARCHETYPE_BIPED_CHILD);
    BIND_ENUM_CONSTANT(SCAFFOLD_ARCHETYPE_BIPED_STYLIZED);
    BIND_ENUM_CONSTANT(SCAFFOLD_ARCHETYPE_QUADRUPED);
    BIND_ENUM_CONSTANT(SCAFFOLD_ARCHETYPE_AVIAN);
    BIND_ENUM_CONSTANT(SCAFFOLD_ARCHETYPE_SERPENTINE);

    ADD_GROUP("Loomis Form Scaffold", "");

    ClassDB::bind_method(D_METHOD("set_scaffold_name", "name"), &FormScaffoldResource::set_scaffold_name);
    ClassDB::bind_method(D_METHOD("get_scaffold_name"), &FormScaffoldResource::get_scaffold_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "scaffold_name"), "set_scaffold_name", "get_scaffold_name");

    ClassDB::bind_method(D_METHOD("set_archetype", "archetype"), &FormScaffoldResource::set_archetype);
    ClassDB::bind_method(D_METHOD("get_archetype"), &FormScaffoldResource::get_archetype);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "archetype", PROPERTY_HINT_ENUM,
                     "Biped Normal (7.5H):0,Biped Idealistic (8H):1,Biped Heroic (8.5H):2,Biped Child (5H):3,Biped Stylized (4.5H):4,Quadruped:5,Avian:6,Serpentine:7"),
        "set_archetype",
        "get_archetype"
    );

    ClassDB::bind_method(D_METHOD("set_ruler_graph", "ruler_graph"), &FormScaffoldResource::set_ruler_graph);
    ClassDB::bind_method(D_METHOD("get_ruler_graph"), &FormScaffoldResource::get_ruler_graph);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "ruler_graph", PROPERTY_HINT_RESOURCE_TYPE, "ProportionRulerGraphResource"),
        "set_ruler_graph",
        "get_ruler_graph"
    );

    ClassDB::bind_method(D_METHOD("set_line_of_action", "spline"), &FormScaffoldResource::set_line_of_action);
    ClassDB::bind_method(D_METHOD("get_line_of_action"), &FormScaffoldResource::get_line_of_action);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "line_of_action", PROPERTY_HINT_RESOURCE_TYPE, "Spline3DResource"),
        "set_line_of_action",
        "get_line_of_action"
    );

    ClassDB::bind_method(D_METHOD("set_contrapposto_amount", "amount"), &FormScaffoldResource::set_contrapposto_amount);
    ClassDB::bind_method(D_METHOD("get_contrapposto_amount"), &FormScaffoldResource::get_contrapposto_amount);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "contrapposto_amount", PROPERTY_HINT_RANGE, "-1.0,1.0,0.02"), "set_contrapposto_amount", "get_contrapposto_amount");

    ClassDB::bind_method(D_METHOD("set_total_mass_kg", "mass"), &FormScaffoldResource::set_total_mass_kg);
    ClassDB::bind_method(D_METHOD("get_total_mass_kg"), &FormScaffoldResource::get_total_mass_kg);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "total_mass_kg", PROPERTY_HINT_RANGE, "0.5,5000.0,0.5"), "set_total_mass_kg", "get_total_mass_kg");

    ClassDB::bind_method(D_METHOD("set_scaffold_color", "color"), &FormScaffoldResource::set_scaffold_color);
    ClassDB::bind_method(D_METHOD("get_scaffold_color"), &FormScaffoldResource::get_scaffold_color);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "scaffold_color"), "set_scaffold_color", "get_scaffold_color");

    ADD_GROUP("Tools & Tutorials", "");

    ClassDB::bind_method(D_METHOD("set_help_topic_id", "topic_id"), &FormScaffoldResource::set_help_topic_id);
    ClassDB::bind_method(D_METHOD("get_help_topic_id"), &FormScaffoldResource::get_help_topic_id);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "help_topic_id"), "set_help_topic_id", "get_help_topic_id");

    ClassDB::bind_method(D_METHOD("set_semantic_tags", "tags"), &FormScaffoldResource::set_semantic_tags);
    ClassDB::bind_method(D_METHOD("get_semantic_tags"), &FormScaffoldResource::get_semantic_tags);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "semantic_tags"), "set_semantic_tags", "get_semantic_tags");

    ClassDB::bind_method(D_METHOD("set_undo_redo", "undo_redo"), &FormScaffoldResource::set_undo_redo);
    ClassDB::bind_method(D_METHOD("get_undo_redo"), &FormScaffoldResource::get_undo_redo);

    ClassDB::bind_method(D_METHOD("get_node_count"), &FormScaffoldResource::get_node_count);
    ClassDB::bind_method(D_METHOD("get_node_local_offset", "node_id"), &FormScaffoldResource::get_node_local_offset);
    ClassDB::bind_method(D_METHOD("set_node_local_offset", "node_id", "offset"), &FormScaffoldResource::set_node_local_offset);
    ClassDB::bind_method(D_METHOD("commit_set_node_local_offset", "node_id", "offset"), &FormScaffoldResource::commit_set_node_local_offset);
    ClassDB::bind_method(D_METHOD("get_node_world_position", "node_id"), &FormScaffoldResource::get_node_world_position);
    ClassDB::bind_method(D_METHOD("get_node_name", "node_id"), &FormScaffoldResource::get_node_name);

    ClassDB::bind_method(D_METHOD("sample_scaffold_frame", "node_id", "u", "v"), &FormScaffoldResource::sample_scaffold_frame);
    ClassDB::bind_method(D_METHOD("get_balance_report"), &FormScaffoldResource::get_balance_report);
    ClassDB::bind_method(D_METHOD("build_preview_array_mesh", "subdivs"), &FormScaffoldResource::build_preview_array_mesh, DEFVAL(10));
    ClassDB::bind_method(D_METHOD("build_skeleton_export_plan"), &FormScaffoldResource::build_skeleton_export_plan);

    ClassDB::bind_method(D_METHOD("setup_loomis_biped", "preset", "head_height_m"), &FormScaffoldResource::setup_loomis_biped, DEFVAL(LOOMIS_BIPED_NORMAL_7_5H), DEFVAL(0.23));
    ClassDB::bind_method(D_METHOD("setup_loomis_quadruped", "shoulder_height_m"), &FormScaffoldResource::setup_loomis_quadruped, DEFVAL(0.80));
    ClassDB::bind_method(D_METHOD("setup_loomis_avian", "keel_length_m"), &FormScaffoldResource::setup_loomis_avian, DEFVAL(0.22));
    ClassDB::bind_method(D_METHOD("setup_loomis_serpentine", "skull_length_m", "segments"), &FormScaffoldResource::setup_loomis_serpentine, DEFVAL(0.14), DEFVAL(10));

    ClassDB::bind_method(D_METHOD("_on_dependency_changed"), &FormScaffoldResource::_on_dependency_changed);
}

FormScaffoldResource::FormScaffoldResource() {
    setup_loomis_biped(LOOMIS_BIPED_NORMAL_7_5H, 0.23f);
}

FormScaffoldResource::~FormScaffoldResource() {
    const godot::Callable cb(this, "_on_dependency_changed");
    if (ruler_graph_.is_valid() && ruler_graph_->is_connected("changed", cb)) ruler_graph_->disconnect("changed", cb);
    if (line_of_action_.is_valid() && line_of_action_->is_connected("changed", cb)) line_of_action_->disconnect("changed", cb);
}

void FormScaffoldResource::_on_dependency_changed() {
    _sync_from_dependencies();
    emit_changed();
}

void FormScaffoldResource::_sync_from_dependencies() {
    if (ruler_graph_.is_valid()) {
        core_scaffold_.set_ruler_graph(ruler_graph_->get_core_graph());
    }
    if (line_of_action_.is_valid() && line_of_action_->get_point_count() >= 2) {
        core::Spline3D<float> fspline;
        const auto &src = line_of_action_->get_core_spline();
        for (const auto &pt : src.get_points()) {
            fspline.add_point(glm::vec3(static_cast<float>(pt.point.x), static_cast<float>(pt.point.y), static_cast<float>(pt.point.z)), core::CurveType::CATMULL_ROM);
        }
        core_scaffold_.set_line_of_action(fspline, true);
    }
    core_scaffold_.evaluate();
}

void FormScaffoldResource::set_scaffold_name(const godot::String &p_name) {
    core_scaffold_.set_scaffold_name(p_name.utf8().get_data());
    emit_changed();
}

godot::String FormScaffoldResource::get_scaffold_name() const {
    return godot::String::utf8(core_scaffold_.get_scaffold_name().data(), static_cast<int64_t>(core_scaffold_.get_scaffold_name().size()));
}

void FormScaffoldResource::set_archetype(FormScaffoldArchetype p_arch) {
    archetype_ = p_arch;
    const real_t base_unit = ruler_graph_.is_valid() ? ruler_graph_->get_base_unit_meters() : static_cast<real_t>(0.23);
    switch (p_arch) {
        case SCAFFOLD_ARCHETYPE_BIPED_IDEALISTIC: setup_loomis_biped(LOOMIS_BIPED_IDEALISTIC_8H, base_unit); break;
        case SCAFFOLD_ARCHETYPE_BIPED_HEROIC:     setup_loomis_biped(LOOMIS_BIPED_HEROIC_8_5H, base_unit); break;
        case SCAFFOLD_ARCHETYPE_BIPED_CHILD:      setup_loomis_biped(LOOMIS_BIPED_CHILD_5H, base_unit); break;
        case SCAFFOLD_ARCHETYPE_BIPED_STYLIZED:   setup_loomis_biped(LOOMIS_BIPED_STYLIZED_4_5H, base_unit); break;
        case SCAFFOLD_ARCHETYPE_QUADRUPED:        setup_loomis_quadruped(0.80f); break;
        case SCAFFOLD_ARCHETYPE_AVIAN:            setup_loomis_avian(0.22f); break;
        case SCAFFOLD_ARCHETYPE_SERPENTINE:       setup_loomis_serpentine(0.14f, 10); break;
        case SCAFFOLD_ARCHETYPE_BIPED_NORMAL:
        default:                                  setup_loomis_biped(LOOMIS_BIPED_NORMAL_7_5H, base_unit); break;
    }
}

void FormScaffoldResource::set_ruler_graph(const godot::Ref<ProportionRulerGraphResource> &p_ruler) {
    const godot::Callable cb(this, "_on_dependency_changed");
    if (ruler_graph_.is_valid() && ruler_graph_->is_connected("changed", cb)) ruler_graph_->disconnect("changed", cb);
    ruler_graph_ = p_ruler;
    if (ruler_graph_.is_valid() && !ruler_graph_->is_connected("changed", cb)) ruler_graph_->connect("changed", cb);
    _sync_from_dependencies();
    emit_changed();
}

void FormScaffoldResource::set_line_of_action(const godot::Ref<Spline3DResource> &p_spline) {
    const godot::Callable cb(this, "_on_dependency_changed");
    if (line_of_action_.is_valid() && line_of_action_->is_connected("changed", cb)) line_of_action_->disconnect("changed", cb);
    line_of_action_ = p_spline;
    if (line_of_action_.is_valid() && !line_of_action_->is_connected("changed", cb)) line_of_action_->connect("changed", cb);
    _sync_from_dependencies();
    emit_changed();
}

void FormScaffoldResource::set_contrapposto_amount(real_t p_amount) {
    core_scaffold_.set_contrapposto_amount(static_cast<float>(p_amount));
    core_scaffold_.evaluate();
    emit_changed();
}

void FormScaffoldResource::set_total_mass_kg(real_t p_mass) {
    core_scaffold_.set_total_mass_kg(static_cast<float>(p_mass));
    emit_changed();
}

void FormScaffoldResource::set_scaffold_color(const godot::Color &p_color) {
    scaffold_color_ = p_color;
    emit_changed();
}

void FormScaffoldResource::set_undo_redo(const godot::Ref<IdeamUndoRedo> &p_ur) {
    undo_redo_ = p_ur;
    if (ruler_graph_.is_valid()) ruler_graph_->set_undo_redo(p_ur);
}

godot::StringName FormScaffoldResource::get_help_topic_id() const {
    if (help_topic_id_ != godot::StringName()) return help_topic_id_;
    return godot::StringName("FormScaffoldResource");
}

void FormScaffoldResource::set_semantic_tags(const godot::PackedStringArray &p_tags) {
    semantic_tags_ = p_tags;
    emit_changed();
}

godot::Vector3 FormScaffoldResource::get_node_local_offset(int p_node_id) const {
    if (p_node_id < 0) return godot::Vector3();
    if (const auto *n = core_scaffold_.get_node(static_cast<uint32_t>(p_node_id))) {
        return glm3_to_gd(n->local_offset);
    }
    return godot::Vector3();
}

void FormScaffoldResource::set_node_local_offset(int p_node_id, const godot::Vector3 &p_offset) {
    if (p_node_id < 0) return;
    if (auto *n = core_scaffold_.get_node_mut(static_cast<uint32_t>(p_node_id))) {
        n->local_offset = glm::vec3(p_offset.x, p_offset.y, p_offset.z);
        core_scaffold_.evaluate();
        emit_changed();
    }
}

void FormScaffoldResource::commit_set_node_local_offset(int p_node_id, const godot::Vector3 &p_offset) {
    const godot::Vector3 old_off = get_node_local_offset(p_node_id);
    if (undo_redo_.is_valid()) {
        undo_redo_->create_action("Move Scaffold Node");
        undo_redo_->add_do_method(this, "set_node_local_offset", p_node_id, p_offset);
        undo_redo_->add_undo_method(this, "set_node_local_offset", p_node_id, old_off);
        undo_redo_->commit_action();
    } else {
        set_node_local_offset(p_node_id, p_offset);
    }
}

godot::Vector3 FormScaffoldResource::get_node_world_position(int p_node_id) const {
    if (p_node_id < 0) return godot::Vector3();
    if (const auto *n = core_scaffold_.get_node(static_cast<uint32_t>(p_node_id))) {
        return glm3_to_gd(glm::vec3(n->world_transform[3]));
    }
    return godot::Vector3();
}

godot::String FormScaffoldResource::get_node_name(int p_node_id) const {
    if (p_node_id < 0) return "";
    if (const auto *n = core_scaffold_.get_node(static_cast<uint32_t>(p_node_id))) {
        return godot::String::utf8(n->name.c_str());
    }
    return "";
}

godot::Transform3D FormScaffoldResource::sample_scaffold_frame(int p_node_id, real_t p_u, real_t p_v) const {
    if (p_node_id < 0) return godot::Transform3D();
    const auto f = core_scaffold_.sample_scaffold_frame(static_cast<uint32_t>(p_node_id), static_cast<float>(p_u), static_cast<float>(p_v));
    godot::Basis b;
    b.set_column(0, glm3_to_gd(f.tangent_u));
    b.set_column(1, glm3_to_gd(f.normal));
    b.set_column(2, glm3_to_gd(f.tangent_v));
    return godot::Transform3D(b, glm3_to_gd(f.position));
}

godot::Dictionary FormScaffoldResource::get_balance_report() const {
    const auto rep = core_scaffold_.compute_balance_report();
    godot::Dictionary d;
    d["center_of_gravity"]  = glm3_to_gd(rep.center_of_gravity);
    d["plumb_ground_point"] = glm3_to_gd(rep.plumb_ground_point);
    d["support_center"]     = glm3_to_gd(rep.support_center);
    d["support_radius"]     = static_cast<double>(rep.support_radius);
    d["horizontal_offset"]  = static_cast<double>(rep.horizontal_offset);
    d["is_balanced"]        = rep.is_balanced;
    return d;
}

godot::Ref<godot::ArrayMesh> FormScaffoldResource::build_preview_array_mesh(int p_subdivs) const {
    const auto qmesh = core_scaffold_.tessellate_scaffold_mesh(static_cast<uint32_t>(std::clamp(p_subdivs, 4, 24)));
    godot::Ref<godot::ArrayMesh> mesh;
    mesh.instantiate();
    const size_t vcount = qmesh.positions.size();
    const std::vector<uint32_t> tris = qmesh.build_triangle_indices();
    if (vcount == 0u || tris.empty()) return mesh;

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
        verts.set(static_cast<int64_t>(i), glm3_to_gd(qmesh.positions[i]));
        norms.set(static_cast<int64_t>(i), glm3_to_gd(qmesh.normals[i]));
        tans.set(static_cast<int64_t>(i * 4u + 0u), qmesh.tangents[i].x);
        tans.set(static_cast<int64_t>(i * 4u + 1u), qmesh.tangents[i].y);
        tans.set(static_cast<int64_t>(i * 4u + 2u), qmesh.tangents[i].z);
        tans.set(static_cast<int64_t>(i * 4u + 3u), qmesh.tangents[i].w);
        uvs.set(static_cast<int64_t>(i), godot::Vector2(qmesh.uvs[i].x, qmesh.uvs[i].y));
        cols.set(static_cast<int64_t>(i), scaffold_color_);
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

godot::TypedArray<godot::Dictionary> FormScaffoldResource::build_skeleton_export_plan() const {
    const auto plan = core_scaffold_.build_skeleton_export_plan();
    godot::TypedArray<godot::Dictionary> out;
    for (const auto &b : plan) {
        godot::Dictionary d;
        d["node_id"]               = static_cast<int64_t>(b.node_id);
        d["parent_bone_index"]     = static_cast<int64_t>(b.parent_bone_index);
        d["bone_name"]             = godot::String::utf8(b.bone_name.c_str());
        d["rest_local_transform"]  = glm4_to_gd(b.rest_local_transform);
        d["rest_world_transform"]  = glm4_to_gd(b.rest_world_transform);
        d["collider_half_extents"] = glm3_to_gd(b.collider_half_extents);
        d["mass_kg"]               = static_cast<double>(b.mass_kg);
        d["physics_binding_mode"]  = static_cast<int64_t>(b.physics_binding_mode);
        d["joint_limit_lower_deg"] = static_cast<double>(b.joint_limit_lower_deg);
        d["joint_limit_upper_deg"] = static_cast<double>(b.joint_limit_upper_deg);
        d["joint_axis_index"]      = static_cast<int64_t>(b.joint_axis_index);
        out.append(d);
    }
    return out;
}

void FormScaffoldResource::setup_loomis_biped(LoomisBipedCanonPreset p_preset, real_t p_head_height_m) {
    core_scaffold_ = core::FormScaffoldCore::create_loomis_biped_scaffold(
        static_cast<core::LoomisBipedCanon>(p_preset),
        static_cast<float>(p_head_height_m)
    );
    if (!ruler_graph_.is_valid()) ruler_graph_.instantiate();
    ruler_graph_->set_core_graph(core_scaffold_.get_ruler_graph());
    emit_changed();
}

void FormScaffoldResource::setup_loomis_quadruped(real_t p_shoulder_height_m) {
    core_scaffold_ = core::FormScaffoldCore::create_loomis_quadruped_scaffold(static_cast<float>(p_shoulder_height_m));
    if (!ruler_graph_.is_valid()) ruler_graph_.instantiate();
    ruler_graph_->set_core_graph(core_scaffold_.get_ruler_graph());
    emit_changed();
}

void FormScaffoldResource::setup_loomis_avian(real_t p_keel_length_m) {
    core_scaffold_ = core::FormScaffoldCore::create_loomis_avian_scaffold(static_cast<float>(p_keel_length_m));
    if (!ruler_graph_.is_valid()) ruler_graph_.instantiate();
    ruler_graph_->set_core_graph(core_scaffold_.get_ruler_graph());
    emit_changed();
}

void FormScaffoldResource::setup_loomis_serpentine(real_t p_skull_length_m, int p_segments) {
    core_scaffold_ = core::FormScaffoldCore::create_loomis_serpentine_scaffold(
        static_cast<float>(p_skull_length_m),
        static_cast<uint32_t>(std::clamp(p_segments, 4, 24))
    );
    if (!ruler_graph_.is_valid()) ruler_graph_.instantiate();
    ruler_graph_->set_core_graph(core_scaffold_.get_ruler_graph());
    emit_changed();
}

} // namespace ideam::godot_ext
