#include "blueprint_core.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace ideam::core {

void ArchitecturalBlueprintCore::clear() noexcept {
    spaces_.clear();
    walls_.clear();
    features_.clear();
    map_key_.clear();
    annotations_.clear();
}

BlueprintLayerVisualStylePOD ArchitecturalBlueprintCore::get_layer_visual_style(
    BlueprintLayerType p_layer,
    bool p_is_exterior_wall
) noexcept {
    // Okabe-Ito color-blind-safe palette + architectural CAD line-weight hierarchy
    switch (p_layer) {
        case BlueprintLayerType::LAYER_SPACES:
            return { glm::vec4(0.34f, 0.71f, 0.91f, 0.35f), 1.25f, FOUR_FOOT_SECTION_CUT_METERS };
        case BlueprintLayerType::LAYER_WALLS:
            return p_is_exterior_wall
                ? BlueprintLayerVisualStylePOD{ glm::vec4(0.00f, 0.45f, 0.70f, 1.00f), 3.50f, FOUR_FOOT_SECTION_CUT_METERS }
                : BlueprintLayerVisualStylePOD{ glm::vec4(0.00f, 0.62f, 0.45f, 1.00f), 2.20f, FOUR_FOOT_SECTION_CUT_METERS };
        case BlueprintLayerType::LAYER_FEATURES_AND_APERTURES:
            return { glm::vec4(0.90f, 0.62f, 0.00f, 1.00f), 2.00f, FOUR_FOOT_SECTION_CUT_METERS };
        case BlueprintLayerType::LAYER_LABELS_AND_MEASUREMENTS:
        default:
            return { glm::vec4(0.80f, 0.47f, 0.65f, 1.00f), 1.00f, FOUR_FOOT_SECTION_CUT_METERS };
    }
}

float ArchitecturalBlueprintCore::compute_polygon_area_2d(std::span<const glm::vec2> p_verts) noexcept {
    if (p_verts.size() < 3u) return 0.0f;
    double accum = 0.0;
    for (size_t i = 0; i < p_verts.size(); ++i) {
        const glm::vec2 &a = p_verts[i];
        const glm::vec2 &b = p_verts[(i + 1u) % p_verts.size()];
        accum += static_cast<double>(a.x) * static_cast<double>(b.y) -
                 static_cast<double>(b.x) * static_cast<double>(a.y);
    }
    return static_cast<float>(std::abs(accum) * 0.5);
}

bool ArchitecturalBlueprintCore::are_segments_collinear_shared(
    const glm::vec2 &p_a0,
    const glm::vec2 &p_a1,
    const glm::vec2 &p_b0,
    const glm::vec2 &p_b1,
    float p_tol
) noexcept {
    const float tol_sq = p_tol * p_tol;
    auto dist_sq = [](const glm::vec2 &u, const glm::vec2 &v) {
        const float dx = u.x - v.x;
        const float dy = u.y - v.y;
        return dx * dx + dy * dy;
    };
    const bool direct  = (dist_sq(p_a0, p_b0) <= tol_sq && dist_sq(p_a1, p_b1) <= tol_sq);
    const bool reverse = (dist_sq(p_a0, p_b1) <= tol_sq && dist_sq(p_a1, p_b0) <= tol_sq);
    return direct || reverse;
}

uint32_t ArchitecturalBlueprintCore::add_space_polygon(
    std::string_view p_room_name,
    std::span<const glm::vec2> p_vertices_2d,
    uint32_t p_semantic_role_id,
    uint32_t p_nuclear_seme_mask,
    float p_floor_elevation,
    float p_ceiling_height
) {
    BlueprintSpacePolygonPOD space{};
    space.space_id          = static_cast<uint32_t>(spaces_.size());
    space.semantic_role_id  = p_semantic_role_id;
    space.nuclear_seme_mask = p_nuclear_seme_mask;
    space.room_name         = std::string(p_room_name);
    space.vertices_2d.assign(p_vertices_2d.begin(), p_vertices_2d.end());
    space.floor_elevation   = p_floor_elevation;
    space.ceiling_height    = std::max(1.0f, p_ceiling_height);
    space.computed_area_sqm = compute_polygon_area_2d(space.vertices_2d);
    spaces_.push_back(std::move(space));

    rebuild_reactive_layers();
    return static_cast<uint32_t>(spaces_.size() - 1u);
}

