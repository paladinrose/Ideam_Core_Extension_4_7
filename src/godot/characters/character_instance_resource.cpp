#include "character_instance_resource.h"

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
#include <godot_cpp/variant/transform3d.hpp>

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

void CharacterInstanceResource::_bind_methods() {
    using namespace godot;

    ADD_GROUP("Character Instance", "");

    ClassDB::bind_method(D_METHOD("set_body_plan", "body_plan"), &CharacterInstanceResource::set_body_plan);
    ClassDB::bind_method(D_METHOD("get_body_plan"), &CharacterInstanceResource::get_body_plan);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "body_plan", PROPERTY_HINT_RESOURCE_TYPE, "BodyPlanResource"),
        "set_body_plan",
        "get_body_plan"
    );

    ClassDB::bind_method(D_METHOD("set_character_name", "name"), &CharacterInstanceResource::set_character_name);
    ClassDB::bind_method(D_METHOD("get_character_name"), &CharacterInstanceResource::get_character_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "character_name"), "set_character_name", "get_character_name");

    ClassDB::bind_method(D_METHOD("set_parameter_values", "values"), &CharacterInstanceResource::set_parameter_values);
    ClassDB::bind_method(D_METHOD("get_parameter_values"), &CharacterInstanceResource::get_parameter_values);
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "parameter_values"), "set_parameter_values", "get_parameter_values");

    ClassDB::bind_method(D_METHOD("set_detail_deltas", "deltas"), &CharacterInstanceResource::set_detail_deltas);
    ClassDB::bind_method(D_METHOD("get_detail_deltas"), &CharacterInstanceResource::get_detail_deltas);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "detail_deltas"), "set_detail_deltas", "get_detail_deltas");

    ClassDB::bind_method(D_METHOD("set_base_color", "color"), &CharacterInstanceResource::set_base_color);
    ClassDB::bind_method(D_METHOD("get_base_color"), &CharacterInstanceResource::get_base_color);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "base_color"), "set_base_color", "get_base_color");

    ADD_GROUP("Tools & Tutorials", "");

    ClassDB::bind_method(D_METHOD("set_help_topic_id", "topic_id"), &CharacterInstanceResource::set_help_topic_id);
    ClassDB::bind_method(D_METHOD("get_help_topic_id"), &CharacterInstanceResource::get_help_topic_id);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "help_topic_id"), "set_help_topic_id", "get_help_topic_id");

    ClassDB::bind_method(D_METHOD("set_semantic_tags", "tags"), &CharacterInstanceResource::set_semantic_tags);
    ClassDB::bind_method(D_METHOD("get_semantic_tags"), &CharacterInstanceResource::get_semantic_tags);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "semantic_tags"), "set_semantic_tags", "get_semantic_tags");

    ClassDB::bind_method(D_METHOD("set_undo_redo", "undo_redo"), &CharacterInstanceResource::set_undo_redo);
    ClassDB::bind_method(D_METHOD("get_undo_redo"), &CharacterInstanceResource::get_undo_redo);

    ClassDB::bind_method(D_METHOD("set_parameter_value", "name", "value"), &CharacterInstanceResource::set_parameter_value);
    ClassDB::bind_method(D_METHOD("commit_set_parameter_value", "name", "value"), &CharacterInstanceResource::commit_set_parameter_value);
    ClassDB::bind_method(D_METHOD("get_parameter_value", "name"), &CharacterInstanceResource::get_parameter_value);

    ClassDB::bind_method(
        D_METHOD("add_detail_delta", "label", "scaffold_node_id", "u", "v", "local_delta_ntuv", "radius", "is_crease"),
        &CharacterInstanceResource::add_detail_delta,
        DEFVAL(0.05),
        DEFVAL(false)
    );
    ClassDB::bind_method(D_METHOD("clear_detail_deltas"), &CharacterInstanceResource::clear_detail_deltas);
    ClassDB::bind_method(D_METHOD("randomize_within_bounds", "seed"), &CharacterInstanceResource::randomize_within_bounds, DEFVAL(0));
    ClassDB::bind_method(D_METHOD("blend_with", "other", "alpha"), &CharacterInstanceResource::blend_with, DEFVAL(0.5));

    ClassDB::bind_method(D_METHOD("build_array_mesh", "lod_level"), &CharacterInstanceResource::build_array_mesh, DEFVAL(0));
    ClassDB::bind_method(D_METHOD("get_evaluated_bridge_regimes"), &CharacterInstanceResource::get_evaluated_bridge_regimes);
    ClassDB::bind_method(D_METHOD("build_skeleton_export_plan"), &CharacterInstanceResource::build_skeleton_export_plan);
    ClassDB::bind_method(D_METHOD("get_balance_report"), &CharacterInstanceResource::get_balance_report);

    ClassDB::bind_method(D_METHOD("_on_body_plan_changed"), &CharacterInstanceResource::_on_body_plan_changed);
}

