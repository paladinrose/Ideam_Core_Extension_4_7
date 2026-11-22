#include "character_layered_stack.h"

#include <algorithm>
#include <cmath>

namespace ideam::core {

namespace {

constexpr float kPi    = 3.14159265358979323846f;
constexpr float kTwoPi = 6.28318530717958647692f;

uint64_t splitmix64_next(uint64_t &r_state) noexcept {
    uint64_t z = (r_state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30u)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27u)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31u);
}

float rand_unit_float(uint64_t &r_state) noexcept {
    return static_cast<float>((splitmix64_next(r_state) >> 40u) & 0xFFFFFFu) / static_cast<float>(0xFFFFFFu);
}

SurfaceQuadMesh3D<float> build_feature_subpatch(
    const ParametricFeatureSpec &p_feat,
    const SurfaceFrame3DPOD<float> &p_host_frame,
    uint32_t p_patch_id,
    uint8_t p_lod_level
) {
    SurfaceQuadMesh3D<float> mesh;
    const uint32_t divs = resolve_surface_lod_divs(6u, p_lod_level);
    const uint32_t cols = divs + 1u;
    const uint32_t rows = divs + 1u;

    const glm::vec3 origin = p_host_frame.offset_point(
        p_feat.local_offset_ntuv.x,
        p_feat.local_offset_ntuv.y,
        p_feat.local_offset_ntuv.z
    );
    const glm::vec3 ext = p_feat.base_half_extents;
    const float sharp = std::clamp(p_feat.planar_sharpness, 0.0f, 1.0f);

    mesh.positions.reserve(cols * rows);
    mesh.normals.reserve(cols * rows);
    mesh.tangents.reserve(cols * rows);
    mesh.uvs.reserve(cols * rows);
    mesh.vertex_patch_ids.reserve(cols * rows);
    mesh.vertex_ring_indices.reserve(cols * rows);
    mesh.vertex_min_lod_levels.reserve(cols * rows);

    for (uint32_t j = 0u; j < rows; ++j) {
        const float v = static_cast<float>(j) / static_cast<float>(divs);
        const float phi = (v - 0.5f) * kPi;
        const float cp = std::cos(phi);
        const float sp = std::sin(phi);
        for (uint32_t i = 0u; i < cols; ++i) {
            const float u = static_cast<float>(i) / static_cast<float>(divs);
            const float theta = u * kTwoPi;
            const float ct = std::cos(theta);
            const float st = std::sin(theta);

            // Shape modulation per feature kind (with Loomis planar sharpness)
            const float p_exp = glm::mix(1.0f, 0.45f, sharp);
            auto spow = [&](float x) { return (x >= 0.0f ? 1.0f : -1.0f) * std::pow(std::abs(x), p_exp); };

            float lx = spow(cp * ct) * ext.x;
            float ly = spow(sp)      * ext.y;
            float lz = spow(cp * st) * ext.z;

            switch (p_feat.kind) {
                case ParametricFeatureKind::FEATURE_EYE:
                    // Lid droop flattens top of eye orb
                    if (ly > 0.0f) ly *= std::clamp(1.0f - p_feat.secondary_param * 0.45f, 0.35f, 1.3f);
                    break;
                case ParametricFeatureKind::FEATURE_EAR:
                    // Ear point pulls upper outer tip
                    if (ly > 0.0f) lx *= (1.0f + p_feat.secondary_param * 0.5f * std::max(0.0f, sp));
                    break;
                case ParametricFeatureKind::FEATURE_NOSE:
                    // Nostril flare widens lower bridge
                    if (ly < 0.0f) lx *= (1.0f + p_feat.secondary_param * 0.4f);
                    lz = std::max(0.0f, lz + ext.z * 0.5f);
                    break;
                case ParametricFeatureKind::FEATURE_HORN:
                case ParametricFeatureKind::FEATURE_BEAK:
                case ParametricFeatureKind::FEATURE_CLAW: {
                    // Tapered projection along outward normal
                    const float tip_taper = std::max(0.08f, 1.0f - v * (0.85f + 0.1f * p_feat.secondary_param));
                    lx = ct * ext.x * tip_taper;
                    ly = st * ext.y * tip_taper;
                    lz = v * ext.z * 2.0f;
                    break;
                }
                default:
                    break;
            }

            const glm::vec3 world_pos = origin + p_host_frame.tangent_u * lx + p_host_frame.tangent_v * ly + p_host_frame.normal * lz;
            glm::vec3 nrm = glm::normalize(world_pos - origin + p_host_frame.normal * 1e-4f);
            mesh.positions.push_back(world_pos);
            mesh.normals.push_back(nrm);
            mesh.tangents.emplace_back(p_host_frame.tangent_u.x, p_host_frame.tangent_u.y, p_host_frame.tangent_u.z, 1.0f);
            mesh.uvs.emplace_back(u, v);
            mesh.vertex_patch_ids.push_back(p_patch_id);
            mesh.vertex_ring_indices.push_back(j);
            mesh.vertex_min_lod_levels.push_back(p_lod_level);
        }
    }

    mesh.quads.reserve(divs * divs);
    mesh.quad_patch_ids.reserve(divs * divs);
    for (uint32_t j = 0u; j < divs; ++j) {
        for (uint32_t i = 0u; i < divs; ++i) {
            const uint32_t v00 = j * cols + i;
            const uint32_t v10 = j * cols + (i + 1u);
            const uint32_t v11 = (j + 1u) * cols + (i + 1u);
            const uint32_t v01 = (j + 1u) * cols + i;
            mesh.quads.emplace_back(v00, v10, v11, v01);
            mesh.quad_patch_ids.push_back(p_patch_id);
        }
    }
    return mesh;
}

} // namespace

