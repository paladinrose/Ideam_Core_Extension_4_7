#include "spherical_goldberg_polyhedron_resource.h"
#include "ideam_tiles_plugin.h"

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>

#include <algorithm>
#include <cmath>

namespace ideam::godot_ext {

// ============================================================================
// SphericalGoldbergPolyhedronResource
// ============================================================================

SphericalGoldbergPolyhedronResource::SphericalGoldbergPolyhedronResource() {
    is_dirty_ = true;
}

SphericalGoldbergPolyhedronResource::~SphericalGoldbergPolyhedronResource() = default;

void SphericalGoldbergPolyhedronResource::_ensure_built() const {
    if (!is_dirty_) {
        return;
    }
    core_goldberg_.configure(subdivision_frequency_, planet_radius_);
    core_goldberg_.build_spherical_region(core_region_);

    const int64_t expected_cells = static_cast<int64_t>(core_goldberg_.cell_count());
    auto &mut_elev = const_cast<godot::PackedFloat32Array &>(cell_elevations_);
    if (mut_elev.size() != expected_cells) {
        const int64_t old_sz = mut_elev.size();
        mut_elev.resize(expected_cells);
        for (int64_t i = old_sz; i < expected_cells; ++i) {
            mut_elev.set(i, 0.0f);
        }
    }
    auto &mut_tags = const_cast<godot::PackedInt32Array &>(cell_tags_);
    if (mut_tags.size() != expected_cells) {
        const int64_t old_sz = mut_tags.size();
        mut_tags.resize(expected_cells);
        for (int64_t i = old_sz; i < expected_cells; ++i) {
            mut_tags.set(i, 0);
        }
    }
    is_dirty_ = false;
}

void SphericalGoldbergPolyhedronResource::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_subdivision_frequency", "frequency"), &SphericalGoldbergPolyhedronResource::set_subdivision_frequency);
    ClassDB::bind_method(D_METHOD("get_subdivision_frequency"), &SphericalGoldbergPolyhedronResource::get_subdivision_frequency);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "subdivision_frequency", PROPERTY_HINT_RANGE, "1,24,1"),
        "set_subdivision_frequency",
        "get_subdivision_frequency"
    );

    ClassDB::bind_method(D_METHOD("set_planet_radius", "radius"), &SphericalGoldbergPolyhedronResource::set_planet_radius);
    ClassDB::bind_method(D_METHOD("get_planet_radius"), &SphericalGoldbergPolyhedronResource::get_planet_radius);
    ADD_PROPERTY(
        PropertyInfo(Variant::FLOAT, "planet_radius", PROPERTY_HINT_RANGE, "0.1,1000.0,0.05"),
        "set_planet_radius",
        "get_planet_radius"
    );

    ClassDB::bind_method(D_METHOD("set_cell_elevations", "elevations"), &SphericalGoldbergPolyhedronResource::set_cell_elevations);
    ClassDB::bind_method(D_METHOD("get_cell_elevations"), &SphericalGoldbergPolyhedronResource::get_cell_elevations);
    ADD_PROPERTY(
        PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "cell_elevations"),
        "set_cell_elevations",
        "get_cell_elevations"
    );

    ClassDB::bind_method(D_METHOD("set_cell_tags", "tags"), &SphericalGoldbergPolyhedronResource::set_cell_tags);
    ClassDB::bind_method(D_METHOD("get_cell_tags"), &SphericalGoldbergPolyhedronResource::get_cell_tags);
    ADD_PROPERTY(
        PropertyInfo(Variant::PACKED_INT32_ARRAY, "cell_tags"),
        "set_cell_tags",
        "get_cell_tags"
    );

    ClassDB::bind_method(D_METHOD("set_cell_elevation", "cell_id", "elevation"), &SphericalGoldbergPolyhedronResource::set_cell_elevation);
    ClassDB::bind_method(D_METHOD("get_cell_elevation", "cell_id"), &SphericalGoldbergPolyhedronResource::get_cell_elevation);
    ClassDB::bind_method(D_METHOD("set_cell_tag", "cell_id", "tag"), &SphericalGoldbergPolyhedronResource::set_cell_tag);
    ClassDB::bind_method(D_METHOD("get_cell_tag", "cell_id"), &SphericalGoldbergPolyhedronResource::get_cell_tag);
    ClassDB::bind_method(D_METHOD("rebuild"), &SphericalGoldbergPolyhedronResource::rebuild);

    ClassDB::bind_method(D_METHOD("commit_set_subdivision_frequency", "frequency"), &SphericalGoldbergPolyhedronResource::commit_set_subdivision_frequency);
    ClassDB::bind_method(D_METHOD("commit_set_planet_radius", "radius"), &SphericalGoldbergPolyhedronResource::commit_set_planet_radius);
    ClassDB::bind_method(D_METHOD("commit_set_cell_elevation", "cell_id", "elevation"), &SphericalGoldbergPolyhedronResource::commit_set_cell_elevation);
    ClassDB::bind_method(D_METHOD("commit_set_cell_tag", "cell_id", "tag"), &SphericalGoldbergPolyhedronResource::commit_set_cell_tag);

    ClassDB::bind_method(D_METHOD("get_cell_count"), &SphericalGoldbergPolyhedronResource::get_cell_count);
    ClassDB::bind_method(D_METHOD("get_pentagon_count"), &SphericalGoldbergPolyhedronResource::get_pentagon_count);
    ClassDB::bind_method(D_METHOD("get_hexagon_count"), &SphericalGoldbergPolyhedronResource::get_hexagon_count);
    ClassDB::bind_method(D_METHOD("is_cell_pentagon", "cell_id"), &SphericalGoldbergPolyhedronResource::is_cell_pentagon);
    ClassDB::bind_method(D_METHOD("get_cell_unit_normal", "cell_id"), &SphericalGoldbergPolyhedronResource::get_cell_unit_normal);
    ClassDB::bind_method(D_METHOD("get_cell_surface_center_3d", "cell_id"), &SphericalGoldbergPolyhedronResource::get_cell_surface_center_3d);
    ClassDB::bind_method(
        D_METHOD("get_cell_polygon_3d", "cell_id", "inset_ratio"),
        &SphericalGoldbergPolyhedronResource::get_cell_polygon_3d,
        DEFVAL(0.0f)
    );
    ClassDB::bind_method(D_METHOD("get_cell_surface_transform_3d", "cell_id"), &SphericalGoldbergPolyhedronResource::get_cell_surface_transform_3d);
    ClassDB::bind_method(D_METHOD("get_cell_neighbors", "cell_id"), &SphericalGoldbergPolyhedronResource::get_cell_neighbors);
    ClassDB::bind_method(D_METHOD("get_pentagon_cell_ids"), &SphericalGoldbergPolyhedronResource::get_pentagon_cell_ids);

    ClassDB::bind_method(D_METHOD("locate_cell_from_direction", "direction"), &SphericalGoldbergPolyhedronResource::locate_cell_from_direction);
    ClassDB::bind_method(
        D_METHOD("find_spherical_path_cells", "start_cell", "goal_cell"),
        &SphericalGoldbergPolyhedronResource::find_spherical_path_cells
    );
    ClassDB::bind_method(
        D_METHOD("find_spherical_path_points_3d", "start_cell", "goal_cell", "radial_offset"),
        &SphericalGoldbergPolyhedronResource::find_spherical_path_points_3d,
        DEFVAL(0.08f)
    );
    ClassDB::bind_method(
        D_METHOD("compute_spherical_flow_field_3d", "goal_cells"),
        &SphericalGoldbergPolyhedronResource::compute_spherical_flow_field_3d
    );
}

