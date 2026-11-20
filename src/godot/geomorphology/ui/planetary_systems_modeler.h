#pragma once

/**
 * planetary_systems_modeler.h
 * Tier 2 Orchestration Workspace (`ToolWorkspace`) unifying Geomorphology, Climatology,
 * and Ecology (`src/godot/geomorphology/ui/`).
 *
 * Sidebar Tabs:
 *   - `Geomorphology`: `GeomorphologyTerrainResource`
 *   - `Climatology`: `ClimatologyAtmosphereResource`
 *   - `Ecology`: `EcologyBiosphereResource`
 *
 * Defaults to `UPDATE_EXPLICIT_TRIGGER` and exposes a causal dependency strip
 * (`Terrain -> Atmosphere -> Biosphere`) plus a `"Step Active Simulation"` trigger in the command bar.
 */

#include "../../tools/tool_workspace.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/window.hpp>

namespace ideam::godot_ext {

class PlanetarySystemsModeler : public ToolWorkspace {
    GDCLASS(PlanetarySystemsModeler, ToolWorkspace)

private:
    static PlanetarySystemsModeler *shared_instance_;
    static godot::Window *shared_window_;

    godot::Button *step_sim_btn_ = nullptr;

protected:
    static void _bind_methods();

    void _populate_command_bar(godot::HBoxContainer *p_custom_actions) override;
    void _on_dirty_banner_action_pressed() override;

public:
    static constexpr const char *WORKSPACE_ID = "PlanetarySystemsModeler";

    PlanetarySystemsModeler();
    ~PlanetarySystemsModeler() override;

    void _on_step_sim_pressed();

    static PlanetarySystemsModeler *get_or_create_shared_modeler(godot::Control *p_owner = nullptr);
    static void open_planetary_resource(
        const godot::Ref<godot::Resource> &p_resource,
        godot::Control *p_owner = nullptr
    );
};

} // namespace ideam::godot_ext
