#include "structure_layout_solver.h"
#include "../geometry/csg_2d.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace ideam::core {

void StructureLayoutSolver::clear() noexcept {
    nodes_.clear();
    sockets_.clear();
    style_rules_.clear();
    aperture_cutouts_.clear();
}

uint32_t StructureLayoutSolver::add_structure_node(const StructureNodePod &p_node) {
    StructureNodePod copy = p_node;
    copy.node_id = static_cast<uint32_t>(nodes_.size());
    nodes_.push_back(copy);
    return copy.node_id;
}

uint32_t StructureLayoutSolver::add_socket(const SocketPod &p_socket) {
    SocketPod copy = p_socket;
    copy.socket_id = static_cast<uint32_t>(sockets_.size());
    sockets_.push_back(copy);
    return copy.socket_id;
}

uint32_t StructureLayoutSolver::add_style_rule(const Style3DRulePod &p_rule) {
    Style3DRulePod copy = p_rule;
    copy.rule_id = static_cast<uint32_t>(style_rules_.size());
    if (copy.specificity == 0u) {
        copy.specificity = compute_style_specificity(copy);
    }
    style_rules_.push_back(copy);
    return copy.rule_id;
}

uint32_t StructureLayoutSolver::add_aperture_cutout(const ApertureCutoutPod &p_cutout) {
    const uint32_t id = static_cast<uint32_t>(aperture_cutouts_.size());
    aperture_cutouts_.push_back(p_cutout);
    return id;
}

uint16_t StructureLayoutSolver::compute_style_specificity(const Style3DRulePod &p_rule) noexcept {
    uint32_t score = 0u;
    if (p_rule.semantic_role_selector != 0u) {
        if ((p_rule.semantic_role_selector & 0x000000FFu) != 0u)      score += 400u;
        else if ((p_rule.semantic_role_selector & 0x0000FF00u) != 0u) score += 300u;
        else if ((p_rule.semantic_role_selector & 0x00FF0000u) != 0u) score += 200u;
        else                                                          score += 100u;
    }
    if (p_rule.parent_role_selector != 0u) {
        score += 150u;
    }
    score += static_cast<uint32_t>(std::popcount(p_rule.required_nuclear_seme_mask)) * 50u;
    score += static_cast<uint32_t>(std::popcount(p_rule.required_peripheral_seme_mask)) * 20u;
    return static_cast<uint16_t>(std::min<uint32_t>(65535u, score));
}

void StructureLayoutSolver::sort_style_rules_by_cascade(std::span<Style3DRulePod> r_rules) noexcept {
    for (Style3DRulePod &r : r_rules) {
        r.specificity = compute_style_specificity(r);
    }
    std::stable_sort(
        r_rules.begin(),
        r_rules.end(),
        [](const Style3DRulePod &a, const Style3DRulePod &b) {
            if (a.is_important != b.is_important) return a.is_important < b.is_important;
            if (a.layer != b.layer)               return static_cast<uint8_t>(a.layer) < static_cast<uint8_t>(b.layer);
            if (a.priority != b.priority)         return a.priority < b.priority;
            if (a.specificity != b.specificity)   return a.specificity < b.specificity;
            return a.rule_id < b.rule_id;
        }
    );
}

void StructureLayoutSolver::apply_theme_cascade_to_nodes() noexcept {
    if (style_rules_.empty() || nodes_.empty()) {
        return;
    }
    sort_style_rules_by_cascade(style_rules_);

    for (StructureNodePod &node : nodes_) {
        const uint32_t parent_role = (node.parent_id < nodes_.size())
            ? nodes_[node.parent_id].semantic_tag_id
            : 0u;

        for (const Style3DRulePod &rule : style_rules_) {
            if (match_semantic_role_hierarchy(rule.semantic_role_selector, node.semantic_tag_id) == 0u) {
                continue;
            }
            if (rule.parent_role_selector != 0u &&
                match_semantic_role_hierarchy(rule.parent_role_selector, parent_role) == 0u) {
                continue;
            }
            if ((node.nuclear_seme_mask_lo32 & rule.required_nuclear_seme_mask) != rule.required_nuclear_seme_mask) {
                continue;
            }
            if ((node.peripheral_seme_mask_lo32 & rule.required_peripheral_seme_mask) != rule.required_peripheral_seme_mask) {
                continue;
            }

            // Apply matching rule in ascending cascade order (highest precedence rule wins last)
            node.style_slot_id    = (rule.mesh_asset_id != 0u) ? rule.mesh_asset_id : rule.rule_id;
            node.scale_mode       = rule.scale_mode;
            node.edge_snap_policy = rule.edge_snap_policy;
            node.cascade_layer    = rule.layer;
            node.rest_obb_extents = glm::vec3(
                node.rest_obb_extents.x * rule.target_extents_scale.x,
                node.rest_obb_extents.y * rule.target_extents_scale.y,
                node.rest_obb_extents.z * rule.target_extents_scale.z
            );
            node.margin_uniform   = rule.margin_override.x;
        }
    }
}

