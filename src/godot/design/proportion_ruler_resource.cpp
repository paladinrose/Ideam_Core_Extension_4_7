#include "proportion_ruler_resource.h"

#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

void ProportionRulerGraphResource::_bind_methods() {
    using namespace godot;

    BIND_ENUM_CONSTANT(RULER_UNIT_HEAD_HEIGHT);
    BIND_ENUM_CONSTANT(RULER_UNIT_SHOULDER_HEIGHT);
    BIND_ENUM_CONSTANT(RULER_UNIT_KEEL_LENGTH);
    BIND_ENUM_CONSTANT(RULER_UNIT_SKULL_LENGTH);
    BIND_ENUM_CONSTANT(RULER_UNIT_SEAT_HEIGHT);
    BIND_ENUM_CONSTANT(RULER_UNIT_CUSTOM);

    BIND_ENUM_CONSTANT(LOOMIS_BIPED_CHILD_5H);
    BIND_ENUM_CONSTANT(LOOMIS_BIPED_NORMAL_7_5H);
    BIND_ENUM_CONSTANT(LOOMIS_BIPED_IDEALISTIC_8H);
    BIND_ENUM_CONSTANT(LOOMIS_BIPED_HEROIC_8_5H);
    BIND_ENUM_CONSTANT(LOOMIS_BIPED_STYLIZED_4_5H);

    ADD_GROUP("Proportion Canon", "");

    ClassDB::bind_method(D_METHOD("set_canon_name", "name"), &ProportionRulerGraphResource::set_canon_name);
    ClassDB::bind_method(D_METHOD("get_canon_name"), &ProportionRulerGraphResource::get_canon_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "canon_name"), "set_canon_name", "get_canon_name");

    ClassDB::bind_method(D_METHOD("set_unit_type", "unit_type"), &ProportionRulerGraphResource::set_unit_type);
    ClassDB::bind_method(D_METHOD("get_unit_type"), &ProportionRulerGraphResource::get_unit_type);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "unit_type", PROPERTY_HINT_ENUM, "Head Height:0,Shoulder Height:1,Keel Length:2,Skull Length:3,Seat Height:4,Custom:5"),
        "set_unit_type",
        "get_unit_type"
    );

    ClassDB::bind_method(D_METHOD("set_base_unit_meters", "meters"), &ProportionRulerGraphResource::set_base_unit_meters);
    ClassDB::bind_method(D_METHOD("get_base_unit_meters"), &ProportionRulerGraphResource::get_base_unit_meters);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "base_unit_meters", PROPERTY_HINT_RANGE, "0.02,5.0,0.01"), "set_base_unit_meters", "get_base_unit_meters");

    ADD_GROUP("Tools & Tutorials", "");

    ClassDB::bind_method(D_METHOD("set_help_topic_id", "topic_id"), &ProportionRulerGraphResource::set_help_topic_id);
    ClassDB::bind_method(D_METHOD("get_help_topic_id"), &ProportionRulerGraphResource::get_help_topic_id);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "help_topic_id"), "set_help_topic_id", "get_help_topic_id");

    ClassDB::bind_method(D_METHOD("set_semantic_tags", "tags"), &ProportionRulerGraphResource::set_semantic_tags);
    ClassDB::bind_method(D_METHOD("get_semantic_tags"), &ProportionRulerGraphResource::get_semantic_tags);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "semantic_tags"), "set_semantic_tags", "get_semantic_tags");

    ClassDB::bind_method(D_METHOD("set_undo_redo", "undo_redo"), &ProportionRulerGraphResource::set_undo_redo);
    ClassDB::bind_method(D_METHOD("get_undo_redo"), &ProportionRulerGraphResource::get_undo_redo);

    ClassDB::bind_method(D_METHOD("add_ruler_node", "name", "parent_id", "ratio", "bias", "is_landmark"), &ProportionRulerGraphResource::add_ruler_node, DEFVAL(0.0), DEFVAL(false));
    ClassDB::bind_method(D_METHOD("set_node_ratio", "node_id", "ratio"), &ProportionRulerGraphResource::set_node_ratio);
    ClassDB::bind_method(D_METHOD("evaluate"), &ProportionRulerGraphResource::evaluate);
    ClassDB::bind_method(D_METHOD("get_value_by_name", "name", "fallback"), &ProportionRulerGraphResource::get_value_by_name, DEFVAL(0.0));
    ClassDB::bind_method(D_METHOD("get_landmarks"), &ProportionRulerGraphResource::get_landmarks);
    ClassDB::bind_method(D_METHOD("get_all_nodes"), &ProportionRulerGraphResource::get_all_nodes);

    ClassDB::bind_method(D_METHOD("setup_loomis_biped", "preset", "head_height_m"), &ProportionRulerGraphResource::setup_loomis_biped, DEFVAL(LOOMIS_BIPED_NORMAL_7_5H), DEFVAL(0.23));
    ClassDB::bind_method(D_METHOD("setup_loomis_quadruped", "shoulder_height_m"), &ProportionRulerGraphResource::setup_loomis_quadruped, DEFVAL(0.80));
    ClassDB::bind_method(D_METHOD("setup_loomis_avian", "keel_length_m"), &ProportionRulerGraphResource::setup_loomis_avian, DEFVAL(0.22));
    ClassDB::bind_method(D_METHOD("setup_loomis_serpentine", "skull_length_m"), &ProportionRulerGraphResource::setup_loomis_serpentine, DEFVAL(0.14));
    ClassDB::bind_method(D_METHOD("setup_prop_chair", "seat_height_m"), &ProportionRulerGraphResource::setup_prop_chair, DEFVAL(0.45));
}

