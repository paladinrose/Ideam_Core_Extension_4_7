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

    // Serialized Schema Arrays (.tres persistence)
    ClassDB::bind_method(D_METHOD("set_parameter_bounds_list", "list"), &BodyPlanResource::set_parameter_bounds_list);
    ClassDB::bind_method(D_METHOD("get_parameter_bounds_list"), &BodyPlanResource::get_parameter_bounds_list);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "parameter_bounds"), "set_parameter_bounds_list", "get_parameter_bounds_list");

    ClassDB::bind_method(D_METHOD("set_coupling_rules_list", "list"), &BodyPlanResource::set_coupling_rules_list);
    ClassDB::bind_method(D_METHOD("get_coupling_rules_list"), &BodyPlanResource::get_coupling_rules_list);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "coupling_rules"), "set_coupling_rules_list", "get_coupling_rules_list");

    ClassDB::bind_method(D_METHOD("set_topology_landmarks_list", "list"), &BodyPlanResource::set_topology_landmarks_list);
    ClassDB::bind_method(D_METHOD("get_topology_landmarks_list"), &BodyPlanResource::get_topology_landmarks_list);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "landmarks"), "set_topology_landmarks_list", "get_topology_landmarks_list");

    ClassDB::bind_method(D_METHOD("set_bridge_rules_list", "list"), &BodyPlanResource::set_bridge_rules_list);
    ClassDB::bind_method(D_METHOD("get_bridge_rules_list"), &BodyPlanResource::get_bridge_rules_list);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "bridge_rules"), "set_bridge_rules_list", "get_bridge_rules_list");

    ClassDB::bind_method(D_METHOD("set_features_list", "list"), &BodyPlanResource::set_features_list);
    ClassDB::bind_method(D_METHOD("get_features_list"), &BodyPlanResource::get_features_list);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "features"), "set_features_list", "get_features_list");

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
    ClassDB::bind_method(D_METHOD("remove_parameter_bound", "index"), &BodyPlanResource::remove_parameter_bound);
    ClassDB::bind_method(D_METHOD("add_coupling_rule", "source_param", "target_param", "weight"), &BodyPlanResource::add_coupling_rule);
    ClassDB::bind_method(D_METHOD("remove_coupling_rule", "index"), &BodyPlanResource::remove_coupling_rule);
    ClassDB::bind_method(
        D_METHOD("add_topology_landmark", "name", "scaffold_node_id", "u", "v", "normal_offset", "symmetry", "planar_sharpness"),
        &BodyPlanResource::add_topology_landmark,
        DEFVAL(0.0),
        DEFVAL(1),
        DEFVAL(0.25)
    );
    ClassDB::bind_method(
        D_METHOD("update_topology_landmark_offset", "id", "normal_offset", "planar_sharpness"),
        &BodyPlanResource::update_topology_landmark_offset,
        DEFVAL(-1.0)
    );
    ClassDB::bind_method(D_METHOD("remove_topology_landmark", "id"), &BodyPlanResource::remove_topology_landmark);
    ClassDB::bind_method(
        D_METHOD("add_bridge_rule", "name", "node_a", "node_b", "driving_param", "crease_threshold", "deep_fold_threshold"),
        &BodyPlanResource::add_bridge_rule,
        DEFVAL(0.50),
        DEFVAL(0.80)
    );
    ClassDB::bind_method(D_METHOD("remove_bridge_rule", "id"), &BodyPlanResource::remove_bridge_rule);
    ClassDB::bind_method(
        D_METHOD("add_feature", "name", "kind", "parent_landmark_id", "half_extents", "symmetry", "secondary_param"),
        &BodyPlanResource::add_feature,
        DEFVAL(1),
        DEFVAL(0.0)
    );
    ClassDB::bind_method(
        D_METHOD("update_feature_params", "id", "half_extents", "secondary_param", "local_offset_ntuv"),
        &BodyPlanResource::update_feature_params,
        DEFVAL(Vector3())
    );
    ClassDB::bind_method(D_METHOD("remove_feature", "id"), &BodyPlanResource::remove_feature);

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

// ============================================================================
// Serialized Array Setters & Getters (.tres persistence)
// ============================================================================

