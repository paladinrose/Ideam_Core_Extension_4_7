#include "ideam_narratives_plugin.h"

#include "ui/location_map.h"
#include "ui/narrative_condition_field.h"
#include "ui/narrative_control.h"
#include "ui/narratives_composer.h"
#include "ui/plot_outliner.h"
#include "ui/relationship_map.h"

#include "../tools/tool_descriptor.h"
#include "../tools/tool_registry.h"
#include "../../core/tasks/registration/ideam_task_registry.h"
#include "../../core/tasks/domains/narratives/registration/narratives_task_domain_pack.h"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

namespace ideam::godot_ext {

IdeamNarrativesPlugin *IdeamNarrativesPlugin::singleton = nullptr;

void IdeamNarrativesPlugin::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("open_narratives_composer"), &IdeamNarrativesPlugin::open_narratives_composer);

    // Cross-Plugin Task Domain & Core Domain Handshakes
    ClassDB::bind_method(D_METHOD("get_task_domain_name"), &IdeamNarrativesPlugin::get_task_domain_name);
    ClassDB::bind_method(D_METHOD("register_task_domain_pack"), &IdeamNarrativesPlugin::register_task_domain_pack);
    ClassDB::bind_method(D_METHOD("is_linguistics_handshake_available"), &IdeamNarrativesPlugin::is_linguistics_handshake_available);
    ClassDB::bind_method(D_METHOD("is_tutorials_handshake_available"), &IdeamNarrativesPlugin::is_tutorials_handshake_available);
    ClassDB::bind_method(D_METHOD("is_tasks_handshake_available"), &IdeamNarrativesPlugin::is_tasks_handshake_available);

#ifdef TOOLS_ENABLED
    ClassDB::bind_static_method("IdeamNarrativesPlugin", D_METHOD("undo_redo"), &IdeamNarrativesPlugin::get_static_undo_redo);
#endif
    ClassDB::bind_static_method("IdeamNarrativesPlugin", D_METHOD("create_undo_redo_wrapper"), &IdeamNarrativesPlugin::create_undo_redo_wrapper);
}

IdeamNarrativesPlugin::IdeamNarrativesPlugin() {
    if (singleton == nullptr) {
        singleton = this;
    }
}

IdeamNarrativesPlugin::~IdeamNarrativesPlugin() {
    if (singleton == this) {
        singleton = nullptr;
    }
}

void IdeamNarrativesPlugin::_enter_tree() {
    validate_script_templates();

    set_plugin_active("IdeamNarratives", true);
    register_to_ecosystem("WizardSettings", "IdeamNarratives", godot::String(NARRATIVES_SETTINGS_PATH.data()));
    register_to_ecosystem("TaskManifests", "IdeamNarratives", godot::String(core::NarrativesTaskDomainPack::MANIFEST_PATH));
    if (is_plugin_active("IdeamTasks")) {
        register_task_domain_pack();
    }

    if (!narratives_inspector_.is_valid()) {
        narratives_inspector_.instantiate();
    }
    add_inspector_plugin(narratives_inspector_);

    _register_tools_and_workspace();

#ifdef TOOLS_ENABLED
    add_tool_menu_item(TOOLS_MENU_COMPOSER_ITEM, godot::Callable(this, "open_narratives_composer"));
#endif

    validate_narrative_script_templates();
}

void IdeamNarrativesPlugin::_exit_tree() {
#ifdef TOOLS_ENABLED
    remove_tool_menu_item(TOOLS_MENU_COMPOSER_ITEM);
#endif

    _unregister_tools_and_workspace();

    if (narratives_inspector_.is_valid()) {
        remove_inspector_plugin(narratives_inspector_);
        narratives_inspector_.unref();
    }

    set_plugin_active("IdeamNarratives", false);
    if (auto *task_reg = core::IdeamTaskRegistry::get_singleton()) {
        task_reg->set_domain_pack_active("IdeamNarratives", false);
    }
}

void IdeamNarrativesPlugin::open_narratives_composer() {
    NarrativesComposer::get_or_create_shared_composer();
}

