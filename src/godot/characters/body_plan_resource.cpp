#include "body_plan_resource.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

#include <algorithm>

namespace ideam::godot_ext {

void BodyPlanResource::_bind_methods() {
    using namespace godot;

    BIND_ENUM_CONSTANT(BODY_PLAN_HUMANOID);
    BIND_ENUM_CONSTANT(BODY_PLAN_QUADRUPED);
    BIND_ENUM_CONSTANT(BODY_PLAN_AVIAN);

    ADD_GROUP("Body Plan Schema", "");

    ClassDB::bind_method(D_METHOD("set_species_name", "name"), &BodyPlanResource::set_species_name);
    ClassDB::bind_method(D_METHOD("get_species_name"), &BodyPlanResource::get_species_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "species_name"), "set_species_name", "get_species_name");

    ClassDB::bind_method(D_METHOD("set_archetype_preset", "preset"), &BodyPlanResource::set_archetype_preset);
    ClassDB::bind_method(D_METHOD("get_archetype_preset"), &BodyPlanResource::get_archetype_preset);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "archetype_preset", PROPERTY_HINT_ENUM, "Loomis Humanoid:0,Loomis Quadruped:1,Loomis Avian:2"),
        "set_archetype_preset",
        "get_archetype_preset"
    );

    ClassDB::bind_method(D_METHOD("set_base_scaffold", "scaffold"), &BodyPlanResource::set_base_scaffold);
    ClassDB::bind_method(D_METHOD("get_base_scaffold"), &BodyPlanResource::get_base_scaffold);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "base_scaffold", PROPERTY_HINT_RESOURCE_TYPE, "FormScaffoldResource"),
        "set_base_scaffold",
        "get_base_scaffold"
    );

    ADD_GROUP("Tools & Tutorials", "");

    ClassDB::bind_method(D_METHOD("set_help_topic_id", "topic_id"), &BodyPlanResource::set_help_topic_id);
    ClassDB::bind_method(D_METHOD("get_help_topic_id"), &BodyPlanResource::get_help_topic_id);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "help_topic_id"), "set_help_topic_id", "get_help_topic_id");

    ClassDB::bind_method(D_METHOD("set_semantic_tags", "tags"), &BodyPlanResource::set_semantic_tags);
    ClassDB::bind_method(D_METHOD("get_semantic_tags"), &BodyPlanResource::get_semantic_tags);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "semantic_tags"), "set_semantic_tags", "get_semantic_tags");

    ClassDB::bind_method(D_METHOD("set_undo_redo", "undo_redo"), &BodyPlanResource::set_undo_redo);
    ClassDB::bind_method(D_METHOD("get_undo_redo"), &BodyPlanResource::get_undo_redo);

    ClassDB::bind_method(
        D_METHOD("add_parameter_bound", "name", "category", "target_name", "min_val", "default_val", "max_val", "symmetry"),
        &BodyPlanResource::add_parameter_bound,
        DEFVAL(1)
    );
    ClassDB::bind_method(D_METHOD("add_coupling_rule", "source_param", "target_param", "weight"), &BodyPlanResource::add_coupling_rule);
    ClassDB::bind_method(
        D_METHOD("add_topology_landmark", "name", "scaffold_node_id", "u", "v", "normal_offset", "symmetry", "planar_sharpness"),
        &BodyPlanResource::add_topology_landmark,
        DEFVAL(0.0),
        DEFVAL(1),
        DEFVAL(0.25)
    );
    ClassDB::bind_method(
        D_METHOD("add_bridge_rule", "name", "node_a", "node_b", "driving_param", "crease_threshold", "deep_fold_threshold"),
        &BodyPlanResource::add_bridge_rule,
        DEFVAL(0.50),
        DEFVAL(0.80)
    );
    ClassDB::bind_method(
        D_METHOD("add_feature", "name", "kind", "parent_landmark_id", "half_extents", "symmetry", "secondary_param"),
        &BodyPlanResource::add_feature,
        DEFVAL(1),
        DEFVAL(0.0)
    );

    ClassDB::bind_method(D_METHOD("get_parameter_bounds_list"), &BodyPlanResource::get_parameter_bounds_list);
    ClassDB::bind_method(D_METHOD("get_topology_landmarks_list"), &BodyPlanResource::get_topology_landmarks_list);
    ClassDB::bind_method(D_METHOD("get_bridge_rules_list"), &BodyPlanResource::get_bridge_rules_list);
    ClassDB::bind_method(D_METHOD("get_features_list"), &BodyPlanResource::get_features_list);

    ClassDB::bind_method(D_METHOD("setup_loomis_humanoid_plan"), &BodyPlanResource::setup_loomis_humanoid_plan);
    ClassDB::bind_method(D_METHOD("setup_loomis_quadruped_plan"), &BodyPlanResource::setup_loomis_quadruped_plan);
    ClassDB::bind_method(D_METHOD("setup_loomis_avian_plan"), &BodyPlanResource::setup_loomis_avian_plan);
    ClassDB::bind_method(D_METHOD("_on_scaffold_changed"), &BodyPlanResource::_on_scaffold_changed);
}

