#include "ideam_gizmo.h"

#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

void IdeamGizmo::_bind_methods() {
    using namespace godot;

    ADD_SIGNAL(MethodInfo("redraw_requested"));

    ClassDB::bind_method(D_METHOD("set_tool_context", "context"), &IdeamGizmo::set_tool_context);
    ClassDB::bind_method(D_METHOD("get_tool_context"), &IdeamGizmo::get_tool_context);

    ClassDB::bind_method(D_METHOD("set_active_tool_mode", "mode"), &IdeamGizmo::set_active_tool_mode);
    ClassDB::bind_method(D_METHOD("get_active_tool_mode"), &IdeamGizmo::get_active_tool_mode);

    ClassDB::bind_method(D_METHOD("set_enabled", "enabled"), &IdeamGizmo::set_enabled);
    ClassDB::bind_method(D_METHOD("is_enabled"), &IdeamGizmo::is_enabled);

    ClassDB::bind_method(D_METHOD("get_gizmo_name"), &IdeamGizmo::get_gizmo_name);
    ClassDB::bind_method(D_METHOD("get_priority"), &IdeamGizmo::get_priority);
    ClassDB::bind_method(D_METHOD("request_redraw"), &IdeamGizmo::request_redraw);
}

godot::Ref<IdeamUndoRedo> IdeamGizmo::resolve_undo_redo() const {
    if (tool_context_.is_valid() && tool_context_->get_undo_redo().is_valid()) {
        return tool_context_->get_undo_redo();
    }
    return godot::Ref<IdeamUndoRedo>();
}

void IdeamGizmo::set_tool_context(const godot::Ref<ToolContext> &p_context) {
    tool_context_ = p_context;
    request_redraw();
}

void IdeamGizmo::set_active_tool_mode(const godot::StringName &p_mode) {
    if (active_tool_mode_ == p_mode) {
        return;
    }
    active_tool_mode_ = p_mode;
    request_redraw();
}

void IdeamGizmo::set_enabled(bool p_enabled) {
    if (enabled_ == p_enabled) {
        return;
    }
    enabled_ = p_enabled;
    request_redraw();
}

void IdeamGizmo::request_redraw() {
    emit_signal("redraw_requested");
}

} // namespace ideam::godot_ext
