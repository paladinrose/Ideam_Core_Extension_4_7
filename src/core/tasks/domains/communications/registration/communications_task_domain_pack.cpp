#include "communications_task_domain_pack.h"
#include "communications_transform_task_registry.h"
#include "communications_query_task_registry.h"
#include "communications_metadata_task_registry.h"
#include "../../../registration/ideam_task_registry.h"

namespace ideam::core {

void CommunicationsTaskDomainPack::register_domain() {
    IdeamTaskRegistry *registry = IdeamTaskRegistry::get_singleton();
    if (!registry) return;

    TaskDomainPack pack;
    pack.domain_name               = godot::StringName("IdeamCommunications");
    pack.ui_menu_prefix            = "Communications";
    pack.manifest_path             = MANIFEST_PATH;
    pack.expected_manifest_version = COMMUNICATIONS_MANIFEST_VERSION;
    pack.is_core                   = false;
    pack.is_active_in_editor       = false;

    pack.init_routing = []() {
        CommunicationsQueryTaskRegistry::init_execution_routing();
        CommunicationsTransformTaskRegistry::init_execution_routing();
        CommunicationsMetadataTaskRegistry::init_execution_routing();
    };
    pack.cleanup_routing = []() {
        CommunicationsQueryTaskRegistry::cleanup_execution_routing();
        CommunicationsTransformTaskRegistry::cleanup_execution_routing();
        CommunicationsMetadataTaskRegistry::cleanup_execution_routing();
    };
    pack.generate_query_ui     = &CommunicationsQueryTaskRegistry::generate_ui_matrices;
    pack.generate_transform_ui = &CommunicationsTransformTaskRegistry::generate_ui_matrices;
    pack.generate_metadata_ui  = &CommunicationsMetadataTaskRegistry::generate_ui_matrices;

    pack.create_transform = &CommunicationsTransformTaskRegistry::create;
    pack.create_query     = &CommunicationsQueryTaskRegistry::create;
    pack.create_metadata  = &CommunicationsMetadataTaskRegistry::create;

    registry->register_domain_pack(pack);
}

} // namespace ideam::core
