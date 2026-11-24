#include "character_modeler.h"

#include "body_plan_authoring_control.h"
#include "character_generator_control.h"
#include "../ideam_characters_plugin.h"
#include "../../tools/domain/domain_tool_interface.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

#ifdef TOOLS_ENABLED
#include <godot_cpp/classes/editor_interface.hpp>
#endif

using namespace godot;

namespace ideam::godot_ext {

CharacterModeler *CharacterModeler::shared_instance_ = nullptr;
Window *CharacterModeler::shared_window_             = nullptr;

void CharacterModeler::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_modeler_mode", "mode"), &CharacterModeler::set_modeler_mode);
    ClassDB::bind_method(D_METHOD("get_modeler_mode"), &CharacterModeler::get_modeler_mode);
    ClassDB::bind_method(D_METHOD("set_active_gallery_bank", "bank"), &CharacterModeler::set_active_gallery_bank);
    ClassDB::bind_method(D_METHOD("get_active_gallery_bank"), &CharacterModeler::get_active_gallery_bank);
    ClassDB::bind_method(D_METHOD("get_viewport_3d_host"), &CharacterModeler::get_viewport_3d_host);
    ClassDB::bind_method(D_METHOD("get_preview_node"), &CharacterModeler::get_preview_node);

    ClassDB::bind_method(D_METHOD("_on_mode_pressed", "mode"), &CharacterModeler::_on_mode_pressed);
    ClassDB::bind_method(D_METHOD("_on_new_body_plan_pressed"), &CharacterModeler::_on_new_body_plan_pressed);
    ClassDB::bind_method(D_METHOD("_on_new_character_pressed"), &CharacterModeler::_on_new_character_pressed);
    ClassDB::bind_method(D_METHOD("_on_new_gallery_bank_pressed"), &CharacterModeler::_on_new_gallery_bank_pressed);
    ClassDB::bind_method(D_METHOD("_on_spatial_mode_changed", "mode_id"), &CharacterModeler::_on_spatial_mode_changed);
    ClassDB::bind_method(D_METHOD("_on_spatial_toggle_changed", "toggle_id", "pressed"), &CharacterModeler::_on_spatial_toggle_changed);
    ClassDB::bind_method(D_METHOD("_on_preview_lod_changed", "lod_level"), &CharacterModeler::_on_preview_lod_changed);

    ClassDB::bind_static_method(
        "CharacterModeler",
        D_METHOD("open_character_resource", "resource", "owner"),
        &CharacterModeler::open_character_resource,
        DEFVAL(nullptr)
    );

    BIND_ENUM_CONSTANT(MODE_PLAN_AUTHORING);
    BIND_ENUM_CONSTANT(MODE_CHARACTER_GENERATOR);
}

CharacterModeler::CharacterModeler() {
    set_workspace_id(StringName(WORKSPACE_ID));
    set_help_topic_id(StringName("IdeamCharacters"));

    ensure_sidebar_tab("Body Plans");
    ensure_sidebar_tab("Characters");
    ensure_sidebar_tab("Gallery Banks");
    ensure_sidebar_tab("Apparel");

    active_gallery_bank_.instantiate();
    configure_theme_registry("res://addons/ideam_characters/resources/character_modeler_theme_registry.tres");

    if (tool_context_.is_valid()) {
        tool_context_->set_undo_redo(IdeamCharactersPlugin::create_undo_redo_wrapper());
    }

    _populate_command_bar(custom_actions_box_);
    _setup_spatial_studio();
}

CharacterModeler::~CharacterModeler() {
    gizmo_scaffold_.unref();
    gizmo_topology_.unref();
    gizmo_feature_.unref();
    if (shared_instance_ == this) {
        shared_instance_ = nullptr;
        shared_window_   = nullptr;
    }
}

