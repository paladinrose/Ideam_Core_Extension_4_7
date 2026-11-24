#include "loomis_character_3d_resource.h"

#include "ideam_characters_plugin.h"

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
#include <span>

namespace ideam::godot_ext {

namespace {

inline godot::Vector3 glm3_to_gd(const glm::vec3 &v) noexcept {
    return godot::Vector3(v.x, v.y, v.z);
}

inline glm::vec3 gd3_to_glm(const godot::Vector3 &v) noexcept {
    return glm::vec3(static_cast<float>(v.x), static_cast<float>(v.y), static_cast<float>(v.z));
}

inline godot::Transform3D glm4_to_gd(const glm::mat4 &m) noexcept {
    godot::Basis b;
    b.set_column(0, godot::Vector3(m[0][0], m[0][1], m[0][2]));
    b.set_column(1, godot::Vector3(m[1][0], m[1][1], m[1][2]));
    b.set_column(2, godot::Vector3(m[2][0], m[2][1], m[2][2]));
    return godot::Transform3D(b, godot::Vector3(m[3][0], m[3][1], m[3][2]));
}

inline const char *morph_channel_label(int p_channel) noexcept {
    switch (p_channel) {
        case core::MORPH_STATURE_HEIGHT:   return "Stature Height";
        case core::MORPH_TORSO_BULK:       return "Torso Bulk";
        case core::MORPH_HEAD_SCALE:       return "Head Scale";
        case core::MORPH_UPPER_LIMB_REACH: return "Upper Limb Reach";
        case core::MORPH_LOWER_LIMB_STRIDE:return "Lower Limb Stride";
        case core::MORPH_DIGIT_CURL_0:     return "Digit 0 (Thumb) Curl";
        case core::MORPH_DIGIT_CURL_1:     return "Digit 1 (Index) Curl";
        case core::MORPH_DIGIT_CURL_2:     return "Digit 2 (Middle) Curl";
        case core::MORPH_DIGIT_CURL_3:     return "Digit 3 (Ring) Curl";
        case core::MORPH_DIGIT_CURL_4:     return "Digit 4 (Pinky) Curl";
        case core::MORPH_TAIL_LENGTH:      return "Tail Length";
        case core::MORPH_WING_SPAN:        return "Wing Span";
        default:                           return "Custom Morph";
    }
}

} // namespace

void LoomisCharacter3DResource::_bind_methods() {
    using namespace godot;

    ADD_GROUP("Loomis Character 3D", "");

    ClassDB::bind_method(D_METHOD("set_body_plan", "body_plan"), &LoomisCharacter3DResource::set_body_plan);
    ClassDB::bind_method(D_METHOD("get_body_plan"), &LoomisCharacter3DResource::get_body_plan);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "body_plan", PROPERTY_HINT_RESOURCE_TYPE, "BodyPlanResource"),
        "set_body_plan",
        "get_body_plan"
    );

    ClassDB::bind_method(D_METHOD("set_character_name", "name"), &LoomisCharacter3DResource::set_character_name);
    ClassDB::bind_method(D_METHOD("get_character_name"), &LoomisCharacter3DResource::get_character_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "character_name"), "set_character_name", "get_character_name");

    ClassDB::bind_method(D_METHOD("set_parameter_values", "values"), &LoomisCharacter3DResource::set_parameter_values);
    ClassDB::bind_method(D_METHOD("get_parameter_values"), &LoomisCharacter3DResource::get_parameter_values);
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "parameter_values"), "set_parameter_values", "get_parameter_values");

    ClassDB::bind_method(D_METHOD("set_details_enabled", "enabled"), &LoomisCharacter3DResource::set_details_enabled);
    ClassDB::bind_method(D_METHOD("get_details_enabled"), &LoomisCharacter3DResource::get_details_enabled);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "details_enabled"), "set_details_enabled", "get_details_enabled");

    ClassDB::bind_method(D_METHOD("set_detail_deltas", "deltas"), &LoomisCharacter3DResource::set_detail_deltas);
    ClassDB::bind_method(D_METHOD("get_detail_deltas"), &LoomisCharacter3DResource::get_detail_deltas);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "detail_deltas"), "set_detail_deltas", "get_detail_deltas");

    ClassDB::bind_method(D_METHOD("set_base_color", "color"), &LoomisCharacter3DResource::set_base_color);
    ClassDB::bind_method(D_METHOD("get_base_color"), &LoomisCharacter3DResource::get_base_color);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "base_color"), "set_base_color", "get_base_color");

    ADD_GROUP("Parametric Morphs & DOD", "");

    ClassDB::bind_method(D_METHOD("set_body_archetype", "archetype"), &LoomisCharacter3DResource::set_body_archetype);
    ClassDB::bind_method(D_METHOD("get_body_archetype"), &LoomisCharacter3DResource::get_body_archetype);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "body_archetype", PROPERTY_HINT_ENUM, "Biped:0,Quadruped:1,Avian:2,FlyingEye:3,Serpentine:4,Custom:5"),
        "set_body_archetype",
        "get_body_archetype"
    );

    ClassDB::bind_method(D_METHOD("set_subdivision_lod_level", "lod"), &LoomisCharacter3DResource::set_subdivision_lod_level);
    ClassDB::bind_method(D_METHOD("get_subdivision_lod_level"), &LoomisCharacter3DResource::get_subdivision_lod_level);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "subdivision_lod_level", PROPERTY_HINT_RANGE, "0,3,1"),
        "set_subdivision_lod_level",
        "get_subdivision_lod_level"
    );

    ClassDB::bind_method(D_METHOD("set_active_partition_mask", "mask"), &LoomisCharacter3DResource::set_active_partition_mask);
    ClassDB::bind_method(D_METHOD("get_active_partition_mask"), &LoomisCharacter3DResource::get_active_partition_mask);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "active_partition_mask"), "set_active_partition_mask", "get_active_partition_mask");

    ClassDB::bind_method(D_METHOD("set_morph_weights", "weights"), &LoomisCharacter3DResource::set_morph_weights);
    ClassDB::bind_method(D_METHOD("get_morph_weights"), &LoomisCharacter3DResource::get_morph_weights);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "morph_weights"), "set_morph_weights", "get_morph_weights");

    ClassDB::bind_method(D_METHOD("set_semantic_tag_set", "tag_set"), &LoomisCharacter3DResource::set_semantic_tag_set);
    ClassDB::bind_method(D_METHOD("get_semantic_tag_set"), &LoomisCharacter3DResource::get_semantic_tag_set);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "semantic_tag_set", PROPERTY_HINT_RESOURCE_TYPE, "SemanticTagSetResource"),
        "set_semantic_tag_set",
        "get_semantic_tag_set"
    );

    ADD_GROUP("Tools & Tutorials", "");

    ClassDB::bind_method(D_METHOD("set_help_topic_id", "topic_id"), &LoomisCharacter3DResource::set_help_topic_id);
    ClassDB::bind_method(D_METHOD("get_help_topic_id"), &LoomisCharacter3DResource::get_help_topic_id);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "help_topic_id"), "set_help_topic_id", "get_help_topic_id");

    ClassDB::bind_method(D_METHOD("set_semantic_tags", "tags"), &LoomisCharacter3DResource::set_semantic_tags);
    ClassDB::bind_method(D_METHOD("get_semantic_tags"), &LoomisCharacter3DResource::get_semantic_tags);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "semantic_tags"), "set_semantic_tags", "get_semantic_tags");

    // --- Methods ---
    ClassDB::bind_method(D_METHOD("set_undo_redo", "undo_redo"), &LoomisCharacter3DResource::set_undo_redo);
    ClassDB::bind_method(D_METHOD("get_undo_redo"), &LoomisCharacter3DResource::get_undo_redo);
    ClassDB::bind_method(D_METHOD("ensure_default_body_plan"), &LoomisCharacter3DResource::ensure_default_body_plan);

    ClassDB::bind_method(D_METHOD("set_parameter_value", "name", "value"), &LoomisCharacter3DResource::set_parameter_value);
    ClassDB::bind_method(D_METHOD("commit_set_parameter_value", "name", "value"), &LoomisCharacter3DResource::commit_set_parameter_value);
    ClassDB::bind_method(D_METHOD("get_parameter_value", "name"), &LoomisCharacter3DResource::get_parameter_value);

    ClassDB::bind_method(D_METHOD("get_morph_channel_count"), &LoomisCharacter3DResource::get_morph_channel_count);
    ClassDB::bind_method(D_METHOD("get_morph_channel_name", "channel"), &LoomisCharacter3DResource::get_morph_channel_name);
    ClassDB::bind_method(D_METHOD("get_all_morph_channel_names"), &LoomisCharacter3DResource::get_all_morph_channel_names);
    ClassDB::bind_method(D_METHOD("set_morph_channel_weight", "channel", "weight"), &LoomisCharacter3DResource::set_morph_channel_weight);
    ClassDB::bind_method(D_METHOD("get_morph_channel_weight", "channel"), &LoomisCharacter3DResource::get_morph_channel_weight);
    ClassDB::bind_method(D_METHOD("reset_all_morph_weights"), &LoomisCharacter3DResource::reset_all_morph_weights);

    ClassDB::bind_method(D_METHOD("commit_set_body_archetype", "archetype", "undo_redo"), &LoomisCharacter3DResource::commit_set_body_archetype, DEFVAL(Ref<IdeamUndoRedo>()));
    ClassDB::bind_method(D_METHOD("commit_set_subdivision_lod_level", "lod", "undo_redo"), &LoomisCharacter3DResource::commit_set_subdivision_lod_level, DEFVAL(Ref<IdeamUndoRedo>()));
    ClassDB::bind_method(D_METHOD("commit_set_active_partition_mask", "mask", "undo_redo"), &LoomisCharacter3DResource::commit_set_active_partition_mask, DEFVAL(Ref<IdeamUndoRedo>()));
    ClassDB::bind_method(D_METHOD("commit_set_morph_weights", "weights", "undo_redo"), &LoomisCharacter3DResource::commit_set_morph_weights, DEFVAL(Ref<IdeamUndoRedo>()));
    ClassDB::bind_method(D_METHOD("commit_set_morph_channel_weight", "channel", "weight", "undo_redo"), &LoomisCharacter3DResource::commit_set_morph_channel_weight, DEFVAL(Ref<IdeamUndoRedo>()));
    ClassDB::bind_method(D_METHOD("commit_set_semantic_tag_set", "tag_set", "undo_redo"), &LoomisCharacter3DResource::commit_set_semantic_tag_set, DEFVAL(Ref<IdeamUndoRedo>()));

    ClassDB::bind_method(
        D_METHOD("set_scaffold_override", "scaffold_node_id", "position_offset", "scale_multiplier"),
        &LoomisCharacter3DResource::set_scaffold_override,
        DEFVAL(Vector3(1.0f, 1.0f, 1.0f))
    );
    ClassDB::bind_method(D_METHOD("set_topology_override", "landmark_id", "offset"), &LoomisCharacter3DResource::set_topology_override);
    ClassDB::bind_method(
        D_METHOD("set_feature_override", "feature_id", "offset", "scale_multiplier"),
        &LoomisCharacter3DResource::set_feature_override,
        DEFVAL(1.0)
    );
    ClassDB::bind_method(
        D_METHOD("add_detail_delta", "label", "scaffold_node_id", "u", "v", "local_delta_ntuv", "radius", "is_crease"),
        &LoomisCharacter3DResource::add_detail_delta,
        DEFVAL(0.05),
        DEFVAL(false)
    );
    ClassDB::bind_method(D_METHOD("clear_detail_deltas"), &LoomisCharacter3DResource::clear_detail_deltas);
    ClassDB::bind_method(D_METHOD("clear_all_detail_overrides"), &LoomisCharacter3DResource::clear_all_detail_overrides);
    ClassDB::bind_method(D_METHOD("get_detail_override_count"), &LoomisCharacter3DResource::get_detail_override_count);
    ClassDB::bind_method(D_METHOD("randomize_within_bounds", "seed"), &LoomisCharacter3DResource::randomize_within_bounds, DEFVAL(0));
    ClassDB::bind_method(D_METHOD("blend_with", "other", "alpha"), &LoomisCharacter3DResource::blend_with, DEFVAL(0.5));

    ClassDB::bind_method(D_METHOD("build_array_mesh", "lod_level"), &LoomisCharacter3DResource::build_array_mesh, DEFVAL(0));
    ClassDB::bind_method(D_METHOD("build_part_array_meshes", "lod_level"), &LoomisCharacter3DResource::build_part_array_meshes, DEFVAL(0));
    ClassDB::bind_method(D_METHOD("get_evaluated_bridge_regimes"), &LoomisCharacter3DResource::get_evaluated_bridge_regimes);
    ClassDB::bind_method(D_METHOD("build_skeleton_export_plan"), &LoomisCharacter3DResource::build_skeleton_export_plan);
    ClassDB::bind_method(D_METHOD("get_balance_report"), &LoomisCharacter3DResource::get_balance_report);

    ClassDB::bind_method(D_METHOD("get_base_vertex_count"), &LoomisCharacter3DResource::get_base_vertex_count);
    ClassDB::bind_method(D_METHOD("get_base_quad_count"), &LoomisCharacter3DResource::get_base_quad_count);
    ClassDB::bind_method(D_METHOD("get_sparse_morph_delta_count"), &LoomisCharacter3DResource::get_sparse_morph_delta_count);
    ClassDB::bind_method(D_METHOD("get_deformed_vertex_positions"), &LoomisCharacter3DResource::get_deformed_vertex_positions);
    ClassDB::bind_method(D_METHOD("get_deformed_vertex_normals"), &LoomisCharacter3DResource::get_deformed_vertex_normals);
    ClassDB::bind_method(D_METHOD("evaluate_garment_point", "quad_id", "u", "v", "d_normal"), &LoomisCharacter3DResource::evaluate_garment_point, DEFVAL(0.015f));
    ClassDB::bind_method(D_METHOD("build_evaluated_array_mesh"), &LoomisCharacter3DResource::build_evaluated_array_mesh);

    ClassDB::bind_method(D_METHOD("_on_body_plan_changed"), &LoomisCharacter3DResource::_on_body_plan_changed);
}

