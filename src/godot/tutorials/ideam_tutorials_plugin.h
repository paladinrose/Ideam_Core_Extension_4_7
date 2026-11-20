#pragma once

/**
 * ideam_tutorials_plugin.h
 * EditorPlugin entry point for `IdeamTutorials` (`src/godot/tutorials/`).
 *
 * Responsibilities:
 *   - Announces `IdeamTutorials` active in `res://.ideam_registry.cfg`.
 *   - Registers a `ToolHelpHandler` into `ToolRegistry::get_singleton()` so pressing `?` or `F1`
 *     in any `ToolWorkspace` (or calling `domain_tool::request_control_help`) opens the matching
 *     `TutorialResource` in `TutorialChecklistDock` or spotlights the requested UI selector.
 *   - Exposes `Ideam/Tutorials Dock` in Godot's Project -> Tools menu.
 */

#include "copilot/tutorial_context_indexer.h"
#include "copilot/tutorial_slm_bridge.h"
#include "copilot/tutorial_tool_dispatcher.h"
#include "ui/tutorial_checklist_dock.h"
#include "ui/tutorial_spotlight_overlay.h"
#include "../editor/ideam_editor_plugin.h"

#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace ideam::godot_ext {

class IdeamTutorialsPlugin : public IdeamEditorPlugin {
    GDCLASS(IdeamTutorialsPlugin, IdeamEditorPlugin)

private:
    static IdeamTutorialsPlugin *singleton_;

    godot::Ref<TutorialContextIndexer> context_indexer_;
    godot::Ref<TutorialToolDispatcher> tool_dispatcher_;
    godot::Ref<TutorialSLMBridge> slm_bridge_;
    TutorialChecklistDock *checklist_dock_       = nullptr;
    TutorialSpotlightOverlay *spotlight_overlay_ = nullptr;
    godot::Window *dock_window_                  = nullptr;

    void _ensure_dock_window();

protected:
    static void _bind_methods();

public:
    static constexpr const char *HELP_PROVIDER_ID = "IdeamTutorials";

    IdeamTutorialsPlugin();
    ~IdeamTutorialsPlugin() override;

    void _enter_tree() override;
    void _exit_tree() override;

    void open_tutorials_dock();
    bool handle_tool_help_request(
        const godot::StringName &p_tool_id,
        const godot::String &p_ui_selector,
        const godot::Dictionary &p_context
    );

    [[nodiscard]] godot::Ref<TutorialContextIndexer> get_context_indexer() const { return context_indexer_; }
    [[nodiscard]] godot::Ref<TutorialToolDispatcher> get_tool_dispatcher() const { return tool_dispatcher_; }
    [[nodiscard]] godot::Ref<TutorialSLMBridge> get_slm_bridge() const { return slm_bridge_; }

    void _on_dock_window_closed();
    void _on_spotlight_step_requested(const godot::Ref<TutorialStepResource> &p_step);

    static IdeamTutorialsPlugin *get_singleton() { return singleton_; }
};

} // namespace ideam::godot_ext
