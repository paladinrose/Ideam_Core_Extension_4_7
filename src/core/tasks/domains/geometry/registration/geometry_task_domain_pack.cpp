#include "geometry_task_domain_pack.h"
#include "geometry_metadata_task_registry.h"
#include "geometry_query_task_registry.h"
#include "geometry_transform_task_registry.h"
#include "../../../registration/ideam_task_registry.h"

namespace ideam::core {

void GeometryTaskDomainPack::register_domain() {
    IdeamTaskRegistry *registry = IdeamTaskRegistry::get_singleton();
    if (!registry) return;

    TaskDomainPack geometry_pack;
    geometry_pack.domain_name               = godot::StringName("IdeamGeometry");
    geometry_pack.ui_menu_prefix            = "Geometry";
    geometry_pack.manifest_path             = MANIFEST_PATH;
    geometry_pack.expected_manifest_version = GEOMETRY_MANIFEST_VERSION;
    geometry_pack.is_core                   = false;
    geometry_pack.is_active_in_editor       = false;

    geometry_pack.init_routing = []() {
        GeometryQueryTaskRegistry::init_execution_routing();
        GeometryTransformTaskRegistry::init_execution_routing();
        GeometryMetadataTaskRegistry::init_execution_routing();
    };
    geometry_pack.cleanup_routing = []() {
        GeometryQueryTaskRegistry::cleanup_execution_routing();
        GeometryTransformTaskRegistry::cleanup_execution_routing();
        GeometryMetadataTaskRegistry::cleanup_execution_routing();
    };
    geometry_pack.generate_query_ui     = &GeometryQueryTaskRegistry::generate_ui_matrices;
    geometry_pack.generate_transform_ui = &GeometryTransformTaskRegistry::generate_ui_matrices;
    geometry_pack.generate_metadata_ui  = &GeometryMetadataTaskRegistry::generate_ui_matrices;

    geometry_pack.create_transform = &GeometryTransformTaskRegistry::create;
    geometry_pack.create_query     = &GeometryQueryTaskRegistry::create;
    geometry_pack.create_metadata  = &GeometryMetadataTaskRegistry::create;

    registry->register_domain_pack(geometry_pack);
}

} // namespace ideam::core
