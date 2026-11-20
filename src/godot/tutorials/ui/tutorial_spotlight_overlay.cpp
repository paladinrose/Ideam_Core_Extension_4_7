#include "tutorial_spotlight_overlay.h"

#include <godot_cpp/variant/callable.hpp>

#include <algorithm>

using namespace godot;

namespace ideam::godot_ext {

// ============================================================================
// TutorialCalloutBubble
// ============================================================================

void TutorialCalloutBubble::_bind_methods() {
    ADD_SIGNAL(MethodInfo("dismiss_requested"));

    ClassDB::bind_method(
        D_METHOD("set_callout_content", "title", "body", "coach_hint"),
        &TutorialCalloutBubble::set_callout_content,
        DEFVAL(String())
    );
    ClassDB::bind_method(D_METHOD("_on_dismiss_pressed"), &TutorialCalloutBubble::_on_dismiss_pressed);
}

TutorialCalloutBubble::TutorialCalloutBubble() {
    set_custom_minimum_size(Vector2(260.0f, 96.0f));
    set_mouse_filter(MOUSE_FILTER_STOP);

    vbox_ = memnew(VBoxContainer);
    vbox_->set_h_size_flags(SIZE_EXPAND_FILL);
    vbox_->set_v_size_flags(SIZE_EXPAND_FILL);
    add_child(vbox_);

    title_label_ = memnew(Label);
    title_label_->set_h_size_flags(SIZE_EXPAND_FILL);
    vbox_->add_child(title_label_);

    body_rtl_ = memnew(RichTextLabel);
    body_rtl_->set_use_bbcode(true);
    body_rtl_->set_fit_content(true);
    body_rtl_->set_h_size_flags(SIZE_EXPAND_FILL);
    vbox_->add_child(body_rtl_);

    coach_hint_label_ = memnew(Label);
    coach_hint_label_->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
    coach_hint_label_->set_visible(false);
    vbox_->add_child(coach_hint_label_);

    dismiss_btn_ = memnew(Button);
    dismiss_btn_->set_text("Got it");
    dismiss_btn_->connect("pressed", Callable(this, "_on_dismiss_pressed"));
    vbox_->add_child(dismiss_btn_);
}

void TutorialCalloutBubble::set_callout_content(
    const String &p_title,
    const String &p_body,
    const String &p_coach_hint
) {
    if (title_label_) {
        title_label_->set_text(p_title);
    }
    if (body_rtl_) {
        body_rtl_->set_text(p_body);
    }
    if (coach_hint_label_) {
        coach_hint_label_->set_text(p_coach_hint);
        coach_hint_label_->set_visible(!p_coach_hint.is_empty());
    }
}

void TutorialCalloutBubble::_on_dismiss_pressed() {
    emit_signal("dismiss_requested");
}

// ============================================================================
// TutorialSpotlightOverlay
// ============================================================================

void TutorialSpotlightOverlay::_bind_methods() {
    ADD_SIGNAL(MethodInfo("spotlight_cleared"));

    ClassDB::bind_method(
        D_METHOD("spotlight_control", "target", "title", "message", "coach_hint"),
        &TutorialSpotlightOverlay::spotlight_control,
        DEFVAL(String())
    );
    ClassDB::bind_method(
        D_METHOD("spotlight_rect", "global_rect", "title", "message", "coach_hint"),
        &TutorialSpotlightOverlay::spotlight_rect,
        DEFVAL(String())
    );
    ClassDB::bind_method(D_METHOD("clear_spotlight"), &TutorialSpotlightOverlay::clear_spotlight);

    ClassDB::bind_method(D_METHOD("set_dimmer_color", "color"), &TutorialSpotlightOverlay::set_dimmer_color);
    ClassDB::bind_method(D_METHOD("get_dimmer_color"), &TutorialSpotlightOverlay::get_dimmer_color);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "dimmer_color"), "set_dimmer_color", "get_dimmer_color");

    ClassDB::bind_method(D_METHOD("set_border_color", "color"), &TutorialSpotlightOverlay::set_border_color);
    ClassDB::bind_method(D_METHOD("get_border_color"), &TutorialSpotlightOverlay::get_border_color);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "border_color"), "set_border_color", "get_border_color");

    ClassDB::bind_method(D_METHOD("_on_callout_dismissed"), &TutorialSpotlightOverlay::_on_callout_dismissed);
}