// ============================================================================
// BodyPlanCore Implementation
// ============================================================================

void BodyPlanCore::clear() {
    base_scaffold_.clear();
    parameter_bounds_.clear();
    coupling_rules_.clear();
    landmarks_.clear();
    bridge_rules_.clear();
    features_.clear();
}

uint32_t BodyPlanCore::add_parameter_bound(const BodyPlanParameterBound &p_bound) {
    parameter_bounds_.push_back(p_bound);
    return static_cast<uint32_t>(parameter_bounds_.size() - 1u);
}

void BodyPlanCore::add_coupling_rule(const BodyPlanCouplingRule &p_rule) {
    coupling_rules_.push_back(p_rule);
}

uint32_t BodyPlanCore::add_topology_landmark(const TopologyLandmark &p_landmark) {
    TopologyLandmark copy = p_landmark;
    copy.id = static_cast<uint32_t>(landmarks_.size());
    landmarks_.push_back(std::move(copy));
    return static_cast<uint32_t>(landmarks_.size() - 1u);
}

uint32_t BodyPlanCore::add_bridge_rule(const TopologicalBridgeRule &p_rule) {
    TopologicalBridgeRule copy = p_rule;
    copy.id = static_cast<uint32_t>(bridge_rules_.size());
    bridge_rules_.push_back(std::move(copy));
    return static_cast<uint32_t>(bridge_rules_.size() - 1u);
}

uint32_t BodyPlanCore::add_feature(const ParametricFeatureSpec &p_feature) {
    ParametricFeatureSpec copy = p_feature;
    copy.id = static_cast<uint32_t>(features_.size());
    features_.push_back(std::move(copy));
    return static_cast<uint32_t>(features_.size() - 1u);
}

const BodyPlanParameterBound *BodyPlanCore::find_parameter_bound(std::string_view p_name) const noexcept {
    for (const auto &b : parameter_bounds_) {
        if (b.name == p_name) return &b;
    }
    return nullptr;
}

