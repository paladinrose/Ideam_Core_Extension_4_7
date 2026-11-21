#include "quantum_darwinism_inspector.h"
#include "../ideam_quantum_darwinism_plugin.h"
#include "../../procedural_generation/ui/procedural_composer.h"

namespace ideam::godot_ext {

void QuantumDarwinismInspector::_bind_methods() {}

bool QuantumDarwinismInspector::_can_handle(godot::Object *p_object) {
    if (p_object == nullptr) {
        return false;
    }
    return godot::Object::cast_to<QuantumDarwinismStabilizerResource>(p_object) != nullptr ||
           godot::Object::cast_to<QuantumDarwinismStabilizerNode>(p_object) != nullptr ||
           godot::Object::cast_to<QuantumDarwinismGeneratorResource>(p_object) != nullptr ||
           godot::Object::cast_to<QuantumDarwinismGeneratorNode>(p_object) != nullptr ||
           godot::Object::cast_to<QuantumDarwinismChannelSpec>(p_object) != nullptr ||
           godot::Object::cast_to<QuantumDarwinismCouplingRule>(p_object) != nullptr;
}

void QuantumDarwinismInspector::_parse_begin(godot::Object *p_object) {
    ProceduralGeneratorInspectorBase::_parse_begin(p_object);
    if (!p_object) return;

    godot::Ref<IdeamUndoRedo> ur = IdeamQuantumDarwinismPlugin::create_undo_redo_wrapper();
    if (ur.is_valid()) {
        if (auto *gen = godot::Object::cast_to<QuantumDarwinismGeneratorResource>(p_object)) {
            gen->propagate_undo_redo(ur);
        } else if (auto *stab = godot::Object::cast_to<QuantumDarwinismStabilizerResource>(p_object)) {
            stab->set_undo_redo(ur);
        } else if (auto *ch = godot::Object::cast_to<QuantumDarwinismChannelSpec>(p_object)) {
            ch->set_undo_redo(ur);
        } else if (auto *cp = godot::Object::cast_to<QuantumDarwinismCouplingRule>(p_object)) {
            cp->set_undo_redo(ur);
        }
    }
}

} // namespace ideam::godot_ext