TutorialSpotlightOverlay::TutorialSpotlightOverlay() {
    set_anchors_preset(PRESET_FULL_RECT);
    set_mouse_filter(MOUSE_FILTER_STOP);
    set_visible(false);
    set_process(false);

    callout_bubble_ = memnew(TutorialCalloutBubble);
    callout_bubble_->connect("dismiss_requested", Callable(this, "_on_callout_dismissed"));
    add_child(callout_bubble_);
}

void TutorialSpotlightOverlay::_sync_cutout_and_callout() {
    if (tracked_control_ && tracked_control_->is_inside_tree()) {
        const Rect2 global_r = tracked_control_->get_global_rect();
        const Vector2 local_pos = global_r.position - get_global_position();
        cutout_rect_ = Rect2(
            local_pos - Vector2(cutout_padding_, cutout_padding_),
            global_r.size + Vector2(cutout_padding_ * 2.0f, cutout_padding_ * 2.0f)
        );
    }

    if (callout_bubble_) {
        const Vector2 overlay_sz = get_size();
        const Vector2 bubble_sz  = callout_bubble_->get_combined_minimum_size();
        Vector2 pos(cutout_rect_.position.x, cutout_rect_.position.y + cutout_rect_.size.y + 12.0f);
        if (pos.y + bubble_sz.y > overlay_sz.y - 12.0f) {
            pos.y = std::max(12.0f, cutout_rect_.position.y - bubble_sz.y - 12.0f);
        }
        pos.x = std::clamp(pos.x, 12.0f, std::max(12.0f, overlay_sz.x - bubble_sz.x - 12.0f));
        callout_bubble_->set_position(pos);
    }
}

void TutorialSpotlightOverlay::_process(double p_delta) {
    (void)p_delta;
    if (!is_visible()) {
        return;
    }
    _sync_cutout_and_callout();
    queue_redraw();
}

void TutorialSpotlightOverlay::_draw() {
    const Vector2 sz = get_size();
    if (sz.x <= 0.0f || sz.y <= 0.0f) {
        return;
    }

    const Rect2 r = cutout_rect_.intersection(Rect2(Vector2(), sz));
    if (r.size.x <= 0.0f || r.size.y <= 0.0f) {
        draw_rect(Rect2(Vector2(), sz), dimmer_color_, true);
        return;
    }

    // Draw 4 dimmer bands around the cutout for 60+ FPS mask without shaders
    draw_rect(Rect2(0.0f, 0.0f, sz.x, r.position.y), dimmer_color_, true);
    draw_rect(Rect2(0.0f, r.position.y + r.size.y, sz.x, sz.y - (r.position.y + r.size.y)), dimmer_color_, true);
    draw_rect(Rect2(0.0f, r.position.y, r.position.x, r.size.y), dimmer_color_, true);
    draw_rect(
        Rect2(r.position.x + r.size.x, r.position.y, sz.x - (r.position.x + r.size.x), r.size.y),
        dimmer_color_,
        true
    );
    draw_rect(r, border_color_, false, 2.0f);
}

bool TutorialSpotlightOverlay::_has_point(const Vector2 &p_point) const {
    // Allow clicks inside the highlighted cutout to pass through directly to the target Control.
    if (cutout_rect_.has_point(p_point)) {
        return false;
    }
    return Rect2(Vector2(), get_size()).has_point(p_point);
}

void TutorialSpotlightOverlay::spotlight_control(
    Control *p_target,
    const String &p_title,
    const String &p_message,
    const String &p_coach_hint
) {
    tracked_control_ = p_target;
    if (callout_bubble_) {
        callout_bubble_->set_callout_content(p_title, p_message, p_coach_hint);
    }
    set_visible(true);
    set_process(true);
    _sync_cutout_and_callout();
    queue_redraw();
}

void TutorialSpotlightOverlay::spotlight_rect(
    const Rect2 &p_global_rect,
    const String &p_title,
    const String &p_message,
    const String &p_coach_hint
) {
    tracked_control_ = nullptr;
    cutout_rect_ = Rect2(p_global_rect.position - get_global_position(), p_global_rect.size);
    if (callout_bubble_) {
        callout_bubble_->set_callout_content(p_title, p_message, p_coach_hint);
    }
    set_visible(true);
    set_process(true);
    _sync_cutout_and_callout();
    queue_redraw();
}

void TutorialSpotlightOverlay::clear_spotlight() {
    tracked_control_ = nullptr;
    cutout_rect_     = Rect2();
    set_visible(false);
    set_process(false);
    emit_signal("spotlight_cleared");
}

void TutorialSpotlightOverlay::_on_callout_dismissed() {
    clear_spotlight();
}

} // namespace ideam::godot_ext
