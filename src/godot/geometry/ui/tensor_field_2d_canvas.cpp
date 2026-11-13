#include "godot/geometry/ui/tensor_field_2d_canvas.h"
#include <godot_cpp/core/class_db.hpp>
#include <algorithm>

namespace godot {

static void draw_tensor_field_common(
    CanvasItem *p_canvas,
    Ref<TensorField2DResource> p_tf,
    bool p_draw_cross,
    bool p_draw_major,
    bool p_draw_minor,
    bool p_draw_singularities,
    bool p_draw_parcels,
    int p_stride,
    const Color &p_fill_color,
    const Color &p_major_color,
    const Color &p_minor_color,
    const Color &p_cross_color,
    float p_line_width
) {
    if (p_tf.is_null()) return;

    if (p_draw_parcels) {
        TypedArray<PackedVector2Array> parcels = p_tf->get_closed_parcel_polygons();
        for (int i = 0; i < parcels.size(); ++i) {
            PackedVector2Array poly = parcels[i];
            if (poly.size() >= 3) {
                Color c = p_fill_color;
                c.h = std::fmod(c.h + static_cast<float>(i) * 0.13f, 1.0f);
                p_canvas->draw_colored_polygon(poly, c);
            }
        }
    }

    if (p_draw_cross) {
        const auto hdr = p_tf->get_core_grid_header();
        const auto &maj = p_tf->get_core_major_eigenvectors();
        const auto &min_v = p_tf->get_core_minor_eigenvectors();
        const int stride = std::max(1, p_stride);
        const float half_len = std::min(hdr.cell_size_x, hdr.cell_size_y) * static_cast<float>(stride) * 0.35f;

        for (uint32_t gy = 0; gy < hdr.height; gy += static_cast<uint32_t>(stride)) {
            for (uint32_t gx = 0; gx < hdr.width; gx += static_cast<uint32_t>(stride)) {
                const size_t idx = static_cast<size_t>(gy) * hdr.width + gx;
                if (idx >= maj.size()) continue;
                const glm::vec2 wp = hdr.grid_to_world(static_cast<float>(gx), static_cast<float>(gy));
                const Vector2 center(wp.x, wp.y);
                const Vector2 v_maj(maj[idx].x * half_len, maj[idx].y * half_len);
                const Vector2 v_min(min_v[idx].x * half_len, min_v[idx].y * half_len);
                p_canvas->draw_line(center - v_maj, center + v_maj, p_cross_color, 1.0f);
                p_canvas->draw_line(center - v_min, center + v_min, p_cross_color, 1.0f);
            }
        }
    }

    if (p_draw_minor) {
        TypedArray<PackedVector2Array> minors = p_tf->get_minor_streamlines();
        for (int i = 0; i < minors.size(); ++i) {
            PackedVector2Array pl = minors[i];
            if (pl.size() >= 2) {
                p_canvas->draw_polyline(pl, p_minor_color, p_line_width * 0.85f);
            }
        }
    }

    if (p_draw_major) {
        TypedArray<PackedVector2Array> majors = p_tf->get_major_streamlines();
        for (int i = 0; i < majors.size(); ++i) {
            PackedVector2Array pl = majors[i];
            if (pl.size() >= 2) {
                p_canvas->draw_polyline(pl, p_major_color, p_line_width * 1.15f);
            }
        }
    }

    if (p_draw_singularities) {
        const auto &sings = p_tf->get_core_singularities();
        for (const auto &s : sings) {
            Color sc = Color(1.0f, 0.35f, 0.35f, 1.0f); // Wedge (+1/2)
            if (s.singularity_type == static_cast<uint32_t>(ideam::geometry::TensorSingularityType::SINGULARITY_TRISECTOR)) {
                sc = Color(0.35f, 0.95f, 0.55f, 1.0f); // Trisector (-1/2)
            } else if (s.singularity_type == static_cast<uint32_t>(ideam::geometry::TensorSingularityType::SINGULARITY_NODE_CENTER)) {
                sc = Color(0.95f, 0.85f, 0.25f, 1.0f); // Center (+1)
            } else if (s.singularity_type == static_cast<uint32_t>(ideam::geometry::TensorSingularityType::SINGULARITY_SADDLE)) {
                sc = Color(0.85f, 0.45f, 1.0f, 1.0f); // Saddle (-1)
            }
            p_canvas->draw_circle(Vector2(s.world_x, s.world_y), 4.5f, sc);
        }
    }
}

void TensorField2DCanvas::set_draw_cross_field(bool p_draw) { draw_cross_field_ = p_draw; queue_redraw(); }
void TensorField2DCanvas::set_draw_major_streamlines(bool p_draw) { draw_major_streamlines_ = p_draw; queue_redraw(); }
void TensorField2DCanvas::set_draw_minor_streamlines(bool p_draw) { draw_minor_streamlines_ = p_draw; queue_redraw(); }
void TensorField2DCanvas::set_draw_singularities(bool p_draw) { draw_singularities_ = p_draw; queue_redraw(); }
void TensorField2DCanvas::set_draw_closed_parcels(bool p_draw) { draw_closed_parcels_ = p_draw; queue_redraw(); }
void TensorField2DCanvas::set_cross_field_stride(int p_stride) { cross_field_stride_ = std::max(1, p_stride); queue_redraw(); }

void TensorField2DCanvas::_draw() {
    Ref<TensorField2DResource> tf = geometry_resource_;
    draw_tensor_field_common(
        this, tf,
        draw_cross_field_, draw_major_streamlines_, draw_minor_streamlines_,
        draw_singularities_, draw_closed_parcels_, cross_field_stride_,
        fill_color_, major_streamline_color_, minor_streamline_color_, cross_field_color_,
        line_width_
    );
}

void TensorField2DCanvas::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_draw_cross_field", "draw"), &TensorField2DCanvas::set_draw_cross_field);
    ClassDB::bind_method(D_METHOD("get_draw_cross_field"), &TensorField2DCanvas::get_draw_cross_field);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "draw_cross_field"), "set_draw_cross_field", "get_draw_cross_field");

    ClassDB::bind_method(D_METHOD("set_draw_major_streamlines", "draw"), &TensorField2DCanvas::set_draw_major_streamlines);
    ClassDB::bind_method(D_METHOD("get_draw_major_streamlines"), &TensorField2DCanvas::get_draw_major_streamlines);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "draw_major_streamlines"), "set_draw_major_streamlines", "get_draw_major_streamlines");

    ClassDB::bind_method(D_METHOD("set_draw_minor_streamlines", "draw"), &TensorField2DCanvas::set_draw_minor_streamlines);
    ClassDB::bind_method(D_METHOD("get_draw_minor_streamlines"), &TensorField2DCanvas::get_draw_minor_streamlines);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "draw_minor_streamlines"), "set_draw_minor_streamlines", "get_draw_minor_streamlines");

    ClassDB::bind_method(D_METHOD("set_draw_singularities", "draw"), &TensorField2DCanvas::set_draw_singularities);
    ClassDB::bind_method(D_METHOD("get_draw_singularities"), &TensorField2DCanvas::get_draw_singularities);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "draw_singularities"), "set_draw_singularities", "get_draw_singularities");

    ClassDB::bind_method(D_METHOD("set_draw_closed_parcels", "draw"), &TensorField2DCanvas::set_draw_closed_parcels);
    ClassDB::bind_method(D_METHOD("get_draw_closed_parcels"), &TensorField2DCanvas::get_draw_closed_parcels);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "draw_closed_parcels"), "set_draw_closed_parcels", "get_draw_closed_parcels");

    ClassDB::bind_method(D_METHOD("set_cross_field_stride", "stride"), &TensorField2DCanvas::set_cross_field_stride);
    ClassDB::bind_method(D_METHOD("get_cross_field_stride"), &TensorField2DCanvas::get_cross_field_stride);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "cross_field_stride", PROPERTY_HINT_RANGE, "1,16,1"), "set_cross_field_stride", "get_cross_field_stride");
}

