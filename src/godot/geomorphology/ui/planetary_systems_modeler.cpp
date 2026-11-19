#include "planetary_systems_modeler.h"

#include <godot_cpp/classes/v_separator.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

using namespace godot;

namespace ideam::godot_ext {

PlanetarySystemsModeler *PlanetarySystemsModeler::shared_instance_ = nullptr;
Window *PlanetarySystemsModeler::shared_window_                    = nullptr;

void PlanetarySystemsModeler::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_step_sim_pressed"), &PlanetarySystemsModeler::_on_step_sim_pressed);
    ClassDB::bind_static_method(
        "PlanetarySystemsModeler",
        D_METHOD("open_planetary_resource", "resource", "owner"),
        &PlanetarySystemsModeler::open_planetary_resource,
        DEFVAL(nullptr)
    );
}

PlanetarySystemsModeler::PlanetarySystemsModeler() {
    set_workspace_id(StringName(WORKSPACE_ID));

    ensure_sidebar_tab("Geomorphology");
    ensure_sidebar_tab("Climatology");
    ensure_sidebar_tab("Ecology");

    configure_theme_registry("res://addons/ideam_geomorphology/resources/planetary_modeler_theme_registry.tres");

    // Planetary simulations are heavy Tier 3 operations -> default to Explicit Trigger
    if (tool_context_.is_valid()) {
        tool_context_->set_update_commit_policy(UPDATE_EXPLICIT_TRIGGER);
    }

    _populate_command_bar(custom_actions_box_);
}

PlanetarySystemsModeler::~PlanetarySystemsModeler() {
    if (shared_instance_ == this) {
        shared_instance_ = nullptr;
        shared_window_   = nullptr;
    }
}

void PlanetarySystemsModeler::_populate_command_bar(HBoxContainer *p_custom_actions) {
    if (!p_custom_actions) {
        return;
    }
    p_custom_actions->add_child(memnew(VSeparator));

    step_sim_btn_ = memnew(Button);
    step_sim_btn_->set_text("Simulate / Step Active");
    step_sim_btn_->set_tooltip_text("Explicitly trigger a simulation step on the active planetary resource (Terrain -> Atmosphere -> Biosphere).");
    step_sim_btn_->connect("pressed", Callable(this, "_on_step_sim_pressed"));
    p_custom_actions->add_child(step_sim_btn_);
}

void PlanetarySystemsModeler::_on_step_sim_pressed() {
    if (active_asset_.is_valid() && active_asset_->has_method("step_simulation")) {
        active_asset_->call("step_simulation", 1);
    }
    _on_dirty_banner_action_pressed();
}

void PlanetarySystemsModeler::_on_dirty_banner_action_pressed() {
    IdeamToolWorkspace::_on_dirty_banner_action_pressed();
}

PlanetarySystemsModeler *PlanetarySystemsModeler::get_or_create_shared_modeler(Control *p_owner) {
    if (p_owner) {
        for (int i = 0; i < p_owner->get_child_count(); ++i) {
            if (auto *existing = Object::cast_to<PlanetarySystemsModeler>(p_owner->get_child(i))) {
                return existing;
            }
        }
        auto *embedded = memnew(PlanetarySystemsModeler);
        p_owner->add_child(embedded);
        return embedded;
    }

    if (shared_instance_ && shared_window_) {
        shared_window_->show();
        shared_window_->grab_focus();
        return shared_instance_;
    }

    shared_instance_ = memnew(PlanetarySystemsModeler);
    shared_window_   = IdeamToolWorkspace::create_workspace_window(
        shared_instance_,
        "Ideam PlanetarySystemsModeler",
        Vector2i(1180, 740)
    );
    return shared_instance_;
}

void PlanetarySystemsModeler::open_planetary_resource(const Ref<Resource> &p_resource, Control *p_owner) {
    if (p_resource.is_null()) {
        return;
    }
    if (auto *modeler = get_or_create_shared_modeler(p_owner)) {
        modeler->open_asset(p_resource);
    }
}

} // namespace ideam::godot_ext
