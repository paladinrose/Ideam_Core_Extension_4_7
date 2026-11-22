#include "ideam_surfaces_plugin.h"

#include "ui/surface_patch_control.h"
#include "ui/surfaces_composer.h"

#include "../tools/tool_descriptor.h"
#include "../tools/tool_registry.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

namespace ideam::godot_ext {

IdeamSurfacesPlugin *IdeamSurfacesPlugin::singleton = nullptr;

void IdeamSurfacesPlugin::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("open_surfaces_composer"), &IdeamSurfacesPlugin::open_surfaces_composer);

    // Cross-Plugin Lateral Handshakes
    ClassDB::bind_method(D_METHOD("get_plugin_domain_name"), &IdeamSurfacesPlugin::get_plugin_domain_name);
    ClassDB::bind_method(D_METHOD("is_splines_handshake_available"), &IdeamSurfacesPlugin::is_splines_handshake_available);
    ClassDB::bind_method(D_METHOD("is_geometry_handshake_available"), &IdeamSurfacesPlugin::is_geometry_handshake_available);
    ClassDB::bind_method(D_METHOD("is_design_handshake_available"), &IdeamSurfacesPlugin::is_design_handshake_available);
    ClassDB::bind_method(D_METHOD("is_characters_handshake_available"), &IdeamSurfacesPlugin::is_characters_handshake_available);
    ClassDB::bind_method(D_METHOD("is_tutorials_handshake_available"), &IdeamSurfacesPlugin::is_tutorials_handshake_available);
    ClassDB::bind_method(D_METHOD("is_tasks_handshake_available"), &IdeamSurfacesPlugin::is_tasks_handshake_available);

#ifdef TOOLS_ENABLED
    ClassDB::bind_static_method("IdeamSurfacesPlugin", D_METHOD("undo_redo"), &IdeamSurfacesPlugin::get_static_undo_redo);
#endif
    ClassDB::bind_static_method("IdeamSurfacesPlugin", D_METHOD("create_undo_redo_wrapper"), &IdeamSurfacesPlugin::create_undo_redo_wrapper);
}

IdeamSurfacesPlugin::IdeamSurfacesPlugin() {
    if (singleton == nullptr) {
        singleton = this;
    }
}

IdeamSurfacesPlugin::~IdeamSurfacesPlugin() {
    if (singleton == this) {
        singleton = nullptr;
    }
}

void IdeamSurfacesPlugin::_enter_tree() {
    validate_script_templates();

    set_plugin_active("IdeamSurfaces", true);
    register_to_ecosystem("WizardSettings", "IdeamSurfaces", godot::String(SURFACES_SETTINGS_PATH.data()));

    if (!surfaces_inspector_.is_valid()) {
        surfaces_inspector_.instantiate();
    }
    add_inspector_plugin(surfaces_inspector_);

    if (!surface_patch_gizmo_.is_valid()) {
        surface_patch_gizmo_.instantiate();
    }
    add_node_3d_gizmo_plugin(surface_patch_gizmo_);

    _register_tools_and_workspace();

#ifdef TOOLS_ENABLED
    add_tool_menu_item(TOOLS_MENU_COMPOSER_ITEM, godot::Callable(this, "open_surfaces_composer"));
#endif
}

void IdeamSurfacesPlugin::_exit_tree() {
#ifdef TOOLS_ENABLED
    remove_tool_menu_item(TOOLS_MENU_COMPOSER_ITEM);
#endif

    _unregister_tools_and_workspace();

    if (surface_patch_gizmo_.is_valid()) {
        remove_node_3d_gizmo_plugin(surface_patch_gizmo_);
        surface_patch_gizmo_.unref();
    }

    if (surfaces_inspector_.is_valid()) {
        remove_inspector_plugin(surfaces_inspector_);
        surfaces_inspector_.unref();
    }

    set_plugin_active("IdeamSurfaces", false);
}

void IdeamSurfacesPlugin::open_surfaces_composer() {
    SurfacesComposer::get_or_create_shared_composer();
}