void CharacterModeler::_setup_spatial_studio() {
    // 1. Mount the 4-tool IdeamSpatialToolbar into ToolWorkspace::ribbon_slot_
    if (ribbon_slot_ && !spatial_toolbar_) {
        spatial_toolbar_ = memnew(IdeamSpatialToolbar);
        spatial_toolbar_->set_tool_context(tool_context_);
        spatial_toolbar_->add_mode_button(
            StringName("scaffolding"),
            "Scaffolding",
            "Tool 1: Manipulate Loomis proportions, ruler canon, and sockets.",
            "characters.modeler.toolbar.scaffolding"
        );
        spatial_toolbar_->add_mode_button(
            StringName("topology"),
            "Topology",
            "Tool 2: Edit bony landmarks and seamless part seams.",
            "characters.modeler.toolbar.topology"
        );
        spatial_toolbar_->add_mode_button(
            StringName("features"),
            "Features",
            "Tool 3: Hold Shift for feature anchors; Shift+Click to isolate.",
            "characters.modeler.toolbar.features"
        );
        spatial_toolbar_->add_separator();
        spatial_toolbar_->add_toggle_button(
            StringName("details"),
            "Details",
            "Tool 4: Non-destructive toggle for asymmetric overrides and surface detail deltas.",
            true,
            "characters.modeler.toolbar.details_toggle"
        );
        spatial_toolbar_->set_active_mode(StringName("scaffolding"), false);
        spatial_toolbar_->connect("mode_changed", Callable(this, "_on_spatial_mode_changed"));
        spatial_toolbar_->connect("toggle_changed", Callable(this, "_on_spatial_toggle_changed"));

        ribbon_slot_->add_child(spatial_toolbar_);
        ribbon_slot_->set_visible(true);
    }

    // 2. Create reusable studio 3D SubViewport host with all 3 character gizmos
    if (!viewport_3d_host_) {
        viewport_3d_host_ = memnew(IdeamViewport3DHost);
        viewport_3d_host_->set_custom_minimum_size(Vector2(360.0f, 360.0f));
        viewport_3d_host_->set_h_size_flags(SIZE_EXPAND_FILL);
        viewport_3d_host_->set_v_size_flags(SIZE_EXPAND_FILL);
        viewport_3d_host_->set_tool_context(tool_context_);

        preview_node_ = memnew(LoomisCharacterNode3D);
        preview_node_->set_name("StudioCharacterPreview");
        viewport_3d_host_->attach_preview_node(preview_node_, true);

        gizmo_scaffold_.instantiate();
        gizmo_scaffold_->set_tool_context(tool_context_);
        gizmo_scaffold_->set_active_tool_mode(StringName("scaffolding"));

        gizmo_topology_.instantiate();
        gizmo_topology_->set_tool_context(tool_context_);
        gizmo_topology_->set_active_tool_mode(StringName("scaffolding"));

        gizmo_feature_.instantiate();
        gizmo_feature_->set_tool_context(tool_context_);
        gizmo_feature_->set_active_tool_mode(StringName("scaffolding"));

        if (IdeamGizmoSession *session = viewport_3d_host_->get_gizmo_session()) {
            session->add_gizmo(gizmo_scaffold_);
            session->add_gizmo(gizmo_topology_);
            session->add_gizmo(gizmo_feature_);
        }
    }
}

void CharacterModeler::_sync_preview_character() {
    if (!preview_node_) return;

    if (active_character_.is_valid()) {
        preview_node_->set_character_resource(active_character_);
        if (spatial_toolbar_) {
            spatial_toolbar_->set_toggle_state(
                StringName("details"),
                active_character_->get_details_enabled(),
                false
            );
        }
    } else if (active_body_plan_.is_valid()) {
        Ref<LoomisCharacter3DResource> temp_inst;
        temp_inst.instantiate();
        temp_inst->set_body_plan(active_body_plan_);
        temp_inst->set_character_name(active_body_plan_->get_species_name() + String(" Preview"));
        preview_node_->set_character_resource(temp_inst);
    }

    if (viewport_3d_host_ && viewport_3d_host_->get_gizmo_session()) {
        viewport_3d_host_->get_gizmo_session()->set_target_node(preview_node_);
        viewport_3d_host_->get_gizmo_session()->rebuild_gizmos_now();
    }
}

void CharacterModeler::_populate_command_bar(HBoxContainer *p_custom_actions) {
    if (!p_custom_actions || btn_mode_authoring_ != nullptr) return;

    auto add_mode_btn = [&](const char *p_label, const char *p_sel, ModelerMode p_mode) -> Button * {
        Button *b = memnew(Button);
        b->set_text(p_label);
        domain_tool::stamp_ui_selector(b, p_sel);
        b->connect("pressed", Callable(this, "_on_mode_pressed").bind(static_cast<int>(p_mode)));
        p_custom_actions->add_child(b);
        return b;
    };

    auto add_create_btn = [&](const char *p_label, const char *p_sel, const char *p_method) -> Button * {
        Button *b = memnew(Button);
        b->set_text(p_label);
        domain_tool::stamp_ui_selector(b, p_sel);
        b->connect("pressed", Callable(this, p_method));
        p_custom_actions->add_child(b);
        return b;
    };

    btn_mode_authoring_ = add_mode_btn("Plan Authoring",      "characters.modeler.mode_plan_authoring", MODE_PLAN_AUTHORING);
    btn_mode_generator_ = add_mode_btn("Character Generator", "characters.modeler.mode_generator",      MODE_CHARACTER_GENERATOR);
    btn_new_plan_       = add_create_btn("+ BodyPlan",        "characters.modeler.new_body_plan",       "_on_new_body_plan_pressed");
    btn_new_character_  = add_create_btn("+ Character",       "characters.modeler.new_character",       "_on_new_character_pressed");
    btn_new_bank_       = add_create_btn("+ Gallery Bank",    "characters.modeler.new_gallery_bank",    "_on_new_gallery_bank_pressed");
}

