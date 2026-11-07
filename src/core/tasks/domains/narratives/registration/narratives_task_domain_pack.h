#pragma once

namespace ideam::core {

class NarrativesTaskDomainPack {
public:
    static constexpr int NARRATIVES_MANIFEST_VERSION = 2;
    static constexpr const char* MANIFEST_PATH = "res://addons/ideam_narratives/resources/narratives_task_manifest.tres";

    static void register_domain();
};

} // namespace ideam::core