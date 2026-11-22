#pragma once

/**
 * form_scaffold.h
 * Volumetric Andrew Loomis Form Scaffold Engine (`ideam::core::FormScaffoldCore`).
 *
 * Built directly on top of `StructureNodePod`, `SocketPod`, `ProportionRulerGraph`,
 * `Spline3D<float>`, `ParallelTransport3D<float>`, and `SurfaceFrame3DPOD<float>` (from `IdeamSurfaces`):
 *   - 7 low-DoF analytic volumetric primitives:
 *       1. `SPHERE`
 *       2. `SLICED_SPHERE` (Loomis cranium with shaved temporal side planes + jaw/muzzle wedge)
 *       3. `EGG_ELLIPSOID` (Loomis thoracic ribcage / avian keel ovoid)
 *       4. `TAPERED_CAPSULE` (Loomis anti-sausage limb with paired asymmetric rhythm offsets)
 *       5. `WEDGE_BOX` (Loomis pelvis block / hand palm wedge / quadruped hip box / prop seat)
 *       6. `DISC` (Loomis joint disc / scapula / wheel)
 *       7. `GESTURE_SPINE` (`Spline3D` Line of Action swept with Bishop frames)
 *   - Contrapposto counter-tilt & twist along the spine/pelvis/thorax.
 *   - Loomis Center-of-Gravity Plumb Line & Support-Polygon Balance check.
 *   - Analytic curvilinear surface frame query `sample_scaffold_frame(node_id, u, v)`
 *     for non-destructive Phase 3 (`IdeamCharacters`) Topology & Feature anchoring.
 *   - Automatic `Skeleton3D` bone tree & ragdoll/joint-limit export derived from `SocketPod`s.
 */

#include "design_common.h"
#include "proportion_ruler_graph.h"
#include "../splines/spline_3d.h"
#include "../surfaces/surface_common.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace ideam::core {

enum class ScaffoldPrimitiveType : uint8_t {
    PRIMITIVE_SPHERE          = 0,
    PRIMITIVE_SLICED_SPHERE   = 1, ///< Loomis Cranium: shaved temporal planes + optional jaw/muzzle wedge
    PRIMITIVE_EGG_ELLIPSOID   = 2, ///< Loomis Thoracic Egg / Keel Ovoid
    PRIMITIVE_TAPERED_CAPSULE = 3, ///< Loomis Anti-Sausage Limb with asymmetric muscle rhythm
    PRIMITIVE_WEDGE_BOX       = 4, ///< Loomis Pelvis / Palm / Foot Wedge
    PRIMITIVE_DISC            = 5, ///< Joint Disc / Scapula / Wheel
    PRIMITIVE_GESTURE_SPINE   = 6  ///< Loomis Line-of-Action Spline3D + tapered tube
};

/**
 * `ScaffoldNodeEntry`
 * Pairs a `StructureNodePod` & `SocketPod` with low-DoF Loomis volumetric form parameters
 * and optional `ProportionRulerGraph` bindings.
 */
struct ScaffoldNodeEntry {
    uint32_t node_id             = 0u;
    uint32_t parent_id           = 0xFFFFFFFFu;
    std::string name;
    ScaffoldPrimitiveType type   = ScaffoldPrimitiveType::PRIMITIVE_EGG_ELLIPSOID;
    uint32_t semantic_role_id    = SEMANTIC_ROLE_ASSEMBLY_ORGANIC;

    // Embedded SocketPod carrying physics joint mode & angular limits for Skeleton3D / Ragdoll export
    SocketPod socket{};

    // Local placement & base dimensions (in meters before ruler overrides)
    glm::vec3 local_offset       = glm::vec3(0.0f);
    glm::vec3 local_rotation_deg = glm::vec3(0.0f);
    glm::vec3 half_extents       = glm::vec3(0.15f, 0.20f, 0.15f);
    float mass_weight            = 1.0f; ///< Relative mass for Loomis Center-of-Gravity balance
    bool is_ground_support       = false; ///< True for feet/paws/base pads defining the support polygon

