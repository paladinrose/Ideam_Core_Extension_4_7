#pragma once

/**
 * @file stratigraphic_column_inspector_control.h
 * @brief Bore-hole cross-section inspector widget for IdeamGeomorphology.
 *
 * Renders the vertical stratigraphic stack at a selected (x, y) terrain cell,
 * showing bedrock elevation, stacked rock strata with color-coded lithology,
 * fracture/porosity indicators, karst cavity void height, regolith/soil humus cap,
 * and surface water depth.
 */

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector2i.hpp>

#include "godot/geomorphology/geomorphology_terrain_resource.h"

namespace ideam::geomorphology {

class StratigraphicColumnInspectorControl : public godot::Control {
    GDCLASS(StratigraphicColumnInspectorControl, godot::Control)

private:
    godot::Ref<GeomorphologyTerrainResource> terrain_resource_;
    godot::Vector2i inspected_cell_{32, 32};
    godot::Dictionary cached_column_data_;

protected:
    static void _bind_methods();

public:
    StratigraphicColumnInspectorControl();
    ~StratigraphicColumnInspectorControl() override = default;

    void set_terrain_resource(const godot::Ref<GeomorphologyTerrainResource>& p_resource);
    godot::Ref<GeomorphologyTerrainResource> get_terrain_resource() const;

    void set_inspected_cell(const godot::Vector2i& p_cell);
    godot::Vector2i get_inspected_cell() const;

    void refresh_column_view();
    void _draw() override;
};

} // namespace ideam::geomorphology
