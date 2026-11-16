#include "godot/climatology/ui/climatology_synoptic_map_preview_canvas.h"

#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/rect2.hpp>

#include <algorithm>

namespace ideam::godot_ext {

ClimatologySynopticMapPreviewCanvas::ClimatologySynopticMapPreviewCanvas() {
    set_custom_minimum_size(godot::Vector2(260.0f, 260.0f));
    set_mouse_filter(MOUSE_FILTER_STOP);
}

void ClimatologySynopticMapPreviewCanvas::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_atmosphere_resource", "resource"), &ClimatologySynopticMapPreviewCanvas::set_atmosphere_resource);
    ClassDB::bind_method(D_METHOD("get_atmosphere_resource"), &ClimatologySynopticMapPreviewCanvas::get_atmosphere_resource);

    ClassDB::bind_method(D_METHOD("set_active_map_layer", "layer"), &ClimatologySynopticMapPreviewCanvas::set_active_map_layer);
    ClassDB::bind_method(D_METHOD("get_active_map_layer"), &ClimatologySynopticMapPreviewCanvas::get_active_map_layer);

    ClassDB::bind_method(D_METHOD("set_selected_cell", "cell"), &ClimatologySynopticMapPreviewCanvas::set_selected_cell);
    ClassDB::bind_method(D_METHOD("get_selected_cell"), &ClimatologySynopticMapPreviewCanvas::get_selected_cell);

    ClassDB::bind_method(D_METHOD("rebuild_preview_texture"), &ClimatologySynopticMapPreviewCanvas::rebuild_preview_texture);

    ADD_SIGNAL(MethodInfo("cell_selected", PropertyInfo(Variant::VECTOR2I, "grid_cell")));
}

void ClimatologySynopticMapPreviewCanvas::set_atmosphere_resource(const godot::Ref<ClimatologyAtmosphereResource>& p_resource) {
    atmosphere_resource_ = p_resource;
    rebuild_preview_texture();
}

godot::Ref<ClimatologyAtmosphereResource> ClimatologySynopticMapPreviewCanvas::get_atmosphere_resource() const {
    return atmosphere_resource_;
}

void ClimatologySynopticMapPreviewCanvas::set_active_map_layer(int p_layer) {
    active_map_layer_ = std::clamp(p_layer, 0, static_cast<int>(core::ClimatologyMapLayer::COUNT) - 1);
    rebuild_preview_texture();
}

int ClimatologySynopticMapPreviewCanvas::get_active_map_layer() const {
    return active_map_layer_;
}

void ClimatologySynopticMapPreviewCanvas::set_selected_cell(const godot::Vector2i& p_cell) {
    selected_cell_ = p_cell;
    queue_redraw();
}

godot::Vector2i ClimatologySynopticMapPreviewCanvas::get_selected_cell() const {
    return selected_cell_;
}

void ClimatologySynopticMapPreviewCanvas::rebuild_preview_texture() {
    if (atmosphere_resource_.is_valid()) {
        godot::Ref<godot::Image> img = atmosphere_resource_->render_map_layer_image(active_map_layer_);
        if (img.is_valid()) {
            preview_texture_ = godot::ImageTexture::create_from_image(img);
        }
    } else {
        preview_texture_.unref();
    }
    queue_redraw();
}

void ClimatologySynopticMapPreviewCanvas::_gui_input(const godot::Ref<godot::InputEvent>& p_event) {
    godot::Ref<godot::InputEventMouseButton> mb = p_event;
    if (mb.is_valid() && mb->is_pressed() && atmosphere_resource_.is_valid() && atmosphere_resource_->get_seed_profile().is_valid()) {
        const auto seed = atmosphere_resource_->get_seed_profile();
        const godot::Vector2 sz = get_size();
        if (sz.x > 1.0f && sz.y > 1.0f) {
            const godot::Vector2 pos = mb->get_position();
            const int gx = std::clamp(static_cast<int>((pos.x / sz.x) * seed->get_grid_width()), 0, seed->get_grid_width() - 1);
            const int gy = std::clamp(static_cast<int>((pos.y / sz.y) * seed->get_grid_height()), 0, seed->get_grid_height() - 1);
            selected_cell_ = godot::Vector2i(gx, gy);
            if (mb->get_button_index() == godot::MOUSE_BUTTON_RIGHT) {
                atmosphere_resource_->spawn_tornado_at_cell(gx, gy);
                rebuild_preview_texture();
            }
            emit_signal("cell_selected", selected_cell_);
            queue_redraw();
        }
    }
}

void ClimatologySynopticMapPreviewCanvas::_draw() {
    using namespace godot;

    const Vector2 sz = get_size();
    draw_rect(Rect2(Vector2(0, 0), sz), Color(0.08f, 0.10f, 0.14f, 1.0f), true);

    if (preview_texture_.is_valid()) {
        draw_texture_rect(preview_texture_, Rect2(Vector2(0, 0), sz), false);
    }

    if (atmosphere_resource_.is_valid() && atmosphere_resource_->get_seed_profile().is_valid()) {
        const auto seed = atmosphere_resource_->get_seed_profile();
        const float cell_w = sz.x / static_cast<float>(std::max(1, seed->get_grid_width()));
        const float cell_h = sz.y / static_cast<float>(std::max(1, seed->get_grid_height()));

        // Draw active tornado vortex markers
        for (const auto& v : atmosphere_resource_->get_vortices_const_span()) {
            if (v.is_active == 0) {
                continue;
            }
            const Vector2 v_pos(v.parent_cell_x * cell_w + cell_w * 0.5f, v.parent_cell_y * cell_h + cell_h * 0.5f);
            draw_circle(v_pos, std::max(5.0f, cell_w * 1.8f), Color(1.0f, 0.18f, 0.28f, 0.85f));
        }

        const Vector2 marker_pos(selected_cell_.x * cell_w, selected_cell_.y * cell_h);
        draw_rect(Rect2(marker_pos, Vector2(std::max(4.0f, cell_w), std::max(4.0f, cell_h))), Color(1.0f, 0.92f, 0.20f, 1.0f), false, 2.0f);
    }

    draw_rect(Rect2(Vector2(0, 0), sz), Color(0.32f, 0.38f, 0.46f, 1.0f), false, 1.0f);
}

} // namespace ideam::godot_ext
