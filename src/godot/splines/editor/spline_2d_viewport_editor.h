#pragma once

/**
 * spline_2d_viewport_editor.h
 * 2D Scene-Space Viewport Editor & Canvas Toolbar (`godot::HBoxContainer`) for
 * `SplineNode2D` and `SplinePolygon2D` (`src/godot/splines/editor/`).
 *
 * Mounted into `EditorPlugin::CONTAINER_CANVAS_EDITOR_MENU` by `IdeamSplinesPlugin`.
 * Coexists seamlessly with Godot's native 2D Select / Move / Rotate / Scale tools:
 *   - In `MODE_TRANSFORM` (default), clicks away from control points pass through (`return false`)
 *     to Godot's built-in 2D transform gizmo; clicking a point or handle edits it directly.
 *   - Dedicated Spline Edit modes (`MODE_EDIT_POINTS`, `MODE_ADD_POINT`, `MODE_DELETE_POINT`)
 *     mirror Godot's `Path2D` / `Polygon2D` workflow.
 *   - Uses `UPDATE_ON_COMMIT` by default: previews anchor/handle drags in the 2D overlay with zero
 *     resource writes during mouse motion, committing a single atomic Undo/Redo snapshot on release.
 */

#include "../../controls/ideam_tool_context.h"
#include "../spline_2d/spline_2d_resource.h"
#include "../spline_2d/spline_node2d.h"
#include "../spline_2d/spline_polygon_2d.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace ideam::godot_ext {

class Spline2DViewportEditor : public godot::HBoxContainer {
    GDCLASS(Spline2DViewportEditor, godot::HBoxContainer)

public:
    enum ToolMode : int32_t {
        MODE_TRANSFORM    = 0, // Native Select/Move/Rotate/Scale passthrough + direct point grab
        MODE_EDIT_POINTS  = 1, // Select/Move points & Bezier handles, double-click curve to split, right-click to delete
        MODE_ADD_POINT    = 2, // Left-click anywhere to append or split spline
        MODE_DELETE_POINT = 3  // Left-click any point to remove it
    };

private:
    enum DragKind : int32_t {
        DRAG_NONE = 0,
        DRAG_POINT,
        DRAG_BACK_HANDLE,
        DRAG_FORE_HANDLE
    };

    godot::EditorPlugin *owner_plugin_ = nullptr;
    godot::Node2D *edited_node_        = nullptr;
    godot::Ref<Spline2DResource> active_spline_;

    ToolMode tool_mode_                           = MODE_TRANSFORM;
    IdeamUpdateCommitPolicy update_commit_policy_ = UPDATE_ON_COMMIT;

    // Toolbar Controls
    godot::Button *btn_transform_ = nullptr;
    godot::Button *btn_edit_      = nullptr;
    godot::Button *btn_add_       = nullptr;
    godot::Button *btn_delete_    = nullptr;
    godot::Button *btn_handles_   = nullptr;
    godot::Button *btn_wrap_      = nullptr;
    godot::Button *btn_live_      = nullptr;

    // Selection, Hover & UPDATE_ON_COMMIT Preview Drag State
    int selected_point_index_ = -1;
    int hovered_point_index_  = -1;
    DragKind drag_kind_       = DRAG_NONE;
    int drag_target_index_    = -1;
    bool drag_modified_       = false;
    godot::Vector2 drag_preview_spline_pos_;
    godot::Dictionary drag_before_snapshot_;
    godot::String active_drag_action_name_;

    static constexpr float POINT_HIT_RADIUS_PX  = 11.0f;
    static constexpr float HANDLE_HIT_RADIUS_PX = 9.0f;
    static constexpr float CURVE_SPLIT_DIST_PX  = 16.0f;

    // --- Coordinate Space Helpers ---
    [[nodiscard]] godot::Ref<Spline2DResource> _resolve_spline_from_node(godot::Node2D *p_node) const;
    [[nodiscard]] godot::Transform2D _get_node_to_overlay_transform() const;
    [[nodiscard]] godot::Vector2 _spline_to_local(const godot::Vector2 &p_spline_pt) const;
    [[nodiscard]] godot::Vector2 _local_to_spline(const godot::Vector2 &p_local_pt) const;
    [[nodiscard]] godot::Vector2 _spline_to_overlay(const godot::Vector2 &p_spline_pt) const;
    [[nodiscard]] godot::Vector2 _overlay_to_spline(const godot::Vector2 &p_overlay_pt) const;

    void _sync_toolbar_state();
    void _ensure_undo_redo_injected() const;
    void _request_overlay_redraw();
    void _disconnect_active_spline();
    void _begin_drag(DragKind p_kind, int p_index, const godot::String &p_action_name);
    void _end_drag();
    void _insert_or_append_point_at_overlay(const godot::Vector2 &p_overlay_pos);

protected:
    static void _bind_methods();

public:
    Spline2DViewportEditor();
    ~Spline2DViewportEditor() override;

    void set_owner_plugin(godot::EditorPlugin *p_plugin) { owner_plugin_ = p_plugin; }

    [[nodiscard]] bool handles_object(godot::Object *p_object) const;
    void edit_object(godot::Object *p_object);
    void set_editor_visible(bool p_visible);

    bool forward_canvas_gui_input(const godot::Ref<godot::InputEvent> &p_event);
    void forward_canvas_draw_over_viewport(godot::Control *p_overlay);

    // --- Toolbar Callbacks ---
    void _on_mode_selected(int p_mode);
    void _on_toggle_handles_pressed();
    void _on_toggle_wrap_pressed();
    void _on_live_toggled(bool p_live);
    void _on_active_spline_changed();
};

} // namespace ideam::godot_ext