ProportionRulerGraphResource::ProportionRulerGraphResource() {
    core_graph_ = core::ProportionRulerGraph::create_loomis_biped_canon(core::LoomisBipedCanon::NORMAL_7_5H, 0.23f);
}

void ProportionRulerGraphResource::set_core_graph(const core::ProportionRulerGraph &p_graph) {
    core_graph_ = p_graph;
    core_graph_.evaluate();
    emit_changed();
}

void ProportionRulerGraphResource::set_canon_name(const godot::String &p_name) {
    core_graph_.set_canon_name(p_name.utf8().get_data());
    emit_changed();
}

godot::String ProportionRulerGraphResource::get_canon_name() const {
    return godot::String::utf8(core_graph_.get_canon_name().data(), static_cast<int64_t>(core_graph_.get_canon_name().size()));
}

void ProportionRulerGraphResource::set_unit_type(ProportionRulerUnitType p_type) {
    core_graph_.set_unit_type(static_cast<core::ProportionUnitType>(p_type));
    emit_changed();
}

ProportionRulerUnitType ProportionRulerGraphResource::get_unit_type() const noexcept {
    return static_cast<ProportionRulerUnitType>(core_graph_.get_unit_type());
}

void ProportionRulerGraphResource::set_base_unit_meters(real_t p_meters) {
    core_graph_.set_base_unit_meters(static_cast<float>(p_meters));
    core_graph_.evaluate();
    emit_changed();
}

real_t ProportionRulerGraphResource::get_base_unit_meters() const noexcept {
    return static_cast<real_t>(core_graph_.get_base_unit_meters());
}

godot::StringName ProportionRulerGraphResource::get_help_topic_id() const {
    if (help_topic_id_ != godot::StringName()) return help_topic_id_;
    return godot::StringName("ProportionRulerGraphResource");
}

void ProportionRulerGraphResource::set_semantic_tags(const godot::PackedStringArray &p_tags) {
    semantic_tags_ = p_tags;
    emit_changed();
}

