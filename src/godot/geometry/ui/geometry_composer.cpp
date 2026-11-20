#include "geometry_composer.h"

#include "geometry_canvas_2d_base.h"
#include "../ideam_geometry_plugin.h"

#include <godot_cpp/classes/v_separator.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

using namespace godot;

namespace ideam::godot_ext {

GeometryComposer *GeometryComposer::shared_instance_ = nullptr;
Window *GeometryComposer::shared_window_             = nullptr;

void GeometryComposer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_fit_view_pressed"), &GeometryComposer::_on_fit_view_pressed);
    ClassDB::bind_method(D_METHOD("_on_grid_toggled", "show"), &GeometryComposer::_on_grid_toggled);
    ClassDB::bind_method(D_METHOD("_on_y_down_toggled", "y_down"), &GeometryComposer::_on_y_down_toggled);

    ClassDB::bind_static_method(
        "GeometryComposer",
        D_METHOD("open_geometry_resource", "resource", "owner"),
        &GeometryComposer::open_geometry_resource,
        DEFVAL(nullptr)
    );
}

GeometryComposer::GeometryComposer() {
    set_workspace_id(StringName(WORKSPACE_ID));

    // Pre-create the three canonical domain sidebar tabs in order
    ensure_sidebar_tab("General");
    ensure_sidebar_tab("Splines");
    ensure_sidebar_tab("Tiles");

    configure_theme_registry("res://addons/ideam_geometry/resources/geometry_composer_theme_registry.tres");

#ifdef TOOLS_ENABLED
    if (tool_context_.is_valid()) {
        if (EditorUndoRedoManager *ed_ur = IdeamGeometryPlugin::get_static_undo_redo()) {
            tool_context_->set_editor_undo_redo(ed_ur);
        }
    }
#endif

    _populate_command_bar(custom_actions_box_);
}

GeometryComposer::~GeometryComposer() {
    if (shared_instance_ == this) {
        shared_instance_ = nullptr;
        shared_window_   = nullptr;
    }
}

void GeometryComposer::_populate_command_bar(HBoxContainer *p_custom_actions) {
    if (!p_custom_actions) {
        return;
    }

    p_custom_actions->add_child(memnew(VSeparator));

    fit_view_btn_ = memnew(Button);
    fit_view_btn_->set_text("Fit View");
    fit_view_btn_->set_tooltip_text("Frame the active 2D canvas around the resource bounds.");
    fit_view_btn_->connect("pressed", Callable(this, "_on_fit_view_pressed"));
    p_custom_actions->add_child(fit_view_btn_);

    grid_btn_ = memnew(Button);
    grid_btn_->set_text("Grid");
    grid_btn_->set_toggle_mode(true);
    grid_btn_->set_pressed_no_signal(true);
    grid_btn_->set_tooltip_text("Toggle background grid visibility on the active 2D canvas.");
    grid_btn_->connect("toggled", Callable(this, "_on_grid_toggled"));
    p_custom_actions->add_child(grid_btn_);

    y_down_btn_ = memnew(Button);
    y_down_btn_->set_text("Y-Down");
    y_down_btn_->set_toggle_mode(true);
    y_down_btn_->set_pressed_no_signal(false);
    y_down_btn_->set_tooltip_text("Toggle between Godot 2D Y-Down and mathematical Y-Up canvas projection.");
    y_down_btn_->connect("toggled", Callable(this, "_on_y_down_toggled"));
    p_custom_actions->add_child(y_down_btn_);

    _sync_canvas_command_buttons();
}

Control *GeometryComposer::_find_active_canvas_recursive(Node *p_root) const {
    if (!p_root) {
        return nullptr;
    }
    if (auto *canvas = Object::cast_to<GeometryCanvas2DBase>(p_root)) {
        return canvas;
    }
    for (int i = 0; i < p_root->get_child_count(); ++i) {
        if (Control *found = _find_active_canvas_recursive(p_root->get_child(i))) {
            return found;
        }
    }
    return nullptr;
}

void GeometryComposer::_sync_canvas_command_buttons() {
    GeometryCanvas2DBase *active_canvas = nullptr;
    for (const auto &entry : open_assets_) {
        if (entry.asset == active_asset_ && entry.tier1_control) {
            active_canvas = Object::cast_to<GeometryCanvas2DBase>(_find_active_canvas_recursive(entry.tier1_control));
            break;
        }
    }

    const bool has_canvas = (active_canvas != nullptr);
    if (fit_view_btn_) fit_view_btn_->set_disabled(!has_canvas);
    if (grid_btn_) {
        grid_btn_->set_disabled(!has_canvas);
        if (has_canvas) {
            grid_btn_->set_pressed_no_signal(active_canvas->get_show_grid());
        }
    }
    if (y_down_btn_) {
        y_down_btn_->set_disabled(!has_canvas);
        if (has_canvas) {
            y_down_btn_->set_pressed_no_signal(active_canvas->get_y_down());
        }
    }
}

void GeometryComposer::_on_active_asset_switched(const Ref<Resource> &p_asset) {
    (void)p_asset;
    _sync_canvas_command_buttons();
}

void GeometryComposer::_on_fit_view_pressed() {
    for (const auto &entry : open_assets_) {
        if (entry.asset == active_asset_ && entry.tier1_control) {
            if (auto *canvas = Object::cast_to<GeometryCanvas2DBase>(_find_active_canvas_recursive(entry.tier1_control))) {
                canvas->fit_view_to_data(true, true);
            }
            return;
        }
    }
}

void GeometryComposer::_on_grid_toggled(bool p_show) {
    for (const auto &entry : open_assets_) {
        if (entry.asset == active_asset_ && entry.tier1_control) {
            if (auto *canvas = Object::cast_to<GeometryCanvas2DBase>(_find_active_canvas_recursive(entry.tier1_control))) {
                canvas->set_show_grid(p_show);
            }
            return;
        }
    }
}

void GeometryComposer::_on_y_down_toggled(bool p_y_down) {
    for (const auto &entry : open_assets_) {
        if (entry.asset == active_asset_ && entry.tier1_control) {
            if (auto *canvas = Object::cast_to<GeometryCanvas2DBase>(_find_active_canvas_recursive(entry.tier1_control))) {
                canvas->set_y_down(p_y_down);
            }
            return;
        }
    }
}

GeometryComposer *GeometryComposer::get_or_create_shared_composer(Control *p_owner) {
    if (p_owner) {
        for (int i = 0; i < p_owner->get_child_count(); ++i) {
            if (auto *existing = Object::cast_to<GeometryComposer>(p_owner->get_child(i))) {
                return existing;
            }
        }
        GeometryComposer *embedded = memnew(GeometryComposer);
        p_owner->add_child(embedded);
        return embedded;
    }

    if (shared_instance_ && shared_window_) {
        shared_window_->show();
        shared_window_->grab_focus();
        return shared_instance_;
    }

    shared_instance_ = memnew(GeometryComposer);
    shared_window_   = ToolWorkspace::create_workspace_window(
        shared_instance_,
        "Ideam GeometryComposer",
        Vector2i(1120, 720)
    );
    return shared_instance_;
}

void GeometryComposer::open_geometry_resource(const Ref<Resource> &p_resource, Control *p_owner) {
    if (p_resource.is_null()) {
        return;
    }
    GeometryComposer *composer = get_or_create_shared_composer(p_owner);
    if (composer) {
        composer->open_asset(p_resource);
    }
}

} // namespace ideam::godot_ext
