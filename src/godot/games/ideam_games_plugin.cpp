#include "ideam_games_plugin.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_selection.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

// Include entity & Phase 4 gameplay editor plugins
#include "editor/game_agent_editor_inspector_plugin.h"
#include "editor/game_agent_action_editor_inspector_plugin.h"
#include "editor/gameplay_rule_editor_inspector_plugin.h"
#include "editor/gameplay_resource_editor_inspector_plugin.h"
#include "editor/gameplay_style_cascade_dock.h"
#include "editor/gameplay_editor_debugger_plugin.h"

#include "game.h"
#include "game_entities/game_agent.h"
#include "game_entity.h"
#include "gameplay/gameplay_mode.h"

#include "../../core/tasks/registration/ideam_task_registry.h"
#include "../../core/tasks/domains/games/registration/games_task_domain_pack.h"
#include "../project_tools/ideam_project_contribution_registry.h"

namespace ideam::godot_ext {

IdeamGamesPlugin* IdeamGamesPlugin::singleton = nullptr;

void IdeamGamesPlugin::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("open_agent_action_tool"), &IdeamGamesPlugin::open_agent_action_tool);
    godot::ClassDB::bind_method(godot::D_METHOD("close_agent_action_tool"), &IdeamGamesPlugin::close_agent_action_tool);
    godot::ClassDB::bind_method(godot::D_METHOD("_scene_change", "new_root"), &IdeamGamesPlugin::_scene_change);
    godot::ClassDB::bind_method(godot::D_METHOD("_on_editor_selection_changed"), &IdeamGamesPlugin::_on_editor_selection_changed);
    godot::ClassDB::bind_method(godot::D_METHOD("get_cascade_dock"), &IdeamGamesPlugin::get_cascade_dock);
    godot::ClassDB::bind_method(godot::D_METHOD("get_gameplay_debugger"), &IdeamGamesPlugin::get_gameplay_debugger);

    // Cross-Plugin Task Domain Handshake
    godot::ClassDB::bind_method(godot::D_METHOD("get_task_domain_name"), &IdeamGamesPlugin::get_task_domain_name);
    godot::ClassDB::bind_method(godot::D_METHOD("register_task_domain_pack"), &IdeamGamesPlugin::register_task_domain_pack);

    // Static bindings so the ecosystem can call these without an instance reference
    godot::ClassDB::bind_static_method("IdeamGamesPlugin", godot::D_METHOD("open_action_tool_window", "agent", "editor"), &IdeamGamesPlugin::open_action_tool_window);
    godot::ClassDB::bind_static_method("IdeamGamesPlugin", godot::D_METHOD("undo_redo"), &IdeamGamesPlugin::get_static_undo_redo);
}

IdeamGamesPlugin::IdeamGamesPlugin() {
    if (singleton == nullptr) {
        singleton = this;
    }
}

IdeamGamesPlugin::~IdeamGamesPlugin() {
    if (singleton == this) {
        singleton = nullptr;
    }
}