bool StructureLayoutSolver::are_sockets_compatible(const SocketPod &p_a, const SocketPod &p_b) noexcept {
    if ((p_a.compatibility_bitmask & p_b.compatibility_bitmask) == 0ULL) {
        return false;
    }
    if (p_a.owner_node_id == p_b.owner_node_id) {
        return false;
    }
    // Check opposing normals (dot <= 0.25) when both normals are non-zero
    const float dot_n =
        p_a.normal.x * p_b.normal.x +
        p_a.normal.y * p_b.normal.y +
        p_a.normal.z * p_b.normal.z;
    return dot_n <= 0.25f;
}

void StructureLayoutSolver::solve_socket_contacts() noexcept {
    for (size_t i = 0; i < sockets_.size(); ++i) {
        SocketPod &sock_a = sockets_[i];
        if (sock_a.mated_socket_id != 0xFFFFFFFFu && sock_a.mated_socket_id < sockets_.size()) {
            const SocketPod &sock_b = sockets_[sock_a.mated_socket_id];
            if (sock_a.owner_node_id < nodes_.size() && sock_b.owner_node_id < nodes_.size()) {
                StructureNodePod &child_node  = nodes_[sock_a.owner_node_id];
                const StructureNodePod &host  = nodes_[sock_b.owner_node_id];
                child_node.rest_obb_center = glm::vec3(
                    host.rest_obb_center.x + sock_b.position.x - sock_a.position.x,
                    host.rest_obb_center.y + sock_b.position.y - sock_a.position.y,
                    host.rest_obb_center.z + sock_b.position.z - sock_a.position.z
                );
                sock_a.is_occupied = 1u;
            }
            continue;
        }

        // Auto-mate unoccupied male plugs to compatible unoccupied female receptacles
        if (sock_a.is_male_plug == 0u || sock_a.is_occupied != 0u) {
            continue;
        }
        for (size_t j = 0; j < sockets_.size(); ++j) {
            if (i == j) continue;
            SocketPod &sock_b = sockets_[j];
            if (sock_b.is_male_plug != 0u || sock_b.is_occupied != 0u) continue;
            if (!are_sockets_compatible(sock_a, sock_b)) continue;

            sock_a.mated_socket_id = static_cast<uint32_t>(j);
            sock_b.mated_socket_id = static_cast<uint32_t>(i);
            sock_a.is_occupied     = 1u;
            sock_b.is_occupied     = 1u;

            if (sock_a.owner_node_id < nodes_.size() && sock_b.owner_node_id < nodes_.size()) {
                StructureNodePod &child_node = nodes_[sock_a.owner_node_id];
                const StructureNodePod &host = nodes_[sock_b.owner_node_id];
                child_node.rest_obb_center = glm::vec3(
                    host.rest_obb_center.x + sock_b.position.x - sock_a.position.x,
                    host.rest_obb_center.y + sock_b.position.y - sock_a.position.y,
                    host.rest_obb_center.z + sock_b.position.z - sock_a.position.z
                );
            }
            break;
        }
    }
}

