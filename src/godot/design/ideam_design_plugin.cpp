#include "ideam_design_plugin.h"

#include "../tools/tool_registry.h"
#include "blueprint_resource.h"
#include "ui/blueprint_editor_control.h"
#include "ui/design_composer.h"

#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/core/class_db.hpp>

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
    inspector_plugin_.instantiate();
    add_inspector_plugin(inspector_plugin_);

    auto &reg = ToolRegistry::get_singleton();
    const godot::StringName ws_id(DesignComposer::WORKSPACE_ID);

    reg.register_workspace_launcher({
        ws_id,
        "Design Composer",
        [](const godot::Ref<godot::Resource> &p_asset, const godot::Ref<ToolContext> &p_ctx) {
            if (DesignComposer *composer = DesignComposer::get_or_create_shared_composer()) {
                if (p_ctx.is_valid()) composer->set_tool_context(p_ctx);
                composer->open_asset(p_asset);
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
}

void IdeamDesignPlugin::_exit_tree() {
    auto &reg = ToolRegistry::get_singleton();
    const godot::StringName ws_id(DesignComposer::WORKSPACE_ID);
    reg.unregister_class(ws_id, "BlueprintResource");
    reg.unregister_class(ws_id, "ParametricAssemblyRecipeResource");
    reg.unregister_class(ws_id, "Theme3DResource");
    reg.unregister_class(ws_id, "Style3DResource");
    reg.unregister_workspace_launcher(ws_id);

    if (inspector_plugin_.is_valid()) {
        remove_inspector_plugin(inspector_plugin_);
        inspector_plugin_.unref();
    }
    set_plugin_active("IdeamDesign", false);
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
    ClassDB::bind_method(D_METHOD("get_plugin_domain_name"), &IdeamDesignPlugin::get_plugin_domain_name);
    ClassDB::bind_method(D_METHOD("is_geometry_handshake_available"), &IdeamDesignPlugin::is_geometry_handshake_available);
    ClassDB::bind_method(D_METHOD("is_splines_handshake_available"), &IdeamDesignPlugin::is_splines_handshake_available);
    ClassDB::bind_method(D_METHOD("is_tiles_handshake_available"), &IdeamDesignPlugin::is_tiles_handshake_available);
    ClassDB::bind_method(D_METHOD("is_linguistics_handshake_available"), &IdeamDesignPlugin::is_linguistics_handshake_available);
    ClassDB::bind_method(D_METHOD("is_tasks_handshake_available"), &IdeamDesignPlugin::is_tasks_handshake_available);

#ifdef TOOLS_ENABLED
    ClassDB::bind_static_method("IdeamDesignPlugin", D_METHOD("undo_redo"), &IdeamDesignPlugin::get_static_undo_redo);
#endif
    ClassDB::bind_static_method("IdeamDesignPlugin", D_METHOD("create_undo_redo_wrapper"), &IdeamDesignPlugin::create_undo_redo_wrapper);
}

} // namespace ideam::godot_ext