void IdeamGamesPlugin::_enter_tree() {
    // Call base class template setup
    validate_script_templates();

    // Ecosystem Handshake: Announce IdeamGames is active and register typed Project Tools manifest
    set_plugin_active("IdeamGames", true);
    register_wizard_manifest("IdeamGames", GAME_SETTINGS_PATH);
    register_to_ecosystem("TaskManifests", "IdeamGames", godot::String(core::GamesTaskDomainPack::MANIFEST_PATH));
    if (is_plugin_active("IdeamTasks")) {
        register_task_domain_pack();
    }

    // Register specialized script templates and programmatic Project Tools contribution
    validate_game_script_templates();
    _register_project_contribution();

    // 1. Instantiate native C++ entity inspector plugins
    agent_editor.instantiate();
    add_inspector_plugin(agent_editor);

    agent_action_editor.instantiate();
    add_inspector_plugin(agent_action_editor);

    // 2. Instantiate Phase 4 GameplayRule & GameplayResource inspector plugins
    gameplay_rule_editor.instantiate();
    add_inspector_plugin(gameplay_rule_editor);

    gameplay_resource_editor.instantiate();
    add_inspector_plugin(gameplay_resource_editor);

    // 3. Instantiate Phase 4 GameplayStyleCascadeDock bottom panel
    cascade_dock = memnew(GameplayStyleCascadeDock);
    cascade_dock_button = add_control_to_bottom_panel(cascade_dock, "Gameplay Cascade");

    // 4. Instantiate Phase 4 GameplayEditorDebuggerPlugin
    gameplay_debugger.instantiate();
    add_debugger_plugin(gameplay_debugger);

    // 5. Connect scene & selection change listeners
    if (!is_connected("scene_changed", godot::Callable(this, "_scene_change"))) {
        connect("scene_changed", godot::Callable(this, "_scene_change"));
    }

    if (get_editor_interface() && get_editor_interface()->get_selection()) {
        godot::EditorSelection* sel = get_editor_interface()->get_selection();
        if (!sel->is_connected("selection_changed", godot::Callable(this, "_on_editor_selection_changed"))) {
            sel->connect("selection_changed", godot::Callable(this, "_on_editor_selection_changed"));
        }
    }

    _sync_cascade_dock_with_selection();
}

void IdeamGamesPlugin::_exit_tree() {
    if (is_connected("scene_changed", godot::Callable(this, "_scene_change"))) {
        disconnect("scene_changed", godot::Callable(this, "_scene_change"));
    }

    if (get_editor_interface() && get_editor_interface()->get_selection()) {
        godot::EditorSelection* sel = get_editor_interface()->get_selection();
        if (sel->is_connected("selection_changed", godot::Callable(this, "_on_editor_selection_changed"))) {
            sel->disconnect("selection_changed", godot::Callable(this, "_on_editor_selection_changed"));
        }
    }

    if (cascade_dock) {
        remove_control_from_bottom_panel(cascade_dock);
        memdelete(cascade_dock);
        cascade_dock = nullptr;
        cascade_dock_button = nullptr;
    }

    if (gameplay_debugger.is_valid()) {
        remove_debugger_plugin(gameplay_debugger);
        gameplay_debugger.unref();
    }

    if (gameplay_resource_editor.is_valid()) {
        remove_inspector_plugin(gameplay_resource_editor);
        gameplay_resource_editor.unref();
    }

    if (gameplay_rule_editor.is_valid()) {
        remove_inspector_plugin(gameplay_rule_editor);
        gameplay_rule_editor.unref();
    }

    if (agent_action_editor.is_valid()) {
        remove_inspector_plugin(agent_action_editor);
        agent_action_editor.unref();
    }

    if (agent_editor.is_valid()) {
        remove_inspector_plugin(agent_editor);
        agent_editor.unref();
    }

    // Ecosystem Handshake: Mark IdeamGames inactive and unregister Project Tools & Task contributions
    IdeamProjectContributionRegistry::get_singleton().unregister_plugin("IdeamGames");
    set_plugin_active("IdeamGames", false);
    if (auto* task_reg = core::IdeamTaskRegistry::get_singleton()) {
        task_reg->set_domain_pack_active("IdeamGames", false);
    }
}

void IdeamGamesPlugin::open_agent_action_tool() {
    godot::Control* top = get_editor_interface()->get_editor_main_screen();
    godot::Control* p = top->get_parent_control();
    while (p != nullptr) {
        top = p;
        p = top->get_parent_control();
    }

    agent_action_tool_window = memnew(godot::Window);
    godot::Vector2 min_size = get_editor_interface()->get_editor_main_screen()->get_viewport()->get_visible_rect().size / 2.0;

    agent_action_tool_window->connect("close_requested", godot::Callable(this, "close_agent_action_tool"));
    agent_action_tool_window->popup_exclusive_centered(top, min_size);

    godot::ScrollContainer* scroll = memnew(godot::ScrollContainer);
    scroll->set_custom_minimum_size(min_size);
    scroll->set_horizontal_scroll_mode(godot::ScrollContainer::SCROLL_MODE_DISABLED);
    agent_action_tool_window->add_child(scroll);

    // Load the tool scene
    godot::Ref<godot::PackedScene> tool_scene = godot::ResourceLoader::get_singleton()->load(GAME_AGENT_ACTION_TOOL_PATH);
    if (tool_scene.is_valid()) {
        agent_action_tool = godot::Object::cast_to<godot::Control>(tool_scene->instantiate());
        if (agent_action_tool) {
            scroll->add_child(agent_action_tool);
            // Dynamic call to handle GDScript integration safely
            agent_action_tool->call("open_tool", _current_agent, _current_editor);
        }
    }
}

