#include "ideam_cymatics_plugin.h"
#include "../../core/tasks/domains/cymatics/registration/cymatics_task_domain_pack.h"
#include "../tools/tool_descriptor.h"
#include "../tools/tool_registry.h"

namespace ideam::godot_ext {

void IdeamCymaticsPlugin::_bind_methods() {}

void IdeamCymaticsPlugin::_enter_tree() {
    set_plugin_active("IdeamCymatics", true);
    core::CymaticsTaskDomainPack::register_domain();

    inspector_plugin_.instantiate();
    add_inspector_plugin(inspector_plugin_);
    music_zone_gizmo_.instantiate();
    music_zone_gizmo_->set_tool_context(get_or_create_plugin_tool_context());
    register_ideam_gizmo(music_zone_gizmo_);

    godot::Ref<ToolDescriptor> desc;
    desc.instantiate();
    desc->set_tool_id(godot::StringName("IdeamCymatics"));
    desc->set_tier(TOOL_TIER_DOMAIN);
    desc->set_plugin_id(godot::StringName("IdeamCymatics"));
    desc->set_display_name("Cymatic Map & Acoustic Zones");
    desc->set_description("2D/3D FDTD acoustic waves, Chladni/Bessel modal fields, and spatial Jukebox music zones.");
    godot::PackedStringArray classes;
    classes.push_back("CymaticMap");
    classes.push_back("JukeboxResource");
    classes.push_back("MusicZoneNode3D");
    desc->set_handled_classes(classes);
    godot::PackedStringArray tags;
    tags.push_back("acoustics");
    tags.push_back("cymatics");
    tags.push_back("audio");
    tags.push_back("music_zone");
    desc->set_semantic_tags(tags);
    desc->set_help_topic_id(godot::StringName("IdeamCymatics"));
    ToolRegistry::get_singleton().register_tool_descriptor(desc);
}

void IdeamCymaticsPlugin::_exit_tree() {
    ToolRegistry::get_singleton().unregister_plugin_tools(godot::StringName("IdeamCymatics"));
    cleanup_ideam_spatial_registrations();
    music_zone_gizmo_.unref();
    if (inspector_plugin_.is_valid()) {
        remove_inspector_plugin(inspector_plugin_);
        inspector_plugin_.unref();
    }
    set_plugin_active("IdeamCymatics", false);
}

} // namespace ideam::godot_ext
