#pragma once

/**
 * tutorial_progress_tracker.h
 * Persists completed tutorial steps identically across Editor (`res://.ideam_tutorials_progress.cfg`)
 * and exported Runtime Games (`user://tutorial_progress.cfg`).
 */

#include "resources/tutorial_resource.h"

#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace ideam::godot_ext {

class TutorialProgressTracker : public godot::RefCounted {
    GDCLASS(TutorialProgressTracker, godot::RefCounted)

private:
    godot::Ref<godot::ConfigFile> config_;
    godot::String storage_path_;

protected:
    static void _bind_methods();

public:
    TutorialProgressTracker();
    ~TutorialProgressTracker() override = default;

    [[nodiscard]] godot::String get_storage_path() const { return storage_path_; }
    void set_storage_path(const godot::String &p_path);

    bool load_progress();
    bool save_progress() const;

    void mark_step_completed(
        const godot::StringName &p_tutorial_id,
        const godot::StringName &p_step_id,
        bool p_completed = true
    );
    [[nodiscard]] bool is_step_completed(
        const godot::StringName &p_tutorial_id,
        const godot::StringName &p_step_id
    ) const;

    [[nodiscard]] float get_completion_ratio(const godot::Ref<TutorialResource> &p_tutorial) const;
    void reset_tutorial(const godot::StringName &p_tutorial_id);
};

} // namespace ideam::godot_ext