    // Optional ProportionRulerGraph node bindings (`0xFFFFFFFFu` = unbound)
    uint32_t ruler_scale_x_id    = 0xFFFFFFFFu;
    uint32_t ruler_scale_y_id    = 0xFFFFFFFFu;
    uint32_t ruler_scale_z_id    = 0xFFFFFFFFu;
    float ruler_mult_x           = 0.5f; ///< Converts full ruler dimension to half-extent
    float ruler_mult_y           = 0.5f;
    float ruler_mult_z           = 0.5f;

    // Loomis Form Shaping Parameters
    float side_shave_ratio       = 0.72f; ///< Sliced Sphere temporal plane distance (fraction of X radius)
    float jaw_drop_ratio         = 0.45f; ///< Hanging jaw wedge drop below cranium equator
    float jaw_width_ratio        = 0.62f; ///< Jaw chin taper relative to cranium width
    float muzzle_length_ratio    = 0.0f;  ///< Forward snout/beak extension (0 for humanoid, >0 for quadruped/avian)
    float egg_taper              = 0.22f; ///< Egg/Ellipsoid vertical fullness asymmetry (-0.5..0.5)
    float top_taper_x            = 1.0f;  ///< Wedge/Capsule distal X taper ratio
    float top_taper_z            = 1.0f;  ///< Wedge/Capsule distal Z taper ratio

    // Loomis Rhythm & Asymmetric Limb Envelope (anti-"sausage limb")
    float asymmetry_high_u       = 0.68f; ///< Normalized V height of outer peak (e.g. high outer calf)
    float asymmetry_high_amp     = 0.0f;  ///< Outer radial bulge amplitude
    float asymmetry_low_u        = 0.42f; ///< Normalized V height of inner peak (e.g. lower inner calf)
    float asymmetry_low_amp      = 0.0f;  ///< Inner radial bulge amplitude

    // Contrapposto response weights (e.g. +1 for ribcage, -1 for pelvis, 0 for static)
    float contrapposto_tilt_weight  = 0.0f;
    float contrapposto_twist_weight = 0.0f;

    // Optional Gesture Spine (used when `type == PRIMITIVE_GESTURE_SPINE`)
    Spline3D<float> gesture_spline;

    // Evaluated outputs (populated by `FormScaffoldCore::evaluate()`)
    glm::vec3 evaluated_half_extents = glm::vec3(0.15f, 0.20f, 0.15f);
    glm::mat4 local_transform        = glm::mat4(1.0f);
    glm::mat4 world_transform        = glm::mat4(1.0f);
    StructureNodePod structure_pod{};
};

/**
 * Derived Skeleton3D / Ragdoll Bone Plan entry emitted from the `SocketPod` hierarchy.
 */
struct ScaffoldBoneExportEntry {
    uint32_t node_id = 0u;
    int32_t parent_bone_index = -1;
    std::string bone_name;
    glm::mat4 rest_local_transform = glm::mat4(1.0f);
    glm::mat4 rest_world_transform = glm::mat4(1.0f);
    glm::vec3 collider_half_extents = glm::vec3(0.1f);
    float mass_kg = 1.0f;
    SocketPhysicsBindingMode physics_binding_mode = SocketPhysicsBindingMode::PHYSICS_BIND_GENERIC_6DOF_JOINT_3D;
    float joint_limit_lower_deg = -45.0f;
    float joint_limit_upper_deg = 45.0f;
    uint8_t joint_axis_index = 0u;
};

/**
 * Loomis Center-of-Gravity & Support-Base Plumb Line Diagnostics.
 */
struct ScaffoldBalanceReport {
    glm::vec3 center_of_gravity = glm::vec3(0.0f);
    glm::vec3 plumb_ground_point = glm::vec3(0.0f);
    glm::vec3 support_center = glm::vec3(0.0f);
    float support_radius = 0.0f;
    float horizontal_offset = 0.0f;
    bool is_balanced = true;
};

class FormScaffoldCore {
private:
    std::string scaffold_name_ = "Loomis Scaffold";
    ProportionRulerGraph ruler_graph_;
    Spline3D<float> line_of_action_;
    bool use_line_of_action_ = false;
    float contrapposto_amount_ = 0.0f; ///< [-1..1] drives opposing shoulder/pelvis tilt & twist
    float total_mass_kg_ = 70.0f;

    std::vector<ScaffoldNodeEntry> nodes_;

