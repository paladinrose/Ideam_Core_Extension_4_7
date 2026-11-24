#pragma once

/**
 * character_layered_stack.h
 * Non-Destructive 4-Layer Anchored Character Stack & Schema/Instance Core
 * (`ideam::core::BodyPlanCore`, `ideam::core::CharacterInstanceCore`) in `src/core/characters/`.
 *
 * Implements Phases 3 & 4:
 *   - Layer 1 (Scaffold & Proportions): `FormScaffoldCore` + `ProportionRulerGraph`
 *   - Layer 2 (Parametric Topology): Relative curvilinear landmarks `(scaffold_node_id, u, v, d_n, d_u, d_v)`
 *     + `SYM_BILATERAL` & `SYM_RADIAL` symmetry + crease-aware split normals (`recompute_normals_with_creases`)
 *     + live `TopologicalBridgeRule` mesh pinch-and-stretch deformation
 *     + `PartSeamLoopSpec<float>` separable part meshes with cross-part averaged seam normals
 *   - Layer 3 (Parametric Features): Dedicated quad-topology templates (Eye, Ear, Nose, Mouth,
 *     Finger, Toe, Horn, Fin, Beak, Claw) anchored to Layer 2 Topology Frames
 *   - Layer 4 (Non-Destructive `CharacterDetailOverrideStack`): Cross-layer asymmetric overrides
 *     (Scaffold, Topology, Features, Surface sculpts with `vertex_patch_ids` filtering) toggled
 *     non-destructively via `details_enabled`.
 *   - Full CRUD mutators (`update_*` / `remove_*`) on `BodyPlanCore`.
 */

#include "character_common.h"
#include "../design/form_scaffold.h"
#include "../surfaces/surface_common.h"
#include "../surfaces/surface_network_3d.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace ideam::core {

enum class LayerSymmetryPolicy : uint8_t {
    SYM_NONE      = 0,
    SYM_BILATERAL = 1, ///< Mirrors across X = 0
    SYM_RADIAL    = 2  ///< Repeats `radial_count` times evenly around scaffold axis
};

enum class BridgeTopologyRegime : uint8_t {
    BRIDGE_REGIME_SMOOTH    = 0, ///< Continuous smooth skin bridge
    BRIDGE_REGIME_CREASE    = 1, ///< Defined compression crease (Loomis pinch-and-stretch)
    BRIDGE_REGIME_DEEP_FOLD = 2  ///< Deep overlapping fold / re-routed seam
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
    FEATURE_CUSTOM = 8,
    FEATURE_FINGER = 9,
    FEATURE_TOE    = 10
};

enum class BodyPlanParamCategory : uint8_t {
    PARAM_RULER_UNIT      = 0,
    PARAM_RULER_RATIO     = 1,
    PARAM_CONTRAPPOSTO    = 2,
    PARAM_TOPOLOGY_OFFSET = 3,
    PARAM_BRIDGE_METRIC   = 4,
    PARAM_FEATURE_SCALE   = 5
};

struct RelativeSurfaceAnchor {
    uint32_t scaffold_node_id = 0u;
    float u                   = 0.5f;
    float v                   = 0.5f;
    float normal_offset       = 0.0f;
    float tangent_u_offset    = 0.0f;
    float tangent_v_offset    = 0.0f;
};

struct TopologyLandmark {
    uint32_t id = 0u;
    std::string name;
    RelativeSurfaceAnchor anchor{};
    LayerSymmetryPolicy symmetry = LayerSymmetryPolicy::SYM_BILATERAL;
    uint8_t radial_count         = 1u;
    float influence_radius       = 0.08f;
    float planar_sharpness       = 0.25f; ///< 0 = smooth organic, 1 = crisp Loomis/Asaro planar break
};

struct TopologicalBridgeRule {
    uint32_t id = 0u;
    std::string name;
    uint32_t node_a_id = 0u;
    uint32_t node_b_id = 0u;
    std::string driving_param_name;
    float base_metric_value    = 0.35f;
    float crease_threshold     = 0.50f;
    float deep_fold_threshold  = 0.80f;
    float pinch_fold_depth     = 0.03f;
    float stretch_smooth_bias  = 0.50f;
};

struct EvaluatedBridgeState {
    uint32_t rule_id = 0u;
    std::string name;
    float evaluated_metric = 0.0f;
    BridgeTopologyRegime active_regime = BridgeTopologyRegime::BRIDGE_REGIME_SMOOTH;
    float active_fold_depth = 0.0f;
    float active_stretch_factor = 0.0f;
};

