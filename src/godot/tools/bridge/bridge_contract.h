#pragma once

/**
 * bridge_contract.h
 * Formal Tier 2 (`BridgeTool`) cross-plugin contribution & handoff protocol (`src/godot/tools/bridge/`).
 *
 * Generalizes the pattern pioneered by `IdeamProjectContributionRegistry` so any BridgeTool
 * (`ProjectWorkspace`, `GeometryComposer`, `PlanetarySystemsModeler`, `DesignComposer`, etc.)
 * can accept decoupled contributions, execute post-run hooks, and emit structured handoff receipts
 * consumable by sibling BridgeTools and Tier 4 `WorkflowTool`s.
 */

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

#include <functional>
#include <vector>

namespace ideam::godot_ext {

class BridgeHandoffReceipt : public godot::RefCounted {
    GDCLASS(BridgeHandoffReceipt, godot::RefCounted)

private:
    godot::StringName source_bridge_id_;
    godot::Dictionary assets_by_workspace_; // workspace_id (String) -> PackedStringArray of res:// paths
    godot::PackedStringArray warnings_;
    godot::PackedStringArray semantic_tags_;
    godot::Dictionary metadata_;

protected:
    static void _bind_methods();

public:
    BridgeHandoffReceipt() = default;
    ~BridgeHandoffReceipt() override = default;

    void set_source_bridge_id(const godot::StringName &p_id) { source_bridge_id_ = p_id; }
    [[nodiscard]] godot::StringName get_source_bridge_id() const { return source_bridge_id_; }

    void set_assets_by_workspace(const godot::Dictionary &p_map) { assets_by_workspace_ = p_map; }
    [[nodiscard]] godot::Dictionary get_assets_by_workspace() const { return assets_by_workspace_; }
    void add_workspace_asset(const godot::String &p_workspace_id, const godot::String &p_asset_path);

    void set_warnings(const godot::PackedStringArray &p_warnings) { warnings_ = p_warnings; }
    [[nodiscard]] godot::PackedStringArray get_warnings() const { return warnings_; }
    void add_warning(const godot::String &p_warning);

    void set_semantic_tags(const godot::PackedStringArray &p_tags) { semantic_tags_ = p_tags; }
    [[nodiscard]] godot::PackedStringArray get_semantic_tags() const { return semantic_tags_; }

    void set_metadata(const godot::Dictionary &p_metadata) { metadata_ = p_metadata; }
    [[nodiscard]] godot::Dictionary get_metadata() const { return metadata_; }

    [[nodiscard]] godot::Dictionary to_dictionary() const;
};

struct BridgeContribution {
    godot::StringName bridge_id;    // Target BridgeTool (e.g. "ProjectWorkspace", "GeometryComposer")
    godot::StringName plugin_id;    // Contributing plugin (e.g. "IdeamGames", "IdeamSplines")
    godot::String display_tab;      // Optional UI grouping tab
    int32_t sort_order = 100;
    godot::PackedStringArray semantic_tags;

    // Lazily produces a typed contribution payload Resource (e.g. manifest, ruleset, preset pack).
    std::function<godot::Ref<godot::Resource>()> build_payload;
};

struct BridgePostExecuteHook {
    godot::StringName bridge_id;
    godot::StringName plugin_id;
    godot::String hook_name;
    int32_t priority = 100; // Lower runs earlier

    std::function<void(const godot::Ref<BridgeHandoffReceipt> &p_receipt)> on_executed;
};

class BridgeContractRegistry {
private:
    std::vector<BridgeContribution> contributions_;
    std::vector<BridgePostExecuteHook> post_execute_hooks_;

    BridgeContractRegistry() = default;

public:
    static BridgeContractRegistry &get_singleton();

    void register_contribution(BridgeContribution p_contribution);
    void register_post_execute_hook(BridgePostExecuteHook p_hook);
    void unregister_plugin(const godot::StringName &p_plugin_id);
    void clear();

    [[nodiscard]] std::vector<BridgeContribution> get_contributions_for_bridge(
        const godot::StringName &p_bridge_id
    ) const;

    void invoke_post_execute_hooks(
        const godot::StringName &p_bridge_id,
        const godot::Ref<BridgeHandoffReceipt> &p_receipt
    ) const;
};

} // namespace ideam::godot_ext