void TensorField2DControl::set_draw_cross_field(bool p_draw) { draw_cross_field_ = p_draw; queue_redraw(); }
void TensorField2DControl::set_draw_major_streamlines(bool p_draw) { draw_major_streamlines_ = p_draw; queue_redraw(); }
void TensorField2DControl::set_draw_minor_streamlines(bool p_draw) { draw_minor_streamlines_ = p_draw; queue_redraw(); }
void TensorField2DControl::set_draw_singularities(bool p_draw) { draw_singularities_ = p_draw; queue_redraw(); }
void TensorField2DControl::set_draw_closed_parcels(bool p_draw) { draw_closed_parcels_ = p_draw; queue_redraw(); }
void TensorField2DControl::set_cross_field_stride(int p_stride) { cross_field_stride_ = std::max(1, p_stride); queue_redraw(); }

void TensorField2DControl::_draw() {
    Ref<TensorField2DResource> tf = geometry_resource_;
    draw_tensor_field_common(
        this, tf,
        draw_cross_field_, draw_major_streamlines_, draw_minor_streamlines_,
        draw_singularities_, draw_closed_parcels_, cross_field_stride_,
        fill_color_, Color(0.95f, 0.75f, 0.25f, 0.9f), Color(0.25f, 0.78f, 0.95f, 0.75f),
        Color(0.75f, 0.82f, 0.9f, 0.35f), line_width_
    );
}

