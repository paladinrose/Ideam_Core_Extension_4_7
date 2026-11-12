#include "hyperbolic_poincare_tiling_resource.h"
#include "ideam_tiles_plugin.h"

#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/theme_db.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <algorithm>
#include <cmath>

namespace ideam::godot_ext {

// ============================================================================
// HyperbolicPoincareTilingResource
// ============================================================================

HyperbolicPoincareTilingResource::HyperbolicPoincareTilingResource() {
    is_dirty_ = true;
}

HyperbolicPoincareTilingResource::~HyperbolicPoincareTilingResource() = default;

void HyperbolicPoincareTilingResource::_ensure_built() const {
    if (!is_dirty_) {
        return;
    }
    core_hyperbolic_.configure(
        schlafli_p_,
        schlafli_q_,
        generation_rings_,
        arc_subdivisions_,
        _to_glm(mobius_focus_)
    );
    core_hyperbolic_.build_poincare_region(core_region_);
    is_dirty_ = false;
}

void HyperbolicPoincareTilingResource::_bind_methods() {
    using namespace godot;

    BIND_ENUM_CONSTANT(HYPERBOLIC_PRESET_CUSTOM);
    BIND_ENUM_CONSTANT(HYPERBOLIC_PRESET_HEPTAGON_7_3);
    BIND_ENUM_CONSTANT(HYPERBOLIC_PRESET_PENTAGON_5_4);
    BIND_ENUM_CONSTANT(HYPERBOLIC_PRESET_SQUARE_4_5);
    BIND_ENUM_CONSTANT(HYPERBOLIC_PRESET_TRIANGLE_3_7);
    BIND_ENUM_CONSTANT(HYPERBOLIC_PRESET_HEXAGON_6_4);
    BIND_ENUM_CONSTANT(HYPERBOLIC_PRESET_OCTAGON_8_3);

    ClassDB::bind_method(D_METHOD("set_schlafli_preset", "preset"), &HyperbolicPoincareTilingResource::set_schlafli_preset);
    ClassDB::bind_method(D_METHOD("get_schlafli_preset"), &HyperbolicPoincareTilingResource::get_schlafli_preset);
    ADD_PROPERTY(
        PropertyInfo(
            Variant::INT,
            "schlafli_preset",
            PROPERTY_HINT_ENUM,
            "Custom {p,q},{7,3} Heptagonal,{5,4} Pentagonal,{4,5} Square,{3,7} Triangular,{6,4} Hexagonal,{8,3} Octagonal"
        ),
        "set_schlafli_preset",
        "get_schlafli_preset"
    );

    ClassDB::bind_method(D_METHOD("set_schlafli_p", "p"), &HyperbolicPoincareTilingResource::set_schlafli_p);
    ClassDB::bind_method(D_METHOD("get_schlafli_p"), &HyperbolicPoincareTilingResource::get_schlafli_p);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "schlafli_p", PROPERTY_HINT_RANGE, "3,16,1"),
        "set_schlafli_p",
        "get_schlafli_p"
    );

    ClassDB::bind_method(D_METHOD("set_schlafli_q", "q"), &HyperbolicPoincareTilingResource::set_schlafli_q);
    ClassDB::bind_method(D_METHOD("get_schlafli_q"), &HyperbolicPoincareTilingResource::get_schlafli_q);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "schlafli_q", PROPERTY_HINT_RANGE, "3,16,1"),
        "set_schlafli_q",
        "get_schlafli_q"
    );

    ClassDB::bind_method(D_METHOD("set_generation_rings", "rings"), &HyperbolicPoincareTilingResource::set_generation_rings);
    ClassDB::bind_method(D_METHOD("get_generation_rings"), &HyperbolicPoincareTilingResource::get_generation_rings);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "generation_rings", PROPERTY_HINT_RANGE, "0,6,1"),
        "set_generation_rings",
        "get_generation_rings"
    );

    ClassDB::bind_method(D_METHOD("set_arc_subdivisions", "subdivisions"), &HyperbolicPoincareTilingResource::set_arc_subdivisions);
    ClassDB::bind_method(D_METHOD("get_arc_subdivisions"), &HyperbolicPoincareTilingResource::get_arc_subdivisions);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "arc_subdivisions", PROPERTY_HINT_RANGE, "1,48,1"),
        "set_arc_subdivisions",
        "get_arc_subdivisions"
    );

    ClassDB::bind_method(D_METHOD("set_mobius_focus", "focus"), &HyperbolicPoincareTilingResource::set_mobius_focus);
    ClassDB::bind_method(D_METHOD("get_mobius_focus"), &HyperbolicPoincareTilingResource::get_mobius_focus);
    ADD_PROPERTY(
        PropertyInfo(Variant::VECTOR2, "mobius_focus"),
        "set_mobius_focus",
        "get_mobius_focus"
    );

    ClassDB::bind_method(D_METHOD("is_valid_hyperbolic_symbol"), &HyperbolicPoincareTilingResource::is_valid_hyperbolic_symbol);
    ClassDB::bind_method(D_METHOD("get_central_polygon_euclidean_radius"), &HyperbolicPoincareTilingResource::get_central_polygon_euclidean_radius);
    ClassDB::bind_method(D_METHOD("rebuild"), &HyperbolicPoincareTilingResource::rebuild);

    ClassDB::bind_method(D_METHOD("commit_set_schlafli_symbol", "p", "q"), &HyperbolicPoincareTilingResource::commit_set_schlafli_symbol);
    ClassDB::bind_method(D_METHOD("commit_set_generation_rings", "rings"), &HyperbolicPoincareTilingResource::commit_set_generation_rings);
    ClassDB::bind_method(D_METHOD("commit_set_mobius_focus", "focus"), &HyperbolicPoincareTilingResource::commit_set_mobius_focus);

    ClassDB::bind_method(D_METHOD("get_cell_count"), &HyperbolicPoincareTilingResource::get_cell_count);
    ClassDB::bind_method(D_METHOD("get_edge_count"), &HyperbolicPoincareTilingResource::get_edge_count);
    ClassDB::bind_method(D_METHOD("get_cell_polygon", "cell_id"), &HyperbolicPoincareTilingResource::get_cell_polygon);
    ClassDB::bind_method(D_METHOD("get_cell_center", "cell_id"), &HyperbolicPoincareTilingResource::get_cell_center);
    ClassDB::bind_method(D_METHOD("get_cell_generation", "cell_id"), &HyperbolicPoincareTilingResource::get_cell_generation);
    ClassDB::bind_method(D_METHOD("get_cell_colour", "cell_id"), &HyperbolicPoincareTilingResource::get_cell_colour);
    ClassDB::bind_method(D_METHOD("get_cell_neighbors", "cell_id"), &HyperbolicPoincareTilingResource::get_cell_neighbors);

    ClassDB::bind_method(
        D_METHOD("sample_geodesic_arc", "z1", "z2", "subdivisions"),
        &HyperbolicPoincareTilingResource::sample_geodesic_arc,
        DEFVAL(16)
    );
    ClassDB::bind_method(
        D_METHOD("compute_hyperbolic_distance", "z1", "z2"),
        &HyperbolicPoincareTilingResource::compute_hyperbolic_distance
    );

    ClassDB::bind_method(D_METHOD("contains_point", "disk_point"), &HyperbolicPoincareTilingResource::contains_point);
    ClassDB::bind_method(
        D_METHOD("find_hyperbolic_path_cells", "start_cell", "goal_cell"),
        &HyperbolicPoincareTilingResource::find_hyperbolic_path_cells
    );
    ClassDB::bind_method(
        D_METHOD("find_hyperbolic_path_points", "start_cell", "goal_cell", "arc_subdivisions"),
        &HyperbolicPoincareTilingResource::find_hyperbolic_path_points,
        DEFVAL(12)
    );
    ClassDB::bind_method(
        D_METHOD("compute_hyperbolic_flow_field", "goal_cells"),
        &HyperbolicPoincareTilingResource::compute_hyperbolic_flow_field
    );
}