struct ParametricFeatureSpec {
    uint32_t id = 0u;
    std::string name;
    ParametricFeatureKind kind   = ParametricFeatureKind::FEATURE_EYE;
    uint32_t parent_landmark_id  = 0u;
    LayerSymmetryPolicy symmetry = LayerSymmetryPolicy::SYM_BILATERAL;
    glm::vec3 local_offset_ntuv  = glm::vec3(0.0f);
    glm::vec3 base_half_extents  = glm::vec3(0.025f, 0.015f, 0.020f);
    float secondary_param        = 0.0f; ///< lid_droop, ear_point, nostril_flare, lip_parting, knuckle_curl, horn_curl
    float planar_sharpness       = 0.20f;
};

struct UnconstrainedDetailDelta {
    uint32_t id = 0u;
    std::string label;
    uint32_t scaffold_node_id  = 0u;
    float u                    = 0.5f;
    float v                    = 0.5f;
    glm::vec3 local_delta_ntuv = glm::vec3(0.0f);
    float radius               = 0.05f;
    bool is_crease             = false;
};

/**
 * Cross-layer non-destructive asymmetric override records (Tool 4 — Details Toggle).
 */
struct ScaffoldDetailOverride {
    uint32_t scaffold_node_id = 0u;
    glm::vec3 offset_delta    = glm::vec3(0.0f);
    glm::vec3 rotation_delta_deg = glm::vec3(0.0f);
    glm::vec3 scale_multiplier = glm::vec3(1.0f);
};

struct TopologyDetailOverride {
    uint32_t landmark_id     = 0u;
    bool target_mirrored_copy = false;
    float normal_offset_delta = 0.0f;
    float crease_sharpness_override = -1.0f; ///< < 0 means unchanged
};

struct FeatureDetailOverride {
    uint32_t feature_id        = 0u;
    bool target_mirrored_copy  = false;
    glm::vec3 offset_delta_ntuv = glm::vec3(0.0f);
    glm::vec3 scale_multiplier = glm::vec3(1.0f);
    float secondary_param_delta = 0.0f;
};

struct CharacterDetailOverrideStack {
    std::vector<ScaffoldDetailOverride> scaffold_overrides;
    std::vector<TopologyDetailOverride> topology_overrides;
    std::vector<FeatureDetailOverride> feature_overrides;
    std::vector<UnconstrainedDetailDelta> surface_sculpt_deltas;

    void clear() {
        scaffold_overrides.clear();
        topology_overrides.clear();
        feature_overrides.clear();
        surface_sculpt_deltas.clear();
    }

    [[nodiscard]] bool is_empty() const noexcept {
        return scaffold_overrides.empty() && topology_overrides.empty() &&
               feature_overrides.empty() && surface_sculpt_deltas.empty();
    }
};

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
    float weight = 0.5f;
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
    std::unordered_map<uint32_t, SurfaceQuadMesh3D<float>> part_meshes; ///< Separable per-part meshes with synced seam normals
    std::vector<ScaffoldBoneExportEntry> skeleton_plan;
    ScaffoldBalanceReport balance_report{};
};