void IdeamGamesPlugin::close_agent_action_tool() {
    _current_agent = nullptr;
    _current_editor = nullptr;

    if (agent_action_tool_window) {
        agent_action_tool_window->hide();
        agent_action_tool_window->queue_free();
        agent_action_tool_window = nullptr;
    }
}

void IdeamGamesPlugin::validate_game_script_templates() {
    const godot::String base_res = "res://addons/ideam_games/resources/";
    register_script_template("Game", "default_template", base_res + String("default_game_template.txt"));
    register_script_template("GameAgent", "default_template", base_res + String("default_game_agent_template.txt"));
    register_script_template("GameHub", "default_template", base_res + String("default_game_hub_template.txt"));
    register_script_template("GameBoard", "default_template", base_res + String("default_game_board_template.txt"));
    register_script_template("GameEntity", "default_template", base_res + String("default_game_entity_template.txt"));
    register_script_template("GamePiece", "default_template", base_res + String("default_game_piece_template.txt"));
    register_script_template("GamePlayer", "default_template", base_res + String("default_game_player_template.txt"));
}

void IdeamGamesPlugin::_register_project_contribution() {
    IdeamProjectContributionRegistry::get_singleton().register_manifest_contribution({
        godot::StringName("IdeamGames"),
        "Games",
        10,
        []() -> godot::Ref<ProjectModuleManifest> {
            godot::Ref<ProjectModuleManifest> mod;
            mod.instantiate();
            mod->set_module_id("ideam_games_core");
            mod->set_display_name("Ideam Games Core");
            mod->set_plugin_domain("Games");
            mod->set_description("Scaffolds GameHub, Game, starter GameplayStyle resource, and core folders.");
            mod->add_folder("scripts/game");
            mod->add_folder("resources/gameplay");
            mod->add_folder("scenes/game");

            godot::Ref<ProjectScriptSpec> hub_script;
            hub_script.instantiate();
            hub_script->set_target_folder("scripts/game");
            hub_script->set_script_name("main_game_hub");
            hub_script->set_base_class("GameHub");
            hub_script->set_template_name("default_template");
            mod->add_script_spec(hub_script);

            godot::Ref<ProjectResourceSpec> style_res;
            style_res.instantiate();
            style_res->set_target_folder("resources/gameplay");
            style_res->set_resource_name("default_gameplay_style");
            style_res->set_resource_class("GameplayStyle");
            mod->add_resource_spec(style_res);

            godot::Ref<ProjectSceneSpec> hub_scene;
            hub_scene.instantiate();
            hub_scene->set_target_folder("scenes/game");
            hub_scene->set_scene_name("main_game_hub");
            godot::Ref<ProjectSceneNodeSpec> root_node = hub_scene->get_root_node();
            root_node->set_node_name("GameHub");
            root_node->set_node_class("GameHub");
            root_node->set_use_generated_script(true);
            root_node->set_generated_script_key("GameHub");
            mod->add_scene_spec(hub_scene);

            return mod;
        }
    });
}

static godot::Ref<GameplayControlMap> _resolve_control_map_from_style(
    const godot::Ref<GameplayStyle>& p_style
) {
    if (p_style.is_null()) {
        return godot::Ref<GameplayControlMap>();
    }
    godot::TypedArray<GameplayMode> modes = p_style->get_modes();
    for (int i = 0; i < modes.size(); ++i) {
        GameplayMode* mode = godot::Object::cast_to<GameplayMode>(modes[i]);
        if (!mode || !mode->get_enabled()) {
            continue;
        }
        godot::TypedArray<GameplayControlMap> control_maps = mode->get_control_maps();
        for (int j = 0; j < control_maps.size(); ++j) {
            GameplayControlMap* cmap = godot::Object::cast_to<GameplayControlMap>(control_maps[j]);
            if (cmap && cmap->get_enabled()) {
                return godot::Ref<GameplayControlMap>(cmap);
            }
        }
    }
    return godot::Ref<GameplayControlMap>();
}