void IdeamNarrativesPlugin::_register_tools_and_workspace() {
    auto &reg = ToolRegistry::get_singleton();
    const godot::StringName ws_id(NarrativesComposer::WORKSPACE_ID);

    // 1. Register NarrativesComposer BridgeLauncher
    reg.register_workspace_launcher({
        ws_id,
        "Narratives Composer",
        [](const godot::Ref<godot::Resource> &p_asset, const godot::Ref<ToolContext> &p_ctx) {
            if (NarrativesComposer *composer = NarrativesComposer::get_or_create_shared_composer()) {
                if (p_ctx.is_valid()) composer->set_tool_context(p_ctx);
                if (p_asset.is_valid()) composer->open_asset(p_asset);
            }
        }
    });

    // 2. Register Tier 1 DomainTool Control Factories
    reg.register_control({
        ws_id, "Narrative", "Narratives", "Narrative Roster", UPDATE_ON_COMMIT,
        [](const godot::Ref<godot::Resource> &p_asset, const godot::Ref<ToolContext> &p_ctx) -> godot::Control * {
            auto *ctrl = memnew(NarrativeControl);
            ctrl->bind_domain_asset(p_asset, p_ctx);
            return ctrl;
        },
        [](godot::Control *p_ctrl, const godot::Ref<godot::Resource> &p_asset, const godot::Ref<ToolContext> &p_ctx) {
            if (auto *nc = godot::Object::cast_to<NarrativeControl>(p_ctrl)) {
                nc->bind_domain_asset(p_asset, p_ctx);
            }
        }
    });

    reg.register_control({
        ws_id, "Location", "Locations", "Location Topology Map", UPDATE_ON_COMMIT,
        [](const godot::Ref<godot::Resource> &p_asset, const godot::Ref<ToolContext> &p_ctx) -> godot::Control * {
            auto *ctrl = memnew(LocationMap);
            ctrl->bind_domain_asset(p_asset, p_ctx);
            return ctrl;
        },
        [](godot::Control *p_ctrl, const godot::Ref<godot::Resource> &p_asset, const godot::Ref<ToolContext> &p_ctx) {
            if (auto *lm = godot::Object::cast_to<LocationMap>(p_ctrl)) {
                lm->bind_domain_asset(p_asset, p_ctx);
            }
        }
    });

    reg.register_control({
        ws_id, "Character", "Characters", "Relationship Map", UPDATE_ON_COMMIT,
        [](const godot::Ref<godot::Resource> &p_asset, const godot::Ref<ToolContext> &p_ctx) -> godot::Control * {
            auto *ctrl = memnew(RelationshipMap);
            ctrl->bind_domain_asset(p_asset, p_ctx);
            return ctrl;
        },
        [](godot::Control *p_ctrl, const godot::Ref<godot::Resource> &p_asset, const godot::Ref<ToolContext> &p_ctx) {
            if (auto *rm = godot::Object::cast_to<RelationshipMap>(p_ctrl)) {
                rm->bind_domain_asset(p_asset, p_ctx);
            }
        }
    });

    auto make_plot_outliner = [](const godot::Ref<godot::Resource> &p_asset, const godot::Ref<ToolContext> &p_ctx) -> godot::Control * {
        auto *ctrl = memnew(PlotOutliner);
        ctrl->bind_domain_asset(p_asset, p_ctx);
        return ctrl;
    };
    auto rebind_plot_outliner = [](godot::Control *p_ctrl, const godot::Ref<godot::Resource> &p_asset, const godot::Ref<ToolContext> &p_ctx) {
        if (auto *po = godot::Object::cast_to<PlotOutliner>(p_ctrl)) {
            po->bind_domain_asset(p_asset, p_ctx);
        }
    };

    reg.register_control({ws_id, "Plot", "Plots", "Plot Outliner", UPDATE_ON_COMMIT, make_plot_outliner, rebind_plot_outliner});
    reg.register_control({ws_id, "Incident", "Incidents", "Incident Outliner", UPDATE_ON_COMMIT, make_plot_outliner, rebind_plot_outliner});

    reg.register_control({
        ws_id, "Narrative_Condition", "Conditions", "Narrative Condition Sentence", UPDATE_ON_COMMIT,
        [](const godot::Ref<godot::Resource> &p_asset, const godot::Ref<ToolContext> &p_ctx) -> godot::Control * {
            auto *ctrl = memnew(NarrativeConditionField);
            ctrl->bind_domain_asset(p_asset, p_ctx);
            return ctrl;
        },
        [](godot::Control *p_ctrl, const godot::Ref<godot::Resource> &p_asset, const godot::Ref<ToolContext> &p_ctx) {
            if (auto *ncf = godot::Object::cast_to<NarrativeConditionField>(p_ctrl)) {
                ncf->bind_domain_asset(p_asset, p_ctx);
            }
        }
    });

    // 3. Register ToolDescriptor Catalog Entries (for IdeamTutorials & AgentToolBridge)
    auto add_desc = [&](const char *p_id, ToolTier p_tier, const char *p_name, const char *p_desc,
                        std::initializer_list<const char *> p_classes, std::initializer_list<const char *> p_tags) {
        godot::Ref<ToolDescriptor> d;
        d.instantiate();
        d->set_tool_id(godot::StringName(p_id));
        d->set_tier(p_tier);
        d->set_plugin_id("IdeamNarratives");
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

    add_desc("NarrativesComposer", TOOL_TIER_BRIDGE, "Narratives Composer",
             "Context-aware ToolWorkspace hosting all Narratives DomainTools (Roster, LocationMap, RelationshipMap, PlotOutliner, NarrativeConditionField).",
             {"Narrative", "Location", "Character", "Plot", "Incident", "Narrative_Condition"},
             {"narrative", "workspace", "composer"});

    add_desc("NarrativeControl", TOOL_TIER_DOMAIN, "Narrative Roster Control",
             "Two-column Literal vs Symbolic Narreme roster with real/symbolic conversion, symbolic instantiation, and best-fit assignment.",
             {"Narrative"}, {"narrative", "roster", "symbolic", "literal"});

    add_desc("NarrativeConditionField", TOOL_TIER_DOMAIN, "Narrative Condition Field",
             "Single-line malleable sentence editor for Narrative_Condition with language-agnostic FrameNet semantic predicates.",
             {"Narrative_Condition"}, {"narrative", "condition", "semantics", "framenet"});

    add_desc("LocationMap", TOOL_TIER_DOMAIN, "Location Topology Map",
             "Interactive directed-graph visualizer/editor for Location connections and BFS shortest-path queries.",
             {"Location", "Narrative"}, {"narrative", "location", "topology", "pathfinding"});

    add_desc("RelationshipMap", TOOL_TIER_DOMAIN, "Character Relationship Map",
             "Interactive multi-character relationship graph visualizer and editor.",
             {"Character", "Relationship", "Narrative"}, {"narrative", "relationship", "character", "graph"});

    add_desc("PlotOutliner", TOOL_TIER_DOMAIN, "Plot & Incident Outliner",
             "Master-Detail Trigger-Condition-Outcome outliner for Plots, Plot_Events, Incidents, and Incident_Conditions.",
             {"Plot", "Plot_Event", "Incident", "Incident_Condition", "Narrative"},
             {"narrative", "plot", "incident", "quest", "outliner"});

    add_desc("NarrativesInspector", TOOL_TIER_DOMAIN, "Narratives Inspector",
             "Inspector telemetry, IdeamUndoRedo injection, and Composer launcher for all Narreme and helper resources.",
             {"Narreme", "Relationship", "Incident_Condition", "Plot_Event"},
             {"narrative", "inspector", "semantics"});
}

void IdeamNarrativesPlugin::_unregister_tools_and_workspace() {
    auto &reg = ToolRegistry::get_singleton();
    const godot::StringName ws_id(NarrativesComposer::WORKSPACE_ID);
    reg.unregister_workspace_controls(ws_id);
    reg.unregister_workspace_launcher(ws_id);
    reg.unregister_plugin_tools("IdeamNarratives");
}

void IdeamNarrativesPlugin::_create_template(const godot::String &folder_name, const godot::String &default_file_name) {
    godot::String base_path   = "res://script_templates/";
    godot::String folder_path = base_path + folder_name;

    godot::Ref<godot::DirAccess> dir = godot::DirAccess::open(base_path);
    if (dir.is_valid() && !dir->dir_exists(folder_name)) {
        dir->make_dir(folder_name);
    }

    godot::Ref<godot::DirAccess> template_dir = godot::DirAccess::open(folder_path);
    if (template_dir.is_valid() && !template_dir->file_exists("default_template.gd")) {
        godot::String source_path = godot::String("res://addons/ideam_narratives/resources/") + default_file_name;
        godot::Ref<godot::FileAccess> default_text = godot::FileAccess::open(source_path, godot::FileAccess::READ);

        if (default_text.is_valid()) {
            godot::Ref<godot::FileAccess> new_template = godot::FileAccess::open(folder_path + godot::String("/default_template.gd"), godot::FileAccess::WRITE);
            if (new_template.is_valid()) {
                new_template->store_string(default_text->get_as_text());
                new_template->close();
            }
            default_text->close();
        }
    }
}

void IdeamNarrativesPlugin::validate_narrative_script_templates() {
    _create_template("Narreme", "default_narreme_template.txt");
    _create_template("Narrative", "default_narrative_template.txt");
    _create_template("Character", "default_character_template.txt");
    _create_template("Location", "default_location_template.txt");
    _create_template("Prop", "default_prop_template.txt");
    _create_template("Incident", "default_incident_template.txt");
    _create_template("Plot", "default_plot_template.txt");
}

void IdeamNarrativesPlugin::register_task_domain_pack() {
    if (auto *task_reg = core::IdeamTaskRegistry::get_singleton()) {
        if (!task_reg->has_domain_pack("IdeamNarratives")) {
            core::NarrativesTaskDomainPack::register_domain();
        }
        task_reg->set_domain_pack_active("IdeamNarratives", true);
    }
}

#ifdef TOOLS_ENABLED
godot::EditorUndoRedoManager *IdeamNarrativesPlugin::get_static_undo_redo() {
    if (singleton) {
        return singleton->get_undo_redo();
    }
    return nullptr;
}
#endif

godot::Ref<IdeamUndoRedo> IdeamNarrativesPlugin::create_undo_redo_wrapper() {
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