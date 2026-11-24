#include "tool_context.h"
#include "tool_registry.h"

#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

void ToolContext::_bind_methods() {
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
        "theme_3d_changed",
        PropertyInfo(Variant::OBJECT, "theme_3d", PROPERTY_HINT_RESOURCE_TYPE, "Resource")
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
    ADD_SIGNAL(MethodInfo(
        "preference_override_changed",
        PropertyInfo(Variant::STRING_NAME, "tool_id"),
        PropertyInfo(Variant::OBJECT, "profile", PROPERTY_HINT_RESOURCE_TYPE, "Resource")
    ));
    ADD_SIGNAL(MethodInfo(
        "active_rubric_changed",
        PropertyInfo(Variant::OBJECT, "rubric", PROPERTY_HINT_RESOURCE_TYPE, "Resource")
    ));
    ADD_SIGNAL(MethodInfo(
        "help_requested",
        PropertyInfo(Variant::STRING_NAME, "tool_id"),
        PropertyInfo(Variant::STRING, "ui_selector"),
        PropertyInfo(Variant::DICTIONARY, "context")
    ));

    ClassDB::bind_method(D_METHOD("set_undo_redo", "undo_redo"), &ToolContext::set_undo_redo);
    ClassDB::bind_method(D_METHOD("get_undo_redo"), &ToolContext::get_undo_redo);

    ClassDB::bind_method(
        D_METHOD("set_active_theme", "theme", "index"),
        &ToolContext::set_active_theme,
        DEFVAL(0)
    );
    ClassDB::bind_method(D_METHOD("get_active_theme"), &ToolContext::get_active_theme);
    ClassDB::bind_method(D_METHOD("get_active_theme_index"), &ToolContext::get_active_theme_index);

    ClassDB::bind_method(D_METHOD("set_active_theme_3d", "theme_3d"), &ToolContext::set_active_theme_3d);
    ClassDB::bind_method(D_METHOD("get_active_theme_3d"), &ToolContext::get_active_theme_3d);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "active_theme_3d", PROPERTY_HINT_RESOURCE_TYPE, "Resource"),
        "set_active_theme_3d",
        "get_active_theme_3d"
    );

    ClassDB::bind_method(D_METHOD("set_update_commit_policy", "policy"), &ToolContext::set_update_commit_policy);
    ClassDB::bind_method(D_METHOD("get_update_commit_policy"), &ToolContext::get_update_commit_policy);
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

    ClassDB::bind_method(D_METHOD("set_active_asset", "asset"), &ToolContext::set_active_asset);
    ClassDB::bind_method(D_METHOD("get_active_asset"), &ToolContext::get_active_asset);

    ClassDB::bind_method(
        D_METHOD("mark_asset_dirty", "asset", "dirty"),
        &ToolContext::mark_asset_dirty,
        DEFVAL(true)
    );
    ClassDB::bind_method(D_METHOD("is_asset_dirty", "asset"), &ToolContext::is_asset_dirty);
    ClassDB::bind_method(D_METHOD("has_any_dirty_assets"), &ToolContext::has_any_dirty_assets);
    ClassDB::bind_method(D_METHOD("clear_all_dirty"), &ToolContext::clear_all_dirty);
    ClassDB::bind_method(
        D_METHOD("request_explicit_rebuild", "asset", "reason"),
        &ToolContext::request_explicit_rebuild,
        DEFVAL(String())
    );

    // Preference Overrides
    ClassDB::bind_method(
        D_METHOD("push_preference_override", "tool_id", "profile"),
        &ToolContext::push_preference_override
    );
    ClassDB::bind_method(D_METHOD("pop_preference_override", "tool_id"), &ToolContext::pop_preference_override);
    ClassDB::bind_method(D_METHOD("get_preference_override", "tool_id"), &ToolContext::get_preference_override);
    ClassDB::bind_method(D_METHOD("has_preference_override", "tool_id"), &ToolContext::has_preference_override);
    ClassDB::bind_method(D_METHOD("clear_preference_overrides"), &ToolContext::clear_preference_overrides);

    // Active Rubric
    ClassDB::bind_method(D_METHOD("set_active_rubric", "rubric"), &ToolContext::set_active_rubric);
    ClassDB::bind_method(D_METHOD("get_active_rubric"), &ToolContext::get_active_rubric);

    // Help / Tutorials Bus
    ClassDB::bind_method(
        D_METHOD("request_help", "tool_id", "ui_selector", "context"),
        &ToolContext::request_help,
        DEFVAL(String()),
        DEFVAL(Dictionary())
    );
}

ToolContext::ToolContext() {
    undo_redo_.instantiate();
}