godot::PackedVector2Array HyperbolicPoincareTilingResource::get_prototile_outline(int p_subdivisions) const {
    _ensure_built();
    if (core_region_.cell_count() == 0) {
        return {};
    }
    return get_cell_polygon(0);
}

godot::PackedInt32Array HyperbolicPoincareTilingResource::get_prototile_triangles(int p_subdivisions) const {
    const godot::PackedVector2Array poly = get_prototile_outline(p_subdivisions);
    godot::PackedInt32Array tris;
    const int n = static_cast<int>(poly.size());
    if (n < 3) {
        return tris;
    }
    tris.resize((n - 2) * 3);
    int w = 0;
    for (int i = 1; i + 1 < n; ++i) {
        tris.set(w++, 0);
        tris.set(w++, i);
        tris.set(w++, i + 1);
    }
    return tris;
}

godot::Dictionary HyperbolicPoincareTilingResource::get_snapshot_state() const {
    godot::Dictionary d;
    d["schlafli_p"]       = schlafli_p_;
    d["schlafli_q"]       = schlafli_q_;
    d["generation_rings"] = generation_rings_;
    d["arc_subdivisions"] = arc_subdivisions_;
    d["mobius_focus"]     = mobius_focus_;
    return d;
}

void HyperbolicPoincareTilingResource::apply_snapshot_state(const godot::Dictionary &p_state) {
    if (p_state.has("schlafli_p")) {
        schlafli_p_ = static_cast<int>(p_state["schlafli_p"]);
    }
    if (p_state.has("schlafli_q")) {
        schlafli_q_ = static_cast<int>(p_state["schlafli_q"]);
    }
    if (p_state.has("generation_rings")) {
        generation_rings_ = std::clamp(static_cast<int>(p_state["generation_rings"]), 0, 6);
    }
    if (p_state.has("arc_subdivisions")) {
        arc_subdivisions_ = std::clamp(static_cast<int>(p_state["arc_subdivisions"]), 1, 48);
    }
    if (p_state.has("mobius_focus")) {
        mobius_focus_ = p_state["mobius_focus"];
    }
    is_dirty_ = true;
    emit_changed();
}

void HyperbolicPoincareTilingResource::set_schlafli_p(int p_p) {
    p_p = std::clamp(p_p, 3, 16);
    int next_q = schlafli_q_;
    if ((p_p - 2) * (next_q - 2) <= 4) {
        while (next_q < 16 && (p_p - 2) * (next_q - 2) <= 4) {
            ++next_q;
        }
    }
    if (schlafli_p_ == p_p && schlafli_q_ == next_q) {
        return;
    }
    schlafli_p_ = p_p;
    schlafli_q_ = next_q;
    is_dirty_   = true;
    emit_changed();
}

void HyperbolicPoincareTilingResource::set_schlafli_q(int p_q) {
    p_q = std::clamp(p_q, 3, 16);
    int next_p = schlafli_p_;
    if ((next_p - 2) * (p_q - 2) <= 4) {
        while (next_p < 16 && (next_p - 2) * (p_q - 2) <= 4) {
            ++next_p;
        }
    }
    if (schlafli_p_ == next_p && schlafli_q_ == p_q) {
        return;
    }
    schlafli_p_ = next_p;
    schlafli_q_ = p_q;
    is_dirty_   = true;
    emit_changed();
}