godot::PackedVector2Array SphericalGoldbergPolyhedronResource::get_prototile_outline(int p_subdivisions) const {
    (void)p_subdivisions;
    godot::PackedVector2Array hex;
    hex.resize(6);
    for (int i = 0; i < 6; ++i) {
        const float angle = static_cast<float>(i) * (Math_TAU / 6.0f);
        hex.set(i, godot::Vector2(std::cos(angle), std::sin(angle)));
    }
    return hex;
}

godot::PackedInt32Array SphericalGoldbergPolyhedronResource::get_prototile_triangles(int p_subdivisions) const {
    (void)p_subdivisions;
    godot::PackedInt32Array tris;
    for (int i = 1; i + 1 < 6; ++i) {
        tris.push_back(0);
        tris.push_back(i);
        tris.push_back(i + 1);
    }
    return tris;
}

godot::Dictionary SphericalGoldbergPolyhedronResource::get_snapshot_state() const {
    _ensure_built();
    godot::Dictionary d;
    d["subdivision_frequency"] = subdivision_frequency_;
    d["planet_radius"]         = planet_radius_;
    d["cell_elevations"]       = cell_elevations_.duplicate();
    d["cell_tags"]             = cell_tags_.duplicate();
    return d;
}

void SphericalGoldbergPolyhedronResource::apply_snapshot_state(const godot::Dictionary &p_state) {
    if (p_state.has("subdivision_frequency")) {
        subdivision_frequency_ = std::clamp(static_cast<int>(p_state["subdivision_frequency"]), 1, 24);
    }
    if (p_state.has("planet_radius")) {
        planet_radius_ = std::max(static_cast<real_t>(0.05), static_cast<real_t>(p_state["planet_radius"]));
    }
    if (p_state.has("cell_elevations")) {
        cell_elevations_ = p_state["cell_elevations"];
    }
    if (p_state.has("cell_tags")) {
        cell_tags_ = p_state["cell_tags"];
    }
    is_dirty_ = true;
    emit_changed();
}

void SphericalGoldbergPolyhedronResource::set_subdivision_frequency(int p_freq) {
    p_freq = std::clamp(p_freq, 1, 24);
    if (subdivision_frequency_ == p_freq) {
        return;
    }
    subdivision_frequency_ = p_freq;
    is_dirty_              = true;
    emit_changed();
}

void SphericalGoldbergPolyhedronResource::set_planet_radius(real_t p_radius) {
    p_radius = std::max(static_cast<real_t>(0.05), p_radius);
    if ( std::abs(planet_radius_ - p_radius) <= static_cast<real_t>(1e-6)) {
        return;
    }
    planet_radius_ = p_radius;
    is_dirty_      = true;
    emit_changed();
}

void SphericalGoldbergPolyhedronResource::set_cell_elevations(const godot::PackedFloat32Array &p_elevations) {
    cell_elevations_ = p_elevations;
    is_dirty_        = true;
    emit_changed();
}

godot::PackedFloat32Array SphericalGoldbergPolyhedronResource::get_cell_elevations() const {
    _ensure_built();
    return cell_elevations_;
}

void SphericalGoldbergPolyhedronResource::set_cell_elevation(int p_cell_id, real_t p_elevation) {
    _ensure_built();
    if (p_cell_id < 0 || p_cell_id >= cell_elevations_.size()) {
        return;
    }
    cell_elevations_.set(p_cell_id, static_cast<float>(p_elevation));
    emit_changed();
}

real_t SphericalGoldbergPolyhedronResource::get_cell_elevation(int p_cell_id) const {
    _ensure_built();
    if (p_cell_id < 0 || p_cell_id >= cell_elevations_.size()) {
        return static_cast<real_t>(0.0);
    }
    return static_cast<real_t>(cell_elevations_[p_cell_id]);
}

void SphericalGoldbergPolyhedronResource::set_cell_tags(const godot::PackedInt32Array &p_tags) {
    cell_tags_ = p_tags;
    is_dirty_  = true;
    emit_changed();
}

godot::PackedInt32Array SphericalGoldbergPolyhedronResource::get_cell_tags() const {
    _ensure_built();
    return cell_tags_;
}

void SphericalGoldbergPolyhedronResource::set_cell_tag(int p_cell_id, int p_tag) {
    _ensure_built();
    if (p_cell_id < 0 || p_cell_id >= cell_tags_.size()) {
        return;
    }
    cell_tags_.set(p_cell_id, p_tag);
    emit_changed();
}

int SphericalGoldbergPolyhedronResource::get_cell_tag(int p_cell_id) const {
    _ensure_built();
    if (p_cell_id < 0 || p_cell_id >= cell_tags_.size()) {
        return 0;
    }
    return cell_tags_[p_cell_id];
}

void SphericalGoldbergPolyhedronResource::rebuild() {
    is_dirty_ = true;
    _ensure_built();
    emit_changed();
}

void SphericalGoldbergPolyhedronResource::commit_set_subdivision_frequency(int p_freq) {
    const godot::Dictionary before = get_snapshot_state();
    set_subdivision_frequency(p_freq);
    commit_snapshot_mutation("Set Goldberg Polyhedron Frequency", before);
}

void SphericalGoldbergPolyhedronResource::commit_set_planet_radius(real_t p_radius) {
    const godot::Dictionary before = get_snapshot_state();
    set_planet_radius(p_radius);
    commit_snapshot_mutation("Set Goldberg Planet Radius", before);
}

void SphericalGoldbergPolyhedronResource::commit_set_cell_elevation(int p_cell_id, real_t p_elevation) {
    const godot::Dictionary before = get_snapshot_state();
    set_cell_elevation(p_cell_id, p_elevation);
    commit_snapshot_mutation("Set Goldberg Tile Elevation", before);
}

void SphericalGoldbergPolyhedronResource::commit_set_cell_tag(int p_cell_id, int p_tag) {
    const godot::Dictionary before = get_snapshot_state();
    set_cell_tag(p_cell_id, p_tag);
    commit_snapshot_mutation("Set Goldberg Tile Tag", before);
}

int SphericalGoldbergPolyhedronResource::get_cell_count() const {
    _ensure_built();
    return static_cast<int>(core_goldberg_.cell_count());
}

int SphericalGoldbergPolyhedronResource::get_hexagon_count() const {
    _ensure_built();
    return std::max(0, static_cast<int>(core_goldberg_.cell_count()) - 12);
}

bool SphericalGoldbergPolyhedronResource::is_cell_pentagon(int p_cell_id) const {
    _ensure_built();
    if (p_cell_id < 0 || static_cast<size_t>(p_cell_id) >= core_goldberg_.cell_count()) {
        return false;
    }
    return core_goldberg_.cells()[static_cast<size_t>(p_cell_id)].is_pentagon;
}

godot::Vector3 SphericalGoldbergPolyhedronResource::get_cell_unit_normal(int p_cell_id) const {
    _ensure_built();
    if (p_cell_id < 0 || static_cast<size_t>(p_cell_id) >= core_goldberg_.cell_count()) {
        return godot::Vector3(0.0f, 1.0f, 0.0f);
    }
    return _to_godot3(core_goldberg_.cells()[static_cast<size_t>(p_cell_id)].unit_normal);
}

godot::Vector3 SphericalGoldbergPolyhedronResource::get_cell_surface_center_3d(int p_cell_id) const {
    _ensure_built();
    if (p_cell_id < 0 || static_cast<size_t>(p_cell_id) >= core_goldberg_.cell_count()) {
        return godot::Vector3();
    }
    const auto &c = core_goldberg_.cells()[static_cast<size_t>(p_cell_id)];
    const real_t elev = get_cell_elevation(p_cell_id);
    return _to_godot3(c.unit_normal * (planet_radius_ + elev));
}