uint32_t ArchitecturalBlueprintCore::add_feature_symbol(
    BlueprintFeatureSymbolType p_type,
    const glm::vec2 &p_position_2d,
    float p_width_meters,
    float p_height_meters,
    float p_sill_elevation,
    std::string_view p_map_key_label
) {
    BlueprintFeatureSymbolPOD feat{};
    feat.feature_id     = static_cast<uint32_t>(features_.size());
    feat.symbol_type    = p_type;
    feat.position_2d    = p_position_2d;
    feat.width_meters   = std::max(0.2f, p_width_meters);
    feat.height_meters  = std::max(0.2f, p_height_meters);
    feat.sill_elevation = p_sill_elevation;
    feat.map_key_code   = static_cast<uint32_t>(p_type) + 1u;
    features_.push_back(feat);

    // Sync Map Key entry
    bool found_key = false;
    for (auto &entry : map_key_) {
        if (entry.symbol_type == p_type) {
            found_key = true;
            if (!p_map_key_label.empty()) {
                entry.symbol_label = std::string(p_map_key_label);
            }
            break;
        }
    }
    if (!found_key) {
        BlueprintMapKeyEntryPOD entry{};
        entry.map_key_code = static_cast<uint32_t>(p_type) + 1u;
        entry.symbol_type  = p_type;
        entry.symbol_label = p_map_key_label.empty() ? "ApertureFeature" : std::string(p_map_key_label);
        map_key_.push_back(std::move(entry));
    }

    rebuild_reactive_layers();
    return feat.feature_id;
}

