#include "tool_registry.h"

#include <algorithm>

namespace ideam::godot_ext {

ToolRegistry &ToolRegistry::get_singleton() {
    static ToolRegistry instance;
    return instance;
}

// ============================================================================
// Tier 1: DomainTool Control Factories
// ============================================================================

void ToolRegistry::register_control(DomainToolDescriptor p_descriptor) {
    for (auto &existing : domain_descriptors_) {
        if (existing.workspace_id == p_descriptor.workspace_id &&
            existing.resource_class == p_descriptor.resource_class) {
            existing = std::move(p_descriptor);
            return;
        }
    }
    domain_descriptors_.push_back(std::move(p_descriptor));
}

void ToolRegistry::unregister_class(
    const godot::StringName &p_workspace_id,
    const godot::StringName &p_resource_class
) {
    std::erase_if(domain_descriptors_, [&](const DomainToolDescriptor &d) {
        return d.workspace_id == p_workspace_id && d.resource_class == p_resource_class;
    });
}

void ToolRegistry::unregister_workspace_controls(const godot::StringName &p_workspace_id) {
    std::erase_if(domain_descriptors_, [&](const DomainToolDescriptor &d) {
        return d.workspace_id == p_workspace_id;
    });
}

const DomainToolDescriptor *ToolRegistry::find_descriptor(
    const godot::StringName &p_workspace_id,
    const godot::StringName &p_resource_class
) const {
    for (const auto &d : domain_descriptors_) {
        if (d.workspace_id == p_workspace_id && d.resource_class == p_resource_class) {
            return &d;
        }
    }
    return nullptr;
}

const DomainToolDescriptor *ToolRegistry::find_descriptor_for_resource(
    const godot::StringName &p_workspace_id,
    const godot::Ref<godot::Resource> &p_asset
) const {
    if (p_asset.is_null()) {
        return nullptr;
    }
    // 1. Exact class match first
    const godot::StringName exact_cls(p_asset->get_class());
    if (const auto *exact = find_descriptor(p_workspace_id, exact_cls)) {
        return exact;
    }
    // 2. Polymorphic inheritance match fallback
    for (const auto &d : domain_descriptors_) {
        if (d.workspace_id == p_workspace_id && p_asset->is_class(godot::String(d.resource_class))) {
            return &d;
        }
    }
    return nullptr;
}

const DomainToolDescriptor *ToolRegistry::find_any_descriptor_for_class(
    const godot::StringName &p_resource_class
) const {
    if (p_resource_class == godot::StringName()) {
        return nullptr;
    }
    for (const auto &d : domain_descriptors_) {
        if (d.resource_class == p_resource_class) {
            return &d;
        }
    }
    return nullptr;
}

const DomainToolDescriptor *ToolRegistry::find_any_descriptor_for_resource(
    const godot::Ref<godot::Resource> &p_asset
) const {
    if (p_asset.is_null()) {
        return nullptr;
    }
    const godot::StringName exact_cls(p_asset->get_class());
    if (const auto *exact = find_any_descriptor_for_class(exact_cls)) {
        return exact;
    }
    for (const auto &d : domain_descriptors_) {
        if (p_asset->is_class(godot::String(d.resource_class))) {
            return &d;
        }
    }
    return nullptr;
}

std::vector<DomainToolDescriptor> ToolRegistry::get_descriptors_for_workspace(
    const godot::StringName &p_workspace_id
) const {
    std::vector<DomainToolDescriptor> out;
    for (const auto &d : domain_descriptors_) {
        if (d.workspace_id == p_workspace_id) {
            out.push_back(d);
        }
    }
    return out;
}

std::vector<godot::String> ToolRegistry::get_sidebar_tabs_for_workspace(
    const godot::StringName &p_workspace_id
) const {
    std::vector<godot::String> tabs;
    for (const auto &d : domain_descriptors_) {
        if (d.workspace_id != p_workspace_id || d.sidebar_tab.is_empty()) {
            continue;
        }
        if (std::find(tabs.begin(), tabs.end(), d.sidebar_tab) == tabs.end()) {
            tabs.push_back(d.sidebar_tab);
        }
    }
    return tabs;
}

// ============================================================================
// Tier 2: BridgeTool Workspace Launchers
// ============================================================================

void ToolRegistry::register_workspace_launcher(BridgeLauncher p_launcher) {
    for (auto &existing : bridge_launchers_) {
        if (existing.workspace_id == p_launcher.workspace_id) {
            existing = std::move(p_launcher);
            return;
        }
    }
    bridge_launchers_.push_back(std::move(p_launcher));
}

void ToolRegistry::unregister_workspace_launcher(const godot::StringName &p_workspace_id) {
    std::erase_if(bridge_launchers_, [&](const BridgeLauncher &l) {
        return l.workspace_id == p_workspace_id;
    });
}

const BridgeLauncher *ToolRegistry::find_workspace_launcher(
    const godot::StringName &p_workspace_id
) const {
    for (const auto &l : bridge_launchers_) {
        if (l.workspace_id == p_workspace_id) {
            return &l;
        }
    }
    return nullptr;
}

bool ToolRegistry::launch_asset_in_workspace(
    const godot::StringName &p_workspace_id,
    const godot::Ref<godot::Resource> &p_asset,
    const godot::Ref<ToolContext> &p_context
) const {
    if (p_asset.is_null()) {
        return false;
    }
    if (const auto *launcher = find_workspace_launcher(p_workspace_id); launcher && launcher->open_asset) {
        launcher->open_asset(p_asset, p_context);
        return true;
    }
    return false;
}

// ============================================================================
// Tiers 1–5: Unified ToolDescriptor Catalog
// ============================================================================

void ToolRegistry::register_tool_descriptor(const godot::Ref<ToolDescriptor> &p_descriptor) {
    if (p_descriptor.is_null() || p_descriptor->get_tool_id() == godot::StringName()) {
        return;
    }
    for (auto &existing : tool_descriptors_) {
        if (existing.is_valid() && existing->get_tool_id() == p_descriptor->get_tool_id()) {
            existing = p_descriptor;
            return;
        }
    }
    tool_descriptors_.push_back(p_descriptor);
}

void ToolRegistry::unregister_tool_descriptor(const godot::StringName &p_tool_id) {
    std::erase_if(tool_descriptors_, [&](const godot::Ref<ToolDescriptor> &d) {
        return d.is_null() || d->get_tool_id() == p_tool_id;
    });
}

void ToolRegistry::unregister_plugin_tools(const godot::StringName &p_plugin_id) {
    std::erase_if(tool_descriptors_, [&](const godot::Ref<ToolDescriptor> &d) {
        return d.is_null() || d->get_plugin_id() == p_plugin_id;
    });
}

godot::Ref<ToolDescriptor> ToolRegistry::find_tool_descriptor(const godot::StringName &p_tool_id) const {
    for (const auto &d : tool_descriptors_) {
        if (d.is_valid() && d->get_tool_id() == p_tool_id) {
            return d;
        }
    }
    return godot::Ref<ToolDescriptor>();
}

std::vector<godot::Ref<ToolDescriptor>> ToolRegistry::get_tool_descriptors_by_tier(ToolTier p_tier) const {
    std::vector<godot::Ref<ToolDescriptor>> out;
    for (const auto &d : tool_descriptors_) {
        if (d.is_valid() && d->get_tier() == p_tier) {
            out.push_back(d);
        }
    }
    return out;
}

godot::Array ToolRegistry::export_tool_catalog_schema() const {
    godot::Array out;
    for (const auto &d : tool_descriptors_) {
        if (d.is_valid()) {
            out.append(d->to_schema_dictionary());
        }
    }
    for (const auto &b : bridge_launchers_) {
        godot::Dictionary entry;
        entry["tool_id"]       = godot::String(b.workspace_id);
        entry["tier"]          = static_cast<int32_t>(TOOL_TIER_BRIDGE);
        entry["tier_name"]     = ToolDescriptor::tier_to_string(TOOL_TIER_BRIDGE);
        entry["display_name"]  = b.display_name;
        entry["semantic_tags"] = b.semantic_tags;
        entry["help_topic_id"] = godot::String(b.help_topic_id);
        out.append(entry);
    }
    return out;
}

// ============================================================================
// Cross-Tier Help / IdeamTutorials Dispatch
// ============================================================================

void ToolRegistry::register_help_handler(ToolHelpHandler p_handler) {
    for (auto &existing : help_handlers_) {
        if (existing.provider_id == p_handler.provider_id) {
            existing = std::move(p_handler);
            std::sort(help_handlers_.begin(), help_handlers_.end(), [](const auto &a, const auto &b) {
                return a.priority < b.priority;
            });
            return;
        }
    }
    help_handlers_.push_back(std::move(p_handler));
    std::sort(help_handlers_.begin(), help_handlers_.end(), [](const auto &a, const auto &b) {
        return a.priority < b.priority;
    });
}

void ToolRegistry::unregister_help_handler(const godot::StringName &p_provider_id) {
    std::erase_if(help_handlers_, [&](const ToolHelpHandler &h) {
        return h.provider_id == p_provider_id;
    });
}

bool ToolRegistry::dispatch_help_request(
    const godot::StringName &p_tool_id,
    const godot::String &p_ui_selector,
    const godot::Dictionary &p_context
) const {
    for (const auto &h : help_handlers_) {
        if (h.on_help_requested && h.on_help_requested(p_tool_id, p_ui_selector, p_context)) {
            return true;
        }
    }
    return false;
}

void ToolRegistry::clear() {
    domain_descriptors_.clear();
    bridge_launchers_.clear();
    tool_descriptors_.clear();
    help_handlers_.clear();
}

} // namespace ideam::godot_ext