void TensorField2DControl::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_draw_cross_field", "draw"), &TensorField2DControl::set_draw_cross_field);
    ClassDB::bind_method(D_METHOD("get_draw_cross_field"), &TensorField2DControl::get_draw_cross_field);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "draw_cross_field"), "set_draw_cross_field", "get_draw_cross_field");

    ClassDB::bind_method(D_METHOD("set_draw_major_streamlines", "draw"), &TensorField2DControl::set_draw_major_streamlines);
    ClassDB::bind_method(D_METHOD("get_draw_major_streamlines"), &TensorField2DControl::get_draw_major_streamlines);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "draw_major_streamlines"), "set_draw_major_streamlines", "get_draw_major_streamlines");

    ClassDB::bind_method(D_METHOD("set_draw_minor_streamlines", "draw"), &TensorField2DControl::set_draw_minor_streamlines);
    ClassDB::bind_method(D_METHOD("get_draw_minor_streamlines"), &TensorField2DControl::get_draw_minor_streamlines);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "draw_minor_streamlines"), "set_draw_minor_streamlines", "get_draw_minor_streamlines");

    ClassDB::bind_method(D_METHOD("set_draw_singularities", "draw"), &TensorField2DControl::set_draw_singularities);
    ClassDB::bind_method(D_METHOD("get_draw_singularities"), &TensorField2DControl::get_draw_singularities);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "draw_singularities"), "set_draw_singularities", "get_draw_singularities");

    ClassDB::bind_method(D_METHOD("set_draw_closed_parcels", "draw"), &TensorField2DControl::set_draw_closed_parcels);
    ClassDB::bind_method(D_METHOD("get_draw_closed_parcels"), &TensorField2DControl::get_draw_closed_parcels);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "draw_closed_parcels"), "set_draw_closed_parcels", "get_draw_closed_parcels");

    ClassDB::bind_method(D_METHOD("set_cross_field_stride", "stride"), &TensorField2DControl::set_cross_field_stride);
    ClassDB::bind_method(D_METHOD("get_cross_field_stride"), &TensorField2DControl::get_cross_field_stride);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "cross_field_stride", PROPERTY_HINT_RANGE, "1,16,1"), "set_cross_field_stride", "get_cross_field_stride");
}

} // namespace godot
