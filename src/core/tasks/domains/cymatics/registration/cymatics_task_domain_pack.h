#pragma once

namespace ideam::core {

class CymaticsTaskDomainPack {
public:
    static constexpr int CYMATICS_MANIFEST_VERSION = 1;
    static constexpr const char *MANIFEST_PATH = "res://addons/ideam_cymatics/resources/cymatics_task_manifest.tres";

    static void register_domain();
};

} // namespace ideam::core