void BodyPlanResource::set_parameter_bounds_list(const godot::TypedArray<godot::Dictionary> &p_list) {
    core_plan_.clear_parameter_bounds();
    for (int64_t i = 0; i < p_list.size(); ++i) {
        const godot::Dictionary d = p_list[i];
        core::BodyPlanParameterBound b{};
        b.name          = godot::String(d.get("name", "")).utf8().get_data();
        b.category      = static_cast<core::BodyPlanParamCategory>(std::clamp(int(d.get("category", 1)), 0, 5));
        b.target_name   = godot::String(d.get("target_name", "")).utf8().get_data();
        b.min_value     = static_cast<float>(double(d.get("min_value", 0.0)));
        b.default_value = static_cast<float>(double(d.get("default_value", 0.5)));
        b.max_value     = static_cast<float>(double(d.get("max_value", 1.0)));
        b.symmetry      = static_cast<core::LayerSymmetryPolicy>(std::clamp(int(d.get("symmetry", 1)), 0, 2));
        core_plan_.add_parameter_bound(b);
    }
    emit_changed();
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

void BodyPlanResource::set_coupling_rules_list(const godot::TypedArray<godot::Dictionary> &p_list) {
    core_plan_.clear_coupling_rules();
    for (int64_t i = 0; i < p_list.size(); ++i) {
        const godot::Dictionary d = p_list[i];
        core_plan_.add_coupling_rule({
            godot::String(d.get("source_param", "")).utf8().get_data(),
            godot::String(d.get("target_param", "")).utf8().get_data(),
            static_cast<float>(double(d.get("weight", 0.5)))
        });
    }
    emit_changed();
}

godot::TypedArray<godot::Dictionary> BodyPlanResource::get_coupling_rules_list() const {
    godot::TypedArray<godot::Dictionary> out;
    for (const auto &c : core_plan_.coupling_rules()) {
        godot::Dictionary d;
        d["source_param"] = godot::String::utf8(c.source_param.c_str());
        d["target_param"] = godot::String::utf8(c.target_param.c_str());
        d["weight"]       = static_cast<double>(c.weight);
        out.append(d);
    }
    return out;
}

void BodyPlanResource::set_topology_landmarks_list(const godot::TypedArray<godot::Dictionary> &p_list) {
    core_plan_.clear_landmarks();
    for (int64_t i = 0; i < p_list.size(); ++i) {
        const godot::Dictionary d = p_list[i];
        core::TopologyLandmark lm{};
        lm.name                    = godot::String(d.get("name", "")).utf8().get_data();
        lm.anchor.scaffold_node_id = static_cast<uint32_t>(std::max(0, int(d.get("scaffold_node_id", 0))));
        lm.anchor.u                = static_cast<float>(double(d.get("u", 0.5)));
        lm.anchor.v                = static_cast<float>(double(d.get("v", 0.5)));
        lm.anchor.normal_offset    = static_cast<float>(double(d.get("normal_offset", 0.0)));
        lm.symmetry                = static_cast<core::LayerSymmetryPolicy>(std::clamp(int(d.get("symmetry", 1)), 0, 2));
        lm.radial_count            = static_cast<uint8_t>(std::clamp(int(d.get("radial_count", 1)), 1, 16));
        lm.influence_radius        = static_cast<float>(double(d.get("influence_radius", 0.08)));
        lm.planar_sharpness        = static_cast<float>(double(d.get("planar_sharpness", 0.25)));
        core_plan_.add_topology_landmark(lm);
    }
    emit_changed();
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
        d["symmetry"]         = static_cast<int64_t>(lm.symmetry);
        d["radial_count"]     = static_cast<int64_t>(lm.radial_count);
        d["influence_radius"] = static_cast<double>(lm.influence_radius);
        d["planar_sharpness"] = static_cast<double>(lm.planar_sharpness);
        out.append(d);
    }
    return out;
}

void BodyPlanResource::set_bridge_rules_list(const godot::TypedArray<godot::Dictionary> &p_list) {
    core_plan_.clear_bridge_rules();
    for (int64_t i = 0; i < p_list.size(); ++i) {
        const godot::Dictionary d = p_list[i];
        core::TopologicalBridgeRule br{};
        br.name                = godot::String(d.get("name", "")).utf8().get_data();
        br.node_a_id           = static_cast<uint32_t>(std::max(0, int(d.get("node_a_id", 0))));
        br.node_b_id           = static_cast<uint32_t>(std::max(0, int(d.get("node_b_id", 0))));
        br.driving_param_name  = godot::String(d.get("driving_param", "")).utf8().get_data();
        br.crease_threshold    = static_cast<float>(double(d.get("crease_threshold", 0.50)));
        br.deep_fold_threshold = static_cast<float>(double(d.get("deep_fold_threshold", 0.80)));
        br.pinch_fold_depth    = static_cast<float>(double(d.get("pinch_fold_depth", 0.03)));
        br.stretch_smooth_bias = static_cast<float>(double(d.get("stretch_smooth_bias", 0.50)));
        core_plan_.add_bridge_rule(br);
    }
    emit_changed();
}

