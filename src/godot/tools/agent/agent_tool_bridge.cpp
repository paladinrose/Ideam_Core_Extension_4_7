#include "agent_tool_bridge.h"

namespace ideam::godot_ext {

void AgentToolBridge::_bind_methods() {
    using namespace godot;

    BIND_ENUM_CONSTANT(AGENT_BACKEND_OFFLINE_SEMEME);
    BIND_ENUM_CONSTANT(AGENT_BACKEND_LOCAL_SLM_IPC);
    BIND_ENUM_CONSTANT(AGENT_BACKEND_EXTERNAL_LLM);

    ADD_SIGNAL(MethodInfo(
        "coverage_evaluated",
        PropertyInfo(Variant::OBJECT, "rubric", PROPERTY_HINT_RESOURCE_TYPE, "SemanticStyleRubric"),
        PropertyInfo(Variant::OBJECT, "report", PROPERTY_HINT_RESOURCE_TYPE, "SemanticCoverageReport")
    ));
    ADD_SIGNAL(MethodInfo(
        "agent_action_dispatched",
        PropertyInfo(Variant::STRING, "action_name"),
        PropertyInfo(Variant::DICTIONARY, "payload")
    ));

    ClassDB::bind_method(D_METHOD("set_agent_id", "id"), &AgentToolBridge::set_agent_id);
    ClassDB::bind_method(D_METHOD("get_agent_id"), &AgentToolBridge::get_agent_id);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "agent_id"), "set_agent_id", "get_agent_id");

    ClassDB::bind_method(D_METHOD("set_backend_kind", "kind"), &AgentToolBridge::set_backend_kind);
    ClassDB::bind_method(D_METHOD("get_backend_kind"), &AgentToolBridge::get_backend_kind);
    ADD_PROPERTY(
        PropertyInfo(
            Variant::INT,
            "backend_kind",
            PROPERTY_HINT_ENUM,
            "Offline Sememe Fallback:0,Local SLM IPC:1,External LLM:2"
        ),
        "set_backend_kind",
        "get_backend_kind"
    );

    ClassDB::bind_method(
        D_METHOD("export_registered_tool_schemas"),
        &AgentToolBridge::export_registered_tool_schemas
    );
    ClassDB::bind_method(
        D_METHOD("evaluate_rubric_against_workflow", "rubric", "workflow", "context", "local_resource_paths"),
        &AgentToolBridge::evaluate_rubric_against_workflow,
        DEFVAL(Ref<ToolContext>()),
        DEFVAL(PackedStringArray())
    );
    ClassDB::bind_method(
        D_METHOD("dispatch_reversible_property_change", "context", "action_name", "target", "property", "new_value"),
        &AgentToolBridge::dispatch_reversible_property_change
    );
}

godot::Array AgentToolBridge::export_registered_tool_schemas() const {
    return ToolRegistry::get_singleton().export_tool_catalog_schema();
}

godot::Ref<SemanticCoverageReport> AgentToolBridge::evaluate_rubric_against_workflow(
    const godot::Ref<SemanticStyleRubric> &p_rubric,
    const godot::Ref<WorkflowOrchestrator> &p_workflow,
    const godot::Ref<ToolContext> &p_context,
    const godot::PackedStringArray &p_local_resource_paths
) {
    if (p_context.is_valid() && p_rubric.is_valid()) {
        p_context->set_active_rubric(p_rubric);
    }

    godot::Ref<SemanticCoverageReport> report;
    if (p_workflow.is_valid()) {
        report = p_workflow->evaluate_semantic_coverage(p_rubric, p_local_resource_paths);
    } else {
        report.instantiate();
    }

    emit_signal("coverage_evaluated", p_rubric, report);
    return report;
}

bool AgentToolBridge::dispatch_reversible_property_change(
    const godot::Ref<ToolContext> &p_context,
    const godot::String &p_action_name,
    godot::Object *p_target,
    const godot::StringName &p_property,
    const godot::Variant &p_new_value
) {
    if (!p_target || p_property == godot::StringName()) {
        return false;
    }

    const godot::Variant old_value = p_target->get(p_property);
    if (p_context.is_valid() && p_context->get_undo_redo().is_valid()) {
        auto ur = p_context->get_undo_redo();
        ur->create_action(p_action_name.is_empty() ? godot::String("Agent Action") : p_action_name);
        ur->add_do_property(p_target, p_property, p_new_value);
        ur->add_undo_property(p_target, p_property, old_value);
        ur->commit_action(true);
    } else {
        p_target->set(p_property, p_new_value);
    }

    godot::Dictionary payload;
    payload["target_class"] = p_target->get_class();
    payload["property"]     = godot::String(p_property);
    payload["old_value"]    = old_value;
    payload["new_value"]    = p_new_value;
    emit_signal("agent_action_dispatched", p_action_name, payload);
    return true;
}

} // namespace ideam::godot_ext
