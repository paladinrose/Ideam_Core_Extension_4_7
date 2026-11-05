#include "games_task_domain_pack.h"
#include "games_transform_task_registry.h"
#include "games_query_task_registry.h"
#include "games_metadata_task_registry.h"
#include "../../../registration/ideam_task_registry.h"

namespace ideam::core {

void GamesTaskDomainPack::register_domain() {
    IdeamTaskRegistry* registry = IdeamTaskRegistry::get_singleton();
    if (!registry) return;

    TaskDomainPack games_pack;
    games_pack.domain_name = godot::StringName("IdeamGames");
    games_pack.ui_menu_prefix = "Games";
    games_pack.manifest_path = MANIFEST_PATH;
    games_pack.expected_manifest_version = GAMES_MANIFEST_VERSION;
    games_pack.is_core = false;
    games_pack.is_active_in_editor = false;

    games_pack.init_routing = []() {
        GamesQueryTaskRegistry::init_execution_routing();
        GamesTransformTaskRegistry::init_execution_routing();
        GamesMetadataTaskRegistry::init_execution_routing();
    };
    games_pack.cleanup_routing = []() {
        GamesQueryTaskRegistry::cleanup_execution_routing();
        GamesTransformTaskRegistry::cleanup_execution_routing();
        GamesMetadataTaskRegistry::cleanup_execution_routing();
    };
    games_pack.generate_query_ui = &GamesQueryTaskRegistry::generate_ui_matrices;
    games_pack.generate_transform_ui = &GamesTransformTaskRegistry::generate_ui_matrices;
    games_pack.generate_metadata_ui = &GamesMetadataTaskRegistry::generate_ui_matrices;

    games_pack.create_transform = &GamesTransformTaskRegistry::create;
    games_pack.create_query = &GamesQueryTaskRegistry::create;
    games_pack.create_metadata = &GamesMetadataTaskRegistry::create;

    registry->register_domain_pack(games_pack);
}

} // namespace ideam::core