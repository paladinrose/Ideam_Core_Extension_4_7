#include "ideam_characters_plugin.h"

#include "../tools/tool_descriptor.h"
#include "../tools/tool_registry.h"
#include "body_plan_resource.h"
#include "character_apparel_resource.h"
#include "character_gallery_bank_resource.h"
#include "loomis_character_3d_resource.h"
#include "loomis_character_node_3d.h"
#include "ui/body_plan_authoring_control.h"
#include "ui/character_generator_control.h"
#include "ui/character_modeler.h"
#include "ui/character_morph_slider_panel.h"

#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>
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

    godot::ClassDB::bind_method(godot::D_METHOD("_on_spatial_mode_changed", "mode_id"), &IdeamCharactersPlugin::_on_spatial_mode_changed);
    godot::ClassDB::bind_method(godot::D_METHOD("_on_spatial_toggle_changed", "toggle_id", "pressed"), &IdeamCharactersPlugin::_on_spatial_toggle_changed);

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

LoomisCharacterNode3D *IdeamCharactersPlugin::_resolve_selected_character() const {
    if (selected_character_id_ == 0u) return nullptr;
    return godot::Object::cast_to<LoomisCharacterNode3D>(godot::ObjectDB::get_instance(selected_character_id_));
}

void IdeamCharactersPlugin::_on_spatial_mode_changed(const godot::StringName &p_mode_id) {
    if (character_scaffold_gizmo_.is_valid()) {
        character_scaffold_gizmo_->set_active_tool_mode(p_mode_id);
    }
    if (character_topology_gizmo_.is_valid()) {
        character_topology_gizmo_->set_active_tool_mode(p_mode_id);
    }
    if (character_feature_gizmo_.is_valid()) {
        character_feature_gizmo_->set_active_tool_mode(p_mode_id);
    }
    if (LoomisCharacterNode3D *ch = _resolve_selected_character()) {
        ch->update_gizmos();
    }
}

void IdeamCharactersPlugin::_on_spatial_toggle_changed(const godot::StringName &p_toggle_id, bool p_pressed) {
    if (p_toggle_id != godot::StringName("details")) {
        return;
    }
    if (LoomisCharacterNode3D *ch = _resolve_selected_character()) {
        if (auto inst = ch->get_character_resource(); inst.is_valid()) {
            godot::Ref<IdeamUndoRedo> ur = get_or_create_plugin_tool_context()->get_undo_redo();
            if (ur.is_valid()) {
                const bool old_val = inst->get_details_enabled();
                if (old_val != p_pressed) {
                    ur->create_action(p_pressed ? "Enable Character Details" : "Disable Character Details");
                    ur->add_do_method(inst.ptr(), "set_details_enabled", p_pressed);
                    ur->add_undo_method(inst.ptr(), "set_details_enabled", old_val);
                    ur->commit_action();
                }
            } else {
                inst->set_details_enabled(p_pressed);
            }
        }
    }
}

void IdeamCharactersPlugin::_enter_tree() {
    validate_script_templates();
    set_plugin_active("IdeamCharacters", true);
    register_to_ecosystem("WizardSettings", "IdeamCharacters", godot::String(CHARACTERS_SETTINGS_PATH.data()));

    godot::Ref<ToolContext> ctx = get_or_create_plugin_tool_context();

    if (!inspector_plugin_.is_valid()) {
        inspector_plugin_.instantiate();
    }
    inspector_plugin_->set_tool_context(ctx);
    add_inspector_plugin(inspector_plugin_);

    // Register Tool 1: Scaffolding Gizmo
    if (!character_scaffold_gizmo_.is_valid()) {
        character_scaffold_gizmo_.instantiate();
    }
    character_scaffold_gizmo_->set_tool_context(ctx);
    character_scaffold_gizmo_->set_active_tool_mode(godot::StringName("scaffolding"));
    register_ideam_gizmo(character_scaffold_gizmo_);

    // Register Tool 2: Topology & Seam Gizmo
    if (!character_topology_gizmo_.is_valid()) {
        character_topology_gizmo_.instantiate();
    }
    character_topology_gizmo_->set_tool_context(ctx);
    character_topology_gizmo_->set_active_tool_mode(godot::StringName("scaffolding"));
    register_ideam_gizmo(character_topology_gizmo_);

    // Register Tool 3: Feature Shift-Focus Gizmo
    if (!character_feature_gizmo_.is_valid()) {
        character_feature_gizmo_.instantiate();
    }
    character_feature_gizmo_->set_tool_context(ctx);
    character_feature_gizmo_->set_active_tool_mode(godot::StringName("scaffolding"));
    register_ideam_gizmo(character_feature_gizmo_);

    // Mount 4-tool IdeamSpatialToolbar in CONTAINER_SPATIAL_EDITOR_MENU (hidden until LoomisCharacterNode3D selected)
    spatial_toolbar_ = memnew(IdeamSpatialToolbar);
    spatial_toolbar_->set_tool_context(ctx);
    spatial_toolbar_->add_mode_button(
        godot::StringName("scaffolding"),
        "Scaffolding",
        "Tool 1: Manipulate Loomis volumetric proportions, ruler canon, and sockets within BodyPlan limits.",
        "characters.spatial_toolbar.scaffolding"
    );
    spatial_toolbar_->add_mode_button(
        godot::StringName("topology"),
        "Topology",
        "Tool 2: Edit scaffold-anchored Spline3D forms, bony landmarks, and seamless part seams.",
        "characters.spatial_toolbar.topology"
    );
    spatial_toolbar_->add_mode_button(
        godot::StringName("features"),
        "Features",
        "Tool 3: Hold Shift to view major feature anchors; Shift+Click to isolate detail gizmos.",
        "characters.spatial_toolbar.features"
    );
    spatial_toolbar_->add_separator();
    spatial_toolbar_->add_toggle_button(
        godot::StringName("details"),
        "Details",
        "Tool 4: Non-destructive toggle for asymmetric overrides and surface detail deltas.",
        true,
        "characters.spatial_toolbar.details_toggle"
    );
    spatial_toolbar_->set_active_mode(godot::StringName("scaffolding"), false);
    spatial_toolbar_->connect("mode_changed", godot::Callable(this, "_on_spatial_mode_changed"));
    spatial_toolbar_->connect("toggle_changed", godot::Callable(this, "_on_spatial_toggle_changed"));
    spatial_toolbar_->set_visible(false);
    register_spatial_toolbar(spatial_toolbar_);

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

    cleanup_ideam_spatial_registrations();
    spatial_toolbar_ = nullptr;
    character_scaffold_gizmo_.unref();
    character_topology_gizmo_.unref();
    character_feature_gizmo_.unref();
    selected_character_id_ = 0u;

    if (inspector_plugin_.is_valid()) {
        remove_inspector_plugin(inspector_plugin_);
        inspector_plugin_.unref();
    }
    set_plugin_active("IdeamCharacters", false);
}

