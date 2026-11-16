#pragma once

#include "../../../i_native_task.h"
#include <godot_cpp/variant/dictionary.hpp>
#include <memory>
#include <cstdint>

namespace ideam::core {

class CharactersQueryTaskRegistry {
public:
    static void init_execution_routing() {}
    static void cleanup_execution_routing() {}
    static void generate_ui_matrices(godot::Dictionary &p_matrix) { (void)p_matrix; }
    static std::unique_ptr<INativeTask> create(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t) { return nullptr; }
};

} // namespace ideam::core
