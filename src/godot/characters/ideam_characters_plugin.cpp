#include "ideam_characters_plugin.h"

#include "../controls/ideam_tier1_control_registry.h"
#include "character_blueprint_resource.h"
#include "ui/character_modeler.h"
#include "ui/character_morph_slider_panel.h"

#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

IdeamCharactersPlugin *IdeamCharactersPlugin::singleton = nullptr;

void IdeamCharactersPlugin::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("get_plugin_domain_name"), &IdeamCharactersPlugin::get_plugin_domain_name);
    godot::ClassDB::bind_method(godot::D_METHOD("is_splines_handshake_available"), &IdeamCharactersPlugin::is_splines_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_design_handshake_available"), &IdeamCharactersPlugin::is_design_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_geometry_handshake_available"), &IdeamCharactersPlugin::is_geometry_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_linguistics_handshake_available"), &IdeamCharactersPlugin::is_linguistics_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_games_handshake_available"), &IdeamCharactersPlugin::is_games_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_tasks_handshake_available"), &IdeamCharactersPlugin::is_tasks_handshake_available);

#ifdef TOOLS_ENABLED
    godot::ClassDB::bind_static_method(
        "IdeamCharactersPlugin",
        godot::D_METHOD("undo_redo"),
        &IdeamCharactersPlugin::get_static_undo_redo
    );
#endif
    godot::ClassDB::bind_static_method(
        "IdeamCharactersPlugin",
        godot::D_METHOD("create_undo_redo_wrapper"),
        &IdeamCharactersPlugin::create_undo_redo_wrapper
    );
}

IdeamCharactersPlugin::IdeamCharactersPlugin() {
    if (singleton == nullptr) {
        singleton = this;
    }
}

IdeamCharactersPlugin::~IdeamCharactersPlugin() {
    if (singleton == this) {
        singleton = nullptr;
    }
}

void IdeamCharactersPlugin::_enter_tree() {
    validate_script_templates();
    set_plugin_active("IdeamCharacters", true);
    register_to_ecosystem("WizardSettings", "IdeamCharacters", godot::String(CHARACTERS_SETTINGS_PATH.data()));
    inspector_plugin_.instantiate();
    add_inspector_plugin(inspector_plugin_);

    auto &reg = IdeamTier1ControlRegistry::get_singleton();
    const godot::StringName ws_id(CharacterModeler::WORKSPACE_ID);

    reg.register_workspace_launcher({
        ws_id,
        "Character Modeler",
        [](const godot::Ref<godot::Resource> &p_asset, const godot::Ref<IdeamToolContext> &p_ctx) {
            if (CharacterModeler *modeler = CharacterModeler::get_or_create_shared_modeler()) {
                if (p_ctx.is_valid()) modeler->set_tool_context(p_ctx);
                modeler->open_asset(p_asset);
            }
        }
    });

    reg.register_control({
        ws_id, "CharacterBlueprintResource", "Blueprints", "Character Blueprint", UPDATE_ON_COMMIT,
        [](const godot::Ref<godot::Resource> &p_asset, const godot::Ref<IdeamToolContext> &p_ctx) -> godot::Control * {
            using namespace godot;
            Ref<CharacterBlueprintResource> bp = p_asset;
            if (bp.is_null()) return nullptr;

            auto *scroll = memnew(ScrollContainer);
            scroll->set_h_size_flags(Control::SIZE_EXPAND_FILL);
            scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);

            auto *panel = memnew(CharacterMorphSliderPanel);
            panel->set_h_size_flags(Control::SIZE_EXPAND_FILL);
            if (p_ctx.is_valid() && p_ctx->get_undo_redo().is_valid()) {
                panel->set_undo_redo(p_ctx->get_undo_redo());
            } else {
                panel->set_undo_redo(IdeamCharactersPlugin::create_undo_redo_wrapper());
            }
            panel->set_blueprint(bp);
            scroll->add_child(panel);
            return scroll;
        },
        nullptr
    });

    reg.register_control({
        ws_id, "CharacterApparelResource", "Apparel", "Character Apparel", UPDATE_ON_COMMIT,
        [](const godot::Ref<godot::Resource> &p_asset, const godot::Ref<IdeamToolContext> &p_ctx) -> godot::Control * {
            using namespace godot;
            (void)p_ctx;
            if (p_asset.is_null()) return nullptr;

            auto *vbox = memnew(VBoxContainer);
            vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
            auto *hdr = memnew(Label);
            hdr->set_text("Character Apparel Asset — Edit garment layer, material, and refit parameters in the Inspector on the right.");
            hdr->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
            vbox->add_child(hdr);
            return vbox;
        },
        nullptr
    });
}

void IdeamCharactersPlugin::_exit_tree() {
    auto &reg = IdeamTier1ControlRegistry::get_singleton();
    const godot::StringName ws_id(CharacterModeler::WORKSPACE_ID);
    reg.unregister_class(ws_id, "CharacterBlueprintResource");
    reg.unregister_class(ws_id, "CharacterApparelResource");
    reg.unregister_workspace_launcher(ws_id);

    if (inspector_plugin_.is_valid()) {
        remove_inspector_plugin(inspector_plugin_);
        inspector_plugin_.unref();
    }
    set_plugin_active("IdeamCharacters", false);
}

#ifdef TOOLS_ENABLED
godot::EditorUndoRedoManager *IdeamCharactersPlugin::get_static_undo_redo() {
    if (singleton) {
        return singleton->get_undo_redo();
    }
    return nullptr;
}
#endif

godot::Ref<IdeamUndoRedo> IdeamCharactersPlugin::create_undo_redo_wrapper() {
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