BodyPlanCore BodyPlanCore::create_loomis_humanoid_plan() {
    BodyPlanCore plan;
    plan.set_species_name("Loomis Humanoid");
    plan.set_base_scaffold(FormScaffoldCore::create_loomis_biped_scaffold(LoomisBipedCanon::NORMAL_7_5H, 0.23f));

    const uint32_t head_id   = plan.get_base_scaffold().find_node_id("Head");
    const uint32_t thorax_id = plan.get_base_scaffold().find_node_id("Thorax");
    const uint32_t pelvis_id = plan.get_base_scaffold().find_node_id("Pelvis");
    const uint32_t uarm_l    = plan.get_base_scaffold().find_node_id("UpperArm_L");
    const uint32_t thigh_l   = plan.get_base_scaffold().find_node_id("Thigh_L");

    // Authoring Parameter Space ([min, default, max])
    plan.add_parameter_bound({"head_height",    BodyPlanParamCategory::PARAM_RULER_UNIT,      "head_height",    0.18f, 0.23f, 0.28f, LayerSymmetryPolicy::SYM_NONE});
    plan.add_parameter_bound({"shoulder_width", BodyPlanParamCategory::PARAM_RULER_RATIO,     "shoulder_width", 1.50f, 2.00f, 2.65f, LayerSymmetryPolicy::SYM_BILATERAL});
    plan.add_parameter_bound({"pelvis_width",   BodyPlanParamCategory::PARAM_RULER_RATIO,     "pelvis_width",   1.15f, 1.45f, 1.85f, LayerSymmetryPolicy::SYM_BILATERAL});
    plan.add_parameter_bound({"contrapposto",   BodyPlanParamCategory::PARAM_CONTRAPPOSTO,    "",              -1.00f, 0.00f, 1.00f, LayerSymmetryPolicy::SYM_NONE});
    plan.add_parameter_bound({"brow_ridge",     BodyPlanParamCategory::PARAM_TOPOLOGY_OFFSET, "BrowKeystone",  -0.015f, 0.008f, 0.025f, LayerSymmetryPolicy::SYM_BILATERAL});
    plan.add_parameter_bound({"deltoid_bulk",   BodyPlanParamCategory::PARAM_BRIDGE_METRIC,   "ArmpitBridge",   0.15f, 0.40f, 1.10f, LayerSymmetryPolicy::SYM_BILATERAL});
    plan.add_parameter_bound({"hip_bulk",       BodyPlanParamCategory::PARAM_BRIDGE_METRIC,   "HipCreaseBridge",0.15f, 0.35f, 1.00f, LayerSymmetryPolicy::SYM_BILATERAL});
    plan.add_parameter_bound({"eye_scale",      BodyPlanParamCategory::PARAM_FEATURE_SCALE,   "Eye",            0.70f, 1.00f, 1.45f, LayerSymmetryPolicy::SYM_BILATERAL});
    plan.add_parameter_bound({"ear_scale",      BodyPlanParamCategory::PARAM_FEATURE_SCALE,   "Ear",            0.65f, 1.00f, 1.60f, LayerSymmetryPolicy::SYM_BILATERAL});

    // Cross-parameter coupling (broader shoulders naturally increase deltoid bulk)
    plan.add_coupling_rule({"shoulder_width", "deltoid_bulk", 0.35f});

    // Layer 2 — Loomis Facial & Anatomical Landmarks (relative to Head / Thorax)
    const uint32_t lm_orbit = plan.add_topology_landmark({
        0u, "OrbitalSocket", {head_id, 0.19f, 0.52f, -0.008f, 0.0f, 0.0f}, LayerSymmetryPolicy::SYM_BILATERAL, 1u, 0.04f, 0.35f
    });
    plan.add_topology_landmark({
        0u, "BrowKeystone", {head_id, 0.25f, 0.60f, 0.008f, 0.0f, 0.0f}, LayerSymmetryPolicy::SYM_NONE, 1u, 0.05f, 0.55f
    });
    const uint32_t lm_nose = plan.add_topology_landmark({
        0u, "NoseBridge", {head_id, 0.25f, 0.45f, 0.006f, 0.0f, 0.0f}, LayerSymmetryPolicy::SYM_NONE, 1u, 0.04f, 0.45f
    });
    const uint32_t lm_ear = plan.add_topology_landmark({
        0u, "TemporalEarAnchor", {head_id, 0.02f, 0.48f, 0.004f, 0.0f, 0.0f}, LayerSymmetryPolicy::SYM_BILATERAL, 1u, 0.04f, 0.25f
    });

    // Layer 2 — Topological Bridge Rules (Armpit & Hip Crease)
    plan.add_bridge_rule({0u, "ArmpitBridge", thorax_id, uarm_l, "deltoid_bulk", 0.40f, 0.55f, 0.85f, 0.035f, 0.55f});
    plan.add_bridge_rule({0u, "HipCreaseBridge", pelvis_id, thigh_l, "hip_bulk", 0.35f, 0.52f, 0.82f, 0.030f, 0.45f});

    // Layer 3 — Parametric Features anchored to Layer 2 Landmarks
    plan.add_feature({0u, "Eye",  ParametricFeatureKind::FEATURE_EYE,  lm_orbit, LayerSymmetryPolicy::SYM_BILATERAL, glm::vec3(0.006f, 0.0f, 0.0f), glm::vec3(0.018f, 0.013f, 0.016f), 0.15f, 0.15f});
    plan.add_feature({0u, "Nose", ParametricFeatureKind::FEATURE_NOSE, lm_nose,  LayerSymmetryPolicy::SYM_NONE,      glm::vec3(0.005f, 0.0f, 0.0f), glm::vec3(0.016f, 0.028f, 0.022f), 0.20f, 0.45f});
    plan.add_feature({0u, "Ear",  ParametricFeatureKind::FEATURE_EAR,  lm_ear,   LayerSymmetryPolicy::SYM_BILATERAL, glm::vec3(0.008f, 0.0f, 0.0f), glm::vec3(0.014f, 0.032f, 0.012f), 0.10f, 0.25f});

    return plan;
}

