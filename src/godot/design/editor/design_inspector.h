#pragma once

#include "../../editor/ideam_editor_inspector_plugin.h"
#include "../blueprint_resource.h"
#include "../container_3d.h"
#include "../form_scaffold_node_3d.h"
#include "../form_scaffold_resource.h"
#include "../parametric_assembly_resource.h"
#include "../proportion_ruler_resource.h"
#include "../style_3d_resource.h"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace ideam::godot_ext {

class DesignInspector : public IdeamEditorInspectorPlugin {
    GDCLASS(DesignInspector, IdeamEditorInspectorPlugin)

protected:
    static void _bind_methods();

public:
    DesignInspector() = default;
    ~DesignInspector() override = default;

    void _on_open_in_design_composer(const godot::Ref<godot::Resource> &p_resource);
    void _on_help_pressed(const godot::StringName &p_topic_id);

    bool _can_handle(godot::Object *p_object) const override;
    void _parse_begin(godot::Object *p_object) override;
    bool _parse_property(godot::Object *p_object, godot::Variant::Type p_type, const godot::String &p_name, godot::PropertyHint p_hint_type, const godot::String &p_hint_string, godot::BitField<godot::PropertyUsageFlags> p_usage_flags, bool p_wide) override { return false; }
};

} // namespace ideam::godot_ext
