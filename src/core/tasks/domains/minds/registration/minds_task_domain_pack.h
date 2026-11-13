#pragma once

namespace ideam::core {

class MindsTaskDomainPack {
public:
    static constexpr int MINDS_MANIFEST_VERSION = 2;
    static constexpr const char *MANIFEST_PATH = "res://addons/ideam_minds/resources/minds_task_manifest.tres";

    static void register_domain();
};

} // namespace ideam::core
