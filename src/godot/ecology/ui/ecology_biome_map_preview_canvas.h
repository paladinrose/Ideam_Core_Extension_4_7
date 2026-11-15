#pragma once

/**
 * @file ecology_biome_map_preview_canvas.h
 * @brief Interactive 2D biome, succession & wildfire heatmap canvas for IdeamEcology.
 *
 * Displays any of the 9 EcologyMapLayer views, emits `cell_selected(grid_coords)`
 * on left click, and ignites a Rothermel wildfire at the clicked cell on right click.
 */

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/variant/vector2i.hpp>

#include "godot/ecology/ecology_biosphere_resource.h"

namespace ideam::ecology {

class EcologyBiomeMapPreviewCanvas : public godot::Control {
    GDCLASS(EcologyBiomeMapPreviewCanvas, godot::Control)

private:
    godot::Ref<EcologyBiosphereResource> biosphere_resource_;
    godot::Ref<godot::ImageTexture> preview_texture_;
    int active_map_layer_ = static_cast<int>(EcologyMapLayer::WHITTAKER_HOLDRIDGE_BIOME);
    godot::Vector2i selected_cell_{32, 32};

protected:
    static void _bind_methods();

public:
    EcologyBiomeMapPreviewCanvas();
    ~EcologyBiomeMapPreviewCanvas() override = default;

    void set_biosphere_resource(const godot::Ref<EcologyBiosphereResource>& p_resource);
    godot::Ref<EcologyBiosphereResource> get_biosphere_resource() const;

    void set_active_map_layer(int p_layer);
    int get_active_map_layer() const;

    void set_selected_cell(const godot::Vector2i& p_cell);
    godot::Vector2i get_selected_cell() const;

    void rebuild_preview_texture();

    void _gui_input(const godot::Ref<godot::InputEvent>& p_event) override;
    void _draw() override;
};

} // namespace ideam::ecology
