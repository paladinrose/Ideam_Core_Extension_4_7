#include "procedural_generator_resource.h"

namespace ideam::godot_ext {

void ProceduralGeneratorResource::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_master_seed", "seed"), &ProceduralGeneratorResource::set_master_seed);
    ClassDB::bind_method(D_METHOD("get_master_seed"), &ProceduralGeneratorResource::get_master_seed);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "master_seed"), "set_master_seed", "get_master_seed");

    ClassDB::bind_method(D_METHOD("set_deterministic_mode", "deterministic"), &ProceduralGeneratorResource::set_deterministic_mode);
    ClassDB::bind_method(D_METHOD("get_deterministic_mode"), &ProceduralGeneratorResource::get_deterministic_mode);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "deterministic_mode"), "set_deterministic_mode", "get_deterministic_mode");

    ClassDB::bind_method(D_METHOD("get_snapshot_state"), &ProceduralGeneratorResource::get_snapshot_state);
    ClassDB::bind_method(D_METHOD("apply_snapshot_state", "state"), &ProceduralGeneratorResource::apply_snapshot_state);
}

void ProceduralGeneratorResource::set_master_seed(int64_t p_seed) {
    if (master_seed_ == p_seed) {
        return;
    }
    master_seed_ = p_seed;
    emit_changed();
}

void ProceduralGeneratorResource::set_deterministic_mode(bool p_deterministic) {
    if (deterministic_mode_ == p_deterministic) {
        return;
    }
    deterministic_mode_ = p_deterministic;
    emit_changed();
}

godot::Dictionary ProceduralGeneratorResource::get_snapshot_state() const {
    godot::Dictionary d;
    d["master_seed"]        = master_seed_;
    d["deterministic_mode"] = deterministic_mode_;
    return d;
}

void ProceduralGeneratorResource::apply_snapshot_state(const godot::Dictionary &p_state) {
    if (p_state.has("master_seed")) {
        master_seed_ = static_cast<int64_t>(p_state["master_seed"]);
    }
    if (p_state.has("deterministic_mode")) {
        deterministic_mode_ = static_cast<bool>(p_state["deterministic_mode"]);
    }
    emit_changed();
}

void ProceduralGeneratorResource::commit_snapshot_mutation(
    const godot::String &p_action_name,
    const godot::Dictionary &p_before_state
) {
    if (undo_redo_.is_null()) {
        return;
    }
    const godot::Dictionary after_state = get_snapshot_state();
    undo_redo_->create_action(p_action_name);
    undo_redo_->add_do_method(this, "apply_snapshot_state", after_state);
    undo_redo_->add_undo_method(this, "apply_snapshot_state", p_before_state);
    undo_redo_->commit_action(false);
}

void ProceduralGenerationHost::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_generator_resource", "resource"), &ProceduralGenerationHost::set_generator_resource);
    ClassDB::bind_method(D_METHOD("get_generator_resource"), &ProceduralGenerationHost::get_generator_resource);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "generator_resource", PROPERTY_HINT_RESOURCE_TYPE, "ProceduralGeneratorResource"),
        "set_generator_resource",
        "get_generator_resource"
    );

    ClassDB::bind_method(D_METHOD("set_auto_generate_on_ready", "auto_generate"), &ProceduralGenerationHost::set_auto_generate_on_ready);
    ClassDB::bind_method(D_METHOD("get_auto_generate_on_ready"), &ProceduralGenerationHost::get_auto_generate_on_ready);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_generate_on_ready"), "set_auto_generate_on_ready", "get_auto_generate_on_ready");
}

void ProceduralGenerationHost::set_generator_resource(const godot::Ref<ProceduralGeneratorResource> &p_res) {
    generator_resource_ = p_res;
}

void ProceduralGenerationHost::set_auto_generate_on_ready(bool p_auto) {
    auto_generate_on_ready_ = p_auto;
}

} // namespace ideam::godot_ext
