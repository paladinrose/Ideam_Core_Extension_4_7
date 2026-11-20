#pragma once

/**
 * tutorial_spotlight_overlay.h
 * Guided UI Layer — Medium visual guidance (`src/godot/tutorials/ui/`):
 *   - `TutorialCalloutBubble`: Anchored instruction + value-entry coaching card
 *   - `TutorialSpotlightOverlay`: 60+ FPS dimmer mask with smooth cutout tracking
 *     `Control::get_global_rect()` (or explicit 3D projected `Rect2`) and click pass-through guard.
 */

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/rich_text_label.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace ideam::godot_ext {

class TutorialCalloutBubble : public godot::PanelContainer {
    GDCLASS(TutorialCalloutBubble, godot::PanelContainer)

private:
    godot::VBoxContainer *vbox_     = nullptr;
    godot::Label *title_label_      = nullptr;
    godot::RichTextLabel *body_rtl_ = nullptr;
    godot::Label *coach_hint_label_ = nullptr;
    godot::Button *dismiss_btn_     = nullptr;

protected:
    static void _bind_methods();

public:
    TutorialCalloutBubble();
    ~TutorialCalloutBubble() override = default;

    void set_callout_content(
        const godot::String &p_title,
        const godot::String &p_body,
        const godot::String &p_coach_hint = ""
    );

    void _on_dismiss_pressed();
};

class TutorialSpotlightOverlay : public godot::Control {
    GDCLASS(TutorialSpotlightOverlay, godot::Control)

private:
    godot::Control *tracked_control_ = nullptr;
    godot::Rect2 cutout_rect_;
    float cutout_padding_     = 6.0f;
    godot::Color dimmer_color_ = godot::Color(0.02f, 0.03f, 0.06f, 0.62f);
    godot::Color border_color_ = godot::Color(0.28f, 0.68f, 1.0f, 0.95f);
    TutorialCalloutBubble *callout_bubble_ = nullptr;

    void _sync_cutout_and_callout();

protected:
    static void _bind_methods();

public:
    TutorialSpotlightOverlay();
    ~TutorialSpotlightOverlay() override = default;

    void _process(double p_delta) override;
    void _draw() override;
    bool _has_point(const godot::Vector2 &p_point) const override;

    void spotlight_control(
        godot::Control *p_target,
        const godot::String &p_title,
        const godot::String &p_message,
        const godot::String &p_coach_hint = ""
    );
    void spotlight_rect(
        const godot::Rect2 &p_global_rect,
        const godot::String &p_title,
        const godot::String &p_message,
        const godot::String &p_coach_hint = ""
    );
    void clear_spotlight();

    void set_dimmer_color(const godot::Color &p_color) { dimmer_color_ = p_color; queue_redraw(); }
    [[nodiscard]] godot::Color get_dimmer_color() const { return dimmer_color_; }

    void set_border_color(const godot::Color &p_color) { border_color_ = p_color; queue_redraw(); }
    [[nodiscard]] godot::Color get_border_color() const { return border_color_; }

    void _on_callout_dismissed();
};

} // namespace ideam::godot_ext
