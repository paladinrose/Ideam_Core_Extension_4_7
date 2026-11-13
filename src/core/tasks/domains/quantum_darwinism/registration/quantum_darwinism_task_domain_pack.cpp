#include "quantum_darwinism_task_domain_pack.h"
#include "quantum_darwinism_transform_task_registry.h"
#include "quantum_darwinism_query_task_registry.h"
#include "quantum_darwinism_metadata_task_registry.h"
#include "../../../registration/ideam_task_registry.h"

namespace ideam::core {

void QuantumDarwinismTaskDomainPack::register_domain() {
    IdeamTaskRegistry *registry = IdeamTaskRegistry::get_singleton();
    if (!registry) return;

    TaskDomainPack pack;
    pack.domain_name               = godot::StringName("IdeamQuantumDarwinism");
    pack.ui_menu_prefix            = "QuantumDarwinism";
    pack.manifest_path             = MANIFEST_PATH;
    pack.expected_manifest_version = QUANTUM_DARWINISM_MANIFEST_VERSION;
    pack.is_core                   = false;
    pack.is_active_in_editor       = false;

    pack.init_routing = []() {
        QuantumDarwinismQueryTaskRegistry::init_execution_routing();
        QuantumDarwinismTransformTaskRegistry::init_execution_routing();
        QuantumDarwinismMetadataTaskRegistry::init_execution_routing();
    };
    pack.cleanup_routing = []() {
        QuantumDarwinismQueryTaskRegistry::cleanup_execution_routing();
        QuantumDarwinismTransformTaskRegistry::cleanup_execution_routing();
        QuantumDarwinismMetadataTaskRegistry::cleanup_execution_routing();
    };
    pack.generate_query_ui     = &QuantumDarwinismQueryTaskRegistry::generate_ui_matrices;
    pack.generate_transform_ui = &QuantumDarwinismTransformTaskRegistry::generate_ui_matrices;
    pack.generate_metadata_ui  = &QuantumDarwinismMetadataTaskRegistry::generate_ui_matrices;

    pack.create_transform = &QuantumDarwinismTransformTaskRegistry::create;
    pack.create_query     = &QuantumDarwinismQueryTaskRegistry::create;
    pack.create_metadata  = &QuantumDarwinismMetadataTaskRegistry::create;

    registry->register_domain_pack(pack);
}

} // namespace ideam::core