godot::PackedVector3Array SphericalGoldbergPolyhedronResource::get_cell_polygon_3d(
    int p_cell_id,
    real_t p_inset_ratio
) const {
    _ensure_built();
    if (p_cell_id < 0 || static_cast<size_t>(p_cell_id) >= core_goldberg_.cell_count()) {
        return {};
    }
    const auto poly = core_goldberg_.cell_polygon_3d(static_cast<uint32_t>(p_cell_id));
    const godot::Vector3 center = _to_godot3(core_goldberg_.cells()[static_cast<size_t>(p_cell_id)].center_3d);
    const float inset = std::clamp(static_cast<float>(p_inset_ratio), 0.0f, 0.85f);

    godot::PackedVector3Array out;
    out.resize(static_cast<int64_t>(poly.size()));
    for (size_t i = 0; i < poly.size(); ++i) {
        const godot::Vector3 v = _to_godot3(poly[i]);
        out.set(static_cast<int64_t>(i), v.lerp(center, inset));
    }
    return out;
}

godot::Transform3D SphericalGoldbergPolyhedronResource::get_cell_surface_transform_3d(int p_cell_id) const {
    _ensure_built();
    if (p_cell_id < 0 || static_cast<size_t>(p_cell_id) >= core_goldberg_.cell_count()) {
        return godot::Transform3D();
    }
    const auto &c = core_goldberg_.cells()[static_cast<size_t>(p_cell_id)];
    const godot::Vector3 up   = _to_godot3(c.unit_normal).normalized();
    const godot::Vector3 tanu = _to_godot3(c.tangent_u).normalized();
    const godot::Vector3 tanv = _to_godot3(c.tangent_v).normalized();
    const godot::Basis basis(tanu, up, tanv);
    return godot::Transform3D(basis, get_cell_surface_center_3d(p_cell_id));
}

godot::PackedInt32Array SphericalGoldbergPolyhedronResource::get_cell_neighbors(int p_cell_id) const {
    _ensure_built();
    if (p_cell_id < 0 || static_cast<size_t>(p_cell_id) >= core_goldberg_.cell_count()) {
        return {};
    }
    const auto nbrs = core_goldberg_.cell_neighbors(static_cast<uint32_t>(p_cell_id));
    godot::PackedInt32Array out;
    out.resize(static_cast<int64_t>(nbrs.size()));
    for (size_t i = 0; i < nbrs.size(); ++i) {
        out.set(static_cast<int64_t>(i), static_cast<int32_t>(nbrs[i]));
    }
    return out;
}

godot::PackedInt32Array SphericalGoldbergPolyhedronResource::get_pentagon_cell_ids() const {
    _ensure_built();
    godot::PackedInt32Array out;
    for (size_t i = 0; i < core_goldberg_.cell_count(); ++i) {
        if (core_goldberg_.cells()[i].is_pentagon) {
            out.push_back(static_cast<int32_t>(i));
        }
    }
    return out;
}

int SphericalGoldbergPolyhedronResource::locate_cell_from_direction(const godot::Vector3 &p_direction) const {
    _ensure_built();
    const uint32_t cid = core_goldberg_.locate_cell_from_direction(_to_glm3(p_direction));
    return (cid == core::INVALID_TILE_ID) ? -1 : static_cast<int>(cid);
}

