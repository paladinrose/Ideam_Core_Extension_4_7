#pragma once

#include "../tools/tool_context.h"

#include <godot_cpp/classes/editor_inspector_plugin.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#ifdef TOOLS_ENABLED
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#endif

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace ideam::godot_ext {

class IdeamEditorInspectorPlugin : public godot::EditorInspectorPlugin {
	GDCLASS(IdeamEditorInspectorPlugin, godot::EditorInspectorPlugin)

private:
	godot::Ref<ToolContext> tool_context_;

	static void _bind_methods();

public:
	IdeamEditorInspectorPlugin();
	~IdeamEditorInspectorPlugin();

	void set_tool_context(const godot::Ref<ToolContext> &p_context);
	[[nodiscard]] godot::Ref<ToolContext> get_tool_context() const { return tool_context_; }

	/**
	 * Resolves the shared `IdeamUndoRedo` instance from `tool_context_` when bound,
	 * otherwise creates a wrapper backed by `get_undo_redo()`.
	 */
	[[nodiscard]] godot::Ref<IdeamUndoRedo> resolve_shared_undo_redo() const;

	virtual bool _can_handle(godot::Object *p_object);
	virtual void _parse_begin(godot::Object *p_object) override;
	virtual bool _parse_property(godot::Object *p_object, godot::Variant::Type p_type, const godot::String &p_name, godot::PropertyHint p_hint_type, const godot::String &p_hint_string, godot::BitField<godot::PropertyUsageFlags> p_usage_flags, bool p_wide) override = 0;

#ifdef TOOLS_ENABLED
	virtual godot::EditorUndoRedoManager *get_undo_redo() const;
#endif
};

} // namespace ideam::godot_ext