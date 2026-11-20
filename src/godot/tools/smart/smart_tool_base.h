#pragma once

/**
 * smart_tool_base.h
 * Tier 3 (`SmartTool`) base helper (`src/godot/tools/smart/`).
 *
 * Resolves the effective `SmartToolProfile` with strict precedence:
 *   1. Active Tier 4 `WorkflowTool` override pushed onto `ToolContext` (if any)
 *   2. User's saved `SmartToolProfile`
 *   3. Built-in `default_profile_`
 */

#include "smart_tool_profile.h"
#include "../tool_context.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace ideam::godot_ext {

class SmartToolBase : public godot::RefCounted {
    GDCLASS(SmartToolBase, godot::RefCounted)

private:
    godot::StringName tool_id_;
    godot::Ref<SmartToolProfile> default_profile_;
    godot::Ref<SmartToolProfile> user_profile_;

protected:
    static void _bind_methods();

public:
    SmartToolBase();
    ~SmartToolBase() override = default;

    void set_tool_id(const godot::StringName &p_id) { tool_id_ = p_id; }
    [[nodiscard]] godot::StringName get_tool_id() const { return tool_id_; }

    void set_default_profile(const godot::Ref<SmartToolProfile> &p_profile) { default_profile_ = p_profile; }
    [[nodiscard]] godot::Ref<SmartToolProfile> get_default_profile() const { return default_profile_; }

    void set_user_profile(const godot::Ref<SmartToolProfile> &p_profile) { user_profile_ = p_profile; }
    [[nodiscard]] godot::Ref<SmartToolProfile> get_user_profile() const { return user_profile_; }

    // Resolves effective profile: Workflow override in `p_context` > user_profile_ > default_profile_.
    [[nodiscard]] godot::Ref<SmartToolProfile> resolve_effective_profile(
        const godot::Ref<ToolContext> &p_context = godot::Ref<ToolContext>()
    ) const;
};

} // namespace ideam::godot_ext
