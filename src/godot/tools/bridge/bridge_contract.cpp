#include "bridge_contract.h"

#include <algorithm>

namespace ideam::godot_ext {

void BridgeHandoffReceipt::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_source_bridge_id", "id"), &BridgeHandoffReceipt::set_source_bridge_id);
    ClassDB::bind_method(D_METHOD("get_source_bridge_id"), &BridgeHandoffReceipt::get_source_bridge_id);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "source_bridge_id"), "set_source_bridge_id", "get_source_bridge_id");

    ClassDB::bind_method(D_METHOD("set_assets_by_workspace", "map"), &BridgeHandoffReceipt::set_assets_by_workspace);
    ClassDB::bind_method(D_METHOD("get_assets_by_workspace"), &BridgeHandoffReceipt::get_assets_by_workspace);
    ClassDB::bind_method(
        D_METHOD("add_workspace_asset", "workspace_id", "asset_path"),
        &BridgeHandoffReceipt::add_workspace_asset
    );

    ClassDB::bind_method(D_METHOD("set_warnings", "warnings"), &BridgeHandoffReceipt::set_warnings);
    ClassDB::bind_method(D_METHOD("get_warnings"), &BridgeHandoffReceipt::get_warnings);
    ClassDB::bind_method(D_METHOD("add_warning", "warning"), &BridgeHandoffReceipt::add_warning);

    ClassDB::bind_method(D_METHOD("set_semantic_tags", "tags"), &BridgeHandoffReceipt::set_semantic_tags);
    ClassDB::bind_method(D_METHOD("get_semantic_tags"), &BridgeHandoffReceipt::get_semantic_tags);

    ClassDB::bind_method(D_METHOD("set_metadata", "metadata"), &BridgeHandoffReceipt::set_metadata);
    ClassDB::bind_method(D_METHOD("get_metadata"), &BridgeHandoffReceipt::get_metadata);

    ClassDB::bind_method(D_METHOD("to_dictionary"), &BridgeHandoffReceipt::to_dictionary);
}

void BridgeHandoffReceipt::add_workspace_asset(
    const godot::String &p_workspace_id,
    const godot::String &p_asset_path
) {
    if (p_workspace_id.is_empty() || p_asset_path.is_empty()) {
        return;
    }
    godot::PackedStringArray paths;
    if (assets_by_workspace_.has(p_workspace_id)) {
        paths = assets_by_workspace_[p_workspace_id];
    }
    if (!paths.has(p_asset_path)) {
        paths.push_back(p_asset_path);
        assets_by_workspace_[p_workspace_id] = paths;
    }
}

void BridgeHandoffReceipt::add_warning(const godot::String &p_warning) {
    if (!p_warning.is_empty()) {
        warnings_.push_back(p_warning);
    }
}

godot::Dictionary BridgeHandoffReceipt::to_dictionary() const {
    godot::Dictionary d;
    d["source_bridge_id"]    = godot::String(source_bridge_id_);
    d["assets_by_workspace"] = assets_by_workspace_;
    d["warnings"]            = warnings_;
    d["semantic_tags"]       = semantic_tags_;
    d["metadata"]            = metadata_;
    return d;
}

BridgeContractRegistry &BridgeContractRegistry::get_singleton() {
    static BridgeContractRegistry instance;
    return instance;
}

void BridgeContractRegistry::register_contribution(BridgeContribution p_contribution) {
    for (auto &existing : contributions_) {
        if (existing.bridge_id == p_contribution.bridge_id &&
            existing.plugin_id == p_contribution.plugin_id) {
            existing = std::move(p_contribution);
            return;
        }
    }
    contributions_.push_back(std::move(p_contribution));
    std::stable_sort(contributions_.begin(), contributions_.end(), [](const auto &a, const auto &b) {
        return a.sort_order < b.sort_order;
    });
}

void BridgeContractRegistry::register_post_execute_hook(BridgePostExecuteHook p_hook) {
    for (auto &existing : post_execute_hooks_) {
        if (existing.bridge_id == p_hook.bridge_id &&
            existing.plugin_id == p_hook.plugin_id &&
            existing.hook_name == p_hook.hook_name) {
            existing = std::move(p_hook);
            return;
        }
    }
    post_execute_hooks_.push_back(std::move(p_hook));
    std::stable_sort(post_execute_hooks_.begin(), post_execute_hooks_.end(), [](const auto &a, const auto &b) {
        return a.priority < b.priority;
    });
}

void BridgeContractRegistry::unregister_plugin(const godot::StringName &p_plugin_id) {
    std::erase_if(contributions_, [&](const BridgeContribution &c) {
        return c.plugin_id == p_plugin_id;
    });
    std::erase_if(post_execute_hooks_, [&](const BridgePostExecuteHook &h) {
        return h.plugin_id == p_plugin_id;
    });
}

void BridgeContractRegistry::clear() {
    contributions_.clear();
    post_execute_hooks_.clear();
}

std::vector<BridgeContribution> BridgeContractRegistry::get_contributions_for_bridge(
    const godot::StringName &p_bridge_id
) const {
    std::vector<BridgeContribution> out;
    for (const auto &c : contributions_) {
        if (c.bridge_id == p_bridge_id) {
            out.push_back(c);
        }
    }
    return out;
}

void BridgeContractRegistry::invoke_post_execute_hooks(
    const godot::StringName &p_bridge_id,
    const godot::Ref<BridgeHandoffReceipt> &p_receipt
) const {
    for (const auto &h : post_execute_hooks_) {
        if (h.bridge_id == p_bridge_id && h.on_executed) {
            h.on_executed(p_receipt);
        }
    }
}

} // namespace ideam::godot_ext