LoomisCharacter3DResource::LoomisCharacter3DResource() {
    core_mesh_.build_archetype_preset(core::CharacterBodyArchetype::BIPED);
    _ensure_morph_weights_size();
    ensure_default_body_plan();
}

LoomisCharacter3DResource::~LoomisCharacter3DResource() {
    if (body_plan_.is_valid()) {
        const godot::Callable cb(this, "_on_body_plan_changed");
        if (body_plan_->is_connected("changed", cb)) body_plan_->disconnect("changed", cb);
    }
}

void LoomisCharacter3DResource::_ensure_morph_weights_size() {
    if (morph_weights_.size() < static_cast<int64_t>(core::MORPH_DEFAULT_CHANNEL_COUNT)) {
        const int64_t old_sz = morph_weights_.size();
        morph_weights_.resize(static_cast<int64_t>(core::MORPH_DEFAULT_CHANNEL_COUNT));
        for (int64_t i = old_sz; i < morph_weights_.size(); ++i) {
            morph_weights_.set(i, 0.0f);
        }
    }
}

void LoomisCharacter3DResource::ensure_default_body_plan() {
    if (body_plan_.is_valid()) return;
    godot::Ref<BodyPlanResource> default_plan;
    default_plan.instantiate();
    default_plan->setup_loomis_humanoid_plan();
    set_body_plan(default_plan);
}

