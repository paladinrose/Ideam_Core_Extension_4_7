#pragma once

/**
 * spline_canvas_base.h
 * Generalized 2D interactive curve, fader, and vertex canvas (`godot::Control`)
 * in `src/godot/splines/ui/`.
 *
 * Designed as the common visual engine for:
 *   1. `Spline1DEqualizerCanvas`: X = normalized cumulative section weight [0, 1],
 *      Y = 1D value, `DRAG_AXIS_Y_ONLY`, `POINT_STYLE_FADER_CAP`, bottom weight strip.
 *   2. `Spline2DCanvas`: X = 2D world X, Y = 2D world Y, `DRAG_AXIS_FREE_2D`,
 *      `POINT_STYLE_CIRCLE`, `lock_aspect_ratio = true`, free 2D pan & zoom.
 *   3. `Polygon2DCanvas`: 2D vertices (`POINT_STYLE_SQUARE`), `wrap = true`, no tangent
 *      handles, ear-clipped polygon fill in `_draw_plot_background_overlay()`.
 *   4. `GradientSplineCanvas` (`Spline4D` RGBA): X = normalized weight [0, 1],
 *      Y = channel value [0, 1], bottom strip renders the evaluated RGBA color ramp.
 *
 * Centralizes:
 *   - 2D World <-> Screen coordinate projection with optional aspect-ratio locking.
 *   - Per-axis configurable Pan (Middle-mouse / Space+Left-drag) and Zoom (Mouse Wheel).
 *   - Adaptive background grid, origin axes (`X = 0`, `Y = 0`), and margin tick labels.
 *   - Unified hit-testing and dragging for Anchor Points, Back/Fore Tangent Handles,
 *     and subclass-defined custom grips, with single-step Undo/Redo snapshot commits.
 */

#include "../spline_resource.h"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace ideam::godot_ext {

/**
 * Constrains how anchor points move when dragged in the 2D canvas.
 */
enum SplineCanvasAxisConstraint : int32_t {
    DRAG_AXIS_FREE_2D = 0, // Free X/Y movement (Spline2DCanvas, Polygon2DCanvas)
    DRAG_AXIS_X_ONLY  = 1, // Horizontal-only movement (Pure timeline/weight sliders)
    DRAG_AXIS_Y_ONLY  = 2  // Vertical-only movement (Spline1DEqualizerCanvas, 4D channel curves)
};

/**
 * Visual rendering style for anchor points on the canvas.
 */
enum SplineCanvasPointStyle : int32_t {
    POINT_STYLE_CIRCLE    = 0, // Circular Bezier/Catmull-Rom control point (Spline2D)
    POINT_STYLE_SQUARE    = 1, // Crisp square vertex (Polygon2D)
    POINT_STYLE_FADER_CAP = 2  // Graphic-equalizer horizontal slider cap + vertical slot track (Spline1D)
};

class SplineCanvasBase : public godot::Control {
    GDCLASS(SplineCanvasBase, godot::Control)

protected:
    enum BaseDragState : int32_t {
        BASE_DRAG_NONE = 0,
        BASE_DRAG_PAN_VIEW,
        BASE_DRAG_POINT,
        BASE_DRAG_BACK_HANDLE,
        BASE_DRAG_FORE_HANDLE,
        BASE_DRAG_CUSTOM // Delegated to subclass (e.g. section weight divider or threshold line)
    };

    static void _bind_methods();

    godot::Ref<SplineResource> spline_resource;

    // Viewport World Bounds & Margins
    godot::Vector2 view_min = godot::Vector2(0.0f, -0.2f);
    godot::Vector2 view_max = godot::Vector2(1.0f, 1.2f);

    float margin_left   = 44.0f;
    float margin_right  = 18.0f;
    float margin_top    = 14.0f;
    float margin_bottom = 22.0f;

    // Camera & Constraint Flags
    bool lock_aspect_ratio = false;
    bool allow_pan_x       = false;
    bool allow_pan_y       = true;
    bool allow_zoom_x      = false;
    bool allow_zoom_y      = true;
    bool auto_fit_on_change = false;
    bool show_grid         = true;
    bool show_origin_axes  = true;
    bool show_point_labels = true;

