#pragma once

/**
 * character_layered_stack.h
 * Non-Destructive 4-Layer Anchored Character Stack & Schema/Instance Core
 * (`ideam::core::BodyPlanCore`, `ideam::core::CharacterInstanceCore`) in `src/core/characters/`.
 *
 * Implements Block 2 Sections 4 & 5:
 *   - Layer 1 (Scaffold & Proportions): `FormScaffoldCore` + `ProportionRulerGraph` (Phase 2)
 *   - Layer 2 (Parametric Topology): Relative curvilinear landmarks `(scaffold_node_id, u, v, d_n, d_u, d_v)`
 *     + symmetry rules (`BILATERAL`, `RADIAL`, `NONE`) + Loomis/Asaro planar break sharpness
 *     + **Topological Bridge Rules** with explicit regime thresholds (`SMOOTH` -> `CREASE` -> `DEEP_FOLD`)
 *       and Loomis Pinch-and-Stretch compression folds for armpits, crotch, and neck-clavicle junctions.
 *   - Layer 3 (Parametric Features): Self-contained sub-patches (Eye, Ear, Nose, Mouth, Horn, Fin, Beak, Claw)
 *     anchored to Layer 2 Topology Frames so upstream scaffold/topology edits carry features non-destructively.
 *   - Layer 4 (Unconstrained Details): Sparse local-frame deltas evaluated last with symmetry forced OFF.
 *   - Schema vs. Instance Split: `BodyPlanCore` defines valid `[min, default, max]` parameter space &
 *     cross-parameter couplings; `CharacterInstanceCore` stores only chosen parameter values + Layer 4 deltas.
 */

#include "character_common.h"
#include "../design/form_scaffold.h"
#include "../surfaces/surface_common.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace ideam::core {

enum class LayerSymmetryPolicy : uint8_t {
    SYM_NONE      = 0,
    SYM_BILATERAL = 1, ///< Mirrors across X = 0 (u -> 1 - u, tangent_u mirrored)
    SYM_RADIAL    = 2  ///< Repeats `radial_count` times around scaffold axis
};

enum class BridgeTopologyRegime : uint8_t {
    BRIDGE_REGIME_SMOOTH    = 0, ///< Continuous smooth skin bridge
    BRIDGE_REGIME_CREASE    = 1, ///< Defined compression crease (Loomis pinch-and-stretch)
    BRIDGE_REGIME_DEEP_FOLD = 2  ///< Deep overlapping fold / re-routed seam (e.g., heavy deltoid/armpit)
};

enum class ParametricFeatureKind : uint8_t {
    FEATURE_EYE    = 0,
    FEATURE_EAR    = 1,
    FEATURE_NOSE   = 2,
    FEATURE_MOUTH  = 3,
    FEATURE_HORN   = 4,
    FEATURE_FIN    = 5,
    FEATURE_BEAK   = 6,
    FEATURE_CLAW   = 7,
    FEATURE_CUSTOM = 8
};

enum class BodyPlanParamCategory : uint8_t {
    PARAM_RULER_UNIT      = 0, ///< Drives `ProportionRulerGraph` root unit (e.g., head_height)
    PARAM_RULER_RATIO     = 1, ///< Drives a named `ProportionRulerGraph` node ratio
    PARAM_CONTRAPPOSTO    = 2, ///< Drives `FormScaffoldCore` contrapposto amount [-1..1]
    PARAM_TOPOLOGY_OFFSET = 3, ///< Modulates a named Layer 2 landmark's normal offset
    PARAM_BRIDGE_METRIC   = 4, ///< Drives a named TopologicalBridgeRule's regime metric (e.g., deltoid_bulk)
    PARAM_FEATURE_SCALE   = 5  ///< Uniformly scales a named Layer 3 feature
};

/**
 * Relative curvilinear anchor riding any Layer 1 scaffold volume `(node_id, u, v)`.
 */
struct RelativeSurfaceAnchor {
    uint32_t scaffold_node_id = 0u;
    float u                   = 0.5f;
    float v                   = 0.5f;
    float normal_offset       = 0.0f;
    float tangent_u_offset    = 0.0f;
    float tangent_v_offset    = 0.0f;
};

/**
 * Layer 2 — Parametric Topology Landmark / Planar Break / Hollow.
 */
struct TopologyLandmark {
    uint32_t id = 0u;
    std::string name;
    RelativeSurfaceAnchor anchor{};
    LayerSymmetryPolicy symmetry = LayerSymmetryPolicy::SYM_BILATERAL;
    uint8_t radial_count         = 1u;
    float influence_radius       = 0.08f; ///< Local falloff radius in meters
    float planar_sharpness       = 0.25f; ///< 0 = smooth organic, 1 = crisp Loomis/Asaro planar break
};

