#ifndef IDEAM_TENSOR_FIELD_2D_CANVAS_H
#define IDEAM_TENSOR_FIELD_2D_CANVAS_H

#include "godot/geometry/geometry_canvas_base.h"
#include "godot/geometry/resources/tensor_field_2d_resource.h"

namespace godot {

// ============================================================================
// TensorField2DCanvas (Node2D) & TensorField2DControl (Control)
// Visualizes 2D symmetric tensor cross-fields, topological singularities
// (Wedges, Trisectors, Centers, Saddles), RK4 major/minor hyperstreamlines,
// and closed DCEL parcel blocks.
// ============================================================================
class TensorField2DCanvas : public GeometryCanvas2DBase {
    GDCLASS(TensorField2DCanvas, GeometryCanvas2DBase)

private:
    bool draw_cross_field_ = true;
    bool draw_major_streamlines_ = true;
    bool draw_minor_streamlines_ = true;
    bool draw_singularities_ = true;
    bool draw_closed_parcels_ = true;
    int cross_field_stride_ = 4;

    Color major_streamline_color_ = Color(0.95f, 0.75f, 0.25f, 0.9f);
    Color minor_streamline_color_ = Color(0.25f, 0.78f, 0.95f, 0.75f);
    Color cross_field_color_ = Color(0.75f, 0.82f, 0.9f, 0.35f);

protected:
    static void _bind_methods();

public:
    TensorField2DCanvas() = default;
    ~TensorField2DCanvas() override = default;

    void set_draw_cross_field(bool p_draw);
    bool get_draw_cross_field() const { return draw_cross_field_; }

    void set_draw_major_streamlines(bool p_draw);
    bool get_draw_major_streamlines() const { return draw_major_streamlines_; }

    void set_draw_minor_streamlines(bool p_draw);
    bool get_draw_minor_streamlines() const { return draw_minor_streamlines_; }

    void set_draw_singularities(bool p_draw);
    bool get_draw_singularities() const { return draw_singularities_; }

    void set_draw_closed_parcels(bool p_draw);
    bool get_draw_closed_parcels() const { return draw_closed_parcels_; }

    void set_cross_field_stride(int p_stride);
    int get_cross_field_stride() const { return cross_field_stride_; }

    void _draw() override;
};

class TensorField2DControl : public GeometryControl2DBase {
    GDCLASS(TensorField2DControl, GeometryControl2DBase)

private:
    bool draw_cross_field_ = true;
    bool draw_major_streamlines_ = true;
    bool draw_minor_streamlines_ = true;
    bool draw_singularities_ = true;
    bool draw_closed_parcels_ = true;
    int cross_field_stride_ = 4;

protected:
    static void _bind_methods();

public:
    TensorField2DControl() = default;
    ~TensorField2DControl() override = default;

    void set_draw_cross_field(bool p_draw);
    bool get_draw_cross_field() const { return draw_cross_field_; }

    void set_draw_major_streamlines(bool p_draw);
    bool get_draw_major_streamlines() const { return draw_major_streamlines_; }

    void set_draw_minor_streamlines(bool p_draw);
    bool get_draw_minor_streamlines() const { return draw_minor_streamlines_; }

    void set_draw_singularities(bool p_draw);
    bool get_draw_singularities() const { return draw_singularities_; }

    void set_draw_closed_parcels(bool p_draw);
    bool get_draw_closed_parcels() const { return draw_closed_parcels_; }

    void set_cross_field_stride(int p_stride);
    int get_cross_field_stride() const { return cross_field_stride_; }

    void _draw() override;
};

} // namespace godot

#endif // IDEAM_TENSOR_FIELD_2D_CANVAS_H