godot::PackedInt32Array SphericalGoldbergPolyhedronResource::find_spherical_path_cells(
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

godot::PackedVector3Array SphericalGoldbergPolyhedronResource::find_spherical_path_points_3d(
    int p_start_cell,
    int p_goal_cell,
    real_t p_radial_offset
) const {
    const godot::PackedInt32Array cells = find_spherical_path_cells(p_start_cell, p_goal_cell);
    godot::PackedVector3Array out;
    out.resize(cells.size());
    for (int64_t i = 0; i < cells.size(); ++i) {
        const int cid = cells[i];
        const godot::Vector3 n = get_cell_unit_normal(cid);
        const real_t r = planet_radius_ + get_cell_elevation(cid) + p_radial_offset;
        out.set(i, n * static_cast<float>(r));
    }
    return out;
}

godot::PackedVector3Array SphericalGoldbergPolyhedronResource::compute_spherical_flow_field_3d(
    const godot::PackedInt32Array &p_goal_cells
) const {
    _ensure_built();
    std::vector<uint32_t> goals;
    for (int64_t i = 0; i < p_goal_cells.size(); ++i) {
        const int cid = p_goal_cells[i];
        if (cid >= 0 && static_cast<size_t>(cid) < core_region_.cell_count()) {
            goals.push_back(static_cast<uint32_t>(cid));
        }
    }
    const auto field = CoreNav::compute_flow_field(core_region_, goals);
    godot::PackedVector3Array out;
    const size_t n = core_goldberg_.cell_count();
    out.resize(static_cast<int64_t>(n));
    for (size_t cid = 0; cid < n; ++cid) {
        const uint32_t next_id = (cid < field.next_cell.size()) ? field.next_cell[cid] : core::INVALID_TILE_ID;
        if (next_id == core::INVALID_TILE_ID || next_id == cid || next_id >= n) {
            out.set(static_cast<int64_t>(cid), godot::Vector3(0.0f, 0.0f, 0.0f));
            continue;
        }
        const godot::Vector3 p0 = get_cell_surface_center_3d(static_cast<int>(cid));
        const godot::Vector3 p1 = get_cell_surface_center_3d(static_cast<int>(next_id));
        const godot::Vector3 n0 = get_cell_unit_normal(static_cast<int>(cid));
        godot::Vector3 tangent  = p1 - p0;
        tangent -= n0 * tangent.dot(n0);
        out.set(static_cast<int64_t>(cid), tangent.normalized());
    }
    return out;
}

// ============================================================================
// GeodesicPlanetNode3D
// ============================================================================

GeodesicPlanetNode3D::GeodesicPlanetNode3D() {
    mesh_.instantiate();
    default_vertex_color_material_.instantiate();
    default_vertex_color_material_->set_flag(
        godot::BaseMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR,
        true
    );
    default_vertex_color_material_->set_roughness(0.65f);
    if ( auto *rs = godot::RenderingServer::get_singleton()) {
        mesh_instance_rid_ = rs->instance_create();
        rs->instance_set_base(mesh_instance_rid_, mesh_->get_rid());
    }
}

GeodesicPlanetNode3D::~GeodesicPlanetNode3D() {
    _disconnect_dependencies();
    if (mesh_instance_rid_.is_valid()) {
        if (auto *rs = godot::RenderingServer::get_singleton()) {
            rs->free_rid(mesh_instance_rid_);
        }
    }
}

void GeodesicPlanetNode3D::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_goldberg_resource", "resource"), &GeodesicPlanetNode3D::set_goldberg_resource);
    ClassDB::bind_method(D_METHOD("get_goldberg_resource"), &GeodesicPlanetNode3D::get_goldberg_resource);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "goldberg_resource", PROPERTY_HINT_RESOURCE_TYPE, "SphericalGoldbergPolyhedronResource"),
        "set_goldberg_resource",
        "get_goldberg_resource"
    );

    ClassDB::bind_method(D_METHOD("set_latitude_elevation_spline", "spline"), &GeodesicPlanetNode3D::set_latitude_elevation_spline);
    ClassDB::bind_method(D_METHOD("get_latitude_elevation_spline"), &GeodesicPlanetNode3D::get_latitude_elevation_spline);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "latitude_elevation_spline", PROPERTY_HINT_RESOURCE_TYPE, "Spline1DResource"),
        "set_latitude_elevation_spline",
        "get_latitude_elevation_spline"
    );

    ClassDB::bind_method(D_METHOD("set_biome_color_spline", "spline"), &GeodesicPlanetNode3D::set_biome_color_spline);
    ClassDB::bind_method(D_METHOD("get_biome_color_spline"), &GeodesicPlanetNode3D::get_biome_color_spline);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "biome_color_spline", PROPERTY_HINT_RESOURCE_TYPE, "Spline4DResource"),
        "set_biome_color_spline",
        "get_biome_color_spline"
    );

    ClassDB::bind_method(D_METHOD("set_tile_inset_ratio", "ratio"), &GeodesicPlanetNode3D::set_tile_inset_ratio);
    ClassDB::bind_method(D_METHOD("get_tile_inset_ratio"), &GeodesicPlanetNode3D::get_tile_inset_ratio);
    ADD_PROPERTY(
        PropertyInfo(Variant::FLOAT, "tile_inset_ratio", PROPERTY_HINT_RANGE, "0.0,0.6,0.01"),
        "set_tile_inset_ratio",
        "get_tile_inset_ratio"
    );

    ClassDB::bind_method(D_METHOD("set_base_tile_extrusion", "extrusion"), &GeodesicPlanetNode3D::set_base_tile_extrusion);
    ClassDB::bind_method(D_METHOD("get_base_tile_extrusion"), &GeodesicPlanetNode3D::get_base_tile_extrusion);
    ADD_PROPERTY(
        PropertyInfo(Variant::FLOAT, "base_tile_extrusion", PROPERTY_HINT_RANGE, "0.0,10.0,0.01"),
        "set_base_tile_extrusion",
        "get_base_tile_extrusion"
    );

    ClassDB::bind_method(D_METHOD("set_latitude_elevation_scale", "scale"), &GeodesicPlanetNode3D::set_latitude_elevation_scale);
    ClassDB::bind_method(D_METHOD("get_latitude_elevation_scale"), &GeodesicPlanetNode3D::get_latitude_elevation_scale);
    ADD_PROPERTY(
        PropertyInfo(Variant::FLOAT, "latitude_elevation_scale", PROPERTY_HINT_RANGE, "-5.0,5.0,0.01"),
        "set_latitude_elevation_scale",
        "get_latitude_elevation_scale"
    );

    ClassDB::bind_method(D_METHOD("set_pentagon_color", "color"), &GeodesicPlanetNode3D::set_pentagon_color);
    ClassDB::bind_method(D_METHOD("get_pentagon_color"), &GeodesicPlanetNode3D::get_pentagon_color);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "pentagon_color"), "set_pentagon_color", "get_pentagon_color");

    ClassDB::bind_method(D_METHOD("set_hexagon_color_a", "color"), &GeodesicPlanetNode3D::set_hexagon_color_a);
    ClassDB::bind_method(D_METHOD("get_hexagon_color_a"), &GeodesicPlanetNode3D::get_hexagon_color_a);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "hexagon_color_a"), "set_hexagon_color_a", "get_hexagon_color_a");

    ClassDB::bind_method(D_METHOD("set_hexagon_color_b", "color"), &GeodesicPlanetNode3D::set_hexagon_color_b);
    ClassDB::bind_method(D_METHOD("get_hexagon_color_b"), &GeodesicPlanetNode3D::get_hexagon_color_b);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "hexagon_color_b"), "set_hexagon_color_b", "get_hexagon_color_b");

    ClassDB::bind_method(D_METHOD("set_side_skirt_color", "color"), &GeodesicPlanetNode3D::set_side_skirt_color);
    ClassDB::bind_method(D_METHOD("get_side_skirt_color"), &GeodesicPlanetNode3D::get_side_skirt_color);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "side_skirt_color"), "set_side_skirt_color", "get_side_skirt_color");

    ClassDB::bind_method(D_METHOD("set_highlighted_cell_id", "cell_id"), &GeodesicPlanetNode3D::set_highlighted_cell_id);
    ClassDB::bind_method(D_METHOD("get_highlighted_cell_id"), &GeodesicPlanetNode3D::get_highlighted_cell_id);

    ClassDB::bind_method(D_METHOD("set_path_endpoints", "start_cell", "goal_cell"), &GeodesicPlanetNode3D::set_path_endpoints);
    ClassDB::bind_method(D_METHOD("get_path_start_cell"), &GeodesicPlanetNode3D::get_path_start_cell);
    ClassDB::bind_method(D_METHOD("get_path_goal_cell"), &GeodesicPlanetNode3D::get_path_goal_cell);

    ClassDB::bind_method(D_METHOD("set_material_override", "material"), &GeodesicPlanetNode3D::set_material_override);
    ClassDB::bind_method(D_METHOD("get_material_override"), &GeodesicPlanetNode3D::get_material_override);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "material_override", PROPERTY_HINT_RESOURCE_TYPE, "Material"),
        "set_material_override",
        "get_material_override"
    );

    ClassDB::bind_method(D_METHOD("get_generated_mesh"), &GeodesicPlanetNode3D::get_generated_mesh);
    ClassDB::bind_method(D_METHOD("compute_effective_cell_radius", "cell_id"), &GeodesicPlanetNode3D::compute_effective_cell_radius);
    ClassDB::bind_method(D_METHOD("raycast_pick_cell_3d", "world_ray_origin", "world_ray_dir"), &GeodesicPlanetNode3D::raycast_pick_cell_3d);
    ClassDB::bind_method(D_METHOD("rebuild_mesh"), &GeodesicPlanetNode3D::rebuild_mesh);
}

void GeodesicPlanetNode3D::_disconnect_dependencies() {
    const godot::Callable cb = callable_mp(this, &GeodesicPlanetNode3D::_on_dependency_changed);
    if (goldberg_resource_.is_valid() && goldberg_resource_->is_connected("changed", cb)) {
        goldberg_resource_->disconnect("changed", cb);
    }
    if (latitude_elevation_spline_.is_valid() && latitude_elevation_spline_->is_connected("changed", cb)) {
        latitude_elevation_spline_->disconnect("changed", cb);
    }
    if (biome_color_spline_.is_valid() && biome_color_spline_->is_connected("changed", cb)) {
        biome_color_spline_->disconnect("changed", cb);
    }
}

void GeodesicPlanetNode3D::_connect_dependencies() {
    const godot::Callable cb = callable_mp(this, &GeodesicPlanetNode3D::_on_dependency_changed);
    if (goldberg_resource_.is_valid() && !goldberg_resource_->is_connected("changed", cb)) {
        goldberg_resource_->connect("changed", cb);
    }
    if (latitude_elevation_spline_.is_valid() && !latitude_elevation_spline_->is_connected("changed", cb)) {
        latitude_elevation_spline_->connect("changed", cb);
    }
    if (biome_color_spline_.is_valid() && !biome_color_spline_->is_connected("changed", cb)) {
        biome_color_spline_->connect("changed", cb);
    }
}

void GeodesicPlanetNode3D::_on_dependency_changed() {
    rebuild_mesh();
    update_gizmos();
}

void GeodesicPlanetNode3D::_notification(int p_what) {
    auto *rs = godot::RenderingServer::get_singleton();
    if (rs == nullptr || !mesh_instance_rid_.is_valid()) {
        return;
    }
    switch (p_what) {
        case NOTIFICATION_ENTER_WORLD: {
            if (get_world_3d().is_valid()) {
                rs->instance_set_scenario(mesh_instance_rid_, get_world_3d()->get_scenario());
            }
            rs->instance_set_transform(mesh_instance_rid_, get_global_transform());
            rs->instance_set_visible(mesh_instance_rid_, is_visible_in_tree());
            rebuild_mesh();
            break;
        }
        case NOTIFICATION_EXIT_WORLD: {
            rs->instance_set_scenario(mesh_instance_rid_, godot::RID());
            break;
        }
        case NOTIFICATION_TRANSFORM_CHANGED: {
            rs->instance_set_transform(mesh_instance_rid_, get_global_transform());
            break;
        }
        case NOTIFICATION_VISIBILITY_CHANGED: {
            rs->instance_set_visible(mesh_instance_rid_, is_visible_in_tree());
            break;
        }
        default:
            break;
    }
}

