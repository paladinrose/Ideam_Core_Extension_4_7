#include "ideam_spatial_toolbar.h"

#include "../tools/domain/domain_tool_interface.h"

#include <godot_cpp/classes/v_separator.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

namespace ideam::godot_ext {

void IdeamSpatialToolbar::_bind_methods() {
    using namespace godot;

    ADD_SIGNAL(MethodInfo("mode_changed", PropertyInfo(Variant::STRING_NAME, "mode_id")));
    ADD_SIGNAL(MethodInfo(
        "toggle_changed",
        PropertyInfo(Variant::STRING_NAME, "toggle_id"),
        PropertyInfo(Variant::BOOL, "pressed")
    ));

    ClassDB::bind_method(D_METHOD("set_tool_context", "context"), &IdeamSpatialToolbar::set_tool_context);
    ClassDB::bind_method(D_METHOD("get_tool_context"), &IdeamSpatialToolbar::get_tool_context);
    ClassDB::bind_method(D_METHOD("clear_tools"), &IdeamSpatialToolbar::clear_tools);
    ClassDB::bind_method(
        D_METHOD("add_mode_button", "id", "label", "tooltip", "ui_selector"),
        &IdeamSpatialToolbar::add_mode_button,
        DEFVAL(String()),
        DEFVAL(String())
    );
    ClassDB::bind_method(
        D_METHOD("add_toggle_button", "id", "label", "tooltip", "default_toggled", "ui_selector"),
        &IdeamSpatialToolbar::add_toggle_button,
        DEFVAL(String()),
        DEFVAL(true),
        DEFVAL(String())
    );
    ClassDB::bind_method(D_METHOD("add_separator"), &IdeamSpatialToolbar::add_separator);
    ClassDB::bind_method(
        D_METHOD("set_active_mode", "mode_id", "emit_signal"),
        &IdeamSpatialToolbar::set_active_mode,
        DEFVAL(true)
    );
    ClassDB::bind_method(D_METHOD("get_active_mode"), &IdeamSpatialToolbar::get_active_mode);
    ClassDB::bind_method(
        D_METHOD("set_toggle_state", "toggle_id", "pressed", "emit_signal"),
        &IdeamSpatialToolbar::set_toggle_state,
        DEFVAL(true)
    );
    ClassDB::bind_method(D_METHOD("get_toggle_state", "toggle_id"), &IdeamSpatialToolbar::get_toggle_state);

    ClassDB::bind_method(D_METHOD("_on_mode_button_pressed", "id"), &IdeamSpatialToolbar::_on_mode_button_pressed);
    ClassDB::bind_method(D_METHOD("_on_toggle_button_toggled", "pressed", "id"), &IdeamSpatialToolbar::_on_toggle_button_toggled);
}

IdeamSpatialToolbar::IdeamSpatialToolbar() {
    set_name("IdeamSpatialToolbar");
    add_theme_constant_override("separation", 4);
}

void IdeamSpatialToolbar::set_tool_context(const godot::Ref<ToolContext> &p_context) {
    tool_context_ = p_context;
}

void IdeamSpatialToolbar::clear_tools() {
    entries_.clear();
    active_mode_id_ = godot::StringName();
    while (get_child_count() > 0) {
        godot::Node *child = get_child(0);
        remove_child(child);
        memdelete(child);
    }
}

void IdeamSpatialToolbar::add_tool_spec(const ToolModeSpec &p_spec) {
    auto *btn = memnew(godot::Button);
    btn->set_text(p_spec.label);
    btn->set_tooltip_text(p_spec.tooltip);
    btn->set_toggle_mode(true);
    btn->set_flat(false);
    btn->set_focus_mode(godot::Control::FOCUS_NONE);

    if (!p_spec.ui_selector.is_empty()) {
        domain_tool::stamp_ui_selector(btn, p_spec.ui_selector);
    }

    if (p_spec.is_toggle) {
        btn->set_pressed_no_signal(p_spec.default_toggled);
        btn->connect("toggled", godot::Callable(this, "_on_toggle_button_toggled").bind(p_spec.id));
    } else {
        const bool is_first_mode = active_mode_id_ == godot::StringName();
        if (is_first_mode) {
            active_mode_id_ = p_spec.id;
            btn->set_pressed_no_signal(true);
        }
        btn->connect("pressed", godot::Callable(this, "_on_mode_button_pressed").bind(p_spec.id));
    }

    add_child(btn);
    entries_.push_back({p_spec, btn});
}

void IdeamSpatialToolbar::add_mode_button(
    const godot::StringName &p_id,
    const godot::String &p_label,
    const godot::String &p_tooltip,
    const godot::String &p_ui_selector
) {
    ToolModeSpec spec{};
    spec.id          = p_id;
    spec.label       = p_label;
    spec.tooltip     = p_tooltip;
    spec.is_toggle   = false;
    spec.ui_selector = p_ui_selector;
    add_tool_spec(spec);
}

void IdeamSpatialToolbar::add_toggle_button(
    const godot::StringName &p_id,
    const godot::String &p_label,
    const godot::String &p_tooltip,
    bool p_default_toggled,
    const godot::String &p_ui_selector
) {
    ToolModeSpec spec{};
    spec.id              = p_id;
    spec.label           = p_label;
    spec.tooltip         = p_tooltip;
    spec.is_toggle       = true;
    spec.default_toggled = p_default_toggled;
    spec.ui_selector     = p_ui_selector;
    add_tool_spec(spec);
}

void IdeamSpatialToolbar::add_separator() {
    auto *sep = memnew(godot::VSeparator);
    add_child(sep);
}

void IdeamSpatialToolbar::_on_mode_button_pressed(const godot::StringName &p_id) {
    if (updating_buttons_) return;
    set_active_mode(p_id, true);
}

void IdeamSpatialToolbar::_on_toggle_button_toggled(bool p_pressed, const godot::StringName &p_id) {
    if (updating_buttons_) return;
    emit_signal("toggle_changed", p_id, p_pressed);
}

void IdeamSpatialToolbar::set_active_mode(const godot::StringName &p_mode_id, bool p_emit_signal) {
    updating_buttons_ = true;
    active_mode_id_   = p_mode_id;
    for (auto &e : entries_) {
        if (!e.spec.is_toggle && e.button) {
            e.button->set_pressed_no_signal(e.spec.id == active_mode_id_);
        }
    }
    updating_buttons_ = false;
    if (p_emit_signal) {
        emit_signal("mode_changed", active_mode_id_);
    }
}

void IdeamSpatialToolbar::set_toggle_state(const godot::StringName &p_toggle_id, bool p_pressed, bool p_emit_signal) {
    updating_buttons_ = true;
    for (auto &e : entries_) {
        if (e.spec.is_toggle && e.spec.id == p_toggle_id && e.button) {
            e.button->set_pressed_no_signal(p_pressed);
        }
    }
    updating_buttons_ = false;
    if (p_emit_signal) {
        emit_signal("toggle_changed", p_toggle_id, p_pressed);
    }
}

bool IdeamSpatialToolbar::get_toggle_state(const godot::StringName &p_toggle_id) const {
    for (const auto &e : entries_) {
        if (e.spec.is_toggle && e.spec.id == p_toggle_id && e.button) {
            return e.button->is_pressed();
        }
    }
    return false;
}

} // namespace ideam::godot_ext