void IdeamSurfacesPlugin::_register_tools_and_workspace() {
    auto &reg = ToolRegistry::get_singleton();
    const godot::StringName ws_id(SurfacesComposer::WORKSPACE_ID);

    // 1. Register SurfacesComposer BridgeLauncher
    reg.register_workspace_launcher({
        ws_id,
        "Surfaces Composer",
        [](const godot::Ref<godot::Resource> &p_asset, const godot::Ref<ToolContext> &p_ctx) {
            if (SurfacesComposer *composer = SurfacesComposer::get_or_create_shared_composer()) {
                if (p_ctx.is_valid()) composer->set_tool_context(p_ctx);
                if (p_asset.is_valid()) composer->open_asset(p_asset);
            }
        }
    });

    // 2. Register Tier 1 DomainTool Control Factories
    auto make_patch_ctrl = [](const godot::Ref<godot::Resource> &p_asset, const godot::Ref<ToolContext> &p_ctx) -> godot::Control * {
        auto *ctrl = memnew(SurfacePatchControl);
        ctrl->bind_domain_asset(p_asset, p_ctx);
        return ctrl;
    };
    auto rebind_patch_ctrl = [](godot::Control *p_ctrl, const godot::Ref<godot::Resource> &p_asset, const godot::Ref<ToolContext> &p_ctx) {
        if (auto *spc = godot::Object::cast_to<SurfacePatchControl>(p_ctrl)) {
            spc->bind_domain_asset(p_asset, p_ctx);
        }
    };

    reg.register_control({ws_id, "NurbsPatch3DResource", "Patches", "NURBS Patch", UPDATE_ON_COMMIT, make_patch_ctrl, rebind_patch_ctrl});
    reg.register_control({ws_id, "CoonsPatch3DResource", "Patches", "Gordon/Coons Patch", UPDATE_ON_COMMIT, make_patch_ctrl, rebind_patch_ctrl});
    reg.register_control({ws_id, "SurfaceNetworkResource", "Networks", "Surface Network", UPDATE_ON_COMMIT, make_patch_ctrl, rebind_patch_ctrl});

    // 3. Register ToolDescriptor Catalog Entries (for IdeamTutorials & AgentToolBridge)
    auto add_desc = [&](const char *p_id, ToolTier p_tier, const char *p_name, const char *p_desc,
                        std::initializer_list<const char *> p_classes, std::initializer_list<const char *> p_tags) {
        godot::Ref<ToolDescriptor> d;
        d.instantiate();
        d->set_tool_id(godot::StringName(p_id));
        d->set_tier(p_tier);
        d->set_plugin_id("IdeamSurfaces");
        d->set_display_name(p_name);
        d->set_description(p_desc);
        godot::PackedStringArray cls;
        for (const char *c : p_classes) cls.append(c);
        d->set_handled_classes(cls);
        godot::PackedStringArray tgs;
        for (const char *t : p_tags) tgs.append(t);
        d->set_semantic_tags(tgs);
        d->set_help_topic_id(godot::StringName(p_id));
        reg.register_tool_descriptor(d);
    };

    add_desc("SurfacesComposer", TOOL_TIER_BRIDGE, "Surfaces Composer",
             "Context-aware ToolWorkspace hosting IdeamSurfaces DomainTools for NURBS patches, Gordon/Coons curve-network patches, and watertight multi-patch networks.",
             {"SurfacePatchResource", "NurbsPatch3DResource", "CoonsPatch3DResource", "SurfaceNetworkResource"},
             {"surfaces", "nurbs", "coons", "gordon", "workspace", "composer"});

    add_desc("SurfacePatchControl", TOOL_TIER_DOMAIN, "Surface Patch & UV Probe Control",
             "Interactive UV isoparm preview, LOD 0..3 quad-budget telemetry, and curvilinear sample_frame(u, v) probe.",
             {"SurfacePatchResource", "NurbsPatch3DResource", "CoonsPatch3DResource", "SurfaceNetworkResource"},
             {"surfaces", "uv", "lod", "quad_mesh", "frame"});

    add_desc("SurfacePatchGizmo", TOOL_TIER_DOMAIN, "3D Surface Patch Gizmo",
             "3D viewport gizmo rendering UV isoparms, optional normal needles, and Undo/Redo NURBS control-point drag handles.",
             {"SurfaceMeshNode3D"},
             {"surfaces", "gizmo", "viewport", "nurbs"});

    add_desc("SurfacesInspector", TOOL_TIER_DOMAIN, "Surfaces Inspector",
             "Inspector telemetry, IdeamUndoRedo injection, and Composer launcher for IdeamSurfaces resources and nodes.",
             {"SurfacePatchResource", "NurbsPatch3DResource", "CoonsPatch3DResource", "SurfaceNetworkResource", "SurfaceMeshNode3D"},
             {"surfaces", "inspector", "lod", "manifold"});
}

void IdeamSurfacesPlugin::_unregister_tools_and_workspace() {
    auto &reg = ToolRegistry::get_singleton();
    const godot::StringName ws_id(SurfacesComposer::WORKSPACE_ID);
    reg.unregister_workspace_controls(ws_id);
    reg.unregister_workspace_launcher(ws_id);
    reg.unregister_plugin_tools("IdeamSurfaces");
}

#ifdef TOOLS_ENABLED
godot::EditorUndoRedoManager *IdeamSurfacesPlugin::get_static_undo_redo() {
    if (singleton) {
        return singleton->get_undo_redo();
    }
    return nullptr;
}
#endif

godot::Ref<IdeamUndoRedo> IdeamSurfacesPlugin::create_undo_redo_wrapper() {
    godot::Ref<IdeamUndoRedo> ur;
    ur.instantiate();
#ifdef TOOLS_ENABLED
    if (auto *editor_ur = get_static_undo_redo()) {
        ur->set_editor_undo_redo(editor_ur);
    }
#endif
    return ur;
}

} // namespace ideam::godot_ext
