#pragma once

namespace ideam::core {

class CommunicationsTaskDomainPack {
public:
    static constexpr int COMMUNICATIONS_MANIFEST_VERSION = 2;
    static constexpr const char *MANIFEST_PATH = "res://addons/ideam_communications/resources/communications_task_manifest.tres";

    static void register_domain();
};

} // namespace ideam::core
