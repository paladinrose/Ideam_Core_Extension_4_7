#pragma once

/**
 * ideam_tool_context.h
 * Shared session context (`godot::RefCounted`) for Tier 1, Tier 2, and Tier 3+ Ideam tools
 * (`src/godot/controls/`).
 *
 * Centralizes:
 *   - `IdeamUpdateCommitPolicy`: Global or per-session data update cadence
 *     (`UPDATE_IMMEDIATE`, `UPDATE_ON_COMMIT` [default], `UPDATE_EXPLICIT_TRIGGER`).
 *   - Shared `IdeamUndoRedo` bridge (automatically supports EditorUndoRedoManager and runtime UndoRedo).
 *   - Active `Theme` and theme dropdown index for synchronized reskinning across embedded workspaces.
 *   - Open/active asset tracking and dirty-state flags for Tier 2/3 orchestration.
 */

#include "../utilities/ideam_undo_redo.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/theme.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#ifdef TOOLS_ENABLED
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#endif

#include <unordered_set>

namespace ideam::godot_ext {

/**
 * Controls when interactive Tier 1 UI controls write back to underlying data objects
 * and whether heavy downstream pipelines rebuild automatically.
 */
enum IdeamUpdateCommitPolicy : int32_t {
    UPDATE_IMMEDIATE        = 0, // Mutate underlying Resource live during drag/edit
    UPDATE_ON_COMMIT        = 1, // Default: preview visually during drag, commit 1 Undo/Redo action on release
    UPDATE_EXPLICIT_TRIGGER = 2  // Commit local edits on release, flag heavy downstream pipelines dirty until explicit trigger
};

class IdeamToolContext : public godot::RefCounted {
    GDCLASS(IdeamToolContext, godot::RefCounted)

private:
    godot::Ref<IdeamUndoRedo> undo_redo_;
    godot::Ref<godot::Theme> active_theme_;
    int active_theme_index_                       = 0;
    IdeamUpdateCommitPolicy update_commit_policy_ = UPDATE_ON_COMMIT;
    godot::Ref<godot::Resource> active_asset_;
    std::unordered_set<uint64_t> dirty_asset_ids_;

protected:
    static void _bind_methods();

public:
    IdeamToolContext();
    ~IdeamToolContext() override = default;

    // --- Undo / Redo Bridge ---
    void set_undo_redo(const godot::Ref<IdeamUndoRedo> &p_undo_redo);
    [[nodiscard]] godot::Ref<IdeamUndoRedo> get_undo_redo() const { return undo_redo_; }

#ifdef TOOLS_ENABLED
    void set_editor_undo_redo(godot::EditorUndoRedoManager *p_manager);
#endif

    // --- Theme Synchronization ---
    void set_active_theme(const godot::Ref<godot::Theme> &p_theme, int p_index = 0);
    [[nodiscard]] godot::Ref<godot::Theme> get_active_theme() const { return active_theme_; }
    [[nodiscard]] int get_active_theme_index() const noexcept { return active_theme_index_; }

    // --- Update Commit Policy ---
    void set_update_commit_policy(IdeamUpdateCommitPolicy p_policy);
    [[nodiscard]] IdeamUpdateCommitPolicy get_update_commit_policy() const noexcept {
        return update_commit_policy_;
    }

    // --- Active Asset & Dirty Tracking ---
    void set_active_asset(const godot::Ref<godot::Resource> &p_asset);
    [[nodiscard]] godot::Ref<godot::Resource> get_active_asset() const { return active_asset_; }

    void mark_asset_dirty(const godot::Ref<godot::Resource> &p_asset, bool p_dirty = true);
    [[nodiscard]] bool is_asset_dirty(const godot::Ref<godot::Resource> &p_asset) const;
    [[nodiscard]] bool has_any_dirty_assets() const noexcept { return !dirty_asset_ids_.empty(); }
    void clear_all_dirty();

    // --- Explicit Pipeline Trigger Bus (Tier 3 / Heavy Operations) ---
    void request_explicit_rebuild(const godot::Ref<godot::Resource> &p_asset, const godot::String &p_reason = "");
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::IdeamUpdateCommitPolicy);
