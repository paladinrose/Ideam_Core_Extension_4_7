#include "ideam_tutorials_plugin.h"
#include "../tools/tool_registry.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/variant/callable.hpp>

using namespace godot;

namespace ideam::godot_ext {

IdeamTutorialsPlugin *IdeamTutorialsPlugin::singleton_ = nullptr;

void IdeamTutorialsPlugin::_bind_methods() {
    ClassDB::bind_method(D_METHOD("open_tutorials_dock"), &IdeamTutorialsPlugin::open_tutorials_dock);
    ClassDB::bind_method(D_METHOD("get_context_indexer"), &IdeamTutorialsPlugin::get_context_indexer);
    ClassDB::bind_method(D_METHOD("get_tool_dispatcher"), &IdeamTutorialsPlugin::get_tool_dispatcher);
    ClassDB::bind_method(D_METHOD("get_slm_bridge"), &IdeamTutorialsPlugin::get_slm_bridge);
    ClassDB::bind_method(D_METHOD("_on_dock_window_closed"), &IdeamTutorialsPlugin::_on_dock_window_closed);
    ClassDB::bind_method(
        D_METHOD("_on_spotlight_step_requested", "step"),
        &IdeamTutorialsPlugin::_on_spotlight_step_requested
    );
}

IdeamTutorialsPlugin::IdeamTutorialsPlugin() {
    singleton_ = this;
    context_indexer_.instantiate();
    tool_dispatcher_.instantiate();
    slm_bridge_.instantiate();
    slm_bridge_->set_context_indexer(context_indexer_);
    slm_bridge_->set_tool_dispatcher(tool_dispatcher_);
}

IdeamTutorialsPlugin::~IdeamTutorialsPlugin() {
    if (singleton_ == this) {
        singleton_ = nullptr;
    }
}

void IdeamTutorialsPlugin::_enter_tree() {
    set_plugin_active("IdeamTutorials", true);

    ToolRegistry::get_singleton().register_help_handler({
        StringName(HELP_PROVIDER_ID),
        10,
        [this](const StringName &p_tool_id, const String &p_ui_selector, const Dictionary &p_context) -> bool {
            return handle_tool_help_request(p_tool_id, p_ui_selector, p_context);
        }
    });

    add_tool_menu_item("Ideam/Tutorials Dock", Callable(this, "open_tutorials_dock"));
}

void IdeamTutorialsPlugin::_exit_tree() {
    remove_tool_menu_item("Ideam/Tutorials Dock");
    ToolRegistry::get_singleton().unregister_help_handler(StringName(HELP_PROVIDER_ID));

    if (dock_window_) {
        dock_window_->queue_free();
        dock_window_    = nullptr;
        checklist_dock_ = nullptr;
    }
    if (spotlight_overlay_) {
        spotlight_overlay_->queue_free();
        spotlight_overlay_ = nullptr;
    }

    set_plugin_active("IdeamTutorials", false);
}

void IdeamTutorialsPlugin::_ensure_dock_window() {
    if (dock_window_ && dock_window_->is_inside_tree()) {
        dock_window_->popup_centered();
        return;
    }

    dock_window_ = memnew(Window);
    dock_window_->set_title("Ideam Tutorials");
    dock_window_->set_min_size(Vector2i(360, 520));
    dock_window_->set_size(Vector2i(380, 560));
    dock_window_->set_transient(false);
    dock_window_->connect("close_requested", Callable(this, "_on_dock_window_closed"));

    checklist_dock_ = memnew(TutorialChecklistDock);
    checklist_dock_->connect("spotlight_step_requested", Callable(this, "_on_spotlight_step_requested"));
    dock_window_->add_child(checklist_dock_);

    if (tool_dispatcher_.is_valid()) {
        tool_dispatcher_->set_checklist_dock(checklist_dock_);
    }

    if (auto *tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop())) {
        if (tree->get_root()) {
            tree->get_root()->add_child(dock_window_);
            if (!spotlight_overlay_ || !spotlight_overlay_->is_inside_tree()) {
                spotlight_overlay_ = memnew(TutorialSpotlightOverlay);
                tree->get_root()->add_child(spotlight_overlay_);
                if (tool_dispatcher_.is_valid()) {
                    tool_dispatcher_->set_spotlight_overlay(spotlight_overlay_);
                }
            }
            dock_window_->popup_centered();
        }
    }
}

void IdeamTutorialsPlugin::open_tutorials_dock() {
    _ensure_dock_window();
}

bool IdeamTutorialsPlugin::handle_tool_help_request(
    const StringName &p_tool_id,
    const String &p_ui_selector,
    const Dictionary &p_context
) {
    (void)p_context;
    _ensure_dock_window();

    if (context_indexer_.is_valid()) {
        Ref<TutorialResource> tut = context_indexer_->find_tutorial_for_help_topic(p_tool_id);
        if (tut.is_valid() && checklist_dock_) {
            checklist_dock_->set_tutorial(tut);
            return true;
        }
    }

    if (tool_dispatcher_.is_valid() && !p_ui_selector.is_empty()) {
        return tool_dispatcher_->point_to_control(
            p_ui_selector,
            String("Help requested for [b]") + String(p_tool_id) + "[/b]."
        );
    }
    return true;
}

void IdeamTutorialsPlugin::_on_dock_window_closed() {
    if (dock_window_) {
        dock_window_->queue_free();
        dock_window_    = nullptr;
        checklist_dock_ = nullptr;
    }
}

void IdeamTutorialsPlugin::_on_spotlight_step_requested(const Ref<TutorialStepResource> &p_step) {
    if (p_step.is_null() || tool_dispatcher_.is_null()) {
        return;
    }
    tool_dispatcher_->point_to_control(
        p_step->get_target_ui_selector(),
        p_step->get_markdown_body().is_empty() ? p_step->get_title() : p_step->get_markdown_body()
    );
}

} // namespace ideam::godot_ext
