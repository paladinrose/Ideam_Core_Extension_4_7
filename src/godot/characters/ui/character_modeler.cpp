#include "character_modeler.h"

#include "body_plan_authoring_control.h"
#include "character_generator_control.h"
#include "../ideam_characters_plugin.h"
#include "../../tools/domain/domain_tool_interface.h"

#include <godot_cpp/classes/engine.hpp>
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

    ClassDB::bind_method(D_METHOD("_on_mode_pressed", "mode"), &CharacterModeler::_on_mode_pressed);
    ClassDB::bind_method(D_METHOD("_on_new_body_plan_pressed"), &CharacterModeler::_on_new_body_plan_pressed);
    ClassDB::bind_method(D_METHOD("_on_new_character_pressed"), &CharacterModeler::_on_new_character_pressed);
    ClassDB::bind_method(D_METHOD("_on_new_gallery_bank_pressed"), &CharacterModeler::_on_new_gallery_bank_pressed);

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
    ensure_sidebar_tab("Blueprints");
    ensure_sidebar_tab("Apparel");

    active_gallery_bank_.instantiate();
    configure_theme_registry("res://addons/ideam_characters/resources/character_modeler_theme_registry.tres");

    if (tool_context_.is_valid()) {
        tool_context_->set_undo_redo(IdeamCharactersPlugin::create_undo_redo_wrapper());
    }

    _populate_command_bar(custom_actions_box_);
}

CharacterModeler::~CharacterModeler() {
    if (shared_instance_ == this) {
        shared_instance_ = nullptr;
        shared_window_   = nullptr;
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
            return gen;
        }
        auto *auth = memnew(BodyPlanAuthoringControl);
        auth->set_gallery_bank(active_gallery_bank_);
        auth->bind_domain_asset(bp, tool_context_);
        return auth;
    }

    if (Ref<CharacterInstanceResource> ci = p_asset; ci.is_valid()) {
        active_character_ = ci;
        if (ci->get_body_plan().is_valid()) active_body_plan_ = ci->get_body_plan();
        if (active_mode_ == MODE_PLAN_AUTHORING && ci->get_body_plan().is_valid()) {
            auto *auth = memnew(BodyPlanAuthoringControl);
            auth->set_gallery_bank(active_gallery_bank_);
            auth->bind_domain_asset(ci->get_body_plan(), tool_context_);
            return auth;
        }
        auto *gen = memnew(CharacterGeneratorControl);
        gen->set_gallery_bank(active_gallery_bank_);
        gen->bind_domain_asset(ci, tool_context_);
        return gen;
    }

    if (Ref<CharacterGalleryBankResource> bank = p_asset; bank.is_valid()) {
        active_gallery_bank_ = bank;
        auto *auth = memnew(BodyPlanAuthoringControl);
        auth->set_gallery_bank(bank);
        if (active_body_plan_.is_valid()) auth->bind_domain_asset(active_body_plan_, tool_context_);
        else auth->bind_domain_asset(bank, tool_context_);
        return auth;
    }

    return ToolWorkspace::_create_domain_control_for_asset(p_asset);
}

void CharacterModeler::_on_active_asset_switched(const Ref<Resource> &p_asset) {
    if (Ref<BodyPlanResource> bp = p_asset; bp.is_valid()) {
        active_body_plan_ = bp;
    } else if (Ref<CharacterInstanceResource> ci = p_asset; ci.is_valid()) {
        active_character_ = ci;
        if (ci->get_body_plan().is_valid()) active_body_plan_ = ci->get_body_plan();
    } else if (Ref<CharacterGalleryBankResource> bank = p_asset; bank.is_valid()) {
        active_gallery_bank_ = bank;
    }
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
    Ref<CharacterInstanceResource> ci;
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
        Vector2i(1180, 740)
    );

    Node *host_parent = nullptr;
#ifdef TOOLS_ENABLED
    if (Engine::get_singleton()->is_editor_hint()) {
        if (auto *ei = EditorInterface::get_singleton()) {
            host_parent = ei->get_base_control();
        }
    }
#endif
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
