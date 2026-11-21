#include "ideam_quantum_darwinism_plugin.h"
#include "ui/pointer_state_horizon_visualizer.h"
#include "../tools/tool_descriptor.h"
#include "../tools/tool_registry.h"
#include "../procedural_generation/ideam_procedural_generation_plugin.h"
#include "../procedural_generation/ui/procedural_composer.h"
#include "../../core/tasks/registration/ideam_task_registry.h"
#include "../../core/tasks/domains/quantum_darwinism/registration/quantum_darwinism_task_domain_pack.h"

#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

IdeamQuantumDarwinismPlugin *IdeamQuantumDarwinismPlugin::singleton = nullptr;

void IdeamQuantumDarwinismPlugin::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("get_plugin_domain_name"), &IdeamQuantumDarwinismPlugin::get_plugin_domain_name);
    godot::ClassDB::bind_method(godot::D_METHOD("is_procedural_generation_handshake_available"), &IdeamQuantumDarwinismPlugin::is_procedural_generation_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_wfc_handshake_available"), &IdeamQuantumDarwinismPlugin::is_wfc_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_linguistics_handshake_available"), &IdeamQuantumDarwinismPlugin::is_linguistics_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_tiles_handshake_available"), &IdeamQuantumDarwinismPlugin::is_tiles_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_minds_handshake_available"), &IdeamQuantumDarwinismPlugin::is_minds_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_narratives_handshake_available"), &IdeamQuantumDarwinismPlugin::is_narratives_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_games_handshake_available"), &IdeamQuantumDarwinismPlugin::is_games_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_tasks_handshake_available"), &IdeamQuantumDarwinismPlugin::is_tasks_handshake_available);

#ifdef TOOLS_ENABLED
    godot::ClassDB::bind_static_method("IdeamQuantumDarwinismPlugin", godot::D_METHOD("undo_redo"), &IdeamQuantumDarwinismPlugin::get_static_undo_redo);
#endif
    godot::ClassDB::bind_static_method("IdeamQuantumDarwinismPlugin", godot::D_METHOD("create_undo_redo_wrapper"), &IdeamQuantumDarwinismPlugin::create_undo_redo_wrapper);
}

IdeamQuantumDarwinismPlugin::IdeamQuantumDarwinismPlugin() {
    if (singleton == nullptr) {
        singleton = this;
    }
}

IdeamQuantumDarwinismPlugin::~IdeamQuantumDarwinismPlugin() {
    if (singleton == this) {
        singleton = nullptr;
    }
}

