#include "design_composer.h"

#include "../ideam_design_plugin.h"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

namespace ideam::godot_ext {

DesignComposer *DesignComposer::shared_instance_ = nullptr;
Window *DesignComposer::shared_window_           = nullptr;

void DesignComposer::_bind_methods() {
    ClassDB::bind_static_method(
        "DesignComposer",
        D_METHOD("open_design_resource", "resource", "owner"),
        &DesignComposer::open_design_resource,
        DEFVAL(nullptr)
    );
}

DesignComposer::DesignComposer() {
    set_workspace_id(StringName(WORKSPACE_ID));

    ensure_sidebar_tab("Blueprints");
    ensure_sidebar_tab("Recipes");
    ensure_sidebar_tab("Themes & Styles");

    configure_theme_registry("res://addons/ideam_design/resources/design_composer_theme_registry.tres");

#ifdef TOOLS_ENABLED
    if (tool_context_.is_valid()) {
        if (EditorUndoRedoManager *ed_ur = IdeamDesignPlugin::get_static_undo_redo()) {
            tool_context_->set_editor_undo_redo(ed_ur);
        }
    }
#endif
}

DesignComposer::~DesignComposer() {
    if (shared_instance_ == this) {
        shared_instance_ = nullptr;
        shared_window_   = nullptr;
    }
}

DesignComposer *DesignComposer::get_or_create_shared_composer(Control *p_owner) {
    if (p_owner) {
        for (int i = 0; i < p_owner->get_child_count(); ++i) {
            if (auto *existing = Object::cast_to<DesignComposer>(p_owner->get_child(i))) {
                return existing;
            }
        }
        auto *embedded = memnew(DesignComposer);
        p_owner->add_child(embedded);
        return embedded;
    }

    if (shared_instance_ && shared_window_) {
        shared_window_->show();
        shared_window_->grab_focus();
        return shared_instance_;
    }

    shared_instance_ = memnew(DesignComposer);
    shared_window_   = IdeamToolWorkspace::create_workspace_window(
        shared_instance_,
        "Ideam DesignComposer",
        Vector2i(1100, 700)
    );
    return shared_instance_;
}

void DesignComposer::open_design_resource(const Ref<Resource> &p_resource, Control *p_owner) {
    if (p_resource.is_null()) {
        return;
    }
    if (auto *composer = get_or_create_shared_composer(p_owner)) {
        composer->open_asset(p_resource);
    }
}

} // namespace ideam::godot_ext