void HyperbolicPoincareTilingResource::set_schlafli_preset(HyperbolicSchlafliPreset p_preset) {
    int p = schlafli_p_;
    int q = schlafli_q_;
    switch (p_preset) {
        case HYPERBOLIC_PRESET_HEPTAGON_7_3: p = 7; q = 3; break;
        case HYPERBOLIC_PRESET_PENTAGON_5_4: p = 5; q = 4; break;
        case HYPERBOLIC_PRESET_SQUARE_4_5:   p = 4; q = 5; break;
        case HYPERBOLIC_PRESET_TRIANGLE_3_7: p = 3; q = 7; break;
        case HYPERBOLIC_PRESET_HEXAGON_6_4:  p = 6; q = 4; break;
        case HYPERBOLIC_PRESET_OCTAGON_8_3:  p = 8; q = 3; break;
        case HYPERBOLIC_PRESET_CUSTOM:
        default:
            return;
    }
    if (schlafli_p_ == p && schlafli_q_ == q) {
        return;
    }
    schlafli_p_ = p;
    schlafli_q_ = q;
    is_dirty_   = true;
    emit_changed();
}

HyperbolicSchlafliPreset HyperbolicPoincareTilingResource::get_schlafli_preset() const {
    if (schlafli_p_ == 7 && schlafli_q_ == 3) return HYPERBOLIC_PRESET_HEPTAGON_7_3;
    if (schlafli_p_ == 5 && schlafli_q_ == 4) return HYPERBOLIC_PRESET_PENTAGON_5_4;
    if (schlafli_p_ == 4 && schlafli_q_ == 5) return HYPERBOLIC_PRESET_SQUARE_4_5;
    if (schlafli_p_ == 3 && schlafli_q_ == 7) return HYPERBOLIC_PRESET_TRIANGLE_3_7;
    if (schlafli_p_ == 6 && schlafli_q_ == 4) return HYPERBOLIC_PRESET_HEXAGON_6_4;
    if (schlafli_p_ == 8 && schlafli_q_ == 3) return HYPERBOLIC_PRESET_OCTAGON_8_3;
    return HYPERBOLIC_PRESET_CUSTOM;
}

void HyperbolicPoincareTilingResource::set_generation_rings(int p_rings) {
    p_rings = std::clamp(p_rings, 0, 6);
    if (generation_rings_ == p_rings) {
        return;
    }
    generation_rings_ = p_rings;
    is_dirty_         = true;
    emit_changed();
}

void HyperbolicPoincareTilingResource::set_arc_subdivisions(int p_subdivs) {
    p_subdivs = std::clamp(p_subdivs, 1, 48);
    if (arc_subdivisions_ == p_subdivs) {
        return;
    }
    arc_subdivisions_ = p_subdivs;
    is_dirty_         = true;
    emit_changed();
}

void HyperbolicPoincareTilingResource::set_mobius_focus(const godot::Vector2 &p_focus) {
    godot::Vector2 clamped = p_focus;
    const real_t len = clamped.length();
    if (len >= static_cast<real_t>(0.94)) {
        clamped = clamped * (static_cast<real_t>(0.94) / len);
    }
    if (mobius_focus_.is_equal_approx(clamped)) {
        return;
    }
    mobius_focus_ = clamped;
    is_dirty_     = true;
    emit_changed();
}

real_t HyperbolicPoincareTilingResource::get_central_polygon_euclidean_radius() const {
    return CoreHyperbolic::compute_central_vertex_radius(schlafli_p_, schlafli_q_);
}

void HyperbolicPoincareTilingResource::rebuild() {
    is_dirty_ = true;
    _ensure_built();
    emit_changed();
}

void HyperbolicPoincareTilingResource::commit_set_schlafli_symbol(int p_p, int p_q) {
    const godot::Dictionary before = get_snapshot_state();
    set_schlafli_p(p_p);
    set_schlafli_q(p_q);
    commit_snapshot_mutation("Set Hyperbolic Schlafli Symbol", before);
}

void HyperbolicPoincareTilingResource::commit_set_generation_rings(int p_rings) {
    const godot::Dictionary before = get_snapshot_state();
    set_generation_rings(p_rings);
    commit_snapshot_mutation("Set Hyperbolic Generation Rings", before);
}

void HyperbolicPoincareTilingResource::commit_set_mobius_focus(const godot::Vector2 &p_focus) {
    const godot::Dictionary before = get_snapshot_state();
    set_mobius_focus(p_focus);
    commit_snapshot_mutation("Pan Hyperbolic Mobius Focus", before);
}

int HyperbolicPoincareTilingResource::get_cell_count() const {
    _ensure_built();
    return static_cast<int>(core_region_.cell_count());
}

int HyperbolicPoincareTilingResource::get_edge_count() const {
    _ensure_built();
    return static_cast<int>(core_region_.edge_count());
}

godot::PackedVector2Array HyperbolicPoincareTilingResource::get_cell_polygon(int p_cell_id) const {
    _ensure_built();
    if (p_cell_id < 0 || static_cast<size_t>(p_cell_id) >= core_region_.cell_count()) {
        return {};
    }
    const auto poly = core_region_.cell_polygon(static_cast<uint32_t>(p_cell_id));
    godot::PackedVector2Array out;
    out.resize(static_cast<int64_t>(poly.size()));
    for (size_t i = 0; i < poly.size(); ++i) {
        out.set(static_cast<int64_t>(i), _to_godot(poly[i]));
    }
    return out;
}

godot::Vector2 HyperbolicPoincareTilingResource::get_cell_center(int p_cell_id) const {
    _ensure_built();
    if (p_cell_id < 0 || static_cast<size_t>(p_cell_id) >= core_region_.cell_count()) {
        return godot::Vector2();
    }
    const auto &c = core_region_.cells()[static_cast<size_t>(p_cell_id)];
    return godot::Vector2(c.center_x, c.center_y);
}

int HyperbolicPoincareTilingResource::get_cell_generation(int p_cell_id) const {
    _ensure_built();
    if (p_cell_id < 0 || static_cast<size_t>(p_cell_id) >= core_region_.cell_count()) {
        return -1;
    }
    return static_cast<int>(core_region_.cells()[static_cast<size_t>(p_cell_id)].generation);
}

