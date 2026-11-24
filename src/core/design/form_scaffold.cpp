#include "form_scaffold.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

namespace ideam::core {

namespace {
constexpr float kPi      = 3.14159265358979323846f;
constexpr float kTwoPi   = 6.28318530717958647692f;
constexpr float kDeg2Rad = kPi / 180.0f;

glm::mat4 compose_trs(const glm::vec3 &p_pos, const glm::vec3 &p_euler_deg) {
    glm::mat4 m(1.0f);
    m = glm::translate(m, p_pos);
    m = glm::rotate(m, p_euler_deg.y * kDeg2Rad, glm::vec3(0.0f, 1.0f, 0.0f));
    m = glm::rotate(m, p_euler_deg.x * kDeg2Rad, glm::vec3(1.0f, 0.0f, 0.0f));
    m = glm::rotate(m, p_euler_deg.z * kDeg2Rad, glm::vec3(0.0f, 0.0f, 1.0f));
    return m;
}

glm::mat4 make_bishop_frame_matrix(const glm::vec3 &p_pos, const glm::vec3 &p_tangent) {
    const auto frame = ParallelTransport3D<float>::make_initial_frame(p_pos, p_tangent);
    // Align local +Y with curve tangent so upright spine/torso volumes flow smoothly along the gesture line
    const glm::vec3 up      = glm::length(frame.tangent) > 1e-5f ? glm::normalize(frame.tangent) : glm::vec3(0.0f, 1.0f, 0.0f);
    const glm::vec3 right   = glm::length(frame.binormal) > 1e-5f ? glm::normalize(frame.binormal) : glm::vec3(1.0f, 0.0f, 0.0f);
    const glm::vec3 forward = glm::normalize(glm::cross(right, up));
    const glm::vec3 ortho_r = glm::normalize(glm::cross(up, forward));

    glm::mat4 m(1.0f);
    m[0] = glm::vec4(ortho_r, 0.0f);
    m[1] = glm::vec4(up, 0.0f);
    m[2] = glm::vec4(forward, 0.0f);
    m[3] = glm::vec4(p_pos, 1.0f);
    return m;
}

} // namespace

void FormScaffoldCore::clear() {
    nodes_.clear();
    ruler_graph_.clear();
    line_of_action_.clear();
    use_line_of_action_ = false;
    contrapposto_amount_ = 0.0f;
}

void FormScaffoldCore::set_contrapposto_amount(float p_amount) noexcept {
    contrapposto_amount_ = std::clamp(p_amount, -1.0f, 1.0f);
}

uint32_t FormScaffoldCore::add_node(const ScaffoldNodeEntry &p_entry) {
    ScaffoldNodeEntry copy = p_entry;
    copy.node_id = static_cast<uint32_t>(nodes_.size());
    copy.socket.socket_id = copy.node_id;
    copy.socket.owner_node_id = copy.node_id;
    nodes_.push_back(std::move(copy));
    return static_cast<uint32_t>(nodes_.size() - 1u);
}

const ScaffoldNodeEntry *FormScaffoldCore::get_node(uint32_t p_id) const noexcept {
    if (p_id < nodes_.size()) return &nodes_[p_id];
    return nullptr;
}

ScaffoldNodeEntry *FormScaffoldCore::get_node_mut(uint32_t p_id) noexcept {
    if (p_id < nodes_.size()) return &nodes_[p_id];
    return nullptr;
}

uint32_t FormScaffoldCore::find_node_id(std::string_view p_name) const noexcept {
    for (const auto &n : nodes_) {
        if (n.name == p_name) return n.node_id;
    }
    return 0xFFFFFFFFu;
}

void FormScaffoldCore::evaluate() {
    ruler_graph_.evaluate();

    const bool has_valid_loa = use_line_of_action_ && (line_of_action_.get_point_count() >= 2u);

    for (auto &n : nodes_) {
        // 1. Live ProportionRulerGraph scale evaluation
        glm::vec3 ext = n.half_extents;
        if (n.ruler_scale_x_id != 0xFFFFFFFFu) {
            ext.x = std::max(0.005f, ruler_graph_.get_value(n.ruler_scale_x_id, ext.x) * n.ruler_mult_x);
        }
        if (n.ruler_scale_y_id != 0xFFFFFFFFu) {
            ext.y = std::max(0.005f, ruler_graph_.get_value(n.ruler_scale_y_id, ext.y) * n.ruler_mult_y);
        }
        if (n.ruler_scale_z_id != 0xFFFFFFFFu) {
            ext.z = std::max(0.005f, ruler_graph_.get_value(n.ruler_scale_z_id, ext.z) * n.ruler_mult_z);
        }
        n.evaluated_half_extents = ext;

        // 2. Live ruler-proportional offset scaling
        glm::vec3 eff_offset = n.local_offset;
        if (n.ruler_offset_scale_id != 0xFFFFFFFFu && n.ruler_offset_ref_value > 1e-4f) {
            const float cur_val = ruler_graph_.get_value(n.ruler_offset_scale_id, n.ruler_offset_ref_value);
            const float ratio   = std::max(0.05f, cur_val / n.ruler_offset_ref_value);
            eff_offset *= ratio;
        }

        // 3. Apply Loomis Contrapposto opposing shoulder/pelvis counter-tilt and axial twist
        glm::vec3 rot = n.local_rotation_deg;
        rot.z += contrapposto_amount_ * 12.0f * n.contrapposto_tilt_weight;
        rot.y += contrapposto_amount_ * 10.0f * n.contrapposto_twist_weight;

        // 4. Line of Action gesture-curve override via Bishop frames (for spine/torso/pelvis/neck/head)
        if (has_valid_loa && n.follows_line_of_action) {
            const float t = std::clamp(n.line_of_action_t, 0.0f, 1.0f);
            const glm::vec3 loa_pos  = line_of_action_.get_point(t, false);
            const glm::vec3 loa_tang = line_of_action_.get_tangent(t, false);
            const glm::mat4 bishop   = make_bishop_frame_matrix(loa_pos, loa_tang);
            const glm::mat4 rot_mat  = compose_trs(glm::vec3(0.0f), rot);
            n.world_transform = bishop * rot_mat;
            if (n.parent_id != 0xFFFFFFFFu && n.parent_id < nodes_.size()) {
                n.local_transform = glm::inverse(nodes_[n.parent_id].world_transform) * n.world_transform;
            } else {
                n.local_transform = n.world_transform;
            }
        } else {
            n.local_transform = compose_trs(eff_offset, rot);
            if (n.parent_id != 0xFFFFFFFFu && n.parent_id < nodes_.size()) {
                n.world_transform = nodes_[n.parent_id].world_transform * n.local_transform;
            } else {
                n.world_transform = n.local_transform;
            }
        }

        // 5. Sync embedded StructureNodePod & SocketPod
        n.structure_pod.node_id          = n.node_id;
        n.structure_pod.parent_id        = n.parent_id;
        n.structure_pod.semantic_tag_id  = n.semantic_role_id;
        n.structure_pod.rest_obb_center  = eff_offset;
        n.structure_pod.rest_obb_extents = n.evaluated_half_extents;
        n.structure_pod.fitted_transform = n.world_transform;
        n.structure_pod.size_flags_x     = DESIGN_SIZE_PROPORTIONAL_RATIO;
        n.structure_pod.size_flags_y     = DESIGN_SIZE_PROPORTIONAL_RATIO;
        n.structure_pod.size_flags_z     = DESIGN_SIZE_PROPORTIONAL_RATIO;

        n.socket.position = glm::vec3(n.world_transform[3]);
        n.socket.clearance_half_extents = n.evaluated_half_extents;
    }
}

ScaffoldBalanceReport FormScaffoldCore::compute_balance_report() const {
    ScaffoldBalanceReport rep{};
    float total_w = 0.0f;
    glm::vec3 weighted_pos(0.0f);

    glm::vec3 support_sum(0.0f);
    uint32_t support_count = 0u;
    std::vector<glm::vec3> support_pts;

    for (const auto &n : nodes_) {
        const glm::vec3 wp = glm::vec3(n.world_transform[3]);
        const float w = std::max(0.01f, n.mass_weight);
        weighted_pos += wp * w;
        total_w += w;
        if (n.is_ground_support) {
            support_sum += wp;
            support_count += 1u;
            support_pts.push_back(wp);
        }
    }

    if (total_w > 0.0f) {
        rep.center_of_gravity = weighted_pos / total_w;
    }
    rep.plumb_ground_point = glm::vec3(rep.center_of_gravity.x, 0.0f, rep.center_of_gravity.z);

    if (support_count > 0u) {
        rep.support_center = support_sum / static_cast<float>(support_count);
        rep.support_center.y = 0.0f;
        float max_r = 0.12f;
        for (const auto &sp : support_pts) {
            const float d = glm::length(glm::vec2(sp.x - rep.support_center.x, sp.z - rep.support_center.z));
            max_r = std::max(max_r, d + 0.08f);
        }
        rep.support_radius = max_r;
        rep.horizontal_offset = glm::length(glm::vec2(
            rep.plumb_ground_point.x - rep.support_center.x,
            rep.plumb_ground_point.z - rep.support_center.z
        ));
        rep.is_balanced = rep.horizontal_offset <= rep.support_radius;
    } else {
        rep.is_balanced = true;
    }
    return rep;
}

glm::vec3 FormScaffoldCore::_eval_local_primitive_surface(
    const ScaffoldNodeEntry &p_node,
    float p_u,
    float p_v
) const {
    const float u = std::clamp(p_u, 0.0f, 1.0f);
    const float v = std::clamp(p_v, 0.0f, 1.0f);
    const glm::vec3 &ext = p_node.evaluated_half_extents;

    const float theta = u * kTwoPi;
    const float phi   = (v - 0.5f) * kPi;
    const float ct = std::cos(theta);
    const float st = std::sin(theta);
    const float cp = std::cos(phi);
    const float sp = std::sin(phi);

    switch (p_node.type) {
        case ScaffoldPrimitiveType::PRIMITIVE_SPHERE: {
            return glm::vec3(cp * ct * ext.x, sp * ext.y, cp * st * ext.z);
        }

        case ScaffoldPrimitiveType::PRIMITIVE_SLICED_SPHERE: {
            float x = cp * ct * ext.x;
            float y = sp * ext.y;
            float z = cp * st * ext.z;

            const float shave_x = std::clamp(p_node.side_shave_ratio, 0.35f, 1.0f) * ext.x;
            x = std::clamp(x, -shave_x, shave_x);

            if (y < 0.0f && z > 0.0f) {
                const float front_factor = std::clamp(z / std::max(1e-4f, ext.z), 0.0f, 1.0f);
                const float lower_factor = std::clamp(-y / std::max(1e-4f, ext.y), 0.0f, 1.0f);
                y -= p_node.jaw_drop_ratio * ext.y * front_factor * lower_factor;
                x *= glm::mix(1.0f, std::clamp(p_node.jaw_width_ratio, 0.25f, 1.0f), lower_factor * front_factor);
                z += p_node.muzzle_length_ratio * ext.z * front_factor * (1.0f - 0.35f * lower_factor);
            }
            return glm::vec3(x, y, z);
        }

        case ScaffoldPrimitiveType::PRIMITIVE_EGG_ELLIPSOID: {
            const float taper = 1.0f + std::clamp(p_node.egg_taper, -0.55f, 0.55f) * sp;
            return glm::vec3(cp * ct * ext.x * taper, sp * ext.y, cp * st * ext.z * taper);
        }

        case ScaffoldPrimitiveType::PRIMITIVE_TAPERED_CAPSULE: {
            const float y = (v * 2.0f - 1.0f) * ext.y;
            const float cap_sin = std::sin(std::clamp(v, 0.001f, 0.999f) * kPi);
            const float cap_scale = std::pow(cap_sin, 0.32f);
            const float tx = glm::mix(1.0f, std::clamp(p_node.top_taper_x, 0.2f, 2.0f), v);
            const float tz = glm::mix(1.0f, std::clamp(p_node.top_taper_z, 0.2f, 2.0f), v);

            const float d_hi = (v - p_node.asymmetry_high_u) / 0.18f;
            const float d_lo = (v - p_node.asymmetry_low_u)  / 0.18f;
            const float bulge_hi = p_node.asymmetry_high_amp * std::exp(-0.5f * d_hi * d_hi) * std::max(0.0f, ct);
            const float bulge_lo = p_node.asymmetry_low_amp  * std::exp(-0.5f * d_lo * d_lo) * std::max(0.0f, -ct);
            const float rhythm = 1.0f + bulge_hi + bulge_lo;

            return glm::vec3(ct * ext.x * tx * cap_scale * rhythm, y, st * ext.z * tz * cap_scale * rhythm);
        }

        case ScaffoldPrimitiveType::PRIMITIVE_WEDGE_BOX: {
            const float sgn_c = (ct >= 0.0f) ? 1.0f : -1.0f;
            const float sgn_s = (st >= 0.0f) ? 1.0f : -1.0f;
            const float bx = sgn_c * std::pow(std::abs(ct), 0.35f);
            const float bz = sgn_s * std::pow(std::abs(st), 0.35f);
            const float y  = (v * 2.0f - 1.0f) * ext.y;
            const float cap = std::pow(std::sin(std::clamp(v, 0.001f, 0.999f) * kPi), 0.18f);
            const float tx = glm::mix(1.0f, std::clamp(p_node.top_taper_x, 0.25f, 2.0f), v);
            const float tz = glm::mix(1.0f, std::clamp(p_node.top_taper_z, 0.25f, 2.0f), v);
            return glm::vec3(bx * ext.x * tx * cap, y, bz * ext.z * tz * cap);
        }

        case ScaffoldPrimitiveType::PRIMITIVE_DISC: {
            const float r = std::sin(std::clamp(v, 0.001f, 0.999f) * kPi);
            return glm::vec3(ct * ext.x * r, sp * ext.y * 0.35f, st * ext.z * r);
        }

        case ScaffoldPrimitiveType::PRIMITIVE_GESTURE_SPINE: {
            if (p_node.gesture_spline.get_point_count() >= 2u) {
                const glm::vec3 center = p_node.gesture_spline.get_point(v, false);
                const glm::vec3 tang   = p_node.gesture_spline.get_tangent(v, false);
                const auto frame = ParallelTransport3D<float>::make_initial_frame(center, tang);
                const float taper = glm::mix(1.0f, std::clamp(p_node.top_taper_x, 0.15f, 1.5f), v);
                return center + frame.normal * (ct * ext.x * taper) + frame.binormal * (st * ext.z * taper);
            }
            return glm::vec3(ct * ext.x, (v * 2.0f - 1.0f) * ext.y, st * ext.z);
        }
    }
    return glm::vec3(0.0f);
}

SurfaceFrame3DPOD<float> FormScaffoldCore::sample_scaffold_frame(
    uint32_t p_node_id,
    float p_u,
    float p_v
) const {
    const auto *node = get_node(p_node_id);
    if (!node) return SurfaceFrame3DPOD<float>{};

    const float u = std::clamp(p_u, 0.0f, 1.0f);
    const float v = std::clamp(p_v, 0.0f, 1.0f);
    constexpr float kH = 1e-3f;

    auto to_world = [&](const glm::vec3 &p_local) -> glm::vec3 {
        return glm::vec3(node->world_transform * glm::vec4(p_local, 1.0f));
    };

    const glm::vec3 pos = to_world(_eval_local_primitive_surface(*node, u, v));
    const float u0 = std::max(0.0f, u - kH);
    const float u1 = std::min(1.0f, u + kH);
    const float v0 = std::max(0.0f, v - kH);
    const float v1 = std::min(1.0f, v + kH);

    const glm::vec3 du = (u1 > u0)
        ? (to_world(_eval_local_primitive_surface(*node, u1, v)) - to_world(_eval_local_primitive_surface(*node, u0, v))) / (u1 - u0)
        : glm::vec3(1.0f, 0.0f, 0.0f);
    const glm::vec3 dv = (v1 > v0)
        ? (to_world(_eval_local_primitive_surface(*node, u, v1)) - to_world(_eval_local_primitive_surface(*node, u, v0))) / (v1 - v0)
        : glm::vec3(0.0f, 1.0f, 0.0f);

    return make_surface_frame_from_partials(pos, du, dv, u, v, p_node_id);
}

SurfaceQuadMesh3D<float> FormScaffoldCore::tessellate_scaffold_mesh(uint32_t p_subdivs) const {
    SurfaceQuadMesh3D<float> combined;
    const uint32_t u_divs = std::clamp<uint32_t>(p_subdivs, 4u, 32u);
    const uint32_t v_divs = std::clamp<uint32_t>(p_subdivs, 4u, 32u);

    for (const auto &node : nodes_) {
        SurfaceQuadMesh3D<float> part;
        const uint32_t cols = u_divs + 1u;
        const uint32_t rows = v_divs + 1u;
        part.positions.reserve(cols * rows);
        part.normals.reserve(cols * rows);
        part.tangents.reserve(cols * rows);
        part.uvs.reserve(cols * rows);
        part.vertex_patch_ids.reserve(cols * rows);
        part.vertex_ring_indices.reserve(cols * rows);
        part.vertex_min_lod_levels.reserve(cols * rows);

        for (uint32_t j = 0u; j < rows; ++j) {
            const float v = static_cast<float>(j) / static_cast<float>(v_divs);
            for (uint32_t i = 0u; i < cols; ++i) {
                const float u = static_cast<float>(i) / static_cast<float>(u_divs);
                const auto f = sample_scaffold_frame(node.node_id, u, v);
                part.positions.push_back(f.position);
                part.normals.push_back(f.normal);
                part.tangents.emplace_back(f.tangent_u.x, f.tangent_u.y, f.tangent_u.z, 1.0f);
                part.uvs.emplace_back(u, v);
                part.vertex_patch_ids.push_back(node.node_id);
                part.vertex_ring_indices.push_back(j);
                part.vertex_min_lod_levels.push_back(0u);
            }
        }

        part.quads.reserve(u_divs * v_divs);
        part.quad_patch_ids.reserve(u_divs * v_divs);
        for (uint32_t j = 0u; j < v_divs; ++j) {
            for (uint32_t i = 0u; i < u_divs; ++i) {
                const uint32_t v00 = j * cols + i;
                const uint32_t v10 = j * cols + (i + 1u);
                const uint32_t v11 = (j + 1u) * cols + (i + 1u);
                const uint32_t v01 = (j + 1u) * cols + i;
                part.quads.emplace_back(v00, v10, v11, v01);
                part.quad_patch_ids.push_back(node.node_id);
            }
        }
        combined.append(part);
    }

    return combined;
}

std::vector<ScaffoldBoneExportEntry> FormScaffoldCore::build_skeleton_export_plan() const {
    std::vector<ScaffoldBoneExportEntry> bones;
    bones.reserve(nodes_.size());

    float total_w = 0.0f;
    for (const auto &n : nodes_) total_w += std::max(0.01f, n.mass_weight);
    const float kg_per_w = (total_w > 0.0f) ? (total_mass_kg_ / total_w) : 1.0f;

    for (const auto &n : nodes_) {
        ScaffoldBoneExportEntry b{};
        b.node_id               = n.node_id;
        b.parent_bone_index     = (n.parent_id != 0xFFFFFFFFu && n.parent_id < nodes_.size())
            ? static_cast<int32_t>(n.parent_id)
            : -1;
        b.bone_name             = n.name.empty() ? ("Bone_" + std::to_string(n.node_id)) : n.name;
        b.rest_local_transform  = n.local_transform;
        b.rest_world_transform  = n.world_transform;
        b.collider_half_extents = n.evaluated_half_extents;
        b.mass_kg               = std::max(0.1f, n.mass_weight * kg_per_w);
        b.physics_binding_mode  = n.socket.physics_binding_mode;
        b.joint_limit_lower_deg = n.socket.joint_limit_lower;
        b.joint_limit_upper_deg = n.socket.joint_limit_upper;
        b.joint_axis_index      = n.socket.joint_axis_index;
        bones.push_back(std::move(b));
    }
    return bones;
}

// ============================================================================
// Universal Andrew Loomis Body-Plan Scaffold Presets (Live Ruler-Bound)
// ============================================================================

FormScaffoldCore FormScaffoldCore::create_loomis_biped_scaffold(
    LoomisBipedCanon p_canon,
    float p_head_height_m
) {
    FormScaffoldCore sc;
    sc.set_ruler_graph(ProportionRulerGraph::create_loomis_biped_canon(p_canon, p_head_height_m));
    sc.set_scaffold_name(sc.get_ruler_graph().get_canon_name());
    const auto &rg = sc.get_ruler_graph();

    const uint32_t r_head_h   = rg.find_node_id("head_height");
    const uint32_t r_head_w   = rg.find_node_id("head_width");
    const uint32_t r_head_d   = rg.find_node_id("head_depth");
    const uint32_t r_neck_h   = rg.find_node_id("neck_height");
    const uint32_t r_sh_w     = rg.find_node_id("shoulder_width");
    const uint32_t r_rib_h    = rg.find_node_id("ribcage_height");
    const uint32_t r_rib_d    = rg.find_node_id("ribcage_depth");
    const uint32_t r_pel_w    = rg.find_node_id("pelvis_width");
    const uint32_t r_pel_h    = rg.find_node_id("pelvis_height");
    const uint32_t r_pel_d    = rg.find_node_id("pelvis_depth");
    const uint32_t r_ua_len   = rg.find_node_id("upper_arm_len");
    const uint32_t r_fa_len   = rg.find_node_id("forearm_len");
    const uint32_t r_hand_len = rg.find_node_id("hand_len");
    const uint32_t r_th_len   = rg.find_node_id("thigh_len");
    const uint32_t r_sh_len   = rg.find_node_id("shin_len");
    const uint32_t r_ft_len   = rg.find_node_id("foot_len");

    const float pelvis_y = rg.get_value_by_name("landmark_pelvis", 0.85f);
    const float sh_w     = rg.get_value_by_name("shoulder_width", 0.46f);
    const float hip_w    = rg.get_value_by_name("pelvis_width", 0.33f);
    const float rib_h    = rg.get_value_by_name("ribcage_height", 0.35f);
    const float pel_h    = rg.get_value_by_name("pelvis_height", 0.22f);
    const float neck_h   = rg.get_value_by_name("neck_height", 0.08f);
    const float head_h   = rg.get_value_by_name("head_height", p_head_height_m);
    const float ua_len   = rg.get_value_by_name("upper_arm_len", 0.31f);
    const float fa_len   = rg.get_value_by_name("forearm_len", 0.26f);
    const float hand_len = rg.get_value_by_name("hand_len", 0.17f);
    const float th_len   = rg.get_value_by_name("thigh_len", 0.44f);
    const float sh_len   = rg.get_value_by_name("shin_len", 0.38f);
    const float ft_len   = rg.get_value_by_name("foot_len", 0.24f);

    // 0. Pelvis Wedge (Root)
    ScaffoldNodeEntry pelvis{};
    pelvis.name = "Pelvis";
    pelvis.parent_id = 0xFFFFFFFFu;
    pelvis.type = ScaffoldPrimitiveType::PRIMITIVE_WEDGE_BOX;
    pelvis.local_offset = glm::vec3(0.0f, pelvis_y, 0.0f);
    pelvis.half_extents = glm::vec3(hip_w * 0.5f, pel_h * 0.5f, rg.get_value_by_name("pelvis_depth", 0.21f) * 0.5f);
    pelvis.ruler_scale_x_id = r_pel_w; pelvis.ruler_mult_x = 0.5f;
    pelvis.ruler_scale_y_id = r_pel_h; pelvis.ruler_mult_y = 0.5f;
    pelvis.ruler_scale_z_id = r_pel_d; pelvis.ruler_mult_z = 0.5f;
    pelvis.ruler_offset_scale_id = r_head_h; pelvis.ruler_offset_ref_value = head_h;
    pelvis.follows_line_of_action = true; pelvis.line_of_action_t = 0.0f;
    pelvis.top_taper_x = 0.88f;
    pelvis.mass_weight = 14.0f;
    pelvis.contrapposto_tilt_weight = -1.0f;
    pelvis.contrapposto_twist_weight = -0.6f;
    pelvis.socket.physics_binding_mode = SocketPhysicsBindingMode::PHYSICS_BIND_GENERIC_6DOF_JOINT_3D;
    const uint32_t id_pelvis = sc.add_node(pelvis);

    // 1. Ribcage Egg (Thorax)
    ScaffoldNodeEntry thorax{};
    thorax.name = "Thorax";
    thorax.parent_id = id_pelvis;
    thorax.type = ScaffoldPrimitiveType::PRIMITIVE_EGG_ELLIPSOID;
    thorax.local_offset = glm::vec3(0.0f, pel_h * 0.5f + rib_h * 0.62f, 0.0f);
    thorax.half_extents = glm::vec3(sh_w * 0.42f, rib_h * 0.55f, rg.get_value_by_name("ribcage_depth", 0.24f) * 0.5f);
    thorax.ruler_scale_x_id = r_sh_w;  thorax.ruler_mult_x = 0.42f;
    thorax.ruler_scale_y_id = r_rib_h; thorax.ruler_mult_y = 0.55f;
    thorax.ruler_scale_z_id = r_rib_d; thorax.ruler_mult_z = 0.50f;
    thorax.ruler_offset_scale_id = r_head_h; thorax.ruler_offset_ref_value = head_h;
    thorax.follows_line_of_action = true; thorax.line_of_action_t = 0.48f;
    thorax.egg_taper = 0.28f;
    thorax.mass_weight = 22.0f;
    thorax.contrapposto_tilt_weight = 1.0f;
    thorax.contrapposto_twist_weight = 1.0f;
    thorax.socket.physics_binding_mode = SocketPhysicsBindingMode::PHYSICS_BIND_GENERIC_6DOF_JOINT_3D;
    thorax.socket.joint_limit_lower = -25.0f;
    thorax.socket.joint_limit_upper = 25.0f;
    const uint32_t id_thorax = sc.add_node(thorax);

    // 2. Neck Capsule
    ScaffoldNodeEntry neck{};
    neck.name = "Neck";
    neck.parent_id = id_thorax;
    neck.type = ScaffoldPrimitiveType::PRIMITIVE_TAPERED_CAPSULE;
    neck.local_offset = glm::vec3(0.0f, rib_h * 0.55f + neck_h * 0.5f, 0.0f);
    neck.half_extents = glm::vec3(head_h * 0.24f, neck_h * 0.55f, head_h * 0.24f);
    neck.ruler_scale_x_id = r_head_h; neck.ruler_mult_x = 0.24f;
    neck.ruler_scale_y_id = r_neck_h; neck.ruler_mult_y = 0.55f;
    neck.ruler_scale_z_id = r_head_h; neck.ruler_mult_z = 0.24f;
    neck.ruler_offset_scale_id = r_head_h; neck.ruler_offset_ref_value = head_h;
    neck.follows_line_of_action = true; neck.line_of_action_t = 0.78f;
    neck.mass_weight = 2.0f;
    neck.socket.physics_binding_mode = SocketPhysicsBindingMode::PHYSICS_BIND_GENERIC_6DOF_JOINT_3D;
    neck.socket.joint_limit_lower = -35.0f;
    neck.socket.joint_limit_upper = 35.0f;
    const uint32_t id_neck = sc.add_node(neck);

    // 3. Loomis Sliced-Sphere Cranium + Jaw Wedge
    ScaffoldNodeEntry head{};
    head.name = "Head";
    head.parent_id = id_neck;
    head.type = ScaffoldPrimitiveType::PRIMITIVE_SLICED_SPHERE;
    head.local_offset = glm::vec3(0.0f, neck_h * 0.5f + head_h * 0.48f, 0.01f);
    head.half_extents = glm::vec3(rg.get_value_by_name("head_width", 0.16f) * 0.5f, head_h * 0.42f, rg.get_value_by_name("head_depth", 0.19f) * 0.5f);
    head.ruler_scale_x_id = r_head_w; head.ruler_mult_x = 0.50f;
    head.ruler_scale_y_id = r_head_h; head.ruler_mult_y = 0.42f;
    head.ruler_scale_z_id = r_head_d; head.ruler_mult_z = 0.50f;
    head.ruler_offset_scale_id = r_head_h; head.ruler_offset_ref_value = head_h;
    head.follows_line_of_action = true; head.line_of_action_t = 1.0f;
    head.side_shave_ratio = 0.72f;
    head.jaw_drop_ratio = 0.48f;
    head.jaw_width_ratio = 0.60f;
    head.mass_weight = 5.0f;
    head.socket.physics_binding_mode = SocketPhysicsBindingMode::PHYSICS_BIND_PIN_JOINT_3D;
    head.socket.joint_limit_lower = -45.0f;
    head.socket.joint_limit_upper = 45.0f;
    sc.add_node(head);

    // Bilateral Arms & Legs with Loomis anti-sausage asymmetric rhythm & live ruler bindings
    for (int side = -1; side <= 1; side += 2) {
        const std::string sfx = (side < 0) ? "_L" : "_R";
        const float sx = static_cast<float>(side);

        ScaffoldNodeEntry uarm{};
        uarm.name = "UpperArm" + sfx;
        uarm.parent_id = id_thorax;
        uarm.type = ScaffoldPrimitiveType::PRIMITIVE_TAPERED_CAPSULE;
        uarm.local_offset = glm::vec3(sx * sh_w * 0.48f, rib_h * 0.35f - ua_len * 0.45f, 0.0f);
        uarm.half_extents = glm::vec3(head_h * 0.21f, ua_len * 0.5f, head_h * 0.21f);
        uarm.ruler_scale_x_id = r_head_h; uarm.ruler_mult_x = 0.21f;
        uarm.ruler_scale_y_id = r_ua_len; uarm.ruler_mult_y = 0.50f;
        uarm.ruler_scale_z_id = r_head_h; uarm.ruler_mult_z = 0.21f;
        uarm.ruler_offset_scale_id = r_head_h; uarm.ruler_offset_ref_value = head_h;
        uarm.top_taper_x = 1.15f;
        uarm.asymmetry_high_u = 0.75f; uarm.asymmetry_high_amp = 0.16f;
        uarm.asymmetry_low_u  = 0.45f; uarm.asymmetry_low_amp  = 0.10f;
        uarm.mass_weight = 2.5f;
        uarm.socket.physics_binding_mode = SocketPhysicsBindingMode::PHYSICS_BIND_PIN_JOINT_3D;
        uarm.socket.joint_limit_lower = -120.0f;
        uarm.socket.joint_limit_upper = 120.0f;
        const uint32_t id_uarm = sc.add_node(uarm);

        ScaffoldNodeEntry farm{};
        farm.name = "Forearm" + sfx;
        farm.parent_id = id_uarm;
        farm.type = ScaffoldPrimitiveType::PRIMITIVE_TAPERED_CAPSULE;
        farm.local_offset = glm::vec3(0.0f, -(ua_len * 0.5f + fa_len * 0.48f), 0.0f);
        farm.half_extents = glm::vec3(head_h * 0.16f, fa_len * 0.5f, head_h * 0.16f);
        farm.ruler_scale_x_id = r_head_h; farm.ruler_mult_x = 0.16f;
        farm.ruler_scale_y_id = r_fa_len; farm.ruler_mult_y = 0.50f;
        farm.ruler_scale_z_id = r_head_h; farm.ruler_mult_z = 0.16f;
        farm.ruler_offset_scale_id = r_head_h; farm.ruler_offset_ref_value = head_h;
        farm.top_taper_x = 1.25f;
        farm.asymmetry_high_u = 0.72f; farm.asymmetry_high_amp = 0.18f;
        farm.mass_weight = 1.8f;
        farm.socket.physics_binding_mode = SocketPhysicsBindingMode::PHYSICS_BIND_HINGE_JOINT_3D;
        farm.socket.joint_limit_lower = 0.0f;
        farm.socket.joint_limit_upper = 145.0f;
        const uint32_t id_farm = sc.add_node(farm);

        ScaffoldNodeEntry hand{};
        hand.name = "Hand" + sfx;
        hand.parent_id = id_farm;
        hand.type = ScaffoldPrimitiveType::PRIMITIVE_WEDGE_BOX;
        hand.local_offset = glm::vec3(0.0f, -(fa_len * 0.5f + hand_len * 0.45f), 0.0f);
        hand.half_extents = glm::vec3(head_h * 0.18f, hand_len * 0.5f, head_h * 0.07f);
        hand.ruler_scale_x_id = r_head_h;   hand.ruler_mult_x = 0.18f;
        hand.ruler_scale_y_id = r_hand_len; hand.ruler_mult_y = 0.50f;
        hand.ruler_scale_z_id = r_head_h;   hand.ruler_mult_z = 0.07f;
        hand.ruler_offset_scale_id = r_head_h; hand.ruler_offset_ref_value = head_h;
        hand.top_taper_x = 1.12f;
        hand.mass_weight = 0.6f;
        hand.socket.physics_binding_mode = SocketPhysicsBindingMode::PHYSICS_BIND_GENERIC_6DOF_JOINT_3D;
        sc.add_node(hand);

        ScaffoldNodeEntry thigh{};
        thigh.name = "Thigh" + sfx;
        thigh.parent_id = id_pelvis;
        thigh.type = ScaffoldPrimitiveType::PRIMITIVE_TAPERED_CAPSULE;
        thigh.local_offset = glm::vec3(sx * hip_w * 0.32f, -(pel_h * 0.35f + th_len * 0.48f), 0.0f);
        thigh.half_extents = glm::vec3(head_h * 0.32f, th_len * 0.5f, head_h * 0.32f);
        thigh.ruler_scale_x_id = r_head_h; thigh.ruler_mult_x = 0.32f;
        thigh.ruler_scale_y_id = r_th_len; thigh.ruler_mult_y = 0.50f;
        thigh.ruler_scale_z_id = r_head_h; thigh.ruler_mult_z = 0.32f;
        thigh.ruler_offset_scale_id = r_head_h; thigh.ruler_offset_ref_value = head_h;
        thigh.top_taper_x = 1.28f;
        thigh.asymmetry_high_u = 0.68f; thigh.asymmetry_high_amp = 0.15f;
        thigh.asymmetry_low_u  = 0.38f; thigh.asymmetry_low_amp  = 0.12f;
        thigh.mass_weight = 7.5f;
        thigh.socket.physics_binding_mode = SocketPhysicsBindingMode::PHYSICS_BIND_PIN_JOINT_3D;
        thigh.socket.joint_limit_lower = -110.0f;
        thigh.socket.joint_limit_upper = 45.0f;
        const uint32_t id_thigh = sc.add_node(thigh);

        ScaffoldNodeEntry shin{};
        shin.name = "Shin" + sfx;
        shin.parent_id = id_thigh;
        shin.type = ScaffoldPrimitiveType::PRIMITIVE_TAPERED_CAPSULE;
        shin.local_offset = glm::vec3(0.0f, -(th_len * 0.5f + sh_len * 0.48f), 0.0f);
        shin.half_extents = glm::vec3(head_h * 0.20f, sh_len * 0.5f, head_h * 0.21f);
        shin.ruler_scale_x_id = r_head_h; shin.ruler_mult_x = 0.20f;
        shin.ruler_scale_y_id = r_sh_len; shin.ruler_mult_y = 0.50f;
        shin.ruler_scale_z_id = r_head_h; shin.ruler_mult_z = 0.21f;
        shin.ruler_offset_scale_id = r_head_h; shin.ruler_offset_ref_value = head_h;
        shin.top_taper_x = 1.32f;
        shin.asymmetry_high_u = 0.72f; shin.asymmetry_high_amp = 0.22f * sx;
        shin.asymmetry_low_u  = 0.52f; shin.asymmetry_low_amp  = 0.16f * (-sx);
        shin.mass_weight = 4.2f;
        shin.socket.physics_binding_mode = SocketPhysicsBindingMode::PHYSICS_BIND_HINGE_JOINT_3D;
        shin.socket.joint_limit_lower = -140.0f;
        shin.socket.joint_limit_upper = 0.0f;
        const uint32_t id_shin = sc.add_node(shin);

        ScaffoldNodeEntry foot{};
        foot.name = "Foot" + sfx;
        foot.parent_id = id_shin;
        foot.type = ScaffoldPrimitiveType::PRIMITIVE_WEDGE_BOX;
        foot.local_offset = glm::vec3(0.0f, -(sh_len * 0.5f + head_h * 0.14f), ft_len * 0.22f);
        foot.half_extents = glm::vec3(head_h * 0.19f, head_h * 0.14f, ft_len * 0.5f);
        foot.ruler_scale_x_id = r_head_h; foot.ruler_mult_x = 0.19f;
        foot.ruler_scale_y_id = r_head_h; foot.ruler_mult_y = 0.14f;
        foot.ruler_scale_z_id = r_ft_len; foot.ruler_mult_z = 0.50f;
        foot.ruler_offset_scale_id = r_head_h; foot.ruler_offset_ref_value = head_h;
        foot.top_taper_z = 0.72f;
        foot.mass_weight = 1.2f;
        foot.is_ground_support = true;
        foot.socket.physics_binding_mode = SocketPhysicsBindingMode::PHYSICS_BIND_HINGE_JOINT_3D;
        sc.add_node(foot);
    }

    sc.evaluate();
    return sc;
}

FormScaffoldCore FormScaffoldCore::create_loomis_quadruped_scaffold(float p_shoulder_height_m) {
    FormScaffoldCore sc;
    sc.set_ruler_graph(ProportionRulerGraph::create_loomis_quadruped_canon(p_shoulder_height_m));
    sc.set_scaffold_name("Loomis Quadruped Scaffold");
    const auto &rg = sc.get_ruler_graph();

    const uint32_t r_sh     = rg.find_node_id("shoulder_height");
    const uint32_t r_rib_l  = rg.find_node_id("ribcage_length");
    const uint32_t r_rib_d  = rg.find_node_id("ribcage_depth");
    const uint32_t r_rib_w  = rg.find_node_id("ribcage_width");
    const uint32_t r_pel_l  = rg.find_node_id("pelvis_length");
    const uint32_t r_pel_w  = rg.find_node_id("pelvis_width");
    const uint32_t r_neck_l = rg.find_node_id("neck_length");
    const uint32_t r_sk_l   = rg.find_node_id("skull_length");

    const float sh      = rg.get_value_by_name("shoulder_height", p_shoulder_height_m);
    const float rib_l   = rg.get_value_by_name("ribcage_length", sh * 0.68f);
    const float rib_d   = rg.get_value_by_name("ribcage_depth", sh * 0.46f);
    const float rib_w   = rg.get_value_by_name("ribcage_width", sh * 0.36f);
    const float pel_l   = rg.get_value_by_name("pelvis_length", sh * 0.38f);
    const float pel_w   = rg.get_value_by_name("pelvis_width", sh * 0.32f);
    const float neck_l  = rg.get_value_by_name("neck_length", sh * 0.42f);
    const float skull_l = rg.get_value_by_name("skull_length", sh * 0.40f);

    ScaffoldNodeEntry thorax{};
    thorax.name = "Thorax";
    thorax.parent_id = 0xFFFFFFFFu;
    thorax.type = ScaffoldPrimitiveType::PRIMITIVE_EGG_ELLIPSOID;
    thorax.local_offset = glm::vec3(0.0f, sh * 0.72f, rib_l * 0.25f);
    thorax.half_extents = glm::vec3(rib_w * 0.5f, rib_d * 0.5f, rib_l * 0.5f);
    thorax.ruler_scale_x_id = r_rib_w; thorax.ruler_mult_x = 0.5f;
    thorax.ruler_scale_y_id = r_rib_d; thorax.ruler_mult_y = 0.5f;
    thorax.ruler_scale_z_id = r_rib_l; thorax.ruler_mult_z = 0.5f;
    thorax.ruler_offset_scale_id = r_sh; thorax.ruler_offset_ref_value = sh;
    thorax.follows_line_of_action = true; thorax.line_of_action_t = 0.45f;
    thorax.mass_weight = 24.0f;
    const uint32_t id_thorax = sc.add_node(thorax);

    ScaffoldNodeEntry pelvis{};
    pelvis.name = "Pelvis";
    pelvis.parent_id = id_thorax;
    pelvis.type = ScaffoldPrimitiveType::PRIMITIVE_WEDGE_BOX;
    pelvis.local_offset = glm::vec3(0.0f, sh * 0.04f, -(rib_l * 0.55f + pel_l * 0.45f));
    pelvis.half_extents = glm::vec3(pel_w * 0.5f, rib_d * 0.38f, pel_l * 0.5f);
    pelvis.ruler_scale_x_id = r_pel_w; pelvis.ruler_mult_x = 0.50f;
    pelvis.ruler_scale_y_id = r_rib_d; pelvis.ruler_mult_y = 0.38f;
    pelvis.ruler_scale_z_id = r_pel_l; pelvis.ruler_mult_z = 0.50f;
    pelvis.ruler_offset_scale_id = r_sh; pelvis.ruler_offset_ref_value = sh;
    pelvis.follows_line_of_action = true; pelvis.line_of_action_t = 0.0f;
    pelvis.mass_weight = 14.0f;
    const uint32_t id_pelvis = sc.add_node(pelvis);

    ScaffoldNodeEntry neck{};
    neck.name = "Neck";
    neck.parent_id = id_thorax;
    neck.type = ScaffoldPrimitiveType::PRIMITIVE_TAPERED_CAPSULE;
    neck.local_offset = glm::vec3(0.0f, rib_d * 0.38f, rib_l * 0.52f);
    neck.local_rotation_deg = glm::vec3(38.0f, 0.0f, 0.0f);
    neck.half_extents = glm::vec3(rib_w * 0.26f, neck_l * 0.5f, rib_w * 0.28f);
    neck.ruler_scale_x_id = r_rib_w;  neck.ruler_mult_x = 0.26f;
    neck.ruler_scale_y_id = r_neck_l; neck.ruler_mult_y = 0.50f;
    neck.ruler_scale_z_id = r_rib_w;  neck.ruler_mult_z = 0.28f;
    neck.ruler_offset_scale_id = r_sh; neck.ruler_offset_ref_value = sh;
    neck.follows_line_of_action = true; neck.line_of_action_t = 0.78f;
    neck.mass_weight = 4.0f;
    const uint32_t id_neck = sc.add_node(neck);

    ScaffoldNodeEntry skull{};
    skull.name = "Skull";
    skull.parent_id = id_neck;
    skull.type = ScaffoldPrimitiveType::PRIMITIVE_SLICED_SPHERE;
    skull.local_offset = glm::vec3(0.0f, neck_l * 0.55f, skull_l * 0.25f);
    skull.local_rotation_deg = glm::vec3(-38.0f, 0.0f, 0.0f);
    skull.half_extents = glm::vec3(skull_l * 0.34f, skull_l * 0.32f, skull_l * 0.42f);
    skull.ruler_scale_x_id = r_sk_l; skull.ruler_mult_x = 0.34f;
    skull.ruler_scale_y_id = r_sk_l; skull.ruler_mult_y = 0.32f;
    skull.ruler_scale_z_id = r_sk_l; skull.ruler_mult_z = 0.42f;
    skull.ruler_offset_scale_id = r_sh; skull.ruler_offset_ref_value = sh;
    skull.follows_line_of_action = true; skull.line_of_action_t = 1.0f;
    skull.side_shave_ratio = 0.78f;
    skull.jaw_drop_ratio = 0.28f;
    skull.muzzle_length_ratio = 0.65f;
    skull.mass_weight = 4.5f;
    sc.add_node(skull);

    const float leg_h = sh * 0.55f;
    for (int side = -1; side <= 1; side += 2) {
        const std::string sfx = (side < 0) ? "_L" : "_R";
        const float sx = static_cast<float>(side);

        ScaffoldNodeEntry fleg{};
        fleg.name = "Foreleg" + sfx;
        fleg.parent_id = id_thorax;
        fleg.type = ScaffoldPrimitiveType::PRIMITIVE_TAPERED_CAPSULE;
        fleg.local_offset = glm::vec3(sx * rib_w * 0.46f, -leg_h * 0.55f, rib_l * 0.22f);
        fleg.half_extents = glm::vec3(sh * 0.065f, leg_h * 0.5f, sh * 0.075f);
        fleg.ruler_scale_x_id = r_sh; fleg.ruler_mult_x = 0.065f;
        fleg.ruler_scale_y_id = r_sh; fleg.ruler_mult_y = 0.275f;
        fleg.ruler_scale_z_id = r_sh; fleg.ruler_mult_z = 0.075f;
        fleg.ruler_offset_scale_id = r_sh; fleg.ruler_offset_ref_value = sh;
        fleg.top_taper_x = 1.35f;
        fleg.mass_weight = 4.0f;
        fleg.is_ground_support = true;
        sc.add_node(fleg);

        ScaffoldNodeEntry hleg{};
        hleg.name = "Hindleg" + sfx;
        hleg.parent_id = id_pelvis;
        hleg.type = ScaffoldPrimitiveType::PRIMITIVE_TAPERED_CAPSULE;
        hleg.local_offset = glm::vec3(sx * pel_w * 0.48f, -leg_h * 0.58f, -pel_l * 0.12f);
        hleg.half_extents = glm::vec3(sh * 0.075f, leg_h * 0.52f, sh * 0.09f);
        hleg.ruler_scale_x_id = r_sh; hleg.ruler_mult_x = 0.075f;
        hleg.ruler_scale_y_id = r_sh; hleg.ruler_mult_y = 0.286f;
        hleg.ruler_scale_z_id = r_sh; hleg.ruler_mult_z = 0.090f;
        hleg.ruler_offset_scale_id = r_sh; hleg.ruler_offset_ref_value = sh;
        hleg.top_taper_x = 1.45f;
        hleg.mass_weight = 5.0f;
        hleg.is_ground_support = true;
        sc.add_node(hleg);
    }

    sc.evaluate();
    return sc;
}

FormScaffoldCore FormScaffoldCore::create_loomis_avian_scaffold(float p_keel_length_m) {
    FormScaffoldCore sc;
    sc.set_ruler_graph(ProportionRulerGraph::create_loomis_avian_canon(p_keel_length_m));
    sc.set_scaffold_name("Loomis Avian Scaffold");
    const auto &rg = sc.get_ruler_graph();

    const uint32_t r_keel   = rg.find_node_id("keel_length");
    const uint32_t r_keel_w = rg.find_node_id("keel_width");
    const uint32_t r_keel_d = rg.find_node_id("keel_depth");
    const uint32_t r_win    = rg.find_node_id("wing_inner_len");
    const uint32_t r_wout   = rg.find_node_id("wing_outer_len");
    const uint32_t r_tarsus = rg.find_node_id("tarsus_length");
    const uint32_t r_skull  = rg.find_node_id("skull_length");

    const float k     = rg.get_value_by_name("keel_length", p_keel_length_m);
    const float w_in  = rg.get_value_by_name("wing_inner_len", k * 0.75f);
    const float w_out = rg.get_value_by_name("wing_outer_len", k * 0.85f);

    ScaffoldNodeEntry keel{};
    keel.name = "Keel";
    keel.parent_id = 0xFFFFFFFFu;
    keel.type = ScaffoldPrimitiveType::PRIMITIVE_EGG_ELLIPSOID;
    keel.local_offset = glm::vec3(0.0f, k * 0.90f, 0.0f);
    keel.local_rotation_deg = glm::vec3(-28.0f, 0.0f, 0.0f);
    keel.half_extents = glm::vec3(k * 0.29f, k * 0.32f, k * 0.52f);
    keel.ruler_scale_x_id = r_keel_w; keel.ruler_mult_x = 0.50f;
    keel.ruler_scale_y_id = r_keel_d; keel.ruler_mult_y = 0.50f;
    keel.ruler_scale_z_id = r_keel;   keel.ruler_mult_z = 0.52f;
    keel.ruler_offset_scale_id = r_keel; keel.ruler_offset_ref_value = k;
    keel.follows_line_of_action = true; keel.line_of_action_t = 0.40f;
    keel.egg_taper = 0.25f;
    keel.mass_weight = 6.0f;
    const uint32_t id_keel = sc.add_node(keel);

    ScaffoldNodeEntry head{};
    head.name = "Skull";
    head.parent_id = id_keel;
    head.type = ScaffoldPrimitiveType::PRIMITIVE_SLICED_SPHERE;
    head.local_offset = glm::vec3(0.0f, k * 0.45f, k * 0.48f);
    head.local_rotation_deg = glm::vec3(28.0f, 0.0f, 0.0f);
    head.half_extents = glm::vec3(k * 0.16f, k * 0.16f, k * 0.20f);
    head.ruler_scale_x_id = r_skull; head.ruler_mult_x = 0.38f;
    head.ruler_scale_y_id = r_skull; head.ruler_mult_y = 0.38f;
    head.ruler_scale_z_id = r_skull; head.ruler_mult_z = 0.48f;
    head.ruler_offset_scale_id = r_keel; head.ruler_offset_ref_value = k;
    head.follows_line_of_action = true; head.line_of_action_t = 1.0f;
    head.muzzle_length_ratio = 0.85f;
    head.jaw_width_ratio = 0.32f;
    head.mass_weight = 1.0f;
    sc.add_node(head);

    for (int side = -1; side <= 1; side += 2) {
        const std::string sfx = (side < 0) ? "_L" : "_R";
        const float sx = static_cast<float>(side);

        ScaffoldNodeEntry wing_in{};
        wing_in.name = "WingInner" + sfx;
        wing_in.parent_id = id_keel;
        wing_in.type = ScaffoldPrimitiveType::PRIMITIVE_WEDGE_BOX;
        wing_in.local_offset = glm::vec3(sx * (k * 0.28f + w_in * 0.45f), k * 0.18f, k * 0.05f);
        wing_in.half_extents = glm::vec3(w_in * 0.5f, k * 0.06f, k * 0.26f);
        wing_in.ruler_scale_x_id = r_win;  wing_in.ruler_mult_x = 0.50f;
        wing_in.ruler_scale_y_id = r_keel; wing_in.ruler_mult_y = 0.06f;
        wing_in.ruler_scale_z_id = r_keel; wing_in.ruler_mult_z = 0.26f;
        wing_in.ruler_offset_scale_id = r_keel; wing_in.ruler_offset_ref_value = k;
        wing_in.mass_weight = 1.2f;
        const uint32_t id_win = sc.add_node(wing_in);

        ScaffoldNodeEntry wing_out{};
        wing_out.name = "WingOuter" + sfx;
        wing_out.parent_id = id_win;
        wing_out.type = ScaffoldPrimitiveType::PRIMITIVE_WEDGE_BOX;
        wing_out.local_offset = glm::vec3(sx * (w_in * 0.5f + w_out * 0.45f), 0.0f, -k * 0.04f);
        wing_out.half_extents = glm::vec3(w_out * 0.5f, k * 0.035f, k * 0.20f);
        wing_out.ruler_scale_x_id = r_wout; wing_out.ruler_mult_x = 0.50f;
        wing_out.ruler_scale_y_id = r_keel; wing_out.ruler_mult_y = 0.035f;
        wing_out.ruler_scale_z_id = r_keel; wing_out.ruler_mult_z = 0.20f;
        wing_out.ruler_offset_scale_id = r_keel; wing_out.ruler_offset_ref_value = k;
        wing_out.top_taper_z = 0.45f;
        wing_out.mass_weight = 0.7f;
        sc.add_node(wing_out);

        ScaffoldNodeEntry leg{};
        leg.name = "Tarsus" + sfx;
        leg.parent_id = id_keel;
        leg.type = ScaffoldPrimitiveType::PRIMITIVE_TAPERED_CAPSULE;
        leg.local_offset = glm::vec3(sx * k * 0.16f, -k * 0.52f, -k * 0.08f);
        leg.local_rotation_deg = glm::vec3(28.0f, 0.0f, 0.0f);
        leg.half_extents = glm::vec3(k * 0.05f, k * 0.34f, k * 0.05f);
        leg.ruler_scale_x_id = r_keel;   leg.ruler_mult_x = 0.05f;
        leg.ruler_scale_y_id = r_tarsus; leg.ruler_mult_y = 0.55f;
        leg.ruler_scale_z_id = r_keel;   leg.ruler_mult_z = 0.05f;
        leg.ruler_offset_scale_id = r_keel; leg.ruler_offset_ref_value = k;
        leg.mass_weight = 0.6f;
        leg.is_ground_support = true;
        sc.add_node(leg);
    }

    sc.evaluate();
    return sc;
}

FormScaffoldCore FormScaffoldCore::create_loomis_serpentine_scaffold(float p_skull_length_m, uint32_t p_segments) {
    FormScaffoldCore sc;
    sc.set_ruler_graph(ProportionRulerGraph::create_loomis_serpentine_canon(p_skull_length_m));
    sc.set_scaffold_name("Loomis Serpentine Scaffold");
    const auto &rg = sc.get_ruler_graph();

    const uint32_t r_skull   = rg.find_node_id("skull_length");
    const uint32_t r_skull_w = rg.find_node_id("skull_width");
    const uint32_t r_body_l  = rg.find_node_id("body_length");

    const uint32_t segs  = std::clamp<uint32_t>(p_segments, 4u, 24u);
    const float skull_l  = rg.get_value_by_name("skull_length", p_skull_length_m);
    const float body_len = rg.get_value_by_name("body_length", p_skull_length_m * 14.0f);
    const float seg_len  = body_len / static_cast<float>(segs);

    ScaffoldNodeEntry head{};
    head.name = "Head";
    head.parent_id = 0xFFFFFFFFu;
    head.type = ScaffoldPrimitiveType::PRIMITIVE_SLICED_SPHERE;
    head.local_offset = glm::vec3(0.0f, skull_l * 0.42f, 0.0f);
    head.half_extents = glm::vec3(skull_l * 0.34f, skull_l * 0.24f, skull_l * 0.50f);
    head.ruler_scale_x_id = r_skull_w; head.ruler_mult_x = 0.52f;
    head.ruler_scale_y_id = r_skull;   head.ruler_mult_y = 0.24f;
    head.ruler_scale_z_id = r_skull;   head.ruler_mult_z = 0.50f;
    head.ruler_offset_scale_id = r_skull; head.ruler_offset_ref_value = skull_l;
    head.follows_line_of_action = true; head.line_of_action_t = 0.0f;
    head.muzzle_length_ratio = 0.55f;
    head.mass_weight = 1.5f;
    head.is_ground_support = true;
    uint32_t prev_id = sc.add_node(head);

    for (uint32_t i = 0u; i < segs; ++i) {
        const float t = static_cast<float>(i + 1u) / static_cast<float>(segs);
        const float girth = std::sin(std::clamp(t, 0.05f, 0.98f) * kPi) * 0.42f + 0.08f;
        const float r = skull_l * girth;

        ScaffoldNodeEntry seg{};
        seg.name = "SpineSeg_" + std::to_string(i);
        seg.parent_id = prev_id;
        seg.type = ScaffoldPrimitiveType::PRIMITIVE_TAPERED_CAPSULE;
        seg.local_offset = glm::vec3(0.0f, 0.0f, -seg_len);
        seg.local_rotation_deg = glm::vec3(90.0f, 0.0f, 0.0f);
        seg.half_extents = glm::vec3(r, seg_len * 0.52f, r);
        seg.ruler_scale_x_id = r_skull;  seg.ruler_mult_x = girth;
        seg.ruler_scale_y_id = r_body_l; seg.ruler_mult_y = 0.52f / static_cast<float>(segs);
        seg.ruler_scale_z_id = r_skull;  seg.ruler_mult_z = girth;
        seg.ruler_offset_scale_id = r_body_l; seg.ruler_offset_ref_value = body_len;
        seg.follows_line_of_action = true; seg.line_of_action_t = t;
        seg.mass_weight = std::max(0.2f, girth * 4.0f);
        seg.is_ground_support = (i == segs / 2u || i == segs - 1u);
        seg.socket.physics_binding_mode = SocketPhysicsBindingMode::PHYSICS_BIND_GENERIC_6DOF_JOINT_3D;
        seg.socket.joint_limit_lower = -35.0f;
        seg.socket.joint_limit_upper = 35.0f;
        prev_id = sc.add_node(seg);
    }

    sc.evaluate();
    return sc;
}

} // namespace ideam::core
