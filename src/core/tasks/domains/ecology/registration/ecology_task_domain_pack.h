#pragma once

namespace ideam::core {

class EcologyTaskDomainPack {
public:
    static constexpr int ECOLOGY_MANIFEST_VERSION = 2;
    static constexpr const char *MANIFEST_PATH = "res://addons/ideam_ecology/resources/ecology_task_manifest.tres";

    static void register_domain();
};

} // namespace ideam::core