godot::TypedArray<godot::Dictionary> BodyPlanResource::get_bridge_rules_list() const {
    godot::TypedArray<godot::Dictionary> out;
    for (const auto &br : core_plan_.bridge_rules()) {
        godot::Dictionary d;
        d["id"]                  = static_cast<int64_t>(br.id);
        d["name"]                = godot::String::utf8(br.name.c_str());
        d["node_a_id"]           = static_cast<int64_t>(br.node_a_id);
        d["node_b_id"]           = static_cast<int64_t>(br.node_b_id);
        d["driving_param"]       = godot::String::utf8(br.driving_param_name.c_str());
        d["crease_threshold"]    = static_cast<double>(br.crease_threshold);
        d["deep_fold_threshold"] = static_cast<double>(br.deep_fold_threshold);
        d["pinch_fold_depth"]    = static_cast<double>(br.pinch_fold_depth);
        d["stretch_smooth_bias"] = static_cast<double>(br.stretch_smooth_bias);
        out.append(d);
    }
    return out;
}

void BodyPlanResource::set_features_list(const godot::TypedArray<godot::Dictionary> &p_list) {
    core_plan_.clear_features();
    for (int64_t i = 0; i < p_list.size(); ++i) {
        const godot::Dictionary d = p_list[i];
        core::ParametricFeatureSpec f{};
        f.name               = godot::String(d.get("name", "")).utf8().get_data();
        f.kind               = static_cast<core::ParametricFeatureKind>(std::clamp(int(d.get("kind", 0)), 0, 10));
        f.parent_landmark_id = static_cast<uint32_t>(std::max(0, int(d.get("parent_landmark_id", 0))));
        f.symmetry           = static_cast<core::LayerSymmetryPolicy>(std::clamp(int(d.get("symmetry", 1)), 0, 2));
        const godot::Vector3 off = d.get("local_offset_ntuv", godot::Vector3());
        f.local_offset_ntuv  = glm::vec3(off.x, off.y, off.z);
        const godot::Vector3 ext = d.get("half_extents", godot::Vector3(0.02f, 0.02f, 0.02f));
        f.base_half_extents  = glm::vec3(ext.x, ext.y, ext.z);
        f.secondary_param    = static_cast<float>(double(d.get("secondary_param", 0.0)));
        f.planar_sharpness   = static_cast<float>(double(d.get("planar_sharpness", 0.2)));
        core_plan_.add_feature(f);
    }
    emit_changed();
}

godot::TypedArray<godot::Dictionary> BodyPlanResource::get_features_list() const {
    godot::TypedArray<godot::Dictionary> out;
    for (const auto &f : core_plan_.features()) {
        godot::Dictionary d;
        d["id"]                 = static_cast<int64_t>(f.id);
        d["name"]               = godot::String::utf8(f.name.c_str());
        d["kind"]               = static_cast<int64_t>(f.kind);
        d["parent_landmark_id"] = static_cast<int64_t>(f.parent_landmark_id);
        d["symmetry"]           = static_cast<int64_t>(f.symmetry);
        d["local_offset_ntuv"]  = godot::Vector3(f.local_offset_ntuv.x, f.local_offset_ntuv.y, f.local_offset_ntuv.z);
        d["half_extents"]       = godot::Vector3(f.base_half_extents.x, f.base_half_extents.y, f.base_half_extents.z);
        d["secondary_param"]    = static_cast<double>(f.secondary_param);
        d["planar_sharpness"]   = static_cast<double>(f.planar_sharpness);
        out.append(d);
    }
    return out;
}

// ============================================================================
// CRUD Mutators
// ============================================================================

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

