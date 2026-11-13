#pragma once

#include "../../editor/ideam_editor_inspector_plugin.h"
#include "../lexicon_resource.h"

namespace ideam::godot_ext {

class LinguisticsInspector : public IdeamEditorInspectorPlugin {
    GDCLASS(LinguisticsInspector, IdeamEditorInspectorPlugin)

protected:
    static void _bind_methods();

public:
    LinguisticsInspector() = default;
    ~LinguisticsInspector() override = default;

    bool _can_handle(godot::Object *p_object) const override;
};

} // namespace ideam::godot_ext
