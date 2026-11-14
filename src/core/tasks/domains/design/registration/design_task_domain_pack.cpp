#include "design_task_domain_pack.h"
#include "design_transform_task_registry.h"
#include "design_query_task_registry.h"
#include "design_metadata_task_registry.h"
#include "../../../registration/ideam_task_registry.h"

namespace ideam::core {

void DesignTaskDomainPack::register_domain() {
    IdeamTaskRegistry *registry = IdeamTaskRegistry::get_singleton();
    if (!registry) return;

    TaskDomainPack pack;
    pack.domain_name               = godot::StringName("IdeamDesign");
    pack.ui_menu_prefix            = "Design";
    pack.manifest_path             = MANIFEST_PATH;
    pack.expected_manifest_version = DESIGN_MANIFEST_VERSION;
    pack.is_core                   = false;
    pack.is_active_in_editor       = false;

    pack.init_routing = []() {
        DesignQueryTaskRegistry::init_execution_routing();
        DesignTransformTaskRegistry::init_execution_routing();
        DesignMetadataTaskRegistry::init_execution_routing();
    };
    pack.cleanup_routing = []() {
        DesignQueryTaskRegistry::cleanup_execution_routing();
        DesignTransformTaskRegistry::cleanup_execution_routing();
        DesignMetadataTaskRegistry::cleanup_execution_routing();
    };
    pack.generate_query_ui     = &DesignQueryTaskRegistry::generate_ui_matrices;
    pack.generate_transform_ui = &DesignTransformTaskRegistry::generate_ui_matrices;
    pack.generate_metadata_ui  = &DesignMetadataTaskRegistry::generate_ui_matrices;

    pack.create_transform = &DesignTransformTaskRegistry::create;
    pack.create_query     = &DesignQueryTaskRegistry::create;
    pack.create_metadata  = &DesignMetadataTaskRegistry::create;

    registry->register_domain_pack(pack);
}

} // namespace ideam::core
