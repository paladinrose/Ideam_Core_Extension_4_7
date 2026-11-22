#include "surfaces_composer.h"

#include "surface_patch_control.h"
#include "../coons_patch_3d_resource.h"
#include "../ideam_surfaces_plugin.h"
#include "../nurbs_patch_3d_resource.h"
#include "../surface_network_resource.h"
#include "../../tools/domain/domain_tool_interface.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

#ifdef TOOLS_ENABLED
#include <godot_cpp/classes/editor_interface.hpp>
#endif

using namespace godot;

namespace ideam::godot_ext {

SurfacesComposer *SurfacesComposer::shared_instance_ = nullptr;
Window *SurfacesComposer::shared_window_             = nullptr;

void SurfacesComposer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_new_nurbs_pressed"), &SurfacesComposer::_on_new_nurbs_pressed);
    ClassDB::bind_method(D_METHOD("_on_new_coons_pressed"), &SurfacesComposer::_on_new_coons_pressed);
    ClassDB::bind_method(D_METHOD("_on_new_network_pressed"), &SurfacesComposer::_on_new_network_pressed);
}

SurfacesComposer::SurfacesComposer() {
    set_workspace_id(StringName(WORKSPACE_ID));
    set_help_topic_id(StringName("IdeamSurfaces"));

    ensure_sidebar_tab("Patches");
    ensure_sidebar_tab("Networks");

    if (tool_context_.is_valid()) {
        tool_context_->set_undo_redo(IdeamSurfacesPlugin::create_undo_redo_wrapper());
    }

    _populate_command_bar(custom_actions_box_);
}

SurfacesComposer::~SurfacesComposer() {
    if (shared_instance_ == this) {
        shared_instance_ = nullptr;
        shared_window_   = nullptr;
    }
}

void SurfacesComposer::_populate_command_bar(HBoxContainer *p_custom_actions) {
    if (!p_custom_actions || btn_new_nurbs_ != nullptr) return;

    auto add_action_btn = [&](const char *p_label, const char *p_sel, const char *p_method) -> Button * {
        Button *b = memnew(Button);
        b->set_text(p_label);
        domain_tool::stamp_ui_selector(b, p_sel);
        b->connect("pressed", Callable(this, p_method));
        p_custom_actions->add_child(b);
        return b;
    };

    btn_new_nurbs_   = add_action_btn("+ NURBS Patch",   "surfaces.composer.new_nurbs",   "_on_new_nurbs_pressed");
    btn_new_coons_   = add_action_btn("+ Gordon/Coons",  "surfaces.composer.new_coons",   "_on_new_coons_pressed");
    btn_new_network_ = add_action_btn("+ Surface Network", "surfaces.composer.new_network", "_on_new_network_pressed");
}

Control *SurfacesComposer::_create_domain_control_for_asset(const Ref<Resource> &p_asset) {
    if (!p_asset.is_valid()) return nullptr;

    if (Object::cast_to<SurfacePatchResource>(p_asset.ptr()) != nullptr ||
        Object::cast_to<SurfaceNetworkResource>(p_asset.ptr()) != nullptr) {
        auto *ctrl = memnew(SurfacePatchControl);
        ctrl->bind_domain_asset(p_asset, tool_context_);
        return ctrl;
    }

    return ToolWorkspace::_create_domain_control_for_asset(p_asset);
}

void SurfacesComposer::_on_new_nurbs_pressed() {
    Ref<NurbsPatch3DResource> nurbs;
    nurbs.instantiate();
    nurbs->set_patch_name("NURBS Patch");
    open_asset(nurbs, "Patches");
}

void SurfacesComposer::_on_new_coons_pressed() {
    Ref<CoonsPatch3DResource> coons;
    coons.instantiate();
    coons->set_patch_name("Gordon/Coons Patch");
    open_asset(coons, "Patches");
}

void SurfacesComposer::_on_new_network_pressed() {
    Ref<SurfaceNetworkResource> net;
    net.instantiate();
    net->set_network_name("Surface Network");
    open_asset(net, "Networks");
}

SurfacesComposer *SurfacesComposer::get_or_create_shared_composer(Control *p_owner) {
    if (shared_instance_ && shared_window_) {
        shared_window_->popup_centered();
        shared_window_->grab_focus();
        return shared_instance_;
    }

    shared_instance_ = memnew(SurfacesComposer);
    shared_window_   = create_workspace_window(shared_instance_, "Ideam Surfaces Composer", Vector2i(1140, 720));

    Node *host_parent = p_owner;
#ifdef TOOLS_ENABLED
    if (!host_parent && Engine::get_singleton()->is_editor_hint()) {
        if (auto *ei = EditorInterface::get_singleton()) {
            host_parent = ei->get_base_control();
        }
    }
#endif
    if (host_parent) {
        host_parent->add_child(shared_window_);
    }
    shared_window_->popup_centered();
    return shared_instance_;
}

void SurfacesComposer::open_surface_resource(const Ref<Resource> &p_resource, Control *p_owner) {
    if (SurfacesComposer *composer = get_or_create_shared_composer(p_owner)) {
        if (p_resource.is_valid()) {
            composer->open_asset(p_resource);
        }
    }
}

} // namespace ideam::godot_ext