/**
 * Layer 2 — Topological Bridge Rule for inter-scaffold junctions (armpits, crotch, neck-clavicle).
 */
struct TopologicalBridgeRule {
    uint32_t id = 0u;
    std::string name;
    uint32_t node_a_id = 0u;
    uint32_t node_b_id = 0u;
    std::string driving_param_name;    ///< Parameter modulating fold metric (e.g., "deltoid_bulk")
    float base_metric_value    = 0.35f;
    float crease_threshold     = 0.50f; ///< Crossing this switches SMOOTH -> CREASE
    float deep_fold_threshold  = 0.80f; ///< Crossing this switches CREASE -> DEEP_FOLD
    float pinch_fold_depth     = 0.03f; ///< Max inward pinch crease depth in meters
    float stretch_smooth_bias  = 0.50f; ///< Loomis rule: opposite side stretches smooth as pinch deepens
};

struct EvaluatedBridgeState {
    uint32_t rule_id = 0u;
    std::string name;
    float evaluated_metric = 0.0f;
    BridgeTopologyRegime active_regime = BridgeTopologyRegime::BRIDGE_REGIME_SMOOTH;
    float active_fold_depth = 0.0f;
    float active_stretch_factor = 0.0f;
};

/**
 * Layer 3 — Parametric Feature anchored to a Layer 2 Topology Landmark Frame.
 */
struct ParametricFeatureSpec {
    uint32_t id = 0u;
    std::string name;
    ParametricFeatureKind kind   = ParametricFeatureKind::FEATURE_EYE;
    uint32_t parent_landmark_id  = 0u;
    LayerSymmetryPolicy symmetry = LayerSymmetryPolicy::SYM_BILATERAL;
    glm::vec3 local_offset_ntuv  = glm::vec3(0.0f); ///< (d_normal, d_tangent_u, d_tangent_v)
    glm::vec3 base_half_extents  = glm::vec3(0.025f, 0.015f, 0.020f);
    float secondary_param        = 0.0f; ///< e.g. lid_droop, nostril_flare, ear_point, horn_curl
    float planar_sharpness       = 0.20f;
};

/**
 * Layer 4 — Unconstrained Asymmetric Detail Delta (symmetry forced OFF).
 */
struct UnconstrainedDetailDelta {
    uint32_t id = 0u;
    std::string label;
    uint32_t scaffold_node_id = 0u;
    float u                   = 0.5f;
    float v                   = 0.5f;
    glm::vec3 local_delta_ntuv = glm::vec3(0.0f); ///< (d_normal, d_tangent_u, d_tangent_v)
    float radius              = 0.05f;
    bool is_crease            = false;
};

/**
 * Authoring-side parameter bound & cross-parameter coupling (QuantumDarwinism-ready).
 */
struct BodyPlanParameterBound {
    std::string name;
    BodyPlanParamCategory category = BodyPlanParamCategory::PARAM_RULER_RATIO;
    std::string target_name;
    float min_value     = 0.0f;
    float default_value = 0.5f;
    float max_value     = 1.0f;
    LayerSymmetryPolicy symmetry = LayerSymmetryPolicy::SYM_BILATERAL;
};

struct BodyPlanCouplingRule {
    std::string source_param;
    std::string target_param;
    float weight = 0.5f; ///< target += (source - source_default) * weight
};

struct EvaluatedLandmarkFrame {
    uint32_t landmark_id = 0u;
    std::string name;
    bool is_mirrored_copy = false;
    SurfaceFrame3DPOD<float> world_frame{};
};

struct EvaluatedCharacterStack {
    FormScaffoldCore evaluated_scaffold;
    std::vector<EvaluatedLandmarkFrame> landmark_frames;
    std::vector<EvaluatedBridgeState> bridge_states;
    SurfaceQuadMesh3D<float> combined_quad_mesh;
    std::vector<ScaffoldBoneExportEntry> skeleton_plan;
    ScaffoldBalanceReport balance_report{};
};

/**
 * `BodyPlanCore` (Authoring Side — Schema)
 */
class BodyPlanCore {
private:
    std::string species_name_ = "Humanoid";
    FormScaffoldCore base_scaffold_;
    std::vector<BodyPlanParameterBound> parameter_bounds_;
    std::vector<BodyPlanCouplingRule> coupling_rules_;
    std::vector<TopologyLandmark> landmarks_;
    std::vector<TopologicalBridgeRule> bridge_rules_;
    std::vector<ParametricFeatureSpec> features_;

public:
    BodyPlanCore() = default;
    ~BodyPlanCore() = default;

