#include "ideam_tier1_control_registry.h"

#include <algorithm>

namespace ideam::godot_ext {

IdeamTier1ControlRegistry &IdeamTier1ControlRegistry::get_singleton() {
    static IdeamTier1ControlRegistry instance;
    return instance;
}

void IdeamTier1ControlRegistry::register_control(Tier1ControlDescriptor p_descriptor) {
    for (auto &existing : descriptors_) {
        if (existing.workspace_id == p_descriptor.workspace_id &&
            existing.resource_class == p_descriptor.resource_class) {
            existing = std::move(p_descriptor);
            return;
        }
    }
    descriptors_.push_back(std::move(p_descriptor));
}

void IdeamTier1ControlRegistry::unregister_class(
    const godot::StringName &p_workspace_id,
    const godot::StringName &p_resource_class
) {
    std::erase_if(descriptors_, [&](const Tier1ControlDescriptor &d) {
        return d.workspace_id == p_workspace_id && d.resource_class == p_resource_class;
    });
}

void IdeamTier1ControlRegistry::unregister_workspace_controls(const godot::StringName &p_workspace_id) {
    std::erase_if(descriptors_, [&](const Tier1ControlDescriptor &d) {
        return d.workspace_id == p_workspace_id;
    });
}

void IdeamTier1ControlRegistry::clear() {
    descriptors_.clear();
}

const Tier1ControlDescriptor *IdeamTier1ControlRegistry::find_descriptor(
    const godot::StringName &p_workspace_id,
    const godot::StringName &p_resource_class
) const {
    for (const auto &d : descriptors_) {
        if (d.workspace_id == p_workspace_id && d.resource_class == p_resource_class) {
            return &d;
        }
    }
    return nullptr;
}

const Tier1ControlDescriptor *IdeamTier1ControlRegistry::find_descriptor_for_resource(
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
    for (const auto &d : descriptors_) {
        if (d.workspace_id == p_workspace_id && p_asset->is_class(godot::String(d.resource_class))) {
            return &d;
        }
    }
    return nullptr;
}

std::vector<Tier1ControlDescriptor> IdeamTier1ControlRegistry::get_descriptors_for_workspace(
    const godot::StringName &p_workspace_id
) const {
    std::vector<Tier1ControlDescriptor> out;
    for (const auto &d : descriptors_) {
        if (d.workspace_id == p_workspace_id) {
            out.push_back(d);
        }
    }
    return out;
}

std::vector<godot::String> IdeamTier1ControlRegistry::get_sidebar_tabs_for_workspace(
    const godot::StringName &p_workspace_id
) const {
    std::vector<godot::String> tabs;
    for (const auto &d : descriptors_) {
        if (d.workspace_id != p_workspace_id || d.sidebar_tab.is_empty()) {
            continue;
        }
        if (std::find(tabs.begin(), tabs.end(), d.sidebar_tab) == tabs.end()) {
            tabs.push_back(d.sidebar_tab);
        }
    }
    return tabs;
}

} // namespace ideam::godot_ext