CharacterInstanceResource::~CharacterInstanceResource() {
    if (body_plan_.is_valid()) {
        const godot::Callable cb(this, "_on_body_plan_changed");
        if (body_plan_->is_connected("changed", cb)) body_plan_->disconnect("changed", cb);
    }
}

void CharacterInstanceResource::_on_body_plan_changed() {
    emit_changed();
}

void CharacterInstanceResource::set_body_plan(const godot::Ref<BodyPlanResource> &p_plan) {
    const godot::Callable cb(this, "_on_body_plan_changed");
    if (body_plan_.is_valid() && body_plan_->is_connected("changed", cb)) {
        body_plan_->disconnect("changed", cb);
    }
    body_plan_ = p_plan;
    if (body_plan_.is_valid() && !body_plan_->is_connected("changed", cb)) {
        body_plan_->connect("changed", cb);
    }
    emit_changed();
}

void CharacterInstanceResource::set_character_name(const godot::String &p_name) {
    core_instance_.set_character_name(p_name.utf8().get_data());
    emit_changed();
}

godot::String CharacterInstanceResource::get_character_name() const {
    return godot::String::utf8(core_instance_.get_character_name().data(), static_cast<int64_t>(core_instance_.get_character_name().size()));
}

void CharacterInstanceResource::set_parameter_values(const godot::Dictionary &p_values) {
    const godot::Array keys = p_values.keys();
    for (int64_t i = 0; i < keys.size(); ++i) {
        const godot::String k = keys[i];
        core_instance_.set_parameter_value(k.utf8().get_data(), static_cast<float>(double(p_values[k])));
    }
    emit_changed();
}

godot::Dictionary CharacterInstanceResource::get_parameter_values() const {
    godot::Dictionary d;
    for (const auto &kv : core_instance_.parameter_values()) {
        d[godot::String::utf8(kv.first.c_str())] = static_cast<double>(kv.second);
    }
    return d;
}

void CharacterInstanceResource::set_detail_deltas(const godot::TypedArray<godot::Dictionary> &p_deltas) {
    core_instance_.clear_detail_deltas();
    for (int64_t i = 0; i < p_deltas.size(); ++i) {
        const godot::Dictionary d = p_deltas[i];
        core::UnconstrainedDetailDelta dd{};
        dd.label            = godot::String(d.get("label", "")).utf8().get_data();
        dd.scaffold_node_id = static_cast<uint32_t>(std::max(0, int(d.get("scaffold_node_id", 0))));
        dd.u                = static_cast<float>(double(d.get("u", 0.5)));
        dd.v                = static_cast<float>(double(d.get("v", 0.5)));
        const godot::Vector3 v3 = d.get("local_delta_ntuv", godot::Vector3());
        dd.local_delta_ntuv = glm::vec3(v3.x, v3.y, v3.z);
        dd.radius           = static_cast<float>(double(d.get("radius", 0.05)));
        dd.is_crease        = bool(d.get("is_crease", false));
        core_instance_.add_detail_delta(dd);
    }
    emit_changed();
}

godot::TypedArray<godot::Dictionary> CharacterInstanceResource::get_detail_deltas() const {
    godot::TypedArray<godot::Dictionary> out;
    for (const auto &dd : core_instance_.detail_deltas()) {
        godot::Dictionary d;
        d["id"]               = static_cast<int64_t>(dd.id);
        d["label"]            = godot::String::utf8(dd.label.c_str());
        d["scaffold_node_id"] = static_cast<int64_t>(dd.scaffold_node_id);
        d["u"]                = static_cast<double>(dd.u);
        d["v"]                = static_cast<double>(dd.v);
        d["local_delta_ntuv"] = glm3_to_gd(dd.local_delta_ntuv);
        d["radius"]           = static_cast<double>(dd.radius);
        d["is_crease"]        = dd.is_crease;
        out.append(d);
    }
    return out;
}

void CharacterInstanceResource::set_base_color(const godot::Color &p_color) { base_color_ = p_color; emit_changed(); }

godot::StringName CharacterInstanceResource::get_help_topic_id() const {
    if (help_topic_id_ != godot::StringName()) return help_topic_id_;
    return godot::StringName("CharacterInstanceResource");
}

void CharacterInstanceResource::set_semantic_tags(const godot::PackedStringArray &p_tags) {
    semantic_tags_ = p_tags;
    emit_changed();
}

void CharacterInstanceResource::set_parameter_value(const godot::String &p_name, real_t p_value) {
    core_instance_.set_parameter_value(p_name.utf8().get_data(), static_cast<float>(p_value));
    emit_changed();
}

void CharacterInstanceResource::commit_set_parameter_value(const godot::String &p_name, real_t p_value) {
    const real_t old_val = get_parameter_value(p_name);
    if (undo_redo_.is_valid()) {
        undo_redo_->create_action("Set Character Parameter: " + p_name);
        undo_redo_->add_do_method(this, "set_parameter_value", {p_name, p_value});
        undo_redo_->add_undo_method(this, "set_parameter_value", {p_name, old_val});
        undo_redo_->commit_action();
    } else {
        set_parameter_value(p_name, p_value);
    }
}

