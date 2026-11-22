#include "ideam_characters_plugin.h"

#include "../tools/tool_descriptor.h"
#include "../tools/tool_registry.h"
#include "body_plan_resource.h"
#include "character_blueprint_resource.h"
#include "character_gallery_bank_resource.h"
#include "character_instance_resource.h"
#include "ui/body_plan_authoring_control.h"
#include "ui/character_generator_control.h"
#include "ui/character_modeler.h"
#include "ui/character_morph_slider_panel.h"

#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

namespace ideam::godot_ext {

IdeamCharactersPlugin *IdeamCharactersPlugin::singleton = nullptr;

void IdeamCharactersPlugin::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("open_character_modeler"), &IdeamCharactersPlugin::open_character_modeler);
    godot::ClassDB::bind_method(godot::D_METHOD("get_plugin_domain_name"), &IdeamCharactersPlugin::get_plugin_domain_name);
    godot::ClassDB::bind_method(godot::D_METHOD("is_splines_handshake_available"), &IdeamCharactersPlugin::is_splines_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_surfaces_handshake_available"), &IdeamCharactersPlugin::is_surfaces_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_design_handshake_available"), &IdeamCharactersPlugin::is_design_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_geometry_handshake_available"), &IdeamCharactersPlugin::is_geometry_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_linguistics_handshake_available"), &IdeamCharactersPlugin::is_linguistics_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_games_handshake_available"), &IdeamCharactersPlugin::is_games_handshake_available);
    godot::ClassDB::bind_method(godot::D_METHOD("is_tutorials_handshake_available"), &IdeamCharactersPlugin::is_tutorials_handshake_available);
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

    if (!inspector_plugin_.is_valid()) {
        inspector_plugin_.instantiate();
    }
    add_inspector_plugin(inspector_plugin_);

    _register_tools_and_workspace();

#ifdef TOOLS_ENABLED
    add_tool_menu_item(TOOLS_MENU_MODELER_ITEM, godot::Callable(this, "open_character_modeler"));
#endif
}

void IdeamCharactersPlugin::_exit_tree() {
#ifdef TOOLS_ENABLED
    remove_tool_menu_item(TOOLS_MENU_MODELER_ITEM);
#endif

    _unregister_tools_and_workspace();

    if (inspector_plugin_.is_valid()) {
        remove_inspector_plugin(inspector_plugin_);
        inspector_plugin_.unref();
    }
    set_plugin_active("IdeamCharacters", false);
}

void IdeamCharactersPlugin::open_character_modeler() {
    CharacterModeler::get_or_create_shared_modeler();
}

