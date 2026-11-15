#include "climatology_task_domain_pack.h"
#include "climatology_transform_task_registry.h"
#include "climatology_query_task_registry.h"
#include "climatology_metadata_task_registry.h"
#include "../../../registration/ideam_task_registry.h"

namespace ideam::core {

void ClimatologyTaskDomainPack::register_domain() {
    IdeamTaskRegistry *registry = IdeamTaskRegistry::get_singleton();
    if (!registry) return;

    TaskDomainPack pack;
    pack.domain_name               = godot::StringName("IdeamClimatology");
    pack.ui_menu_prefix            = "Climatology";
    pack.manifest_path             = MANIFEST_PATH;
    pack.expected_manifest_version = CLIMATOLOGY_MANIFEST_VERSION;
    pack.is_core                   = false;
    pack.is_active_in_editor       = false;

    pack.init_routing = []() {
        ClimatologyQueryTaskRegistry::init_execution_routing();
        ClimatologyTransformTaskRegistry::init_execution_routing();
        ClimatologyMetadataTaskRegistry::init_execution_routing();
    };
    pack.cleanup_routing = []() {
        ClimatologyQueryTaskRegistry::cleanup_execution_routing();
        ClimatologyTransformTaskRegistry::cleanup_execution_routing();
        ClimatologyMetadataTaskRegistry::cleanup_execution_routing();
    };
    pack.generate_query_ui     = &ClimatologyQueryTaskRegistry::generate_ui_matrices;
    pack.generate_transform_ui = &ClimatologyTransformTaskRegistry::generate_ui_matrices;
    pack.generate_metadata_ui  = &ClimatologyMetadataTaskRegistry::generate_ui_matrices;

    pack.create_transform = &ClimatologyTransformTaskRegistry::create;
    pack.create_query     = &ClimatologyQueryTaskRegistry::create;
    pack.create_metadata  = &ClimatologyMetadataTaskRegistry::create;

    registry->register_domain_pack(pack);
}

} // namespace ideam::core