Control *CharacterModeler::_create_domain_control_for_asset(const Ref<Resource> &p_asset) {
    if (!p_asset.is_valid()) return nullptr;

    Control *domain_ctrl = nullptr;

    if (Ref<BodyPlanResource> bp = p_asset; bp.is_valid()) {
        active_body_plan_ = bp;
        if (active_mode_ == MODE_CHARACTER_GENERATOR) {
            if (!active_character_.is_valid() || active_character_->get_body_plan() != bp) {
                active_character_.instantiate();
                active_character_->set_body_plan(bp);
                active_character_->set_character_name(bp->get_species_name() + String(" Instance"));
            }
            auto *gen = memnew(CharacterGeneratorControl);
            gen->set_gallery_bank(active_gallery_bank_);
            gen->bind_domain_asset(active_character_, tool_context_);
            gen->connect("lod_changed", Callable(this, "_on_preview_lod_changed"));
            domain_ctrl = gen;
        } else {
            auto *auth = memnew(BodyPlanAuthoringControl);
            auth->set_gallery_bank(active_gallery_bank_);
            auth->bind_domain_asset(bp, tool_context_);
            domain_ctrl = auth;
        }
    } else if (Ref<LoomisCharacter3DResource> ci = p_asset; ci.is_valid()) {
        active_character_ = ci;
        if (ci->get_body_plan().is_valid()) active_body_plan_ = ci->get_body_plan();
        if (active_mode_ == MODE_PLAN_AUTHORING && ci->get_body_plan().is_valid()) {
            auto *auth = memnew(BodyPlanAuthoringControl);
            auth->set_gallery_bank(active_gallery_bank_);
            auth->bind_domain_asset(ci->get_body_plan(), tool_context_);
            domain_ctrl = auth;
        } else {
            auto *gen = memnew(CharacterGeneratorControl);
            gen->set_gallery_bank(active_gallery_bank_);
            gen->bind_domain_asset(ci, tool_context_);
            gen->connect("lod_changed", Callable(this, "_on_preview_lod_changed"));
            domain_ctrl = gen;
        }
    } else if (Ref<CharacterGalleryBankResource> bank = p_asset; bank.is_valid()) {
        active_gallery_bank_ = bank;
        auto *auth = memnew(BodyPlanAuthoringControl);
        auth->set_gallery_bank(bank);
        if (active_body_plan_.is_valid()) auth->bind_domain_asset(active_body_plan_, tool_context_);
        else auth->bind_domain_asset(bank, tool_context_);
        domain_ctrl = auth;
    } else {
        return ToolWorkspace::_create_domain_control_for_asset(p_asset);
    }

    _sync_preview_character();

    // Wrap domain control beside the shared live 3D studio viewport
    if (viewport_3d_host_) {
        if (Node *old_parent = viewport_3d_host_->get_parent()) {
            old_parent->remove_child(viewport_3d_host_);
        }
        auto *split = memnew(HSplitContainer);
        split->set_h_size_flags(SIZE_EXPAND_FILL);
        split->set_v_size_flags(SIZE_EXPAND_FILL);
        split->add_child(viewport_3d_host_);
        split->add_child(domain_ctrl);
        return split;
    }

    return domain_ctrl;
}

void CharacterModeler::_on_active_asset_switched(const Ref<Resource> &p_asset) {
    if (Ref<BodyPlanResource> bp = p_asset; bp.is_valid()) {
        active_body_plan_ = bp;
    } else if (Ref<LoomisCharacter3DResource> ci = p_asset; ci.is_valid()) {
        active_character_ = ci;
        if (ci->get_body_plan().is_valid()) active_body_plan_ = ci->get_body_plan();
    } else if (Ref<CharacterGalleryBankResource> bank = p_asset; bank.is_valid()) {
        active_gallery_bank_ = bank;
    }
    _sync_preview_character();
}

void CharacterModeler::set_modeler_mode(ModelerMode p_mode) {
    if (active_mode_ == p_mode) return;
    active_mode_ = p_mode;
    _rebuild_active_view();
}

void CharacterModeler::set_active_gallery_bank(const Ref<CharacterGalleryBankResource> &p_bank) {
    if (p_bank.is_valid()) {
        active_gallery_bank_ = p_bank;
        _rebuild_active_view();
    }
}

