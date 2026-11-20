#pragma once

/**
 * tutorial_common.h
 * Pure C++26 core enums and lightweight constants for `IdeamTutorials` (`src/core/tutorials/`).
 * Shared across Editor and Runtime without Godot header dependencies.
 */

#include <cstdint>
#include <string_view>

namespace ideam::core::tutorials {

enum class TutorialCompletionMode : int32_t {
    MANUAL_CHECKBOX       = 0,
    PROPERTY_EQUALS       = 1,
    SIGNAL_EMITTED        = 2,
    UNDO_REDO_ACTION_MATCH = 3,
    TASK_GRAPH_VALIDATED  = 4
};

enum class TutorialTargetContext : int32_t {
    CONTEXT_EDITOR  = 0,
    CONTEXT_RUNTIME = 1,
    CONTEXT_BOTH    = 2
};

inline constexpr std::string_view EDITOR_PROGRESS_PATH  = "res://.ideam_tutorials_progress.cfg";
inline constexpr std::string_view RUNTIME_PROGRESS_PATH = "user://tutorial_progress.cfg";
inline constexpr std::string_view UI_SELECTOR_META_KEY  = "ideam_ui_selector";

} // namespace ideam::core::tutorials
