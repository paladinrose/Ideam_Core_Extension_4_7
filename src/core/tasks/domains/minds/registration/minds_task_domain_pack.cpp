#include "minds_task_domain_pack.h"
#include "minds_transform_task_registry.h"
#include "minds_query_task_registry.h"
#include "minds_metadata_task_registry.h"
#include "../../../registration/ideam_task_registry.h"

namespace ideam::core {

void MindsTaskDomainPack::register_domain() {
    IdeamTaskRegistry *registry = IdeamTaskRegistry::get_singleton();
    if (!registry) return;

    TaskDomainPack minds_pack;
    minds_pack.domain_name               = godot::StringName("IdeamMinds");
    minds_pack.ui_menu_prefix            = "Minds";
    minds_pack.manifest_path             = MANIFEST_PATH;
    minds_pack.expected_manifest_version = MINDS_MANIFEST_VERSION;
    minds_pack.is_core                   = false;
    minds_pack.is_active_in_editor       = false;

    minds_pack.init_routing = []() {
        MindsQueryTaskRegistry::init_execution_routing();
        MindsTransformTaskRegistry::init_execution_routing();
        MindsMetadataTaskRegistry::init_execution_routing();
    };
    minds_pack.cleanup_routing = []() {
        MindsQueryTaskRegistry::cleanup_execution_routing();
        MindsTransformTaskRegistry::cleanup_execution_routing();
        MindsMetadataTaskRegistry::cleanup_execution_routing();
    };
    minds_pack.generate_query_ui     = &MindsQueryTaskRegistry::generate_ui_matrices;
    minds_pack.generate_transform_ui = &MindsTransformTaskRegistry::generate_ui_matrices;
    minds_pack.generate_metadata_ui  = &MindsMetadataTaskRegistry::generate_ui_matrices;

    minds_pack.create_transform = &MindsTransformTaskRegistry::create;
    minds_pack.create_query     = &MindsQueryTaskRegistry::create;
    minds_pack.create_metadata  = &MindsMetadataTaskRegistry::create;

    registry->register_domain_pack(minds_pack);
}

} // namespace ideam::core
