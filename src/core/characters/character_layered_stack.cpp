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

/**
 * Builds dedicated all-quad feature sub-patches for Layer 3 Parametric Features
 * (Eye with eyelid loops, Ear with helix/lobe contour, Mouth with orbicularis concentric loops
 * & lip parting, Finger/Toe with knuckle-segmented phalanx loops, Nose, Horn, Fin, Beak, Claw).
 */
SurfaceQuadMesh3D<float> build_feature_subpatch(
    const ParametricFeatureSpec &p_feat,
    const SurfaceFrame3DPOD<float> &p_host_frame,
    uint32_t p_patch_id,
    uint8_t p_lod_level
) {
    SurfaceQuadMesh3D<float> mesh;
    const uint32_t divs = resolve_surface_lod_divs(8u, p_lod_level);
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
    mesh.vertex_crease_sharpness.reserve(cols * rows);

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

            const float p_exp = glm::mix(1.0f, 0.45f, sharp);
            auto spow = [&](float x) { return (x >= 0.0f ? 1.0f : -1.0f) * std::pow(std::abs(x), p_exp); };

            float lx = spow(cp * ct) * ext.x;
            float ly = spow(sp)      * ext.y;
            float lz = spow(cp * st) * ext.z;
            float v_crease = sharp * 0.35f;

            switch (p_feat.kind) {
                case ParametricFeatureKind::FEATURE_EYE: {
                    // Eyelid loops + orbital recess around equator
                    const float lid_droop = std::clamp(p_feat.secondary_param, -0.5f, 0.8f);
                    if (ly > 0.0f) {
                        ly *= std::clamp(1.0f - lid_droop * 0.45f, 0.35f, 1.35f);
                    }
                    // Subtle eyelid crease along upper/lower rim
                    const float rim = std::exp(-std::pow((v - 0.68f) / 0.08f, 2.0f));
                    lz += rim * ext.z * 0.22f;
                    v_crease = std::max(v_crease, rim * 0.72f);
                    break;
                }
                case ParametricFeatureKind::FEATURE_EAR: {
                    // Helix outer ridge + antihelix bowl + lower lobe
                    const float ear_point = std::clamp(p_feat.secondary_param, 0.0f, 1.2f);
                    if (ly > 0.0f) {
                        lx *= (1.0f + ear_point * 0.55f * std::max(0.0f, sp));
                    }
                    const float concha_bowl = std::exp(-std::pow((v - 0.45f) / 0.22f, 2.0f)) * std::max(0.0f, ct);
                    lz -= concha_bowl * ext.z * 0.42f;
                    const float helix_rim = std::exp(-std::pow((v - 0.78f) / 0.14f, 2.0f));
                    lz += helix_rim * ext.z * 0.28f;
                    v_crease = std::max(v_crease, helix_rim * 0.55f);
                    break;
                }
                case ParametricFeatureKind::FEATURE_MOUTH: {
                    // Orbicularis concentric quad loops with upper/lower lip parting
                    const float part = std::clamp(p_feat.secondary_param, 0.0f, 1.0f);
                    const float ring_r = 0.25f + 0.75f * v;
                    lx = ct * ext.x * ring_r;
                    ly = st * ext.y * ring_r * (1.0f + 0.35f * part);
                    // Lip fullness ridge around mid-ring, recessed inward at mouth parting (v near 0)
                    const float lip_fullness = std::sin(v * kPi);
                    const float cupid_bow = (st > 0.0f) ? (1.0f + 0.18f * std::cos(2.0f * theta)) : 1.0f;
                    lz = (lip_fullness * cupid_bow * 0.85f - (1.0f - v) * (0.45f + 0.5f * part)) * ext.z;
                    v_crease = (v < 0.22f) ? 0.78f : (sharp * 0.30f);
                    break;
                }
                case ParametricFeatureKind::FEATURE_FINGER:
                case ParametricFeatureKind::FEATURE_TOE: {
                    // Knuckle-segmented phalanx quad tube with subtle dorsal/palmar curl
                    const float curl = std::clamp(p_feat.secondary_param, -0.5f, 1.2f);
                    const float seg_len = ext.z * 2.0f;
                    const float knuckle_bump =
                        0.12f * std::exp(-std::pow((v - 0.33f) / 0.06f, 2.0f)) +
                        0.10f * std::exp(-std::pow((v - 0.66f) / 0.06f, 2.0f));
                    const float tip_cap = std::pow(std::sin(std::clamp(v, 0.002f, 0.998f) * kPi), 0.22f);
                    const float taper = glm::mix(1.0f, 0.62f, v) * tip_cap * (1.0f + knuckle_bump);
                    lx = ct * ext.x * taper;
                    ly = st * ext.y * taper - curl * v * v * seg_len * 0.25f;
                    lz = v * seg_len;
                    v_crease = std::max(v_crease, knuckle_bump * 3.5f);
                    break;
                }
                case ParametricFeatureKind::FEATURE_NOSE: {
                    if (ly < 0.0f) lx *= (1.0f + p_feat.secondary_param * 0.4f);
                    lz = std::max(0.0f, lz + ext.z * 0.55f);
                    v_crease = std::max(v_crease, sharp * 0.75f);
                    break;
                }
                case ParametricFeatureKind::FEATURE_FIN: {
                    // Swept hydrodynamic blade fin with dorsal rake
                    const float rake = p_feat.secondary_param;
                    const float chord = glm::mix(1.0f, 0.22f, v);
                    lx = ct * ext.x * chord * 0.35f;
                    ly = st * ext.y * chord + v * rake * ext.y * 0.6f;
                    lz = v * ext.z * 2.0f;
                    v_crease = 0.68f;
                    break;
                }
                case ParametricFeatureKind::FEATURE_HORN:
                case ParametricFeatureKind::FEATURE_BEAK:
                case ParametricFeatureKind::FEATURE_CLAW: {
                    const float tip_taper = std::max(0.06f, 1.0f - v * (0.88f + 0.08f * p_feat.secondary_param));
                    const float curl = p_feat.secondary_param * v * v * ext.z * 0.5f;
                    lx = ct * ext.x * tip_taper;
                    ly = st * ext.y * tip_taper + curl;
                    lz = v * ext.z * 2.0f;
                    v_crease = std::max(v_crease, sharp * 0.65f);
                    break;
                }
                case ParametricFeatureKind::FEATURE_CUSTOM:
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
            mesh.vertex_crease_sharpness.push_back(std::clamp(v_crease, 0.0f, 1.0f));
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
// BodyPlanCore Implementation (with Full CRUD)
// ============================================================================

void BodyPlanCore::clear() {
    base_scaffold_.clear();
    parameter_bounds_.clear();
    coupling_rules_.clear();
    landmarks_.clear();
    bridge_rules_.clear();
    features_.clear();
    part_seams_.clear();
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
    uint32_t next_id = 0u;
    for (const auto &lm : landmarks_) next_id = std::max(next_id, lm.id + 1u);
    copy.id = next_id;
    landmarks_.push_back(std::move(copy));
    return copy.id;
}

uint32_t BodyPlanCore::add_bridge_rule(const TopologicalBridgeRule &p_rule) {
    TopologicalBridgeRule copy = p_rule;
    uint32_t next_id = 0u;
    for (const auto &br : bridge_rules_) next_id = std::max(next_id, br.id + 1u);
    copy.id = next_id;
    bridge_rules_.push_back(std::move(copy));
    return copy.id;
}

uint32_t BodyPlanCore::add_feature(const ParametricFeatureSpec &p_feature) {
    ParametricFeatureSpec copy = p_feature;
    uint32_t next_id = 0u;
    for (const auto &f : features_) next_id = std::max(next_id, f.id + 1u);
    copy.id = next_id;
    features_.push_back(std::move(copy));
    return copy.id;
}

uint32_t BodyPlanCore::add_part_seam(const PartSeamLoopSpec<float> &p_seam) {
    PartSeamLoopSpec<float> copy = p_seam;
    uint32_t next_id = 0u;
    for (const auto &s : part_seams_) next_id = std::max(next_id, s.seam_id + 1u);
    copy.seam_id = next_id;
    part_seams_.push_back(std::move(copy));
    return copy.seam_id;
}

bool BodyPlanCore::update_parameter_bound(uint32_t p_index, const BodyPlanParameterBound &p_bound) {
    if (p_index >= parameter_bounds_.size()) return false;
    parameter_bounds_[p_index] = p_bound;
    return true;
}

bool BodyPlanCore::remove_parameter_bound(uint32_t p_index) {
    if (p_index >= parameter_bounds_.size()) return false;
    parameter_bounds_.erase(parameter_bounds_.begin() + static_cast<ptrdiff_t>(p_index));
    return true;
}

bool BodyPlanCore::remove_coupling_rule(uint32_t p_index) {
    if (p_index >= coupling_rules_.size()) return false;
    coupling_rules_.erase(coupling_rules_.begin() + static_cast<ptrdiff_t>(p_index));
    return true;
}

bool BodyPlanCore::update_topology_landmark(uint32_t p_id, const TopologyLandmark &p_landmark) {
    for (auto &lm : landmarks_) {
        if (lm.id == p_id) {
            lm = p_landmark;
            lm.id = p_id;
            return true;
        }
    }
    return false;
}

bool BodyPlanCore::remove_topology_landmark(uint32_t p_id) {
    const auto old_sz = landmarks_.size();
    std::erase_if(landmarks_, [&](const TopologyLandmark &lm) { return lm.id == p_id; });
    return landmarks_.size() != old_sz;
}

bool BodyPlanCore::update_bridge_rule(uint32_t p_id, const TopologicalBridgeRule &p_rule) {
    for (auto &br : bridge_rules_) {
        if (br.id == p_id) {
            br = p_rule;
            br.id = p_id;
            return true;
        }
    }
    return false;
}

bool BodyPlanCore::remove_bridge_rule(uint32_t p_id) {
    const auto old_sz = bridge_rules_.size();
    std::erase_if(bridge_rules_, [&](const TopologicalBridgeRule &br) { return br.id == p_id; });
    return bridge_rules_.size() != old_sz;
}

bool BodyPlanCore::update_feature(uint32_t p_id, const ParametricFeatureSpec &p_feature) {
    for (auto &f : features_) {
        if (f.id == p_id) {
            f = p_feature;
            f.id = p_id;
            return true;
        }
    }
    return false;
}

bool BodyPlanCore::remove_feature(uint32_t p_id) {
    const auto old_sz = features_.size();
    std::erase_if(features_, [&](const ParametricFeatureSpec &f) { return f.id == p_id; });
    return features_.size() != old_sz;
}

bool BodyPlanCore::remove_part_seam(uint32_t p_seam_id) {
    const auto old_sz = part_seams_.size();
    std::erase_if(part_seams_, [&](const PartSeamLoopSpec<float> &s) { return s.seam_id == p_seam_id; });
    return part_seams_.size() != old_sz;
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
    const uint32_t neck_id   = plan.get_base_scaffold().find_node_id("Neck");
    const uint32_t uarm_l    = plan.get_base_scaffold().find_node_id("UpperArm_L");
    const uint32_t uarm_r    = plan.get_base_scaffold().find_node_id("UpperArm_R");
    const uint32_t thigh_l   = plan.get_base_scaffold().find_node_id("Thigh_L");
    const uint32_t thigh_r   = plan.get_base_scaffold().find_node_id("Thigh_R");

    plan.add_parameter_bound({"head_height",    BodyPlanParamCategory::PARAM_RULER_UNIT,      "head_height",    0.18f, 0.23f, 0.28f, LayerSymmetryPolicy::SYM_NONE});
    plan.add_parameter_bound({"shoulder_width", BodyPlanParamCategory::PARAM_RULER_RATIO,     "shoulder_width", 1.50f, 2.00f, 2.65f, LayerSymmetryPolicy::SYM_BILATERAL});
    plan.add_parameter_bound({"pelvis_width",   BodyPlanParamCategory::PARAM_RULER_RATIO,     "pelvis_width",   1.15f, 1.45f, 1.85f, LayerSymmetryPolicy::SYM_BILATERAL});
    plan.add_parameter_bound({"contrapposto",   BodyPlanParamCategory::PARAM_CONTRAPPOSTO,    "",              -1.00f, 0.00f, 1.00f, LayerSymmetryPolicy::SYM_NONE});
    plan.add_parameter_bound({"brow_ridge",     BodyPlanParamCategory::PARAM_TOPOLOGY_OFFSET, "BrowKeystone",  -0.015f, 0.008f, 0.025f, LayerSymmetryPolicy::SYM_BILATERAL});
    plan.add_parameter_bound({"deltoid_bulk",   BodyPlanParamCategory::PARAM_BRIDGE_METRIC,   "ArmpitBridge",   0.15f, 0.40f, 1.10f, LayerSymmetryPolicy::SYM_BILATERAL});
    plan.add_parameter_bound({"hip_bulk",       BodyPlanParamCategory::PARAM_BRIDGE_METRIC,   "HipCreaseBridge",0.15f, 0.35f, 1.00f, LayerSymmetryPolicy::SYM_BILATERAL});
    plan.add_parameter_bound({"eye_scale",      BodyPlanParamCategory::PARAM_FEATURE_SCALE,   "Eye",            0.70f, 1.00f, 1.45f, LayerSymmetryPolicy::SYM_BILATERAL});
    plan.add_parameter_bound({"ear_scale",      BodyPlanParamCategory::PARAM_FEATURE_SCALE,   "Ear",            0.65f, 1.00f, 1.60f, LayerSymmetryPolicy::SYM_BILATERAL});
    plan.add_parameter_bound({"mouth_scale",    BodyPlanParamCategory::PARAM_FEATURE_SCALE,   "Mouth",          0.70f, 1.00f, 1.45f, LayerSymmetryPolicy::SYM_NONE});

    plan.add_coupling_rule({"shoulder_width", "deltoid_bulk", 0.35f});

    // Layer 2 — Loomis Facial & Anatomical Landmarks
    const uint32_t lm_orbit = plan.add_topology_landmark({
        0u, "OrbitalSocket", {head_id, 0.19f, 0.52f, -0.008f, 0.0f, 0.0f}, LayerSymmetryPolicy::SYM_BILATERAL, 1u, 0.04f, 0.35f
    });
    plan.add_topology_landmark({
        0u, "BrowKeystone", {head_id, 0.25f, 0.60f, 0.008f, 0.0f, 0.0f}, LayerSymmetryPolicy::SYM_NONE, 1u, 0.05f, 0.72f
    });
    const uint32_t lm_nose = plan.add_topology_landmark({
        0u, "NoseBridge", {head_id, 0.25f, 0.45f, 0.006f, 0.0f, 0.0f}, LayerSymmetryPolicy::SYM_NONE, 1u, 0.04f, 0.62f
    });
    const uint32_t lm_mouth = plan.add_topology_landmark({
        0u, "MouthAperture", {head_id, 0.25f, 0.30f, 0.004f, 0.0f, 0.0f}, LayerSymmetryPolicy::SYM_NONE, 1u, 0.035f, 0.45f
    });
    const uint32_t lm_ear = plan.add_topology_landmark({
        0u, "TemporalEarAnchor", {head_id, 0.02f, 0.48f, 0.004f, 0.0f, 0.0f}, LayerSymmetryPolicy::SYM_BILATERAL, 1u, 0.04f, 0.25f
    });

    // Layer 2 — Topological Bridge Rules
    plan.add_bridge_rule({0u, "ArmpitBridge", thorax_id, uarm_l, "deltoid_bulk", 0.40f, 0.55f, 0.85f, 0.035f, 0.55f});
    plan.add_bridge_rule({0u, "HipCreaseBridge", pelvis_id, thigh_l, "hip_bulk", 0.35f, 0.52f, 0.82f, 0.030f, 0.45f});

    // Layer 2 — Default Separable Part Seam Contracts (Chest <-> Shoulders, Chest <-> Neck, Pelvis <-> Thighs)
    auto add_seam = [&](const char *p_name, uint32_t p_a, uint32_t p_b, uint32_t p_ring) {
        if (p_a == 0xFFFFFFFFu || p_b == 0xFFFFFFFFu) return;
        PartSeamLoopSpec<float> seam{};
        seam.seam_name = p_name;
        seam.part_a_id = p_a;
        seam.part_b_id = p_b;
        seam.canonical_ring_count = p_ring;
        plan.add_part_seam(seam);
    };
    add_seam("Seam_Chest_ArmL", thorax_id, uarm_l, 12u);
    add_seam("Seam_Chest_ArmR", thorax_id, uarm_r, 12u);
    add_seam("Seam_Chest_Neck", thorax_id, neck_id, 12u);
    add_seam("Seam_Pelvis_ThighL", pelvis_id, thigh_l, 12u);
    add_seam("Seam_Pelvis_ThighR", pelvis_id, thigh_r, 12u);

    // Layer 3 — Parametric Features
    plan.add_feature({0u, "Eye",   ParametricFeatureKind::FEATURE_EYE,   lm_orbit, LayerSymmetryPolicy::SYM_BILATERAL, glm::vec3(0.006f, 0.0f, 0.0f), glm::vec3(0.018f, 0.013f, 0.016f), 0.15f, 0.15f});
    plan.add_feature({0u, "Nose",  ParametricFeatureKind::FEATURE_NOSE,  lm_nose,  LayerSymmetryPolicy::SYM_NONE,      glm::vec3(0.005f, 0.0f, 0.0f), glm::vec3(0.016f, 0.028f, 0.022f), 0.20f, 0.55f});
    plan.add_feature({0u, "Mouth", ParametricFeatureKind::FEATURE_MOUTH, lm_mouth, LayerSymmetryPolicy::SYM_NONE,      glm::vec3(0.004f, 0.0f, 0.0f), glm::vec3(0.026f, 0.010f, 0.010f), 0.10f, 0.35f});
    plan.add_feature({0u, "Ear",   ParametricFeatureKind::FEATURE_EAR,   lm_ear,   LayerSymmetryPolicy::SYM_BILATERAL, glm::vec3(0.008f, 0.0f, 0.0f), glm::vec3(0.014f, 0.032f, 0.012f), 0.10f, 0.25f});

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
    detail_overrides_.clear();
    details_enabled_ = true;
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

void CharacterInstanceCore::set_scaffold_override(const ScaffoldDetailOverride &p_override) {
    for (auto &o : detail_overrides_.scaffold_overrides) {
        if (o.scaffold_node_id == p_override.scaffold_node_id) {
            o = p_override;
            return;
        }
    }
    detail_overrides_.scaffold_overrides.push_back(p_override);
}

void CharacterInstanceCore::set_topology_override(const TopologyDetailOverride &p_override) {
    for (auto &o : detail_overrides_.topology_overrides) {
        if (o.landmark_id == p_override.landmark_id && o.target_mirrored_copy == p_override.target_mirrored_copy) {
            o = p_override;
            return;
        }
    }
    detail_overrides_.topology_overrides.push_back(p_override);
}

void CharacterInstanceCore::set_feature_override(const FeatureDetailOverride &p_override) {
    for (auto &o : detail_overrides_.feature_overrides) {
        if (o.feature_id == p_override.feature_id && o.target_mirrored_copy == p_override.target_mirrored_copy) {
            o = p_override;
            return;
        }
    }
    detail_overrides_.feature_overrides.push_back(p_override);
}

uint32_t CharacterInstanceCore::add_detail_delta(const UnconstrainedDetailDelta &p_delta) {
    UnconstrainedDetailDelta copy = p_delta;
    copy.id = static_cast<uint32_t>(detail_overrides_.surface_sculpt_deltas.size());
    detail_overrides_.surface_sculpt_deltas.push_back(std::move(copy));
    return static_cast<uint32_t>(detail_overrides_.surface_sculpt_deltas.size() - 1u);
}

std::unordered_map<std::string, float> CharacterInstanceCore::_resolve_effective_parameters(const BodyPlanCore &p_plan) const {
    std::unordered_map<std::string, float> eff;
    for (const auto &b : p_plan.parameter_bounds()) {
        const float raw = get_parameter_value(b.name, p_plan);
        eff[b.name] = std::clamp(raw, b.min_value, b.max_value);
    }
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
    out.set_details_enabled(details_enabled_ || p_other.details_enabled_);
    for (const auto &b : p_plan.parameter_bounds()) {
        const float v0 = get_parameter_value(b.name, p_plan);
        const float v1 = p_other.get_parameter_value(b.name, p_plan);
        out.set_parameter_value(b.name, glm::mix(v0, v1, a));
    }
    for (const auto &d : detail_overrides_.surface_sculpt_deltas) {
        UnconstrainedDetailDelta scaled = d;
        scaled.local_delta_ntuv *= (1.0f - a);
        out.add_detail_delta(scaled);
    }
    for (const auto &d : p_other.detail_overrides_.surface_sculpt_deltas) {
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
    // LAYER 1 — Scaffold & Proportions (+ Optional Asymmetric Scaffold Overrides)
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

    if (details_enabled_) {
        for (const auto &so : detail_overrides_.scaffold_overrides) {
            if (auto *sn = out.evaluated_scaffold.get_node_mut(so.scaffold_node_id)) {
                sn->local_offset       += so.offset_delta;
                sn->local_rotation_deg += so.rotation_delta_deg;
                sn->half_extents       *= so.scale_multiplier;
                sn->ruler_mult_x       *= so.scale_multiplier.x;
                sn->ruler_mult_y       *= so.scale_multiplier.y;
                sn->ruler_mult_z       *= so.scale_multiplier.z;
            }
        }
    }

    out.evaluated_scaffold.evaluate();
    out.balance_report = out.evaluated_scaffold.compute_balance_report();
    out.skeleton_plan  = out.evaluated_scaffold.build_skeleton_export_plan();

    // Base quad mesh from Layer 1 Scaffold scaled by LOD level
    const uint32_t subdivs = resolve_surface_lod_divs(12u, p_lod_level);
    out.combined_quad_mesh = out.evaluated_scaffold.tessellate_scaffold_mesh(subdivs);
    out.combined_quad_mesh.vertex_crease_sharpness.assign(out.combined_quad_mesh.positions.size(), 0.0f);

    // ========================================================================
    // LAYER 2 — Parametric Topology Landmarks (SYM_BILATERAL + SYM_RADIAL)
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
            float disp = lm.anchor.normal_offset + extra_n;
            float eff_sharp = lm.planar_sharpness;
            if (details_enabled_) {
                for (const auto &to : detail_overrides_.topology_overrides) {
                    if (to.landmark_id == lm.id && to.target_mirrored_copy == p_mirrored) {
                        disp += to.normal_offset_delta;
                        if (to.crease_sharpness_override >= 0.0f) {
                            eff_sharp = std::clamp(to.crease_sharpness_override, 0.0f, 1.0f);
                        }
                    }
                }
            }

            SurfaceFrame3DPOD<float> f = out.evaluated_scaffold.sample_scaffold_frame(lm.anchor.scaffold_node_id, p_u, lm.anchor.v);
            f.position = f.offset_point(disp, lm.anchor.tangent_u_offset, lm.anchor.tangent_v_offset);
            resolved_landmarks[lm.id].push_back(f);
            out.landmark_frames.push_back({lm.id, lm.name, p_mirrored, f});

            const float r2 = lm.influence_radius * lm.influence_radius;
            if (r2 > 1e-6f && (std::abs(disp) > 1e-5f || eff_sharp > 0.45f)) {
                for (size_t vi = 0; vi < out.combined_quad_mesh.positions.size(); ++vi) {
                    if (out.combined_quad_mesh.vertex_patch_ids[vi] != lm.anchor.scaffold_node_id) continue;
                    const glm::vec3 d = out.combined_quad_mesh.positions[vi] - f.position;
                    const float dist2 = glm::dot(d, d);
                    if (dist2 < r2) {
                        const float falloff = 1.0f - std::sqrt(dist2) / lm.influence_radius;
                        const float shaped  = std::pow(falloff, glm::mix(2.0f, 0.7f, eff_sharp));
                        out.combined_quad_mesh.positions[vi] += f.normal * (disp * shaped);
                        out.combined_quad_mesh.vertex_crease_sharpness[vi] = std::max(
                            out.combined_quad_mesh.vertex_crease_sharpness[vi],
                            eff_sharp * falloff
                        );
                    }
                }
            }
        };

        eval_one(lm.anchor.u, false);
        if (lm.symmetry == LayerSymmetryPolicy::SYM_BILATERAL) {
            float mu = 0.5f - lm.anchor.u;
            if (mu < 0.0f) mu += 1.0f;
            if (std::abs(mu - lm.anchor.u) > 1e-3f) {
                eval_one(mu, true);
            }
        } else if (lm.symmetry == LayerSymmetryPolicy::SYM_RADIAL) {
            const uint8_t count = std::max<uint8_t>(2u, lm.radial_count);
            for (uint8_t k = 1u; k < count; ++k) {
                float ru = lm.anchor.u + static_cast<float>(k) / static_cast<float>(count);
                ru -= std::floor(ru);
                eval_one(ru, true);
            }
        }
    }

    // Evaluate & Deform Topological Bridge Rules (Pinch-and-Stretch geometry deformation!)
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
            st.active_regime         = BridgeTopologyRegime::BRIDGE_REGIME_DEEP_FOLD;
            st.active_fold_depth     = br.pinch_fold_depth;
            st.active_stretch_factor = br.stretch_smooth_bias;
        } else if (st.evaluated_metric >= br.crease_threshold) {
            st.active_regime = BridgeTopologyRegime::BRIDGE_REGIME_CREASE;
            const float t = (st.evaluated_metric - br.crease_threshold) /
                            std::max(1e-4f, br.deep_fold_threshold - br.crease_threshold);
            st.active_fold_depth     = br.pinch_fold_depth * (0.35f + 0.65f * t);
            st.active_stretch_factor = br.stretch_smooth_bias * t;
        } else {
            st.active_regime         = BridgeTopologyRegime::BRIDGE_REGIME_SMOOTH;
            st.active_fold_depth     = 0.0f;
            st.active_stretch_factor = 0.0f;
        }

        // Apply real mesh pinch/crease & opposite-side stretch deformation between node_a and node_b
        if (st.active_fold_depth > 1e-5f) {
            const auto *na = out.evaluated_scaffold.get_node(br.node_a_id);
            const auto *nb = out.evaluated_scaffold.get_node(br.node_b_id);
            if (na && nb) {
                const glm::vec3 pa = glm::vec3(na->world_transform[3]);
                const glm::vec3 pb = glm::vec3(nb->world_transform[3]);
                const glm::vec3 mid = (pa + pb) * 0.5f;
                const glm::vec3 axis = glm::normalize((pb - pa) + glm::vec3(1e-5f, 0.0f, 0.0f));
                const float radius = std::max(0.08f, glm::length(pb - pa) * 0.65f);
                const float r2     = radius * radius;
                const float crease_tag = (st.active_regime == BridgeTopologyRegime::BRIDGE_REGIME_DEEP_FOLD) ? 0.92f : 0.70f;

                for (size_t vi = 0; vi < out.combined_quad_mesh.positions.size(); ++vi) {
                    const uint32_t pid = out.combined_quad_mesh.vertex_patch_ids[vi];
                    if (pid != br.node_a_id && pid != br.node_b_id) continue;
                    const glm::vec3 rel = out.combined_quad_mesh.positions[vi] - mid;
                    const float d2 = glm::dot(rel, rel);
                    if (d2 < r2) {
                        const float w = 1.0f - std::sqrt(d2) / radius;
                        const float side = glm::dot(rel, glm::vec3(0.0f, -1.0f, 0.0f));
                        if (side >= 0.0f) {
                            // Inner pinch fold (e.g., armpit / groin crease)
                            out.combined_quad_mesh.positions[vi] -= out.combined_quad_mesh.normals[vi] * (st.active_fold_depth * w * w);
                            out.combined_quad_mesh.vertex_crease_sharpness[vi] = std::max(
                                out.combined_quad_mesh.vertex_crease_sharpness[vi],
                                crease_tag * w
                            );
                        } else {
                            // Opposite side stretches smoothly (Loomis pinch-and-stretch law)
                            out.combined_quad_mesh.positions[vi] += axis * (st.active_stretch_factor * 0.015f * w);
                        }
                    }
                }
            }
        }

        out.bridge_states.push_back(std::move(st));
    }

    // ========================================================================
    // LAYER 3 — Parametric Features (+ Optional Unilateral Feature Overrides)
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

        for (size_t copy_idx = 0; copy_idx < it->second.size(); ++copy_idx) {
            const bool is_mirrored = (copy_idx > 0u);
            ParametricFeatureSpec scaled = feat;
            scaled.base_half_extents *= scale_mult;

            if (details_enabled_) {
                for (const auto &fo : detail_overrides_.feature_overrides) {
                    if (fo.feature_id == feat.id && fo.target_mirrored_copy == is_mirrored) {
                        scaled.local_offset_ntuv += fo.offset_delta_ntuv;
                        scaled.base_half_extents *= fo.scale_multiplier;
                        scaled.secondary_param   += fo.secondary_param_delta;
                    }
                }
            }

            out.combined_quad_mesh.append(build_feature_subpatch(scaled, it->second[copy_idx], next_patch_id++, p_lod_level));
            if (feat.symmetry == LayerSymmetryPolicy::SYM_NONE) {
                break;
            }
        }
    }

    // ========================================================================
    // LAYER 4 — Unconstrained Surface Sculpt Deltas (Non-Destructive + Patch-Filtered)
    // ========================================================================
    if (details_enabled_) {
        for (const auto &delta : detail_overrides_.surface_sculpt_deltas) {
            const auto anchor_frame = out.evaluated_scaffold.sample_scaffold_frame(delta.scaffold_node_id, delta.u, delta.v);
            const glm::vec3 world_disp =
                anchor_frame.normal    * delta.local_delta_ntuv.x +
                anchor_frame.tangent_u * delta.local_delta_ntuv.y +
                anchor_frame.tangent_v * delta.local_delta_ntuv.z;

            const float r = std::max(0.005f, delta.radius);
            const float r2 = r * r;
            for (size_t vi = 0; vi < out.combined_quad_mesh.positions.size(); ++vi) {
                // Strictly filter by scaffold_node_id so deltas never bleed onto adjacent limbs/parts
                if (vi < out.combined_quad_mesh.vertex_patch_ids.size() &&
                    out.combined_quad_mesh.vertex_patch_ids[vi] != delta.scaffold_node_id) {
                    continue;
                }
                const glm::vec3 d = out.combined_quad_mesh.positions[vi] - anchor_frame.position;
                const float dist2 = glm::dot(d, d);
                if (dist2 < r2) {
                    const float w = 1.0f - std::sqrt(dist2) / r;
                    const float shaped = delta.is_crease ? (w * w * w) : (w * w * (3.0f - 2.0f * w));
                    out.combined_quad_mesh.positions[vi] += world_disp * shaped;
                    if (delta.is_crease && vi < out.combined_quad_mesh.vertex_crease_sharpness.size()) {
                        out.combined_quad_mesh.vertex_crease_sharpness[vi] = std::max(
                            out.combined_quad_mesh.vertex_crease_sharpness[vi],
                            0.85f * w
                        );
                    }
                }
            }
        }
    }

    // Crease-aware split normals preserving bony landmarks & planar breaks
    if (!out.combined_quad_mesh.quads.empty()) {
        out.combined_quad_mesh.recompute_normals_with_creases(42.0f);
    }

    // Extract separable per-part meshes and synchronize cross-part seam boundary normals
    for (size_t qi = 0; qi < out.combined_quad_mesh.quads.size(); ++qi) {
        const uint32_t pid = (qi < out.combined_quad_mesh.quad_patch_ids.size())
            ? out.combined_quad_mesh.quad_patch_ids[qi]
            : 0u;
        auto &pm = out.part_meshes[pid];
        const auto &q = out.combined_quad_mesh.quads[qi];
        const uint32_t base = static_cast<uint32_t>(pm.positions.size());
        for (int c = 0; c < 4; ++c) {
            const uint32_t vi = q[c];
            pm.positions.push_back(out.combined_quad_mesh.positions[vi]);
            pm.normals.push_back(out.combined_quad_mesh.normals[vi]);
            if (vi < out.combined_quad_mesh.tangents.size()) pm.tangents.push_back(out.combined_quad_mesh.tangents[vi]);
            if (vi < out.combined_quad_mesh.uvs.size()) pm.uvs.push_back(out.combined_quad_mesh.uvs[vi]);
            pm.vertex_patch_ids.push_back(pid);
        }
        pm.quads.emplace_back(base, base + 1u, base + 2u, base + 3u);
        pm.quad_patch_ids.push_back(pid);
    }

    for (const auto &seam : p_plan.part_seams()) {
        auto it_a = out.part_meshes.find(seam.part_a_id);
        auto it_b = out.part_meshes.find(seam.part_b_id);
        if (it_a != out.part_meshes.end() && it_b != out.part_meshes.end()) {
            PartSeamContract<float>::synchronize_seam_boundary_normals(it_a->second, it_b->second, 0.015f);
        }
    }

    return out;
}

} // namespace ideam::core
