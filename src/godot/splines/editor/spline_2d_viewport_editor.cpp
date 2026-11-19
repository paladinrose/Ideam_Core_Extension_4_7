#include "spline_2d_viewport_editor.h"

#include "../ideam_splines_plugin.h"

#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/v_separator.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/color.hpp>

#include <algorithm>
#include <cmath>

using namespace godot;

namespace ideam::godot_ext {

void Spline2DViewportEditor::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_mode_selected", "mode"), &Spline2DViewportEditor::_on_mode_selected);
    ClassDB::bind_method(D_METHOD("_on_toggle_handles_pressed"), &Spline2DViewportEditor::_on_toggle_handles_pressed);
    ClassDB::bind_method(D_METHOD("_on_toggle_wrap_pressed"), &Spline2DViewportEditor::_on_toggle_wrap_pressed);
    ClassDB::bind_method(D_METHOD("_on_live_toggled", "live"), &Spline2DViewportEditor::_on_live_toggled);
    ClassDB::bind_method(D_METHOD("_on_active_spline_changed"), &Spline2DViewportEditor::_on_active_spline_changed);
}

Spline2DViewportEditor::Spline2DViewportEditor() {
    set_visible(false);

    auto make_mode_btn = [&](const String &p_text, const String &p_tip, ToolMode p_mode) -> Button * {
        Button *b = memnew(Button);
        b->set_text(p_text);
        b->set_tooltip_text(p_tip);
        b->set_toggle_mode(true);
        b->set_flat(true);
        b->connect("pressed", Callable(this, "_on_mode_selected").bind(static_cast<int>(p_mode)));
        add_child(b);
        return b;
    };

    btn_transform_ = make_mode_btn(
        "Transform",
        "Use Godot's native Select / Move / Rotate / Scale tools (direct click on control points still drags them).",
        MODE_TRANSFORM
    );
    btn_edit_ = make_mode_btn(
        "Edit Points",
        "Select and drag spline anchors or Bezier handles; double-click curve to split; right-click anchor to delete.",
        MODE_EDIT_POINTS
    );
    btn_add_ = make_mode_btn(
        "+ Point",
        "Left-click in the 2D viewport to append points or split an existing curve segment.",
        MODE_ADD_POINT
    );
    btn_delete_ = make_mode_btn(
        "- Point",
        "Left-click an anchor point in the 2D viewport to remove it.",
        MODE_DELETE_POINT
    );

    add_child(memnew(VSeparator));

    btn_handles_ = memnew(Button);
    btn_handles_->set_text("Handles");
    btn_handles_->set_tooltip_text("Toggle Bezier back/fore handles on the selected anchor point.");
    btn_handles_->set_flat(true);
    btn_handles_->connect("pressed", Callable(this, "_on_toggle_handles_pressed"));
    add_child(btn_handles_);

    btn_wrap_ = memnew(Button);
    btn_wrap_->set_text("Closed");
    btn_wrap_->set_tooltip_text("Toggle closed/wrapped loop on the active Spline2DResource.");
    btn_wrap_->set_toggle_mode(true);
    btn_wrap_->set_flat(true);
    btn_wrap_->connect("pressed", Callable(this, "_on_toggle_wrap_pressed"));
    add_child(btn_wrap_);

    add_child(memnew(VSeparator));

    btn_live_ = memnew(Button);
    btn_live_->set_text("On Release");
    btn_live_->set_tooltip_text("Unchecked: UPDATE_ON_COMMIT (preview drag, 1 Undo action on release). Checked: live updates while dragging.");
    btn_live_->set_toggle_mode(true);
    btn_live_->set_flat(true);
    btn_live_->connect("toggled", Callable(this, "_on_live_toggled"));
    add_child(btn_live_);

    _sync_toolbar_state();
}

Spline2DViewportEditor::~Spline2DViewportEditor() {
    _disconnect_active_spline();
}

// ============================================================================
// Coordinate Space & Resource Resolution
// ============================================================================