int ProportionRulerGraphResource::add_ruler_node(
    const godot::String &p_name,
    int p_parent_id,
    real_t p_ratio,
    real_t p_bias,
    bool p_is_landmark
) {
    const uint32_t pid = (p_parent_id < 0) ? 0xFFFFFFFFu : static_cast<uint32_t>(p_parent_id);
    const uint32_t id = core_graph_.add_node(
        p_name.utf8().get_data(),
        pid,
        static_cast<float>(p_ratio),
        static_cast<float>(p_bias),
        p_is_landmark
    );
    core_graph_.evaluate();
    emit_changed();
    return static_cast<int>(id);
}

bool ProportionRulerGraphResource::set_node_ratio(int p_node_id, real_t p_ratio) {
    if (p_node_id < 0) return false;
    const bool ok = core_graph_.set_node_ratio(static_cast<uint32_t>(p_node_id), static_cast<float>(p_ratio));
    if (ok) {
        core_graph_.evaluate();
        emit_changed();
    }
    return ok;
}

bool ProportionRulerGraphResource::evaluate() {
    const bool ok = core_graph_.evaluate();
    emit_changed();
    return ok;
}

real_t ProportionRulerGraphResource::get_value_by_name(const godot::String &p_name, real_t p_fallback) const {
    return static_cast<real_t>(core_graph_.get_value_by_name(p_name.utf8().get_data(), static_cast<float>(p_fallback)));
}

godot::TypedArray<godot::Dictionary> ProportionRulerGraphResource::get_landmarks() const {
    godot::TypedArray<godot::Dictionary> out;
    for (const auto &n : core_graph_.nodes()) {
        if (!n.is_landmark) continue;
        godot::Dictionary d;
        d["id"]             = static_cast<int64_t>(n.id);
        d["name"]           = godot::String::utf8(n.name.c_str());
        d["ratio"]          = static_cast<double>(n.ratio);
        d["computed_value"] = static_cast<double>(n.computed_value);
        out.append(d);
    }
    return out;
}

godot::TypedArray<godot::Dictionary> ProportionRulerGraphResource::get_all_nodes() const {
    godot::TypedArray<godot::Dictionary> out;
    for (const auto &n : core_graph_.nodes()) {
        godot::Dictionary d;
        d["id"]             = static_cast<int64_t>(n.id);
        d["parent_id"]      = (n.parent_id == 0xFFFFFFFFu) ? -1 : static_cast<int64_t>(n.parent_id);
        d["name"]           = godot::String::utf8(n.name.c_str());
        d["ratio"]          = static_cast<double>(n.ratio);
        d["bias"]           = static_cast<double>(n.bias);
        d["computed_value"] = static_cast<double>(n.computed_value);
        d["is_landmark"]    = n.is_landmark;
        out.append(d);
    }
    return out;
}

void ProportionRulerGraphResource::setup_loomis_biped(LoomisBipedCanonPreset p_preset, real_t p_head_height_m) {
    core_graph_ = core::ProportionRulerGraph::create_loomis_biped_canon(
        static_cast<core::LoomisBipedCanon>(p_preset),
        static_cast<float>(p_head_height_m)
    );
    emit_changed();
}

void ProportionRulerGraphResource::setup_loomis_quadruped(real_t p_shoulder_height_m) {
    core_graph_ = core::ProportionRulerGraph::create_loomis_quadruped_canon(static_cast<float>(p_shoulder_height_m));
    emit_changed();
}

void ProportionRulerGraphResource::setup_loomis_avian(real_t p_keel_length_m) {
    core_graph_ = core::ProportionRulerGraph::create_loomis_avian_canon(static_cast<float>(p_keel_length_m));
    emit_changed();
}

void ProportionRulerGraphResource::setup_loomis_serpentine(real_t p_skull_length_m) {
    core_graph_ = core::ProportionRulerGraph::create_loomis_serpentine_canon(static_cast<float>(p_skull_length_m));
    emit_changed();
}

void ProportionRulerGraphResource::setup_prop_chair(real_t p_seat_height_m) {
    core_graph_ = core::ProportionRulerGraph::create_prop_chair_canon(static_cast<float>(p_seat_height_m));
    emit_changed();
}

} // namespace ideam::godot_ext