int HyperbolicPoincareTilingResource::get_cell_colour(int p_cell_id) const {
    _ensure_built();
    if (p_cell_id < 0 || static_cast<size_t>(p_cell_id) >= core_region_.cell_count()) {
        return 0;
    }
    return static_cast<int>(core_region_.cells()[static_cast<size_t>(p_cell_id)].colour_index);
}

godot::PackedInt32Array HyperbolicPoincareTilingResource::get_cell_neighbors(int p_cell_id) const {
    _ensure_built();
    if (p_cell_id < 0 || static_cast<size_t>(p_cell_id) >= core_region_.cell_count()) {
        return {};
    }
    const auto nbrs = core_region_.cell_neighbors(static_cast<uint32_t>(p_cell_id));
    godot::PackedInt32Array out;
    out.resize(static_cast<int64_t>(nbrs.size()));
    for (size_t i = 0; i < nbrs.size(); ++i) {
        out.set(static_cast<int64_t>(i), static_cast<int32_t>(nbrs[i]));
    }
    return out;
}

godot::PackedVector2Array HyperbolicPoincareTilingResource::sample_geodesic_arc(
    const godot::Vector2 &p_z1,
    const godot::Vector2 &p_z2,
    int p_subdivisions
) const {
    const auto pts = CoreHyperbolic::sample_geodesic_arc(
        _to_glm(p_z1),
        _to_glm(p_z2),
        std::clamp(p_subdivisions, 1, 64)
    );
    godot::PackedVector2Array out;
    out.resize(static_cast<int64_t>(pts.size()));
    for (size_t i = 0; i < pts.size(); ++i) {
        out.set(static_cast<int64_t>(i), _to_godot(pts[i]));
    }
    return out;
}

real_t HyperbolicPoincareTilingResource::compute_hyperbolic_distance(
    const godot::Vector2 &p_z1,
    const godot::Vector2 &p_z2
) const {
    return CoreHyperbolic::hyperbolic_distance(_to_glm(p_z1), _to_glm(p_z2));
}

int HyperbolicPoincareTilingResource::contains_point(const godot::Vector2 &p_disk_point) const {
    _ensure_built();
    if (p_disk_point.length_squared() >= static_cast<real_t>(1.0)) {
        return -1;
    }
    const uint32_t cid = core_region_.contains_point(_to_glm(p_disk_point));
    return (cid == core::INVALID_TILE_ID) ? -1 : static_cast<int>(cid);
}

godot::PackedInt32Array HyperbolicPoincareTilingResource::find_hyperbolic_path_cells(
    int p_start_cell,
    int p_goal_cell
) const {
    _ensure_built();
    if (p_start_cell < 0 || p_goal_cell < 0 ||
        static_cast<size_t>(p_start_cell) >= core_region_.cell_count() ||
        static_cast<size_t>(p_goal_cell) >= core_region_.cell_count()) {
        return {};
    }
    const auto res = CoreNav::find_path_cells(
        core_region_,
        static_cast<uint32_t>(p_start_cell),
        static_cast<uint32_t>(p_goal_cell),
        false
    );
    godot::PackedInt32Array out;
    if (!res.reachable) {
        return out;
    }
    out.resize(static_cast<int64_t>(res.cell_path.size()));
    for (size_t i = 0; i < res.cell_path.size(); ++i) {
        out.set(static_cast<int64_t>(i), static_cast<int32_t>(res.cell_path[i]));
    }
    return out;
}

godot::PackedVector2Array HyperbolicPoincareTilingResource::find_hyperbolic_path_points(
    int p_start_cell,
    int p_goal_cell,
    int p_arc_subdivisions
) const {
    const godot::PackedInt32Array cells = find_hyperbolic_path_cells(p_start_cell, p_goal_cell);
    godot::PackedVector2Array out;
    if (cells.is_empty()) {
        return out;
    }
    if (cells.size() == 1) {
        out.push_back(get_cell_center(cells[0]));
        return out;
    }
    const int subdiv = std::clamp(p_arc_subdivisions, 2, 32);
    for (int64_t i = 0; i + 1 < cells.size(); ++i) {
        const godot::Vector2 c0 = get_cell_center(cells[i]);
        const godot::Vector2 c1 = get_cell_center(cells[i + 1]);
        const godot::PackedVector2Array seg = sample_geodesic_arc(c0, c1, subdiv);
        const int64_t start_k = (i == 0) ? 0 : 1;
        for (int64_t k = start_k; k < seg.size(); ++k) {
            out.push_back(seg[k]);
        }
    }
    return out;
}

godot::PackedVector2Array HyperbolicPoincareTilingResource::compute_hyperbolic_flow_field(
    const godot::PackedInt32Array &p_goal_cells
) const {
    _ensure_built();
    std::vector<uint32_t> goals;
    goals.reserve(static_cast<size_t>(p_goal_cells.size()));
    for (int64_t i = 0; i < p_goal_cells.size(); ++i) {
        const int cid = p_goal_cells[i];
        if (cid >= 0 && static_cast<size_t>(cid) < core_region_.cell_count()) {
            goals.push_back(static_cast<uint32_t>(cid));
        }
    }
    const auto field = CoreNav::compute_flow_field(core_region_, goals);
    godot::PackedVector2Array out;
    out.resize(static_cast<int64_t>(field.flow_vectors.size()));
    for (size_t i = 0; i < field.flow_vectors.size(); ++i) {
        out.set(static_cast<int64_t>(i), _to_godot(field.flow_vectors[i]));
    }
    return out;
}

// ============================================================================
// PoincareDiskTilingCanvas
// ============================================================================

PoincareDiskTilingCanvas::PoincareDiskTilingCanvas() {
    set_custom_minimum_size(godot::Vector2(280.0f, 280.0f));
    frame_rect(godot::Rect2(-1.15f, -1.15f, 2.30f, 2.30f), 14.0f);
}

