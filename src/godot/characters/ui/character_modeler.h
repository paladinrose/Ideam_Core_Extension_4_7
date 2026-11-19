#pragma once

/**
 * character_modeler.h
 * Tier 2 Orchestration Workspace (`IdeamToolWorkspace`) for the IdeamCharacters domain
 * (`src/godot/characters/ui/`).
 *
 * Sidebar Tabs:
 *   - `Blueprints`: `CharacterBlueprintResource` (hosts `CharacterMorphSliderPanel`)
 *   - `Apparel`: `CharacterApparelResource`
 */

#include "../../controls/ideam_tool_workspace.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/window.hpp>

namespace ideam::godot_ext {

class CharacterModeler : public IdeamToolWorkspace {
    GDCLASS(CharacterModeler, IdeamToolWorkspace)

private:
    static CharacterModeler *shared_instance_;
    static godot::Window *shared_window_;

protected:
    static void _bind_methods();

public:
    static constexpr const char *WORKSPACE_ID = "CharacterModeler";

    CharacterModeler();
    ~CharacterModeler() override;

    static CharacterModeler *get_or_create_shared_modeler(godot::Control *p_owner = nullptr);
    static void open_character_resource(
        const godot::Ref<godot::Resource> &p_resource,
        godot::Control *p_owner = nullptr
    );
};

} // namespace ideam::godot_ext
