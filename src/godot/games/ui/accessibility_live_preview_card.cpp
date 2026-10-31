#include "accessibility_live_preview_card.h"

#include <cmath>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/rect2.hpp>

namespace ideam::godot_ext {

void AccessibilityLivePreviewCard::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("set_accessibility_option", "option"), &AccessibilityLivePreviewCard::set_accessibility_option);
    godot::ClassDB::bind_method(godot::D_METHOD("get_accessibility_option"), &AccessibilityLivePreviewCard::get_accessibility_option);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "accessibility_option", godot::PROPERTY_HINT_RESOURCE_TYPE, "GameplayAccessibilityOption"), "set_accessibility_option", "get_accessibility_option");

    godot::ClassDB::bind_method(godot::D_METHOD("set_sample_speaker_name", "speaker"), &AccessibilityLivePreviewCard::set_sample_speaker_name);
    godot::ClassDB::bind_method(godot::D_METHOD("get_sample_speaker_name"), &AccessibilityLivePreviewCard::get_sample_speaker_name);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING, "sample_speaker_name"), "set_sample_speaker_name", "get_sample_speaker_name");

    godot::ClassDB::bind_method(godot::D_METHOD("set_sample_caption_text", "text"), &AccessibilityLivePreviewCard::set_sample_caption_text);
    godot::ClassDB::bind_method(godot::D_METHOD("get_sample_caption_text"), &AccessibilityLivePreviewCard::get_sample_caption_text);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING, "sample_caption_text"), "set_sample_caption_text", "get_sample_caption_text");

    godot::ClassDB::bind_method(godot::D_METHOD("set_show_cymatic_sound_ring", "show"), &AccessibilityLivePreviewCard::set_show_cymatic_sound_ring);
    godot::ClassDB::bind_method(godot::D_METHOD("get_show_cymatic_sound_ring"), &AccessibilityLivePreviewCard::get_show_cymatic_sound_ring);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "show_cymatic_sound_ring"), "set_show_cymatic_sound_ring", "get_show_cymatic_sound_ring");

    godot::ClassDB::bind_method(
        godot::D_METHOD("simulate_accessible_color", "input", "mode", "cvd_strength", "brightness", "contrast", "saturation"),
        &AccessibilityLivePreviewCard::simulate_accessible_color,
        DEFVAL(1.0f),
        DEFVAL(1.0f),
        DEFVAL(1.0f)
    );

    godot::ClassDB::bind_method(godot::D_METHOD("_on_accessibility_changed"), &AccessibilityLivePreviewCard::_on_accessibility_changed);
}

AccessibilityLivePreviewCard::AccessibilityLivePreviewCard() {
    set_clip_contents(true);
}

AccessibilityLivePreviewCard::~AccessibilityLivePreviewCard() {
}

godot::Vector2 AccessibilityLivePreviewCard::_get_minimum_size() const {
    return godot::Vector2(320.0f, 170.0f);
}

godot::Color AccessibilityLivePreviewCard::simulate_accessible_color(
    const godot::Color &p_input,
    ColorVisionMode p_mode,
    float p_cvd_strength,
    float p_brightness,
    float p_contrast,
    float p_saturation
) {
    // 1. Apply Brightness, Contrast, and Saturation
    float r = (p_input.r - 0.5f) * p_contrast + 0.5f;
    float g = (p_input.g - 0.5f) * p_contrast + 0.5f;
    float b = (p_input.b - 0.5f) * p_contrast + 0.5f;

    r *= p_brightness;
    g *= p_brightness;
    b *= p_brightness;

    float luma = 0.2126f * r + 0.7152f * g + 0.0722f * b;
    r = godot::Math::lerp(luma, r, p_saturation);
    g = godot::Math::lerp(luma, g, p_saturation);
    b = godot::Math::lerp(luma, b, p_saturation);

    // 2. Apply Color Vision Mode Daltonization / Simulation Matrix
    float tr = r;
    float tg = g;
    float tb = b;

    switch (p_mode) {
        case ColorVisionMode::CVD_PROTANOPIA:
            tr = 0.56667f * r + 0.43333f * g;
            tg = 0.55833f * r + 0.44167f * g;
            tb = 0.24167f * g + 0.75833f * b;
            break;
        case ColorVisionMode::CVD_DEUTERANOPIA:
            tr = 0.625f * r + 0.375f * g;
            tg = 0.70f * r + 0.30f * g;
            tb = 0.30f * g + 0.70f * b;
            break;
        case ColorVisionMode::CVD_TRITANOPIA:
            tr = 0.95f * r + 0.05f * g;
            tg = 0.43333f * g + 0.56667f * b;
            tb = 0.475f * g + 0.525f * b;
            break;
        case ColorVisionMode::CVD_ACHROMATOPSIA:
            tr = luma;
            tg = luma;
            tb = luma;
            break;
        case ColorVisionMode::CVD_NONE:
        case ColorVisionMode::CVD_CUSTOM_LUT:
        default:
            break;
    }

    float s = godot::Math::clamp(p_cvd_strength, 0.0f, 1.0f);
    return godot::Color(
        godot::Math::clamp(godot::Math::lerp(r, tr, s), 0.0f, 1.0f),
        godot::Math::clamp(godot::Math::lerp(g, tg, s), 0.0f, 1.0f),
        godot::Math::clamp(godot::Math::lerp(b, tb, s), 0.0f, 1.0f),
        p_input.a
    );
}

