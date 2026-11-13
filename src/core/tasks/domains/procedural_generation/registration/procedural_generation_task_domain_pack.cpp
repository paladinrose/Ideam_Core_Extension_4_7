#include "procedural_generation_task_domain_pack.h"
#include "procedural_generation_transform_task_registry.h"
#include "procedural_generation_query_task_registry.h"
#include "procedural_generation_metadata_task_registry.h"
#include "../../../registration/ideam_task_registry.h"

namespace ideam::core {

void ProceduralGenerationTaskDomainPack::register_domain() {
    IdeamTaskRegistry *registry = IdeamTaskRegistry::get_singleton();
    if (!registry) return;

    TaskDomainPack pack;
    pack.domain_name               = godot::StringName("IdeamProceduralGeneration");
    pack.ui_menu_prefix            = "ProceduralGeneration";
    pack.manifest_path             = MANIFEST_PATH;
    pack.expected_manifest_version = PROCEDURAL_GENERATION_MANIFEST_VERSION;
    pack.is_core                   = false;
    pack.is_active_in_editor       = false;

    pack.init_routing = []() {
        ProceduralGenerationQueryTaskRegistry::init_execution_routing();
        ProceduralGenerationTransformTaskRegistry::init_execution_routing();
        ProceduralGenerationMetadataTaskRegistry::init_execution_routing();
    };
    pack.cleanup_routing = []() {
        ProceduralGenerationQueryTaskRegistry::cleanup_execution_routing();
        ProceduralGenerationTransformTaskRegistry::cleanup_execution_routing();
        ProceduralGenerationMetadataTaskRegistry::cleanup_execution_routing();
    };
    pack.generate_query_ui     = &ProceduralGenerationQueryTaskRegistry::generate_ui_matrices;
    pack.generate_transform_ui = &ProceduralGenerationTransformTaskRegistry::generate_ui_matrices;
    pack.generate_metadata_ui  = &ProceduralGenerationMetadataTaskRegistry::generate_ui_matrices;

    pack.create_transform = &ProceduralGenerationTransformTaskRegistry::create;
    pack.create_query     = &ProceduralGenerationQueryTaskRegistry::create;
    pack.create_metadata  = &ProceduralGenerationMetadataTaskRegistry::create;

    registry->register_domain_pack(pack);
}

} // namespace ideam::core