real_t CharacterInstanceResource::get_parameter_value(const godot::String &p_name) const {
    if (body_plan_.is_valid()) {
        return static_cast<real_t>(core_instance_.get_parameter_value(p_name.utf8().get_data(), body_plan_->get_core_plan()));
    }
    auto it = core_instance_.parameter_values().find(p_name.utf8().get_data());
    return (it != core_instance_.parameter_values().end()) ? static_cast<real_t>(it->second) : static_cast<real_t>(0.0);
}

int CharacterInstanceResource::add_detail_delta(
    const godot::String &p_label,
    int p_scaffold_node_id,
    real_t p_u,
    real_t p_v,
    const godot::Vector3 &p_local_delta_ntuv,
    real_t p_radius,
    bool p_is_crease
) {
    core::UnconstrainedDetailDelta dd{};
    dd.label            = p_label.utf8().get_data();
    dd.scaffold_node_id = static_cast<uint32_t>(std::max(0, p_scaffold_node_id));
    dd.u                = static_cast<float>(p_u);
    dd.v                = static_cast<float>(p_v);
    dd.local_delta_ntuv = glm::vec3(p_local_delta_ntuv.x, p_local_delta_ntuv.y, p_local_delta_ntuv.z);
    dd.radius           = static_cast<float>(p_radius);
    dd.is_crease        = p_is_crease;
    const uint32_t id = core_instance_.add_detail_delta(dd);
    emit_changed();
    return static_cast<int>(id);
}

void CharacterInstanceResource::clear_detail_deltas() {
    core_instance_.clear_detail_deltas();
    emit_changed();
}

void CharacterInstanceResource::randomize_within_bounds(int64_t p_seed) {
    if (!body_plan_.is_valid()) return;
    core_instance_.randomize_within_bounds(body_plan_->get_core_plan(), static_cast<uint64_t>(p_seed));
    emit_changed();
}

godot::Ref<CharacterInstanceResource> CharacterInstanceResource::blend_with(
    const godot::Ref<CharacterInstanceResource> &p_other,
    real_t p_alpha
) const {
    godot::Ref<CharacterInstanceResource> out;
    out.instantiate();
    out->set_body_plan(body_plan_);
    if (body_plan_.is_valid() && p_other.is_valid()) {
        out->core_instance_ = core_instance_.blend_with(
            p_other->core_instance_,
            body_plan_->get_core_plan(),
            static_cast<float>(p_alpha)
        );
    }
    return out;
}

godot::Ref<godot::ArrayMesh> CharacterInstanceResource::build_array_mesh(int p_lod_level) const {
    godot::Ref<godot::ArrayMesh> mesh;
    mesh.instantiate();
    if (!body_plan_.is_valid()) return mesh;

    const uint8_t lod = static_cast<uint8_t>(std::clamp(p_lod_level, 0, 3));
    const auto evaluated = core_instance_.evaluate_stack(body_plan_->get_core_plan(), lod);
    const auto &qmesh = evaluated.combined_quad_mesh;
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

godot::TypedArray<godot::Dictionary> CharacterInstanceResource::get_evaluated_bridge_regimes() const {
    godot::TypedArray<godot::Dictionary> out;
    if (!body_plan_.is_valid()) return out;
    const auto eval = core_instance_.evaluate_stack(body_plan_->get_core_plan(), 3u);
    for (const auto &bs : eval.bridge_states) {
        godot::Dictionary d;
        d["rule_id"]               = static_cast<int64_t>(bs.rule_id);
        d["name"]                  = godot::String::utf8(bs.name.c_str());
        d["evaluated_metric"]      = static_cast<double>(bs.evaluated_metric);
        d["active_regime"]         = static_cast<int64_t>(bs.active_regime);
        d["active_fold_depth"]     = static_cast<double>(bs.active_fold_depth);
        d["active_stretch_factor"] = static_cast<double>(bs.active_stretch_factor);
        out.append(d);
    }
    return out;
}

godot::TypedArray<godot::Dictionary> CharacterInstanceResource::build_skeleton_export_plan() const {
    godot::TypedArray<godot::Dictionary> out;
    if (!body_plan_.is_valid()) return out;
    const auto eval = core_instance_.evaluate_stack(body_plan_->get_core_plan(), 3u);
    for (const auto &b : eval.skeleton_plan) {
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
        out.append(d);
    }
    return out;
}

godot::Dictionary CharacterInstanceResource::get_balance_report() const {
    godot::Dictionary d;
    if (!body_plan_.is_valid()) return d;
    const auto eval = core_instance_.evaluate_stack(body_plan_->get_core_plan(), 3u);
    const auto &rep = eval.balance_report;
    d["center_of_gravity"]  = glm3_to_gd(rep.center_of_gravity);
    d["plumb_ground_point"] = glm3_to_gd(rep.plumb_ground_point);
    d["horizontal_offset"]  = static_cast<double>(rep.horizontal_offset);
    d["is_balanced"]        = rep.is_balanced;
    return d;
}

} // namespace ideam::godot_ext
