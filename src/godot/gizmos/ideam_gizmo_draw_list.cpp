#include "ideam_gizmo_draw_list.h"

#include <algorithm>
#include <cmath>

namespace ideam::godot_ext {

void IdeamGizmoDrawList::clear() {
    line_batches_.clear();
    meshes_.clear();
    handles_.clear();
    labels_.clear();
    current_depth_mode_ = DEPTH_TEST_OVERLAY;
}

void IdeamGizmoDrawList::add_lines(
    const godot::PackedVector3Array &p_pairs,
    GizmoSemanticRole p_role,
    uint32_t p_state_flags
) {
    if (p_pairs.size() < 2) {
        return;
    }
    // Coalesce into existing batch if role, state, and depth mode match for minimal draw calls.
    for (auto &batch : line_batches_) {
        if (batch.role == p_role && batch.state_flags == p_state_flags && batch.depth_test == current_depth_mode_) {
            const int64_t old_sz = batch.points.size();
            const int64_t add_sz = p_pairs.size() - (p_pairs.size() % 2);
            batch.points.resize(old_sz + add_sz);
            for (int64_t i = 0; i < add_sz; ++i) {
                batch.points.set(old_sz + i, p_pairs[i]);
            }
            return;
        }
    }
    GizmoLineBatchCommand cmd{};
    cmd.role        = p_role;
    cmd.state_flags = p_state_flags;
    cmd.depth_test  = current_depth_mode_;
    const int64_t count = p_pairs.size() - (p_pairs.size() % 2);
    cmd.points.resize(count);
    for (int64_t i = 0; i < count; ++i) {
        cmd.points.set(i, p_pairs[i]);
    }
    line_batches_.push_back(std::move(cmd));
}

void IdeamGizmoDrawList::add_segment(
    const godot::Vector3 &p_a,
    const godot::Vector3 &p_b,
    GizmoSemanticRole p_role,
    uint32_t p_state_flags
) {
    godot::PackedVector3Array seg;
    seg.resize(2);
    seg.set(0, p_a);
    seg.set(1, p_b);
    add_lines(seg, p_role, p_state_flags);
}

void IdeamGizmoDrawList::add_polyline(
    const godot::PackedVector3Array &p_points,
    bool p_closed,
    GizmoSemanticRole p_role,
    uint32_t p_state_flags
) {
    const int64_t n = p_points.size();
    if (n < 2) {
        return;
    }
    const int64_t segs = p_closed ? n : (n - 1);
    godot::PackedVector3Array pairs;
    pairs.resize(segs * 2);
    for (int64_t i = 0; i < segs; ++i) {
        pairs.set(i * 2, p_points[i]);
        pairs.set(i * 2 + 1, p_points[(i + 1) % n]);
    }
    add_lines(pairs, p_role, p_state_flags);
}

void IdeamGizmoDrawList::add_circle(
    const godot::Vector3 &p_center,
    const godot::Vector3 &p_normal,
    real_t p_radius,
    int p_segments,
    GizmoSemanticRole p_role,
    uint32_t p_state_flags
) {
    const int segs = std::clamp(p_segments, 6, 96);
    const real_t r = std::max(static_cast<real_t>(1e-4), p_radius);
    const godot::Vector3 n = p_normal.length_squared() > 1e-8f
        ? p_normal.normalized()
        : godot::Vector3(0.0f, 1.0f, 0.0f);

    const godot::Vector3 ref_axis = (std::abs(n.y) < 0.95f)
        ? godot::Vector3(0.0f, 1.0f, 0.0f)
        : godot::Vector3(1.0f, 0.0f, 0.0f);
    const godot::Vector3 u = n.cross(ref_axis).normalized();
    const godot::Vector3 v = n.cross(u).normalized();

    godot::PackedVector3Array ring;
    ring.resize(segs);
    constexpr float kTwoPi = 6.28318530717958647692f;
    for (int i = 0; i < segs; ++i) {
        const float a = (static_cast<float>(i) / static_cast<float>(segs)) * kTwoPi;
        ring.set(i, p_center + (u * std::cos(a) + v * std::sin(a)) * static_cast<float>(r));
    }
    add_polyline(ring, true, p_role, p_state_flags);
}

void IdeamGizmoDrawList::add_wire_box(
    const godot::Vector3 &p_center,
    const godot::Vector3 &p_half_extents,
    const godot::Basis &p_basis,
    GizmoSemanticRole p_role,
    uint32_t p_state_flags
) {
    const godot::Vector3 e(
        std::max(static_cast<real_t>(1e-4), std::abs(p_half_extents.x)),
        std::max(static_cast<real_t>(1e-4), std::abs(p_half_extents.y)),
        std::max(static_cast<real_t>(1e-4), std::abs(p_half_extents.z))
    );

    godot::Vector3 c[8];
    for (int i = 0; i < 8; ++i) {
        const godot::Vector3 local(
            (i & 1) ? e.x : -e.x,
            (i & 2) ? e.y : -e.y,
            (i & 4) ? e.z : -e.z
        );
        c[i] = p_center + p_basis.xform(local);
    }

    static constexpr int kEdges[12][2] = {
        {0, 1}, {2, 3}, {4, 5}, {6, 7},
        {0, 2}, {1, 3}, {4, 6}, {5, 7},
        {0, 4}, {1, 5}, {2, 6}, {3, 7}
    };

    godot::PackedVector3Array pairs;
    pairs.resize(24);
    for (int i = 0; i < 12; ++i) {
        pairs.set(i * 2, c[kEdges[i][0]]);
        pairs.set(i * 2 + 1, c[kEdges[i][1]]);
    }
    add_lines(pairs, p_role, p_state_flags);
}

void IdeamGizmoDrawList::add_cone_arrow(
    const godot::Vector3 &p_origin,
    const godot::Vector3 &p_direction,
    real_t p_length,
    real_t p_head_radius,
    GizmoSemanticRole p_role,
    uint32_t p_state_flags
) {
    const godot::Vector3 dir = p_direction.length_squared() > 1e-8f
        ? p_direction.normalized()
        : godot::Vector3(0.0f, 1.0f, 0.0f);
    const real_t len = std::max(static_cast<real_t>(1e-3), p_length);
    const real_t hr  = std::max(static_cast<real_t>(1e-4), p_head_radius);
    const real_t head_len = std::min(len * static_cast<real_t>(0.35), hr * static_cast<real_t>(2.5));

    const godot::Vector3 tip       = p_origin + dir * len;
    const godot::Vector3 head_base = tip - dir * head_len;

    const godot::Vector3 ref_axis = (std::abs(dir.y) < 0.95f)
        ? godot::Vector3(0.0f, 1.0f, 0.0f)
        : godot::Vector3(1.0f, 0.0f, 0.0f);
    const godot::Vector3 u = dir.cross(ref_axis).normalized();
    const godot::Vector3 v = dir.cross(u).normalized();

    constexpr int kSpokes = 6;
    constexpr float kTwoPi = 6.28318530717958647692f;
    godot::PackedVector3Array pairs;
    pairs.resize((1 + kSpokes * 2) * 2);
    int idx = 0;
    pairs.set(idx++, p_origin);
    pairs.set(idx++, tip);

    godot::Vector3 rim[kSpokes];
    for (int i = 0; i < kSpokes; ++i) {
        const float a = (static_cast<float>(i) / static_cast<float>(kSpokes)) * kTwoPi;
        rim[i] = head_base + (u * std::cos(a) + v * std::sin(a)) * static_cast<float>(hr);
    }
    for (int i = 0; i < kSpokes; ++i) {
        pairs.set(idx++, tip);
        pairs.set(idx++, rim[i]);
        pairs.set(idx++, rim[i]);
        pairs.set(idx++, rim[(i + 1) % kSpokes]);
    }
    add_lines(pairs, p_role, p_state_flags);
}

void IdeamGizmoDrawList::add_mesh(
    const godot::Ref<godot::Mesh> &p_mesh,
    const godot::Transform3D &p_transform,
    GizmoSemanticRole p_role,
    uint32_t p_state_flags
) {
    if (p_mesh.is_null()) {
        return;
    }
    GizmoMeshCommand cmd{};
    cmd.mesh        = p_mesh;
    cmd.transform   = p_transform;
    cmd.role        = p_role;
    cmd.state_flags = p_state_flags;
    cmd.depth_test  = current_depth_mode_;
    meshes_.push_back(std::move(cmd));
}

void IdeamGizmoDrawList::add_handle(
    int32_t p_id,
    const godot::Vector3 &p_position,
    GizmoSemanticRole p_role,
    GizmoHandleShapeHint p_shape,
    real_t p_radius,
    uint32_t p_state_flags,
    const godot::String &p_label,
    bool p_secondary
) {
    GizmoHandleCommand cmd{};
    cmd.id          = p_id;
    cmd.position    = p_position;
    cmd.role        = p_role;
    cmd.shape       = p_shape;
    cmd.radius      = std::max(static_cast<real_t>(0.005), p_radius);
    cmd.state_flags = p_state_flags;
    cmd.secondary   = p_secondary;
    cmd.label       = p_label;
    cmd.depth_test  = current_depth_mode_;
    handles_.push_back(std::move(cmd));
}

void IdeamGizmoDrawList::add_label_3d(
    const godot::Vector3 &p_position,
    const godot::String &p_text,
    GizmoSemanticRole p_role,
    uint32_t p_state_flags
) {
    if (p_text.is_empty()) {
        return;
    }
    GizmoLabel3DCommand cmd{};
    cmd.position    = p_position;
    cmd.text        = p_text;
    cmd.role        = p_role;
    cmd.state_flags = p_state_flags;
    labels_.push_back(std::move(cmd));
}

} // namespace ideam::godot_ext