Ref<Spline2DResource> Spline2DViewportEditor::_resolve_spline_from_node(Node2D *p_node) const {
    if (!p_node) {
        return Ref<Spline2DResource>();
    }
    if (auto *sn = Object::cast_to<SplineNode2D>(p_node)) {
        return sn->get_spline();
    }
    if (auto *sp = Object::cast_to<SplinePolygon2D>(p_node)) {
        return sp->get_spline();
    }
    return Ref<Spline2DResource>();
}

Transform2D Spline2DViewportEditor::_get_node_to_overlay_transform() const {
    if (!edited_node_ || !edited_node_->is_inside_tree()) {
        return Transform2D();
    }
    return edited_node_->get_viewport_transform() * edited_node_->get_global_transform();
}

Vector2 Spline2DViewportEditor::_spline_to_local(const Vector2 &p_spline_pt) const {
    if (auto *sn = Object::cast_to<SplineNode2D>(edited_node_)) {
        return (p_spline_pt * sn->get_output_scale()) + sn->get_output_offset();
    }
    return p_spline_pt;
}

Vector2 Spline2DViewportEditor::_local_to_spline(const Vector2 &p_local_pt) const {
    if (auto *sn = Object::cast_to<SplineNode2D>(edited_node_)) {
        const Vector2 scale  = sn->get_output_scale();
        const Vector2 offset = sn->get_output_offset();
        const float sx = (std::abs(scale.x) > 1e-5f) ? scale.x : 1.0f;
        const float sy = (std::abs(scale.y) > 1e-5f) ? scale.y : 1.0f;
        return Vector2((p_local_pt.x - offset.x) / sx, (p_local_pt.y - offset.y) / sy);
    }
    return p_local_pt;
}

Vector2 Spline2DViewportEditor::_spline_to_overlay(const Vector2 &p_spline_pt) const {
    return _get_node_to_overlay_transform().xform(_spline_to_local(p_spline_pt));
}

Vector2 Spline2DViewportEditor::_overlay_to_spline(const Vector2 &p_overlay_pt) const {
    const Vector2 local_pt = _get_node_to_overlay_transform().affine_inverse().xform(p_overlay_pt);
    return _local_to_spline(local_pt);
}

void Spline2DViewportEditor::_ensure_undo_redo_injected() const {
    if (active_spline_.is_null()) {
        return;
    }
#ifdef TOOLS_ENABLED
    if (EditorUndoRedoManager *ed_ur = IdeamSplinesPlugin::get_static_undo_redo()) {
        Ref<IdeamUndoRedo> ur = active_spline_->get_undo_redo();
        if (ur.is_valid()) {
            ur->set_editor_undo_redo(ed_ur);
        }
    }
#endif
}

void Spline2DViewportEditor::_request_overlay_redraw() {
    if (owner_plugin_) {
        owner_plugin_->update_overlays();
    }
}

void Spline2DViewportEditor::_disconnect_active_spline() {
    if (active_spline_.is_valid()) {
        const Callable cb(this, "_on_active_spline_changed");
        if (active_spline_->is_connected("changed", cb)) {
            active_spline_->disconnect("changed", cb);
        }
        active_spline_.unref();
    }
}

void Spline2DViewportEditor::_sync_toolbar_state() {
    if (btn_transform_) btn_transform_->set_pressed_no_signal(tool_mode_ == MODE_TRANSFORM);
    if (btn_edit_)      btn_edit_->set_pressed_no_signal(tool_mode_ == MODE_EDIT_POINTS);
    if (btn_add_)       btn_add_->set_pressed_no_signal(tool_mode_ == MODE_ADD_POINT);
    if (btn_delete_)    btn_delete_->set_pressed_no_signal(tool_mode_ == MODE_DELETE_POINT);

    const bool has_spline = active_spline_.is_valid();
    if (btn_wrap_) {
        btn_wrap_->set_disabled(!has_spline);
        btn_wrap_->set_pressed_no_signal(has_spline && active_spline_->get_wrap());
    }
    if (btn_handles_) {
        btn_handles_->set_disabled(!has_spline || selected_point_index_ < 0);
    }
    if (btn_live_) {
        const bool is_live = (update_commit_policy_ == UPDATE_IMMEDIATE);
        btn_live_->set_pressed_no_signal(is_live);
        btn_live_->set_text(is_live ? "Live" : "On Release");
    }
}

