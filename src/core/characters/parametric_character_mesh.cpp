#include "parametric_character_mesh.h"
#include <algorithm>
#include <cmath>

namespace ideam::core {

void ParametricCharacterMesh::clear() noexcept {
    base_vertices_.clear();
    base_quads_.clear();
    sparse_morph_deltas_.clear();
}

uint32_t ParametricCharacterMesh::add_vertex(const QuadLoopVertexPod &p_vertex) {
    const uint32_t id = static_cast<uint32_t>(base_vertices_.size());
    base_vertices_.push_back(p_vertex);
    return id;
}

uint32_t ParametricCharacterMesh::add_quad(const QuadFacePod &p_quad) {
    const uint32_t id = static_cast<uint32_t>(base_quads_.size());
    base_quads_.push_back(p_quad);
    return id;
}

uint32_t ParametricCharacterMesh::add_morph_delta(const MorphDeltaPod &p_delta) {
    const uint32_t id = static_cast<uint32_t>(sparse_morph_deltas_.size());
    sparse_morph_deltas_.push_back(p_delta);
    return id;
}

uint32_t ParametricCharacterMesh::_append_quad_ring(
    const glm::vec3 &p_center,
    const glm::vec3 &p_radii,
    AnatomicalPartitionId p_partition,
    uint8_t p_min_lod_level,
    uint64_t p_edge_ring_mask,
    uint32_t p_ring_index,
    const glm::vec3 &p_axis_u,
    const glm::vec3 &p_axis_v
) {
    static constexpr float CORNER_SIGNS[4][2] = {
        {-1.0f, -1.0f},
        { 1.0f, -1.0f},
        { 1.0f,  1.0f},
        {-1.0f,  1.0f}
    };

    const uint32_t start_idx = static_cast<uint32_t>(base_vertices_.size());
    for (size_t c = 0; c < 4; ++c) {
        const glm::vec3 local_offset =
            p_axis_u * (CORNER_SIGNS[c][0] * p_radii.x) +
            p_axis_v * (CORNER_SIGNS[c][1] * p_radii.z);

        QuadLoopVertexPod v{};
        v.rest_position  = p_center + local_offset;
        const float len  = glm::length(local_offset);
        v.rest_normal    = (len > 1e-6f) ? (local_offset / len) : glm::vec3(0.0f, 1.0f, 0.0f);
        v.partition_id   = static_cast<uint16_t>(p_partition);
        v.min_lod_level  = p_min_lod_level;
        v.ring_index     = p_ring_index;
        v.edge_ring_mask = p_edge_ring_mask;
        base_vertices_.push_back(v);
    }
    return start_idx;
}

void ParametricCharacterMesh::_bridge_quad_rings(uint32_t p_ring_a_start, uint32_t p_ring_b_start) {
    for (uint32_t i = 0; i < 4u; ++i) {
        const uint32_t next_i = (i + 1u) & 3u;
        base_quads_.emplace_back(
            p_ring_a_start + i,
            p_ring_a_start + next_i,
            p_ring_b_start + next_i,
            p_ring_b_start + i
        );
    }
}

void ParametricCharacterMesh::build_archetype_preset(CharacterBodyArchetype p_archetype) {
    clear();
    archetype_ = p_archetype;

    switch (p_archetype) {
        case CharacterBodyArchetype::BIPED: {
            // Pelvis -> Waist -> Chest -> Neck -> Head crown (Rings 0..5)
            const uint32_t r_pelvis   = _append_quad_ring({0.0f, 0.85f, 0.0f}, {0.20f, 0.0f, 0.13f}, PARTITION_PELVIS, 0, 1ULL << 0, 0);
            const uint32_t r_waist    = _append_quad_ring({0.0f, 1.05f, 0.0f}, {0.18f, 0.0f, 0.12f}, PARTITION_TORSO,  1, 1ULL << 1, 1);
            const uint32_t r_chest    = _append_quad_ring({0.0f, 1.35f, 0.0f}, {0.24f, 0.0f, 0.15f}, PARTITION_TORSO,  0, 1ULL << 2, 2);
            const uint32_t r_neck     = _append_quad_ring({0.0f, 1.50f, 0.0f}, {0.09f, 0.0f, 0.09f}, PARTITION_HEAD,   1, 1ULL << 3, 3);
            const uint32_t r_head_mid = _append_quad_ring({0.0f, 1.65f, 0.0f}, {0.13f, 0.0f, 0.14f}, PARTITION_HEAD,   0, 1ULL << 4, 4);
            const uint32_t r_head_top = _append_quad_ring({0.0f, 1.80f, 0.0f}, {0.10f, 0.0f, 0.11f}, PARTITION_HEAD,   2, 1ULL << 5, 5);

            _bridge_quad_rings(r_pelvis, r_waist);
            _bridge_quad_rings(r_waist, r_chest);
            _bridge_quad_rings(r_chest, r_neck);
            _bridge_quad_rings(r_neck, r_head_mid);
            _bridge_quad_rings(r_head_mid, r_head_top);

            // Upper limbs (Left & Right shoulder -> wrist) + 5 digit tips
            for (int side = -1; side <= 1; side += 2) {
                const float sx = static_cast<float>(side);
                const uint32_t r_shldr = _append_quad_ring({sx * 0.30f, 1.35f, 0.0f}, {0.07f, 0.0f, 0.07f}, PARTITION_UPPER_LIMB, 0, 1ULL << 6, 6);
                const uint32_t r_elbow = _append_quad_ring({sx * 0.52f, 1.15f, 0.0f}, {0.06f, 0.0f, 0.06f}, PARTITION_UPPER_LIMB, 1, 1ULL << 7, 7);
                const uint32_t r_wrist = _append_quad_ring({sx * 0.70f, 0.95f, 0.0f}, {0.05f, 0.0f, 0.04f}, PARTITION_UPPER_LIMB, 0, 1ULL << 8, 8);
                _bridge_quad_rings(r_shldr, r_elbow);
                _bridge_quad_rings(r_elbow, r_wrist);

                // Digits 0..4 (Thumb through Pinky) tagged at LOD 2..3
                for (uint16_t d = 0; d < 5u; ++d) {
                    const float z_off = (static_cast<float>(d) - 2.0f) * 0.025f;
                    const auto part = static_cast<AnatomicalPartitionId>(PARTITION_DIGIT_0 + d);
                    const uint32_t r_knuckle = _append_quad_ring(
                        {sx * 0.74f, 0.92f, z_off}, {0.015f, 0.0f, 0.015f}, part, 2, 1ULL << (9 + d), 9 + d
                    );
                    const uint32_t r_tip = _append_quad_ring(
                        {sx * 0.81f, 0.87f, z_off}, {0.012f, 0.0f, 0.012f}, part, 3, 1ULL << (14 + d), 14 + d
                    );
                    _bridge_quad_rings(r_knuckle, r_tip);
                }
            }

            // Lower limbs (Left & Right hip -> knee -> ankle)
            for (int side = -1; side <= 1; side += 2) {
                const float sx = static_cast<float>(side);
                const uint32_t r_hip   = _append_quad_ring({sx * 0.12f, 0.82f, 0.0f}, {0.09f, 0.0f, 0.09f}, PARTITION_LOWER_LIMB, 0, 1ULL << 19, 19);
                const uint32_t r_knee  = _append_quad_ring({sx * 0.12f, 0.45f, 0.0f}, {0.07f, 0.0f, 0.07f}, PARTITION_LOWER_LIMB, 1, 1ULL << 20, 20);
                const uint32_t r_ankle = _append_quad_ring({sx * 0.12f, 0.05f, 0.0f}, {0.06f, 0.0f, 0.08f}, PARTITION_LOWER_LIMB, 0, 1ULL << 21, 21);
                _bridge_quad_rings(r_hip, r_knee);
                _bridge_quad_rings(r_knee, r_ankle);
            }
            break;
        }

        case CharacterBodyArchetype::QUADRUPED: {
            const uint32_t r_pelvis = _append_quad_ring({0.0f, 0.70f, -0.45f}, {0.22f, 0.0f, 0.22f}, PARTITION_PELVIS, 0, 1ULL << 0, 0);
            const uint32_t r_belly  = _append_quad_ring({0.0f, 0.72f,  0.00f}, {0.25f, 0.0f, 0.25f}, PARTITION_TORSO,  1, 1ULL << 1, 1);
            const uint32_t r_chest  = _append_quad_ring({0.0f, 0.78f,  0.45f}, {0.24f, 0.0f, 0.24f}, PARTITION_TORSO,  0, 1ULL << 2, 2);
            const uint32_t r_head   = _append_quad_ring({0.0f, 1.02f,  0.75f}, {0.15f, 0.0f, 0.18f}, PARTITION_HEAD,   0, 1ULL << 3, 3);
            const uint32_t r_tail_0 = _append_quad_ring({0.0f, 0.75f, -0.70f}, {0.07f, 0.0f, 0.07f}, PARTITION_TAIL,   1, 1ULL << 4, 4);
            const uint32_t r_tail_1 = _append_quad_ring({0.0f, 0.65f, -1.05f}, {0.04f, 0.0f, 0.04f}, PARTITION_TAIL,   2, 1ULL << 5, 5);
            _bridge_quad_rings(r_tail_1, r_tail_0);
            _bridge_quad_rings(r_tail_0, r_pelvis);
            _bridge_quad_rings(r_pelvis, r_belly);
            _bridge_quad_rings(r_belly, r_chest);
            _bridge_quad_rings(r_chest, r_head);

            // Forelimbs and hindlimbs
            for (int side = -1; side <= 1; side += 2) {
                const float sx = static_cast<float>(side);
                const uint32_t f_top = _append_quad_ring({sx * 0.18f, 0.65f,  0.40f}, {0.07f, 0.0f, 0.07f}, PARTITION_UPPER_LIMB, 0, 1ULL << 6, 6);
                const uint32_t f_bot = _append_quad_ring({sx * 0.18f, 0.05f,  0.40f}, {0.05f, 0.0f, 0.05f}, PARTITION_UPPER_LIMB, 0, 1ULL << 7, 7);
                _bridge_quad_rings(f_top, f_bot);

                const uint32_t h_top = _append_quad_ring({sx * 0.18f, 0.65f, -0.40f}, {0.08f, 0.0f, 0.08f}, PARTITION_LOWER_LIMB, 0, 1ULL << 8, 8);
                const uint32_t h_bot = _append_quad_ring({sx * 0.18f, 0.05f, -0.40f}, {0.05f, 0.0f, 0.05f}, PARTITION_LOWER_LIMB, 0, 1ULL << 9, 9);
                _bridge_quad_rings(h_top, h_bot);
            }
            break;
        }

        case CharacterBodyArchetype::AVIAN: {
            const uint32_t r_tail  = _append_quad_ring({0.0f, 0.55f, -0.45f}, {0.14f, 0.0f, 0.05f}, PARTITION_TAIL,  1, 1ULL << 0, 0);
            const uint32_t r_pelv  = _append_quad_ring({0.0f, 0.60f, -0.15f}, {0.16f, 0.0f, 0.14f}, PARTITION_PELVIS,0, 1ULL << 1, 1);
            const uint32_t r_torso = _append_quad_ring({0.0f, 0.78f,  0.15f}, {0.18f, 0.0f, 0.16f}, PARTITION_TORSO, 0, 1ULL << 2, 2);
            const uint32_t r_head  = _append_quad_ring({0.0f, 1.00f,  0.30f}, {0.11f, 0.0f, 0.12f}, PARTITION_HEAD,  0, 1ULL << 3, 3);
            _bridge_quad_rings(r_tail, r_pelv);
            _bridge_quad_rings(r_pelv, r_torso);
            _bridge_quad_rings(r_torso, r_head);

            for (int side = -1; side <= 1; side += 2) {
                const float sx = static_cast<float>(side);
                const uint32_t w_root = _append_quad_ring({sx * 0.22f, 0.78f, 0.05f}, {0.06f, 0.0f, 0.14f}, PARTITION_WING, 0, 1ULL << 4, 4);
                const uint32_t w_tip  = _append_quad_ring({sx * 0.85f, 0.82f, 0.00f}, {0.03f, 0.0f, 0.08f}, PARTITION_WING, 1, 1ULL << 5, 5);
                _bridge_quad_rings(w_root, w_tip);

                const uint32_t l_top  = _append_quad_ring({sx * 0.10f, 0.50f, 0.0f}, {0.04f, 0.0f, 0.04f}, PARTITION_LOWER_LIMB, 0, 1ULL << 6, 6);
                const uint32_t l_bot  = _append_quad_ring({sx * 0.10f, 0.05f, 0.0f}, {0.04f, 0.0f, 0.06f}, PARTITION_LOWER_LIMB, 0, 1ULL << 7, 7);
                _bridge_quad_rings(l_top, l_bot);
            }
            break;
        }

        case CharacterBodyArchetype::FLYING_EYE: {
            const uint32_t r_front = _append_quad_ring({0.0f, 1.15f,  0.28f}, {0.16f, 0.0f, 0.16f}, PARTITION_HEAD, 0, 1ULL << 0, 0);
            const uint32_t r_mid   = _append_quad_ring({0.0f, 1.15f,  0.00f}, {0.32f, 0.0f, 0.32f}, PARTITION_HEAD, 0, 1ULL << 1, 1);
            const uint32_t r_back  = _append_quad_ring({0.0f, 1.15f, -0.26f}, {0.18f, 0.0f, 0.18f}, PARTITION_TORSO,1, 1ULL << 2, 2);
            const uint32_t r_nerve = _append_quad_ring({0.0f, 0.95f, -0.65f}, {0.05f, 0.0f, 0.05f}, PARTITION_TAIL, 2, 1ULL << 3, 3);
            _bridge_quad_rings(r_front, r_mid);
            _bridge_quad_rings(r_mid, r_back);
            _bridge_quad_rings(r_back, r_nerve);
            break;
        }

        case CharacterBodyArchetype::SERPENTINE: {
            uint32_t prev_ring = 0;
            for (uint32_t r = 0; r < 6u; ++r) {
                const float t = static_cast<float>(r) / 5.0f;
                const float radius = glm::mix(0.16f, 0.04f, t);
                const auto part = (r == 0u) ? PARTITION_HEAD : ((r < 3u) ? PARTITION_TORSO : PARTITION_TAIL);
                const uint8_t lod = (r % 2u == 1u) ? 1u : 0u;
                const uint32_t cur_ring = _append_quad_ring(
                    {0.0f, 0.20f + (1.0f - t) * 0.45f, 0.60f - t * 1.40f},
                    {radius, 0.0f, radius},
                    part,
                    lod,
                    1ULL << r,
                    r
                );
                if (r > 0u) {
                    _bridge_quad_rings(prev_ring, cur_ring);
                }
                prev_ring = cur_ring;
            }
            break;
        }

        case CharacterBodyArchetype::CUSTOM:
        default: {
            const uint32_t r0 = _append_quad_ring({0.0f, 0.0f, 0.0f}, {0.25f, 0.0f, 0.15f}, PARTITION_PELVIS, 0, 1ULL << 0, 0);
            const uint32_t r1 = _append_quad_ring({0.0f, 1.0f, 0.0f}, {0.25f, 0.0f, 0.15f}, PARTITION_TORSO,  0, 1ULL << 1, 1);
            _bridge_quad_rings(r0, r1);
            break;
        }
    }

    _build_default_sparse_morph_deltas();
}

void ParametricCharacterMesh::_build_default_sparse_morph_deltas() {
    sparse_morph_deltas_.clear();
    sparse_morph_deltas_.reserve(base_vertices_.size() * 2u);

    for (uint32_t i = 0; i < static_cast<uint32_t>(base_vertices_.size()); ++i) {
        const QuadLoopVertexPod &v = base_vertices_[i];

        // 0: MORPH_STATURE_HEIGHT — vertical stretch proportional to rest height
        if (std::abs(v.rest_position.y) > 1e-4f) {
            MorphDeltaPod d{};
            d.vertex_index   = i;
            d.morph_channel  = MORPH_STATURE_HEIGHT;
            d.position_delta = glm::vec3(0.0f, v.rest_position.y * 0.25f, 0.0f);
            sparse_morph_deltas_.push_back(d);
        }

        // Partition-specific anatomical morph deltas
        switch (v.partition_id) {
            case PARTITION_TORSO:
            case PARTITION_PELVIS: {
                MorphDeltaPod d{};
                d.vertex_index   = i;
                d.morph_channel  = MORPH_TORSO_BULK;
                d.position_delta = glm::vec3(v.rest_normal.x * 0.12f, 0.0f, v.rest_normal.z * 0.12f);
                d.normal_delta   = glm::vec3(v.rest_normal.x * 0.05f, 0.0f, v.rest_normal.z * 0.05f);
                sparse_morph_deltas_.push_back(d);
                break;
            }
            case PARTITION_HEAD: {
                MorphDeltaPod d{};
                d.vertex_index   = i;
                d.morph_channel  = MORPH_HEAD_SCALE;
                d.position_delta = v.rest_normal * 0.10f + glm::vec3(0.0f, 0.04f, 0.0f);
                sparse_morph_deltas_.push_back(d);
                break;
            }
            case PARTITION_UPPER_LIMB: {
                MorphDeltaPod d{};
                d.vertex_index   = i;
                d.morph_channel  = MORPH_UPPER_LIMB_REACH;
                const float sx   = (v.rest_position.x >= 0.0f) ? 1.0f : -1.0f;
                d.position_delta = glm::vec3(sx * 0.15f, -0.05f, 0.0f);
                sparse_morph_deltas_.push_back(d);
                break;
            }
            case PARTITION_LOWER_LIMB: {
                MorphDeltaPod d{};
                d.vertex_index   = i;
                d.morph_channel  = MORPH_LOWER_LIMB_STRIDE;
                d.position_delta = glm::vec3(v.rest_normal.x * 0.04f, -0.12f, v.rest_normal.z * 0.04f);
                sparse_morph_deltas_.push_back(d);
                break;
            }
            case PARTITION_DIGIT_0:
            case PARTITION_DIGIT_1:
            case PARTITION_DIGIT_2:
            case PARTITION_DIGIT_3:
            case PARTITION_DIGIT_4: {
                const uint32_t digit_idx = static_cast<uint32_t>(v.partition_id - PARTITION_DIGIT_0);
                MorphDeltaPod d{};
                d.vertex_index   = i;
                d.morph_channel  = MORPH_DIGIT_CURL_0 + digit_idx;
                const float inward_x = (v.rest_position.x >= 0.0f) ? -0.04f : 0.04f;
                d.position_delta = glm::vec3(inward_x, -0.05f, 0.02f);
                d.normal_delta   = glm::vec3(0.0f, -0.25f, 0.15f);
                sparse_morph_deltas_.push_back(d);
                break;
            }
            case PARTITION_TAIL: {
                MorphDeltaPod d{};
                d.vertex_index   = i;
                d.morph_channel  = MORPH_TAIL_LENGTH;
                d.position_delta = glm::vec3(0.0f, -0.04f, -0.25f);
                sparse_morph_deltas_.push_back(d);
                break;
            }
            case PARTITION_WING: {
                MorphDeltaPod d{};
                d.vertex_index   = i;
                d.morph_channel  = MORPH_WING_SPAN;
                const float sx   = (v.rest_position.x >= 0.0f) ? 1.0f : -1.0f;
                d.position_delta = glm::vec3(sx * 0.35f, 0.05f, 0.08f);
                sparse_morph_deltas_.push_back(d);
                break;
            }
            default:
                break;
        }
    }
}

uint32_t ParametricCharacterMesh::evaluate_quad_loop_lod(
    std::span<const QuadLoopVertexPod> p_vertices,
    std::span<const QuadFacePod> p_quads,
    uint8_t p_target_lod_level,
    uint64_t p_active_ring_mask,
    glm::vec3 *r_out_positions,
    glm::vec3 *r_out_normals,
    uint32_t *r_out_active_masks,
    size_t p_out_capacity
) noexcept {
    const size_t base_count = std::min<size_t>(p_vertices.size(), p_out_capacity);
    uint32_t active_count   = 0u;

    for (size_t i = 0; i < base_count; ++i) {
        const QuadLoopVertexPod &v = p_vertices[i];
        const bool ring_enabled = (p_active_ring_mask == 0ULL) || ((v.edge_ring_mask & p_active_ring_mask) != 0ULL);
        const bool lod_active   = (v.min_lod_level <= p_target_lod_level) && ring_enabled;

        if (r_out_positions) {
            r_out_positions[i] = v.rest_position;
        }
        if (r_out_normals) {
            r_out_normals[i] = v.rest_normal;
        }
        if (r_out_active_masks) {
            r_out_active_masks[i] = lod_active ? (1u | (static_cast<uint32_t>(v.partition_id) << 8u)) : 0u;
        }
        if (lod_active) {
            ++active_count;
        }
    }

    // If target LOD is subdivided (>= 2) and output buffer has extra capacity beyond base_vertices,
    // append deterministic quad-centroid subdivision vertices for active quads.
    size_t write_cursor = base_count;
    if (p_target_lod_level >= 2u && write_cursor < p_out_capacity && !p_quads.empty()) {
        for (const auto &q : p_quads) {
            if (write_cursor >= p_out_capacity) break;
            if (q[0] >= p_vertices.size() || q[1] >= p_vertices.size() ||
                q[2] >= p_vertices.size() || q[3] >= p_vertices.size()) {
                continue;
            }
            const auto &v0 = p_vertices[q[0]];
            const auto &v1 = p_vertices[q[1]];
            const auto &v2 = p_vertices[q[2]];
            const auto &v3 = p_vertices[q[3]];
            const uint8_t q_lod = std::max({v0.min_lod_level, v1.min_lod_level, v2.min_lod_level, v3.min_lod_level});
            if (q_lod > p_target_lod_level) continue;

            const glm::vec3 centroid_pos = 0.25f * (v0.rest_position + v1.rest_position + v2.rest_position + v3.rest_position);
            const glm::vec3 raw_nrm      = v0.rest_normal + v1.rest_normal + v2.rest_normal + v3.rest_normal;
            const float nlen             = glm::length(raw_nrm);
            const glm::vec3 centroid_nrm = (nlen > 1e-6f) ? (raw_nrm / nlen) : glm::vec3(0.0f, 1.0f, 0.0f);

            if (r_out_positions)    r_out_positions[write_cursor]    = centroid_pos;
            if (r_out_normals)      r_out_normals[write_cursor]      = centroid_nrm;
            if (r_out_active_masks) r_out_active_masks[write_cursor] = 1u | (static_cast<uint32_t>(v0.partition_id) << 8u);
            ++write_cursor;
            ++active_count;
        }
    }

    return active_count;
}

void ParametricCharacterMesh::evaluate_sparse_morph_sliders(
    std::span<const QuadLoopVertexPod> p_vertices,
    std::span<const MorphDeltaPod> p_deltas,
    std::span<const float> p_channel_weights,
    uint32_t p_active_partition_mask,
    glm::vec3 *r_positions,
    glm::vec3 *r_normals,
    size_t p_vertex_count
) noexcept {
    if (!r_positions || p_vertex_count == 0u) return;

    const size_t v_limit = std::min<size_t>(p_vertices.size(), p_vertex_count);
    for (const MorphDeltaPod &delta : p_deltas) {
        if (delta.vertex_index >= v_limit) continue;
        if (delta.morph_channel >= p_channel_weights.size()) continue;

        const float w = p_channel_weights[delta.morph_channel];
        if (std::abs(w) <= 1e-6f) continue;

        const uint16_t part_id = p_vertices[delta.vertex_index].partition_id;
        if (part_id < 32u && ((p_active_partition_mask & (1u << part_id)) == 0u)) {
            continue;
        }

        r_positions[delta.vertex_index] += delta.position_delta * w;
        if (r_normals) {
            r_normals[delta.vertex_index] += delta.normal_delta * w;
        }
    }

    if (r_normals) {
        for (size_t i = 0; i < v_limit; ++i) {
            const float len = glm::length(r_normals[i]);
            if (len > 1e-6f) {
                r_normals[i] /= len;
            } else {
                r_normals[i] = glm::vec3(0.0f, 1.0f, 0.0f);
            }
        }
    }
}

glm::vec3 ParametricCharacterMesh::evaluate_garment_surface_point_on_buffers(
    const SurfaceGarmentBindPod &p_bind,
    std::span<const QuadFacePod> p_quads,
    std::span<const glm::vec3> p_deformed_positions,
    std::span<const glm::vec3> p_deformed_normals,
    float p_extra_normal_offset,
    glm::vec3 *r_out_surface_normal
) noexcept {
    if (p_quads.empty() || p_deformed_positions.empty()) {
        if (r_out_surface_normal) *r_out_surface_normal = glm::vec3(0.0f, 1.0f, 0.0f);
        return glm::vec3(0.0f);
    }

    const uint32_t qid = std::min<uint32_t>(p_bind.quad_id, static_cast<uint32_t>(p_quads.size() - 1u));
    const QuadFacePod &q = p_quads[qid];
    const size_t n_verts = p_deformed_positions.size();

    const uint32_t i00 = std::min<uint32_t>(q[0], static_cast<uint32_t>(n_verts - 1u));
    const uint32_t i10 = std::min<uint32_t>(q[1], static_cast<uint32_t>(n_verts - 1u));
    const uint32_t i11 = std::min<uint32_t>(q[2], static_cast<uint32_t>(n_verts - 1u));
    const uint32_t i01 = std::min<uint32_t>(q[3], static_cast<uint32_t>(n_verts - 1u));

    const float u = std::clamp(p_bind.u, 0.0f, 1.0f);
    const float v = std::clamp(p_bind.v, 0.0f, 1.0f);

    const glm::vec3 &p00 = p_deformed_positions[i00];
    const glm::vec3 &p10 = p_deformed_positions[i10];
    const glm::vec3 &p11 = p_deformed_positions[i11];
    const glm::vec3 &p01 = p_deformed_positions[i01];

    const glm::vec3 pos = glm::mix(glm::mix(p00, p10, u), glm::mix(p01, p11, u), v);

    glm::vec3 nrm(0.0f, 1.0f, 0.0f);
    if (p_deformed_normals.size() >= n_verts) {
        const glm::vec3 interp_n = glm::mix(
            glm::mix(p_deformed_normals[i00], p_deformed_normals[i10], u),
            glm::mix(p_deformed_normals[i01], p_deformed_normals[i11], u),
            v
        );
        const float n_len = glm::length(interp_n);
        if (n_len > 1e-6f) {
            nrm = interp_n / n_len;
        }
    } else {
        const glm::vec3 cross_n = glm::cross(p10 - p00, p01 - p00);
        const float c_len       = glm::length(cross_n);
        if (c_len > 1e-6f) {
            nrm = cross_n / c_len;
        }
    }

    if (r_out_surface_normal) {
        *r_out_surface_normal = nrm;
    }
    return pos + nrm * (p_bind.d_normal + p_extra_normal_offset);
}

glm::vec3 ParametricCharacterMesh::evaluate_garment_surface_point(
    const SurfaceGarmentBindPod &p_bind
) const noexcept {
    if (base_quads_.empty() || base_vertices_.empty()) {
        return glm::vec3(0.0f);
    }
    const uint32_t qid = std::min<uint32_t>(p_bind.quad_id, static_cast<uint32_t>(base_quads_.size() - 1u));
    const auto &q = base_quads_[qid];
    const glm::vec3 &p00 = base_vertices_[q[0]].rest_position;
    const glm::vec3 &p10 = base_vertices_[q[1]].rest_position;
    const glm::vec3 &p11 = base_vertices_[q[2]].rest_position;
    const glm::vec3 &p01 = base_vertices_[q[3]].rest_position;

    const float u = std::clamp(p_bind.u, 0.0f, 1.0f);
    const float v = std::clamp(p_bind.v, 0.0f, 1.0f);

    const glm::vec3 pos = glm::mix(glm::mix(p00, p10, u), glm::mix(p01, p11, u), v);
    const glm::vec3 interp_n = glm::mix(
        glm::mix(base_vertices_[q[0]].rest_normal, base_vertices_[q[1]].rest_normal, u),
        glm::mix(base_vertices_[q[3]].rest_normal, base_vertices_[q[2]].rest_normal, u),
        v
    );
    const float n_len = glm::length(interp_n);
    glm::vec3 nrm = (n_len > 1e-6f) ? (interp_n / n_len) : glm::vec3(0.0f, 1.0f, 0.0f);
    return pos + nrm * p_bind.d_normal;
}

void ParametricCharacterMesh::evaluate_garment_surface_batch(
    std::span<const SurfaceGarmentBindPod> p_binds,
    std::span<const QuadFacePod> p_quads,
    std::span<const glm::vec3> p_deformed_positions,
    std::span<const glm::vec3> p_deformed_normals,
    float p_extra_normal_offset,
    glm::vec3 *r_out_garment_positions,
    size_t p_out_capacity
) noexcept {
    if (!r_out_garment_positions || p_out_capacity == 0u) return;
    const size_t count = std::min<size_t>(p_binds.size(), p_out_capacity);
    for (size_t i = 0; i < count; ++i) {
        r_out_garment_positions[i] = evaluate_garment_surface_point_on_buffers(
            p_binds[i],
            p_quads,
            p_deformed_positions,
            p_deformed_normals,
            p_extra_normal_offset,
            nullptr
        );
    }
}

void ParametricCharacterMesh::generate_default_garment_bindings(
    std::vector<SurfaceGarmentBindPod> &r_out_binds,
    uint32_t p_partition_mask,
    float p_normal_offset,
    uint32_t p_substeps_per_quad_axis
) const {
    r_out_binds.clear();
    const uint32_t steps = std::clamp<uint32_t>(p_substeps_per_quad_axis, 1u, 8u);

    for (uint32_t qid = 0; qid < static_cast<uint32_t>(base_quads_.size()); ++qid) {
        const QuadFacePod &q = base_quads_[qid];
        if (q[0] >= base_vertices_.size()) continue;
        const uint16_t part_id = base_vertices_[q[0]].partition_id;
        if (part_id < 32u && ((p_partition_mask & (1u << part_id)) == 0u)) {
            continue;
        }

        for (uint32_t iv = 0; iv <= steps; ++iv) {
            const float v = static_cast<float>(iv) / static_cast<float>(steps);
            for (uint32_t iu = 0; iu <= steps; ++iu) {
                const float u = static_cast<float>(iu) / static_cast<float>(steps);
                SurfaceGarmentBindPod bind{};
                bind.quad_id  = qid;
                bind.u        = u;
                bind.v        = v;
                bind.d_normal = p_normal_offset;
                r_out_binds.push_back(bind);
            }
        }
    }
}

} // namespace ideam::core