PoincareDiskTilingCanvas::~PoincareDiskTilingCanvas() {
    if (tiling_resource_.is_valid()) {
        const godot::Callable cb = callable_mp(this, &PoincareDiskTilingCanvas::_on_resource_changed);
        if (tiling_resource_->is_connected("changed", cb)) {
            tiling_resource_->disconnect("changed", cb);
        }
    }
}

void PoincareDiskTilingCanvas::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_tiling_resource", "resource"), &PoincareDiskTilingCanvas::set_tiling_resource);
    ClassDB::bind_method(D_METHOD("get_tiling_resource"), &PoincareDiskTilingCanvas::get_tiling_resource);

    ClassDB::bind_method(D_METHOD("set_show_geodesic_edges", "show"), &PoincareDiskTilingCanvas::set_show_geodesic_edges);
    ClassDB::bind_method(D_METHOD("get_show_geodesic_edges"), &PoincareDiskTilingCanvas::get_show_geodesic_edges);

    ClassDB::bind_method(D_METHOD("set_show_cell_centers", "show"), &PoincareDiskTilingCanvas::set_show_cell_centers);
    ClassDB::bind_method(D_METHOD("get_show_cell_centers"), &PoincareDiskTilingCanvas::get_show_cell_centers);

    ClassDB::bind_method(D_METHOD("set_show_path_overlay", "show"), &PoincareDiskTilingCanvas::set_show_path_overlay);
    ClassDB::bind_method(D_METHOD("get_show_path_overlay"), &PoincareDiskTilingCanvas::get_show_path_overlay);

    ClassDB::bind_method(D_METHOD("set_show_flow_field", "show"), &PoincareDiskTilingCanvas::set_show_flow_field);
    ClassDB::bind_method(D_METHOD("get_show_flow_field"), &PoincareDiskTilingCanvas::get_show_flow_field);

    ClassDB::bind_method(D_METHOD("set_path_endpoints", "start_cell", "goal_cell"), &PoincareDiskTilingCanvas::set_path_endpoints);
    ClassDB::bind_method(D_METHOD("get_path_start_cell"), &PoincareDiskTilingCanvas::get_path_start_cell);
    ClassDB::bind_method(D_METHOD("get_path_goal_cell"), &PoincareDiskTilingCanvas::get_path_goal_cell);

    ADD_SIGNAL(MethodInfo("cell_selected", PropertyInfo(Variant::INT, "cell_id")));
}

void PoincareDiskTilingCanvas::_on_resource_changed() {
    queue_redraw();
}

void PoincareDiskTilingCanvas::set_tiling_resource(const godot::Ref<HyperbolicPoincareTilingResource> &p_resource) {
    if (tiling_resource_ == p_resource) {
        return;
    }
    const godot::Callable cb = callable_mp(this, &PoincareDiskTilingCanvas::_on_resource_changed);
    if (tiling_resource_.is_valid() && tiling_resource_->is_connected("changed", cb)) {
        tiling_resource_->disconnect("changed", cb);
    }
    tiling_resource_ = p_resource;
    if (tiling_resource_.is_valid() && !tiling_resource_->is_connected("changed", cb)) {
        tiling_resource_->connect("changed", cb);
    }
    path_start_cell_id_ = 0;
    path_goal_cell_id_  = (tiling_resource_.is_valid() && tiling_resource_->get_cell_count() > 1)
        ? (tiling_resource_->get_cell_count() - 1)
        : -1;
    queue_redraw();
}

void PoincareDiskTilingCanvas::set_show_geodesic_edges(bool p_show) {
    show_geodesic_edges_ = p_show;
    queue_redraw();
}

void PoincareDiskTilingCanvas::set_show_cell_centers(bool p_show) {
    show_cell_centers_ = p_show;
    queue_redraw();
}

void PoincareDiskTilingCanvas::set_show_path_overlay(bool p_show) {
    show_path_overlay_ = p_show;
    queue_redraw();
}

void PoincareDiskTilingCanvas::set_show_flow_field(bool p_show) {
    show_flow_field_ = p_show;
    queue_redraw();
}

void PoincareDiskTilingCanvas::set_path_endpoints(int p_start_cell, int p_goal_cell) {
    path_start_cell_id_ = p_start_cell;
    path_goal_cell_id_  = p_goal_cell;
    queue_redraw();
}

void PoincareDiskTilingCanvas::_gui_input(const godot::Ref<godot::InputEvent> &p_event) {
    using namespace godot;
    if (p_event.is_null() || tiling_resource_.is_null()) {
        GeometryCanvas2DBase::_gui_input(p_event);
        return;
    }

    const Ref<InputEventMouseButton> mb = p_event;
    if (mb.is_valid()) {
        const Vector2 world_pt = screen_to_world(mb->get_position());
        if (mb->get_button_index() == MOUSE_BUTTON_RIGHT) {
            if (mb->is_pressed() && world_pt.length() < 0.98f) {
                dragging_mobius_  = true;
                drag_start_focus_ = tiling_resource_->get_mobius_focus();
                drag_start_disk_  = world_pt;
                accept_event();
                return;
            } else if (!mb->is_pressed() && dragging_mobius_) {
                dragging_mobius_ = false;
                const Vector2 final_focus = tiling_resource_->get_mobius_focus();
                tiling_resource_->set_mobius_focus(drag_start_focus_);
                tiling_resource_->commit_set_mobius_focus(final_focus);
                accept_event();
                return;
            }
        } else if (mb->get_button_index() == MOUSE_BUTTON_LEFT && mb->is_pressed()) {
            const int picked = tiling_resource_->contains_point(world_pt);
            if (picked >= 0) {
                if (mb->is_shift_pressed()) {
                    path_goal_cell_id_ = picked;
                } else {
                    path_start_cell_id_ = picked;
                    if (path_goal_cell_id_ < 0 && tiling_resource_->get_cell_count() > 1) {
                        path_goal_cell_id_ = tiling_resource_->get_cell_count() - 1;
                    }
                }
                emit_signal("cell_selected", picked);
                queue_redraw();
                accept_event();
                return;
            }
        }
    }

    const Ref<InputEventMouseMotion> mm = p_event;
    if (mm.is_valid()) {
        const Vector2 world_pt = screen_to_world(mm->get_position());
        if (dragging_mobius_) {
            const Vector2 delta = world_pt - drag_start_disk_;
            tiling_resource_->set_mobius_focus(drag_start_focus_ - delta * 0.65f);
            accept_event();
            return;
        }
        const int hovered = tiling_resource_->contains_point(world_pt);
        if (hovered != hovered_cell_id_) {
            hovered_cell_id_ = hovered;
            queue_redraw();
        }
    }

    GeometryCanvas2DBase::_gui_input(p_event);
}