void ArchitecturalBlueprintCore::rebuild_reactive_layers() {
    walls_.clear();
    annotations_.clear();

    // 1. Extract wall segments from all space polygons and classify Exterior (heavy) vs Interior (medium-heavy)
    for (size_t s = 0; s < spaces_.size(); ++s) {
        const BlueprintSpacePolygonPOD &sp = spaces_[s];
        const size_t nv = sp.vertices_2d.size();
        if (nv < 3u) continue;

        glm::vec2 centroid{0.0f, 0.0f};
        for (const glm::vec2 &v : sp.vertices_2d) {
            centroid.x += v.x;
            centroid.y += v.y;
        }
        centroid.x /= static_cast<float>(nv);
        centroid.y /= static_cast<float>(nv);

        // Emit room area annotation on LAYER_LABELS_AND_MEASUREMENTS
        BlueprintMeasurementAnnotationPOD area_ann{};
        area_ann.annotation_id = static_cast<uint32_t>(annotations_.size());
        area_ann.target_id     = sp.space_id;
        area_ann.anchor_2d     = centroid;
        area_ann.measured_val  = sp.computed_area_sqm;
        area_ann.label_text    = sp.room_name;
        annotations_.push_back(std::move(area_ann));

        for (size_t i = 0; i < nv; ++i) {
            const glm::vec2 &p0 = sp.vertices_2d[i];
            const glm::vec2 &p1 = sp.vertices_2d[(i + 1u) % nv];

            // Check if an existing wall segment is shared with another space polygon
            bool merged_interior = false;
            for (BlueprintWallSegmentPOD &existing : walls_) {
                if (are_segments_collinear_shared(p0, p1, existing.start_2d, existing.end_2d)) {
                    existing.space_b_id       = sp.space_id;
                    existing.is_exterior      = 0u;
                    existing.thickness_meters = 0.15f; // Interior partition wall
                    existing.stroke_weight_px = get_layer_visual_style(BlueprintLayerType::LAYER_WALLS, false).stroke_weight_px;
                    merged_interior           = true;
                    break;
                }
            }
            if (!merged_interior) {
                BlueprintWallSegmentPOD wall{};
                wall.wall_id          = static_cast<uint32_t>(walls_.size());
                wall.space_a_id       = sp.space_id;
                wall.space_b_id       = 0xFFFFFFFFu;
                wall.start_2d         = p0;
                wall.end_2d           = p1;
                wall.thickness_meters = 0.30f; // Exterior heavy structural wall
                wall.height_meters    = sp.ceiling_height;
                wall.stroke_weight_px = get_layer_visual_style(BlueprintLayerType::LAYER_WALLS, true).stroke_weight_px;
                wall.is_exterior      = 1u;
                walls_.push_back(wall);
            }
        }
    }

    // 2. Bind each feature/aperture symbol on LAYER_FEATURES_AND_APERTURES to its nearest host wall
    for (auto &entry : map_key_) {
        entry.instance_count = 0u;
    }

    for (BlueprintFeatureSymbolPOD &feat : features_) {
        float best_dist_sq   = 1e9f;
        uint32_t best_wall   = 0xFFFFFFFFu;
        for (const BlueprintWallSegmentPOD &w : walls_) {
            const float vx = w.end_2d.x - w.start_2d.x;
            const float vy = w.end_2d.y - w.start_2d.y;
            const float len_sq = vx * vx + vy * vy;
            float t = 0.5f;
            if (len_sq > 1e-6f) {
                t = ((feat.position_2d.x - w.start_2d.x) * vx + (feat.position_2d.y - w.start_2d.y) * vy) / len_sq;
                t = std::clamp(t, 0.0f, 1.0f);
            }
            const float px = w.start_2d.x + t * vx;
            const float py = w.start_2d.y + t * vy;
            const float d2 = (feat.position_2d.x - px) * (feat.position_2d.x - px) +
                             (feat.position_2d.y - py) * (feat.position_2d.y - py);
            if (d2 < best_dist_sq) {
                best_dist_sq = d2;
                best_wall    = w.wall_id;
            }
        }
        feat.host_wall_id = best_wall;

        for (auto &entry : map_key_) {
            if (entry.symbol_type == feat.symbol_type) {
                ++entry.instance_count;
            }
        }
    }
}