    SplineCanvasAxisConstraint anchor_drag_constraint = DRAG_AXIS_FREE_2D;
    SplineCanvasPointStyle point_style                = POINT_STYLE_CIRCLE;
    float snap_step                                   = 0.05f;
    float point_hit_radius                            = 10.0f;
    float handle_hit_radius                           = 8.0f;

    // Theme Colors
    godot::Color bg_color          = godot::Color(0.11f, 0.12f, 0.15f, 1.0f);
    godot::Color plot_bg_color     = godot::Color(0.14f, 0.16f, 0.20f, 1.0f);
    godot::Color grid_color        = godot::Color(0.24f, 0.27f, 0.33f, 0.65f);
    godot::Color origin_axis_color = godot::Color(0.45f, 0.52f, 0.62f, 0.85f);
    godot::Color curve_color       = godot::Color(0.38f, 0.82f, 1.0f, 0.96f);

    // Selection & Hover State
    int selected_point_index   = -1;
    int selected_section_index = -1;
    int hovered_point_index    = -1;
    int hovered_section_index  = -1;

    // Active Drag Transaction State
    BaseDragState base_drag_state = BASE_DRAG_NONE;
    int drag_target_index         = -1;
    int custom_drag_submode       = 0;
    godot::Vector2 drag_start_mouse;
    godot::Vector2 pan_start_view_min;
    godot::Vector2 pan_start_view_max;
    godot::Dictionary drag_before_snapshot;
    bool drag_modified            = false;
    godot::String active_drag_action_name = "Edit Spline Canvas";

    // --- Internal Helpers ---
    void _on_bound_resource_changed();
    void _begin_snapshot_drag(
        BaseDragState p_state,
        int p_target_index,
        const godot::Vector2 &p_mouse_pos,
        const godot::String &p_action_name,
        int p_custom_submode = 0
    );
    void _end_snapshot_drag();

    void _draw_grid_and_axes(const godot::Rect2 &p_plot);
    void _draw_handles_and_points(const godot::Rect2 &p_plot);

    // ========================================================================
    // Virtual Data-Projection & Customization Hooks for Subclasses
    // ========================================================================

    [[nodiscard]] virtual int _get_canvas_point_count() const;
    [[nodiscard]] virtual int _get_canvas_section_count() const;
    [[nodiscard]] virtual bool _is_canvas_wrapped() const;

    /**
     * Maps point `p_index` to 2D canvas world coordinates:
     *   - In 1D Equalizer: `Vector2(cumulative_weight_norm[p_index], value[p_index])`
     *   - In 2D Spline/Polygon: `Vector2(pos.x, pos.y)`
     *   - In 4D Gradient: `Vector2(cumulative_weight_norm[p_index], channel_val[p_index])`
     */
    [[nodiscard]] virtual godot::Vector2 _get_point_world_pos(int p_index) const = 0;
    virtual void _set_point_world_pos(int p_index, const godot::Vector2 &p_world_pos) = 0;

    /**
     * Optional ghost/wrapped endpoint rendering (e.g., showing P0 at X = 1.0 in wrapped 1D mode).
     */
    [[nodiscard]] virtual bool _has_wrap_ghost_endpoint(godot::Vector2 &r_ghost_world_pos) const {
        (void)r_ghost_world_pos;
        return false;
    }

    // --- Back / Fore Tangent Handles in 2D World Coordinates (or Screen Offset) ---
    [[nodiscard]] virtual bool _has_back_handle(int p_index) const;
    [[nodiscard]] virtual godot::Vector2 _get_back_handle_screen_pos(
        int p_index,
        const godot::Rect2 &p_plot
    ) const;
    virtual void _set_back_handle_from_screen_pos(
        int p_index,
        const godot::Vector2 &p_screen_pos,
        const godot::Rect2 &p_plot
    ) {}

    [[nodiscard]] virtual bool _has_fore_handle(int p_index) const;
    [[nodiscard]] virtual godot::Vector2 _get_fore_handle_screen_pos(
        int p_index,
        const godot::Rect2 &p_plot
    ) const;
    virtual void _set_fore_handle_from_screen_pos(
        int p_index,
        const godot::Vector2 &p_screen_pos,
        const godot::Rect2 &p_plot
    ) {}

    // --- Sampled Polyline & Bounds ---
    [[nodiscard]] virtual godot::PackedVector2Array _sample_curve_screen_points(
        const godot::Rect2 &p_plot
    ) const = 0;
    [[nodiscard]] virtual bool _compute_world_bounds(godot::Rect2 &r_bounds) const;

