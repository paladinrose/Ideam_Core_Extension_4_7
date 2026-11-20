#pragma once

/**
 * tutorial_tool_dispatcher.h
 * Co-Pilot Layer reversible tool-call dispatcher (`src/godot/tutorials/copilot/`).
 *
 * Implements the 5 bounded, reversible Co-Pilot tool calls:
 *   1. point_to_control(selector, message)
 *   2. coach_value_entry(selector, property_name, recommended_value, rationale)
 *   3. generate_dynamic_checklist(title, steps_array)
 *   4. launch_smart_tool_or_wizard(tool_key, preset_dict)
 *   5. demonstrate_step_reversible(action_name, target_selector, property, value)
 */

#include "../resources/tutorial_resource.h"
#include "../ui/tutorial_checklist_dock.h"
#include "../ui/tutorial_input_validator.h"
#include "../ui/tutorial_spotlight_overlay.h"
#include "../../tools/agent/agent_tool_bridge.h"
#include "../../tools/tool_context.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace ideam::godot_ext {

class TutorialToolDispatcher : public godot::RefCounted {
    GDCLASS(TutorialToolDispatcher, godot::RefCounted)

private:
    TutorialChecklistDock *checklist_dock_       = nullptr;
    TutorialSpotlightOverlay *spotlight_overlay_ = nullptr;
    godot::Ref<ToolContext> tool_context_;
    godot::Ref<AgentToolBridge> agent_bridge_;

protected:
    static void _bind_methods();

public:
    TutorialToolDispatcher();
    ~TutorialToolDispatcher() override = default;

    void set_checklist_dock(TutorialChecklistDock *p_dock) { checklist_dock_ = p_dock; }
    void set_spotlight_overlay(TutorialSpotlightOverlay *p_overlay) { spotlight_overlay_ = p_overlay; }
    void set_tool_context(const godot::Ref<ToolContext> &p_context) { tool_context_ = p_context; }
    void set_agent_bridge(const godot::Ref<AgentToolBridge> &p_bridge) { agent_bridge_ = p_bridge; }

    // --- 5 Reversible Co-Pilot Tool Calls ---
    bool point_to_control(const godot::String &p_selector, const godot::String &p_message);

    bool coach_value_entry(
        const godot::String &p_selector,
        const godot::StringName &p_property_name,
        const godot::Variant &p_recommended_value,
        const godot::String &p_rationale
    );

    godot::Ref<TutorialResource> generate_dynamic_checklist(
        const godot::String &p_title,
        const godot::Array &p_steps_array
    );

    bool launch_smart_tool_or_wizard(
        const godot::StringName &p_tool_key,
        const godot::Dictionary &p_preset_dict = godot::Dictionary()
    );

    bool demonstrate_step_reversible(
        const godot::String &p_action_name,
        const godot::String &p_target_selector,
        const godot::StringName &p_property,
        const godot::Variant &p_value
    );
};

} // namespace ideam::godot_ext
