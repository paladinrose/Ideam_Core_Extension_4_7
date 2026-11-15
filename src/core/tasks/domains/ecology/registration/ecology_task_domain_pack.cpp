#include "ecology_task_domain_pack.h"
#include "ecology_transform_task_registry.h"
#include "ecology_query_task_registry.h"
#include "ecology_metadata_task_registry.h"
#include "../../../registration/ideam_task_registry.h"

namespace ideam::core {

void EcologyTaskDomainPack::register_domain() {
    IdeamTaskRegistry *registry = IdeamTaskRegistry::get_singleton();
    if (!registry) return;

    TaskDomainPack pack;
    pack.domain_name               = godot::StringName("IdeamEcology");
    pack.ui_menu_prefix            = "Ecology";
    pack.manifest_path             = MANIFEST_PATH;
    pack.expected_manifest_version = ECOLOGY_MANIFEST_VERSION;
    pack.is_core                   = false;
    pack.is_active_in_editor       = false;

    pack.init_routing = []() {
        EcologyQueryTaskRegistry::init_execution_routing();
        EcologyTransformTaskRegistry::init_execution_routing();
        EcologyMetadataTaskRegistry::init_execution_routing();
    };
    pack.cleanup_routing = []() {
        EcologyQueryTaskRegistry::cleanup_execution_routing();
        EcologyTransformTaskRegistry::cleanup_execution_routing();
        EcologyMetadataTaskRegistry::cleanup_execution_routing();
    };
    pack.generate_query_ui     = &EcologyQueryTaskRegistry::generate_ui_matrices;
    pack.generate_transform_ui = &EcologyTransformTaskRegistry::generate_ui_matrices;
    pack.generate_metadata_ui  = &EcologyMetadataTaskRegistry::generate_ui_matrices;

    pack.create_transform = &EcologyTransformTaskRegistry::create;
    pack.create_query     = &EcologyQueryTaskRegistry::create;
    pack.create_metadata  = &EcologyMetadataTaskRegistry::create;

    registry->register_domain_pack(pack);
}

} // namespace ideam::core
