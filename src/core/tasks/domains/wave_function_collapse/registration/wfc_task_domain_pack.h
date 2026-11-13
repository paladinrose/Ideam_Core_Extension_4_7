#pragma once

namespace ideam::core {

class WaveFunctionCollapseTaskDomainPack {
public:
    static constexpr int WFC_MANIFEST_VERSION = 2;
    static constexpr const char *MANIFEST_PATH = "res://addons/ideam_wave_function_collapse/resources/wfc_task_manifest.tres";

    static void register_domain();
};

} // namespace ideam::core