void GeodesicPlanetNode3D::set_goldberg_resource(const godot::Ref<SphericalGoldbergPolyhedronResource> &p_res) {
    if (goldberg_resource_ == p_res) {
        return;
    }
    _disconnect_dependencies();
    goldberg_resource_ = p_res;
    _connect_dependencies();
    _on_dependency_changed();
}

void GeodesicPlanetNode3D::set_latitude_elevation_spline(const godot::Ref<Spline1DResource> &p_spline) {
    if (latitude_elevation_spline_ == p_spline) {
        return;
    }
    _disconnect_dependencies();
    latitude_elevation_spline_ = p_spline;
    _connect_dependencies();
    _on_dependency_changed();
}

void GeodesicPlanetNode3D::set_biome_color_spline(const godot::Ref<Spline4DResource> &p_spline) {
    if (biome_color_spline_ == p_spline) {
        return;
    }
    _disconnect_dependencies();
    biome_color_spline_ = p_spline;
    _connect_dependencies();
    _on_dependency_changed();
}

void GeodesicPlanetNode3D::set_tile_inset_ratio(real_t p_ratio) {
    tile_inset_ratio_ = std::clamp(p_ratio, static_cast<real_t>(0.0), static_cast<real_t>(0.6));
    rebuild_mesh();
}

void GeodesicPlanetNode3D::set_base_tile_extrusion(real_t p_extrusion) {
    base_tile_extrusion_ = std::max(static_cast<real_t>(0.0), p_extrusion);
    rebuild_mesh();
}

void GeodesicPlanetNode3D::set_latitude_elevation_scale(real_t p_scale) {
    latitude_elevation_scale_ = p_scale;
    rebuild_mesh();
}

void GeodesicPlanetNode3D::set_pentagon_color(const godot::Color &p_color) {
    pentagon_color_ = p_color;
    rebuild_mesh();
}

void GeodesicPlanetNode3D::set_hexagon_color_a(const godot::Color &p_color) {
    hexagon_color_a_ = p_color;
    rebuild_mesh();
}

void GeodesicPlanetNode3D::set_hexagon_color_b(const godot::Color &p_color) {
    hexagon_color_b_ = p_color;
    rebuild_mesh();
}

void GeodesicPlanetNode3D::set_side_skirt_color(const godot::Color &p_color) {
    side_skirt_color_ = p_color;
    rebuild_mesh();
}

void GeodesicPlanetNode3D::set_highlighted_cell_id(int p_cell_id) {
    highlighted_cell_id_ = p_cell_id;
    rebuild_mesh();
}

void GeodesicPlanetNode3D::set_path_endpoints(int p_start_cell, int p_goal_cell) {
    path_start_cell_id_ = p_start_cell;
    path_goal_cell_id_  = p_goal_cell;
    rebuild_mesh();
}

void GeodesicPlanetNode3D::set_material_override(const godot::Ref<godot::Material> &p_material) {
    material_override_ = p_material;
    rebuild_mesh();
}

real_t GeodesicPlanetNode3D::compute_effective_cell_radius(int p_cell_id) const {
    if (goldberg_resource_.is_null()) {
        return static_cast<real_t>(1.0);
    }
    real_t r = goldberg_resource_->get_planet_radius() + base_tile_extrusion_ +
               goldberg_resource_->get_cell_elevation(p_cell_id);
    if (latitude_elevation_spline_.is_valid() && std::abs(latitude_elevation_scale_) > static_cast<real_t>(1e-5)) {
        const godot::Vector3 n = goldberg_resource_->get_cell_unit_normal(p_cell_id);
        const real_t lat_u     = std::clamp((static_cast<real_t>(n.y) + static_cast<real_t>(1.0)) * static_cast<real_t>(0.5), static_cast<real_t>(0.0), static_cast<real_t>(1.0));
        r += latitude_elevation_spline_->sample(lat_u) * latitude_elevation_scale_;
    }
    return std::max(static_cast<real_t>(0.05), r);
}

int GeodesicPlanetNode3D::raycast_pick_cell_3d(
    const godot::Vector3 &p_world_ray_origin,
    const godot::Vector3 &p_world_ray_dir
) const {
    if (goldberg_resource_.is_null()) {
        return -1;
    }
    const godot::Transform3D inv_xform = get_global_transform().affine_inverse();
    const godot::Vector3 ro = inv_xform.xform(p_world_ray_origin);
    const godot::Vector3 rd = inv_xform.basis.xform(p_world_ray_dir).normalized();
    const real_t radius     = goldberg_resource_->get_planet_radius() + base_tile_extrusion_;

    const real_t b = ro.dot(rd);
    const real_t c = ro.length_squared() - radius * radius;
    const real_t disc = b * b - c;
    if (disc < static_cast<real_t>(0.0)) {
        return -1;
    }
    real_t t = -b - std::sqrt(disc);
    if (t < static_cast<real_t>(0.0)) {
        t = -b + std::sqrt(disc);
    }
    if (t < static_cast<real_t>(0.0)) {
        return -1;
    }
    const godot::Vector3 hit_local = ro + rd * t;
    return goldberg_resource_->locate_cell_from_direction(hit_local.normalized());
}

