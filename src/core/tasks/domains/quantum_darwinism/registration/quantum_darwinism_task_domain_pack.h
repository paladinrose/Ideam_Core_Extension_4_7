#pragma once

namespace ideam::core {

class QuantumDarwinismTaskDomainPack {
public:
    static constexpr int QUANTUM_DARWINISM_MANIFEST_VERSION = 2;
    static constexpr const char *MANIFEST_PATH = "res://addons/ideam_quantum_darwinism/resources/quantum_darwinism_task_manifest.tres";

    static void register_domain();
};

} // namespace ideam::core
