#include "ideam_procedural_generation_plugin.h"
#include "../tools/tool_registry.h"
#include "ui/procedural_composer.h"
#include "../../core/tasks/registration/ideam_task_registry.h"

#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

IdeamProceduralGenerationPlugin *IdeamProceduralGenerationPlugin::singleton = nullptr;

void IdeamProceduralGenerationPlugin::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("get_plugin_domain_name"), &IdeamProceduralGenerationPlugin::get_plugin_domain_name);
    godot::ClassDB::bind_method(godot::D_METHOD("is_geometry_handshake_available"), &IdeamProceduralGenerationPlugin::is_geometry_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_splines_handshake_available"), &IdeamProceduralGenerationPlugin::is_splines_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_tiles_handshake_available"), &IdeamProceduralGenerationPlugin::is_tiles_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_design_handshake_available"), &IdeamProceduralGenerationPlugin::is_design_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_wfc_handshake_available"), &IdeamProceduralGenerationPlugin::is_wfc_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_quantum_darwinism_handshake_available"), &IdeamProceduralGenerationPlugin::is_quantum_darwinism_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_tasks_handshake_available"), &IdeamProceduralGenerationPlugin::is_tasks_handshake_available);

#ifdef TOOLS_ENABLED
    godot::ClassDB::bind_static_method(
        "IdeamProceduralGenerationPlugin",
        godot::D_METHOD("undo_redo"),
        &IdeamProceduralGenerationPlugin::get_static_undo_redo
    );
#endif
    godot::ClassDB::bind_static_method(
        "IdeamProceduralGenerationPlugin",
        godot::D_METHOD("create_undo_redo_wrapper"),
        &IdeamProceduralGenerationPlugin::create_undo_redo_wrapper
    );
}

IdeamProceduralGenerationPlugin::IdeamProceduralGenerationPlugin() {
    if (singleton == nullptr) {
        singleton = this;
    }
}

IdeamProceduralGenerationPlugin::~IdeamProceduralGenerationPlugin() {
    if (singleton == this) {
        singleton = nullptr;
    }
}

void IdeamProceduralGenerationPlugin::_enter_tree() {
    validate_script_templates();
    set_plugin_active("IdeamProceduralGeneration", true);
    register_to_ecosystem("WizardSettings", "IdeamProceduralGeneration", godot::String(PROCGEN_SETTINGS_PATH.data()));

    if (core::IdeamTaskRegistry *reg = core::IdeamTaskRegistry::get_singleton()) {
        reg->set_domain_active(godot::StringName("IdeamProceduralGeneration"), true);
    }

    inspector_plugin_.instantiate();
    add_inspector_plugin(inspector_plugin_);

    auto &reg = ToolRegistry::get_singleton();
    const godot::StringName ws_id(ProceduralComposer::WORKSPACE_ID);

    reg.register_workspace_launcher({
        ws_id,
        "Procedural Composer",
        [](const godot::Ref<godot::Resource> &p_asset, const godot::Ref<ToolContext> &p_ctx) {
            if (ProceduralComposer *composer = ProceduralComposer::get_or_create_shared_composer()) {
                if (p_ctx.is_valid()) composer->set_tool_context(p_ctx);
                composer->open_asset(p_asset);
            }
        }
    });

    auto make_procgen_panel = [](const godot::Ref<godot::Resource> &p_asset, const godot::Ref<ToolContext> &p_ctx) -> godot::Control * {
        using namespace godot;
        (void)p_ctx;
        if (p_asset.is_null()) return nullptr;
        auto *vbox = memnew(VBoxContainer);
        vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
        auto *hdr = memnew(Label);
        hdr->set_text("Procedural Asset (" + p_asset->get_class() + ") — Adjust seed, rules, and constraints in the Inspector on the right, then click Execute Generator.");
        hdr->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
        vbox->add_child(hdr);
        return vbox;
    };

    reg.register_control({ws_id, "ProceduralGeneratorResource", "Generators", "Procedural Generator", UPDATE_EXPLICIT_TRIGGER, make_procgen_panel, nullptr});
    reg.register_control({ws_id, "ProceduralPattern", "Generators", "Procedural Pattern", UPDATE_EXPLICIT_TRIGGER, make_procgen_panel, nullptr});
    reg.register_control({ws_id, "GenerationRule", "Generators", "Generation Rule", UPDATE_EXPLICIT_TRIGGER, make_procgen_panel, nullptr});
}

void IdeamProceduralGenerationPlugin::_exit_tree() {
    auto &reg = ToolRegistry::get_singleton();
    const godot::StringName ws_id(ProceduralComposer::WORKSPACE_ID);
    reg.unregister_class(ws_id, "ProceduralGeneratorResource");
    reg.unregister_class(ws_id, "ProceduralPattern");
    reg.unregister_class(ws_id, "GenerationRule");
    reg.unregister_workspace_launcher(ws_id);

    if (inspector_plugin_.is_valid()) {
        remove_inspector_plugin(inspector_plugin_);
        inspector_plugin_.unref();
    }
    if (core::IdeamTaskRegistry *task_reg = core::IdeamTaskRegistry::get_singleton()) {
        task_reg->set_domain_active(godot::StringName("IdeamProceduralGeneration"), false);
    }
    set_plugin_active("IdeamProceduralGeneration", false);
}

#ifdef TOOLS_ENABLED
godot::EditorUndoRedoManager *IdeamProceduralGenerationPlugin::get_static_undo_redo() {
    if (singleton) {
        return singleton->get_undo_redo();
    }
    return nullptr;
}
#endif

godot::Ref<IdeamUndoRedo> IdeamProceduralGenerationPlugin::create_undo_redo_wrapper() {
    godot::Ref<IdeamUndoRedo> ur;
    ur.instantiate();
#ifdef TOOLS_ENABLED
    if (godot::EditorUndoRedoManager *ed_ur = get_static_undo_redo()) {
        ur->set_editor_undo_redo(ed_ur);
    }
#endif
    return ur;
}

} // namespace ideam::godot_ext
