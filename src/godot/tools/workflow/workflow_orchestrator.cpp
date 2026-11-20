#include "workflow_orchestrator.h"

namespace ideam::godot_ext {

void WorkflowOrchestrator::_bind_methods() {
    using namespace godot;

    ADD_SIGNAL(MethodInfo(
        "workflow_completed",
        PropertyInfo(Variant::STRING_NAME, "workflow_id"),
        PropertyInfo(Variant::OBJECT, "receipt", PROPERTY_HINT_RESOURCE_TYPE, "BridgeHandoffReceipt")
    ));

    ClassDB::bind_method(D_METHOD("set_workflow_id", "id"), &WorkflowOrchestrator::set_workflow_id);
    ClassDB::bind_method(D_METHOD("get_workflow_id"), &WorkflowOrchestrator::get_workflow_id);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "workflow_id"), "set_workflow_id", "get_workflow_id");

    ClassDB::bind_method(D_METHOD("set_workflow_profile", "profile"), &WorkflowOrchestrator::set_workflow_profile);
    ClassDB::bind_method(D_METHOD("get_workflow_profile"), &WorkflowOrchestrator::get_workflow_profile);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "workflow_profile", PROPERTY_HINT_RESOURCE_TYPE, "WorkflowProfile"),
        "set_workflow_profile",
        "get_workflow_profile"
    );

    ClassDB::bind_method(D_METHOD("get_last_receipt"), &WorkflowOrchestrator::get_last_receipt);

    ClassDB::bind_method(
        D_METHOD("evaluate_semantic_coverage", "rubric", "local_resource_paths"),
        &WorkflowOrchestrator::evaluate_semantic_coverage,
        DEFVAL(PackedStringArray())
    );

    ClassDB::bind_method(
        D_METHOD("execute_workflow", "context", "inputs"),
        &WorkflowOrchestrator::execute_workflow,
        DEFVAL(Dictionary())
    );
}

WorkflowOrchestrator::WorkflowOrchestrator() {
    workflow_profile_.instantiate();
}

godot::Ref<BridgeHandoffReceipt> WorkflowOrchestrator::_execute_workflow_impl(
    const godot::Ref<ToolContext> &p_context,
    const godot::Dictionary &p_inputs
) {
    (void)p_context;
    (void)p_inputs;
    godot::Ref<BridgeHandoffReceipt> receipt;
    receipt.instantiate();
    receipt->set_source_bridge_id(workflow_id_);
    return receipt;
}

godot::Ref<SemanticCoverageReport> WorkflowOrchestrator::evaluate_semantic_coverage(
    const godot::Ref<godot::Resource> &p_rubric,
    const godot::PackedStringArray &p_local_resource_paths
) const {
    (void)p_local_resource_paths;
    godot::Ref<SemanticCoverageReport> report;
    report.instantiate();

    if (p_rubric.is_valid()) {
        if (p_rubric->has_method("get_rubric_id")) {
            report->set_rubric_id(godot::StringName(godot::String(p_rubric->call("get_rubric_id"))));
        }
        if (p_rubric->has_method("get_prioritized_tags") && p_rubric->has_method("get_coverage_thresholds")) {
            const godot::Dictionary tags       = p_rubric->call("get_prioritized_tags");
            const godot::Dictionary thresholds = p_rubric->call("get_coverage_thresholds");
            const godot::Array keys            = tags.keys();
            for (int i = 0; i < keys.size(); ++i) {
                const godot::String tag = keys[i];
                const float threshold   = thresholds.has(tag) ? static_cast<float>(thresholds[tag]) : 0.5f;
                // Subclasses (e.g. GameWizardWorkflow) query MetadataLinguisticsManager / local indexes.
                report->record_tag_coverage(tag, 0.0f, threshold);
            }
        }
    }
    return report;
}

godot::Ref<BridgeHandoffReceipt> WorkflowOrchestrator::execute_workflow(
    const godot::Ref<ToolContext> &p_context,
    const godot::Dictionary &p_inputs
) {
    godot::Ref<ToolContext> ctx = p_context;
    if (ctx.is_null()) {
        ctx.instantiate();
    }

    if (workflow_profile_.is_valid()) {
        workflow_profile_->apply_overrides_to_context(ctx);
    }

    last_receipt_ = _execute_workflow_impl(ctx, p_inputs);

    if (workflow_profile_.is_valid()) {
        workflow_profile_->release_overrides_from_context(ctx);
    }

    emit_signal("workflow_completed", workflow_id_, last_receipt_);
    return last_receipt_;
}

} // namespace ideam::godot_ext
