#pragma once

/**
 * workflow_orchestrator.h
 * Tier 4 (`WorkflowTool`) execution base (`src/godot/tools/workflow/`).
 *
 * Coordinates `BridgeTool`s and `SmartTool`s under a scoped `WorkflowProfile`:
 *   - Automatically pushes `WorkflowProfile` overrides into `ToolContext` before execution and
 *     pops them on exit so global user preferences for Tier 3 `SmartTool`s stay untouched.
 *   - Exposes `evaluate_semantic_coverage()` for Tier 5 `AgentTool` rubric heatmaps.
 *   - Emits `BridgeHandoffReceipt` metacontent receipts so generated outputs remain editable by
 *     all lower-tier tools.
 */

#include "semantic_coverage_report.h"
#include "workflow_profile.h"
#include "../bridge/bridge_contract.h"
#include "../tool_context.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace ideam::godot_ext {

class WorkflowOrchestrator : public godot::RefCounted {
    GDCLASS(WorkflowOrchestrator, godot::RefCounted)

protected:
    godot::StringName workflow_id_;
    godot::Ref<WorkflowProfile> workflow_profile_;
    godot::Ref<BridgeHandoffReceipt> last_receipt_;

    static void _bind_methods();

    // Subclasses override this to run TaskGraphs, ProcGen, and BridgeTools under active overrides.
    virtual godot::Ref<BridgeHandoffReceipt> _execute_workflow_impl(
        const godot::Ref<ToolContext> &p_context,
        const godot::Dictionary &p_inputs
    );

public:
    WorkflowOrchestrator();
    ~WorkflowOrchestrator() override = default;

    void set_workflow_id(const godot::StringName &p_id) { workflow_id_ = p_id; }
    [[nodiscard]] godot::StringName get_workflow_id() const { return workflow_id_; }

    void set_workflow_profile(const godot::Ref<WorkflowProfile> &p_profile) { workflow_profile_ = p_profile; }
    [[nodiscard]] godot::Ref<WorkflowProfile> get_workflow_profile() const { return workflow_profile_; }

    [[nodiscard]] godot::Ref<BridgeHandoffReceipt> get_last_receipt() const { return last_receipt_; }

    // Evaluates local resource coverage against a Tier 5 SemanticStyleRubric (or tag/threshold Dictionary).
    virtual godot::Ref<SemanticCoverageReport> evaluate_semantic_coverage(
        const godot::Ref<godot::Resource> &p_rubric,
        const godot::PackedStringArray &p_local_resource_paths = godot::PackedStringArray()
    ) const;

    // Scoped execution wrapper: pushes WorkflowProfile overrides, executes, pops overrides, emits signal.
    godot::Ref<BridgeHandoffReceipt> execute_workflow(
        const godot::Ref<ToolContext> &p_context,
        const godot::Dictionary &p_inputs = godot::Dictionary()
    );
};

} // namespace ideam::godot_ext
