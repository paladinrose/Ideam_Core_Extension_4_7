#include "ideam_project_contribution_registry.h"

#include <algorithm>

namespace ideam::godot_ext {

IdeamProjectContributionRegistry &IdeamProjectContributionRegistry::get_singleton() {
    static IdeamProjectContributionRegistry instance;
    return instance;
}

void IdeamProjectContributionRegistry::register_manifest_contribution(ProjectManifestContribution p_contribution) {
    for (auto &existing : manifest_contributions_) {
        if (existing.plugin_id == p_contribution.plugin_id) {
            existing = std::move(p_contribution);
            return;
        }
    }
    manifest_contributions_.push_back(std::move(p_contribution));
    std::stable_sort(
        manifest_contributions_.begin(),
        manifest_contributions_.end(),
        [](const ProjectManifestContribution &a, const ProjectManifestContribution &b) {
            return a.sort_order < b.sort_order;
        }
    );
}

void IdeamProjectContributionRegistry::register_post_generate_hook(ProjectPostGenerateHook p_hook) {
    for (auto &existing : post_generate_hooks_) {
        if (existing.plugin_id == p_hook.plugin_id && existing.hook_name == p_hook.hook_name) {
            existing = std::move(p_hook);
            return;
        }
    }
    post_generate_hooks_.push_back(std::move(p_hook));
    std::stable_sort(
        post_generate_hooks_.begin(),
        post_generate_hooks_.end(),
        [](const ProjectPostGenerateHook &a, const ProjectPostGenerateHook &b) {
            return a.priority < b.priority;
        }
    );
}

void IdeamProjectContributionRegistry::register_script_template(ProjectScriptTemplateEntry p_entry) {
    for (auto &existing : script_templates_) {
        if (existing.base_class == p_entry.base_class && existing.template_name == p_entry.template_name) {
            existing = std::move(p_entry);
            return;
        }
    }
    script_templates_.push_back(std::move(p_entry));
}

void IdeamProjectContributionRegistry::unregister_plugin(const godot::StringName &p_plugin_id) {
    std::erase_if(manifest_contributions_, [&](const ProjectManifestContribution &c) {
        return c.plugin_id == p_plugin_id;
    });
    std::erase_if(post_generate_hooks_, [&](const ProjectPostGenerateHook &h) {
        return h.plugin_id == p_plugin_id;
    });
    std::erase_if(script_templates_, [&](const ProjectScriptTemplateEntry &t) {
        return t.plugin_id == p_plugin_id;
    });
}

void IdeamProjectContributionRegistry::clear() {
    manifest_contributions_.clear();
    post_generate_hooks_.clear();
    script_templates_.clear();
}

const ProjectScriptTemplateEntry *IdeamProjectContributionRegistry::find_script_template(
    const godot::StringName &p_base_class,
    const godot::String &p_template_name
) const {
    const godot::String target_name = p_template_name.is_empty() ? godot::String("default") : p_template_name;
    // 1. Exact class + template name
    for (const auto &entry : script_templates_) {
        if (entry.base_class == p_base_class && entry.template_name == target_name) {
            return &entry;
        }
    }
    // 2. Fallback to any template for that base class
    for (const auto &entry : script_templates_) {
        if (entry.base_class == p_base_class) {
            return &entry;
        }
    }
    return nullptr;
}

void IdeamProjectContributionRegistry::invoke_post_generate_hooks(const godot::Dictionary &p_receipt) const {
    for (const auto &hook : post_generate_hooks_) {
        if (hook.on_project_generated) {
            hook.on_project_generated(p_receipt);
        }
    }
}

} // namespace ideam::godot_ext