void LoomisCharacter3DResource::_on_body_plan_changed() {
    emit_changed();
}

void LoomisCharacter3DResource::set_body_plan(const godot::Ref<BodyPlanResource> &p_plan) {
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

void LoomisCharacter3DResource::set_character_name(const godot::String &p_name) {
    core_instance_.set_character_name(p_name.utf8().get_data());
    emit_changed();
}

godot::String LoomisCharacter3DResource::get_character_name() const {
    return godot::String::utf8(
        core_instance_.get_character_name().data(),
        static_cast<int64_t>(core_instance_.get_character_name().size())
    );
}

void LoomisCharacter3DResource::set_parameter_values(const godot::Dictionary &p_values) {
    const godot::Array keys = p_values.keys();
    for (int64_t i = 0; i < keys.size(); ++i) {
        const godot::String k = keys[i];
        core_instance_.set_parameter_value(k.utf8().get_data(), static_cast<float>(double(p_values[k])));
    }
    emit_changed();
}

godot::Dictionary LoomisCharacter3DResource::get_parameter_values() const {
    godot::Dictionary d;
    for (const auto &kv : core_instance_.parameter_values()) {
        d[godot::String::utf8(kv.first.c_str())] = static_cast<double>(kv.second);
    }
    return d;
}

void LoomisCharacter3DResource::set_details_enabled(bool p_enabled) {
    if (core_instance_.get_details_enabled() == p_enabled) return;
    core_instance_.set_details_enabled(p_enabled);
    emit_changed();
}

void LoomisCharacter3DResource::set_detail_deltas(const godot::TypedArray<godot::Dictionary> &p_deltas) {
    core_instance_.clear_detail_deltas();
    for (int64_t i = 0; i < p_deltas.size(); ++i) {
        const godot::Dictionary d = p_deltas[i];
        core::UnconstrainedDetailDelta dd{};
        dd.label            = godot::String(d.get("label", "")).utf8().get_data();
        dd.scaffold_node_id = static_cast<uint32_t>(std::max(0, int(d.get("scaffold_node_id", 0))));
        dd.u                = static_cast<float>(double(d.get("u", 0.5)));
        dd.v                = static_cast<float>(double(d.get("v", 0.5)));
        const godot::Vector3 v3 = d.get("local_delta_ntuv", godot::Vector3());
        dd.local_delta_ntuv = gd3_to_glm(v3);
        dd.radius           = static_cast<float>(double(d.get("radius", 0.05)));
        dd.is_crease        = bool(d.get("is_crease", false));
        core_instance_.add_detail_delta(dd);
    }
    emit_changed();
}

godot::TypedArray<godot::Dictionary> LoomisCharacter3DResource::get_detail_deltas() const {
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

void LoomisCharacter3DResource::set_base_color(const godot::Color &p_color) {
    base_color_ = p_color;
    emit_changed();
}

void LoomisCharacter3DResource::set_body_archetype(int p_archetype) {
    const int clamped = std::clamp(p_archetype, 0, static_cast<int>(core::CharacterBodyArchetype::CUSTOM));
    core_mesh_.build_archetype_preset(static_cast<core::CharacterBodyArchetype>(clamped));
    _ensure_morph_weights_size();
    emit_changed();
}

void LoomisCharacter3DResource::set_subdivision_lod_level(int p_lod) {
    subdivision_lod_level_ = std::clamp(p_lod, 0, 3);
    emit_changed();
}

void LoomisCharacter3DResource::set_active_partition_mask(int p_mask) {
    active_partition_mask_ = static_cast<uint32_t>(p_mask) & core::PARTITION_MASK_ALL;
    emit_changed();
}

void LoomisCharacter3DResource::set_morph_weights(const godot::PackedFloat32Array &p_weights) {
    morph_weights_ = p_weights;
    _ensure_morph_weights_size();
    emit_changed();
}

void LoomisCharacter3DResource::set_semantic_tag_set(const godot::Ref<SemanticTagSetResource> &p_tags) {
    semantic_tag_set_ = p_tags;
    emit_changed();
}

godot::String LoomisCharacter3DResource::get_morph_channel_name(int p_channel) const {
    return godot::String(morph_channel_label(p_channel));
}

godot::PackedStringArray LoomisCharacter3DResource::get_all_morph_channel_names() const {
    godot::PackedStringArray out;
    for (int i = 0; i < static_cast<int>(core::MORPH_DEFAULT_CHANNEL_COUNT); ++i) {
        out.append(godot::String(morph_channel_label(i)));
    }
    return out;
}

void LoomisCharacter3DResource::set_morph_channel_weight(int p_channel, float p_weight) {
    _ensure_morph_weights_size();
    if (p_channel < 0 || p_channel >= morph_weights_.size()) return;
    morph_weights_.set(p_channel, std::clamp(p_weight, -2.0f, 2.0f));
    emit_changed();
}

float LoomisCharacter3DResource::get_morph_channel_weight(int p_channel) const {
    if (p_channel < 0 || p_channel >= morph_weights_.size()) return 0.0f;
    return morph_weights_[p_channel];
}

void LoomisCharacter3DResource::reset_all_morph_weights() {
    _ensure_morph_weights_size();
    for (int64_t i = 0; i < morph_weights_.size(); ++i) {
        morph_weights_.set(i, 0.0f);
    }
    emit_changed();
}

godot::StringName LoomisCharacter3DResource::get_help_topic_id() const {
    if (help_topic_id_ != godot::StringName()) return help_topic_id_;
    return godot::StringName("LoomisCharacter3DResource");
}

void LoomisCharacter3DResource::set_semantic_tags(const godot::PackedStringArray &p_tags) {
    semantic_tags_ = p_tags;
    emit_changed();
}

void LoomisCharacter3DResource::set_parameter_value(const godot::String &p_name, real_t p_value) {
    core_instance_.set_parameter_value(p_name.utf8().get_data(), static_cast<float>(p_value));
    emit_changed();
}

void LoomisCharacter3DResource::commit_set_parameter_value(const godot::String &p_name, real_t p_value) {
    const real_t old_val = get_parameter_value(p_name);
    if (undo_redo_.is_valid()) {
        undo_redo_->create_action(godot::String("Set Character Parameter: ") + p_name);
        undo_redo_->add_do_method(this, "set_parameter_value", p_name, p_value);
        undo_redo_->add_undo_method(this, "set_parameter_value", p_name, old_val);
        undo_redo_->commit_action();
    } else {
        set_parameter_value(p_name, p_value);
    }
}

real_t LoomisCharacter3DResource::get_parameter_value(const godot::String &p_name) const {
    if (body_plan_.is_valid()) {
        return static_cast<real_t>(core_instance_.get_parameter_value(p_name.utf8().get_data(), body_plan_->get_core_plan()));
    }
    auto it = core_instance_.parameter_values().find(p_name.utf8().get_data());
    return (it != core_instance_.parameter_values().end()) ? static_cast<real_t>(it->second) : static_cast<real_t>(0.0);
}

void LoomisCharacter3DResource::commit_set_body_archetype(int p_archetype, const godot::Ref<IdeamUndoRedo> &p_undo_redo) {
    const int old_val = get_body_archetype();
    godot::Ref<IdeamUndoRedo> ur = p_undo_redo.is_valid() ? p_undo_redo : (undo_redo_.is_valid() ? undo_redo_ : IdeamCharactersPlugin::create_undo_redo_wrapper());
    if (ur.is_valid()) {
        ur->create_action("Set Loomis Character Archetype");
        ur->add_do_property(this, "body_archetype", p_archetype);
        ur->add_undo_property(this, "body_archetype", old_val);
        ur->commit_action();
    } else {
        set_body_archetype(p_archetype);
    }
}

void LoomisCharacter3DResource::commit_set_subdivision_lod_level(int p_lod, const godot::Ref<IdeamUndoRedo> &p_undo_redo) {
    const int old_val = subdivision_lod_level_;
    godot::Ref<IdeamUndoRedo> ur = p_undo_redo.is_valid() ? p_undo_redo : (undo_redo_.is_valid() ? undo_redo_ : IdeamCharactersPlugin::create_undo_redo_wrapper());
    if (ur.is_valid()) {
        ur->create_action("Set Loomis Character Subdivision LOD");
        ur->add_do_property(this, "subdivision_lod_level", p_lod);
        ur->add_undo_property(this, "subdivision_lod_level", old_val);
        ur->commit_action();
    } else {
        set_subdivision_lod_level(p_lod);
    }
}

void LoomisCharacter3DResource::commit_set_active_partition_mask(int p_mask, const godot::Ref<IdeamUndoRedo> &p_undo_redo) {
    const int old_val = static_cast<int>(active_partition_mask_);
    godot::Ref<IdeamUndoRedo> ur = p_undo_redo.is_valid() ? p_undo_redo : (undo_redo_.is_valid() ? undo_redo_ : IdeamCharactersPlugin::create_undo_redo_wrapper());
    if (ur.is_valid()) {
        ur->create_action("Set Loomis Character Partition Mask");
        ur->add_do_property(this, "active_partition_mask", p_mask);
        ur->add_undo_property(this, "active_partition_mask", old_val);
        ur->commit_action();
    } else {
        set_active_partition_mask(p_mask);
    }
}

void LoomisCharacter3DResource::commit_set_morph_weights(const godot::PackedFloat32Array &p_weights, const godot::Ref<IdeamUndoRedo> &p_undo_redo) {
    const godot::PackedFloat32Array old_val = morph_weights_;
    godot::Ref<IdeamUndoRedo> ur = p_undo_redo.is_valid() ? p_undo_redo : (undo_redo_.is_valid() ? undo_redo_ : IdeamCharactersPlugin::create_undo_redo_wrapper());
    if (ur.is_valid()) {
        ur->create_action("Set Loomis Character Morph Weights");
        ur->add_do_property(this, "morph_weights", p_weights);
        ur->add_undo_property(this, "morph_weights", old_val);
        ur->commit_action();
    } else {
        set_morph_weights(p_weights);
    }
}

void LoomisCharacter3DResource::commit_set_morph_channel_weight(int p_channel, float p_weight, const godot::Ref<IdeamUndoRedo> &p_undo_redo) {
    const float old_val = get_morph_channel_weight(p_channel);
    godot::Ref<IdeamUndoRedo> ur = p_undo_redo.is_valid() ? p_undo_redo : (undo_redo_.is_valid() ? undo_redo_ : IdeamCharactersPlugin::create_undo_redo_wrapper());
    if (ur.is_valid()) {
        ur->create_action(godot::String("Adjust Morph: ") + get_morph_channel_name(p_channel));
        ur->add_do_method(this, "set_morph_channel_weight", p_channel, p_weight);
        ur->add_undo_method(this, "set_morph_channel_weight", p_channel, old_val);
        ur->commit_action();
    } else {
        set_morph_channel_weight(p_channel, p_weight);
    }
}

void LoomisCharacter3DResource::commit_set_semantic_tag_set(const godot::Ref<SemanticTagSetResource> &p_tags, const godot::Ref<IdeamUndoRedo> &p_undo_redo) {
    const godot::Ref<SemanticTagSetResource> old_val = semantic_tag_set_;
    godot::Ref<IdeamUndoRedo> ur = p_undo_redo.is_valid() ? p_undo_redo : (undo_redo_.is_valid() ? undo_redo_ : IdeamCharactersPlugin::create_undo_redo_wrapper());
    if (ur.is_valid()) {
        ur->create_action("Set Loomis Character Semantic Tags");
        ur->add_do_property(this, "semantic_tag_set", p_tags);
        ur->add_undo_property(this, "semantic_tag_set", old_val);
        ur->commit_action();
    } else {
        set_semantic_tag_set(p_tags);
    }
}

void LoomisCharacter3DResource::set_scaffold_override(
    int p_scaffold_node_id,
    const godot::Vector3 &p_position_offset,
    const godot::Vector3 &p_scale_multiplier
) {
    core::ScaffoldDetailOverride ov{};
    ov.scaffold_node_id = static_cast<uint32_t>(std::max(0, p_scaffold_node_id));
    ov.offset_delta  = gd3_to_glm(p_position_offset);
    ov.scale_multiplier = gd3_to_glm(p_scale_multiplier);
    core_instance_.set_scaffold_override(ov);
    emit_changed();
}

void LoomisCharacter3DResource::set_topology_override(int p_landmark_id, const godot::Vector3 &p_offset) {
    core::TopologyDetailOverride ov{};
    ov.landmark_id     = static_cast<uint32_t>(std::max(0, p_landmark_id));
    ov.normal_offset_delta = gd3_to_glm(p_offset);
    core_instance_.set_topology_override(ov);
    emit_changed();
}

void LoomisCharacter3DResource::set_feature_override(
    int p_feature_id,
    const godot::Vector3 &p_offset,
    real_t p_scale_multiplier
) {
    core::FeatureDetailOverride ov{};
    ov.feature_id       = static_cast<uint32_t>(std::max(0, p_feature_id));
    ov.offset_delta_ntuv = gd3_to_glm(p_offset);
    ov.scale_multiplier = static_cast<float>(p_scale_multiplier);
    core_instance_.set_feature_override(ov);
    emit_changed();
}

int LoomisCharacter3DResource::add_detail_delta(
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
    dd.local_delta_ntuv = gd3_to_glm(p_local_delta_ntuv);
    dd.radius           = static_cast<float>(p_radius);
    dd.is_crease        = p_is_crease;
    const uint32_t id = core_instance_.add_detail_delta(dd);
    emit_changed();
    return static_cast<int>(id);
}

void LoomisCharacter3DResource::clear_detail_deltas() {
    core_instance_.clear_detail_deltas();
    emit_changed();
}

void LoomisCharacter3DResource::clear_all_detail_overrides() {
    core_instance_.clear_all_detail_overrides();
    emit_changed();
}

int LoomisCharacter3DResource::get_detail_override_count() const {
    const auto &ov = core_instance_.detail_overrides();
    return static_cast<int>(
        ov.scaffold_overrides.size() +
        ov.topology_overrides.size() +
        ov.feature_overrides.size() +
        ov.surface_sculpt_deltas.size()
    );
}

void LoomisCharacter3DResource::randomize_within_bounds(int64_t p_seed) {
    if (!body_plan_.is_valid()) return;
    core_instance_.randomize_within_bounds(body_plan_->get_core_plan(), static_cast<uint64_t>(p_seed));
    emit_changed();
}

godot::Ref<LoomisCharacter3DResource> LoomisCharacter3DResource::blend_with(
    const godot::Ref<LoomisCharacter3DResource> &p_other,
    real_t p_alpha
) const {
    godot::Ref<LoomisCharacter3DResource> out;
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

godot::Ref<godot::ArrayMesh> LoomisCharacter3DResource::_quad_mesh_to_array_mesh(
    const core::SurfaceQuadMesh3D<float> &p_qmesh
) const {
    godot::Ref<godot::ArrayMesh> mesh;
    mesh.instantiate();
    const size_t vcount = p_qmesh.positions.size();
    const std::vector<uint32_t> tris = p_qmesh.build_triangle_indices();
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
        verts.set(static_cast<int64_t>(i), glm3_to_gd(p_qmesh.positions[i]));
        norms.set(static_cast<int64_t>(i), glm3_to_gd(p_qmesh.normals[i]));
        tans.set(static_cast<int64_t>(i * 4u + 0u), p_qmesh.tangents[i].x);
        tans.set(static_cast<int64_t>(i * 4u + 1u), p_qmesh.tangents[i].y);
        tans.set(static_cast<int64_t>(i * 4u + 2u), p_qmesh.tangents[i].z);
        tans.set(static_cast<int64_t>(i * 4u + 3u), p_qmesh.tangents[i].w);
        uvs.set(static_cast<int64_t>(i), godot::Vector2(p_qmesh.uvs[i].x, p_qmesh.uvs[i].y));
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

godot::Ref<godot::ArrayMesh> LoomisCharacter3DResource::build_array_mesh(int p_lod_level) const {
    if (!body_plan_.is_valid()) {
        godot::Ref<godot::ArrayMesh> empty;
        empty.instantiate();
        return empty;
    }
    const uint8_t lod = static_cast<uint8_t>(std::clamp(p_lod_level, 0, 3));
    const auto evaluated = core_instance_.evaluate_stack(body_plan_->get_core_plan(), lod);
    return _quad_mesh_to_array_mesh(evaluated.combined_quad_mesh);
}

godot::Dictionary LoomisCharacter3DResource::build_part_array_meshes(int p_lod_level) const {
    godot::Dictionary out;
    if (!body_plan_.is_valid()) return out;
    const uint8_t lod = static_cast<uint8_t>(std::clamp(p_lod_level, 0, 3));
    const auto evaluated = core_instance_.evaluate_stack(body_plan_->get_core_plan(), lod);
    for (const auto &kv : evaluated.part_meshes) {
        out[static_cast<int64_t>(kv.first)] = _quad_mesh_to_array_mesh(kv.second);
    }
    return out;
}

godot::TypedArray<godot::Dictionary> LoomisCharacter3DResource::get_evaluated_bridge_regimes() const {
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

godot::TypedArray<godot::Dictionary> LoomisCharacter3DResource::build_skeleton_export_plan() const {
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

godot::Dictionary LoomisCharacter3DResource::get_balance_report() const {
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

void LoomisCharacter3DResource::evaluate_deformed_buffers(
    std::vector<glm::vec3> &r_positions,
    std::vector<glm::vec3> &r_normals,
    std::vector<uint32_t> *r_active_masks
) const {
    const size_t n_verts = core_mesh_.vertex_count();
    r_positions.assign(n_verts, glm::vec3(0.0f));
    r_normals.assign(n_verts, glm::vec3(0.0f, 1.0f, 0.0f));
    if (n_verts == 0u) return;

    if (r_active_masks) {
        r_active_masks->assign(n_verts, 0u);
        core::ParametricCharacterMesh::evaluate_lod_visibility_masks(
            std::span<const core::QuadLoopVertexPod>(core_mesh_.base_vertices()),
            static_cast<uint32_t>(subdivision_lod_level_),
            active_partition_mask_,
            r_active_masks->data(),
            n_verts
        );
    }

    std::vector<float> weights(static_cast<size_t>(morph_weights_.size()), 0.0f);
    for (int64_t i = 0; i < morph_weights_.size(); ++i) {
        weights[static_cast<size_t>(i)] = morph_weights_[i];
    }

    core::ParametricCharacterMesh::evaluate_sparse_morph_sliders(
        std::span<const core::QuadLoopVertexPod>(core_mesh_.base_vertices()),
        std::span<const core::MorphDeltaPod>(core_mesh_.sparse_morph_deltas()),
        std::span<const float>(weights),
        active_partition_mask_,
        r_positions.data(),
        r_normals.data(),
        n_verts
    );
}

godot::PackedVector3Array LoomisCharacter3DResource::get_deformed_vertex_positions() const {
    std::vector<glm::vec3> pos, nrm;
    evaluate_deformed_buffers(pos, nrm);
    godot::PackedVector3Array out;
    out.resize(static_cast<int>(pos.size()));
    for (size_t i = 0; i < pos.size(); ++i) {
        out.set(static_cast<int>(i), godot::Vector3(pos[i].x, pos[i].y, pos[i].z));
    }
    return out;
}

godot::PackedVector3Array LoomisCharacter3DResource::get_deformed_vertex_normals() const {
    std::vector<glm::vec3> pos, nrm;
    evaluate_deformed_buffers(pos, nrm);
    godot::PackedVector3Array out;
    out.resize(static_cast<int>(nrm.size()));
    for (size_t i = 0; i < nrm.size(); ++i) {
        out.set(static_cast<int>(i), godot::Vector3(nrm[i].x, nrm[i].y, nrm[i].z));
    }
    return out;
}

godot::Vector3 LoomisCharacter3DResource::evaluate_garment_point(
    int p_quad_id,
    float p_u,
    float p_v,
    float p_d_normal
) const {
    std::vector<glm::vec3> pos, nrm;
    evaluate_deformed_buffers(pos, nrm);
    core::SurfaceGarmentBindPod bind{};
    bind.quad_id  = static_cast<uint32_t>(std::max(0, p_quad_id));
    bind.u        = p_u;
    bind.v        = p_v;
    bind.d_normal = p_d_normal;
    const glm::vec3 p = core::ParametricCharacterMesh::evaluate_garment_surface_point_on_buffers(
        bind,
        std::span<const core::QuadFacePod>(core_mesh_.base_quads()),
        std::span<const glm::vec3>(pos),
        std::span<const glm::vec3>(nrm)
    );
    return godot::Vector3(p.x, p.y, p.z);
}

godot::Ref<godot::ArrayMesh> LoomisCharacter3DResource::build_evaluated_array_mesh() const {
    using namespace godot;
    Ref<ArrayMesh> mesh;
    mesh.instantiate();

    std::vector<glm::vec3> pos, nrm;
    std::vector<uint32_t> active_masks;
    evaluate_deformed_buffers(pos, nrm, &active_masks);
    if (pos.empty() || core_mesh_.base_quads().empty()) return mesh;

    PackedVector3Array gd_verts;
    PackedVector3Array gd_norms;
    PackedVector2Array gd_uvs;
    PackedInt32Array gd_indices;

    gd_verts.resize(static_cast<int>(pos.size()));
    gd_norms.resize(static_cast<int>(nrm.size()));
    gd_uvs.resize(static_cast<int>(pos.size()));

    for (size_t i = 0; i < pos.size(); ++i) {
        gd_verts.set(static_cast<int>(i), Vector3(pos[i].x, pos[i].y, pos[i].z));
        gd_norms.set(static_cast<int>(i), Vector3(nrm[i].x, nrm[i].y, nrm[i].z));
        const float u = static_cast<float>(i & 3u) / 3.0f;
        const float v = static_cast<float>(core_mesh_.base_vertices()[i].ring_index) * 0.1f;
        gd_uvs.set(static_cast<int>(i), Vector2(u, v));
    }

    for (const auto &q : core_mesh_.base_quads()) {
        if (q[0] >= pos.size() || q[1] >= pos.size() || q[2] >= pos.size() || q[3] >= pos.size()) continue;
        const uint8_t q_lod = std::max({
            core_mesh_.base_vertices()[q[0]].min_lod_level,
            core_mesh_.base_vertices()[q[1]].min_lod_level,
            core_mesh_.base_vertices()[q[2]].min_lod_level,
            core_mesh_.base_vertices()[q[3]].min_lod_level
        });
        if (q_lod > static_cast<uint8_t>(subdivision_lod_level_)) continue;
        const uint16_t part_id = core_mesh_.base_vertices()[q[0]].partition_id;
        if (part_id < 32u && ((active_partition_mask_ & (1u << part_id)) == 0u)) continue;

        gd_indices.push_back(static_cast<int32_t>(q[0]));
        gd_indices.push_back(static_cast<int32_t>(q[1]));
        gd_indices.push_back(static_cast<int32_t>(q[2]));

        gd_indices.push_back(static_cast<int32_t>(q[0]));
        gd_indices.push_back(static_cast<int32_t>(q[2]));
        gd_indices.push_back(static_cast<int32_t>(q[3]));
    }

    if (gd_indices.size() >= 3) {
        Array arrays;
        arrays.resize(Mesh::ARRAY_MAX);
        arrays[Mesh::ARRAY_VERTEX] = gd_verts;
        arrays[Mesh::ARRAY_NORMAL] = gd_norms;
        arrays[Mesh::ARRAY_TEX_UV] = gd_uvs;
        arrays[Mesh::ARRAY_INDEX]  = gd_indices;
        mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);
    }

    return mesh;
}

} // namespace ideam::godot_ext
