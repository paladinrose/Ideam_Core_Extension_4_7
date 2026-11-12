#pragma once

namespace ideam::core {

class TilesTaskDomainPack {
public:
    static constexpr int TILES_MANIFEST_VERSION = 3;
    static constexpr const char *MANIFEST_PATH =
        "res://addons/ideam_tiles/resources/tiles_task_manifest.tres";

    static void register_domain();
};

} // namespace ideam::core