// ============================================================================
// EditorPlugin Lifecycle Bridges
// ============================================================================

bool Spline2DViewportEditor::handles_object(Object *p_object) const {
    if (!p_object) {
        return false;
    }
    return Object::cast_to<SplineNode2D>(p_object) != nullptr ||
           Object::cast_to<SplinePolygon2D>(p_object) != nullptr;
}

void Spline2DViewportEditor::edit_object(Object *p_object) {
    _disconnect_active_spline();
    edited_node_          = Object::cast_to<Node2D>(p_object);
    selected_point_index_ = -1;
    hovered_point_index_  = -1;
    drag_kind_            = DRAG_NONE;
    drag_target_index_    = -1;
    drag_modified_        = false;

    if (edited_node_) {
        active_spline_ = _resolve_spline_from_node(edited_node_);
        if (active_spline_.is_valid()) {
            const Callable cb(this, "_on_active_spline_changed");
            if (!active_spline_->is_connected("changed", cb)) {
                active_spline_->connect("changed", cb);
            }
            _ensure_undo_redo_injected();
        }
    }

    _sync_toolbar_state();
    _request_overlay_redraw();
}

void Spline2DViewportEditor::set_editor_visible(bool p_visible) {
    set_visible(p_visible);
    if (!p_visible) {
        drag_kind_         = DRAG_NONE;
        drag_target_index_ = -1;
        drag_modified_     = false;
    }
    _request_overlay_redraw();
}

// ============================================================================
// UPDATE_ON_COMMIT Drag Transaction Helpers
// ============================================================================

void Spline2DViewportEditor::_begin_drag(DragKind p_kind, int p_index, const String &p_action_name) {
    drag_kind_               = p_kind;
    drag_target_index_       = p_index;
    drag_modified_           = false;
    active_drag_action_name_ = p_action_name;

    if (active_spline_.is_valid()) {
        _ensure_undo_redo_injected();
        drag_before_snapshot_ = active_spline_->get_snapshot_state();
        if (p_kind == DRAG_POINT) {
            drag_preview_spline_pos_ = active_spline_->get_point_position(p_index);
        } else if (p_kind == DRAG_BACK_HANDLE) {
            drag_preview_spline_pos_ = active_spline_->get_point_position(p_index) + active_spline_->get_point_back_handle(p_index);
        } else if (p_kind == DRAG_FORE_HANDLE) {
            drag_preview_spline_pos_ = active_spline_->get_point_position(p_index) + active_spline_->get_point_fore_handle(p_index);
        }
    }
}

void Spline2DViewportEditor::_end_drag() {
    if (drag_kind_ != DRAG_NONE && drag_modified_ && active_spline_.is_valid() && drag_target_index_ >= 0) {
        if (update_commit_policy_ != UPDATE_IMMEDIATE) {
            if (drag_kind_ == DRAG_POINT) {
                active_spline_->set_point_position(drag_target_index_, drag_preview_spline_pos_);
            } else if (drag_kind_ == DRAG_BACK_HANDLE) {
                const Vector2 anchor = active_spline_->get_point_position(drag_target_index_);
                active_spline_->set_point_back_handle(drag_target_index_, drag_preview_spline_pos_ - anchor, true);
            } else if (drag_kind_ == DRAG_FORE_HANDLE) {
                const Vector2 anchor = active_spline_->get_point_position(drag_target_index_);
                active_spline_->set_point_fore_handle(drag_target_index_, drag_preview_spline_pos_ - anchor, true);
            }
        }
        _ensure_undo_redo_injected();
        const Dictionary after_state = active_spline_->get_snapshot_state();
        active_spline_->commit_snapshot_action(active_drag_action_name_, drag_before_snapshot_, after_state);
    }

    drag_kind_         = DRAG_NONE;
    drag_target_index_ = -1;
    drag_modified_     = false;
    _request_overlay_redraw();
}