BodyPlanResource::BodyPlanResource() {
    setup_loomis_humanoid_plan();
}

BodyPlanResource::~BodyPlanResource() {
    if (base_scaffold_.is_valid()) {
        const godot::Callable cb(this, "_on_scaffold_changed");
        if (base_scaffold_->is_connected("changed", cb)) base_scaffold_->disconnect("changed", cb);
    }
}

void BodyPlanResource::_on_scaffold_changed() {
    if (base_scaffold_.is_valid()) {
        core_plan_.set_base_scaffold(base_scaffold_->get_core_scaffold());
        emit_changed();
    }
}

void BodyPlanResource::set_species_name(const godot::String &p_name) {
    core_plan_.set_species_name(p_name.utf8().get_data());
    emit_changed();
}

godot::String BodyPlanResource::get_species_name() const {
    return godot::String::utf8(core_plan_.get_species_name().data(), static_cast<int64_t>(core_plan_.get_species_name().size()));
}

void BodyPlanResource::set_archetype_preset(BodyPlanArchetypePreset p_preset) {
    preset_ = p_preset;
    switch (p_preset) {
        case BODY_PLAN_QUADRUPED: setup_loomis_quadruped_plan(); break;
        case BODY_PLAN_AVIAN:     setup_loomis_avian_plan(); break;
        case BODY_PLAN_HUMANOID:
        default:                  setup_loomis_humanoid_plan(); break;
    }
}

void BodyPlanResource::set_base_scaffold(const godot::Ref<FormScaffoldResource> &p_scaffold) {
    const godot::Callable cb(this, "_on_scaffold_changed");
    if (base_scaffold_.is_valid() && base_scaffold_->is_connected("changed", cb)) {
        base_scaffold_->disconnect("changed", cb);
    }
    base_scaffold_ = p_scaffold;
    if (base_scaffold_.is_valid()) {
        if (!base_scaffold_->is_connected("changed", cb)) base_scaffold_->connect("changed", cb);
        core_plan_.set_base_scaffold(base_scaffold_->get_core_scaffold());
    }
    emit_changed();
}

void BodyPlanResource::set_undo_redo(const godot::Ref<IdeamUndoRedo> &p_ur) {
    undo_redo_ = p_ur;
    if (base_scaffold_.is_valid()) base_scaffold_->set_undo_redo(p_ur);
}

godot::StringName BodyPlanResource::get_help_topic_id() const {
    if (help_topic_id_ != godot::StringName()) return help_topic_id_;
    return godot::StringName("BodyPlanResource");
}

void BodyPlanResource::set_semantic_tags(const godot::PackedStringArray &p_tags) {
    semantic_tags_ = p_tags;
    emit_changed();
}