bool BodyPlanResource::remove_parameter_bound(int p_index) {
    if (p_index < 0) return false;
    const bool ok = core_plan_.remove_parameter_bound(static_cast<uint32_t>(p_index));
    if (ok) emit_changed();
    return ok;
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

bool BodyPlanResource::remove_coupling_rule(int p_index) {
    if (p_index < 0) return false;
    const bool ok = core_plan_.remove_coupling_rule(static_cast<uint32_t>(p_index));
    if (ok) emit_changed();
    return ok;
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

bool BodyPlanResource::update_topology_landmark_offset(int p_id, real_t p_normal_offset, real_t p_planar_sharpness) {
    if (p_id < 0) return false;
    for (const auto &lm : core_plan_.landmarks()) {
        if (lm.id == static_cast<uint32_t>(p_id)) {
            core::TopologyLandmark updated = lm;
            updated.anchor.normal_offset = static_cast<float>(p_normal_offset);
            if (p_planar_sharpness >= static_cast<real_t>(0.0)) {
                updated.planar_sharpness = std::clamp(static_cast<float>(p_planar_sharpness), 0.0f, 1.0f);
            }
            const bool ok = core_plan_.update_topology_landmark(lm.id, updated);
            if (ok) emit_changed();
            return ok;
        }
    }
    return false;
}

bool BodyPlanResource::remove_topology_landmark(int p_id) {
    if (p_id < 0) return false;
    const bool ok = core_plan_.remove_topology_landmark(static_cast<uint32_t>(p_id));
    if (ok) emit_changed();
    return ok;
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

bool BodyPlanResource::remove_bridge_rule(int p_id) {
    if (p_id < 0) return false;
    const bool ok = core_plan_.remove_bridge_rule(static_cast<uint32_t>(p_id));
    if (ok) emit_changed();
    return ok;
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
    f.kind               = static_cast<core::ParametricFeatureKind>(std::clamp(p_kind, 0, 10));
    f.parent_landmark_id = static_cast<uint32_t>(std::max(0, p_parent_landmark_id));
    f.base_half_extents  = glm::vec3(p_half_extents.x, p_half_extents.y, p_half_extents.z);
    f.symmetry           = static_cast<core::LayerSymmetryPolicy>(std::clamp(p_symmetry, 0, 2));
    f.secondary_param    = static_cast<float>(p_secondary_param);
    const uint32_t id = core_plan_.add_feature(f);
    emit_changed();
    return static_cast<int>(id);
}

bool BodyPlanResource::update_feature_params(
    int p_id,
    const godot::Vector3 &p_half_extents,
    real_t p_secondary_param,
    const godot::Vector3 &p_local_offset_ntuv
) {
    if (p_id < 0) return false;
    for (const auto &f : core_plan_.features()) {
        if (f.id == static_cast<uint32_t>(p_id)) {
            core::ParametricFeatureSpec updated = f;
            updated.base_half_extents = glm::vec3(
                std::max(0.002f, static_cast<float>(p_half_extents.x)),
                std::max(0.002f, static_cast<float>(p_half_extents.y)),
                std::max(0.002f, static_cast<float>(p_half_extents.z))
            );
            updated.secondary_param   = static_cast<float>(p_secondary_param);
            updated.local_offset_ntuv = glm::vec3(p_local_offset_ntuv.x, p_local_offset_ntuv.y, p_local_offset_ntuv.z);
            const bool ok = core_plan_.update_feature(f.id, updated);
            if (ok) emit_changed();
            return ok;
        }
    }
    return false;
}

bool BodyPlanResource::remove_feature(int p_id) {
    if (p_id < 0) return false;
    const bool ok = core_plan_.remove_feature(static_cast<uint32_t>(p_id));
    if (ok) emit_changed();
    return ok;
}

void BodyPlanResource::setup_loomis_humanoid_plan() {
    preset_ = BODY_PLAN_HUMANOID;
    core_plan_ = core::BodyPlanCore::create_loomis_humanoid_plan();
    godot::Ref<FormScaffoldResource> sc;
    sc.instantiate();
    sc->setup_loomis_biped(LOOMIS_BIPED_NORMAL_7_5H, 0.23f);
    set_base_scaffold(sc);
}

void BodyPlanResource::setup_loomis_quadruped_plan() {
    preset_ = BODY_PLAN_QUADRUPED;
    core_plan_ = core::BodyPlanCore::create_loomis_quadruped_plan();
    godot::Ref<FormScaffoldResource> sc;
    sc.instantiate();
    sc->setup_loomis_quadruped(0.80f);
    set_base_scaffold(sc);
}

void BodyPlanResource::setup_loomis_avian_plan() {
    preset_ = BODY_PLAN_AVIAN;
    core_plan_ = core::BodyPlanCore::create_loomis_avian_plan();
    godot::Ref<FormScaffoldResource> sc;
    sc.instantiate();
    sc->setup_loomis_avian(0.22f);
    set_base_scaffold(sc);
}

} // namespace ideam::godot_ext