void Spline2DViewportEditor::_insert_or_append_point_at_overlay(const Vector2 &p_overlay_pos) {
    if (active_spline_.is_null()) {
        return;
    }
    _ensure_undo_redo_injected();
    const Vector2 world_pt = _overlay_to_spline(p_overlay_pos);
    const int pt_count     = active_spline_->get_point_count();
    const int sec_count    = active_spline_->get_section_count();

    if (pt_count >= 2 && sec_count > 0) {
        const Vector2 closest_w   = active_spline_->get_closest_point_on_spline(world_pt, 16);
        const Vector2 closest_scr = _spline_to_overlay(closest_w);
        if (p_overlay_pos.distance_to(closest_scr) <= CURVE_SPLIT_DIST_PX) {
            const real_t pct        = active_spline_->get_closest_percent_on_spline(world_pt, 16, true);
            const Vector2 sec_info  = active_spline_->get_section_at_percent(pct, true);
            const int target_sec    = std::clamp(static_cast<int>(sec_info.x), 0, sec_count - 1);
            const SplineCurveType t = active_spline_->get_section_curve_type(target_sec);
            active_spline_->commit_insert_point(target_sec + 1, world_pt, t);
            selected_point_index_ = target_sec + 1;
            _sync_toolbar_state();
            _request_overlay_redraw();
            return;
        }
    }

    active_spline_->commit_add_point(world_pt);
    selected_point_index_ = active_spline_->get_point_count() - 1;
    _sync_toolbar_state();
    _request_overlay_redraw();
}

// ============================================================================
// 2D Viewport Input Forwarding
// ============================================================================