    [[nodiscard]] glm::vec3 _eval_local_primitive_surface(
        const ScaffoldNodeEntry &p_node,
        float p_u,
        float p_v
    ) const;

public:
    FormScaffoldCore() = default;
    ~FormScaffoldCore() = default;

    void clear();

    void set_scaffold_name(std::string_view p_name) { scaffold_name_ = std::string(p_name); }
    [[nodiscard]] std::string_view get_scaffold_name() const noexcept { return scaffold_name_; }

    void set_ruler_graph(const ProportionRulerGraph &p_graph) { ruler_graph_ = p_graph; }
    [[nodiscard]] const ProportionRulerGraph &get_ruler_graph() const noexcept { return ruler_graph_; }
    [[nodiscard]] ProportionRulerGraph &get_ruler_graph_mut() noexcept { return ruler_graph_; }

    void set_line_of_action(const Spline3D<float> &p_spline, bool p_enable = true) {
        line_of_action_ = p_spline;
        use_line_of_action_ = p_enable;
    }
    [[nodiscard]] const Spline3D<float> &get_line_of_action() const noexcept { return line_of_action_; }
    [[nodiscard]] bool get_use_line_of_action() const noexcept { return use_line_of_action_; }

    void set_contrapposto_amount(float p_amount) noexcept;
    [[nodiscard]] float get_contrapposto_amount() const noexcept { return contrapposto_amount_; }

    void set_total_mass_kg(float p_mass) noexcept { total_mass_kg_ = std::max(0.1f, p_mass); }
    [[nodiscard]] float get_total_mass_kg() const noexcept { return total_mass_kg_; }

    uint32_t add_node(const ScaffoldNodeEntry &p_entry);
    [[nodiscard]] const ScaffoldNodeEntry *get_node(uint32_t p_id) const noexcept;
    [[nodiscard]] ScaffoldNodeEntry *get_node_mut(uint32_t p_id) noexcept;
    [[nodiscard]] uint32_t find_node_id(std::string_view p_name) const noexcept;
    [[nodiscard]] const std::vector<ScaffoldNodeEntry> &nodes() const noexcept { return nodes_; }

    /**
     * Evaluates `ProportionRulerGraph`, applies Contrapposto & Line of Action, and updates
     * `local_transform`, `world_transform`, and `StructureNodePod` for every scaffold volume.
     */
    void evaluate();

    /**
     * Loomis Center-of-Gravity & Plumb-Line balance evaluation over ground-support nodes.
     */
    [[nodiscard]] ScaffoldBalanceReport compute_balance_report() const;

    /**
     * Non-destructive curvilinear anchoring query `sample_scaffold_frame(node_id, u, v)`.
     * Returns an orthonormal `SurfaceFrame3DPOD<float>` in world space on any scaffold volume.
     */
    [[nodiscard]] SurfaceFrame3DPOD<float> sample_scaffold_frame(
        uint32_t p_node_id,
        float p_u,
        float p_v
    ) const;

    /**
     * Tessellates all scaffold primitives into a unified `SurfaceQuadMesh3D<float>` for
     * live 3D viewport rendering and rough collision hulls.
     */
    [[nodiscard]] SurfaceQuadMesh3D<float> tessellate_scaffold_mesh(uint32_t p_subdivs = 10u) const;

    /**
     * Emits the ordered bone hierarchy and ragdoll/joint limits derived from `SocketPod`s.
     */
    [[nodiscard]] std::vector<ScaffoldBoneExportEntry> build_skeleton_export_plan() const;

    // ========================================================================
    // Universal Andrew Loomis Body-Plan Scaffold Presets
    // ========================================================================

    [[nodiscard]] static FormScaffoldCore create_loomis_biped_scaffold(
        LoomisBipedCanon p_canon = LoomisBipedCanon::NORMAL_7_5H,
        float p_head_height_m = 0.23f
    );

    [[nodiscard]] static FormScaffoldCore create_loomis_quadruped_scaffold(
        float p_shoulder_height_m = 0.80f
    );

    [[nodiscard]] static FormScaffoldCore create_loomis_avian_scaffold(
        float p_keel_length_m = 0.22f
    );

    [[nodiscard]] static FormScaffoldCore create_loomis_serpentine_scaffold(
        float p_skull_length_m = 0.14f,
        uint32_t p_segments = 10u
    );
};

} // namespace ideam::core
