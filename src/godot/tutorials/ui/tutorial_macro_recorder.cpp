#include "tutorial_macro_recorder.h"

namespace ideam::godot_ext {

void TutorialMacroRecorder::_bind_methods() {
    using namespace godot;

    ADD_SIGNAL(MethodInfo(
        "step_recorded",
        PropertyInfo(Variant::OBJECT, "step", PROPERTY_HINT_RESOURCE_TYPE, "TutorialStepResource")
    ));

    ClassDB::bind_method(
        D_METHOD("start_recording", "tutorial_id", "title"),
        &TutorialMacroRecorder::start_recording,
        DEFVAL(String("Recorded Tutorial"))
    );
    ClassDB::bind_method(D_METHOD("stop_recording"), &TutorialMacroRecorder::stop_recording);
    ClassDB::bind_method(D_METHOD("is_recording"), &TutorialMacroRecorder::is_recording);
    ClassDB::bind_method(D_METHOD("get_recorded_tutorial"), &TutorialMacroRecorder::get_recorded_tutorial);
}

TutorialMacroRecorder::TutorialMacroRecorder() {
    observer_id_ = godot::StringName(
        godot::String("TutorialMacroRecorder_") + godot::String::num_uint64(get_instance_id())
    );
}

TutorialMacroRecorder::~TutorialMacroRecorder() {
    if (recording_) {
        IdeamUndoRedo::unregister_commit_observer(observer_id_);
        recording_ = false;
    }
}

void TutorialMacroRecorder::start_recording(
    const godot::StringName &p_tutorial_id,
    const godot::String &p_title
) {
    if (recording_) {
        stop_recording();
    }
    recorded_tutorial_.instantiate();
    recorded_tutorial_->set_tutorial_id(p_tutorial_id);
    recorded_tutorial_->set_title(p_title);
    recorded_tutorial_->set_target_context(CONTEXT_BOTH);
    step_seq_  = 0;
    recording_ = true;

    IdeamUndoRedo::register_commit_observer({
        observer_id_,
        [this](const godot::String &p_action_name, bool p_executed) {
            _on_undo_redo_committed(p_action_name, p_executed);
        }
    });
}

godot::Ref<TutorialResource> TutorialMacroRecorder::stop_recording() {
    if (recording_) {
        IdeamUndoRedo::unregister_commit_observer(observer_id_);
        recording_ = false;
    }
    return recorded_tutorial_;
}

void TutorialMacroRecorder::_on_undo_redo_committed(const godot::String &p_action_name, bool p_executed) {
    if (!recording_ || !p_executed || recorded_tutorial_.is_null() || p_action_name.is_empty()) {
        return;
    }
    ++step_seq_;
    godot::Ref<TutorialStepResource> step;
    step.instantiate();
    step->set_step_id(godot::StringName(godot::String("step_") + godot::String::num_int64(step_seq_)));
    step->set_title(p_action_name);
    step->set_markdown_body(godot::String("Perform action: [b]") + p_action_name + "[/b].");
    step->set_completion_mode(UNDO_REDO_ACTION_MATCH);
    step->set_expected_action_name(p_action_name);

    recorded_tutorial_->add_step(step);
    emit_signal("step_recorded", step);
}

} // namespace ideam::godot_ext
