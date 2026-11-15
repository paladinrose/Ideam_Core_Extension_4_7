#pragma once

namespace ideam::core {

class GeomorphologyTaskDomainPack {
public:
    static constexpr int GEOMORPHOLOGY_MANIFEST_VERSION = 2;
    static constexpr const char *MANIFEST_PATH = "res://addons/ideam_geomorphology/resources/geomorphology_task_manifest.tres";

    static void register_domain();
};

} // namespace ideam::core