void ToolContext::set_undo_redo(const godot::Ref<IdeamUndoRedo> &p_undo_redo) {
    if (p_undo_redo.is_valid()) {
        undo_redo_ = p_undo_redo;
    } else {
        undo_redo_.instantiate();
    }
}

#ifdef TOOLS_ENABLED
void ToolContext::set_editor_undo_redo(godot::EditorUndoRedoManager *p_manager) {
    if (undo_redo_.is_null()) {
        undo_redo_.instantiate();
    }
    undo_redo_->set_editor_undo_redo(p_manager);
}
#endif

void ToolContext::set_active_theme(const godot::Ref<godot::Theme> &p_theme, int p_index) {
    active_theme_       = p_theme;
    active_theme_index_ = p_index;
    emit_signal("theme_changed", active_theme_, active_theme_index_);
}

void ToolContext::set_active_theme_3d(const godot::Ref<godot::Resource> &p_theme_3d) {
    if (active_theme_3d_ == p_theme_3d) {
        return;
    }
    active_theme_3d_ = p_theme_3d;
    emit_signal("theme_3d_changed", active_theme_3d_);
}

void ToolContext::set_update_commit_policy(UpdateCommitPolicy p_policy) {
    if (update_commit_policy_ == p_policy) {
        return;
    }
    update_commit_policy_ = p_policy;
    emit_signal("update_policy_changed", static_cast<int>(update_commit_policy_));
}

void ToolContext::set_active_asset(const godot::Ref<godot::Resource> &p_asset) {
    if (active_asset_ == p_asset) {
        return;
    }
    active_asset_ = p_asset;
    emit_signal("active_asset_changed", active_asset_);
}

void ToolContext::mark_asset_dirty(const godot::Ref<godot::Resource> &p_asset, bool p_dirty) {
    if (p_asset.is_null()) {
        return;
    }
    const uint64_t id    = p_asset->get_instance_id();
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

bool ToolContext::is_asset_dirty(const godot::Ref<godot::Resource> &p_asset) const {
    if (p_asset.is_null()) {
        return false;
    }
    return dirty_asset_ids_.contains(p_asset->get_instance_id());
}

void ToolContext::clear_all_dirty() {
    dirty_asset_ids_.clear();
}

void ToolContext::request_explicit_rebuild(
    const godot::Ref<godot::Resource> &p_asset,
    const godot::String &p_reason
) {
    emit_signal("explicit_rebuild_requested", p_asset, p_reason);
}

void ToolContext::push_preference_override(
    const godot::StringName &p_tool_id,
    const godot::Ref<godot::Resource> &p_profile
) {
    if (p_tool_id == godot::StringName() || p_profile.is_null()) {
        return;
    }
    preference_override_stacks_[p_tool_id].push_back(p_profile);
    emit_signal("preference_override_changed", p_tool_id, p_profile);
}

void ToolContext::pop_preference_override(const godot::StringName &p_tool_id) {
    auto it = preference_override_stacks_.find(p_tool_id);
    if (it == preference_override_stacks_.end() || it->second.empty()) {
        return;
    }
    it->second.pop_back();
    godot::Ref<godot::Resource> next_active;
    if (it->second.empty()) {
        preference_override_stacks_.erase(it);
    } else {
        next_active = it->second.back();
    }
    emit_signal("preference_override_changed", p_tool_id, next_active);
}

godot::Ref<godot::Resource> ToolContext::get_preference_override(const godot::StringName &p_tool_id) const {
    const auto it = preference_override_stacks_.find(p_tool_id);
    if (it == preference_override_stacks_.end() || it->second.empty()) {
        return godot::Ref<godot::Resource>();
    }
    return it->second.back();
}

bool ToolContext::has_preference_override(const godot::StringName &p_tool_id) const {
    const auto it = preference_override_stacks_.find(p_tool_id);
    return it != preference_override_stacks_.end() && !it->second.empty();
}

void ToolContext::clear_preference_overrides() {
    preference_override_stacks_.clear();
}

void ToolContext::set_active_rubric(const godot::Ref<godot::Resource> &p_rubric) {
    if (active_rubric_ == p_rubric) {
        return;
    }
    active_rubric_ = p_rubric;
    emit_signal("active_rubric_changed", active_rubric_);
}

bool ToolContext::request_help(
    const godot::StringName &p_tool_id,
    const godot::String &p_ui_selector,
    const godot::Dictionary &p_context
) {
    emit_signal("help_requested", p_tool_id, p_ui_selector, p_context);
    return ToolRegistry::get_singleton().dispatch_help_request(p_tool_id, p_ui_selector, p_context);
}

} // namespace ideam::godot_ext
