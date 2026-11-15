#pragma once

namespace ideam::core {

class CharactersTaskDomainPack {
public:
    static constexpr int CHARACTERS_MANIFEST_VERSION = 2;
    static constexpr const char *MANIFEST_PATH = "res://addons/ideam_characters/resources/characters_task_manifest.tres";

    static void register_domain();
};

} // namespace ideam::core