BodyPlanCore BodyPlanCore::create_loomis_quadruped_plan() {
    BodyPlanCore plan;
    plan.set_species_name("Loomis Quadruped");
    plan.set_base_scaffold(FormScaffoldCore::create_loomis_quadruped_scaffold(0.80f));

    const uint32_t skull_id  = plan.get_base_scaffold().find_node_id("Skull");
    const uint32_t thorax_id = plan.get_base_scaffold().find_node_id("Thorax");
    const uint32_t fleg_l    = plan.get_base_scaffold().find_node_id("Foreleg_L");

    plan.add_parameter_bound({"shoulder_height", BodyPlanParamCategory::PARAM_RULER_UNIT,  "shoulder_height", 0.35f, 0.80f, 1.50f, LayerSymmetryPolicy::SYM_NONE});
    plan.add_parameter_bound({"body_length",     BodyPlanParamCategory::PARAM_RULER_RATIO, "body_length",     1.05f, 1.25f, 1.55f, LayerSymmetryPolicy::SYM_NONE});
    plan.add_parameter_bound({"shoulder_bulk",   BodyPlanParamCategory::PARAM_BRIDGE_METRIC,"ForelegBridge",  0.20f, 0.42f, 1.00f, LayerSymmetryPolicy::SYM_BILATERAL});
    plan.add_parameter_bound({"ear_scale",       BodyPlanParamCategory::PARAM_FEATURE_SCALE,"Ear",            0.60f, 1.00f, 1.80f, LayerSymmetryPolicy::SYM_BILATERAL});

    const uint32_t lm_ear = plan.add_topology_landmark({
        0u, "CrownEarSocket", {skull_id, 0.14f, 0.72f, 0.01f, 0.0f, 0.0f}, LayerSymmetryPolicy::SYM_BILATERAL, 1u, 0.05f, 0.30f
    });
    plan.add_bridge_rule({0u, "ForelegBridge", thorax_id, fleg_l, "shoulder_bulk", 0.42f, 0.58f, 0.85f, 0.04f, 0.50f});
    plan.add_feature({0u, "Ear", ParametricFeatureKind::FEATURE_EAR, lm_ear, LayerSymmetryPolicy::SYM_BILATERAL, glm::vec3(0.015f, 0.0f, 0.0f), glm::vec3(0.030f, 0.055f, 0.020f), 0.45f, 0.35f});
    return plan;
}

BodyPlanCore BodyPlanCore::create_loomis_avian_plan() {
    BodyPlanCore plan;
    plan.set_species_name("Loomis Avian");
    plan.set_base_scaffold(FormScaffoldCore::create_loomis_avian_scaffold(0.22f));

    const uint32_t skull_id = plan.get_base_scaffold().find_node_id("Skull");
    const uint32_t keel_id  = plan.get_base_scaffold().find_node_id("Keel");
    const uint32_t wing_l   = plan.get_base_scaffold().find_node_id("WingInner_L");

    plan.add_parameter_bound({"keel_length", BodyPlanParamCategory::PARAM_RULER_UNIT,   "keel_length", 0.10f, 0.22f, 0.65f, LayerSymmetryPolicy::SYM_NONE});
    plan.add_parameter_bound({"wingspan",    BodyPlanParamCategory::PARAM_RULER_RATIO,  "wingspan",    2.20f, 3.20f, 4.50f, LayerSymmetryPolicy::SYM_BILATERAL});
    plan.add_parameter_bound({"wing_fold",   BodyPlanParamCategory::PARAM_BRIDGE_METRIC,"WingRootBridge", 0.20f, 0.45f, 1.00f, LayerSymmetryPolicy::SYM_BILATERAL});
    plan.add_parameter_bound({"beak_scale",  BodyPlanParamCategory::PARAM_FEATURE_SCALE,"Beak",        0.60f, 1.00f, 1.90f, LayerSymmetryPolicy::SYM_NONE});

    const uint32_t lm_beak = plan.add_topology_landmark({
        0u, "BeakRoot", {skull_id, 0.25f, 0.48f, 0.01f, 0.0f, 0.0f}, LayerSymmetryPolicy::SYM_NONE, 1u, 0.03f, 0.65f
    });
    plan.add_bridge_rule({0u, "WingRootBridge", keel_id, wing_l, "wing_fold", 0.45f, 0.60f, 0.85f, 0.025f, 0.60f});
    plan.add_feature({0u, "Beak", ParametricFeatureKind::FEATURE_BEAK, lm_beak, LayerSymmetryPolicy::SYM_NONE, glm::vec3(0.005f, 0.0f, 0.0f), glm::vec3(0.018f, 0.015f, 0.038f), 0.35f, 0.70f});
    return plan;
}