void StructureLayoutSolver::solve_layout_constraints() {
    apply_theme_cascade_to_nodes();

    // 1. Solve Container3D layout modes per parent node (`LINEAR_BOX`, `GRID_3D`, `RADIAL_RING`, `SPLINE_PATH`)
    for (size_t p = 0; p < nodes_.size(); ++p) {
        const StructureNodePod &parent = nodes_[p];
        std::vector<uint32_t> child_ids;
        for (size_t c = 0; c < nodes_.size(); ++c) {
            if (nodes_[c].parent_id == static_cast<uint32_t>(p)) {
                child_ids.push_back(static_cast<uint32_t>(c));
            }
        }
        if (child_ids.empty()) {
            continue;
        }

        if (parent.container_mode == DesignContainerLayoutMode::LAYOUT_RADIAL_RING) {
            const float radius = std::max(0.1f, parent.rest_obb_extents.x);
            const size_t n     = child_ids.size();
            for (size_t idx = 0; idx < n; ++idx) {
                StructureNodePod &child = nodes_[child_ids[idx]];
                const float angle = (2.0f * std::numbers::pi_v<float> * static_cast<float>(idx)) / static_cast<float>(n);
                child.rest_obb_center = glm::vec3(
                    parent.rest_obb_center.x + std::cos(angle) * radius,
                    parent.rest_obb_center.y + child.rest_obb_center.y,
                    parent.rest_obb_center.z + std::sin(angle) * radius
                );
            }
        } else if (parent.container_mode == DesignContainerLayoutMode::LAYOUT_GRID_3D) {
            const uint32_t cols = std::max<uint32_t>(1u, static_cast<uint32_t>(std::ceil(std::sqrt(static_cast<float>(child_ids.size())))));
            for (size_t idx = 0; idx < child_ids.size(); ++idx) {
                StructureNodePod &child = nodes_[child_ids[idx]];
                const uint32_t gx = static_cast<uint32_t>(idx) % cols;
                const uint32_t gz = static_cast<uint32_t>(idx) / cols;
                const float step_x = (child.rest_obb_extents.x + child.margin_uniform) * 2.0f;
                const float step_z = (child.rest_obb_extents.z + child.margin_uniform) * 2.0f;
                child.rest_obb_center = glm::vec3(
                    parent.rest_obb_center.x + static_cast<float>(gx) * step_x,
                    parent.rest_obb_center.y,
                    parent.rest_obb_center.z + static_cast<float>(gz) * step_z
                );
            }
        } else if (parent.container_mode == DesignContainerLayoutMode::LAYOUT_LINEAR_BOX) {
            // Stack children along Y if multiple children have DESIGN_SIZE_EXPAND, while preserving explicit offsets
            float cursor_y = parent.rest_obb_center.y - parent.rest_obb_extents.y;
            for (uint32_t c_id : child_ids) {
                StructureNodePod &child = nodes_[c_id];
                if ((child.size_flags_y & DESIGN_SIZE_EXPAND) != 0u) {
                    cursor_y += child.rest_obb_extents.y + child.margin_uniform;
                    child.rest_obb_center.y = cursor_y;
                    cursor_y += child.rest_obb_extents.y + child.margin_uniform;
                }
            }
        }
    }

    // 2. Solve SocketPod contacts
    solve_socket_contacts();

    // 3. Compose world-space `fitted_transform` matrices
    for (auto &node : nodes_) {
        // Store translation in column 3 of fitted_transform
        node.fitted_transform = glm::mat4(1.0f);
        node.fitted_transform[0][0] = std::max(0.001f, node.rest_obb_extents.x * 2.0f);
        node.fitted_transform[1][1] = std::max(0.001f, node.rest_obb_extents.y * 2.0f);
        node.fitted_transform[2][2] = std::max(0.001f, node.rest_obb_extents.z * 2.0f);
        node.fitted_transform[3][0] = node.rest_obb_center.x;
        node.fitted_transform[3][1] = node.rest_obb_center.y;
        node.fitted_transform[3][2] = node.rest_obb_center.z;
        node.fitted_transform[3][3] = 1.0f;

        if (node.parent_id < nodes_.size()) {
            const auto &parent = nodes_[node.parent_id];
            node.fitted_transform[3][0] += parent.rest_obb_center.x;
            node.fitted_transform[3][1] += parent.rest_obb_center.y;
            node.fitted_transform[3][2] += parent.rest_obb_center.z;
        }
    }

    // 4. Evaluate boolean aperture cutouts on host walls
    evaluate_aperture_cutouts();
}