    // --- Structural Mouse Actions ---
    virtual void _on_double_click_plot(
        const godot::Vector2 &p_world_pos,
        const godot::Vector2 &p_screen_pos,
        const godot::Rect2 &p_plot
    ) {}
    virtual void _on_right_click_point(int p_index);

    // --- Custom Subclass Overlays & Custom Drag Hooks ---
    virtual void _draw_plot_background_overlay(const godot::Rect2 &p_plot) {}
    virtual void _draw_plot_foreground_overlay(const godot::Rect2 &p_plot) {}
    [[nodiscard]] virtual bool _try_begin_custom_drag(
        const godot::Ref<godot::InputEvent> &p_event,
        const godot::Rect2 &p_plot
    ) {
        return false;
    }
    [[nodiscard]] virtual bool _update_custom_drag(
        const godot::Ref<godot::InputEvent> &p_event,
        const godot::Rect2 &p_plot
    ) {
        return false;
    }
    virtual void _update_custom_hover(
        const godot::Vector2 &p_mouse_pos,
        const godot::Rect2 &p_plot
    ) {}
    virtual void _on_resource_bound() {}

public:
    SplineCanvasBase();
    ~SplineCanvasBase() override;

    void _draw() override;
    void _gui_input(const godot::Ref<godot::InputEvent> &p_event) override;

    // --- Resource Binding ---
    void set_spline_resource(const godot::Ref<SplineResource> &p_resource);
    [[nodiscard]] godot::Ref<SplineResource> get_spline_resource() const { return spline_resource; }

    // --- Coordinate Transformations (Respecting `lock_aspect_ratio`) ---
    [[nodiscard]] godot::Rect2 get_plot_rect() const;
    [[nodiscard]] godot::Vector2 world_to_screen(
        const godot::Vector2 &p_world,
        const godot::Rect2 &p_plot
    ) const;
    [[nodiscard]] godot::Vector2 screen_to_world(
        const godot::Vector2 &p_screen,
        const godot::Rect2 &p_plot
    ) const;

    // --- Viewport Bounds, Pan, Zoom & Auto-Fit ---
    void set_view_bounds(const godot::Vector2 &p_min, const godot::Vector2 &p_max);
    [[nodiscard]] godot::Vector2 get_view_min() const noexcept { return view_min; }
    [[nodiscard]] godot::Vector2 get_view_max() const noexcept { return view_max; }

    void set_plot_margins(float p_left, float p_right, float p_top, float p_bottom);
    void set_pan_zoom_axes(bool p_pan_x, bool p_pan_y, bool p_zoom_x, bool p_zoom_y);

    void set_lock_aspect_ratio(bool p_lock);
    [[nodiscard]] bool get_lock_aspect_ratio() const noexcept { return lock_aspect_ratio; }

    void set_auto_fit_on_change(bool p_auto_fit);
    [[nodiscard]] bool get_auto_fit_on_change() const noexcept { return auto_fit_on_change; }

    void fit_view_to_data(
        bool p_fit_x = true,
        bool p_fit_y = true,
        real_t p_padding_ratio = static_cast<real_t>(0.15)
    );

    // --- Visual & Interaction Mode Configuration ---
    void set_anchor_drag_constraint(SplineCanvasAxisConstraint p_constraint);
    [[nodiscard]] SplineCanvasAxisConstraint get_anchor_drag_constraint() const noexcept {
        return anchor_drag_constraint;
    }

    void set_point_style(SplineCanvasPointStyle p_style);
    [[nodiscard]] SplineCanvasPointStyle get_point_style() const noexcept { return point_style; }

    void set_show_grid(bool p_show);
    [[nodiscard]] bool get_show_grid() const noexcept { return show_grid; }

    // --- Selection ---
    void set_selected_point_index(int p_index);
    [[nodiscard]] int get_selected_point_index() const noexcept { return selected_point_index; }

    void set_selected_section_index(int p_index);
    [[nodiscard]] int get_selected_section_index() const noexcept { return selected_section_index; }
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::SplineCanvasAxisConstraint);
VARIANT_ENUM_CAST(ideam::godot_ext::SplineCanvasPointStyle);