void AccessibilityLivePreviewCard::set_accessibility_option(const godot::Ref<GameplayAccessibilityOption> &p_option) {
    if (accessibility_option == p_option) return;

    if (accessibility_option.is_valid() && accessibility_option->is_connected("changed", godot::Callable(this, "_on_accessibility_changed"))) {
        accessibility_option->disconnect("changed", godot::Callable(this, "_on_accessibility_changed"));
    }
    accessibility_option = p_option;
    if (accessibility_option.is_valid() && !accessibility_option->is_connected("changed", godot::Callable(this, "_on_accessibility_changed"))) {
        accessibility_option->connect("changed", godot::Callable(this, "_on_accessibility_changed"));
    }
    queue_redraw();
}
godot::Ref<GameplayAccessibilityOption> AccessibilityLivePreviewCard::get_accessibility_option() const { return accessibility_option; }

void AccessibilityLivePreviewCard::set_sample_speaker_name(const godot::String &p_speaker) {
    sample_speaker_name = p_speaker;
    queue_redraw();
}
godot::String AccessibilityLivePreviewCard::get_sample_speaker_name() const { return sample_speaker_name; }

void AccessibilityLivePreviewCard::set_sample_caption_text(const godot::String &p_text) {
    sample_caption_text = p_text;
    queue_redraw();
}
godot::String AccessibilityLivePreviewCard::get_sample_caption_text() const { return sample_caption_text; }

void AccessibilityLivePreviewCard::set_show_cymatic_sound_ring(bool p_show) {
    show_cymatic_sound_ring = p_show;
    queue_redraw();
}
bool AccessibilityLivePreviewCard::get_show_cymatic_sound_ring() const { return show_cymatic_sound_ring; }

void AccessibilityLivePreviewCard::_on_accessibility_changed() {
    queue_redraw();
}

