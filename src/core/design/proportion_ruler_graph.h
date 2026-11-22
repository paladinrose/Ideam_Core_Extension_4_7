#pragma once

/**
 * proportion_ruler_graph.h
 * Relational Proportion Ruler Constraint DAG (`ideam::core::ProportionRulerGraph`).
 *
 * Implements Andrew Loomis's canonical-unit proportion system generalized across
 * body plans and non-character assemblies:
 *   - Humanoids / Bipeds: keyed to `HEAD_HEIGHT` (Child 5H, Normal 7.5H, Idealistic 8H, Heroic 8.5H, Stylized 4.5H)
 *   - Quadrupeds: keyed to `SHOULDER_HEIGHT` (ribcage length, chest depth, skull length, limb segments)
 *   - Avians: keyed to `KEEL_LENGTH` (sternum egg, wingspan ratio, wing segments, tarsus)
 *   - Serpentines: keyed to `SKULL_LENGTH` (total body length, peak girth, neck/tail taper)
 *   - Props / Architecture: keyed to `SEAT_HEIGHT` or `CUSTOM` unit
 *
 * Changing the root canonical unit cascades through the topological DAG in O(V+E),
 * updating all landmark heights, scaffold dimensions, and socket offsets before any
 * downstream geometry or character layer evaluates.
 */

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace ideam::core {

enum class ProportionUnitType : uint8_t {
    HEAD_HEIGHT     = 0, ///< Classic Loomis humanoid head-unit
    SHOULDER_HEIGHT = 1, ///< Quadruped comparative box unit
    KEEL_LENGTH     = 2, ///< Avian sternum / keel unit
    SKULL_LENGTH    = 3, ///< Serpentine / reptile skull unit
    SEAT_HEIGHT     = 4, ///< Furniture / prop ergonomic unit
    CUSTOM          = 5
};

enum class LoomisBipedCanon : uint8_t {
    CHILD_5H      = 0, ///< 5.0 heads tall
    NORMAL_7_5H   = 1, ///< 7.5 heads tall (Loomis "Normal")
    IDEALISTIC_8H = 2, ///< 8.0 heads tall (Loomis "Idealistic")
    HEROIC_8_5H   = 3, ///< 8.5 heads tall (Loomis "Heroic")
    STYLIZED_4_5H = 4  ///< 4.5 heads tall (Loomis stylized/chibi)
};

struct ProportionRulerNode {
    uint32_t id        = 0u;
    uint32_t parent_id = 0xFFFFFFFFu; ///< `0xFFFFFFFFu` = root canonical unit
    std::string name;
    float ratio          = 1.0f;      ///< Multiplier applied to parent's computed value
    float bias           = 0.0f;      ///< Additive offset in meters
    float min_value      = 0.001f;
    float max_value      = 100.0f;
    float computed_value = 0.0f;      ///< Output after `evaluate()`
    bool is_landmark     = false;     ///< True if drawn as a tick on the Loomis proportion ruler staff
};

class ProportionRulerGraph {
private:
    ProportionUnitType unit_type_ = ProportionUnitType::HEAD_HEIGHT;
    std::string canon_name_       = "Loomis Normal (7.5H)";
    float base_unit_meters_       = 0.23f;
    std::vector<ProportionRulerNode> nodes_;

public:
    ProportionRulerGraph() = default;
    ~ProportionRulerGraph() = default;

    void clear();

    void set_unit_type(ProportionUnitType p_type) noexcept { unit_type_ = p_type; }
    [[nodiscard]] ProportionUnitType get_unit_type() const noexcept { return unit_type_; }

    void set_canon_name(std::string_view p_name) { canon_name_ = std::string(p_name); }
    [[nodiscard]] std::string_view get_canon_name() const noexcept { return canon_name_; }

    void set_base_unit_meters(float p_meters) noexcept;
    [[nodiscard]] float get_base_unit_meters() const noexcept { return base_unit_meters_; }

    /**
     * Adds a ruler node. Passing `parent_id = 0xFFFFFFFFu` creates/binds to the root unit.
     */
    uint32_t add_node(
        std::string_view p_name,
        uint32_t p_parent_id,
        float p_ratio,
        float p_bias = 0.0f,
        bool p_is_landmark = false,
        float p_min_val = 0.001f,
        float p_max_val = 100.0f
    );

    [[nodiscard]] uint32_t find_node_id(std::string_view p_name) const noexcept;
    [[nodiscard]] const ProportionRulerNode *get_node(uint32_t p_id) const noexcept;
    [[nodiscard]] ProportionRulerNode *get_node_mut(uint32_t p_id) noexcept;
    [[nodiscard]] const std::vector<ProportionRulerNode> &nodes() const noexcept { return nodes_; }

    bool set_node_ratio(uint32_t p_id, float p_ratio) noexcept;
    bool set_node_bias(uint32_t p_id, float p_bias) noexcept;

    /**
     * Evaluates the entire constraint DAG in topological order (cycle-safe).
     * Returns true if the graph is acyclic and all nodes evaluated cleanly.
     */
    bool evaluate();

    [[nodiscard]] float get_value(uint32_t p_id, float p_fallback = 0.0f) const noexcept;
    [[nodiscard]] float get_value_by_name(std::string_view p_name, float p_fallback = 0.0f) const noexcept;

    // ========================================================================
    // Universal Loomis Body-Plan & Prop Canon Presets
    // ========================================================================

    [[nodiscard]] static ProportionRulerGraph create_loomis_biped_canon(
        LoomisBipedCanon p_preset = LoomisBipedCanon::NORMAL_7_5H,
        float p_head_height_m = 0.23f
    );

    [[nodiscard]] static ProportionRulerGraph create_loomis_quadruped_canon(
        float p_shoulder_height_m = 0.80f
    );

    [[nodiscard]] static ProportionRulerGraph create_loomis_avian_canon(
        float p_keel_length_m = 0.22f
    );

    [[nodiscard]] static ProportionRulerGraph create_loomis_serpentine_canon(
        float p_skull_length_m = 0.14f
    );

    [[nodiscard]] static ProportionRulerGraph create_prop_chair_canon(
        float p_seat_height_m = 0.45f
    );
};

} // namespace ideam::core