/**
 * `BodyPlanCore` (Authoring Side — Schema with Full CRUD & PartSeamContracts)
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
    std::vector<PartSeamLoopSpec<float>> part_seams_;

public:
    BodyPlanCore() = default;
    ~BodyPlanCore() = default;

    void clear();

    void set_species_name(std::string_view p_name) { species_name_ = std::string(p_name); }
    [[nodiscard]] std::string_view get_species_name() const noexcept { return species_name_; }

    void set_base_scaffold(const FormScaffoldCore &p_scaffold) { base_scaffold_ = p_scaffold; }
    [[nodiscard]] const FormScaffoldCore &get_base_scaffold() const noexcept { return base_scaffold_; }
    [[nodiscard]] FormScaffoldCore &get_base_scaffold_mut() noexcept { return base_scaffold_; }

    // --- Create ---
    uint32_t add_parameter_bound(const BodyPlanParameterBound &p_bound);
    void add_coupling_rule(const BodyPlanCouplingRule &p_rule);
    uint32_t add_topology_landmark(const TopologyLandmark &p_landmark);
    uint32_t add_bridge_rule(const TopologicalBridgeRule &p_rule);
    uint32_t add_feature(const ParametricFeatureSpec &p_feature);
    uint32_t add_part_seam(const PartSeamLoopSpec<float> &p_seam);

    // --- Update & Remove (Phase 4 Non-Linear CRUD) ---
    bool update_parameter_bound(uint32_t p_index, const BodyPlanParameterBound &p_bound);
    bool remove_parameter_bound(uint32_t p_index);
    bool remove_coupling_rule(uint32_t p_index);
    bool update_topology_landmark(uint32_t p_id, const TopologyLandmark &p_landmark);
    bool remove_topology_landmark(uint32_t p_id);
    bool update_bridge_rule(uint32_t p_id, const TopologicalBridgeRule &p_rule);
    bool remove_bridge_rule(uint32_t p_id);
    bool update_feature(uint32_t p_id, const ParametricFeatureSpec &p_feature);
    bool remove_feature(uint32_t p_id);
    bool remove_part_seam(uint32_t p_seam_id);

    void clear_parameter_bounds() { parameter_bounds_.clear(); }
    void clear_coupling_rules() { coupling_rules_.clear(); }
    void clear_landmarks() { landmarks_.clear(); }
    void clear_bridge_rules() { bridge_rules_.clear(); }
    void clear_features() { features_.clear(); }
    void clear_part_seams() { part_seams_.clear(); }

    [[nodiscard]] const BodyPlanParameterBound *find_parameter_bound(std::string_view p_name) const noexcept;
    [[nodiscard]] const std::vector<BodyPlanParameterBound> &parameter_bounds() const noexcept { return parameter_bounds_; }
    [[nodiscard]] const std::vector<BodyPlanCouplingRule> &coupling_rules() const noexcept { return coupling_rules_; }
    [[nodiscard]] const std::vector<TopologyLandmark> &landmarks() const noexcept { return landmarks_; }
    [[nodiscard]] const std::vector<TopologicalBridgeRule> &bridge_rules() const noexcept { return bridge_rules_; }
    [[nodiscard]] const std::vector<ParametricFeatureSpec> &features() const noexcept { return features_; }
    [[nodiscard]] const std::vector<PartSeamLoopSpec<float>> &part_seams() const noexcept { return part_seams_; }

    // --- Built-In Andrew Loomis Body-Plan Schemas ---
    [[nodiscard]] static BodyPlanCore create_loomis_humanoid_plan();
    [[nodiscard]] static BodyPlanCore create_loomis_quadruped_plan();
    [[nodiscard]] static BodyPlanCore create_loomis_avian_plan();
};

/**
 * `CharacterInstanceCore` (Generator Side — Lightweight Instance + Non-Destructive Details Stack)
 */
class CharacterInstanceCore {
private:
    std::string character_name_ = "Unnamed Character";
    std::unordered_map<std::string, float> parameter_values_;
    bool details_enabled_ = true;
    CharacterDetailOverrideStack detail_overrides_;

    [[nodiscard]] std::unordered_map<std::string, float> _resolve_effective_parameters(const BodyPlanCore &p_plan) const;

public:
    CharacterInstanceCore() = default;
    ~CharacterInstanceCore() = default;

    void clear();

    void set_character_name(std::string_view p_name) { character_name_ = std::string(p_name); }
    [[nodiscard]] std::string_view get_character_name() const noexcept { return character_name_; }

    void set_details_enabled(bool p_enabled) noexcept { details_enabled_ = p_enabled; }
    [[nodiscard]] bool get_details_enabled() const noexcept { return details_enabled_; }

    void set_parameter_value(std::string_view p_name, float p_value);
    [[nodiscard]] float get_parameter_value(std::string_view p_name, const BodyPlanCore &p_plan) const;
    [[nodiscard]] const std::unordered_map<std::string, float> &parameter_values() const noexcept { return parameter_values_; }

    // --- Cross-Layer Detail Override Stack ---
    [[nodiscard]] const CharacterDetailOverrideStack &detail_overrides() const noexcept { return detail_overrides_; }
    [[nodiscard]] CharacterDetailOverrideStack &detail_overrides_mut() noexcept { return detail_overrides_; }

    void set_scaffold_override(const ScaffoldDetailOverride &p_override);
    void set_topology_override(const TopologyDetailOverride &p_override);
    void set_feature_override(const FeatureDetailOverride &p_override);

    uint32_t add_detail_delta(const UnconstrainedDetailDelta &p_delta);
    void clear_detail_deltas() { detail_overrides_.surface_sculpt_deltas.clear(); }
    void clear_all_detail_overrides() { detail_overrides_.clear(); }
    [[nodiscard]] const std::vector<UnconstrainedDetailDelta> &detail_deltas() const noexcept {
        return detail_overrides_.surface_sculpt_deltas;
    }

    void randomize_within_bounds(const BodyPlanCore &p_plan, uint64_t p_seed);

    [[nodiscard]] CharacterInstanceCore blend_with(
        const CharacterInstanceCore &p_other,
        const BodyPlanCore &p_plan,
        float p_alpha
    ) const;

    [[nodiscard]] EvaluatedCharacterStack evaluate_stack(
        const BodyPlanCore &p_plan,
        uint8_t p_lod_level = 0u
    ) const;
};

} // namespace ideam::core