void GeodesicPlanetNode3D::rebuild_mesh() {
    using namespace godot;
    if (mesh_.is_null()) {
        mesh_.instantiate();
    }
    mesh_->clear_surfaces();
    if (goldberg_resource_.is_null()) {
        return;
    }

    const auto &core_gb = goldberg_resource_->get_core_goldberg();
    const int n_cells   = static_cast<int>(core_gb.cell_count());
    if (n_cells == 0) {
        return;
    }

    // Optional path cells set for highlighting
    PackedInt32Array path_cells;
    if (path_start_cell_id_ >= 0 && path_goal_cell_id_ >= 0 && path_start_cell_id_ != path_goal_cell_id_) {
        path_cells = goldberg_resource_->find_spherical_path_cells(path_start_cell_id_, path_goal_cell_id_);
    }

    PackedVector3Array vertices;
    PackedVector3Array normals;
    PackedColorArray colors;
    PackedInt32Array indices;

    const real_t base_r = goldberg_resource_->get_planet_radius();
    const float inset   = std::clamp(static_cast<float>(tile_inset_ratio_), 0.0f, 0.6f);

    for (int cid = 0; cid < n_cells; ++cid) {
        const auto &cell      = core_gb.cells()[static_cast<size_t>(cid)];
        const auto poly_unit  = core_gb.cell_polygon_3d(static_cast<uint32_t>(cid));
        const int v_count     = static_cast<int>(poly_unit.size());
        if (v_count < 3) {
            continue;
        }

        const Vector3 cell_normal(
            static_cast<float>(cell.unit_normal.x),
            static_cast<float>(cell.unit_normal.y),
            static_cast<float>(cell.unit_normal.z)
        );
        const float top_r    = static_cast<float>(compute_effective_cell_radius(cid));
        const float bottom_r = static_cast<float>(std::max(static_cast<real_t>(0.02), base_r * static_cast<real_t>(0.96)));

        Color top_col = cell.is_pentagon
            ? pentagon_color_
            : ((cid & 1) == 0 ? hexagon_color_a_ : hexagon_color_b_);

        if (biome_color_spline_.is_valid()) {
            const real_t elev_norm = std::clamp(
                (static_cast<real_t>(top_r) - base_r) / std::max(static_cast<real_t>(0.5), base_r * static_cast<real_t>(0.35)) + static_cast<real_t>(0.5),
                static_cast<real_t>(0.0),
                static_cast<real_t>(1.0)
            );
            const Vector4 rgba = biome_color_spline_->sample(elev_norm);
            top_col = Color(rgba.x, rgba.y, rgba.z, std::clamp(rgba.w, 0.2f, 1.0f));
        }

        if (cid == highlighted_cell_id_) {
            top_col = top_col.lightened(0.32f);
        }
        for (int64_t pi = 0; pi < path_cells.size(); ++pi) {
            if (path_cells[pi] == cid) {
                top_col = Color(0.98f, 0.88f, 0.22f, 1.0f);
                break;
            }
        }

        const int32_t base_v_idx = static_cast<int32_t>(vertices.size());
        for (int k = 0; k < v_count; ++k) {
            const Vector3 raw_dir = Vector3(
                static_cast<float>(poly_unit[static_cast<size_t>(k)].x),
                static_cast<float>(poly_unit[static_cast<size_t>(k)].y),
                static_cast<float>(poly_unit[static_cast<size_t>(k)].z)
            ).normalized();
            const Vector3 inset_dir = raw_dir.lerp(cell_normal, inset).normalized();
            vertices.push_back(inset_dir * top_r);
            normals.push_back(cell_normal);
            colors.push_back(top_col);
        }

        for (int k = 1; k + 1 < v_count; ++k) {
            indices.push_back(base_v_idx);
            indices.push_back(base_v_idx + k);
            indices.push_back(base_v_idx + k + 1);
        }

        // Extruded prism side skirts
        if (top_r > bottom_r + 1e-4f || inset > 1e-4f) {
            for (int k = 0; k < v_count; ++k) {
                const int kn = (k + 1) % v_count;
                const Vector3 d0 = Vector3(
                    static_cast<float>(poly_unit[static_cast<size_t>(k)].x),
                    static_cast<float>(poly_unit[static_cast<size_t>(k)].y),
                    static_cast<float>(poly_unit[static_cast<size_t>(k)].z)
                ).normalized();
                const Vector3 d1 = Vector3(
                    static_cast<float>(poly_unit[static_cast<size_t>(kn)].x),
                    static_cast<float>(poly_unit[static_cast<size_t>(kn)].y),
                    static_cast<float>(poly_unit[static_cast<size_t>(kn)].z)
                ).normalized();

                const Vector3 t0 = d0.lerp(cell_normal, inset).normalized() * top_r;
                const Vector3 t1 = d1.lerp(cell_normal, inset).normalized() * top_r;
                const Vector3 b0 = d0 * bottom_r;
                const Vector3 b1 = d1 * bottom_r;

                const Vector3 side_n = (t1 - t0).cross(b0 - t0).normalized();
                const int32_t s_idx  = static_cast<int32_t>(vertices.size());

                vertices.push_back(t0);
                vertices.push_back(t1);
                vertices.push_back(b1);
                vertices.push_back(b0);
                for (int q = 0; q < 4; ++q) {
                    normals.push_back(side_n);
                    colors.push_back(side_skirt_color_);
                }
                indices.push_back(s_idx + 0);
                indices.push_back(s_idx + 2);
                indices.push_back(s_idx + 1);
                indices.push_back(s_idx + 0);
                indices.push_back(s_idx + 3);
                indices.push_back(s_idx + 2);
            }
        }
    }

    if (vertices.is_empty() || indices.is_empty()) {
        return;
    }

    Array arrays;
    arrays.resize(Mesh::ARRAY_MAX);
    arrays[Mesh::ARRAY_VERTEX] = vertices;
    arrays[Mesh::ARRAY_NORMAL] = normals;
    arrays[Mesh::ARRAY_COLOR]  = colors;
    arrays[Mesh::ARRAY_INDEX]  = indices;

    mesh_->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);
    const Ref<Material> active_mat = material_override_.is_valid()
        ? material_override_
        : Ref<Material>(default_vertex_color_material_);
    if (active_mat.is_valid()) {
        mesh_->surface_set_material(0, active_mat);
    }
}

// ============================================================================
// GeodesicPlanetGizmo
// ============================================================================

GeodesicPlanetGizmo::GeodesicPlanetGizmo() {
    create_material("planet_wire_material", godot::Color(0.32f, 0.84f, 0.98f, 0.85f));
    create_material("pentagon_hub_material", godot::Color(0.98f, 0.64f, 0.22f, 0.95f));
    create_handle_material("handles");
}

void GeodesicPlanetGizmo::_bind_methods() {}

bool GeodesicPlanetGizmo::_has_gizmo(godot::Node3D *p_for_node_3d) const {
    return godot::Object::cast_to<GeodesicPlanetNode3D>(p_for_node_3d) != nullptr;
}

godot::String GeodesicPlanetGizmo::_get_gizmo_name() const {
    return "GeodesicPlanetGizmo";
}

int32_t GeodesicPlanetGizmo::_get_priority() const {
    return -1;
}

void GeodesicPlanetGizmo::_redraw(const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo) {
    using namespace godot;
    if (p_gizmo.is_null()) {
        return;
    }
    p_gizmo->clear();

    auto *planet = Object::cast_to<GeodesicPlanetNode3D>(p_gizmo->get_node_3d());
    if (planet == nullptr || planet->get_goldberg_resource().is_null()) {
        return;
    }

    const Ref<SphericalGoldbergPolyhedronResource> res = planet->get_goldberg_resource();
    const float radius = static_cast<float>(res->get_planet_radius());

    // Highlight the 12 pentagon hubs and expose radial handles
    PackedVector3Array hub_lines;
    PackedVector3Array handles;
    handles.push_back(Vector3(radius, 0.0f, 0.0f)); // Handle 0 = Planet Radius

    const PackedInt32Array pentagons = res->get_pentagon_cell_ids();
    for (int64_t i = 0; i < pentagons.size(); ++i) {
        const int cid = pentagons[i];
        const PackedVector3Array poly = res->get_cell_polygon_3d(cid, 0.0f);
        for (int64_t k = 0; k < poly.size(); ++k) {
            hub_lines.push_back(poly[k]);
            hub_lines.push_back(poly[(k + 1) % poly.size()]);
        }
        handles.push_back(res->get_cell_surface_center_3d(cid));
    }

    if (!hub_lines.is_empty()) {
        p_gizmo->add_lines(hub_lines, get_material("pentagon_hub_material", p_gizmo));
    }
    if (!handles.is_empty()) {
        p_gizmo->add_handles(handles, get_material("handles", p_gizmo));
    }
}

godot::String GeodesicPlanetGizmo::_get_handle_name(
    const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo,
    int32_t p_handle_id,
    bool p_secondary
) const {
    (void)p_gizmo;
    (void)p_secondary;
    if (p_handle_id == 0) {
        return "Planet Radius";
    }
    return godot::String("Pentagon Hub #") + godot::String::num_int64(p_handle_id - 1) + " Elevation";
}

godot::Variant GeodesicPlanetGizmo::_get_handle_value(
    const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo,
    int32_t p_handle_id,
    bool p_secondary
) const {
    (void)p_secondary;
    auto *planet = (p_gizmo.is_valid()) ? godot::Object::cast_to<GeodesicPlanetNode3D>(p_gizmo->get_node_3d()) : nullptr;
    if (planet == nullptr || planet->get_goldberg_resource().is_null()) {
        return 0.0f;
    }
    const auto res = planet->get_goldberg_resource();
    if (p_handle_id == 0) {
        return res->get_planet_radius();
    }
    const auto pentagons = res->get_pentagon_cell_ids();
    const int idx = p_handle_id - 1;
    if (idx >= 0 && idx < pentagons.size()) {
        return res->get_cell_elevation(pentagons[idx]);
    }
    return 0.0f;
}

void GeodesicPlanetGizmo::_begin_handle_action(
    const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo,
    int32_t p_handle_id,
    bool p_secondary
) {
    drag_initial_value_ = static_cast<real_t>(_get_handle_value(p_gizmo, p_handle_id, p_secondary));
}