void PoincareDiskTilingCanvas::_draw_geometry_canvas() {
    using namespace godot;

    // 1. Draw the Poincaré Ideal Boundary Circle (|z| = 1)
    const Vector2 center_screen = world_to_screen(Vector2(0.0f, 0.0f));
    const float radius_px       = static_cast<float>(get_zoom_scale());
    draw_circle(center_screen, radius_px, Color(0.06f, 0.08f, 0.12f, 0.92f));
    draw_arc(center_screen, radius_px, 0.0f, Math_TAU, 96, Color(0.45f, 0.72f, 0.95f, 0.85f), 2.0f, true);

    if (tiling_resource_.is_null()) {
        return;
    }

    const int n_cells = tiling_resource_->get_cell_count();
    static const Color GEN_PALETTE[7] = {
        Color(0.26f, 0.56f, 0.88f, 0.72f),
        Color(0.22f, 0.72f, 0.64f, 0.68f),
        Color(0.84f, 0.62f, 0.24f, 0.65f),
        Color(0.78f, 0.36f, 0.52f, 0.62f),
        Color(0.54f, 0.42f, 0.84f, 0.60f),
        Color(0.36f, 0.76f, 0.38f, 0.58f),
        Color(0.72f, 0.52f, 0.34f, 0.55f)
    };

    // 2. Draw hyperbolic cells
    for (int cid = 0; cid < n_cells; ++cid) {
        const PackedVector2Array world_poly = tiling_resource_->get_cell_polygon(cid);
        if (world_poly.size() < 3) {
            continue;
        }
        PackedVector2Array screen_poly;
        screen_poly.resize(world_poly.size() + 1);
        for (int64_t i = 0; i < world_poly.size(); ++i) {
            screen_poly.set(i, world_to_screen(world_poly[i]));
        }
        screen_poly.set(world_poly.size(), screen_poly[0]);

        const int gen = std::clamp(tiling_resource_->get_cell_generation(cid), 0, 6);
        Color fill_col = GEN_PALETTE[gen];
        if (cid == hovered_cell_id_) {
            fill_col = fill_col.lightened(0.28f);
        }
        if (cid == path_start_cell_id_) {
            fill_col = Color(0.22f, 0.90f, 0.46f, 0.82f);
        } else if (cid == path_goal_cell_id_) {
            fill_col = Color(0.96f, 0.38f, 0.32f, 0.82f);
        }

        PackedVector2Array closed_fill = world_poly;
        for (int64_t i = 0; i < closed_fill.size(); ++i) {
            closed_fill.set(i, screen_poly[i]);
        }
        PackedColorArray colors;
        colors.push_back(fill_col);
        draw_polygon(closed_fill, colors);

        if (show_geodesic_edges_) {
            draw_polyline(screen_poly, Color(0.88f, 0.94f, 1.0f, 0.62f), 1.25f, true);
        }
        if (show_cell_centers_) {
            const Vector2 c_scr = world_to_screen(tiling_resource_->get_cell_center(cid));
            draw_circle(c_scr, 2.5f, Color(1.0f, 0.92f, 0.55f, 0.85f));
        }
    }

    // 3. Optional Flow-Field Arrows toward Goal Cell
    if (show_flow_field_ && path_goal_cell_id_ >= 0 && path_goal_cell_id_ < n_cells) {
        PackedInt32Array goals;
        goals.push_back(path_goal_cell_id_);
        const PackedVector2Array flow = tiling_resource_->compute_hyperbolic_flow_field(goals);
        for (int cid = 0; cid < std::min<int>(n_cells, static_cast<int>(flow.size())); ++cid) {
            const Vector2 dir = flow[cid];
            if (dir.length_squared() < 1e-4f) {
                continue;
            }
            const Vector2 c0 = tiling_resource_->get_cell_center(cid);
            const float scale_euclid = std::max(0.025f, 0.10f * (1.0f - c0.length_squared()));
            const Vector2 p0 = world_to_screen(c0);
            const Vector2 p1 = world_to_screen(c0 + dir * scale_euclid);
            draw_line(p0, p1, Color(0.98f, 0.86f, 0.28f, 0.85f), 1.6f, true);
        }
    }

    // 4. Optional Hyperbolic Geodesic A* Path Ribbon
    if (show_path_overlay_ &&
        path_start_cell_id_ >= 0 && path_start_cell_id_ < n_cells &&
        path_goal_cell_id_ >= 0 && path_goal_cell_id_ < n_cells &&
        path_start_cell_id_ != path_goal_cell_id_) {
        const PackedVector2Array path_pts = tiling_resource_->find_hyperbolic_path_points(
            path_start_cell_id_,
            path_goal_cell_id_,
            12
        );
        if (path_pts.size() >= 2) {
            PackedVector2Array screen_path;
            screen_path.resize(path_pts.size());
            for (int64_t i = 0; i < path_pts.size(); ++i) {
                screen_path.set(i, world_to_screen(path_pts[i]));
            }
            draw_polyline(screen_path, Color(1.0f, 0.92f, 0.24f, 0.96f), 3.0f, true);
        }
    }
}

