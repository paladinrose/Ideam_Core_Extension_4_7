#pragma once

namespace ideam::core {

class ClimatologyTaskDomainPack {
public:
    static constexpr int CLIMATOLOGY_MANIFEST_VERSION = 2;
    static constexpr const char *MANIFEST_PATH = "res://addons/ideam_climatology/resources/climatology_task_manifest.tres";

    static void register_domain();
};

} // namespace ideam::core