uint32_t StructureLayoutSolver::evaluate_aperture_cutouts() {
    uint32_t applied_count = 0u;

    // Automatically discover any SEMANTIC_ROLE_APERTURE node parented to a SEMANTIC_ROLE_SURFACE_WALL node
    for (const StructureNodePod &node : nodes_) {
        if (match_semantic_role_hierarchy(SEMANTIC_ROLE_APERTURE, node.semantic_tag_id) > 0u &&
            node.parent_id < nodes_.size() &&
            match_semantic_role_hierarchy(SEMANTIC_ROLE_SURFACE_WALL, nodes_[node.parent_id].semantic_tag_id) > 0u) {
            bool exists = false;
            for (const ApertureCutoutPod &cut : aperture_cutouts_) {
                if (cut.aperture_node_id == node.node_id && cut.host_wall_node_id == node.parent_id) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                ApertureCutoutPod cut{};
                cut.aperture_node_id       = node.node_id;
                cut.host_wall_node_id      = node.parent_id;
                cut.aperture_semantic_role = node.semantic_tag_id;
                cut.local_center_on_wall   = node.rest_obb_center;
                cut.cutout_half_extents    = node.rest_obb_extents;
                cut.wall_thickness         = nodes_[node.parent_id].rest_obb_extents.z * 2.0f;
                cut.spawns_hinge_socket    = (node.semantic_tag_id == SEMANTIC_ROLE_APERTURE_DOOR) ? 1u : 0u;
                aperture_cutouts_.push_back(cut);
            }
        }
    }

    for (ApertureCutoutPod &cut : aperture_cutouts_) {
        if (cut.host_wall_node_id >= nodes_.size()) continue;
        StructureNodePod &wall = nodes_[cut.host_wall_node_id];

        // Classify and clip the 2D wall elevation rectangle against the aperture rectangle
        const float wx = std::max(0.1f, wall.rest_obb_extents.x);
        const float wy = std::max(0.1f, wall.rest_obb_extents.y);
        const float ax = std::max(0.05f, cut.cutout_half_extents.x);
        const float ay = std::max(0.05f, cut.cutout_half_extents.y);
        const float cx = cut.local_center_on_wall.x;
        const float cy = cut.local_center_on_wall.y;

        // Compute remaining wall segments around the aperture (left pier, right pier, lintel header, sill)
        uint32_t segs = 0u;
        if ((cx - ax) > -wx + 1e-3f) ++segs; // Left wall pier
        if ((cx + ax) <  wx - 1e-3f) ++segs; // Right wall pier
        if ((cy + ay) <  wy - 1e-3f) ++segs; // Top lintel header
        if ((cy - ay) > -wy + 1e-3f) ++segs; // Bottom window sill

        cut.resulting_segment_count   = std::max<uint32_t>(1u, segs);
        cut.is_applied                = 1u;
        wall.is_aperture_cutout_dirty = 1u;
        ++applied_count;

        // If this aperture is a Door and spawns_hinge_socket is enabled, ensure a HINGE SocketPod exists
        if (cut.spawns_hinge_socket != 0u) {
            bool has_hinge = false;
            for (const SocketPod &s : sockets_) {
                if (s.owner_node_id == cut.host_wall_node_id &&
                    s.physics_binding_mode == SocketPhysicsBindingMode::PHYSICS_BIND_HINGE_JOINT_3D) {
                    has_hinge = true;
                    break;
                }
            }
            if (!has_hinge) {
                SocketPod hinge{};
                hinge.owner_node_id        = cut.host_wall_node_id;
                hinge.compatibility_bitmask = static_cast<uint64_t>(SEMANTIC_ROLE_APERTURE_DOOR);
                hinge.position             = glm::vec3(cx - ax, cy, 0.0f);
                hinge.normal               = glm::vec3(1.0f, 0.0f, 0.0f);
                hinge.tangent              = glm::vec3(0.0f, 1.0f, 0.0f);
                hinge.physics_binding_mode = SocketPhysicsBindingMode::PHYSICS_BIND_HINGE_JOINT_3D;
                hinge.joint_limit_lower    = 0.0f;
                hinge.joint_limit_upper    = 1.5707963f; // 90 deg door swing
                add_socket(hinge);
            }
        }
    }

    return applied_count;
}

} // namespace ideam::core