// ============================================================================
// HyperbolicPoincareTilingControl
// ============================================================================

HyperbolicPoincareTilingControl::HyperbolicPoincareTilingControl() {
    _build_ui();
}

HyperbolicPoincareTilingControl::~HyperbolicPoincareTilingControl() {
    if (tiling_resource_.is_valid()) {
        const godot::Callable cb = callable_mp(this, &HyperbolicPoincareTilingControl::_sync_from_resource);
        if (tiling_resource_->is_connected("changed", cb)) {
            tiling_resource_->disconnect("changed", cb);
        }
    }
}

void HyperbolicPoincareTilingControl::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_tiling_resource", "resource"), &HyperbolicPoincareTilingControl::set_tiling_resource);
    ClassDB::bind_method(D_METHOD("get_tiling_resource"), &HyperbolicPoincareTilingControl::get_tiling_resource);
    ClassDB::bind_method(D_METHOD("set_undo_redo", "undo_redo"), &HyperbolicPoincareTilingControl::set_undo_redo);
    ClassDB::bind_method(D_METHOD("get_undo_redo"), &HyperbolicPoincareTilingControl::get_undo_redo);
    ClassDB::bind_method(D_METHOD("get_canvas"), &HyperbolicPoincareTilingControl::get_canvas);
}

void HyperbolicPoincareTilingControl::_notification(int p_what) {
    if (p_what == NOTIFICATION_READY) {
        _sync_from_resource();
    }
}

void HyperbolicPoincareTilingControl::_build_ui() {
    using namespace godot;

    HBoxContainer *row1 = memnew(HBoxContainer);
    add_child(row1);

    Label *lbl_preset = memnew(Label);
    lbl_preset->set_text("Schlafli {p,q}:");
    row1->add_child(lbl_preset);

    preset_selector_ = memnew(OptionButton);
    preset_selector_->add_item("Custom {p,q}", HYPERBOLIC_PRESET_CUSTOM);
    preset_selector_->add_item("{7,3} Heptagonal", HYPERBOLIC_PRESET_HEPTAGON_7_3);
    preset_selector_->add_item("{5,4} Pentagonal", HYPERBOLIC_PRESET_PENTAGON_5_4);
    preset_selector_->add_item("{4,5} Square", HYPERBOLIC_PRESET_SQUARE_4_5);
    preset_selector_->add_item("{3,7} Triangular", HYPERBOLIC_PRESET_TRIANGLE_3_7);
    preset_selector_->add_item("{6,4} Hexagonal", HYPERBOLIC_PRESET_HEXAGON_6_4);
    preset_selector_->add_item("{8,3} Octagonal", HYPERBOLIC_PRESET_OCTAGON_8_3);
    preset_selector_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    preset_selector_->connect("item_selected", callable_mp(this, &HyperbolicPoincareTilingControl::_on_preset_selected));
    row1->add_child(preset_selector_);

    spin_p_ = memnew(SpinBox);
    spin_p_->set_min(3.0);
    spin_p_->set_max(16.0);
    spin_p_->set_step(1.0);
    spin_p_->set_prefix("p=");
    spin_p_->connect("value_changed", callable_mp(this, &HyperbolicPoincareTilingControl::_on_schlafli_changed));
    row1->add_child(spin_p_);

    spin_q_ = memnew(SpinBox);
    spin_q_->set_min(3.0);
    spin_q_->set_max(16.0);
    spin_q_->set_step(1.0);
    spin_q_->set_prefix("q=");
    spin_q_->connect("value_changed", callable_mp(this, &HyperbolicPoincareTilingControl::_on_schlafli_changed));
    row1->add_child(spin_q_);

    HBoxContainer *row2 = memnew(HBoxContainer);
    add_child(row2);

    Label *lbl_rings = memnew(Label);
    lbl_rings->set_text("Rings:");
    row2->add_child(lbl_rings);

    spin_rings_ = memnew(SpinBox);
    spin_rings_->set_min(0.0);
    spin_rings_->set_max(6.0);
    spin_rings_->set_step(1.0);
    spin_rings_->connect("value_changed", callable_mp(this, &HyperbolicPoincareTilingControl::_on_rings_changed));
    row2->add_child(spin_rings_);

    chk_path_ = memnew(CheckBox);
    chk_path_->set_text("A* Geodesic");
    chk_path_->set_pressed(true);
    chk_path_->connect("toggled", callable_mp(this, &HyperbolicPoincareTilingControl::_on_toggle_changed));
    row2->add_child(chk_path_);

    chk_flow_ = memnew(CheckBox);
    chk_flow_->set_text("Flow-Field");
    chk_flow_->set_pressed(false);
    chk_flow_->connect("toggled", callable_mp(this, &HyperbolicPoincareTilingControl::_on_toggle_changed));
    row2->add_child(chk_flow_);

    btn_reset_focus_ = memnew(Button);
    btn_reset_focus_->set_text("Origin Focus");
    btn_reset_focus_->connect("pressed", callable_mp(this, &HyperbolicPoincareTilingControl::_on_reset_focus_pressed));
    row2->add_child(btn_reset_focus_);

    btn_frame_disk_ = memnew(Button);
    btn_frame_disk_->set_text("Frame Disk");
    btn_frame_disk_->connect("pressed", callable_mp(this, &HyperbolicPoincareTilingControl::_on_frame_disk_pressed));
    row2->add_child(btn_frame_disk_);

    canvas_ = memnew(PoincareDiskTilingCanvas);
    canvas_->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    add_child(canvas_);

    status_label_ = memnew(Label);
    status_label_->set_text("Poincare Disk {7, 3} | LMB: Start Cell | Shift+LMB: Goal Cell | RMB Drag: SU(1,1) Mobius Pan");
    add_child(status_label_);
}

