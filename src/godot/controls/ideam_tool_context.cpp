#include "ideam_tool_context.h"

#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

void IdeamToolContext::_bind_methods() {
    using namespace godot;

    BIND_ENUM_CONSTANT(UPDATE_IMMEDIATE);
    BIND_ENUM_CONSTANT(UPDATE_ON_COMMIT);
    BIND_ENUM_CONSTANT(UPDATE_EXPLICIT_TRIGGER);

    ADD_SIGNAL(MethodInfo(
        "theme_changed",
        PropertyInfo(Variant::OBJECT, "theme", PROPERTY_HINT_RESOURCE_TYPE, "Theme"),
        PropertyInfo(Variant::INT, "theme_index")
    ));
    ADD_SIGNAL(MethodInfo(
        "update_policy_changed",
        PropertyInfo(Variant::INT, "policy")
    ));
    ADD_SIGNAL(MethodInfo(
        "active_asset_changed",
        PropertyInfo(Variant::OBJECT, "asset", PROPERTY_HINT_RESOURCE_TYPE, "Resource")
    ));
    ADD_SIGNAL(MethodInfo(
        "asset_dirty_changed",
        PropertyInfo(Variant::OBJECT, "asset", PROPERTY_HINT_RESOURCE_TYPE, "Resource"),
        PropertyInfo(Variant::BOOL, "dirty")
    ));
    ADD_SIGNAL(MethodInfo(
        "explicit_rebuild_requested",
        PropertyInfo(Variant::OBJECT, "asset", PROPERTY_HINT_RESOURCE_TYPE, "Resource"),
        PropertyInfo(Variant::STRING, "reason")
    ));

    ClassDB::bind_method(D_METHOD("set_undo_redo", "undo_redo"), &IdeamToolContext::set_undo_redo);
    ClassDB::bind_method(D_METHOD("get_undo_redo"), &IdeamToolContext::get_undo_redo);

    ClassDB::bind_method(
        D_METHOD("set_active_theme", "theme", "index"),
        &IdeamToolContext::set_active_theme,
        DEFVAL(0)
    );
    ClassDB::bind_method(D_METHOD("get_active_theme"), &IdeamToolContext::get_active_theme);
    ClassDB::bind_method(D_METHOD("get_active_theme_index"), &IdeamToolContext::get_active_theme_index);

    ClassDB::bind_method(D_METHOD("set_update_commit_policy", "policy"), &IdeamToolContext::set_update_commit_policy);
    ClassDB::bind_method(D_METHOD("get_update_commit_policy"), &IdeamToolContext::get_update_commit_policy);
    ADD_PROPERTY(
        PropertyInfo(
            Variant::INT,
            "update_commit_policy",
            PROPERTY_HINT_ENUM,
            "Immediate:0,On Commit (Default):1,Explicit Trigger:2"
        ),
        "set_update_commit_policy",
        "get_update_commit_policy"
    );

    ClassDB::bind_method(D_METHOD("set_active_asset", "asset"), &IdeamToolContext::set_active_asset);
    ClassDB::bind_method(D_METHOD("get_active_asset"), &IdeamToolContext::get_active_asset);

    ClassDB::bind_method(
        D_METHOD("mark_asset_dirty", "asset", "dirty"),
        &IdeamToolContext::mark_asset_dirty,
        DEFVAL(true)
    );
    ClassDB::bind_method(D_METHOD("is_asset_dirty", "asset"), &IdeamToolContext::is_asset_dirty);
    ClassDB::bind_method(D_METHOD("has_any_dirty_assets"), &IdeamToolContext::has_any_dirty_assets);
    ClassDB::bind_method(D_METHOD("clear_all_dirty"), &IdeamToolContext::clear_all_dirty);
    ClassDB::bind_method(
        D_METHOD("request_explicit_rebuild", "asset", "reason"),
        &IdeamToolContext::request_explicit_rebuild,
        DEFVAL(String())
    );
}

IdeamToolContext::IdeamToolContext() {
    undo_redo_.instantiate();
}

void IdeamToolContext::set_undo_redo(const godot::Ref<IdeamUndoRedo> &p_undo_redo) {
    if (p_undo_redo.is_valid()) {
        undo_redo_ = p_undo_redo;
    } else {
        undo_redo_.instantiate();
    }
}

#ifdef TOOLS_ENABLED
void IdeamToolContext::set_editor_undo_redo(godot::EditorUndoRedoManager *p_manager) {
    if (undo_redo_.is_null()) {
        undo_redo_.instantiate();
    }
    undo_redo_->set_editor_undo_redo(p_manager);
}
#endif

void IdeamToolContext::set_active_theme(const godot::Ref<godot::Theme> &p_theme, int p_index) {
    active_theme_       = p_theme;
    active_theme_index_ = p_index;
    emit_signal("theme_changed", active_theme_, active_theme_index_);
}

void IdeamToolContext::set_update_commit_policy(IdeamUpdateCommitPolicy p_policy) {
    if (update_commit_policy_ == p_policy) {
        return;
    }
    update_commit_policy_ = p_policy;
    emit_signal("update_policy_changed", static_cast<int>(update_commit_policy_));
}

void IdeamToolContext::set_active_asset(const godot::Ref<godot::Resource> &p_asset) {
    if (active_asset_ == p_asset) {
        return;
    }
    active_asset_ = p_asset;
    emit_signal("active_asset_changed", active_asset_);
}

void IdeamToolContext::mark_asset_dirty(const godot::Ref<godot::Resource> &p_asset, bool p_dirty) {
    if (p_asset.is_null()) {
        return;
    }
    const uint64_t id = p_asset->get_instance_id();
    const bool was_dirty = dirty_asset_ids_.contains(id);
    if (p_dirty) {
        dirty_asset_ids_.insert(id);
    } else {
        dirty_asset_ids_.erase(id);
    }
    if (was_dirty != p_dirty) {
        emit_signal("asset_dirty_changed", p_asset, p_dirty);
    }
}

bool IdeamToolContext::is_asset_dirty(const godot::Ref<godot::Resource> &p_asset) const {
    if (p_asset.is_null()) {
        return false;
    }
    return dirty_asset_ids_.contains(p_asset->get_instance_id());
}

void IdeamToolContext::clear_all_dirty() {
    dirty_asset_ids_.clear();
}

void IdeamToolContext::request_explicit_rebuild(
    const godot::Ref<godot::Resource> &p_asset,
    const godot::String &p_reason
) {
    emit_signal("explicit_rebuild_requested", p_asset, p_reason);
}

} // namespace ideam::godot_ext