void IdeamGamesPlugin::_scene_change(godot::Node* new_root) {
    if (agent_action_editor.is_valid()) {
        agent_action_editor->set_editor_root(godot::Object::cast_to<godot::Control>(new_root));
    }

    if (cascade_dock) {
        cascade_dock->set_scene_root(new_root);
        cascade_dock->inspect_node(new_root);
    }

    if (gameplay_debugger.is_valid() && new_root) {
        godot::Ref<GameplayStyle> style;
        if (Game* game = godot::Object::cast_to<Game>(new_root)) {
            style = game->get_gameplay_style();
            if (style.is_null()) {
                style = game->get_resolved_gameplay_style();
            }
        } else if (GameEntity* entity = godot::Object::cast_to<GameEntity>(new_root)) {
            style = entity->get_gameplay_style();
            if (style.is_null()) {
                style = entity->get_resolved_gameplay_style();
            }
        }

        gameplay_debugger->set_target_style(style);
        gameplay_debugger->set_target_control_map(_resolve_control_map_from_style(style));
    }
}

void IdeamGamesPlugin::_on_editor_selection_changed() {
    _sync_cascade_dock_with_selection();
}

void IdeamGamesPlugin::_sync_cascade_dock_with_selection() {
    if (!cascade_dock || !get_editor_interface()) {
        return;
    }

    godot::EditorSelection* sel = get_editor_interface()->get_selection();
    if (!sel) {
        return;
    }

    godot::TypedArray<godot::Node> selected_nodes = sel->get_selected_nodes();
    if (!selected_nodes.is_empty()) {
        godot::Node* primary_node = godot::Object::cast_to<godot::Node>(selected_nodes[0]);
        if (primary_node) {
            cascade_dock->inspect_node(primary_node);

            // Walk ancestors to find owning Game or GameEntity's GameplayStyle
            godot::Node* curr = primary_node;
            while (curr != nullptr) {
                godot::Ref<GameplayStyle> style;
                if (Game* game = godot::Object::cast_to<Game>(curr)) {
                    style = game->get_gameplay_style();
                    if (style.is_null()) {
                        style = game->get_resolved_gameplay_style();
                    }
                } else if (GameEntity* entity = godot::Object::cast_to<GameEntity>(curr)) {
                    style = entity->get_gameplay_style();
                    if (style.is_null()) {
                        style = entity->get_resolved_gameplay_style();
                    }
                }

                if (style.is_valid()) {
                    if (gameplay_debugger.is_valid()) {
                        gameplay_debugger->set_target_style(style);
                        gameplay_debugger->set_target_control_map(
                            _resolve_control_map_from_style(style)
                        );
                    }
                    break;
                }
                curr = curr->get_parent();
            }
        }
    }

}

void IdeamGamesPlugin::register_task_domain_pack() {
    if (auto* task_reg = core::IdeamTaskRegistry::get_singleton()) {
        if (!task_reg->has_domain_pack("IdeamGames")) {
            core::GamesTaskDomainPack::register_domain();
        }
        task_reg->set_domain_pack_active("IdeamGames", true);
    }
}
// --- Static Godot API ---

godot::Window* IdeamGamesPlugin::open_action_tool_window(GameAgent* agent, godot::Object* editor) {
    if (singleton) {
        singleton->_current_agent = agent;
        singleton->_current_editor = editor;
        singleton->open_agent_action_tool();
        return singleton->agent_action_tool_window;
    }
    return nullptr;
}

godot::EditorUndoRedoManager* IdeamGamesPlugin::get_static_undo_redo() {
    if (singleton) {
        return singleton->get_undo_redo();
    }
    return nullptr;
}

} // namespace ideam::godot_ext