void IdeamQuantumDarwinismPlugin::_enter_tree() {
    validate_script_templates();

    set_plugin_active("IdeamQuantumDarwinism", true);
    set_plugin_active("IdeamQD", true);
    register_to_ecosystem("WizardSettings", "IdeamQuantumDarwinism", godot::String(QD_SETTINGS_PATH.data()));
    register_to_ecosystem("TaskManifests", "IdeamQuantumDarwinism", godot::String(core::QuantumDarwinismTaskDomainPack::MANIFEST_PATH));

    core::QuantumDarwinismTaskDomainPack::register_domain();
    if (auto *task_reg = core::IdeamTaskRegistry::get_singleton()) {
        task_reg->set_domain_pack_active("IdeamQuantumDarwinism", true);
    }

    qd_inspector_.instantiate();
    add_inspector_plugin(qd_inspector_);

    auto &reg = ToolRegistry::get_singleton();
    const godot::StringName ws_id(ProceduralComposer::WORKSPACE_ID);

    auto make_visualizer = [](const godot::Ref<godot::Resource> &p_asset, const godot::Ref<ToolContext> &p_ctx) -> godot::Control * {
        if (p_asset.is_null()) return nullptr;
        auto *ctrl = memnew(PointerStateHorizonVisualizer);
        ctrl->bind_domain_asset(p_asset, p_ctx);
        return ctrl;
    };
    auto rebind_visualizer = [](godot::Control *p_ctrl, const godot::Ref<godot::Resource> &p_asset, const godot::Ref<ToolContext> &p_ctx) {
        if (auto *vis = godot::Object::cast_to<PointerStateHorizonVisualizer>(p_ctrl)) {
            vis->bind_domain_asset(p_asset, p_ctx);
        }
    };

    godot::PackedStringArray qd_tags;
    qd_tags.append("quantum_darwinism");
    qd_tags.append("procgen");
    qd_tags.append("wavefront");
    qd_tags.append("pointer_state");

    reg.register_control({
        ws_id, "QuantumDarwinismGeneratorResource", "Quantum Darwinism", "QD Generator", UPDATE_EXPLICIT_TRIGGER,
        make_visualizer, rebind_visualizer, qd_tags, godot::StringName("PointerStateHorizonVisualizer"), {}
    });

    reg.register_control({
        ws_id, "QuantumDarwinismStabilizerResource", "Quantum Darwinism", "QD Stabilizer", UPDATE_EXPLICIT_TRIGGER,
        make_visualizer, rebind_visualizer, qd_tags, godot::StringName("PointerStateHorizonVisualizer"), {}
    });

    // Register ToolDescriptor catalog entries for IdeamTutorials & AgentToolBridge
    auto add_desc = [&](const char *p_id, ToolTier p_tier, const char *p_name, const char *p_desc,
                        std::initializer_list<const char *> p_classes, std::initializer_list<const char *> p_tags) {
        godot::Ref<ToolDescriptor> d;
        d.instantiate();
        d->set_tool_id(godot::StringName(p_id));
        d->set_tier(p_tier);
        d->set_plugin_id("IdeamQuantumDarwinism");
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

    add_desc("PointerStateHorizonVisualizer", TOOL_TIER_DOMAIN, "Pointer-State & Wavefront Visualizer",
             "Interactive DomainTool for configuring Godot-canvas Quantum Darwinism channels, cross-channel couplings, outward wavefront stepping, spontaneous nucleation, and conflict horizons.",
             {"QuantumDarwinismGeneratorResource", "QuantumDarwinismStabilizerResource"},
             {"quantum_darwinism", "procgen", "pointer_state", "wavefront", "horizon"});

    add_desc("QuantumDarwinismGenerator", TOOL_TIER_DOMAIN, "Quantum Darwinism Generator",
             "Standalone outward multi-channel procedural generator over Godot SceneTrees, ClassDB reflection, groups, semes, and arbitrary graphs.",
             {"QuantumDarwinismGeneratorResource", "QuantumDarwinismGeneratorNode", "QuantumDarwinismChannelSpec", "QuantumDarwinismCouplingRule"},
             {"quantum_darwinism", "procgen", "generator", "channels", "coupling"});

    add_desc("QuantumDarwinismInspector", TOOL_TIER_DOMAIN, "Quantum Darwinism Inspector",
             "Inspector telemetry, IdeamUndoRedo injection, and ProceduralComposer integration for Quantum Darwinism resources and nodes.",
             {"QuantumDarwinismGeneratorResource", "QuantumDarwinismStabilizerResource", "QuantumDarwinismChannelSpec", "QuantumDarwinismCouplingRule"},
             {"quantum_darwinism", "inspector", "undo_redo"});
}

void IdeamQuantumDarwinismPlugin::_exit_tree() {
    auto &reg = ToolRegistry::get_singleton();
    const godot::StringName ws_id(ProceduralComposer::WORKSPACE_ID);
    reg.unregister_class(ws_id, "QuantumDarwinismGeneratorResource");
    reg.unregister_class(ws_id, "QuantumDarwinismStabilizerResource");
    reg.unregister_plugin_tools("IdeamQuantumDarwinism");

    if (qd_inspector_.is_valid()) {
        remove_inspector_plugin(qd_inspector_);
        qd_inspector_.unref();
    }
    if (auto *task_reg = core::IdeamTaskRegistry::get_singleton()) {
        task_reg->set_domain_pack_active("IdeamQuantumDarwinism", false);
    }
    set_plugin_active("IdeamQuantumDarwinism", false);
    set_plugin_active("IdeamQD", false);
}

#ifdef TOOLS_ENABLED
godot::EditorUndoRedoManager *IdeamQuantumDarwinismPlugin::get_static_undo_redo() {
    if (singleton) {
        return singleton->get_undo_redo();
    }
    return IdeamProceduralGenerationPlugin::get_static_undo_redo();
}
#endif

godot::Ref<IdeamUndoRedo> IdeamQuantumDarwinismPlugin::create_undo_redo_wrapper() {
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
