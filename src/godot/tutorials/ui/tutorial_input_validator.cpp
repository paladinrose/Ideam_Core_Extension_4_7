#include "tutorial_input_validator.h"
#include "../../../core/tutorials/tutorial_common.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>

namespace ideam::godot_ext {

void TutorialInputValidator::_bind_methods() {
    using namespace godot;

    ADD_SIGNAL(MethodInfo(
        "step_auto_validated",
        PropertyInfo(Variant::STRING_NAME, "tutorial_id"),
        PropertyInfo(Variant::STRING_NAME, "step_id")
    ));

    ClassDB::bind_method(D_METHOD("set_active_tutorial", "tutorial"), &TutorialInputValidator::set_active_tutorial);
    ClassDB::bind_method(D_METHOD("get_active_tutorial"), &TutorialInputValidator::get_active_tutorial);
    ClassDB::bind_method(D_METHOD("set_progress_tracker", "tracker"), &TutorialInputValidator::set_progress_tracker);
    ClassDB::bind_method(D_METHOD("get_progress_tracker"), &TutorialInputValidator::get_progress_tracker);

    ClassDB::bind_method(D_METHOD("start_listening"), &TutorialInputValidator::start_listening);
    ClassDB::bind_method(D_METHOD("stop_listening"), &TutorialInputValidator::stop_listening);
    ClassDB::bind_method(
        D_METHOD("validate_step", "step", "search_root"),
        &TutorialInputValidator::validate_step,
        DEFVAL(nullptr)
    );
    ClassDB::bind_method(
        D_METHOD("poll_active_tutorial", "search_root"),
        &TutorialInputValidator::poll_active_tutorial,
        DEFVAL(nullptr)
    );
    ClassDB::bind_method(
        D_METHOD("notify_undo_redo_action", "action_name"),
        &TutorialInputValidator::notify_undo_redo_action
    );
    ClassDB::bind_method(
        D_METHOD("notify_signal_emitted", "signal_name", "ui_selector"),
        &TutorialInputValidator::notify_signal_emitted,
        DEFVAL(String())
    );
}

TutorialInputValidator::TutorialInputValidator() {
    undo_observer_id_ = godot::StringName(
        godot::String("TutorialInputValidator_") + godot::String::num_uint64(get_instance_id())
    );
}

TutorialInputValidator::~TutorialInputValidator() {
    stop_listening();
}

void TutorialInputValidator::start_listening() {
    if (listening_) {
        return;
    }
    listening_ = true;
    IdeamUndoRedo::register_commit_observer({
        undo_observer_id_,
        [this](const godot::String &p_action_name, bool p_executed) {
            if (p_executed) {
                notify_undo_redo_action(p_action_name);
            }
        }
    });
}

void TutorialInputValidator::stop_listening() {
    if (!listening_) {
        return;
    }
    listening_ = false;
    IdeamUndoRedo::unregister_commit_observer(undo_observer_id_);
}

godot::Control *TutorialInputValidator::_find_control_recursive(
    godot::Node *p_root,
    const godot::String &p_selector
) {
    if (!p_root || p_selector.is_empty()) {
        return nullptr;
    }
    const godot::StringName meta_key(core::tutorials::UI_SELECTOR_META_KEY.data());
    if (auto *ctrl = godot::Object::cast_to<godot::Control>(p_root)) {
        if (ctrl->has_meta(meta_key) && godot::String(ctrl->get_meta(meta_key)) == p_selector) {
            return ctrl;
        }
        if (godot::String(ctrl->get_name()) == p_selector) {
            return ctrl;
        }
    }
    const int count = p_root->get_child_count();
    for (int i = 0; i < count; ++i) {
        if (auto *found = _find_control_recursive(p_root->get_child(i), p_selector)) {
            return found;
        }
    }
    return nullptr;
}

godot::Control *TutorialInputValidator::resolve_ui_selector(
    godot::Node *p_search_root,
    const godot::String &p_selector
) {
    if (p_selector.is_empty()) {
        return nullptr;
    }
    godot::Node *root = p_search_root;
    if (!root) {
        if (auto *tree = godot::Object::cast_to<godot::SceneTree>(godot::Engine::get_singleton()->get_main_loop())) {
            root = tree->get_root();
        }
    }
    if (!root) {
        return nullptr;
    }
    if (godot::Node *direct = root->get_node_or_null(godot::NodePath(p_selector))) {
        if (auto *ctrl = godot::Object::cast_to<godot::Control>(direct)) {
            return ctrl;
        }
    }
    return _find_control_recursive(root, p_selector);
}

bool TutorialInputValidator::validate_step(
    const godot::Ref<TutorialStepResource> &p_step,
    godot::Node *p_search_root
) {
    if (p_step.is_null()) {
        return false;
    }
    if (p_step->get_completion_mode() == PROPERTY_EQUALS) {
        godot::Control *target = resolve_ui_selector(p_search_root, p_step->get_target_ui_selector());
        if (!target || p_step->get_target_property() == godot::StringName()) {
            return false;
        }
        const godot::Variant current = target->get(p_step->get_target_property());
        return current == p_step->get_expected_value();
    }
    return false;
}

void TutorialInputValidator::poll_active_tutorial(godot::Node *p_search_root) {
    if (active_tutorial_.is_null() || progress_tracker_.is_null()) {
        return;
    }
    const auto steps = active_tutorial_->get_steps();
    const godot::StringName tut_id = active_tutorial_->get_tutorial_id();
    for (int i = 0; i < steps.size(); ++i) {
        godot::Ref<TutorialStepResource> step = steps[i];
        if (step.is_null() || progress_tracker_->is_step_completed(tut_id, step->get_step_id())) {
            continue;
        }
        if (step->get_completion_mode() == PROPERTY_EQUALS && validate_step(step, p_search_root)) {
            progress_tracker_->mark_step_completed(tut_id, step->get_step_id(), true);
            emit_signal("step_auto_validated", tut_id, step->get_step_id());
        }
    }
}

void TutorialInputValidator::notify_undo_redo_action(const godot::String &p_action_name) {
    if (active_tutorial_.is_null() || progress_tracker_.is_null() || p_action_name.is_empty()) {
        return;
    }
    const auto steps = active_tutorial_->get_steps();
    const godot::StringName tut_id = active_tutorial_->get_tutorial_id();
    for (int i = 0; i < steps.size(); ++i) {
        godot::Ref<TutorialStepResource> step = steps[i];
        if (step.is_null() || step->get_completion_mode() != UNDO_REDO_ACTION_MATCH) {
            continue;
        }
        if (progress_tracker_->is_step_completed(tut_id, step->get_step_id())) {
            continue;
        }
        const godot::String expected = step->get_expected_action_name();
        if (!expected.is_empty() && (p_action_name == expected || p_action_name.contains(expected))) {
            progress_tracker_->mark_step_completed(tut_id, step->get_step_id(), true);
            emit_signal("step_auto_validated", tut_id, step->get_step_id());
        }
    }
}

void TutorialInputValidator::notify_signal_emitted(
    const godot::StringName &p_signal_name,
    const godot::String &p_ui_selector
) {
    if (active_tutorial_.is_null() || progress_tracker_.is_null()) {
        return;
    }
    const auto steps = active_tutorial_->get_steps();
    const godot::StringName tut_id = active_tutorial_->get_tutorial_id();
    for (int i = 0; i < steps.size(); ++i) {
        godot::Ref<TutorialStepResource> step = steps[i];
        if (step.is_null() || step->get_completion_mode() != SIGNAL_EMITTED) {
            continue;
        }
        if (progress_tracker_->is_step_completed(tut_id, step->get_step_id())) {
            continue;
        }
        const bool selector_ok = p_ui_selector.is_empty() || step->get_target_ui_selector() == p_ui_selector;
        if (selector_ok && step->get_expected_action_name() == godot::String(p_signal_name)) {
            progress_tracker_->mark_step_completed(tut_id, step->get_step_id(), true);
            emit_signal("step_auto_validated", tut_id, step->get_step_id());
        }
    }
}

} // namespace ideam::godot_ext
