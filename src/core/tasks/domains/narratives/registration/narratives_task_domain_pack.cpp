#include "narratives_task_domain_pack.h"
#include "narratives_transform_task_registry.h"
#include "narratives_query_task_registry.h"
#include "narratives_metadata_task_registry.h"
#include "../../../registration/ideam_task_registry.h"

namespace ideam::core {

void NarrativesTaskDomainPack::register_domain() {
    IdeamTaskRegistry* registry = IdeamTaskRegistry::get_singleton();
    if (!registry) return;

    TaskDomainPack narratives_pack;
    narratives_pack.domain_name = godot::StringName("IdeamNarratives");
    narratives_pack.ui_menu_prefix = "Narratives";
    narratives_pack.manifest_path = MANIFEST_PATH;
    narratives_pack.expected_manifest_version = NARRATIVES_MANIFEST_VERSION;
    narratives_pack.is_core = false;
    narratives_pack.is_active_in_editor = false;

    narratives_pack.init_routing = []() {
        NarrativesQueryTaskRegistry::init_execution_routing();
        NarrativesTransformTaskRegistry::init_execution_routing();
        NarrativesMetadataTaskRegistry::init_execution_routing();
    };
    narratives_pack.cleanup_routing = []() {
        NarrativesQueryTaskRegistry::cleanup_execution_routing();
        NarrativesTransformTaskRegistry::cleanup_execution_routing();
        NarrativesMetadataTaskRegistry::cleanup_execution_routing();
    };
    narratives_pack.generate_query_ui = &NarrativesQueryTaskRegistry::generate_ui_matrices;
    narratives_pack.generate_transform_ui = &NarrativesTransformTaskRegistry::generate_ui_matrices;
    narratives_pack.generate_metadata_ui = &NarrativesMetadataTaskRegistry::generate_ui_matrices;

    narratives_pack.create_transform = &NarrativesTransformTaskRegistry::create;
    narratives_pack.create_query = &NarrativesQueryTaskRegistry::create;
    narratives_pack.create_metadata = &NarrativesMetadataTaskRegistry::create;

    registry->register_domain_pack(narratives_pack);
}

} // namespace ideam::core