#include "climatology_weather_gizmo.h"

#include <algorithm>

namespace ideam::godot_ext {

bool ClimatologyWeatherGizmo::can_handle(godot::Node3D *p_target) const {
    return godot::Object::cast_to<ClimatologyWeatherNode3D>(p_target) != nullptr;
}

void ClimatologyWeatherGizmo::build_draw_list(
    godot::Node3D *p_target,
    IdeamGizmoDrawList &r_out,
    const GizmoViewState &p_view
) const {
    (void)p_view;
    auto *node = godot::Object::cast_to<ClimatologyWeatherNode3D>(p_target);
    if (!node || node->get_atmosphere_resource().is_null()) return;

    auto res = node->get_atmosphere_resource();
    const float w_m = static_cast<float>(std::max(1, res->get_grid_width())) * res->get_cell_size_meters();
    const float h_m = static_cast<float>(std::max(1, res->get_grid_height())) * res->get_cell_size_meters();

    r_out.add_wire_box(
        godot::Vector3(w_m * 0.5f, 1.0f, h_m * 0.5f),
        godot::Vector3(w_m * 0.5f, 1.0f, h_m * 0.5f),
        godot::Basis(),
        GIZMO_LINE_AXIS_NORMAL
    );

    // 3D Wind Vector Arrows across a 5x5 atmospheric sample grid
    constexpr int kSamples = 5;
    const godot::Transform3D xf = node->get_global_transform();
    const float arrow_scale = std::max(0.5f, res->get_cell_size_meters() * 0.25f);
    for (int gz = 0; gz < kSamples; ++gz) {
        const float z = ((static_cast<float>(gz) + 0.5f) / static_cast<float>(kSamples)) * h_m;
        for (int gx = 0; gx < kSamples; ++gx) {
            const float x = ((static_cast<float>(gx) + 0.5f) / static_cast<float>(kSamples)) * w_m;
            const godot::Vector3 wind = node->sample_wind_velocity_at_world(xf.xform(godot::Vector3(x, 2.0f, z)));
            const float spd = wind.length();
            if (spd > 1e-3f) {
                const float len = std::clamp(spd * 0.15f, 0.35f, arrow_scale * 2.0f);
                r_out.add_cone_arrow(godot::Vector3(x, 2.0f, z), wind / spd, len, len * 0.18f, GIZMO_SNAP_PREVIEW);
            }
        }
    }
}

} // namespace ideam::godot_ext
