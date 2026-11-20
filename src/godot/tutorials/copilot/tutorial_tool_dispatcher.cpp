#include "tutorial_tool_dispatcher.h"
#include "../../tools/tool_registry.h"

namespace ideam::godot_ext {

void TutorialToolDispatcher::_bind_methods() {
    using namespace godot;

    ADD_SIGNAL(MethodInfo(
        "tool_launch_requested",
        PropertyInfo(Variant::STRING_NAME, "tool_key"),
        PropertyInfo(Variant::DICTIONARY, "preset_dict")
    ));

    ClassDB::bind_method(D_METHOD("set_checklist_dock", "dock"), &TutorialToolDispatcher::set_checklist_dock);
    ClassDB::bind_method(D_METHOD("set_spotlight_overlay", "overlay"), &TutorialToolDispatcher::set_spotlight_overlay);
    ClassDB::bind_method(D_METHOD("set_tool_context", "context"), &TutorialToolDispatcher::set_tool_context);
    ClassDB::bind_method(D_METHOD("set_agent_bridge", "bridge"), &TutorialToolDispatcher::set_agent_bridge);

    ClassDB::bind_method(
        D_METHOD("point_to_control", "selector", "message"),
        &TutorialToolDispatcher::point_to_control
    );
    ClassDB::bind_method(
        D_METHOD("coach_value_entry", "selector", "property_name", "recommended_value", "rationale"),
        &TutorialToolDispatcher::coach_value_entry
    );
    ClassDB::bind_method(
        D_METHOD("generate_dynamic_checklist", "title", "steps_array"),
        &TutorialToolDispatcher::generate_dynamic_checklist
    );
    ClassDB::bind_method(
        D_METHOD("launch_smart_tool_or_wizard", "tool_key", "preset_dict"),
        &TutorialToolDispatcher::launch_smart_tool_or_wizard,
        DEFVAL(Dictionary())
    );
    ClassDB::bind_method(
        D_METHOD("demonstrate_step_reversible", "action_name", "target_selector", "property", "value"),
        &TutorialToolDispatcher::demonstrate_step_reversible
    );
}

TutorialToolDispatcher::TutorialToolDispatcher() {
    tool_context_.instantiate();
    agent_bridge_.instantiate();
}

bool TutorialToolDispatcher::point_to_control(
    const godot::String &p_selector,
    const godot::String &p_message
) {
    if (!spotlight_overlay_) {
        return false;
    }
    godot::Control *target = TutorialInputValidator::resolve_ui_selector(nullptr, p_selector);
    if (!target) {
        return false;
    }
    spotlight_overlay_->spotlight_control(target, "Guided Spotlight", p_message);
    return true;
}

bool TutorialToolDispatcher::coach_value_entry(
    const godot::String &p_selector,
    const godot::StringName &p_property_name,
    const godot::Variant &p_recommended_value,
    const godot::String &p_rationale
) {
    if (!spotlight_overlay_) {
        return false;
    }
    godot::Control *target = TutorialInputValidator::resolve_ui_selector(nullptr, p_selector);
    if (!target) {
        return false;
    }
    const godot::String hint = godot::String("Recommended ") + godot::String(p_property_name) +
                               " = " + godot::String(p_recommended_value);
    spotlight_overlay_->spotlight_control(target, "Value Coach", p_rationale, hint);
    return true;
}

godot::Ref<TutorialResource> TutorialToolDispatcher::generate_dynamic_checklist(
    const godot::String &p_title,
    const godot::Array &p_steps_array
) {
    godot::Ref<TutorialResource> tut;
    tut.instantiate();
    tut->set_tutorial_id(godot::StringName(godot::String("dynamic_") + p_title.to_snake_case()));
    tut->set_title(p_title);
    tut->set_target_context(CONTEXT_BOTH);

    for (int i = 0; i < p_steps_array.size(); ++i) {
        godot::Ref<TutorialStepResource> step;
        step.instantiate();
        step->set_step_id(godot::StringName(godot::String("dyn_step_") + godot::String::num_int64(i + 1)));

        if (p_steps_array[i].get_type() == godot::Variant::DICTIONARY) {
            const godot::Dictionary sd = p_steps_array[i];
            step->set_title(sd.get("title", godot::String("Step ") + godot::String::num_int64(i + 1)));
            step->set_markdown_body(sd.get("body", ""));
            step->set_target_ui_selector(sd.get("selector", ""));
        } else {
            step->set_title(godot::String(p_steps_array[i]));
        }
        step->set_completion_mode(MANUAL_CHECKBOX);
        tut->add_step(step);
    }

    if (checklist_dock_) {
        checklist_dock_->set_tutorial(tut);
    }
    return tut;
}

bool TutorialToolDispatcher::launch_smart_tool_or_wizard(
    const godot::StringName &p_tool_key,
    const godot::Dictionary &p_preset_dict
) {
    emit_signal("tool_launch_requested", p_tool_key, p_preset_dict);
    const auto *launcher = ToolRegistry::get_singleton().find_workspace_launcher(p_tool_key);
    if (launcher && launcher->open_asset) {
        godot::Ref<godot::Resource> asset;
        if (p_preset_dict.has("asset")) {
            asset = p_preset_dict["asset"];
        }
        launcher->open_asset(asset, tool_context_);
        return true;
    }
    return false;
}

bool TutorialToolDispatcher::demonstrate_step_reversible(
    const godot::String &p_action_name,
    const godot::String &p_target_selector,
    const godot::StringName &p_property,
    const godot::Variant &p_value
) {
    godot::Control *target = TutorialInputValidator::resolve_ui_selector(nullptr, p_target_selector);
    if (!target || agent_bridge_.is_null()) {
        return false;
    }
    return agent_bridge_->dispatch_reversible_property_change(
        tool_context_,
        p_action_name,
        target,
        p_property,
        p_value
    );
}

} // namespace ideam::godot_ext
