#include "tutorial_checklist_dock.h"

#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/rich_text_label.hpp>
#include <godot_cpp/variant/callable.hpp>

using namespace godot;

namespace ideam::godot_ext {

void TutorialChecklistDock::_bind_methods() {
    ADD_SIGNAL(MethodInfo(
        "spotlight_step_requested",
        PropertyInfo(Variant::OBJECT, "step", PROPERTY_HINT_RESOURCE_TYPE, "TutorialStepResource")
    ));

    ClassDB::bind_method(D_METHOD("set_tutorial", "tutorial"), &TutorialChecklistDock::set_tutorial);
    ClassDB::bind_method(D_METHOD("get_tutorial"), &TutorialChecklistDock::get_tutorial);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "tutorial", PROPERTY_HINT_RESOURCE_TYPE, "TutorialResource"),
        "set_tutorial",
        "get_tutorial"
    );

    ClassDB::bind_method(D_METHOD("set_progress_tracker", "tracker"), &TutorialChecklistDock::set_progress_tracker);
    ClassDB::bind_method(D_METHOD("get_progress_tracker"), &TutorialChecklistDock::get_progress_tracker);
    ClassDB::bind_method(D_METHOD("get_validator"), &TutorialChecklistDock::get_validator);

    ClassDB::bind_method(
        D_METHOD("_on_step_checkbox_toggled", "pressed", "step_id"),
        &TutorialChecklistDock::_on_step_checkbox_toggled
    );
    ClassDB::bind_method(
        D_METHOD("_on_step_spotlight_pressed", "step_id"),
        &TutorialChecklistDock::_on_step_spotlight_pressed
    );
    ClassDB::bind_method(
        D_METHOD("_on_step_demonstrate_pressed", "step_id"),
        &TutorialChecklistDock::_on_step_demonstrate_pressed
    );
    ClassDB::bind_method(
        D_METHOD("_on_step_auto_validated", "tutorial_id", "step_id"),
        &TutorialChecklistDock::_on_step_auto_validated
    );
    ClassDB::bind_method(D_METHOD("_on_reset_pressed"), &TutorialChecklistDock::_on_reset_pressed);
}

TutorialChecklistDock::TutorialChecklistDock() {
    set_h_size_flags(SIZE_EXPAND_FILL);
    set_v_size_flags(SIZE_EXPAND_FILL);
    set_custom_minimum_size(Vector2(280.0f, 240.0f));
    set_meta("ideam_ui_selector", "IdeamTutorials/checklist_dock");

    progress_tracker_.instantiate();
    validator_.instantiate();
    validator_->set_progress_tracker(progress_tracker_);
    validator_->connect("step_auto_validated", Callable(this, "_on_step_auto_validated"));

    header_bar_ = memnew(HBoxContainer);
    header_bar_->set_h_size_flags(SIZE_EXPAND_FILL);
    add_child(header_bar_);

    progress_gauge_ = memnew(CircularGauge);
    progress_gauge_->set_custom_minimum_size(Vector2(44.0f, 44.0f));
    progress_gauge_->set_min_value(0.0f);
    progress_gauge_->set_max_value(100.0f);
    progress_gauge_->set_value_1(0.0f);
    progress_gauge_->set_value_2(0.0f);
    progress_gauge_->set_draw_as_band(true);
    progress_gauge_->set_draw_head_tail_markers(false);
    header_bar_->add_child(progress_gauge_);

    VBoxContainer *title_col = memnew(VBoxContainer);
    title_col->set_h_size_flags(SIZE_EXPAND_FILL);
    header_bar_->add_child(title_col);

    title_label_ = memnew(Label);
    title_label_->set_text("No Tutorial Loaded");
    title_col->add_child(title_label_);

    progress_label_ = memnew(Label);
    progress_label_->set_text("0% complete");
    title_col->add_child(progress_label_);

    reset_btn_ = memnew(Button);
    reset_btn_->set_text("Reset");
    reset_btn_->connect("pressed", Callable(this, "_on_reset_pressed"));
    header_bar_->add_child(reset_btn_);

    steps_scroll_ = memnew(ScrollContainer);
    steps_scroll_->set_h_size_flags(SIZE_EXPAND_FILL);
    steps_scroll_->set_v_size_flags(SIZE_EXPAND_FILL);
    add_child(steps_scroll_);

    steps_vbox_ = memnew(VBoxContainer);
    steps_vbox_->set_h_size_flags(SIZE_EXPAND_FILL);
    steps_scroll_->add_child(steps_vbox_);
}

TutorialChecklistDock::~TutorialChecklistDock() {
    if (validator_.is_valid()) {
        validator_->stop_listening();
    }
}

void TutorialChecklistDock::set_tutorial(const Ref<TutorialResource> &p_tutorial) {
    tutorial_ = p_tutorial;
    if (validator_.is_valid()) {
        validator_->set_active_tutorial(tutorial_);
        if (tutorial_.is_valid()) {
            validator_->start_listening();
        } else {
            validator_->stop_listening();
        }
    }
    _rebuild_step_rows();
    _update_progress_gauge();
}

void TutorialChecklistDock::set_progress_tracker(const Ref<TutorialProgressTracker> &p_tracker) {
    progress_tracker_ = p_tracker.is_valid() ? p_tracker : Ref<TutorialProgressTracker>();
    if (progress_tracker_.is_null()) {
        progress_tracker_.instantiate();
    }
    if (validator_.is_valid()) {
        validator_->set_progress_tracker(progress_tracker_);
    }
    _rebuild_step_rows();
    _update_progress_gauge();
}

