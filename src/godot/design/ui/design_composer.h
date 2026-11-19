#pragma once

/**
 * design_composer.h
 * Tier 2 Orchestration Workspace (`IdeamToolWorkspace`) for the IdeamDesign domain
 * (`src/godot/design/ui/`).
 *
 * Sidebar Tabs:
 *   - `Blueprints`: `BlueprintResource` (hosts `BlueprintEditorControl`)
 *   - `Recipes`: `ParametricAssemblyRecipeResource`
 *   - `Themes & Styles`: `Theme3DResource`, `Style3DResource`
 */

#include "../../controls/ideam_tool_workspace.h"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/window.hpp>

namespace ideam::godot_ext {

class DesignComposer : public IdeamToolWorkspace {
    GDCLASS(DesignComposer, IdeamToolWorkspace)

private:
    static DesignComposer *shared_instance_;
    static godot::Window *shared_window_;

protected:
    static void _bind_methods();

public:
    static constexpr const char *WORKSPACE_ID = "DesignComposer";

    DesignComposer();
    ~DesignComposer() override;

    static DesignComposer *get_or_create_shared_composer(godot::Control *p_owner = nullptr);
    static void open_design_resource(
        const godot::Ref<godot::Resource> &p_resource,
        godot::Control *p_owner = nullptr
    );
};

} // namespace ideam::godot_ext
