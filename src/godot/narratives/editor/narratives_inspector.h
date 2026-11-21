#pragma once

/**
 * narratives_inspector.h
 * EditorInspectorPlugin for `IdeamNarratives` (`src/godot/narratives/editor/`).
 */

#include "../../editor/ideam_editor_inspector_plugin.h"
#include "../narreme.h"
#include "../narremes/character.h"
#include "../narremes/incident.h"
#include "../narremes/location.h"
#include "../narremes/narrative.h"
#include "../narremes/plot.h"
#include "../narremes/prop.h"
#include "../helpers/incident_condition.h"
#include "../helpers/narrative_condition.h"
#include "../helpers/plot_event.h"
#include "../helpers/relationship.h"

#include <godot_cpp/core/object_id.hpp>

namespace ideam::godot_ext {

class NarrativesInspector : public IdeamEditorInspectorPlugin {
    GDCLASS(NarrativesInspector, IdeamEditorInspectorPlugin)

private:
    godot::ObjectID current_object_id_;
    godot::ObjectID summary_label_id_;

    void _refresh_summary_panel();

protected:
    static void _bind_methods();

public:
    NarrativesInspector() = default;
    ~NarrativesInspector() override = default;

#ifdef TOOLS_ENABLED
    virtual godot::EditorUndoRedoManager *get_undo_redo() const override;
#endif

    virtual bool _can_handle(godot::Object *p_object) override;
    virtual void _parse_begin(godot::Object *p_object) override;
    virtual bool _parse_property(
        godot::Object *p_object,
        godot::Variant::Type p_type,
        const godot::String &p_name,
        godot::PropertyHint p_hint_type,
        const godot::String &p_hint_string,
        godot::BitField<godot::PropertyUsageFlags> p_usage_flags,
        bool p_wide
    ) override;

    void _on_open_in_composer_pressed();
    void _on_help_pressed();
};

} // namespace ideam::godot_ext
