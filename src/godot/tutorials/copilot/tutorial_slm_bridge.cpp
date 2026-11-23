#include "tutorial_slm_bridge.h"

namespace ideam::godot_ext {

void TutorialSLMBridge::_bind_methods() {
    using namespace godot;

    ADD_SIGNAL(MethodInfo(
        "copilot_response_ready",
        PropertyInfo(Variant::DICTIONARY, "response")
    ));

    ClassDB::bind_method(D_METHOD("set_context_indexer", "indexer"), &TutorialSLMBridge::set_context_indexer);
    ClassDB::bind_method(D_METHOD("get_context_indexer"), &TutorialSLMBridge::get_context_indexer);
    ClassDB::bind_method(D_METHOD("set_tool_dispatcher", "dispatcher"), &TutorialSLMBridge::set_tool_dispatcher);
    ClassDB::bind_method(D_METHOD("get_tool_dispatcher"), &TutorialSLMBridge::get_tool_dispatcher);

    ClassDB::bind_method(D_METHOD("set_slm_ipc_endpoint", "endpoint"), &TutorialSLMBridge::set_slm_ipc_endpoint);
    ClassDB::bind_method(D_METHOD("get_slm_ipc_endpoint"), &TutorialSLMBridge::get_slm_ipc_endpoint);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "slm_ipc_endpoint"), "set_slm_ipc_endpoint", "get_slm_ipc_endpoint");

    ClassDB::bind_method(D_METHOD("query_copilot", "user_prompt"), &TutorialSLMBridge::query_copilot);
}

TutorialSLMBridge::TutorialSLMBridge() {
    set_agent_id(godot::StringName("IdeamTutorialsCoPilot"));
    set_backend_kind(AGENT_BACKEND_OFFLINE_SEMEME);
    context_indexer_.instantiate();
    tool_dispatcher_.instantiate();
    prompt_parser_.instantiate();
}

godot::Dictionary TutorialSLMBridge::query_copilot(const godot::String &p_user_prompt) {
    godot::Dictionary result;
    result["prompt"]       = p_user_prompt;
    result["backend_kind"] = static_cast<int32_t>(get_backend_kind());

    if (prompt_parser_.is_valid()) {
        result["slm_constraints"] = prompt_parser_->compile_prompt_to_slm_constraints(p_user_prompt);
        result["tokens"]          = prompt_parser_->tokenize_and_normalize_prompt(p_user_prompt);
    }

    if (context_indexer_.is_valid()) {
        const godot::Dictionary snapshot = context_indexer_->build_context_snapshot();
        result["context_snapshot"]       = snapshot;

        // Deterministic offline Sememe-matching fallback across indexed tutorials
        const godot::Array tuts = snapshot.get("tutorials", godot::Array());
        float best_score        = -1.0f;
        godot::String best_id;
        for (int i = 0; i < tuts.size(); ++i) {
            const godot::Dictionary td = tuts[i];
            const godot::String title  = td.get("title", "");
            const godot::String desc   = td.get("description", "");
            const float score = prompt_parser_.is_valid()
                ? prompt_parser_->score_prompt_similarity(p_user_prompt, title + godot::String(" ") + desc)
                : 0.0f;
            if (score > best_score) {
                best_score = score;
                best_id    = td.get("tutorial_id", "");
            }
        }
        if (!best_id.is_empty()) {
            result["matched_tutorial_id"] = best_id;
            result["match_score"]         = best_score;
        }
    }

    emit_signal("copilot_response_ready", result);
    return result;
}

} // namespace ideam::godot_ext
