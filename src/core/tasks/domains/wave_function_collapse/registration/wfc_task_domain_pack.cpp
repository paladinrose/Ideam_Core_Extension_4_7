#include "wfc_task_domain_pack.h"
#include "wfc_transform_task_registry.h"
#include "wfc_query_task_registry.h"
#include "wfc_metadata_task_registry.h"
#include "../../../registration/ideam_task_registry.h"

namespace ideam::core {

void WaveFunctionCollapseTaskDomainPack::register_domain() {
    IdeamTaskRegistry *registry = IdeamTaskRegistry::get_singleton();
    if (!registry) return;

    TaskDomainPack pack;
    pack.domain_name               = godot::StringName("IdeamWaveFunctionCollapse");
    pack.ui_menu_prefix            = "WaveFunctionCollapse";
    pack.manifest_path             = MANIFEST_PATH;
    pack.expected_manifest_version = WFC_MANIFEST_VERSION;
    pack.is_core                   = false;
    pack.is_active_in_editor       = false;

    pack.init_routing = []() {
        WaveFunctionCollapseQueryTaskRegistry::init_execution_routing();
        WaveFunctionCollapseTransformTaskRegistry::init_execution_routing();
        WaveFunctionCollapseMetadataTaskRegistry::init_execution_routing();
    };
    pack.cleanup_routing = []() {
        WaveFunctionCollapseQueryTaskRegistry::cleanup_execution_routing();
        WaveFunctionCollapseTransformTaskRegistry::cleanup_execution_routing();
        WaveFunctionCollapseMetadataTaskRegistry::cleanup_execution_routing();
    };
    pack.generate_query_ui     = &WaveFunctionCollapseQueryTaskRegistry::generate_ui_matrices;
    pack.generate_transform_ui = &WaveFunctionCollapseTransformTaskRegistry::generate_ui_matrices;
    pack.generate_metadata_ui  = &WaveFunctionCollapseMetadataTaskRegistry::generate_ui_matrices;

    pack.create_transform = &WaveFunctionCollapseTransformTaskRegistry::create;
    pack.create_query     = &WaveFunctionCollapseQueryTaskRegistry::create;
    pack.create_metadata  = &WaveFunctionCollapseMetadataTaskRegistry::create;

    registry->register_domain_pack(pack);
}

} // namespace ideam::core