    void clear();

    void set_species_name(std::string_view p_name) { species_name_ = std::string(p_name); }
    [[nodiscard]] std::string_view get_species_name() const noexcept { return species_name_; }

    void set_base_scaffold(const FormScaffoldCore &p_scaffold) { base_scaffold_ = p_scaffold; }
    [[nodiscard]] const FormScaffoldCore &get_base_scaffold() const noexcept { return base_scaffold_; }
    [[nodiscard]] FormScaffoldCore &get_base_scaffold_mut() noexcept { return base_scaffold_; }

    uint32_t add_parameter_bound(const BodyPlanParameterBound &p_bound);
    void add_coupling_rule(const BodyPlanCouplingRule &p_rule);
    uint32_t add_topology_landmark(const TopologyLandmark &p_landmark);
    uint32_t add_bridge_rule(const TopologicalBridgeRule &p_rule);
    uint32_t add_feature(const ParametricFeatureSpec &p_feature);

    [[nodiscard]] const BodyPlanParameterBound *find_parameter_bound(std::string_view p_name) const noexcept;
    [[nodiscard]] const std::vector<BodyPlanParameterBound> &parameter_bounds() const noexcept { return parameter_bounds_; }
    [[nodiscard]] const std::vector<BodyPlanCouplingRule> &coupling_rules() const noexcept { return coupling_rules_; }
    [[nodiscard]] const std::vector<TopologyLandmark> &landmarks() const noexcept { return landmarks_; }
    [[nodiscard]] const std::vector<TopologicalBridgeRule> &bridge_rules() const noexcept { return bridge_rules_; }
    [[nodiscard]] const std::vector<ParametricFeatureSpec> &features() const noexcept { return features_; }

    // --- Built-In Andrew Loomis Body-Plan Schemas ---
    [[nodiscard]] static BodyPlanCore create_loomis_humanoid_plan();
    [[nodiscard]] static BodyPlanCore create_loomis_quadruped_plan();
    [[nodiscard]] static BodyPlanCore create_loomis_avian_plan();
};

/**
 * `CharacterInstanceCore` (Generator Side — Lightweight Instance + Layer 4 Deltas)
 */
class CharacterInstanceCore {
private:
    std::string character_name_ = "Unnamed Character";
    std::unordered_map<std::string, float> parameter_values_;
    std::vector<UnconstrainedDetailDelta> detail_deltas_;

    [[nodiscard]] std::unordered_map<std::string, float> _resolve_effective_parameters(const BodyPlanCore &p_plan) const;

public:
    CharacterInstanceCore() = default;
    ~CharacterInstanceCore() = default;

    void clear();

    void set_character_name(std::string_view p_name) { character_name_ = std::string(p_name); }
    [[nodiscard]] std::string_view get_character_name() const noexcept { return character_name_; }

    void set_parameter_value(std::string_view p_name, float p_value);
    [[nodiscard]] float get_parameter_value(std::string_view p_name, const BodyPlanCore &p_plan) const;
    [[nodiscard]] const std::unordered_map<std::string, float> &parameter_values() const noexcept { return parameter_values_; }

    uint32_t add_detail_delta(const UnconstrainedDetailDelta &p_delta);
    void clear_detail_deltas() { detail_deltas_.clear(); }
    [[nodiscard]] const std::vector<UnconstrainedDetailDelta> &detail_deltas() const noexcept { return detail_deltas_; }

    /**
     * Randomizes all parameters cleanly within `p_plan`'s `[min, max]` bounds using a
     * deterministic SplitMix64 PRNG seeded by `p_seed`.
     */
    void randomize_within_bounds(const BodyPlanCore &p_plan, uint64_t p_seed);

    /**
     * Non-destructively blends this instance's parameter values & Layer 4 deltas with `p_other`.
     */
    [[nodiscard]] CharacterInstanceCore blend_with(
        const CharacterInstanceCore &p_other,
        const BodyPlanCore &p_plan,
        float p_alpha
    ) const;

    /**
     * Executes the full 4-Layer non-destructive stack:
     *   Layer 1 (Scaffold & Proportions) -> Layer 2 (Topology & Bridge Regimes)
     *   -> Layer 3 (Parametric Features) -> Layer 4 (Unconstrained Asymmetric Details).
     */
    [[nodiscard]] EvaluatedCharacterStack evaluate_stack(
        const BodyPlanCore &p_plan,
        uint8_t p_lod_level = 0u
    ) const;
};

} // namespace ideam::core
