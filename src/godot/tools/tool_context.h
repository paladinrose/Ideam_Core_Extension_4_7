#pragma once

/**
 * tool_context.h
 * Shared session context (`godot::RefCounted`) across all five tool tiers (`src/godot/tools/`).
 *
 * Centralizes:
 *   - `UpdateCommitPolicy`: Global or per-session data update cadence
 *     (`UPDATE_IMMEDIATE`, `UPDATE_ON_COMMIT` [default], `UPDATE_EXPLICIT_TRIGGER`).
 *   - Shared `IdeamUndoRedo` bridge (EditorUndoRedoManager + runtime UndoRedo).
 *   - Active 2D `Theme` + active 3D `Theme3DResource` (opaque `Resource`) for synchronized
 *     2D UI and 3D diegetic gizmo reskinning across Editor & Runtime contexts.
 *   - Open/active asset tracking and dirty-state flags for Bridge/Workflow orchestration.
 *   - Scoped `SmartToolProfile` override stack so `WorkflowTool`s can temporarily override
 *     `SmartTool` user preferences without mutating global user settings.
 *   - Active semantic rubric slot (opaque `Resource` to avoid hard coupling to `IdeamLinguistics`).
 *   - Cross-tier Help / Tutorial request bus (`request_help` / `help_requested`).
 */

#include "../utilities/ideam_undo_redo.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/theme.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

#ifdef TOOLS_ENABLED
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#endif

#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ideam::godot_ext {

/**
 * Controls when interactive DomainTool UI controls write back to underlying data objects
 * and whether heavy downstream pipelines rebuild automatically.
 */
enum UpdateCommitPolicy : int32_t {
    UPDATE_IMMEDIATE        = 0, // Mutate underlying Resource live during drag/edit
    UPDATE_ON_COMMIT        = 1, // Default: preview visually during drag, commit 1 Undo/Redo action on release
    UPDATE_EXPLICIT_TRIGGER = 2  // Commit local edits on release, flag heavy downstream pipelines dirty until explicit trigger
};

class ToolContext : public godot::RefCounted {
    GDCLASS(ToolContext, godot::RefCounted)

private:
    struct StringNameHash {
        size_t operator()(const godot::StringName &p_key) const noexcept {
            return static_cast<size_t>(p_key.hash());
        }
    };

    godot::Ref<IdeamUndoRedo> undo_redo_;
    godot::Ref<godot::Theme> active_theme_;
    int active_theme_index_                  = 0;
    godot::Ref<godot::Resource> active_theme_3d_; ///< Opaque `Theme3DResource` for diegetic 3D gizmo/stage reskinning
    UpdateCommitPolicy update_commit_policy_ = UPDATE_ON_COMMIT;
    godot::Ref<godot::Resource> active_asset_;
    std::unordered_set<uint64_t> dirty_asset_ids_;

    // Scoped Tier 4 (WorkflowTool) -> Tier 3 (SmartTool) preference override stack
    std::unordered_map<godot::StringName, std::vector<godot::Ref<godot::Resource>>, StringNameHash>
        preference_override_stacks_;

    // Optional Tier 5/4 active semantic rubric (`SemanticStyleRubric` or `SemanticTagSetResource`)
    godot::Ref<godot::Resource> active_rubric_;

protected:
    static void _bind_methods();

public:
    ToolContext();
    ~ToolContext() override = default;

    // --- Undo / Redo Bridge ---
    void set_undo_redo(const godot::Ref<IdeamUndoRedo> &p_undo_redo);
    [[nodiscard]] godot::Ref<IdeamUndoRedo> get_undo_redo() const { return undo_redo_; }

#ifdef TOOLS_ENABLED
    void set_editor_undo_redo(godot::EditorUndoRedoManager *p_manager);
#endif

    // --- 2D & 3D Theme Synchronization ---
    void set_active_theme(const godot::Ref<godot::Theme> &p_theme, int p_index = 0);
    [[nodiscard]] godot::Ref<godot::Theme> get_active_theme() const { return active_theme_; }
    [[nodiscard]] int get_active_theme_index() const noexcept { return active_theme_index_; }

    void set_active_theme_3d(const godot::Ref<godot::Resource> &p_theme_3d);
    [[nodiscard]] godot::Ref<godot::Resource> get_active_theme_3d() const { return active_theme_3d_; }

    // --- Update Commit Policy ---
    void set_update_commit_policy(UpdateCommitPolicy p_policy);
    [[nodiscard]] UpdateCommitPolicy get_update_commit_policy() const noexcept {
        return update_commit_policy_;
    }

    // --- Active Asset & Dirty Tracking ---
    void set_active_asset(const godot::Ref<godot::Resource> &p_asset);
    [[nodiscard]] godot::Ref<godot::Resource> get_active_asset() const { return active_asset_; }

    void mark_asset_dirty(const godot::Ref<godot::Resource> &p_asset, bool p_dirty = true);
    [[nodiscard]] bool is_asset_dirty(const godot::Ref<godot::Resource> &p_asset) const;
    [[nodiscard]] bool has_any_dirty_assets() const noexcept { return !dirty_asset_ids_.empty(); }
    void clear_all_dirty();

    // --- Explicit Pipeline Trigger Bus (Heavy / Workflow Operations) ---
    void request_explicit_rebuild(const godot::Ref<godot::Resource> &p_asset, const godot::String &p_reason = "");

    // --- Scoped SmartTool Preference Override Stack (WorkflowTool -> SmartTool) ---
    void push_preference_override(const godot::StringName &p_tool_id, const godot::Ref<godot::Resource> &p_profile);
    void pop_preference_override(const godot::StringName &p_tool_id);
    [[nodiscard]] godot::Ref<godot::Resource> get_preference_override(const godot::StringName &p_tool_id) const;
    [[nodiscard]] bool has_preference_override(const godot::StringName &p_tool_id) const;
    void clear_preference_overrides();

    // --- Active Semantic Rubric (WorkflowTool / AgentTool) ---
    void set_active_rubric(const godot::Ref<godot::Resource> &p_rubric);
    [[nodiscard]] godot::Ref<godot::Resource> get_active_rubric() const { return active_rubric_; }

    // --- Cross-Tier Help / Tutorials Bus ---
    bool request_help(
        const godot::StringName &p_tool_id,
        const godot::String &p_ui_selector = "",
        const godot::Dictionary &p_context = godot::Dictionary()
    );
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::UpdateCommitPolicy);
