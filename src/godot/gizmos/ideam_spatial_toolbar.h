#pragma once

/**
 * ideam_spatial_toolbar.h
 * Reusable Trans-Environment 3D Viewport Toolbar (`IdeamSpatialToolbar`, `godot::HBoxContainer`)
 * in `src/godot/gizmos/`.
 *
 * Driven by a `ToolModeSpec` list supporting:
 *   - Mutually exclusive tool modes (e.g., Scaffolding / Topology / Features)
 *   - Persistent independent toggles (e.g., Details: ON/OFF)
 *   - `ToolContext` synchronization and `IdeamTutorials` `ideam_ui_selector` stamping
 *
 * Mounts seamlessly in all 4 contexts:
 *   1. Godot Editor 3D Viewport (`EditorPlugin::CONTAINER_SPATIAL_EDITOR_MENU`)
 *   2. Custom Editor 3D SubViewport (`ToolWorkspace::ribbon_slot_`)
 *   3. Runtime 3D Gameplay HUD / Diegetic Panel
 *   4. Runtime Dedicated Avatar Creator (`CharacterModeler`)
 */

#include "../tools/tool_context.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

#include <unordered_map>
#include <vector>

namespace ideam::godot_ext {

struct ToolModeSpec {
    godot::StringName id;
    godot::String label;
    godot::String tooltip;
    bool is_toggle       = false; ///< false = exclusive mode button; true = independent persistent toggle
    bool default_toggled = false;
    godot::Key shortcut_key = godot::KEY_NONE;
    godot::String ui_selector;
};

class IdeamSpatialToolbar : public godot::HBoxContainer {
    GDCLASS(IdeamSpatialToolbar, godot::HBoxContainer)

private:
    struct ButtonEntry {
        ToolModeSpec spec;
        godot::Button *button = nullptr;
    };

    godot::Ref<ToolContext> tool_context_;
    std::vector<ButtonEntry> entries_;
    godot::StringName active_mode_id_;
    bool updating_buttons_ = false;

    void _on_mode_button_pressed(const godot::StringName &p_id);
    void _on_toggle_button_toggled(bool p_pressed, const godot::StringName &p_id);

protected:
    static void _bind_methods();

public:
    IdeamSpatialToolbar();
    ~IdeamSpatialToolbar() override = default;

    void set_tool_context(const godot::Ref<ToolContext> &p_context);
    [[nodiscard]] godot::Ref<ToolContext> get_tool_context() const { return tool_context_; }

    void clear_tools();
    void add_tool_spec(const ToolModeSpec &p_spec);
    void add_mode_button(
        const godot::StringName &p_id,
        const godot::String &p_label,
        const godot::String &p_tooltip = godot::String(),
        const godot::String &p_ui_selector = godot::String()
    );
    void add_toggle_button(
        const godot::StringName &p_id,
        const godot::String &p_label,
        const godot::String &p_tooltip = godot::String(),
        bool p_default_toggled = true,
        const godot::String &p_ui_selector = godot::String()
    );
    void add_separator();

    void set_active_mode(const godot::StringName &p_mode_id, bool p_emit_signal = true);
    [[nodiscard]] godot::StringName get_active_mode() const { return active_mode_id_; }

    void set_toggle_state(const godot::StringName &p_toggle_id, bool p_pressed, bool p_emit_signal = true);
    [[nodiscard]] bool get_toggle_state(const godot::StringName &p_toggle_id) const;
};

} // namespace ideam::godot_ext