void AccessibilityLivePreviewCard::_notification(int p_what) {
    if (p_what == NOTIFICATION_THEME_CHANGED || p_what == NOTIFICATION_RESIZED) {
        queue_redraw();
        return;
    }
    if (p_what != NOTIFICATION_DRAW) {
        return;
    }

    ColorVisionMode cvd = accessibility_option.is_valid() ? accessibility_option->get_color_vision_mode() : ColorVisionMode::CVD_NONE;
    float cvd_str = accessibility_option.is_valid() ? accessibility_option->get_color_vision_strength() : 1.0f;
    float bright = accessibility_option.is_valid() ? accessibility_option->get_brightness_adjustment() : 1.0f;
    float contr = accessibility_option.is_valid() ? accessibility_option->get_contrast_adjustment() : 1.0f;
    float sat = accessibility_option.is_valid() ? accessibility_option->get_saturation_adjustment() : 1.0f;

    bool outlines = accessibility_option.is_valid() ? accessibility_option->get_high_contrast_entity_outlines() : false;
    godot::Color enemy_col = accessibility_option.is_valid() ? accessibility_option->get_enemy_highlight_color() : godot::Color(1.0f, 0.2f, 0.2f, 1.0f);
    godot::Color inter_col = accessibility_option.is_valid() ? accessibility_option->get_interactable_highlight_color() : godot::Color(0.2f, 0.9f, 1.0f, 1.0f);
    godot::Color haz_col = accessibility_option.is_valid() ? accessibility_option->get_hazard_highlight_color() : godot::Color(1.0f, 0.85f, 0.1f, 1.0f);

    bool captions_on = accessibility_option.is_valid() ? accessibility_option->get_closed_captions_enabled() : true;
    float cap_scale = accessibility_option.is_valid() ? accessibility_option->get_caption_text_scale() : 1.0f;
    float cap_bg_alpha = accessibility_option.is_valid() ? accessibility_option->get_caption_background_opacity() : 0.75f;
    bool cap_speaker = accessibility_option.is_valid() ? accessibility_option->get_caption_speaker_labels() : true;
    bool sound_ind = accessibility_option.is_valid() ? accessibility_option->get_visual_sound_indicators() : false;
    float reticle_sc = accessibility_option.is_valid() ? accessibility_option->get_reticle_scale() : 1.0f;

    auto xform = [&](const godot::Color &c) {
        return simulate_accessible_color(c, cvd, cvd_str, bright, contr, sat);
    };

    godot::Vector2 sz = get_size();
    godot::Rect2 canvas_rect(4.0f, 4.0f, sz.x - 8.0f, sz.y - 8.0f);

    // 1. Simulated Viewport Sky & Ground Backdrop
    draw_rect(canvas_rect, xform(godot::Color(0.14f, 0.18f, 0.24f, 1.0f)), true);
    godot::Rect2 ground_rect(canvas_rect.position.x, canvas_rect.position.y + canvas_rect.size.y * 0.45f, canvas_rect.size.x, canvas_rect.size.y * 0.55f);
    draw_rect(ground_rect, xform(godot::Color(0.20f, 0.23f, 0.22f, 1.0f)), true);

    // 2. Three Entity Silhouettes: Enemy (Left), Interactable (Center), Hazard (Right)
    struct EntitySample {
        const char *label;
        float norm_x;
        godot::Color base_fill;
        godot::Color outline_col;
    };
    EntitySample samples[3] = {
        {"Enemy (RedRobot)", 0.22f, godot::Color(0.68f, 0.24f, 0.24f, 1.0f), enemy_col},
        {"Interactable", 0.50f, godot::Color(0.24f, 0.58f, 0.72f, 1.0f), inter_col},
        {"Hazard", 0.78f, godot::Color(0.75f, 0.60f, 0.18f, 1.0f), haz_col}
    };

    godot::Ref<godot::Font> font = get_theme_default_font();
    int base_font_size = get_theme_default_font_size() > 0 ? get_theme_default_font_size() - 2 : 12;

    for (int i = 0; i < 3; ++i) {
        godot::Vector2 center(
            canvas_rect.position.x + canvas_rect.size.x * samples[i].norm_x,
            canvas_rect.position.y + canvas_rect.size.y * 0.44f
        );
        godot::Rect2 body(center.x - 24.0f, center.y - 26.0f, 48.0f, 52.0f);
        draw_rect(body, xform(samples[i].base_fill), true);
        if (outlines) {
            draw_rect(body.grow(2.5f), xform(samples[i].outline_col), false, 3.0f);
        }
        if (font.is_valid()) {
            draw_string(
                font,
                godot::Vector2(center.x - 50.0f, body.position.y - 6.0f),
                samples[i].label,
                godot::HORIZONTAL_ALIGNMENT_CENTER,
                100.0f,
                base_font_size,
                godot::Color(0.95f, 0.96f, 0.98f, 1.0f)
            );
        }
    }

    // 3. Center Reticle & Optional Cymatic Visual Sound Indicator Arc
    godot::Vector2 hud_center(canvas_rect.position.x + canvas_rect.size.x * 0.50f, canvas_rect.position.y + canvas_rect.size.y * 0.44f);
    float ret_r = 6.0f * godot::Math::clamp(reticle_sc, 0.5f, 2.5f);
    draw_arc(hud_center, ret_r, 0.0f, Math_TAU, 24, godot::Color(1.0f, 1.0f, 1.0f, 0.9f), 1.5f);

    if (show_cymatic_sound_ring && sound_ind) {
        draw_arc(hud_center, 38.0f, Math_PI * 0.75f, Math_PI * 1.25f, 24, xform(enemy_col), 4.0f);
    }

    // 4. Closed Caption Preview Box at Bottom
    if (captions_on && font.is_valid()) {
        float cap_h = godot::Math::clamp(34.0f * cap_scale, 26.0f, canvas_rect.size.y * 0.38f);
        godot::Rect2 cap_rect(
            canvas_rect.position.x + 16.0f,
            canvas_rect.position.y + canvas_rect.size.y - cap_h - 8.0f,
            canvas_rect.size.x - 32.0f,
            cap_h
        );
        draw_rect(cap_rect, godot::Color(0.04f, 0.04f, 0.06f, godot::Math::clamp(cap_bg_alpha, 0.0f, 1.0f)), true);

        godot::String full_line = cap_speaker && !sample_speaker_name.is_empty()
            ? ("[" + sample_speaker_name + "]: " + sample_caption_text)
            : sample_caption_text;
        int cap_font_sz = godot::Math::clamp(static_cast<int>(std::round(static_cast<float>(base_font_size) * cap_scale)), 10, 26);

        draw_string(
            font,
            godot::Vector2(cap_rect.position.x + 8.0f, cap_rect.position.y + cap_rect.size.y * 0.66f),
            full_line,
            godot::HORIZONTAL_ALIGNMENT_CENTER,
            cap_rect.size.x - 16.0f,
            cap_font_sz,
            godot::Color(0.98f, 0.98f, 0.94f, 1.0f)
        );
    }

    draw_rect(canvas_rect, godot::Color(1.0f, 1.0f, 1.0f, 0.18f), false, 1.25f);
}

} // namespace ideam::godot_ext