int BodyPlanResource::add_parameter_bound(
    const godot::String &p_name,
    int p_category,
    const godot::String &p_target_name,
    real_t p_min_val,
    real_t p_default_val,
    real_t p_max_val,
    int p_symmetry
) {
    core::BodyPlanParameterBound b{};
    b.name          = p_name.utf8().get_data();
    b.category      = static_cast<core::BodyPlanParamCategory>(std::clamp(p_category, 0, 5));
    b.target_name   = p_target_name.utf8().get_data();
    b.min_value     = static_cast<float>(p_min_val);
    b.default_value = static_cast<float>(p_default_val);
    b.max_value     = static_cast<float>(p_max_val);
    b.symmetry      = static_cast<core::LayerSymmetryPolicy>(std::clamp(p_symmetry, 0, 2));
    const uint32_t idx = core_plan_.add_parameter_bound(b);
    emit_changed();
    return static_cast<int>(idx);
}

void BodyPlanResource::add_coupling_rule(
    const godot::String &p_source_param,
    const godot::String &p_target_param,
    real_t p_weight
) {
    core_plan_.add_coupling_rule({
        p_source_param.utf8().get_data(),
        p_target_param.utf8().get_data(),
        static_cast<float>(p_weight)
    });
    emit_changed();
}

int BodyPlanResource::add_topology_landmark(
    const godot::String &p_name,
    int p_scaffold_node_id,
    real_t p_u,
    real_t p_v,
    real_t p_normal_offset,
    int p_symmetry,
    real_t p_planar_sharpness
) {
    core::TopologyLandmark lm{};
    lm.name                    = p_name.utf8().get_data();
    lm.anchor.scaffold_node_id = static_cast<uint32_t>(std::max(0, p_scaffold_node_id));
    lm.anchor.u                = static_cast<float>(p_u);
    lm.anchor.v                = static_cast<float>(p_v);
    lm.anchor.normal_offset    = static_cast<float>(p_normal_offset);
    lm.symmetry                = static_cast<core::LayerSymmetryPolicy>(std::clamp(p_symmetry, 0, 2));
    lm.planar_sharpness        = static_cast<float>(p_planar_sharpness);
    const uint32_t id = core_plan_.add_topology_landmark(lm);
    emit_changed();
    return static_cast<int>(id);
}

int BodyPlanResource::add_bridge_rule(
    const godot::String &p_name,
    int p_node_a,
    int p_node_b,
    const godot::String &p_driving_param,
    real_t p_crease_threshold,
    real_t p_deep_fold_threshold
) {
    core::TopologicalBridgeRule br{};
    br.name                = p_name.utf8().get_data();
    br.node_a_id           = static_cast<uint32_t>(std::max(0, p_node_a));
    br.node_b_id           = static_cast<uint32_t>(std::max(0, p_node_b));
    br.driving_param_name  = p_driving_param.utf8().get_data();
    br.crease_threshold    = static_cast<float>(p_crease_threshold);
    br.deep_fold_threshold = static_cast<float>(p_deep_fold_threshold);
    const uint32_t id = core_plan_.add_bridge_rule(br);
    emit_changed();
    return static_cast<int>(id);
}

int BodyPlanResource::add_feature(
    const godot::String &p_name,
    int p_kind,
    int p_parent_landmark_id,
    const godot::Vector3 &p_half_extents,
    int p_symmetry,
    real_t p_secondary_param
) {
    core::ParametricFeatureSpec f{};
    f.name               = p_name.utf8().get_data();
    f.kind               = static_cast<core::ParametricFeatureKind>(std::clamp(p_kind, 0, 8));
    f.parent_landmark_id = static_cast<uint32_t>(std::max(0, p_parent_landmark_id));
    f.base_half_extents  = glm::vec3(p_half_extents.x, p_half_extents.y, p_half_extents.z);
    f.symmetry           = static_cast<core::LayerSymmetryPolicy>(std::clamp(p_symmetry, 0, 2));
    f.secondary_param    = static_cast<float>(p_secondary_param);
    const uint32_t id = core_plan_.add_feature(f);
    emit_changed();
    return static_cast<int>(id);
}