bool Spline2DViewportEditor::forward_canvas_gui_input(const Ref<InputEvent> &p_event) {
    if (!edited_node_ || p_event.is_null()) {
        return false;
    }

    // Re-sync in case a spline resource was assigned in the Inspector after selection
    Ref<Spline2DResource> current_spline = _resolve_spline_from_node(edited_node_);
    if (current_spline != active_spline_) {
        edit_object(edited_node_);
    }
    if (active_spline_.is_null()) {
        return false;
    }

    const int pt_count = active_spline_->get_point_count();

    // Keyboard Delete removes selected point when in Edit Points mode
    if (Ref<InputEventKey> key = p_event; key.is_valid() && key->is_pressed() && !key->is_echo()) {
        if ((key->get_keycode() == KEY_DELETE || key->get_keycode() == KEY_BACKSPACE) &&
            tool_mode_ != MODE_TRANSFORM &&
            selected_point_index_ >= 0 && selected_point_index_ < pt_count) {
            _ensure_undo_redo_injected();
            active_spline_->commit_remove_point(selected_point_index_);
            selected_point_index_ = std::min(selected_point_index_, active_spline_->get_point_count() - 1);
            _sync_toolbar_state();
            _request_overlay_redraw();
            return true;
        }
    }

    if (Ref<InputEventMouseButton> mb = p_event; mb.is_valid()) {
        const Vector2 mpos     = mb->get_position();
        const MouseButton bidx = mb->get_button_index();

        if (bidx == MOUSE_BUTTON_LEFT && !mb->is_pressed() && drag_kind_ != DRAG_NONE) {
            _end_drag();
            return true;
        }

        if (bidx == MOUSE_BUTTON_LEFT && mb->is_pressed()) {
            // 1. Delete Mode
            if (tool_mode_ == MODE_DELETE_POINT) {
                for (int i = 0; i < pt_count; ++i) {
                    if (mpos.distance_to(_spline_to_overlay(active_spline_->get_point_position(i))) <= POINT_HIT_RADIUS_PX) {
                        _ensure_undo_redo_injected();
                        active_spline_->commit_remove_point(i);
                        if (selected_point_index_ == i) selected_point_index_ = -1;
                        _sync_toolbar_state();
                        _request_overlay_redraw();
                        return true;
                    }
                }
                return true;
            }

            // 2. Add Point Mode
            if (tool_mode_ == MODE_ADD_POINT) {
                _insert_or_append_point_at_overlay(mpos);
                return true;
            }

            // 3. Double-click in Edit Points mode splits/appends
            if (tool_mode_ == MODE_EDIT_POINTS && mb->is_double_click()) {
                _insert_or_append_point_at_overlay(mpos);
                return true;
            }

            // 4. Hit-test Bezier handles first (works in both Transform and Edit Points modes)
            for (int i = 0; i < pt_count; ++i) {
                const Vector2 anchor = active_spline_->get_point_position(i);
                if (active_spline_->get_point_use_back_handle(i)) {
                    const Vector2 bh_scr = _spline_to_overlay(anchor + active_spline_->get_point_back_handle(i));
                    if (mpos.distance_to(bh_scr) <= HANDLE_HIT_RADIUS_PX) {
                        selected_point_index_ = i;
                        _begin_drag(DRAG_BACK_HANDLE, i, "Drag 2D Spline Back Handle");
                        _sync_toolbar_state();
                        _request_overlay_redraw();
                        return true;
                    }
                }
                if (active_spline_->get_point_use_fore_handle(i)) {
                    const Vector2 fh_scr = _spline_to_overlay(anchor + active_spline_->get_point_fore_handle(i));
                    if (mpos.distance_to(fh_scr) <= HANDLE_HIT_RADIUS_PX) {
                        selected_point_index_ = i;
                        _begin_drag(DRAG_FORE_HANDLE, i, "Drag 2D Spline Fore Handle");
                        _sync_toolbar_state();
                        _request_overlay_redraw();
                        return true;
                    }
                }
            }

            // 5. Hit-test anchor points (works in both Transform and Edit Points modes)
            for (int i = 0; i < pt_count; ++i) {
                const Vector2 pt_scr = _spline_to_overlay(active_spline_->get_point_position(i));
                if (mpos.distance_to(pt_scr) <= POINT_HIT_RADIUS_PX) {
                    selected_point_index_ = i;
                    _begin_drag(DRAG_POINT, i, "Move 2D Spline Point");
                    _sync_toolbar_state();
                    _request_overlay_redraw();
                    return true;
                }
            }

            // In MODE_TRANSFORM, let Godot's native Select / Move / Rotate / Scale gizmo handle clicks!
            return false;
        }

        if (bidx == MOUSE_BUTTON_RIGHT && mb->is_pressed() && tool_mode_ == MODE_EDIT_POINTS) {
            for (int i = 0; i < pt_count; ++i) {
                if (mpos.distance_to(_spline_to_overlay(active_spline_->get_point_position(i))) <= POINT_HIT_RADIUS_PX) {
                    _ensure_undo_redo_injected();
                    active_spline_->commit_remove_point(i);
                    if (selected_point_index_ == i) selected_point_index_ = -1;
                    _sync_toolbar_state();
                    _request_overlay_redraw();
                    return true;
                }
            }
        }
    }

    if (Ref<InputEventMouseMotion> mm = p_event; mm.is_valid()) {
        const Vector2 mpos = mm->get_position();

        if (drag_kind_ != DRAG_NONE && drag_target_index_ >= 0 && drag_target_index_ < pt_count) {
            Vector2 target_w = _overlay_to_spline(mpos);
            if (mm->is_shift_pressed()) {
                constexpr float SNAP = 8.0f;
                target_w.x = std::round(target_w.x / SNAP) * SNAP;
                target_w.y = std::round(target_w.y / SNAP) * SNAP;
            }

            if (update_commit_policy_ == UPDATE_IMMEDIATE) {
                if (drag_kind_ == DRAG_POINT) {
                    active_spline_->set_point_position(drag_target_index_, target_w);
                } else if (drag_kind_ == DRAG_BACK_HANDLE) {
                    const Vector2 anchor = active_spline_->get_point_position(drag_target_index_);
                    active_spline_->set_point_back_handle(drag_target_index_, target_w - anchor, true);
                } else if (drag_kind_ == DRAG_FORE_HANDLE) {
                    const Vector2 anchor = active_spline_->get_point_position(drag_target_index_);
                    active_spline_->set_point_fore_handle(drag_target_index_, target_w - anchor, true);
                }
            } else {
                drag_preview_spline_pos_ = target_w;
            }

            drag_modified_ = true;
            _request_overlay_redraw();
            return true;
        }

        int new_hov = -1;
        for (int i = 0; i < pt_count; ++i) {
            if (mpos.distance_to(_spline_to_overlay(active_spline_->get_point_position(i))) <= POINT_HIT_RADIUS_PX) {
                new_hov = i;
                break;
            }
        }
        if (new_hov != hovered_point_index_) {
            hovered_point_index_ = new_hov;
            _request_overlay_redraw();
        }
    }

    return false;
}