void ArchitecturalBlueprintCore::extrude_to_structure_layout(StructureLayoutSolver &r_solver) const {
    // 1. Create Room Assembly & Floor/Ceiling nodes for each space polygon
    for (const BlueprintSpacePolygonPOD &sp : spaces_) {
        if (sp.vertices_2d.empty()) continue;
        glm::vec2 min_pt = sp.vertices_2d.front();
        glm::vec2 max_pt = sp.vertices_2d.front();
        for (const glm::vec2 &v : sp.vertices_2d) {
            min_pt.x = std::min(min_pt.x, v.x);
            min_pt.y = std::min(min_pt.y, v.y);
            max_pt.x = std::max(max_pt.x, v.x);
            max_pt.y = std::max(max_pt.y, v.y);
        }
        const glm::vec2 center_2d{ (min_pt.x + max_pt.x) * 0.5f, (min_pt.y + max_pt.y) * 0.5f };
        const glm::vec2 half_2d{ std::max(0.25f, (max_pt.x - min_pt.x) * 0.5f), std::max(0.25f, (max_pt.y - min_pt.y) * 0.5f) };

        StructureNodePod room_node{};
        room_node.parent_id              = 0xFFFFFFFFu;
        room_node.semantic_tag_id        = sp.semantic_role_id;
        room_node.nuclear_seme_mask_lo32 = sp.nuclear_seme_mask;
        room_node.rest_obb_center        = glm::vec3(center_2d.x, sp.floor_elevation + sp.ceiling_height * 0.5f, center_2d.y);
        room_node.rest_obb_extents       = glm::vec3(half_2d.x, sp.ceiling_height * 0.5f, half_2d.y);
        const uint32_t room_id           = r_solver.add_structure_node(room_node);

        StructureNodePod floor_node{};
        floor_node.parent_id              = room_id;
        floor_node.semantic_tag_id        = SEMANTIC_ROLE_SURFACE_FLOOR;
        floor_node.nuclear_seme_mask_lo32 = sp.nuclear_seme_mask;
        floor_node.scale_mode             = DesignScaleMode::REPEAT_TILES;
        floor_node.rest_obb_center        = glm::vec3(0.0f, -sp.ceiling_height * 0.5f, 0.0f);
        floor_node.rest_obb_extents       = glm::vec3(half_2d.x, 0.10f, half_2d.y);
        r_solver.add_structure_node(floor_node);
    }

    // 2. Create 3D Wall StructureNodePods for each wall segment
    std::vector<uint32_t> wall_to_node_id(walls_.size(), 0xFFFFFFFFu);
    for (size_t i = 0; i < walls_.size(); ++i) {
        const BlueprintWallSegmentPOD &w = walls_[i];
        const float dx  = w.end_2d.x - w.start_2d.x;
        const float dz  = w.end_2d.y - w.start_2d.y;
        const float len = std::sqrt(dx * dx + dz * dz);

        StructureNodePod wall_node{};
        wall_node.parent_id        = 0xFFFFFFFFu;
        wall_node.semantic_tag_id  = (w.is_exterior != 0u)
            ? SEMANTIC_ROLE_SURFACE_WALL_EXTERIOR
            : SEMANTIC_ROLE_SURFACE_WALL_INTERIOR;
        wall_node.scale_mode       = DesignScaleMode::STRETCH_MIDDLE;
        wall_node.edge_snap_policy = EdgeSnapPolicy::SNAP_MITER_CORNER;
        wall_node.rest_obb_center  = glm::vec3(
            (w.start_2d.x + w.end_2d.x) * 0.5f,
            w.height_meters * 0.5f,
            (w.start_2d.y + w.end_2d.y) * 0.5f
        );
        wall_node.rest_obb_extents = glm::vec3(
            std::max(0.1f, len * 0.5f),
            w.height_meters * 0.5f,
            w.thickness_meters * 0.5f
        );
        wall_to_node_id[i] = r_solver.add_structure_node(wall_node);
    }

    // 3. Create Aperture nodes & boolean cutouts on host walls
    for (const BlueprintFeatureSymbolPOD &feat : features_) {
        if (feat.host_wall_id >= wall_to_node_id.size()) continue;
        const uint32_t host_node_id = wall_to_node_id[feat.host_wall_id];
        if (host_node_id == 0xFFFFFFFFu) continue;

        uint32_t role = SEMANTIC_ROLE_APERTURE_DOOR;
        if (feat.symbol_type == BlueprintFeatureSymbolType::SYMBOL_WINDOW_SASH) {
            role = SEMANTIC_ROLE_APERTURE_WINDOW;
        } else if (feat.symbol_type == BlueprintFeatureSymbolType::SYMBOL_ARCHWAY) {
            role = SEMANTIC_ROLE_APERTURE_ARCHWAY;
        } else if (feat.symbol_type == BlueprintFeatureSymbolType::SYMBOL_STAIR_RUN) {
            role = SEMANTIC_ROLE_APERTURE_STAIR_OPENING;
        }

        StructureNodePod ap_node{};
        ap_node.parent_id        = host_node_id;
        ap_node.semantic_tag_id  = role;
        ap_node.edge_snap_policy = EdgeSnapPolicy::SNAP_SOCKET_ANCHOR;
        ap_node.rest_obb_center  = glm::vec3(0.0f, feat.sill_elevation, 0.0f);
        ap_node.rest_obb_extents = glm::vec3(feat.width_meters * 0.5f, feat.height_meters * 0.5f, 0.20f);
        r_solver.add_structure_node(ap_node);
    }

    r_solver.solve_layout_constraints();
}

} // namespace ideam::core
