#include "characters_task_domain_pack.h"
#include "characters_transform_task_registry.h"
#include "characters_query_task_registry.h"
#include "characters_metadata_task_registry.h"
#include "../../../registration/ideam_task_registry.h"

namespace ideam::core {

void CharactersTaskDomainPack::register_domain() {
    IdeamTaskRegistry *registry = IdeamTaskRegistry::get_singleton();
    if (!registry) return;

    TaskDomainPack pack;
    pack.domain_name               = godot::StringName("IdeamCharacters");
    pack.ui_menu_prefix            = "Characters";
    pack.manifest_path             = MANIFEST_PATH;
    pack.expected_manifest_version = CHARACTERS_MANIFEST_VERSION;
    pack.is_core                   = false;
    pack.is_active_in_editor       = false;

    pack.init_routing = []() {
        CharactersQueryTaskRegistry::init_execution_routing();
        CharactersTransformTaskRegistry::init_execution_routing();
        CharactersMetadataTaskRegistry::init_execution_routing();
    };
    pack.cleanup_routing = []() {
        CharactersQueryTaskRegistry::cleanup_execution_routing();
        CharactersTransformTaskRegistry::cleanup_execution_routing();
        CharactersMetadataTaskRegistry::cleanup_execution_routing();
    };
    pack.generate_query_ui     = &CharactersQueryTaskRegistry::generate_ui_matrices;
    pack.generate_transform_ui = &CharactersTransformTaskRegistry::generate_ui_matrices;
    pack.generate_metadata_ui  = &CharactersMetadataTaskRegistry::generate_ui_matrices;

    pack.create_transform = &CharactersTransformTaskRegistry::create;
    pack.create_query     = &CharactersQueryTaskRegistry::create;
    pack.create_metadata  = &CharactersMetadataTaskRegistry::create;

    registry->register_domain_pack(pack);
}

} // namespace ideam::core
