#include "ideam_editor_inspector_plugin.h"
#include "../utilities/ideam_undo_redo.h"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

namespace ideam::godot_ext {

void IdeamEditorInspectorPlugin::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_tool_context", "context"), &IdeamEditorInspectorPlugin::set_tool_context);
	ClassDB::bind_method(D_METHOD("get_tool_context"), &IdeamEditorInspectorPlugin::get_tool_context);
	ClassDB::bind_method(D_METHOD("resolve_shared_undo_redo"), &IdeamEditorInspectorPlugin::resolve_shared_undo_redo);
#ifdef TOOLS_ENABLED
	ClassDB::bind_method(D_METHOD("get_undo_redo"), &IdeamEditorInspectorPlugin::get_undo_redo);
#endif
}

IdeamEditorInspectorPlugin::IdeamEditorInspectorPlugin() {
}

IdeamEditorInspectorPlugin::~IdeamEditorInspectorPlugin() {
}

void IdeamEditorInspectorPlugin::set_tool_context(const Ref<ToolContext> &p_context) {
	tool_context_ = p_context;
}

Ref<IdeamUndoRedo> IdeamEditorInspectorPlugin::resolve_shared_undo_redo() const {
	// 1. Prefer the shared ToolContext's IdeamUndoRedo so Inspector edits & 3D Gizmo drags share one stack.
	if (tool_context_.is_valid() && tool_context_->get_undo_redo().is_valid()) {
		Ref<IdeamUndoRedo> shared_ur = tool_context_->get_undo_redo();
#ifdef TOOLS_ENABLED
		if (EditorUndoRedoManager *editor_manager = get_undo_redo()) {
			if (!shared_ur->get_editor_undo_redo()) {
				shared_ur->set_editor_undo_redo(editor_manager);
			}
		}
#endif
		return shared_ur;
	}

	// 2. Fallback: allocate a wrapper backed by the plugin's EditorUndoRedoManager.
	Ref<IdeamUndoRedo> ideam_ur;
	ideam_ur.instantiate();
#ifdef TOOLS_ENABLED
	if (EditorUndoRedoManager *editor_manager = get_undo_redo()) {
		ideam_ur->set_editor_undo_redo(editor_manager);
	}
#endif
	return ideam_ur;
}

#ifdef TOOLS_ENABLED
EditorUndoRedoManager *IdeamEditorInspectorPlugin::get_undo_redo() const {
	return nullptr;
}
#endif

bool IdeamEditorInspectorPlugin::_can_handle(Object *p_object) {
	return true;
}

void IdeamEditorInspectorPlugin::_parse_begin(Object *p_object) {
	if (!p_object) {
		return;
	}

	// Inject the shared IdeamUndoRedo if the inspected object exposes an 'undo_redo' property or setter.
	if (p_object->has_method("set_undo_redo")) {
		p_object->call("set_undo_redo", resolve_shared_undo_redo());
		return;
	}

	Variant prop = p_object->get("undo_redo");
	if (prop.get_type() != Variant::NIL) {
		p_object->set("undo_redo", resolve_shared_undo_redo());
	}
}

} // namespace ideam::godot_ext