void GeodesicPlanetGizmo::_set_handle(
    const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo,
    int32_t p_handle_id,
    bool p_secondary,
    godot::Camera3D *p_camera,
    const godot::Vector2 &p_screen_pos
) {
    (void)p_secondary;
    if (p_gizmo.is_null() || p_camera == nullptr) {
        return;
    }
    auto *planet = godot::Object::cast_to<GeodesicPlanetNode3D>(p_gizmo->get_node_3d());
    if (planet == nullptr || planet->get_goldberg_resource().is_null()) {
        return;
    }
    const auto res = planet->get_goldberg_resource();

    const godot::Transform3D inv_xform = planet->get_global_transform().affine_inverse();
    const godot::Vector3 ray_from      = inv_xform.xform(p_camera->project_ray_origin(p_screen_pos));
    const godot::Vector3 ray_dir       = inv_xform.basis.xform(p_camera->project_ray_normal(p_screen_pos)).normalized();

    const godot::Vector3 axis = (p_handle_id == 0)
        ? godot::Vector3(1.0f, 0.0f, 0.0f)
        : res->get_cell_unit_normal(res->get_pentagon_cell_ids()[p_handle_id - 1]);

    // Closest distance along radial axis from origin to camera ray
    const real_t d_dot_a = ray_dir.dot(axis);
    const real_t denom   = static_cast<real_t>(1.0) - d_dot_a * d_dot_a;
    real_t s = res->get_planet_radius();
    if (std::abs(denom) > static_cast<real_t>(1e-5)) {
        s = (ray_from.dot(axis) - ray_from.dot(ray_dir) * d_dot_a) / denom;
    }

    if (p_handle_id == 0) {
        res->set_planet_radius(std::max(static_cast<real_t>(0.2), s));
    } else {
        const auto pentagons = res->get_pentagon_cell_ids();
        const int idx = p_handle_id - 1;
        if (idx >= 0 && idx < pentagons.size()) {
            res->set_cell_elevation(pentagons[idx], s - res->get_planet_radius());
        }
    }
    p_gizmo->get_node_3d()->update_gizmos();
}

void GeodesicPlanetGizmo::_commit_handle(
    const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo,
    int32_t p_handle_id,
    bool p_secondary,
    const godot::Variant &p_restore,
    bool p_cancel
) {
    (void)p_secondary;
    if (p_gizmo.is_null()) {
        return;
    }
    auto *planet = godot::Object::cast_to<GeodesicPlanetNode3D>(p_gizmo->get_node_3d());
    if (planet == nullptr || planet->get_goldberg_resource().is_null()) {
        return;
    }
    const auto res = planet->get_goldberg_resource();
    const real_t restore_val = p_restore.get_type() == godot::Variant::NIL
        ? drag_initial_value_
        : static_cast<real_t>(p_restore);

    if (p_handle_id == 0) {
        const real_t final_r = res->get_planet_radius();
        res->set_planet_radius(restore_val);
        if (!p_cancel) {
            res->commit_set_planet_radius(final_r);
        }
    } else {
        const auto pentagons = res->get_pentagon_cell_ids();
        const int idx = p_handle_id - 1;
        if (idx >= 0 && idx < pentagons.size()) {
            const int cid = pentagons[idx];
            const real_t final_elev = res->get_cell_elevation(cid);
            res->set_cell_elevation(cid, restore_val);
            if (!p_cancel) {
                res->commit_set_cell_elevation(cid, final_elev);
            }
        }
    }
    planet->update_gizmos();
}

// ============================================================================
// SphericalGoldbergPreviewCanvas & SphericalGoldbergControl & Inspector
// ============================================================================

SphericalGoldbergPreviewCanvas::SphericalGoldbergPreviewCanvas() {
    set_custom_minimum_size(godot::Vector2(260.0f, 260.0f));
    frame_rect(godot::Rect2(-1.2f, -1.2f, 2.4f, 2.4f), 12.0f);
}

SphericalGoldbergPreviewCanvas::~SphericalGoldbergPreviewCanvas() {
    if (resource_.is_valid()) {
        const godot::Callable cb = callable_mp(this, &SphericalGoldbergPreviewCanvas::_on_resource_changed);
        if (resource_->is_connected("changed", cb)) {
            resource_->disconnect("changed", cb);
        }
    }
}

void SphericalGoldbergPreviewCanvas::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_goldberg_resource", "resource"), &SphericalGoldbergPreviewCanvas::set_goldberg_resource);
    ClassDB::bind_method(D_METHOD("get_goldberg_resource"), &SphericalGoldbergPreviewCanvas::get_goldberg_resource);
}

void SphericalGoldbergPreviewCanvas::_on_resource_changed() {
    queue_redraw();
}

void SphericalGoldbergPreviewCanvas::set_goldberg_resource(const godot::Ref<SphericalGoldbergPolyhedronResource> &p_res) {
    if (resource_ == p_res) {
        return;
    }
    const godot::Callable cb = callable_mp(this, &SphericalGoldbergPreviewCanvas::_on_resource_changed);
    if (resource_.is_valid() && resource_->is_connected("changed", cb)) {
        resource_->disconnect("changed", cb);
    }
    resource_ = p_res;
    if (resource_.is_valid() && !resource_->is_connected("changed", cb)) {
        resource_->connect("changed", cb);
    }
    queue_redraw();
}

void SphericalGoldbergPreviewCanvas::_gui_input(const godot::Ref<godot::InputEvent> &p_event) {
    using namespace godot;
    const Ref<InputEventMouseButton> mb = p_event;
    if (mb.is_valid() && mb->get_button_index() == MOUSE_BUTTON_LEFT) {
        rotating_globe_ = mb->is_pressed();
        last_mouse_pos_ = mb->get_position();
        accept_event();
        return;
    }
    const Ref<InputEventMouseMotion> mm = p_event;
    if (mm.is_valid() && rotating_globe_) {
        const Vector2 delta = mm->get_position() - last_mouse_pos_;
        last_mouse_pos_     = mm->get_position();
        yaw_radians_       += delta.x * 0.012f;
        pitch_radians_      = std::clamp(pitch_radians_ + delta.y * 0.012f, -1.45f, 1.45f);
        queue_redraw();
        accept_event();
        return;
    }
    GeometryCanvas2DBase::_gui_input(p_event);
}

void SphericalGoldbergPreviewCanvas::_draw_geometry_canvas() {
    using namespace godot;
    const Vector2 center_scr = world_to_screen(Vector2(0.0f, 0.0f));
    const float rad_px       = static_cast<float>(get_zoom_scale());
    draw_circle(center_scr, rad_px, Color(0.07f, 0.10f, 0.15f, 0.95f));

    if (resource_.is_null()) {
        return;
    }

    const Basis rot = Basis::from_euler(Vector3(pitch_radians_, yaw_radians_, 0.0f));
    const auto &gb  = resource_->get_core_goldberg();
    const float inv_r = 1.0f / std::max(0.05f, static_cast<float>(resource_->get_planet_radius()));

    for (size_t cid = 0; cid < gb.cell_count(); ++cid) {
        const auto &cell = gb.cells()[cid];
        const Vector3 n_rot = rot.xform(Vector3(
            static_cast<float>(cell.unit_normal.x),
            static_cast<float>(cell.unit_normal.y),
            static_cast<float>(cell.unit_normal.z)
        ));
        if (n_rot.z <= -0.05f) {
            continue; // Back-face cull on orthographic hemisphere
        }
        const auto poly3 = gb.cell_polygon_3d(static_cast<uint32_t>(cid));
        if (poly3.size() < 3) {
            continue;
        }
        PackedVector2Array poly2;
        poly2.resize(static_cast<int64_t>(poly3.size()) + 1);
        PackedVector2Array fill2;
        fill2.resize(static_cast<int64_t>(poly3.size()));
        for (size_t k = 0; k < poly3.size(); ++k) {
            const Vector3 v_rot = rot.xform(Vector3(
                static_cast<float>(poly3[k].x),
                static_cast<float>(poly3[k].y),
                static_cast<float>(poly3[k].z)
            ) * inv_r);
            const Vector2 scr = world_to_screen(Vector2(v_rot.x, v_rot.y));
            poly2.set(static_cast<int64_t>(k), scr);
            fill2.set(static_cast<int64_t>(k), scr);
        }
        poly2.set(static_cast<int64_t>(poly3.size()), poly2[0]);

        const float shade = std::clamp(0.35f + 0.65f * n_rot.z, 0.25f, 1.0f);
        const Color base_col = cell.is_pentagon
            ? Color(0.92f, 0.56f, 0.22f, 0.90f)
            : ((cid & 1) == 0 ? Color(0.24f, 0.56f, 0.82f, 0.85f) : Color(0.28f, 0.70f, 0.58f, 0.85f));
        PackedColorArray cols;
        cols.push_back(Color(base_col.r * shade, base_col.g * shade, base_col.b * shade, base_col.a));
        draw_polygon(fill2, cols);
        draw_polyline(poly2, Color(0.88f, 0.95f, 1.0f, 0.55f), 1.1f, true);
    }

    draw_arc(center_scr, rad_px, 0.0f, Math_TAU, 96, Color(0.45f, 0.76f, 0.96f, 0.85f), 2.0f, true);
}