// ============================================================================
// CharacterInstanceCore Implementation
// ============================================================================

void CharacterInstanceCore::clear() {
    parameter_values_.clear();
    detail_deltas_.clear();
}

void CharacterInstanceCore::set_parameter_value(std::string_view p_name, float p_value) {
    parameter_values_[std::string(p_name)] = p_value;
}

float CharacterInstanceCore::get_parameter_value(std::string_view p_name, const BodyPlanCore &p_plan) const {
    auto it = parameter_values_.find(std::string(p_name));
    if (it != parameter_values_.end()) return it->second;
    if (const auto *b = p_plan.find_parameter_bound(p_name)) return b->default_value;
    return 0.0f;
}

uint32_t CharacterInstanceCore::add_detail_delta(const UnconstrainedDetailDelta &p_delta) {
    UnconstrainedDetailDelta copy = p_delta;
    copy.id = static_cast<uint32_t>(detail_deltas_.size());
    detail_deltas_.push_back(std::move(copy));
    return static_cast<uint32_t>(detail_deltas_.size() - 1u);
}

std::unordered_map<std::string, float> CharacterInstanceCore::_resolve_effective_parameters(const BodyPlanCore &p_plan) const {
    std::unordered_map<std::string, float> eff;
    for (const auto &b : p_plan.parameter_bounds()) {
        const float raw = get_parameter_value(b.name, p_plan);
        eff[b.name] = std::clamp(raw, b.min_value, b.max_value);
    }
    // Apply cross-parameter couplings
    for (const auto &c : p_plan.coupling_rules()) {
        const auto *src_b = p_plan.find_parameter_bound(c.source_param);
        const auto *dst_b = p_plan.find_parameter_bound(c.target_param);
        if (!src_b || !dst_b) continue;
        const float delta = (eff[c.source_param] - src_b->default_value) * c.weight;
        eff[c.target_param] = std::clamp(eff[c.target_param] + delta, dst_b->min_value, dst_b->max_value);
    }
    return eff;
}

void CharacterInstanceCore::randomize_within_bounds(const BodyPlanCore &p_plan, uint64_t p_seed) {
    uint64_t rng = (p_seed == 0ULL) ? 0xC0FFEE123456789ULL : p_seed;
    parameter_values_.clear();
    for (const auto &b : p_plan.parameter_bounds()) {
        const float t = rand_unit_float(rng);
        parameter_values_[b.name] = glm::mix(b.min_value, b.max_value, t);
    }
}

CharacterInstanceCore CharacterInstanceCore::blend_with(
    const CharacterInstanceCore &p_other,
    const BodyPlanCore &p_plan,
    float p_alpha
) const {
    const float a = std::clamp(p_alpha, 0.0f, 1.0f);
    CharacterInstanceCore out;
    out.set_character_name(character_name_ + "_blend");
    for (const auto &b : p_plan.parameter_bounds()) {
        const float v0 = get_parameter_value(b.name, p_plan);
        const float v1 = p_other.get_parameter_value(b.name, p_plan);
        out.set_parameter_value(b.name, glm::mix(v0, v1, a));
    }
    for (const auto &d : detail_deltas_) {
        UnconstrainedDetailDelta scaled = d;
        scaled.local_delta_ntuv *= (1.0f - a);
        out.add_detail_delta(scaled);
    }
    for (const auto &d : p_other.detail_deltas_) {
        UnconstrainedDetailDelta scaled = d;
        scaled.local_delta_ntuv *= a;
        out.add_detail_delta(scaled);
    }
    return out;
}

