#pragma once

namespace ideam::core {

class GeometryTaskDomainPack {
public:
    static constexpr int GEOMETRY_MANIFEST_VERSION = 2;
    static constexpr const char *MANIFEST_PATH =
        "res://addons/ideam_geometry/resources/geometry_task_manifest.tres";

    static void register_domain();
};

} // namespace ideam::core
