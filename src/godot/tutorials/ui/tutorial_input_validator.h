#pragma once

/**
 * tutorial_input_validator.h
 * Guided UI Layer validator (`src/godot/tutorials/ui/`) evaluating `TutorialStepResource`
 * completion criteria across Editor and Runtime:
 *   - Resolves stable `ideam_ui_selector` metadata tags across the active `SceneTree`
 *   - Evaluates `PROPERTY_EQUALS` and `TASK_GRAPH_VALIDATED` checks
 *   - Hooks `IdeamUndoRedo::register_commit_observer` for `UNDO_REDO_ACTION_MATCH` auto check-off
 */

#include "../resources/tutorial_resource.h"
#include "../tutorial_progress_tracker.h"
#include "../../utilities/ideam_undo_redo.h"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace ideam::godot_ext {

class TutorialInputValidator : public godot::RefCounted {
    GDCLASS(TutorialInputValidator, godot::RefCounted)

private:
    godot::Ref<TutorialResource> active_tutorial_;
    godot::Ref<TutorialProgressTracker> progress_tracker_;
    godot::StringName undo_observer_id_;
    bool listening_ = false;

    static godot::Control *_find_control_recursive(godot::Node *p_root, const godot::String &p_selector);

protected:
    static void _bind_methods();

public:
    TutorialInputValidator();
    ~TutorialInputValidator() override;

    void set_active_tutorial(const godot::Ref<TutorialResource> &p_tutorial) { active_tutorial_ = p_tutorial; }
    [[nodiscard]] godot::Ref<TutorialResource> get_active_tutorial() const { return active_tutorial_; }

    void set_progress_tracker(const godot::Ref<TutorialProgressTracker> &p_tracker) { progress_tracker_ = p_tracker; }
    [[nodiscard]] godot::Ref<TutorialProgressTracker> get_progress_tracker() const { return progress_tracker_; }

    void start_listening();
    void stop_listening();

    [[nodiscard]] static godot::Control *resolve_ui_selector(
        godot::Node *p_search_root,
        const godot::String &p_selector
    );

    bool validate_step(const godot::Ref<TutorialStepResource> &p_step, godot::Node *p_search_root = nullptr);
    void poll_active_tutorial(godot::Node *p_search_root = nullptr);
    void notify_undo_redo_action(const godot::String &p_action_name);
    void notify_signal_emitted(const godot::StringName &p_signal_name, const godot::String &p_ui_selector = "");
};

} // namespace ideam::godot_ext
