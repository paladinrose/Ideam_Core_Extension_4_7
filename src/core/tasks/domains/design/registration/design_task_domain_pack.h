#pragma once

namespace ideam::core {

class DesignTaskDomainPack {
public:
    static constexpr int DESIGN_MANIFEST_VERSION = 2;
    static constexpr const char *MANIFEST_PATH = "res://addons/ideam_design/resources/design_task_manifest.tres";

    static void register_domain();
};

} // namespace ideam::core
