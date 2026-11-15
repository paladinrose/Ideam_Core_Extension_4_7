#include "geomorphology_task_domain_pack.h"
#include "geomorphology_transform_task_registry.h"
#include "geomorphology_query_task_registry.h"
#include "geomorphology_metadata_task_registry.h"
#include "../../../registration/ideam_task_registry.h"

namespace ideam::core {

void GeomorphologyTaskDomainPack::register_domain() {
    IdeamTaskRegistry *registry = IdeamTaskRegistry::get_singleton();
    if (!registry) return;

    TaskDomainPack pack;
    pack.domain_name               = godot::StringName("IdeamGeomorphology");
    pack.ui_menu_prefix            = "Geomorphology";
    pack.manifest_path             = MANIFEST_PATH;
    pack.expected_manifest_version = GEOMORPHOLOGY_MANIFEST_VERSION;
    pack.is_core                   = false;
    pack.is_active_in_editor       = false;

    pack.init_routing = []() {
        GeomorphologyQueryTaskRegistry::init_execution_routing();
        GeomorphologyTransformTaskRegistry::init_execution_routing();
        GeomorphologyMetadataTaskRegistry::init_execution_routing();
    };
    pack.cleanup_routing = []() {
        GeomorphologyQueryTaskRegistry::cleanup_execution_routing();
        GeomorphologyTransformTaskRegistry::cleanup_execution_routing();
        GeomorphologyMetadataTaskRegistry::cleanup_execution_routing();
    };
    pack.generate_query_ui     = &GeomorphologyQueryTaskRegistry::generate_ui_matrices;
    pack.generate_transform_ui = &GeomorphologyTransformTaskRegistry::generate_ui_matrices;
    pack.generate_metadata_ui  = &GeomorphologyMetadataTaskRegistry::generate_ui_matrices;

    pack.create_transform = &GeomorphologyTransformTaskRegistry::create;
    pack.create_query     = &GeomorphologyQueryTaskRegistry::create;
    pack.create_metadata  = &GeomorphologyMetadataTaskRegistry::create;

    registry->register_domain_pack(pack);
}

} // namespace ideam::core