void IdeamCharactersPlugin::_register_tools_and_workspace() {
    auto &reg = ToolRegistry::get_singleton();
    const godot::StringName ws_id(CharacterModeler::WORKSPACE_ID);

    reg.register_workspace_launcher({
        ws_id,
        "Character Modeler",
        [](const godot::Ref<godot::Resource> &p_asset, const godot::Ref<ToolContext> &p_ctx) {
            if (CharacterModeler *modeler = CharacterModeler::get_or_create_shared_modeler()) {
                if (p_ctx.is_valid()) modeler->set_tool_context(p_ctx);
                if (p_asset.is_valid()) modeler->open_asset(p_asset);
            }
        }
    });

    auto make_authoring = [](const godot::Ref<godot::Resource> &p_asset, const godot::Ref<ToolContext> &p_ctx) -> godot::Control * {
        auto *ctrl = memnew(BodyPlanAuthoringControl);
        ctrl->bind_domain_asset(p_asset, p_ctx);
        return ctrl;
    };
    auto rebind_authoring = [](godot::Control *p_ctrl, const godot::Ref<godot::Resource> &p_asset, const godot::Ref<ToolContext> &p_ctx) {
        if (auto *a = godot::Object::cast_to<BodyPlanAuthoringControl>(p_ctrl)) {
            a->bind_domain_asset(p_asset, p_ctx);
        }
    };

    auto make_generator = [](const godot::Ref<godot::Resource> &p_asset, const godot::Ref<ToolContext> &p_ctx) -> godot::Control * {
        auto *ctrl = memnew(CharacterGeneratorControl);
        ctrl->bind_domain_asset(p_asset, p_ctx);
        return ctrl;
    };
    auto rebind_generator = [](godot::Control *p_ctrl, const godot::Ref<godot::Resource> &p_asset, const godot::Ref<ToolContext> &p_ctx) {
        if (auto *g = godot::Object::cast_to<CharacterGeneratorControl>(p_ctrl)) {
            g->bind_domain_asset(p_asset, p_ctx);
        }
    };

    reg.register_control({ws_id, "BodyPlanResource", "Body Plans", "Body Plan Schema", UPDATE_ON_COMMIT, make_authoring, rebind_authoring});
    reg.register_control({ws_id, "CharacterGalleryBankResource", "Gallery Banks", "Character Gallery Bank", UPDATE_ON_COMMIT, make_authoring, rebind_authoring});
    reg.register_control({ws_id, "CharacterInstanceResource", "Characters", "Character Instance", UPDATE_ON_COMMIT, make_generator, rebind_generator});

    reg.register_control({
        ws_id, "CharacterBlueprintResource", "Blueprints", "Character Blueprint", UPDATE_ON_COMMIT,
        [](const godot::Ref<godot::Resource> &p_asset, const godot::Ref<ToolContext> &p_ctx) -> godot::Control * {
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
        [](const godot::Ref<godot::Resource> &p_asset, const godot::Ref<ToolContext> &p_ctx) -> godot::Control * {
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

    auto add_desc = [&](const char *p_id, ToolTier p_tier, const char *p_name, const char *p_desc,
                        std::initializer_list<const char *> p_classes, std::initializer_list<const char *> p_tags) {
        godot::Ref<ToolDescriptor> d;
        d.instantiate();
        d->set_tool_id(godot::StringName(p_id));
        d->set_tier(p_tier);
        d->set_plugin_id("IdeamCharacters");
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

    add_desc("CharacterModeler", TOOL_TIER_BRIDGE, "Character Modeler (Dual Mode)",
             "Dual-mode ToolWorkspace switching between Plan Authoring (Loomis gallery banks & BodyPlan schemas) and Character Generator (bounded sliders, randomization, LOD).",
             {"BodyPlanResource", "CharacterInstanceResource", "CharacterGalleryBankResource", "CharacterBlueprintResource", "CharacterApparelResource"},
             {"characters", "loomis", "body_plan", "generator", "workspace"});

    add_desc("BodyPlanAuthoringControl", TOOL_TIER_DOMAIN, "BodyPlan Authoring & Gallery Banks",
             "Searchable ScaffoldBank, TopologyBank, FeatureBank, and DetailBank galleries with 1-click Loomis preset application.",
             {"BodyPlanResource", "CharacterGalleryBankResource"},
             {"characters", "body_plan", "gallery_banks", "loomis"});

    add_desc("CharacterGeneratorControl", TOOL_TIER_DOMAIN, "Character Generator",
             "Bounded parameter sliders, seeded randomization, live topological bridge regime telemetry, LOD toggle, and Layer 4 detail presets.",
             {"CharacterInstanceResource"},
             {"characters", "generator", "sliders", "randomize", "lod"});

    add_desc("CharacterGalleryBankResource", TOOL_TIER_SMART, "Character Gallery Bank Profile",
             "Tier 3 SmartToolProfile asset bank storing reusable Scaffold, Topology, Feature, and Detail presets.",
             {"CharacterGalleryBankResource"},
             {"characters", "smart_tool", "asset_bank", "presets"});
}

void IdeamCharactersPlugin::_unregister_tools_and_workspace() {
    auto &reg = ToolRegistry::get_singleton();
    const godot::StringName ws_id(CharacterModeler::WORKSPACE_ID);
    reg.unregister_workspace_controls(ws_id);
    reg.unregister_workspace_launcher(ws_id);
    reg.unregister_plugin_tools("IdeamCharacters");
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
