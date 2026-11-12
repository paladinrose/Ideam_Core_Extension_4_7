#include "procedural_generator_inspector_base.h"
#include "../ideam_procedural_generation_plugin.h"

namespace ideam::godot_ext {

void ProceduralGeneratorInspectorBase::_bind_methods() {}

#ifdef TOOLS_ENABLED
godot::EditorUndoRedoManager *ProceduralGeneratorInspectorBase::get_undo_redo() const {
    return IdeamProceduralGenerationPlugin::get_static_undo_redo();
}
#endif

bool ProceduralGeneratorInspectorBase::_can_handle(godot::Object *p_object) {
    return p_object != nullptr && godot::Object::cast_to<ProceduralGeneratorResource>(p_object) != nullptr;
}

} // namespace ideam::godot_ext
