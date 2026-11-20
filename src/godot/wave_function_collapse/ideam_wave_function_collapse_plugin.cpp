#include "ideam_wave_function_collapse_plugin.h"
#include "../tools/tool_registry.h"
#include "../procedural_generation/ideam_procedural_generation_plugin.h"
#include "../procedural_generation/ui/procedural_composer.h"
#include "../../core/tasks/domains/wave_function_collapse/registration/wfc_task_domain_pack.h"

#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

IdeamWaveFunctionCollapsePlugin *IdeamWaveFunctionCollapsePlugin::singleton = nullptr;

void IdeamWaveFunctionCollapsePlugin::_bind_methods() {
    godot::ClassDB::bind_method(
        godot::D_METHOD("get_plugin_domain_name"),
        &IdeamWaveFunctionCollapsePlugin::get_plugin_domain_name
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("is_procedural_generation_handshake_available"),
        &IdeamWaveFunctionCollapsePlugin::is_procedural_generation_handshake_available
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("is_linguistics_handshake_available"),
        &IdeamWaveFunctionCollapsePlugin::is_linguistics_handshake_available
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("is_tiles_handshake_available"),
        &IdeamWaveFunctionCollapsePlugin::is_tiles_handshake_available
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("is_design_handshake_available"),
        &IdeamWaveFunctionCollapsePlugin::is_design_handshake_available
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("is_narratives_handshake_available"),
        &IdeamWaveFunctionCollapsePlugin::is_narratives_handshake_available
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("is_games_handshake_available"),
        &IdeamWaveFunctionCollapsePlugin::is_games_handshake_available
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("is_quantum_darwinism_handshake_available"),
        &IdeamWaveFunctionCollapsePlugin::is_quantum_darwinism_handshake_available
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("is_tasks_handshake_available"),
        &IdeamWaveFunctionCollapsePlugin::is_tasks_handshake_available
    );

#ifdef TOOLS_ENABLED
    godot::ClassDB::bind_static_method(
        "IdeamWaveFunctionCollapsePlugin",
        godot::D_METHOD("undo_redo"),
        &IdeamWaveFunctionCollapsePlugin::get_static_undo_redo
    );
#endif
    godot::ClassDB::bind_static_method(
        "IdeamWaveFunctionCollapsePlugin",
        godot::D_METHOD("create_undo_redo_wrapper"),
        &IdeamWaveFunctionCollapsePlugin::create_undo_redo_wrapper
    );
}

IdeamWaveFunctionCollapsePlugin::IdeamWaveFunctionCollapsePlugin() {
    if (singleton == nullptr) {
        singleton = this;
    }
}

IdeamWaveFunctionCollapsePlugin::~IdeamWaveFunctionCollapsePlugin() {
    if (singleton == this) {
        singleton = nullptr;
    }
}

void IdeamWaveFunctionCollapsePlugin::_enter_tree() {
    validate_script_templates();

    set_plugin_active("IdeamWaveFunctionCollapse", true);
    set_plugin_active("IdeamWFC", true);
    register_to_ecosystem("WizardSettings", "IdeamWaveFunctionCollapse", godot::String(WFC_SETTINGS_PATH.data()));

    core::WaveFunctionCollapseTaskDomainPack::register_domain();

    wfc_rule_tensor_inspector_.instantiate();
    add_inspector_plugin(wfc_rule_tensor_inspector_);

    auto &reg = ToolRegistry::get_singleton();
    const godot::StringName ws_id(ProceduralComposer::WORKSPACE_ID);

    reg.register_control({
        ws_id, "WFCRuleTensorResource", "WFC", "WFC Rule Tensor", UPDATE_EXPLICIT_TRIGGER,
        [](const godot::Ref<godot::Resource> &p_asset, const godot::Ref<ToolContext> &p_ctx) -> godot::Control * {
            using namespace godot;
            (void)p_ctx;
            Ref<WFCRuleTensorResource> wfc = p_asset;
            if (wfc.is_null()) return nullptr;

            auto *vbox = memnew(VBoxContainer);
            vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);

            auto *hdr = memnew(Label);
            hdr->set_text(
                "Wave Function Collapse Rule Tensor — States: " + String::num_int64(wfc->get_state_count()) +
                " | Observed Rules: " + String::num_int64(wfc->get_observed_rule_count()) +
                " | Contradiction Horizon: " + String::num_int64(wfc->get_contradiction_horizon_radius())
            );
            hdr->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
            vbox->add_child(hdr);

            auto *hint = memnew(Label);
            hint->set_text("Edit token weights, entropy heuristics, and source patterns in the Inspector on the right.");
            hint->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
            vbox->add_child(hint);
            return vbox;
        },
        nullptr
    });
}

void IdeamWaveFunctionCollapsePlugin::_exit_tree() {
    ToolRegistry::get_singleton().unregister_class(
        godot::StringName(ProceduralComposer::WORKSPACE_ID),
        "WFCRuleTensorResource"
    );
    if (wfc_rule_tensor_inspector_.is_valid()) {
        remove_inspector_plugin(wfc_rule_tensor_inspector_);
        wfc_rule_tensor_inspector_.unref();
    }
    set_plugin_active("IdeamWaveFunctionCollapse", false);
    set_plugin_active("IdeamWFC", false);
}

#ifdef TOOLS_ENABLED
godot::EditorUndoRedoManager *IdeamWaveFunctionCollapsePlugin::get_static_undo_redo() {
    if (singleton) {
        return singleton->get_undo_redo();
    }
    return IdeamProceduralGenerationPlugin::get_static_undo_redo();
}
#endif

godot::Ref<IdeamUndoRedo> IdeamWaveFunctionCollapsePlugin::create_undo_redo_wrapper() {
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