EvaluatedCharacterStack CharacterInstanceCore::evaluate_stack(
    const BodyPlanCore &p_plan,
    uint8_t p_lod_level
) const {
    EvaluatedCharacterStack out;
    const auto eff = _resolve_effective_parameters(p_plan);

    // ========================================================================
    // LAYER 1 — Scaffold & Proportions
    // ========================================================================
    out.evaluated_scaffold = p_plan.get_base_scaffold();
    auto &rg = out.evaluated_scaffold.get_ruler_graph_mut();

    for (const auto &b : p_plan.parameter_bounds()) {
        const float val = eff.at(b.name);
        if (b.category == BodyPlanParamCategory::PARAM_RULER_UNIT) {
            rg.set_base_unit_meters(val);
        } else if (b.category == BodyPlanParamCategory::PARAM_RULER_RATIO) {
            const uint32_t rid = rg.find_node_id(b.target_name);
            if (rid != 0xFFFFFFFFu) rg.set_node_ratio(rid, val);
        } else if (b.category == BodyPlanParamCategory::PARAM_CONTRAPPOSTO) {
            out.evaluated_scaffold.set_contrapposto_amount(val);
        }
    }
    out.evaluated_scaffold.evaluate();
    out.balance_report = out.evaluated_scaffold.compute_balance_report();
    out.skeleton_plan  = out.evaluated_scaffold.build_skeleton_export_plan();

    // Base quad mesh from Layer 1 Scaffold scaled by LOD level
    const uint32_t subdivs = resolve_surface_lod_divs(12u, p_lod_level);
    out.combined_quad_mesh = out.evaluated_scaffold.tessellate_scaffold_mesh(subdivs);

    // ========================================================================
    // LAYER 2 — Parametric Topology Landmarks & Topological Bridge Rules
    // ========================================================================
    std::unordered_map<uint32_t, std::vector<SurfaceFrame3DPOD<float>>> resolved_landmarks;

    for (const auto &lm : p_plan.landmarks()) {
        float extra_n = 0.0f;
        for (const auto &b : p_plan.parameter_bounds()) {
            if (b.category == BodyPlanParamCategory::PARAM_TOPOLOGY_OFFSET && b.target_name == lm.name) {
                extra_n += eff.at(b.name);
            }
        }

        auto eval_one = [&](float p_u, bool p_mirrored) {
            SurfaceFrame3DPOD<float> f = out.evaluated_scaffold.sample_scaffold_frame(lm.anchor.scaffold_node_id, p_u, lm.anchor.v);
            f.position = f.offset_point(lm.anchor.normal_offset + extra_n, lm.anchor.tangent_u_offset, lm.anchor.tangent_v_offset);
            resolved_landmarks[lm.id].push_back(f);
            out.landmark_frames.push_back({lm.id, lm.name, p_mirrored, f});

            // Apply smooth local normal displacement onto host scaffold vertices within influence_radius
            const float r2 = lm.influence_radius * lm.influence_radius;
            const float disp = lm.anchor.normal_offset + extra_n;
            if (r2 > 1e-6f && std::abs(disp) > 1e-5f) {
                for (size_t vi = 0; vi < out.combined_quad_mesh.positions.size(); ++vi) {
                    if (out.combined_quad_mesh.vertex_patch_ids[vi] != lm.anchor.scaffold_node_id) continue;
                    const glm::vec3 d = out.combined_quad_mesh.positions[vi] - f.position;
                    const float dist2 = glm::dot(d, d);
                    if (dist2 < r2) {
                        const float falloff = 1.0f - std::sqrt(dist2) / lm.influence_radius;
                        const float shaped = std::pow(falloff, glm::mix(2.0f, 0.7f, lm.planar_sharpness));
                        out.combined_quad_mesh.positions[vi] += f.normal * (disp * shaped);
                    }
                }
            }
        };

        eval_one(lm.anchor.u, false);
        if (lm.symmetry == LayerSymmetryPolicy::SYM_BILATERAL) {
            // Mirror across front/back plane (u=0.25 is +Z front, so mirror is 0.5 - u wrapped to [0,1])
            float mu = 0.5f - lm.anchor.u;
            if (mu < 0.0f) mu += 1.0f;
            if (std::abs(mu - lm.anchor.u) > 1e-3f) {
                eval_one(mu, true);
            }
        }
    }

    // Evaluate Topological Bridge Rules (Armpit / Hip / Wing-Root regime switching)
    for (const auto &br : p_plan.bridge_rules()) {
        EvaluatedBridgeState st{};
        st.rule_id = br.id;
        st.name    = br.name;
        st.evaluated_metric = br.base_metric_value;
        if (!br.driving_param_name.empty()) {
            auto it = eff.find(br.driving_param_name);
            if (it != eff.end()) st.evaluated_metric = it->second;
        }
        if (st.evaluated_metric >= br.deep_fold_threshold) {
            st.active_regime = BridgeTopologyRegime::BRIDGE_REGIME_DEEP_FOLD;
            st.active_fold_depth = br.pinch_fold_depth;
            st.active_stretch_factor = br.stretch_smooth_bias;
        } else if (st.evaluated_metric >= br.crease_threshold) {
            st.active_regime = BridgeTopologyRegime::BRIDGE_REGIME_CREASE;
            const float t = (st.evaluated_metric - br.crease_threshold) /
                            std::max(1e-4f, br.deep_fold_threshold - br.crease_threshold);
            st.active_fold_depth = br.pinch_fold_depth * (0.35f + 0.65f * t);
            st.active_stretch_factor = br.stretch_smooth_bias * t;
        } else {
            st.active_regime = BridgeTopologyRegime::BRIDGE_REGIME_SMOOTH;
            st.active_fold_depth = 0.0f;
            st.active_stretch_factor = 0.0f;
        }
        out.bridge_states.push_back(std::move(st));
    }

    // ========================================================================
    // LAYER 3 — Parametric Features anchored to Layer 2 Topology Frames
    // ========================================================================
    uint32_t next_patch_id = static_cast<uint32_t>(out.evaluated_scaffold.nodes().size());
    for (const auto &feat : p_plan.features()) {
        auto it = resolved_landmarks.find(feat.parent_landmark_id);
        if (it == resolved_landmarks.end()) continue;

        float scale_mult = 1.0f;
        for (const auto &b : p_plan.parameter_bounds()) {
            if (b.category == BodyPlanParamCategory::PARAM_FEATURE_SCALE && b.target_name == feat.name) {
                scale_mult *= eff.at(b.name);
            }
        }

        ParametricFeatureSpec scaled = feat;
        scaled.base_half_extents *= scale_mult;

        for (const auto &host_frame : it->second) {
            out.combined_quad_mesh.append(build_feature_subpatch(scaled, host_frame, next_patch_id++, p_lod_level));
            if (feat.symmetry == LayerSymmetryPolicy::SYM_NONE) {
                break; // Only emit primary instance when symmetry is NONE
            }
        }
    }

    // ========================================================================
    // LAYER 4 — Unconstrained Asymmetric Details (Symmetry FORCED OFF)
    // ========================================================================
    for (const auto &delta : detail_deltas_) {
        const auto anchor_frame = out.evaluated_scaffold.sample_scaffold_frame(delta.scaffold_node_id, delta.u, delta.v);
        const glm::vec3 world_disp =
            anchor_frame.normal    * delta.local_delta_ntuv.x +
            anchor_frame.tangent_u * delta.local_delta_ntuv.y +
            anchor_frame.tangent_v * delta.local_delta_ntuv.z;

        const float r = std::max(0.005f, delta.radius);
        const float r2 = r * r;
        for (size_t vi = 0; vi < out.combined_quad_mesh.positions.size(); ++vi) {
            const glm::vec3 d = out.combined_quad_mesh.positions[vi] - anchor_frame.position;
            const float dist2 = glm::dot(d, d);
            if (dist2 < r2) {
                const float w = 1.0f - std::sqrt(dist2) / r;
                const float shaped = delta.is_crease ? (w * w * w) : (w * w * (3.0f - 2.0f * w));
                out.combined_quad_mesh.positions[vi] += world_disp * shaped;
            }
        }
    }

    if (!out.combined_quad_mesh.quads.empty()) {
        out.combined_quad_mesh.recompute_smooth_normals();
    }
    return out;
}

} // namespace ideam::core