void CharacterModeler::_rebuild_active_view() {
    if (!active_asset_.is_valid()) return;
    for (auto &entry : open_assets_) {
        if (entry.asset == active_asset_) {
            if (viewport_3d_host_ && viewport_3d_host_->get_parent()) {
                viewport_3d_host_->get_parent()->remove_child(viewport_3d_host_);
            }
            if (entry.domain_control) {
                if (entry.domain_control->get_parent()) {
                    entry.domain_control->get_parent()->remove_child(entry.domain_control);
                }
                entry.domain_control->queue_free();
                entry.domain_control = nullptr;
            }
            _show_asset_in_center(entry);
            break;
        }
    }
}

void CharacterModeler::_on_mode_pressed(int p_mode) {
    set_modeler_mode(static_cast<ModelerMode>(p_mode));
}

void CharacterModeler::_on_new_body_plan_pressed() {
    Ref<BodyPlanResource> bp;
    bp.instantiate();
    bp->set_species_name("Loomis Humanoid");
    active_mode_ = MODE_PLAN_AUTHORING;
    open_asset(bp, "Body Plans");
}

void CharacterModeler::_on_new_character_pressed() {
    if (!active_body_plan_.is_valid()) {
        active_body_plan_.instantiate();
    }
    Ref<LoomisCharacter3DResource> ci;
    ci.instantiate();
    ci->set_body_plan(active_body_plan_);
    ci->set_character_name(active_body_plan_->get_species_name() + String(" Character"));
    active_mode_ = MODE_CHARACTER_GENERATOR;
    open_asset(ci, "Characters");
}

void CharacterModeler::_on_new_gallery_bank_pressed() {
    Ref<CharacterGalleryBankResource> bank;
    bank.instantiate();
    active_gallery_bank_ = bank;
    active_mode_ = MODE_PLAN_AUTHORING;
    open_asset(bank, "Gallery Banks");
}

void CharacterModeler::_on_spatial_mode_changed(const StringName &p_mode_id) {
    if (gizmo_scaffold_.is_valid()) gizmo_scaffold_->set_active_tool_mode(p_mode_id);
    if (gizmo_topology_.is_valid()) gizmo_topology_->set_active_tool_mode(p_mode_id);
    if (gizmo_feature_.is_valid()) gizmo_feature_->set_active_tool_mode(p_mode_id);
    if (viewport_3d_host_ && viewport_3d_host_->get_gizmo_session()) {
        viewport_3d_host_->get_gizmo_session()->rebuild_gizmos_now();
    }
}

void CharacterModeler::_on_spatial_toggle_changed(const StringName &p_toggle_id, bool p_pressed) {
    if (p_toggle_id != StringName("details") || !preview_node_) return;
    if (auto inst = preview_node_->get_character_resource(); inst.is_valid()) {
        if (tool_context_.is_valid() && tool_context_->get_undo_redo().is_valid()) {
            auto ur = tool_context_->get_undo_redo();
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

void CharacterModeler::_on_preview_lod_changed(int p_lod_level) {
    if (preview_node_) {
        preview_node_->set_lod_level(p_lod_level);
    }
}

CharacterModeler *CharacterModeler::get_or_create_shared_modeler(Control *p_owner) {
    if (p_owner) {
        for (int i = 0; i < p_owner->get_child_count(); ++i) {
            if (auto *existing = Object::cast_to<CharacterModeler>(p_owner->get_child(i))) {
                return existing;
            }
        }
        auto *embedded = memnew(CharacterModeler);
        p_owner->add_child(embedded);
        return embedded;
    }

    if (shared_instance_ && shared_window_) {
        shared_window_->popup_centered();
        shared_window_->grab_focus();
        return shared_instance_;
    }

    shared_instance_ = memnew(CharacterModeler);
    shared_window_   = ToolWorkspace::create_workspace_window(
        shared_instance_,
        "Ideam CharacterModeler",
        Vector2i(1280, 780)
    );

    Node *host_parent = nullptr;
#ifdef TOOLS_ENABLED
    if (Engine::get_singleton()->is_editor_hint()) {
        if (auto *ei = EditorInterface::get_singleton()) {
            host_parent = ei->get_base_control();
        }
    }
#endif
    // Runtime Context 4 fallback: attach floating modeler window to SceneTree root
    if (!host_parent) {
        if (auto *tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop())) {
            host_parent = tree->get_root();
        }
    }
    if (host_parent && !shared_window_->get_parent()) {
        host_parent->add_child(shared_window_);
    }
    shared_window_->popup_centered();
    return shared_instance_;
}

void CharacterModeler::open_character_resource(const Ref<Resource> &p_resource, Control *p_owner) {
    if (p_resource.is_null()) {
        return;
    }
    if (auto *modeler = get_or_create_shared_modeler(p_owner)) {
        modeler->open_asset(p_resource);
    }
}

} // namespace ideam::godot_ext
