#pragma once

namespace ideam::core {

class ProceduralGenerationTaskDomainPack {
public:
    static constexpr int PROCGEN_MANIFEST_VERSION = 3;
    static constexpr int PROCEDURAL_GENERATION_MANIFEST_VERSION = PROCGEN_MANIFEST_VERSION;
    static constexpr const char *MANIFEST_PATH = "res://addons/ideam_procedural_generation/resources/procedural_generation_task_manifest.tres";

    static void register_domain();
};

} // namespace ideam::core
