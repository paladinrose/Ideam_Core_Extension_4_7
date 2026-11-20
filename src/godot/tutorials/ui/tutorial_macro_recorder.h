#pragma once

/**
 * tutorial_macro_recorder.h
 * Authoring tool (`src/godot/tutorials/ui/`) hooking `IdeamUndoRedo::commit_action`
 * to record developer actions directly into a `TutorialResource`.
 */

#include "../resources/tutorial_resource.h"
#include "../../utilities/ideam_undo_redo.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace ideam::godot_ext {

class TutorialMacroRecorder : public godot::RefCounted {
    GDCLASS(TutorialMacroRecorder, godot::RefCounted)

private:
    godot::Ref<TutorialResource> recorded_tutorial_;
    godot::StringName observer_id_;
    bool recording_   = false;
    int32_t step_seq_ = 0;

    void _on_undo_redo_committed(const godot::String &p_action_name, bool p_executed);

protected:
    static void _bind_methods();

public:
    TutorialMacroRecorder();
    ~TutorialMacroRecorder() override;

    void start_recording(
        const godot::StringName &p_tutorial_id,
        const godot::String &p_title = "Recorded Tutorial"
    );
    godot::Ref<TutorialResource> stop_recording();

    [[nodiscard]] bool is_recording() const noexcept { return recording_; }
    [[nodiscard]] godot::Ref<TutorialResource> get_recorded_tutorial() const { return recorded_tutorial_; }
};

} // namespace ideam::godot_ext
