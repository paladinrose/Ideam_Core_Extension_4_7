#pragma once

namespace ideam::core {

class LinguisticsTaskDomainPack {
public:
    static constexpr int LINGUISTICS_MANIFEST_VERSION = 2;
    static constexpr const char *MANIFEST_PATH = "res://addons/ideam_linguistics/resources/linguistics_task_manifest.tres";

    static void register_domain();
};

} // namespace ideam::core
