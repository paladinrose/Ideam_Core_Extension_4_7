#pragma once

/**
 * agent_tool_bridge.h
 * Tier 5 (`AgentTool`) transport-agnostic SLM/LLM tool-calling bridge (`src/godot/tools/agent/`).
 *
 * Shared foundation for:
 *   - Built-in local SLM workers (GGUF/IPC with grammar-constrained decoding via `IdeamLinguistics`)
 *   - External LLM agents (such as Gemini) running Rubric -> Coverage Heatmap -> Gap-Fill -> Workflow loops
 *   - `IdeamTutorials` Co-Pilot (`TutorialSLMBridge` / `TutorialToolDispatcher`) reversible actions
 */

#include "semantic_style_rubric.h"
#include "../tool_context.h"
#include "../tool_registry.h"
#include "../workflow/semantic_coverage_report.h"
#include "../workflow/workflow_orchestrator.h"

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace ideam::godot_ext {

enum AgentBackendKind : int32_t {
    AGENT_BACKEND_OFFLINE_SEMEME = 0, // Deterministic offline IdeamLinguistics fallback
    AGENT_BACKEND_LOCAL_SLM_IPC  = 1, // Built-in local GGUF/IPC SLM worker
    AGENT_BACKEND_EXTERNAL_LLM   = 2  // External LLM agent hook (e.g. Gemini)
};

class AgentToolBridge : public godot::RefCounted {
    GDCLASS(AgentToolBridge, godot::RefCounted)

private:
    godot::StringName agent_id_ = godot::StringName("DefaultAgentBridge");
    AgentBackendKind backend_kind_ = AGENT_BACKEND_OFFLINE_SEMEME;

protected:
    static void _bind_methods();

public:
    AgentToolBridge() = default;
    ~AgentToolBridge() override = default;

    void set_agent_id(const godot::StringName &p_id) { agent_id_ = p_id; }
    [[nodiscard]] godot::StringName get_agent_id() const { return agent_id_; }

    void set_backend_kind(AgentBackendKind p_kind) { backend_kind_ = p_kind; }
    [[nodiscard]] AgentBackendKind get_backend_kind() const noexcept { return backend_kind_; }

    // Exports JSON-compatible tool schemas from `ToolRegistry` for SLM/LLM tool calling.
    [[nodiscard]] godot::Array export_registered_tool_schemas() const;

    // Sends a Tier 5 `SemanticStyleRubric` to a Tier 4 `WorkflowOrchestrator` and returns the coverage heatmap.
    godot::Ref<SemanticCoverageReport> evaluate_rubric_against_workflow(
        const godot::Ref<SemanticStyleRubric> &p_rubric,
        const godot::Ref<WorkflowOrchestrator> &p_workflow,
        const godot::Ref<ToolContext> &p_context = godot::Ref<ToolContext>(),
        const godot::PackedStringArray &p_local_resource_paths = godot::PackedStringArray()
    );

    // Executes a reversible property mutation wrapped in `ToolContext::get_undo_redo()`.
    bool dispatch_reversible_property_change(
        const godot::Ref<ToolContext> &p_context,
        const godot::String &p_action_name,
        godot::Object *p_target,
        const godot::StringName &p_property,
        const godot::Variant &p_new_value
    );
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::AgentBackendKind);
