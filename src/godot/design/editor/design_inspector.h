#pragma once

#include "../../editor/ideam_editor_inspector_plugin.h"
#include "../style_3d_resource.h"
#include "../parametric_assembly_resource.h"
#include "../blueprint_resource.h"
#include "../container_3d.h"

namespace ideam::godot_ext {

class DesignInspector : public IdeamEditorInspectorPlugin {
    GDCLASS(DesignInspector, IdeamEditorInspectorPlugin)

protected:
    static void _bind_methods();

public:
    DesignInspector() = default;
    ~DesignInspector() override = default;

    bool _can_handle(godot::Object *p_object) const override;
    void _parse_begin(godot::Object *p_object) override;
};

} // namespace ideam::godot_ext
