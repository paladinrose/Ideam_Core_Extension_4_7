#pragma once

/**
 * tutorial_checklist_dock.h
 * Guided UI Layer — Light collapsible dock (`src/godot/tutorials/ui/`):
 *   - Renders `TutorialResource` steps with interactive checkboxes
 *   - Displays overall completion via `CircularGauge`
 *   - Provides clickable Spotlight & Auto-Demonstrate triggers per step
 *   - Syncs live with `TutorialProgressTracker` and `TutorialInputValidator`
 */

#include "tutorial_input_validator.h"
#include "../resources/tutorial_resource.h"
#include "../tutorial_progress_tracker.h"
#include "../../controls/circular_gauge.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace ideam::godot_ext {

class TutorialChecklistDock : public godot::VBoxContainer {
    GDCLASS(TutorialChecklistDock, godot::VBoxContainer)

private:
    godot::Ref<TutorialResource> tutorial_;
    godot::Ref<TutorialProgressTracker> progress_tracker_;
    godot::Ref<TutorialInputValidator> validator_;

    godot::HBoxContainer *header_bar_    = nullptr;
    CircularGauge *progress_gauge_       = nullptr;
    godot::Label *title_label_           = nullptr;
    godot::Label *progress_label_        = nullptr;
    godot::Button *reset_btn_            = nullptr;
    godot::ScrollContainer *steps_scroll_ = nullptr;
    godot::VBoxContainer *steps_vbox_    = nullptr;

    void _rebuild_step_rows();
    void _update_progress_gauge();

protected:
    static void _bind_methods();

public:
    TutorialChecklistDock();
    ~TutorialChecklistDock() override;

    void set_tutorial(const godot::Ref<TutorialResource> &p_tutorial);
    [[nodiscard]] godot::Ref<TutorialResource> get_tutorial() const { return tutorial_; }

    void set_progress_tracker(const godot::Ref<TutorialProgressTracker> &p_tracker);
    [[nodiscard]] godot::Ref<TutorialProgressTracker> get_progress_tracker() const { return progress_tracker_; }

    [[nodiscard]] godot::Ref<TutorialInputValidator> get_validator() const { return validator_; }

    void _on_step_checkbox_toggled(bool p_pressed, const godot::StringName &p_step_id);
    void _on_step_spotlight_pressed(const godot::StringName &p_step_id);
    void _on_step_demonstrate_pressed(const godot::StringName &p_step_id);
    void _on_step_auto_validated(const godot::StringName &p_tutorial_id, const godot::StringName &p_step_id);
    void _on_reset_pressed();
};

} // namespace ideam::godot_ext
