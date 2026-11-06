#pragma once

namespace ideam::core {

class GamesTaskDomainPack {
public:
    static constexpr int GAMES_MANIFEST_VERSION = 2;
    static constexpr const char* MANIFEST_PATH = "res://addons/ideam_games/resources/games_task_manifest.tres";

    static void register_domain();
};

} // namespace ideam::core