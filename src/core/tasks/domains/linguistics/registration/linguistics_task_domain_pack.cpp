#include "linguistics_task_domain_pack.h"
#include "linguistics_transform_task_registry.h"
#include "linguistics_query_task_registry.h"
#include "linguistics_metadata_task_registry.h"
#include "../../../registration/ideam_task_registry.h"

namespace ideam::core {

void LinguisticsTaskDomainPack::register_domain() {
    IdeamTaskRegistry *registry = IdeamTaskRegistry::get_singleton();
    if (!registry) return;

    TaskDomainPack pack;
    pack.domain_name               = godot::StringName("IdeamLinguistics");
    pack.ui_menu_prefix            = "Linguistics";
    pack.manifest_path             = MANIFEST_PATH;
    pack.expected_manifest_version = LINGUISTICS_MANIFEST_VERSION;
    pack.is_core                   = false;
    pack.is_active_in_editor       = false;

    pack.init_routing = []() {
        LinguisticsQueryTaskRegistry::init_execution_routing();
        LinguisticsTransformTaskRegistry::init_execution_routing();
        LinguisticsMetadataTaskRegistry::init_execution_routing();
    };
    pack.cleanup_routing = []() {
        LinguisticsQueryTaskRegistry::cleanup_execution_routing();
        LinguisticsTransformTaskRegistry::cleanup_execution_routing();
        LinguisticsMetadataTaskRegistry::cleanup_execution_routing();
    };
    pack.generate_query_ui     = &LinguisticsQueryTaskRegistry::generate_ui_matrices;
    pack.generate_transform_ui = &LinguisticsTransformTaskRegistry::generate_ui_matrices;
    pack.generate_metadata_ui  = &LinguisticsMetadataTaskRegistry::generate_ui_matrices;

    pack.create_transform = &LinguisticsTransformTaskRegistry::create;
    pack.create_query     = &LinguisticsQueryTaskRegistry::create;
    pack.create_metadata  = &LinguisticsMetadataTaskRegistry::create;

    registry->register_domain_pack(pack);
}

} // namespace ideam::core