godot::TypedArray<godot::Dictionary> BodyPlanResource::get_parameter_bounds_list() const {
    godot::TypedArray<godot::Dictionary> out;
    for (const auto &b : core_plan_.parameter_bounds()) {
        godot::Dictionary d;
        d["name"]          = godot::String::utf8(b.name.c_str());
        d["category"]      = static_cast<int64_t>(b.category);
        d["target_name"]   = godot::String::utf8(b.target_name.c_str());
        d["min_value"]     = static_cast<double>(b.min_value);
        d["default_value"] = static_cast<double>(b.default_value);
        d["max_value"]     = static_cast<double>(b.max_value);
        d["symmetry"]      = static_cast<int64_t>(b.symmetry);
        out.append(d);
    }
    return out;
}

godot::TypedArray<godot::Dictionary> BodyPlanResource::get_topology_landmarks_list() const {
    godot::TypedArray<godot::Dictionary> out;
    for (const auto &lm : core_plan_.landmarks()) {
        godot::Dictionary d;
        d["id"]               = static_cast<int64_t>(lm.id);
        d["name"]             = godot::String::utf8(lm.name.c_str());
        d["scaffold_node_id"] = static_cast<int64_t>(lm.anchor.scaffold_node_id);
        d["u"]                = static_cast<double>(lm.anchor.u);
        d["v"]                = static_cast<double>(lm.anchor.v);
        d["normal_offset"]    = static_cast<double>(lm.anchor.normal_offset);
        d["planar_sharpness"] = static_cast<double>(lm.planar_sharpness);
        out.append(d);
    }
    return out;
}

godot::TypedArray<godot::Dictionary> BodyPlanResource::get_bridge_rules_list() const {
    godot::TypedArray<godot::Dictionary> out;
    for (const auto &br : core_plan_.bridge_rules()) {
        godot::Dictionary d;
        d["id"]                  = static_cast<int64_t>(br.id);
        d["name"]                = godot::String::utf8(br.name.c_str());
        d["driving_param"]       = godot::String::utf8(br.driving_param_name.c_str());
        d["crease_threshold"]    = static_cast<double>(br.crease_threshold);
        d["deep_fold_threshold"] = static_cast<double>(br.deep_fold_threshold);
        out.append(d);
    }
    return out;
}

godot::TypedArray<godot::Dictionary> BodyPlanResource::get_features_list() const {
    godot::TypedArray<godot::Dictionary> out;
    for (const auto &f : core_plan_.features()) {
        godot::Dictionary d;
        d["id"]                 = static_cast<int64_t>(f.id);
        d["name"]               = godot::String::utf8(f.name.c_str());
        d["kind"]               = static_cast<int64_t>(f.kind);
        d["parent_landmark_id"] = static_cast<int64_t>(f.parent_landmark_id);
        d["secondary_param"]    = static_cast<double>(f.secondary_param);
        out.append(d);
    }
    return out;
}

void BodyPlanResource::setup_loomis_humanoid_plan() {
    preset_ = BODY_PLAN_HUMANOID;
    core_plan_ = core::BodyPlanCore::create_loomis_humanoid_plan();
    if (!base_scaffold_.is_valid()) base_scaffold_.instantiate();
    base_scaffold_->setup_loomis_biped(LOOMIS_BIPED_NORMAL_7_5H, 0.23f);
    emit_changed();
}

void BodyPlanResource::setup_loomis_quadruped_plan() {
    preset_ = BODY_PLAN_QUADRUPED;
    core_plan_ = core::BodyPlanCore::create_loomis_quadruped_plan();
    if (!base_scaffold_.is_valid()) base_scaffold_.instantiate();
    base_scaffold_->setup_loomis_quadruped(0.80f);
    emit_changed();
}

void BodyPlanResource::setup_loomis_avian_plan() {
    preset_ = BODY_PLAN_AVIAN;
    core_plan_ = core::BodyPlanCore::create_loomis_avian_plan();
    if (!base_scaffold_.is_valid()) base_scaffold_.instantiate();
    base_scaffold_->setup_loomis_avian(0.22f);
    emit_changed();
}

} // namespace ideam::godot_ext
