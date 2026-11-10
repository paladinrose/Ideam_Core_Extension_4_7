#include "tiles_task_domain_pack.h"
#include "tiles_metadata_task_registry.h"
#include "tiles_query_task_registry.h"
#include "tiles_transform_task_registry.h"
#include "../../../registration/ideam_task_registry.h"

namespace ideam::core {

void TilesTaskDomainPack::register_domain() {
    IdeamTaskRegistry *registry = IdeamTaskRegistry::get_singleton();
    if (!registry) return;

    TaskDomainPack tiles_pack;
    tiles_pack.domain_name               = godot::StringName("IdeamTiles");
    tiles_pack.ui_menu_prefix            = "Tiles";
    tiles_pack.manifest_path             = MANIFEST_PATH;
    tiles_pack.expected_manifest_version = TILES_MANIFEST_VERSION;
    tiles_pack.is_core                   = false;
    tiles_pack.is_active_in_editor       = false;

    tiles_pack.init_routing = []() {
        TilesQueryTaskRegistry::init_execution_routing();
        TilesTransformTaskRegistry::init_execution_routing();
        TilesMetadataTaskRegistry::init_execution_routing();
    };
    tiles_pack.cleanup_routing = []() {
        TilesQueryTaskRegistry::cleanup_execution_routing();
        TilesTransformTaskRegistry::cleanup_execution_routing();
        TilesMetadataTaskRegistry::cleanup_execution_routing();
    };
    tiles_pack.generate_query_ui     = &TilesQueryTaskRegistry::generate_ui_matrices;
    tiles_pack.generate_transform_ui = &TilesTransformTaskRegistry::generate_ui_matrices;
    tiles_pack.generate_metadata_ui  = &TilesMetadataTaskRegistry::generate_ui_matrices;

    tiles_pack.create_transform = &TilesTransformTaskRegistry::create;
    tiles_pack.create_query     = &TilesQueryTaskRegistry::create;
    tiles_pack.create_metadata  = &TilesMetadataTaskRegistry::create;

    registry->register_domain_pack(tiles_pack);
}

} // namespace ideam::core