// ============================================================================
// 2D Viewport Overlay Drawing
// ============================================================================

void Spline2DViewportEditor::forward_canvas_draw_over_viewport(Control *p_overlay) {
    if (!p_overlay || !edited_node_ || active_spline_.is_null()) {
        return;
    }

    const int pt_count = active_spline_->get_point_count();
    if (pt_count <= 0) {
        return;
    }

    const Ref<Font> font = p_overlay->get_theme_default_font();
    const bool previewing = (update_commit_policy_ != UPDATE_IMMEDIATE) &&
                            drag_kind_ != DRAG_NONE && drag_modified_ &&
                            drag_target_index_ >= 0 && drag_target_index_ < pt_count;

    // 1. Preview segments & tether during UPDATE_ON_COMMIT drag
    if (previewing) {
        const Color preview_col(1.0f, 0.78f, 0.24f, 0.95f);
        const Color tether_col(1.0f, 0.78f, 0.24f, 0.50f);
        const Vector2 orig_scr = _spline_to_overlay(active_spline_->get_point_position(drag_target_index_));
        const Vector2 prev_scr = _spline_to_overlay(drag_preview_spline_pos_);

        if (drag_kind_ == DRAG_POINT) {
            p_overlay->draw_dashed_line(orig_scr, prev_scr, tether_col, 1.5f, 5.0f);
            if (pt_count >= 2) {
                const bool wrapped = active_spline_->get_wrap();
                const int prev_i = (drag_target_index_ > 0) ? (drag_target_index_ - 1) : (wrapped ? pt_count - 1 : -1);
                const int next_i = (drag_target_index_ + 1 < pt_count) ? (drag_target_index_ + 1) : (wrapped ? 0 : -1);
                if (prev_i >= 0) {
                    p_overlay->draw_line(_spline_to_overlay(active_spline_->get_point_position(prev_i)), prev_scr, preview_col, 2.0f, true);
                }
                if (next_i >= 0) {
                    p_overlay->draw_line(prev_scr, _spline_to_overlay(active_spline_->get_point_position(next_i)), preview_col, 2.0f, true);
                }
            }
        } else {
            p_overlay->draw_line(orig_scr, prev_scr, preview_col, 2.0f, true);
            p_overlay->draw_circle(prev_scr, 5.5f, preview_col);
        }
    }

    // 2. Bezier Handles & Anchor Points
    for (int i = 0; i < pt_count; ++i) {
        const Vector2 anchor_w = active_spline_->get_point_position(i);
        const Vector2 scr      = _spline_to_overlay(anchor_w);
        const bool is_sel      = (i == selected_point_index_);
        const bool is_hov      = (i == hovered_point_index_);

        if (active_spline_->get_point_use_back_handle(i)) {
            const Vector2 bh_scr = _spline_to_overlay(anchor_w + active_spline_->get_point_back_handle(i));
            p_overlay->draw_line(scr, bh_scr, Color(0.95f, 0.52f, 0.36f, 0.90f), 1.5f, true);
            p_overlay->draw_circle(bh_scr, 4.5f, Color(0.95f, 0.52f, 0.36f, 1.0f));
        }
        if (active_spline_->get_point_use_fore_handle(i)) {
            const Vector2 fh_scr = _spline_to_overlay(anchor_w + active_spline_->get_point_fore_handle(i));
            p_overlay->draw_line(scr, fh_scr, Color(0.46f, 0.92f, 0.56f, 0.90f), 1.5f, true);
            p_overlay->draw_circle(fh_scr, 4.5f, Color(0.46f, 0.92f, 0.56f, 1.0f));
        }

        const bool is_dragged_pt = previewing && drag_kind_ == DRAG_POINT && i == drag_target_index_;
        const Color fill = is_dragged_pt
            ? Color(0.55f, 0.62f, 0.72f, 0.55f)
            : (is_sel ? Color(1.0f, 0.82f, 0.28f, 1.0f)
                      : (is_hov ? Color(0.62f, 0.88f, 1.0f, 1.0f) : Color(0.92f, 0.95f, 1.0f, 0.96f)));

        p_overlay->draw_circle(scr, is_sel ? 7.0f : 5.5f, Color(0.10f, 0.12f, 0.16f, 1.0f));
        p_overlay->draw_circle(scr, is_sel ? 5.2f : 4.0f, fill);

        if (is_dragged_pt) {
            const Vector2 prev_scr = _spline_to_overlay(drag_preview_spline_pos_);
            p_overlay->draw_circle(prev_scr, 7.0f, Color(0.10f, 0.12f, 0.16f, 1.0f));
            p_overlay->draw_circle(prev_scr, 5.2f, Color(1.0f, 0.82f, 0.28f, 1.0f));
        }

        if (font.is_valid()) {
            p_overlay->draw_string(
                font,
                scr + Vector2(8.0f, -6.0f),
                String("P") + String::num_int64(i),
                HORIZONTAL_ALIGNMENT_LEFT,
                -1.0f,
                11,
                is_sel ? Color(1.0f, 0.86f, 0.36f, 1.0f) : Color(0.80f, 0.86f, 0.94f, 0.85f)
            );
        }
    }
}

