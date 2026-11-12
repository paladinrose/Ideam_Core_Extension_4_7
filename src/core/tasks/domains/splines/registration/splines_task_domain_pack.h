#pragma once

namespace ideam::core {

class SplinesTaskDomainPack {
public:
    static constexpr int SPLINES_MANIFEST_VERSION = 3;
    static constexpr const char *MANIFEST_PATH =
        "res://addons/ideam_splines/resources/splines_task_manifest.tres";

    static void register_domain();
};

} // namespace ideam::core
