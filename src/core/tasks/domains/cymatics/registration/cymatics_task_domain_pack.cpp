#include "cymatics_task_domain_pack.h"
#include "cymatics_transform_task_registry.h"
#include "cymatics_metadata_task_registry.h"
#include "../../../registration/ideam_task_registry.h"

namespace ideam::core {

void CymaticsTaskDomainPack::register_domain() {
    IdeamTaskRegistry *registry = IdeamTaskRegistry::get_singleton();
    if (!registry) return;

    TaskDomainPack pack;
    pack.domain_name               = godot::StringName("IdeamCymatics");
    pack.ui_menu_prefix            = "Cymatics";
    pack.manifest_path             = MANIFEST_PATH;
    pack.expected_manifest_version = CYMATICS_MANIFEST_VERSION;
    pack.is_core                   = false;
    pack.is_active_in_editor       = false;

    pack.init_routing = []() {
        CymaticsTransformTaskRegistry::init_execution_routing();
        CymaticsMetadataTaskRegistry::init_execution_routing();
    };
    pack.cleanup_routing = []() {
        CymaticsTransformTaskRegistry::cleanup_execution_routing();
        CymaticsMetadataTaskRegistry::cleanup_execution_routing();
    };
    pack.generate_transform_ui = &CymaticsTransformTaskRegistry::generate_ui_matrices;
    pack.generate_metadata_ui  = &CymaticsMetadataTaskRegistry::generate_ui_matrices;

    pack.create_transform = &CymaticsTransformTaskRegistry::create;
    pack.create_metadata  = &CymaticsMetadataTaskRegistry::create;

    registry->register_domain_pack(pack);
}

} // namespace ideam::core