void HyperbolicPoincareTilingControl::set_tiling_resource(const godot::Ref<HyperbolicPoincareTilingResource> &p_resource) {
    if (tiling_resource_ == p_resource) {
        return;
    }
    const godot::Callable cb = callable_mp(this, &HyperbolicPoincareTilingControl::_sync_from_resource);
    if (tiling_resource_.is_valid() && tiling_resource_->is_connected("changed", cb)) {
        tiling_resource_->disconnect("changed", cb);
    }
    tiling_resource_ = p_resource;
    if (tiling_resource_.is_valid()) {
        if (undo_redo_.is_valid()) {
            tiling_resource_->set_undo_redo(undo_redo_);
        }
        if (!tiling_resource_->is_connected("changed", cb)) {
            tiling_resource_->connect("changed", cb);
        }
    }
    if (canvas_ != nullptr) {
        canvas_->set_tiling_resource(tiling_resource_);
    }
    _sync_from_resource();
}

void HyperbolicPoincareTilingControl::set_undo_redo(const godot::Ref<IdeamUndoRedo> &p_undo_redo) {
    undo_redo_ = p_undo_redo;
    if (tiling_resource_.is_valid()) {
        tiling_resource_->set_undo_redo(undo_redo_);
    }
}

void HyperbolicPoincareTilingControl::_sync_from_resource() {
    if (tiling_resource_.is_null()) {
        return;
    }
    updating_ui_ = true;
    const int p = tiling_resource_->get_schlafli_p();
    const int q = tiling_resource_->get_schlafli_q();
    if (spin_p_ != nullptr) {
        spin_p_->set_value(p);
    }
    if (spin_q_ != nullptr) {
        spin_q_->set_value(q);
    }
    if (spin_rings_ != nullptr) {
        spin_rings_->set_value(tiling_resource_->get_generation_rings());
    }
    if (preset_selector_ != nullptr) {
        preset_selector_->select(static_cast<int>(tiling_resource_->get_schlafli_preset()));
    }
    if (status_label_ != nullptr) {
        status_label_->set_text(
            godot::String("Poincare Disk {") + godot::String::num_int64(p) + ", " +
            godot::String::num_int64(q) + "} | Cells: " +
            godot::String::num_int64(tiling_resource_->get_cell_count()) +
            " | Edges: " + godot::String::num_int64(tiling_resource_->get_edge_count()) +
            " | RMB Drag: SU(1,1) Mobius Focus"
        );
    }
    updating_ui_ = false;
}

void HyperbolicPoincareTilingControl::_on_preset_selected(int p_index) {
    if (updating_ui_ || tiling_resource_.is_null()) {
        return;
    }
    const auto preset = static_cast<HyperbolicSchlafliPreset>(p_index);
    const godot::Dictionary before = tiling_resource_->get_snapshot_state();
    tiling_resource_->set_schlafli_preset(preset);
    tiling_resource_->commit_snapshot_mutation("Select Hyperbolic Schlafli Preset", before);
}

void HyperbolicPoincareTilingControl::_on_schlafli_changed(double p_val) {
    (void)p_val;
    if (updating_ui_ || tiling_resource_.is_null() || spin_p_ == nullptr || spin_q_ == nullptr) {
        return;
    }
    tiling_resource_->commit_set_schlafli_symbol(
        static_cast<int>(spin_p_->get_value()),
        static_cast<int>(spin_q_->get_value())
    );
}

void HyperbolicPoincareTilingControl::_on_rings_changed(double p_val) {
    if (updating_ui_ || tiling_resource_.is_null()) {
        return;
    }
    tiling_resource_->commit_set_generation_rings(static_cast<int>(p_val));
}

void HyperbolicPoincareTilingControl::_on_reset_focus_pressed() {
    if (tiling_resource_.is_null()) {
        return;
    }
    tiling_resource_->commit_set_mobius_focus(godot::Vector2(0.0f, 0.0f));
}

void HyperbolicPoincareTilingControl::_on_frame_disk_pressed() {
    if (canvas_ != nullptr) {
        canvas_->frame_rect(godot::Rect2(-1.15f, -1.15f, 2.30f, 2.30f), 14.0f);
    }
}

void HyperbolicPoincareTilingControl::_on_toggle_changed(bool p_pressed) {
    (void)p_pressed;
    if (canvas_ == nullptr) {
        return;
    }
    if (chk_path_ != nullptr) {
        canvas_->set_show_path_overlay(chk_path_->is_pressed());
    }
    if (chk_flow_ != nullptr) {
        canvas_->set_show_flow_field(chk_flow_->is_pressed());
    }
}

// ============================================================================
// HyperbolicPoincareTilingResourceInspector
// ============================================================================

void HyperbolicPoincareTilingResourceInspector::_bind_methods() {}

bool HyperbolicPoincareTilingResourceInspector::_can_handle(godot::Object *p_object) {
    return p_object != nullptr && godot::Object::cast_to<HyperbolicPoincareTilingResource>(p_object) != nullptr;
}

godot::Control *HyperbolicPoincareTilingResourceInspector::_create_inspector_control(godot::Object *p_object) {
    auto *res = godot::Object::cast_to<HyperbolicPoincareTilingResource>(p_object);
    if (res == nullptr) {
        return nullptr;
    }
    godot::Ref<HyperbolicPoincareTilingResource> ref(res);
    godot::Ref<IdeamUndoRedo> ur = IdeamTilesPlugin::create_undo_redo_wrapper();
    ref->set_undo_redo(ur);

    auto *ctrl = memnew(HyperbolicPoincareTilingControl);
    ctrl->set_undo_redo(ur);
    ctrl->set_tiling_resource(ref);
    return ctrl;
}

} // namespace ideam::godot_ext