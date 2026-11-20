#include "tutorial_progress_tracker.h"
#include "../../core/tutorials/tutorial_common.h"

#include <godot_cpp/classes/engine.hpp>

namespace ideam::godot_ext {

void TutorialProgressTracker::_bind_methods() {
    using namespace godot;

    ADD_SIGNAL(MethodInfo(
        "step_completion_changed",
        PropertyInfo(Variant::STRING_NAME, "tutorial_id"),
        PropertyInfo(Variant::STRING_NAME, "step_id"),
        PropertyInfo(Variant::BOOL, "completed")
    ));

    ClassDB::bind_method(D_METHOD("get_storage_path"), &TutorialProgressTracker::get_storage_path);
    ClassDB::bind_method(D_METHOD("set_storage_path", "path"), &TutorialProgressTracker::set_storage_path);
    ClassDB::bind_method(D_METHOD("load_progress"), &TutorialProgressTracker::load_progress);
    ClassDB::bind_method(D_METHOD("save_progress"), &TutorialProgressTracker::save_progress);

    ClassDB::bind_method(
        D_METHOD("mark_step_completed", "tutorial_id", "step_id", "completed"),
        &TutorialProgressTracker::mark_step_completed,
        DEFVAL(true)
    );
    ClassDB::bind_method(
        D_METHOD("is_step_completed", "tutorial_id", "step_id"),
        &TutorialProgressTracker::is_step_completed
    );
    ClassDB::bind_method(
        D_METHOD("get_completion_ratio", "tutorial"),
        &TutorialProgressTracker::get_completion_ratio
    );
    ClassDB::bind_method(D_METHOD("reset_tutorial", "tutorial_id"), &TutorialProgressTracker::reset_tutorial);
}

TutorialProgressTracker::TutorialProgressTracker() {
    config_.instantiate();
    if (godot::Engine::get_singleton()->is_editor_hint()) {
        storage_path_ = godot::String(core::tutorials::EDITOR_PROGRESS_PATH.data());
    } else {
        storage_path_ = godot::String(core::tutorials::RUNTIME_PROGRESS_PATH.data());
    }
    load_progress();
}

void TutorialProgressTracker::set_storage_path(const godot::String &p_path) {
    if (!p_path.is_empty()) {
        storage_path_ = p_path;
        load_progress();
    }
}

bool TutorialProgressTracker::load_progress() {
    if (config_.is_null()) {
        config_.instantiate();
    }
    return config_->load(storage_path_) == godot::OK;
}

bool TutorialProgressTracker::save_progress() const {
    if (config_.is_null() || storage_path_.is_empty()) {
        return false;
    }
    return config_->save(storage_path_) == godot::OK;
}

void TutorialProgressTracker::mark_step_completed(
    const godot::StringName &p_tutorial_id,
    const godot::StringName &p_step_id,
    bool p_completed
) {
    if (config_.is_null() || p_tutorial_id == godot::StringName() || p_step_id == godot::StringName()) {
        return;
    }
    const godot::String sec(p_tutorial_id);
    const godot::String key(p_step_id);
    config_->set_value(sec, key, p_completed);
    save_progress();
    emit_signal("step_completion_changed", p_tutorial_id, p_step_id, p_completed);
}

bool TutorialProgressTracker::is_step_completed(
    const godot::StringName &p_tutorial_id,
    const godot::StringName &p_step_id
) const {
    if (config_.is_null() || p_tutorial_id == godot::StringName() || p_step_id == godot::StringName()) {
        return false;
    }
    return static_cast<bool>(config_->get_value(godot::String(p_tutorial_id), godot::String(p_step_id), false));
}

float TutorialProgressTracker::get_completion_ratio(const godot::Ref<TutorialResource> &p_tutorial) const {
    if (p_tutorial.is_null()) {
        return 0.0f;
    }
    const auto steps = p_tutorial->get_steps();
    if (steps.is_empty()) {
        return 0.0f;
    }
    int done = 0;
    for (int i = 0; i < steps.size(); ++i) {
        godot::Ref<TutorialStepResource> step = steps[i];
        if (step.is_valid() && is_step_completed(p_tutorial->get_tutorial_id(), step->get_step_id())) {
            ++done;
        }
    }
    return static_cast<float>(done) / static_cast<float>(steps.size());
}

void TutorialProgressTracker::reset_tutorial(const godot::StringName &p_tutorial_id) {
    if (config_.is_null() || p_tutorial_id == godot::StringName()) {
        return;
    }
    const godot::String sec(p_tutorial_id);
    if (config_->has_section(sec)) {
        config_->erase_section(sec);
        save_progress();
    }
}

} // namespace ideam::godot_ext
