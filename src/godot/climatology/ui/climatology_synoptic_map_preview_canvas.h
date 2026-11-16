#pragma once

/**
 * @file climatology_synoptic_map_preview_canvas.h
 * @brief Interactive 2D synoptic weather map & tornado vortex canvas for IdeamClimatology.
 *
 * Displays any of the 9 core::ClimatologyMapLayer views, overlays active supercell
 * tornado vortex cores, emits `cell_selected(grid_coords)` on left click, and
 * spawns a supercell tornado vortex on right click.
 */

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/variant/vector2i.hpp>

#include "godot/climatology/climatology_weather_resource.h"

namespace ideam::godot_ext {

class ClimatologySynopticMapPreviewCanvas : public godot::Control {
    GDCLASS(ClimatologySynopticMapPreviewCanvas, godot::Control)

private:
    godot::Ref<ClimatologyAtmosphereResource> atmosphere_resource_;
    godot::Ref<godot::ImageTexture> preview_texture_;
    int active_map_layer_ = static_cast<int>(core::ClimatologyMapLayer::POTENTIAL_TEMPERATURE_KELVIN);
    godot::Vector2i selected_cell_{32, 32};

protected:
    static void _bind_methods();

public:
    ClimatologySynopticMapPreviewCanvas();
    ~ClimatologySynopticMapPreviewCanvas() override = default;

    void set_atmosphere_resource(const godot::Ref<ClimatologyAtmosphereResource>& p_resource);
    godot::Ref<ClimatologyAtmosphereResource> get_atmosphere_resource() const;

    void set_active_map_layer(int p_layer);
    int get_active_map_layer() const;

    void set_selected_cell(const godot::Vector2i& p_cell);
    godot::Vector2i get_selected_cell() const;

    void rebuild_preview_texture();

    void _gui_input(const godot::Ref<godot::InputEvent>& p_event) override;
    void _draw() override;
};

} // namespace ideam::godot_ext
