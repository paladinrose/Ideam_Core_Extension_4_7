#include "quantum_darwinism_inspector.h"

namespace ideam::godot_ext {

void QuantumDarwinismInspector::_bind_methods() {}

bool QuantumDarwinismInspector::_can_handle(godot::Object *p_object) {
    return p_object != nullptr && godot::Object::cast_to<QuantumDarwinismStabilizerResource>(p_object) != nullptr;
}

} // namespace ideam::godot_ext
