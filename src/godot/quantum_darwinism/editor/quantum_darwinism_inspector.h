#pragma once

/**
 * quantum_darwinism_inspector.h
 * EditorInspectorPlugin for all `IdeamQuantumDarwinism` resources and nodes
 * (`src/godot/quantum_darwinism/editor/`), inheriting from `ProceduralGeneratorInspectorBase`.
 * Auto-injects `IdeamUndoRedo` and provides a direct launcher into `ProceduralComposer`.
 */

#include "../../procedural_generation/editor/procedural_generator_inspector_base.h"
#include "../quantum_darwinism_channel_spec.h"
#include "../quantum_darwinism_generator_resource.h"
#include "../quantum_darwinism_stabilizer_resource.h"

namespace ideam::godot_ext {

class QuantumDarwinismInspector : public ProceduralGeneratorInspectorBase {
    GDCLASS(QuantumDarwinismInspector, ProceduralGeneratorInspectorBase)

protected:
    static void _bind_methods();

public:
    QuantumDarwinismInspector() = default;
    ~QuantumDarwinismInspector() override = default;

    bool _can_handle(godot::Object *p_object) override;
    void _parse_begin(godot::Object *p_object) override;
};

} // namespace ideam::godot_ext
