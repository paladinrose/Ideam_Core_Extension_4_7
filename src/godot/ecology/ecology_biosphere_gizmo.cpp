#include "ecology_biosphere_gizmo.h"

#include <algorithm>

namespace ideam::godot_ext {

bool EcologyBiosphereGizmo::can_handle(godot::Node3D *p_target) const {
    return godot::Object::cast_to<EcologyBiosphereNode3D>(p_target) != nullptr;
}

void EcologyBiosphereGizmo::build_draw_list(
    godot::Node3D *p_target,
    IdeamGizmoDrawList &r_out,
    const GizmoViewState &p_view
) const {
    (void)p_view;
    auto *node = godot::Object::cast_to<EcologyBiosphereNode3D>(p_target);
    if (!node || node->get_biosphere_resource().is_null()) return;

    auto res = node->get_biosphere_resource();
    const float w_m = static_cast<float>(std::max(1, res->get_grid_width())) * res->get_cell_size_meters();
    const float h_m = static_cast<float>(std::max(1, res->get_grid_height())) * res->get_cell_size_meters();

    r_out.add_wire_box(
        godot::Vector3(w_m * 0.5f, 0.25f, h_m * 0.5f),
        godot::Vector3(w_m * 0.5f, 0.25f, h_m * 0.5f),
        godot::Basis(),
        GIZMO_PLUMB_BALANCED
    );

    // Sample canopy height needles across a 6x6 quadrat grid
    constexpr int kSamples = 6;
    const godot::Transform3D xf = node->get_global_transform();
    for (int gz = 0; gz < kSamples; ++gz) {
        const float z = ((static_cast<float>(gz) + 0.5f) / static_cast<float>(kSamples)) * h_m;
        for (int gx = 0; gx < kSamples; ++gx) {
            const float x = ((static_cast<float>(gx) + 0.5f) / static_cast<float>(kSamples)) * w_m;
            const float canopy = std::max(0.2f, node->sample_canopy_height_at_world(xf.xform(godot::Vector3(x, 0.0f, z))));
            r_out.add_segment(godot::Vector3(x, 0.0f, z), godot::Vector3(x, canopy, z), GIZMO_HANDLE_SPLINE_ANCHOR);
        }
    }
}

} // namespace ideam::godot_ext
