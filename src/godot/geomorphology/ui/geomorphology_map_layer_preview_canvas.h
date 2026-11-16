#pragma once

/**
 * @file geomorphology_map_layer_preview_canvas.h
 * @brief Interactive 2D heatmap preview and click-to-inspect canvas for IdeamGeomorphology.
 *
 * Displays any of the 9 core::GeomorphologyMapLayer views (Shaded Relief, Tectonic Plates & Stress,
 * Top Exposed Lithology, Surface Water Depth, Stream Power Erosion, Strahler Stream Order,
 * Karst Cavity Potential, Soil Humus Fertility, Mineral Vein Potential) and emits
 * `cell_selected(grid_coords)` when clicked.
 */

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/variant/vector2i.hpp>

#include "godot/geomorphology/geomorphology_terrain_resource.h"

namespace ideam::godot_ext {

class GeomorphologyMapLayerPreviewCanvas : public godot::Control {
    GDCLASS(GeomorphologyMapLayerPreviewCanvas, godot::Control)

private:
    godot::Ref<GeomorphologyTerrainResource> terrain_resource_;
    godot::Ref<godot::ImageTexture> preview_texture_;
    int active_map_layer_ = static_cast<int>(core::GeomorphologyMapLayer::ELEVATION_SHADED_RELIEF);
    godot::Vector2i selected_cell_{32, 32};

protected:
    static void _bind_methods();

public:
    GeomorphologyMapLayerPreviewCanvas();
    ~GeomorphologyMapLayerPreviewCanvas() override = default;

    void set_terrain_resource(const godot::Ref<GeomorphologyTerrainResource>& p_resource);
    godot::Ref<GeomorphologyTerrainResource> get_terrain_resource() const;

    void set_active_map_layer(int p_layer);
    int get_active_map_layer() const;

    void set_selected_cell(const godot::Vector2i& p_cell);
    godot::Vector2i get_selected_cell() const;

    void rebuild_preview_texture();

    void _gui_input(const godot::Ref<godot::InputEvent>& p_event) override;
    void _draw() override;
};

} // namespace ideam::godot_ext