void TutorialChecklistDock::_rebuild_step_rows() {
    if (!steps_vbox_) {
        return;
    }
    while (steps_vbox_->get_child_count() > 0) {
        Node *child = steps_vbox_->get_child(0);
        steps_vbox_->remove_child(child);
        child->queue_free();
    }

    if (tutorial_.is_null()) {
        if (title_label_) title_label_->set_text("No Tutorial Loaded");
        return;
    }

    if (title_label_) {
        title_label_->set_text(
            tutorial_->get_title().is_empty() ? String(tutorial_->get_tutorial_id()) : tutorial_->get_title()
        );
    }

    const auto steps        = tutorial_->get_steps();
    const StringName tut_id = tutorial_->get_tutorial_id();

    for (int i = 0; i < steps.size(); ++i) {
        Ref<TutorialStepResource> step = steps[i];
        if (step.is_null()) {
            continue;
        }
        const StringName step_id = step->get_step_id();
        const bool is_done       = progress_tracker_.is_valid() && progress_tracker_->is_step_completed(tut_id, step_id);

        PanelContainer *card = memnew(PanelContainer);
        card->set_h_size_flags(SIZE_EXPAND_FILL);
        steps_vbox_->add_child(card);

        VBoxContainer *card_vb = memnew(VBoxContainer);
        card_vb->set_h_size_flags(SIZE_EXPAND_FILL);
        card->add_child(card_vb);

        HBoxContainer *row = memnew(HBoxContainer);
        row->set_h_size_flags(SIZE_EXPAND_FILL);
        card_vb->add_child(row);

        CheckBox *cb = memnew(CheckBox);
        cb->set_text(step->get_title().is_empty() ? String(step_id) : step->get_title());
        cb->set_pressed_no_signal(is_done);
        cb->set_h_size_flags(SIZE_EXPAND_FILL);
        cb->connect("toggled", Callable(this, "_on_step_checkbox_toggled").bind(step_id));
        row->add_child(cb);

        if (!step->get_target_ui_selector().is_empty()) {
            Button *spot_btn = memnew(Button);
            spot_btn->set_text("Locate");
            spot_btn->set_tooltip_text(String("Highlight ") + step->get_target_ui_selector());
            spot_btn->connect("pressed", Callable(this, "_on_step_spotlight_pressed").bind(step_id));
            row->add_child(spot_btn);
        }

        if (step->get_auto_demonstrate_callable().is_valid()) {
            Button *demo_btn = memnew(Button);
            demo_btn->set_text("Show Me");
            demo_btn->connect("pressed", Callable(this, "_on_step_demonstrate_pressed").bind(step_id));
            row->add_child(demo_btn);
        }

        if (!step->get_markdown_body().is_empty()) {
            RichTextLabel *body = memnew(RichTextLabel);
            body->set_use_bbcode(true);
            body->set_fit_content(true);
            body->set_text(step->get_markdown_body());
            card_vb->add_child(body);
        }
    }
}

void TutorialChecklistDock::_update_progress_gauge() {
    float ratio = 0.0f;
    if (tutorial_.is_valid() && progress_tracker_.is_valid()) {
        ratio = progress_tracker_->get_completion_ratio(tutorial_);
    }
    const float pct = ratio * 100.0f;
    if (progress_gauge_) {
        progress_gauge_->set_value_1(0.0f);
        progress_gauge_->set_value_2(pct);
    }
    if (progress_label_) {
        progress_label_->set_text(String::num_int64(static_cast<int64_t>(pct + 0.5f)) + String("% complete"));
    }
}

void TutorialChecklistDock::_on_step_checkbox_toggled(bool p_pressed, const StringName &p_step_id) {
    if (tutorial_.is_valid() && progress_tracker_.is_valid()) {
        progress_tracker_->mark_step_completed(tutorial_->get_tutorial_id(), p_step_id, p_pressed);
        _update_progress_gauge();
    }
}

void TutorialChecklistDock::_on_step_spotlight_pressed(const StringName &p_step_id) {
    if (tutorial_.is_null()) {
        return;
    }
    const auto steps = tutorial_->get_steps();
    for (int i = 0; i < steps.size(); ++i) {
        Ref<TutorialStepResource> step = steps[i];
        if (step.is_valid() && step->get_step_id() == p_step_id) {
            emit_signal("spotlight_step_requested", step);
            return;
        }
    }
}

void TutorialChecklistDock::_on_step_demonstrate_pressed(const StringName &p_step_id) {
    if (tutorial_.is_null()) {
        return;
    }
    const auto steps = tutorial_->get_steps();
    for (int i = 0; i < steps.size(); ++i) {
        Ref<TutorialStepResource> step = steps[i];
        if (step.is_valid() && step->get_step_id() == p_step_id) {
            const Callable cb = step->get_auto_demonstrate_callable();
            if (cb.is_valid()) {
                cb.call();
            }
            return;
        }
    }
}

void TutorialChecklistDock::_on_step_auto_validated(const StringName &p_tutorial_id, const StringName &p_step_id) {
    (void)p_step_id;
    if (tutorial_.is_valid() && tutorial_->get_tutorial_id() == p_tutorial_id) {
        _rebuild_step_rows();
        _update_progress_gauge();
    }
}

void TutorialChecklistDock::_on_reset_pressed() {
    if (tutorial_.is_valid() && progress_tracker_.is_valid()) {
        progress_tracker_->reset_tutorial(tutorial_->get_tutorial_id());
        _rebuild_step_rows();
        _update_progress_gauge();
    }
}

} // namespace ideam::godot_ext
