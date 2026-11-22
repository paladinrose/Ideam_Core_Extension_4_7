#pragma once

/**
 * surfaces_inspector.h
 * EditorInspectorPlugin for `IdeamSurfaces` (`src/godot/surfaces/editor/`)
 * mirroring `NarrativesInspector`:
 *   - Injects `IdeamUndoRedo` into inspected `SurfacePatchResource` & `SurfaceNetworkResource`.
 *   - Displays live LOD 0..3 quad/vertex budget & manifold telemetry header.
 *   - Provides "Open in Composer" and `?` `IdeamTutorials` help buttons stamped with
 *     `ideam_ui_selector` metadata.
 */

#include "../../editor/ideam_editor_inspector_plugin.h"
#include "../coons_patch_3d_resource.h"
#include "../nurbs_patch_3d_resource.h"
#include "../surface_mesh_node_3d.h"
#include "../surface_network_resource.h"
#include "../surface_patch_resource.h"

#include <godot_cpp/core/object_id.hpp>

namespace ideam::godot_ext {

class SurfacesInspector : public IdeamEditorInspectorPlugin {
    GDCLASS(SurfacesInspector, IdeamEditorInspectorPlugin)

private:
    godot::ObjectID current_object_id_;
    godot::ObjectID summary_label_id_;

    void _refresh_summary_panel();

protected:
    static void _bind_methods();

public:
    SurfacesInspector() = default;
    ~SurfacesInspector() override = default;

#ifdef TOOLS_ENABLED
    godot::EditorUndoRedoManager *get_undo_redo() const override;
#endif

    bool _can_handle(godot::Object *p_object) override;
    void _parse_begin(godot::Object *p_object) override;

    void _on_open_in_composer_pressed();
    void _on_help_pressed();
};

} // namespace ideam::godot_ext