// ============================================================================
// Toolbar Callbacks
// ============================================================================

void Spline2DViewportEditor::_on_mode_selected(int p_mode) {
    tool_mode_ = static_cast<ToolMode>(std::clamp(p_mode, 0, 3));
    _sync_toolbar_state();
    _request_overlay_redraw();
}

void Spline2DViewportEditor::_on_toggle_handles_pressed() {
    if (active_spline_.is_null() || selected_point_index_ < 0 ||
        selected_point_index_ >= active_spline_->get_point_count()) {
        return;
    }
    _ensure_undo_redo_injected();
    const bool currently_on = active_spline_->get_point_use_back_handle(selected_point_index_) ||
                              active_spline_->get_point_use_fore_handle(selected_point_index_);
    const Dictionary before_state = active_spline_->get_snapshot_state();
    if (currently_on) {
        active_spline_->set_point_use_back_handle(selected_point_index_, false);
        active_spline_->set_point_use_fore_handle(selected_point_index_, false);
    } else {
        Vector2 bh = active_spline_->get_point_back_handle(selected_point_index_);
        Vector2 fh = active_spline_->get_point_fore_handle(selected_point_index_);
        if (bh.length_squared() < 1e-4f) bh = Vector2(-24.0f, 0.0f);
        if (fh.length_squared() < 1e-4f) fh = Vector2(24.0f, 0.0f);
        active_spline_->set_point_handles(selected_point_index_, bh, true, fh, true);
    }
    active_spline_->commit_snapshot_action(
        currently_on ? "Disable 2D Spline Handles" : "Enable 2D Spline Handles",
        before_state,
        active_spline_->get_snapshot_state()
    );
    _request_overlay_redraw();
}

void Spline2DViewportEditor::_on_toggle_wrap_pressed() {
    if (active_spline_.is_null()) {
        return;
    }
    _ensure_undo_redo_injected();
    active_spline_->commit_set_wrap(!active_spline_->get_wrap());
    _sync_toolbar_state();
    _request_overlay_redraw();
}

void Spline2DViewportEditor::_on_live_toggled(bool p_live) {
    update_commit_policy_ = p_live ? UPDATE_IMMEDIATE : UPDATE_ON_COMMIT;
    _sync_toolbar_state();
}

void Spline2DViewportEditor::_on_active_spline_changed() {
    _sync_toolbar_state();
    _request_overlay_redraw();
}

} // namespace ideam::godot_ext
