#pragma once

/**
 * tutorial_slm_bridge.h
 * Co-Pilot Layer neuro-symbolic bridge (`src/godot/tutorials/copilot/`).
 *
 * First concrete consumer of Tier 5 `AgentToolBridge`:
 *   - Uses `SemanticPromptParser` for grammar-constrained SLM prompt compilation
 *     (`compile_prompt_to_slm_constraints()`) and deterministic offline Sememe-matching fallback.
 *   - Queries `TutorialContextIndexer` and dispatches reversible actions via `TutorialToolDispatcher`.
 */

#include "tutorial_context_indexer.h"
#include "tutorial_tool_dispatcher.h"
#include "../../linguistics/semantic_prompt_parser.h"
#include "../../tools/agent/agent_tool_bridge.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

namespace ideam::godot_ext {

class TutorialSLMBridge : public AgentToolBridge {
    GDCLASS(TutorialSLMBridge, AgentToolBridge)

private:
    godot::Ref<TutorialContextIndexer> context_indexer_;
    godot::Ref<TutorialToolDispatcher> tool_dispatcher_;
    godot::Ref<SemanticPromptParser> prompt_parser_;
    godot::String slm_ipc_endpoint_;

protected:
    static void _bind_methods();

public:
    TutorialSLMBridge();
    ~TutorialSLMBridge() override = default;

    void set_context_indexer(const godot::Ref<TutorialContextIndexer> &p_indexer) { context_indexer_ = p_indexer; }
    [[nodiscard]] godot::Ref<TutorialContextIndexer> get_context_indexer() const { return context_indexer_; }

    void set_tool_dispatcher(const godot::Ref<TutorialToolDispatcher> &p_dispatcher) { tool_dispatcher_ = p_dispatcher; }
    [[nodiscard]] godot::Ref<TutorialToolDispatcher> get_tool_dispatcher() const { return tool_dispatcher_; }

    void set_slm_ipc_endpoint(const godot::String &p_endpoint) { slm_ipc_endpoint_ = p_endpoint; }
    [[nodiscard]] godot::String get_slm_ipc_endpoint() const { return slm_ipc_endpoint_; }

    // Compiles a user query with IdeamLinguistics constraints and runs offline Sememe-matching fallback
    // (or forwards to the configured GGUF/IPC SLM worker when connected).
    [[nodiscard]] godot::Dictionary query_copilot(const godot::String &p_user_prompt);
};

} // namespace ideam::godot_ext