SphericalGoldbergControl::SphericalGoldbergControl() {
    _build_ui();
}

SphericalGoldbergControl::~SphericalGoldbergControl() {
    if (resource_.is_valid()) {
        const godot::Callable cb = callable_mp(this, &SphericalGoldbergControl::_sync_from_resource);
        if (resource_->is_connected("changed", cb)) {
            resource_->disconnect("changed", cb);
        }
    }
}

void SphericalGoldbergControl::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_goldberg_resource", "resource"), &SphericalGoldbergControl::set_goldberg_resource);
    ClassDB::bind_method(D_METHOD("get_goldberg_resource"), &SphericalGoldbergControl::get_goldberg_resource);
    ClassDB::bind_method(D_METHOD("set_undo_redo", "undo_redo"), &SphericalGoldbergControl::set_undo_redo);
    ClassDB::bind_method(D_METHOD("get_undo_redo"), &SphericalGoldbergControl::get_undo_redo);
}

void SphericalGoldbergControl::_build_ui() {
    using namespace godot;

    HBoxContainer *row = memnew(HBoxContainer);
    add_child(row);

    Label *lbl_f = memnew(Label);
    lbl_f->set_text("Frequency GP(f,0):");
    row->add_child(lbl_f);

    spin_frequency_ = memnew(SpinBox);
    spin_frequency_->set_min(1.0);
    spin_frequency_->set_max(24.0);
    spin_frequency_->set_step(1.0);
    spin_frequency_->connect("value_changed", callable_mp(this, &SphericalGoldbergControl::_on_frequency_changed));
    row->add_child(spin_frequency_);

    Label *lbl_r = memnew(Label);
    lbl_r->set_text("Radius:");
    row->add_child(lbl_r);

    spin_radius_ = memnew(SpinBox);
    spin_radius_->set_min(0.1);
    spin_radius_->set_max(1000.0);
    spin_radius_->set_step(0.1);
    spin_radius_->connect("value_changed", callable_mp(this, &SphericalGoldbergControl::_on_radius_changed));
    row->add_child(spin_radius_);

    btn_reset_elevations_ = memnew(Button);
    btn_reset_elevations_->set_text("Flatten Elevations");
    btn_reset_elevations_->connect("pressed", callable_mp(this, &SphericalGoldbergControl::_on_reset_elevations_pressed));
    row->add_child(btn_reset_elevations_);

    canvas_ = memnew(SphericalGoldbergPreviewCanvas);
    canvas_->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    add_child(canvas_);

    summary_label_ = memnew(Label);
    summary_label_->set_text("Drag LMB on Globe to Orbit | 12 Pentagons + Hexagons");
    add_child(summary_label_);
}

void SphericalGoldbergControl::set_goldberg_resource(const godot::Ref<SphericalGoldbergPolyhedronResource> &p_res) {
    if (resource_ == p_res) {
        return;
    }
    const godot::Callable cb = callable_mp(this, &SphericalGoldbergControl::_sync_from_resource);
    if (resource_.is_valid() && resource_->is_connected("changed", cb)) {
        resource_->disconnect("changed", cb);
    }
    resource_ = p_res;
    if (resource_.is_valid()) {
        if (undo_redo_.is_valid()) {
            resource_->set_undo_redo(undo_redo_);
        }
        if (!resource_->is_connected("changed", cb)) {
            resource_->connect("changed", cb);
        }
    }
    if (canvas_ != nullptr) {
        canvas_->set_goldberg_resource(resource_);
    }
    _sync_from_resource();
}

void SphericalGoldbergControl::set_undo_redo(const godot::Ref<IdeamUndoRedo> &p_undo_redo) {
    undo_redo_ = p_undo_redo;
    if (resource_.is_valid()) {
        resource_->set_undo_redo(undo_redo_);
    }
}

void SphericalGoldbergControl::_sync_from_resource() {
    if (resource_.is_null()) {
        return;
    }
    updating_ui_ = true;
    if (spin_frequency_ != nullptr) {
        spin_frequency_->set_value(resource_->get_subdivision_frequency());
    }
    if (spin_radius_ != nullptr) {
        spin_radius_->set_value(resource_->get_planet_radius());
    }
    if (summary_label_ != nullptr) {
        summary_label_->set_text(
            godot::String("Goldberg GP(") + godot::String::num_int64(resource_->get_subdivision_frequency()) +
            ", 0) | Total Cells: " + godot::String::num_int64(resource_->get_cell_count()) +
            " (12 Pentagons + " + godot::String::num_int64(resource_->get_hexagon_count()) +
            " Hexagons) | LMB Drag: Orbit"
        );
    }
    updating_ui_ = false;
}

void SphericalGoldbergControl::_on_frequency_changed(double p_val) {
    if (updating_ui_ || resource_.is_null()) {
        return;
    }
    resource_->commit_set_subdivision_frequency(static_cast<int>(p_val));
}

void SphericalGoldbergControl::_on_radius_changed(double p_val) {
    if (updating_ui_ || resource_.is_null()) {
        return;
    }
    resource_->commit_set_planet_radius(static_cast<real_t>(p_val));
}

void SphericalGoldbergControl::_on_reset_elevations_pressed() {
    if (resource_.is_null()) {
        return;
    }
    const godot::Dictionary before = resource_->get_snapshot_state();
    godot::PackedFloat32Array zeros;
    zeros.resize(resource_->get_cell_count());
    zeros.fill(0.0f);
    resource_->set_cell_elevations(zeros);
    resource_->commit_snapshot_mutation("Flatten Goldberg Elevations", before);
}

void SphericalGoldbergResourceInspector::_bind_methods() {}

bool SphericalGoldbergResourceInspector::_can_handle(godot::Object *p_object) {
    return p_object != nullptr && godot::Object::cast_to<SphericalGoldbergPolyhedronResource>(p_object) != nullptr;
}

godot::Control *SphericalGoldbergResourceInspector::_create_inspector_control(godot::Object *p_object) {
    auto *res = godot::Object::cast_to<SphericalGoldbergPolyhedronResource>(p_object);
    if (res == nullptr) {
        return nullptr;
    }
    godot::Ref<SphericalGoldbergPolyhedronResource> ref(res);
    godot::Ref<IdeamUndoRedo> ur = IdeamTilesPlugin::create_undo_redo_wrapper();
    ref->set_undo_redo(ur);

    auto *ctrl = memnew(SphericalGoldbergControl);
    ctrl->set_undo_redo(ur);
    ctrl->set_goldberg_resource(ref);
    return ctrl;
}

} // namespace ideam::godot_ext