#include "splines_task_domain_pack.h"
#include "splines_metadata_task_registry.h"
#include "splines_query_task_registry.h"
#include "splines_transform_task_registry.h"
#include "../../../registration/ideam_task_registry.h"

namespace ideam::core {

void SplinesTaskDomainPack::register_domain() {
    IdeamTaskRegistry *registry = IdeamTaskRegistry::get_singleton();
    if (!registry) return;

    TaskDomainPack splines_pack;
    splines_pack.domain_name               = godot::StringName("IdeamSplines");
    splines_pack.ui_menu_prefix            = "Splines";
    splines_pack.manifest_path             = MANIFEST_PATH;
    splines_pack.expected_manifest_version = SPLINES_MANIFEST_VERSION;
    splines_pack.is_core                   = false;
    splines_pack.is_active_in_editor       = false;

    splines_pack.init_routing = []() {
        SplinesQueryTaskRegistry::init_execution_routing();
        SplinesTransformTaskRegistry::init_execution_routing();
        SplinesMetadataTaskRegistry::init_execution_routing();
    };
    splines_pack.cleanup_routing = []() {
        SplinesQueryTaskRegistry::cleanup_execution_routing();
        SplinesTransformTaskRegistry::cleanup_execution_routing();
        SplinesMetadataTaskRegistry::cleanup_execution_routing();
    };
    splines_pack.generate_query_ui     = &SplinesQueryTaskRegistry::generate_ui_matrices;
    splines_pack.generate_transform_ui = &SplinesTransformTaskRegistry::generate_ui_matrices;
    splines_pack.generate_metadata_ui  = &SplinesMetadataTaskRegistry::generate_ui_matrices;

    splines_pack.create_transform = &SplinesTransformTaskRegistry::create;
    splines_pack.create_query     = &SplinesQueryTaskRegistry::create;
    splines_pack.create_metadata  = &SplinesMetadataTaskRegistry::create;

    registry->register_domain_pack(splines_pack);
}

} // namespace ideam::core
