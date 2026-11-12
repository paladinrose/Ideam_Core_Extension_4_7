#pragma once

/**
 * wfc_common.h
 * Pure C++26 POD types for `IdeamWaveFunctionCollapse` / `IdeamWFC`
 * (`src/core/wave_function_collapse/`), leaning on `src/core/procedural_generation/procgen_common.h`.
 *
 * Implements the Section 8 Reflection & Hypergraph CSP structures:
 *   - `WFCRuleEntryPOD`: Flat `(port_direction, source_state) -> allowed_neighbor_mask` entry.
 *   - `WFCDirectedEdgePOD`: Directed hypergraph / dual-graph / `Socket3D` adjacency link.
 *   - `WFCObservedCoOccurrencePod`: Structural hierarchy or spatial adjacency rule extracted
 *     by `WFCSceneAnalyzer` from `PackedScene::get_state()` (`SceneState`).
 */

#include "../procedural_generation/procgen_common.h"
#include <cstdint>
#include <glm/glm.hpp>

namespace ideam::core {

struct WFCRuleEntryPOD {
    uint32_t            port_id       = 0;
    uint32_t            source_state  = 0;
    BitmaskDomain256POD allowed_mask{};
};

struct WFCDirectedEdgePOD {
    uint32_t from_slot = 0;
    uint32_t to_slot   = 0;
    uint32_t port_id   = 0;
};

struct WFCObservedCoOccurrencePod {
    uint32_t  parent_or_anchor_state = 0;
    uint32_t  child_or_neighbor_state= 0;
    uint32_t  port_or_socket_tag     = 0;
    uint32_t  min_occurrences        = 1;
    uint32_t  max_occurrences        = 1;
    glm::vec3 quantized_offset_3d    = glm::vec3(0.0f);
};

} // namespace ideam::core
