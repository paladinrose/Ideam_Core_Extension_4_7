#include "ideam_design_plugin.h"

#include "../tools/tool_descriptor.h"
#include "../tools/tool_registry.h"
#include "blueprint_resource.h"
#include "ui/blueprint_editor_control.h"
#include "ui/design_composer.h"
#include "ui/form_scaffold_control.h"

#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

namespace ideam::godot_ext {

IdeamDesignPlugin *IdeamDesignPlugin::singleton = nullptr;

IdeamDesignPlugin::IdeamDesignPlugin() {
    if (singleton == nullptr) {
        singleton = this;
    }
}

IdeamDesignPlugin::~IdeamDesignPlugin() {
    if (singleton == this) {
        singleton = nullptr;
    }
}

void IdeamDesignPlugin::_enter_tree() {
    validate_script_templates();
    set_plugin_active("IdeamDesign", true);
    register_to_ecosystem("WizardSettings", "IdeamDesign", godot::String(DESIGN_SETTINGS_PATH.data()));

    if (!inspector_plugin_.is_valid()) {
        inspector_plugin_.instantiate();
    }
    add_inspector_plugin(inspector_plugin_);

    if (!form_scaffold_gizmo_.is_valid()) {
        form_scaffold_gizmo_.instantiate();
    }
    add_node_3d_gizmo_plugin(form_scaffold_gizmo_);

    _register_tools_and_workspace();

#ifdef TOOLS_ENABLED
    add_tool_menu_item(TOOLS_MENU_COMPOSER_ITEM, godot::Callable(this, "open_design_composer"));
#endif
}

void IdeamDesignPlugin::_exit_tree() {
#ifdef TOOLS_ENABLED
    remove_tool_menu_item(TOOLS_MENU_COMPOSER_ITEM);
#endif

    _unregister_tools_and_workspace();

    if (form_scaffold_gizmo_.is_valid()) {
        remove_node_3d_gizmo_plugin(form_scaffold_gizmo_);
        form_scaffold_gizmo_.unref();
    }

    if (inspector_plugin_.is_valid()) {
        remove_inspector_plugin(inspector_plugin_);
        inspector_plugin_.unref();
    }
    set_plugin_active("IdeamDesign", false);
}

void IdeamDesignPlugin::open_design_composer() {
    DesignComposer::get_or_create_shared_composer();
}

void IdeamDesignPlugin::_register_tools_and_workspace() {
    auto &reg = ToolRegistry::get_singleton();
    const godot::StringName ws_id(DesignComposer::WORKSPACE_ID);

    reg.register_workspace_launcher({
        ws_id,
        "Design Composer",
        [](const godot::Ref<godot::Resource> &p_asset, const godot::Ref<ToolContext> &p_ctx) {
            if (DesignComposer *composer = DesignComposer::get_or_create_shared_composer()) {
                if (p_ctx.is_valid()) composer->set_tool_context(p_ctx);
                if (p_asset.is_valid()) composer->open_asset(p_asset);
            }
        }
    });

    reg.register_control({
        ws_id, "BlueprintResource", "Blueprints", "Architectural Blueprint", UPDATE_ON_COMMIT,
        [](const godot::Ref<godot::Resource> &p_asset, const godot::Ref<ToolContext> &p_ctx) -> godot::Control * {
            (void)p_ctx;
            godot::Ref<BlueprintResource> bp = p_asset;
            if (bp.is_null()) return nullptr;
            auto *ctrl = memnew(BlueprintEditorControl);
            ctrl->set_blueprint(bp);
            return ctrl;
        },
        nullptr
    });

    auto make_scaffold_ctrl = [](const godot::Ref<godot::Resource> &p_asset, const godot::Ref<ToolContext> &p_ctx) -> godot::Control * {
        auto *ctrl = memnew(FormScaffoldControl);
        ctrl->bind_domain_asset(p_asset, p_ctx);
        return ctrl;
    };
    auto rebind_scaffold_ctrl = [](godot::Control *p_ctrl, const godot::Ref<godot::Resource> &p_asset, const godot::Ref<ToolContext> &p_ctx) {
        if (auto *fsc = godot::Object::cast_to<FormScaffoldControl>(p_ctrl)) {
            fsc->bind_domain_asset(p_asset, p_ctx);
        }
    };

    reg.register_control({ws_id, "FormScaffoldResource", "Scaffolds & Rulers", "Loomis Form Scaffold", UPDATE_ON_COMMIT, make_scaffold_ctrl, rebind_scaffold_ctrl});
    reg.register_control({ws_id, "ProportionRulerGraphResource", "Scaffolds & Rulers", "Proportion Ruler Graph", UPDATE_ON_COMMIT, make_scaffold_ctrl, rebind_scaffold_ctrl});

    reg.register_control({
        ws_id, "ParametricAssemblyRecipeResource", "Recipes", "Assembly Recipe", UPDATE_ON_COMMIT,
        [](const godot::Ref<godot::Resource> &p_asset, const godot::Ref<ToolContext> &p_ctx) -> godot::Control * {
            using namespace godot;
            (void)p_ctx;
            if (p_asset.is_null()) return nullptr;
            auto *vbox = memnew(VBoxContainer);
            vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
            auto *lbl = memnew(Label);
            lbl->set_text("Parametric Assembly Recipe — Author recipe rules, sockets, and assembly parameters in the Inspector on the right.");
            lbl->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
            vbox->add_child(lbl);
            return vbox;
        },
        nullptr
    });

    auto make_style_host = [](const godot::Ref<godot::Resource> &p_asset, const godot::Ref<ToolContext> &p_ctx) -> godot::Control * {
        using namespace godot;
        (void)p_ctx;
        if (p_asset.is_null()) return nullptr;
        auto *vbox = memnew(VBoxContainer);
        vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
        auto *lbl = memnew(Label);
        lbl->set_text(p_asset->get_class() + " — Inspect 3D CSS theme/style tokens and cascading rules in the Inspector on the right.");
        lbl->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
        vbox->add_child(lbl);
        return vbox;
    };

    reg.register_control({ws_id, "Theme3DResource", "Themes & Styles", "3D Theme", UPDATE_ON_COMMIT, make_style_host, nullptr});
    reg.register_control({ws_id, "Style3DResource", "Themes & Styles", "3D Style", UPDATE_ON_COMMIT, make_style_host, nullptr});

    // Register ToolDescriptor catalog entries for IdeamTutorials & AgentToolBridge
    auto add_desc = [&](const char *p_id, ToolTier p_tier, const char *p_name, const char *p_desc,
                        std::initializer_list<const char *> p_classes, std::initializer_list<const char *> p_tags) {
        godot::Ref<ToolDescriptor> d;
        d.instantiate();
        d->set_tool_id(godot::StringName(p_id));
        d->set_tier(p_tier);
        d->set_plugin_id("IdeamDesign");
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

    add_desc("DesignComposer", TOOL_TIER_BRIDGE, "Design Composer",
             "Context-aware ToolWorkspace hosting Blueprints, Loomis Form Scaffolds, Proportion Rulers, Assembly Recipes, and 3D Themes.",
             {"BlueprintResource", "FormScaffoldResource", "ProportionRulerGraphResource", "ParametricAssemblyRecipeResource", "Theme3DResource", "Style3DResource"},
             {"design", "blueprint", "loomis", "scaffold", "workspace"});

    add_desc("FormScaffoldControl", TOOL_TIER_DOMAIN, "Loomis Form Scaffold & Ruler Control",
             "Interactive proportion canon preset strip, canonical unit scaler, contrapposto slider, and orthographic plumb-line balance canvas.",
             {"FormScaffoldResource", "ProportionRulerGraphResource"},
             {"design", "loomis", "scaffold", "proportions", "contrapposto", "skeleton"});

    add_desc("FormScaffoldGizmo", TOOL_TIER_DOMAIN, "3D Loomis Scaffold Gizmo",
             "3D viewport gizmo drawing canonical proportion ruler staves, center-of-gravity plumb lines, bone hierarchies, and Undo/Redo node handles.",
             {"FormScaffoldNode3D"},
             {"design", "loomis", "gizmo", "ruler", "balance"});
}

void IdeamDesignPlugin::_unregister_tools_and_workspace() {
    auto &reg = ToolRegistry::get_singleton();
    const godot::StringName ws_id(DesignComposer::WORKSPACE_ID);
    reg.unregister_workspace_controls(ws_id);
    reg.unregister_workspace_launcher(ws_id);
    reg.unregister_plugin_tools("IdeamDesign");
}

#ifdef TOOLS_ENABLED
godot::EditorUndoRedoManager *IdeamDesignPlugin::get_static_undo_redo() {
    if (singleton) {
        return singleton->get_undo_redo();
    }
    return nullptr;
}
#endif

godot::Ref<IdeamUndoRedo> IdeamDesignPlugin::create_undo_redo_wrapper() {
    godot::Ref<IdeamUndoRedo> ur;
    ur.instantiate();
#ifdef TOOLS_ENABLED
    if (godot::EditorUndoRedoManager *ed_ur = get_static_undo_redo()) {
        ur->set_editor_undo_redo(ed_ur);
    }
#endif
    return ur;
}

void IdeamDesignPlugin::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("open_design_composer"), &IdeamDesignPlugin::open_design_composer);
    ClassDB::bind_method(D_METHOD("get_plugin_domain_name"), &IdeamDesignPlugin::get_plugin_domain_name);
    ClassDB::bind_method(D_METHOD("is_geometry_handshake_available"), &IdeamDesignPlugin::is_geometry_handshake_available);
    ClassDB::bind_method(D_METHOD("is_splines_handshake_available"), &IdeamDesignPlugin::is_splines_handshake_available);
    ClassDB::bind_method(D_METHOD("is_surfaces_handshake_available"), &IdeamDesignPlugin::is_surfaces_handshake_available);
    ClassDB::bind_method(D_METHOD("is_characters_handshake_available"), &IdeamDesignPlugin::is_characters_handshake_available);
    ClassDB::bind_method(D_METHOD("is_tiles_handshake_available"), &IdeamDesignPlugin::is_tiles_handshake_available);
    ClassDB::bind_method(D_METHOD("is_linguistics_handshake_available"), &IdeamDesignPlugin::is_linguistics_handshake_available);
    ClassDB::bind_method(D_METHOD("is_tutorials_handshake_available"), &IdeamDesignPlugin::is_tutorials_handshake_available);
    ClassDB::bind_method(D_METHOD("is_tasks_handshake_available"), &IdeamDesignPlugin::is_tasks_handshake_available);

#ifdef TOOLS_ENABLED
    ClassDB::bind_static_method("IdeamDesignPlugin", D_METHOD("undo_redo"), &IdeamDesignPlugin::get_static_undo_redo);
#endif
    ClassDB::bind_static_method("IdeamDesignPlugin", D_METHOD("create_undo_redo_wrapper"), &IdeamDesignPlugin::create_undo_redo_wrapper);
}

} // namespace ideam::godot_ext