bool IdeamCharactersPlugin::_handles(godot::Object *p_object) const {
    return godot::Object::cast_to<LoomisCharacterNode3D>(p_object) != nullptr;
}

void IdeamCharactersPlugin::_edit(godot::Object *p_object) {
    auto *ch = godot::Object::cast_to<LoomisCharacterNode3D>(p_object);
    selected_character_id_ = ch ? ch->get_instance_id() : 0u;

    if (spatial_toolbar_) {
        if (ch && ch->get_character_resource().is_valid()) {
            spatial_toolbar_->set_toggle_state(
                godot::StringName("details"),
                ch->get_character_resource()->get_details_enabled(),
                false
            );
        }
    }
}

void IdeamCharactersPlugin::_make_visible(bool p_visible) {
    if (spatial_toolbar_) {
        spatial_toolbar_->set_visible(p_visible && selected_character_id_ != 0u);
    }
}

int32_t IdeamCharactersPlugin::_forward_3d_gui_input(
    godot::Camera3D *p_viewport_camera,
    const godot::Ref<godot::InputEvent> &p_event
) {
    return forward_gizmos_3d_gui_input(p_viewport_camera, p_event, _resolve_selected_character());
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
    reg.register_control({ws_id, "LoomisCharacter3DResource", "Characters", "Loomis Character 3D", UPDATE_ON_COMMIT, make_generator, rebind_generator});

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
             "Dual-mode ToolWorkspace with embedded 3D studio viewport, 4-tool spatial ribbon, Plan Authoring, and Character Generator.",
             {"BodyPlanResource", "LoomisCharacter3DResource", "CharacterGalleryBankResource", "CharacterApparelResource"},
             {"characters", "loomis", "body_plan", "generator", "workspace"});

    add_desc("BodyPlanAuthoringControl", TOOL_TIER_DOMAIN, "BodyPlan Authoring & Gallery Banks",
             "Searchable ScaffoldBank, TopologyBank, FeatureBank, and DetailBank galleries with 1-click Loomis preset application.",
             {"BodyPlanResource", "CharacterGalleryBankResource"},
             {"characters", "body_plan", "gallery_banks", "loomis"});

    add_desc("CharacterGeneratorControl", TOOL_TIER_DOMAIN, "Character Generator",
             "Bounded parameter sliders, seeded randomization, live topological bridge regime telemetry, LOD toggle, and Layer 4 detail controls.",
             {"LoomisCharacter3DResource"},
             {"characters", "generator", "sliders", "randomize", "lod"});

    add_desc("CharacterScaffoldGizmo", TOOL_TIER_DOMAIN, "3D Character Scaffolding Gizmo (Tool 1)",
             "Trans-environment BodyPlan-clamped scaffolding gizmo with canonical proportion ruler handle, balance plumb line, and volume handles.",
             {"LoomisCharacterNode3D"},
             {"characters", "loomis", "scaffolding", "gizmo"});

    add_desc("CharacterTopologyGizmo", TOOL_TIER_DOMAIN, "3D Character Topology & Seam Gizmo (Tool 2)",
             "Trans-environment bony landmark handles and PartSeamContract boundary ring overlays with crease-aware quad mesh updates.",
             {"LoomisCharacterNode3D"},
             {"characters", "topology", "seams", "landmarks", "gizmo"});

    add_desc("CharacterFeatureGizmo", TOOL_TIER_DOMAIN, "3D Character Feature Shift-Focus Gizmo (Tool 3)",
             "Hold Shift for major feature overview anchors; Shift+Click to isolate a feature and manipulate its detail sub-handles.",
             {"LoomisCharacterNode3D"},
             {"characters", "features", "shift_focus", "gizmo"});

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
