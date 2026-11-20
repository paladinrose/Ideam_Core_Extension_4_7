#include "procedural_composer.h"

#include "../ideam_procedural_generation_plugin.h"

#include <godot_cpp/classes/v_separator.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

using namespace godot;

namespace ideam::godot_ext {

ProceduralComposer *ProceduralComposer::shared_instance_ = nullptr;
Window *ProceduralComposer::shared_window_               = nullptr;

void ProceduralComposer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_execute_pressed"), &ProceduralComposer::_on_execute_pressed);
    ClassDB::bind_static_method(
        "ProceduralComposer",
        D_METHOD("open_procedural_resource", "resource", "owner"),
        &ProceduralComposer::open_procedural_resource,
        DEFVAL(nullptr)
    );
}

ProceduralComposer::ProceduralComposer() {
    set_workspace_id(StringName(WORKSPACE_ID));

    ensure_sidebar_tab("Generators");
    ensure_sidebar_tab("WFC");
    ensure_sidebar_tab("Quantum Darwinism");

    configure_theme_registry("res://addons/ideam_procedural_generation/resources/procedural_composer_theme_registry.tres");

    if (tool_context_.is_valid()) {
        tool_context_->set_update_commit_policy(UPDATE_EXPLICIT_TRIGGER);
#ifdef TOOLS_ENABLED
        if (EditorUndoRedoManager *ed_ur = IdeamProceduralGenerationPlugin::get_static_undo_redo()) {
            tool_context_->set_editor_undo_redo(ed_ur);
        }
#endif
    }

    _populate_command_bar(custom_actions_box_);
}

ProceduralComposer::~ProceduralComposer() {
    if (shared_instance_ == this) {
        shared_instance_ = nullptr;
        shared_window_   = nullptr;
    }
}

void ProceduralComposer::_populate_command_bar(HBoxContainer *p_custom_actions) {
    if (!p_custom_actions) {
        return;
    }
    p_custom_actions->add_child(memnew(VSeparator));

    execute_btn_ = memnew(Button);
    execute_btn_->set_text("Generate / Collapse / Stabilize");
    execute_btn_->set_tooltip_text("Explicitly trigger the active procedural, WFC, or Quantum Darwinism solver.");
    execute_btn_->connect("pressed", Callable(this, "_on_execute_pressed"));
    p_custom_actions->add_child(execute_btn_);
}

void ProceduralComposer::_on_execute_pressed() {
    _on_dirty_banner_action_pressed();
}

ProceduralComposer *ProceduralComposer::get_or_create_shared_composer(Control *p_owner) {
    if (p_owner) {
        for (int i = 0; i < p_owner->get_child_count(); ++i) {
            if (auto *existing = Object::cast_to<ProceduralComposer>(p_owner->get_child(i))) {
                return existing;
            }
        }
        auto *embedded = memnew(ProceduralComposer);
        p_owner->add_child(embedded);
        return embedded;
    }

    if (shared_instance_ && shared_window_) {
        shared_window_->show();
        shared_window_->grab_focus();
        return shared_instance_;
    }

    shared_instance_ = memnew(ProceduralComposer);
    shared_window_   = ToolWorkspace::create_workspace_window(
        shared_instance_,
        "Ideam ProceduralComposer",
        Vector2i(1100, 700)
    );
    return shared_instance_;
}

void ProceduralComposer::open_procedural_resource(const Ref<Resource> &p_resource, Control *p_owner) {
    if (p_resource.is_null()) {
        return;
    }
    if (auto *composer = get_or_create_shared_composer(p_owner)) {
        composer->open_asset(p_resource);
    }
}

} // namespace ideam::godot_ext
