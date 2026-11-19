#pragma once

/**
 * procedural_composer.h
 * Tier 2 Orchestration Workspace (`IdeamToolWorkspace`) unifying Procedural Generation,
 * Wave Function Collapse, and Quantum Darwinism (`src/godot/procedural_generation/ui/`).
 *
 * Sidebar Tabs:
 *   - `Generators`: `ProceduralGeneratorResource`, `ProceduralPattern`, `GenerationRule`
 *   - `WFC`: `WFCRuleTensorResource`
 *   - `Quantum Darwinism`: `QuantumDarwinismStabilizerResource`
 *
 * Defaults to `UPDATE_EXPLICIT_TRIGGER` so heavy solver runs only execute on explicit button press.
 */

#include "../../controls/ideam_tool_workspace.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/window.hpp>

namespace ideam::godot_ext {

class ProceduralComposer : public IdeamToolWorkspace {
    GDCLASS(ProceduralComposer, IdeamToolWorkspace)

private:
    static ProceduralComposer *shared_instance_;
    static godot::Window *shared_window_;

    godot::Button *execute_btn_ = nullptr;

protected:
    static void _bind_methods();

    void _populate_command_bar(godot::HBoxContainer *p_custom_actions) override;

public:
    static constexpr const char *WORKSPACE_ID = "ProceduralComposer";

    ProceduralComposer();
    ~ProceduralComposer() override;

    void _on_execute_pressed();

    static ProceduralComposer *get_or_create_shared_composer(godot::Control *p_owner = nullptr);
    static void open_procedural_resource(
        const godot::Ref<godot::Resource> &p_resource,
        godot::Control *p_owner = nullptr
    );
};

} // namespace ideam::godot_ext
