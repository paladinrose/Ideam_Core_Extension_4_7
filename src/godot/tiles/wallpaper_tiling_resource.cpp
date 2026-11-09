#include "wallpaper_tiling_resource.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

namespace ideam::godot_ext {

void WallpaperTilingResource::_bind_methods() {
    using namespace godot;

    // --- Symmetry Group & IH Type Configuration ---
    ADD_GROUP("Symmetry & Prototile", "");

    ClassDB::bind_method(D_METHOD("set_authoring_mode", "mode"), &WallpaperTilingResource::set_authoring_mode);
    ClassDB::bind_method(D_METHOD("get_authoring_mode"), &WallpaperTilingResource::get_authoring_mode);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "authoring_mode", PROPERTY_HINT_ENUM, "Wallpaper Group (17):0,Isohedral Tactile (81):1"),
        "set_authoring_mode",
        "get_authoring_mode"
    );

    ClassDB::bind_method(D_METHOD("set_wallpaper_group", "group"), &WallpaperTilingResource::set_wallpaper_group);
    ClassDB::bind_method(D_METHOD("get_wallpaper_group"), &WallpaperTilingResource::get_wallpaper_group);
    ADD_PROPERTY(
        PropertyInfo(
            Variant::INT,
            "wallpaper_group",
            PROPERTY_HINT_ENUM,
            "P1 (o):0,P2 (2222):1,PM (**):2,PG (xx):3,CM (*x):4,PMM (*2222):5,PMG (22*):6,PGG (22x):7,CMM (2*22):8,"
            "P4 (442):9,P4M (*442):10,P4G (4*2):11,P3 (333):12,P3M1 (*333):13,P31M (3*3):14,P6 (632):15,P6M (*632):16"
        ),
        "set_wallpaper_group",
        "get_wallpaper_group"
    );

    ClassDB::bind_method(D_METHOD("set_tactile_ih_type", "ih_type"), &WallpaperTilingResource::set_tactile_ih_type);
    ClassDB::bind_method(D_METHOD("get_tactile_ih_type"), &WallpaperTilingResource::get_tactile_ih_type);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "tactile_ih_type", PROPERTY_HINT_RANGE, "1,93,1"),
        "set_tactile_ih_type",
        "get_tactile_ih_type"
    );

    ClassDB::bind_method(D_METHOD("set_shape_preset", "preset"), &WallpaperTilingResource::set_shape_preset);
    ClassDB::bind_method(D_METHOD("get_shape_preset"), &WallpaperTilingResource::get_shape_preset);
    ADD_PROPERTY(
        PropertyInfo(
            Variant::INT,
            "shape_preset",
            PROPERTY_HINT_ENUM,
            "Tactile Default:0,Quad:1,Brick Hex:2,Right Triangle:3,Equilateral Triangle:4,Flat Triangle:5,Tall Triangle:6,Rhombus 60:7"
        ),
        "set_shape_preset",
        "get_shape_preset"
    );

    ClassDB::bind_method(D_METHOD("get_lattice_family"), &WallpaperTilingResource::get_lattice_family);
    ClassDB::bind_method(D_METHOD("get_iucr_symbol"), &WallpaperTilingResource::get_iucr_symbol);
    ClassDB::bind_method(D_METHOD("get_orbifold_symbol"), &WallpaperTilingResource::get_orbifold_symbol);
    ClassDB::bind_method(D_METHOD("get_rotational_order"), &WallpaperTilingResource::get_rotational_order);
    ClassDB::bind_method(D_METHOD("has_reflection"), &WallpaperTilingResource::has_reflection);
    ClassDB::bind_method(D_METHOD("has_glide_reflection"), &WallpaperTilingResource::has_glide_reflection);

    // --- Tactile Vertex Parameters ---
    ADD_GROUP("Tactile Parameters & Edge Splines", "");

    ClassDB::bind_method(D_METHOD("get_parameter_count"), &WallpaperTilingResource::get_parameter_count);
    ClassDB::bind_method(D_METHOD("set_tactile_parameters", "params"), &WallpaperTilingResource::set_tactile_parameters);
    ClassDB::bind_method(D_METHOD("get_tactile_parameters"), &WallpaperTilingResource::get_tactile_parameters);
    ADD_PROPERTY(
        PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "tactile_parameters"),
        "set_tactile_parameters",
        "get_tactile_parameters"
    );

    ClassDB::bind_method(D_METHOD("set_tactile_parameter", "index", "value"), &WallpaperTilingResource::set_tactile_parameter);
    ClassDB::bind_method(D_METHOD("get_tactile_parameter", "index"), &WallpaperTilingResource::get_tactile_parameter);

    ClassDB::bind_method(D_METHOD("set_edge_splines", "splines"), &WallpaperTilingResource::set_edge_splines);
    ClassDB::bind_method(D_METHOD("get_edge_splines"), &WallpaperTilingResource::get_edge_splines);
    ADD_PROPERTY(
        PropertyInfo(
            Variant::ARRAY,
            "edge_splines",
            PROPERTY_HINT_ARRAY_TYPE,
            String::num(Variant::OBJECT) + "/" + String::num(PROPERTY_HINT_RESOURCE_TYPE) + ":Spline2DResource"
        ),
        "set_edge_splines",
        "get_edge_splines"
    );

    ClassDB::bind_method(D_METHOD("set_edge_spline", "edge_shape_id", "spline"), &WallpaperTilingResource::set_edge_spline);
    ClassDB::bind_method(D_METHOD("get_edge_spline", "edge_shape_id"), &WallpaperTilingResource::get_edge_spline);
    ClassDB::bind_method(D_METHOD("get_edge_shape_constraint", "edge_shape_id"), &WallpaperTilingResource::get_edge_shape_constraint);
    ClassDB::bind_method(D_METHOD("clear_edge_splines"), &WallpaperTilingResource::clear_edge_splines);

    // --- Lattice Transform & Symmetry Element Handles ---
    ADD_GROUP("Lattice Transform & Symmetry Handles", "");

    ClassDB::bind_method(D_METHOD("set_lattice_scale", "scale"), &WallpaperTilingResource::set_lattice_scale);
    ClassDB::bind_method(D_METHOD("get_lattice_scale"), &WallpaperTilingResource::get_lattice_scale);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "lattice_scale"), "set_lattice_scale", "get_lattice_scale");

    ClassDB::bind_method(D_METHOD("set_lattice_rotation_radians", "rotation_rad"), &WallpaperTilingResource::set_lattice_rotation_radians);
    ClassDB::bind_method(D_METHOD("get_lattice_rotation_radians"), &WallpaperTilingResource::get_lattice_rotation_radians);
    ADD_PROPERTY(
        PropertyInfo(Variant::FLOAT, "lattice_rotation_radians", PROPERTY_HINT_RANGE, "-6.28319,6.28319,0.01,radians_as_degrees"),
        "set_lattice_rotation_radians",
        "get_lattice_rotation_radians"
    );

    ClassDB::bind_method(D_METHOD("set_lattice_origin", "origin"), &WallpaperTilingResource::set_lattice_origin);
    ClassDB::bind_method(D_METHOD("get_lattice_origin"), &WallpaperTilingResource::get_lattice_origin);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "lattice_origin"), "set_lattice_origin", "get_lattice_origin");

    ClassDB::bind_method(D_METHOD("get_translation_vector_t1"), &WallpaperTilingResource::get_translation_vector_t1);
    ClassDB::bind_method(D_METHOD("get_translation_vector_t2"), &WallpaperTilingResource::get_translation_vector_t2);

    ClassDB::bind_method(D_METHOD("set_axes_of_symmetry", "axes"), &WallpaperTilingResource::set_axes_of_symmetry);
    ClassDB::bind_method(D_METHOD("get_axes_of_symmetry"), &WallpaperTilingResource::get_axes_of_symmetry);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_VECTOR2_ARRAY, "axes_of_symmetry"), "set_axes_of_symmetry", "get_axes_of_symmetry");

    ClassDB::bind_method(D_METHOD("set_points_of_rotation", "points"), &WallpaperTilingResource::set_points_of_rotation);
    ClassDB::bind_method(D_METHOD("get_points_of_rotation"), &WallpaperTilingResource::get_points_of_rotation);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_VECTOR2_ARRAY, "points_of_rotation"), "set_points_of_rotation", "get_points_of_rotation");

    ClassDB::bind_method(D_METHOD("set_glides", "glides"), &WallpaperTilingResource::set_glides);
    ClassDB::bind_method(D_METHOD("get_glides"), &WallpaperTilingResource::get_glides);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_VECTOR2_ARRAY, "glides"), "set_glides", "get_glides");

    // --- Undo/Redo Commit Methods ---
    ClassDB::bind_method(D_METHOD("commit_set_wallpaper_group", "group"), &WallpaperTilingResource::commit_set_wallpaper_group);
    ClassDB::bind_method(D_METHOD("commit_set_tactile_ih_type", "ih_type"), &WallpaperTilingResource::commit_set_tactile_ih_type);
    ClassDB::bind_method(D_METHOD("commit_set_shape_preset", "preset"), &WallpaperTilingResource::commit_set_shape_preset);
    ClassDB::bind_method(D_METHOD("commit_set_tactile_parameter", "index", "value"), &WallpaperTilingResource::commit_set_tactile_parameter);
    ClassDB::bind_method(D_METHOD("commit_set_lattice_scale", "scale"), &WallpaperTilingResource::commit_set_lattice_scale);
    ClassDB::bind_method(D_METHOD("commit_set_lattice_rotation_radians", "rotation_rad"), &WallpaperTilingResource::commit_set_lattice_rotation_radians);
    ClassDB::bind_method(D_METHOD("commit_set_lattice_origin", "origin"), &WallpaperTilingResource::commit_set_lattice_origin);
    ClassDB::bind_method(D_METHOD("commit_set_edge_spline", "edge_shape_id", "spline"), &WallpaperTilingResource::commit_set_edge_spline);

    // --- Evaluation & Spatial Queries ---
    ClassDB::bind_method(D_METHOD("get_base_prototile_vertices"), &WallpaperTilingResource::get_base_prototile_vertices);
    ClassDB::bind_method(D_METHOD("get_cell_transform_2d", "t1", "t2", "aspect"), &WallpaperTilingResource::get_cell_transform_2d);
    ClassDB::bind_method(
        D_METHOD("get_cell_world_polygon", "t1", "t2", "aspect", "use_deformed_edges", "subdivisions"),
        &WallpaperTilingResource::get_cell_world_polygon,
        DEFVAL(true),
        DEFVAL(12)
    );
    ClassDB::bind_method(D_METHOD("get_cell_colour", "t1", "t2", "aspect"), &WallpaperTilingResource::get_cell_colour);
    ClassDB::bind_method(D_METHOD("locate_cell_at_world_point", "world_point"), &WallpaperTilingResource::locate_cell_at_world_point);
    ClassDB::bind_method(D_METHOD("step_to_neighbor", "t1", "t2", "aspect", "edge_slot"), &WallpaperTilingResource::step_to_neighbor);

    ClassDB::bind_static_method(
        "WallpaperTilingResource",
        D_METHOD("get_all_valid_ih_types"),
        &WallpaperTilingResource::get_all_valid_ih_types
    );
    ClassDB::bind_static_method(
        "WallpaperTilingResource",
        D_METHOD("get_ih_types_for_group", "group"),
        &WallpaperTilingResource::get_ih_types_for_group
    );

    ClassDB::bind_method(D_METHOD("_on_edge_spline_changed"), &WallpaperTilingResource::_on_edge_spline_changed);
}

WallpaperTilingResource::WallpaperTilingResource() = default;

WallpaperTilingResource::~WallpaperTilingResource() {
    _disconnect_edge_spline_signals();
}

void WallpaperTilingResource::_disconnect_edge_spline_signals() {
    const godot::Callable cb(this, "_on_edge_spline_changed");
    for (int i = 0; i < edge_splines.size(); ++i) {
        godot::Ref<Spline2DResource> sp = edge_splines[i];
        if (sp.is_valid() && sp->is_connected("changed", cb)) {
            sp->disconnect("changed", cb);
        }
    }
}

void WallpaperTilingResource::_connect_edge_spline_signals() {
    const godot::Callable cb(this, "_on_edge_spline_changed");
    for (int i = 0; i < edge_splines.size(); ++i) {
        godot::Ref<Spline2DResource> sp = edge_splines[i];
        if (sp.is_valid() && !sp->is_connected("changed", cb)) {
            sp->connect("changed", cb);
        }
    }
}

void WallpaperTilingResource::_sync_edge_splines_to_core() {
    core_tiling.clear_all_edge_splines();
    const int count = std::min<int>(edge_splines.size(), 6);
    for (int i = 0; i < count; ++i) {
        godot::Ref<Spline2DResource> sp = edge_splines[i];
        if (sp.is_valid() && sp->get_point_count() >= 2) {
            core_tiling.set_edge_spline(static_cast<uint8_t>(i), sp->get_core_spline());
        }
    }
}

void WallpaperTilingResource::_on_edge_spline_changed() {
    _sync_edge_splines_to_core();
    emit_signal("prototile_changed");
    emit_changed();
}

// ============================================================================
// TilingResource Virtual Overrides
// ============================================================================

int WallpaperTilingResource::get_vertex_count() const {
    return static_cast<int>(core_tiling.get_vertex_count());
}

int WallpaperTilingResource::get_aspect_count() const {
    return static_cast<int>(core_tiling.get_aspect_count());
}

int WallpaperTilingResource::get_edge_shape_count() const {
    return static_cast<int>(core_tiling.get_edge_shape_count());
}

godot::PackedVector2Array WallpaperTilingResource::get_prototile_outline(int p_subdivisions) const {
    const auto pts = core_tiling.get_deformed_prototile_outline(p_subdivisions);
    godot::PackedVector2Array out;
    out.resize(static_cast<int>(pts.size()));
    for (size_t i = 0; i < pts.size(); ++i) {
        out.set(static_cast<int>(i), _to_godot(pts[i]));
    }
    return out;
}

godot::PackedInt32Array WallpaperTilingResource::get_prototile_triangles(int p_subdivisions) const {
    const auto tris = core_tiling.get_deformed_prototile_triangles(p_subdivisions);
    godot::PackedInt32Array out;
    out.resize(static_cast<int>(tris.size()));
    for (size_t i = 0; i < tris.size(); ++i) {
        out.set(static_cast<int>(i), static_cast<int32_t>(tris[i]));
    }
    return out;
}

godot::Dictionary WallpaperTilingResource::get_snapshot_state() const {
    godot::Dictionary d;
    d["authoring_mode"]           = static_cast<int>(get_authoring_mode());
    d["wallpaper_group"]          = static_cast<int>(get_wallpaper_group());
    d["tactile_ih_type"]          = get_tactile_ih_type();
    d["shape_preset"]             = static_cast<int>(get_shape_preset());
    d["tactile_parameters"]       = get_tactile_parameters();
    d["lattice_scale"]            = get_lattice_scale();
    d["lattice_rotation_radians"] = get_lattice_rotation_radians();
    d["lattice_origin"]           = get_lattice_origin();
    d["axes_of_symmetry"]         = get_axes_of_symmetry();
    d["points_of_rotation"]       = get_points_of_rotation();
    d["glides"]                   = get_glides();
    return d;
}

void WallpaperTilingResource::apply_snapshot_state(const godot::Dictionary &p_state) {
    if (p_state.is_empty()) return;

    const int mode = p_state.get("authoring_mode", static_cast<int>(get_authoring_mode()));
    if (mode == TILING_MODE_ISOHEDRAL_81 && p_state.has("tactile_ih_type")) {
        core_tiling.reset_from_ih_type(static_cast<uint8_t>(int(p_state["tactile_ih_type"])));
    } else if (p_state.has("wallpaper_group")) {
        const auto grp = static_cast<core::WallpaperGroupId>(int(p_state["wallpaper_group"]));
        const auto preset = static_cast<core::FundamentalShapePreset>(
            int(p_state.get("shape_preset", static_cast<int>(SHAPE_PRESET_TACTILE_DEFAULT)))
        );
        core_tiling.reset_from_wallpaper_group(grp, preset);
    }

    if (p_state.has("tactile_parameters")) {
        set_tactile_parameters(p_state["tactile_parameters"]);
    }

    const godot::Vector2 scl = p_state.get("lattice_scale", get_lattice_scale());
    const real_t rot         = p_state.get("lattice_rotation_radians", get_lattice_rotation_radians());
    const godot::Vector2 org = p_state.get("lattice_origin", get_lattice_origin());
    core_tiling.set_lattice_transform(_to_glm(scl), rot, _to_glm(org));

    emit_signal("topology_changed");
    emit_signal("prototile_changed");
    emit_changed();
}

// ============================================================================
// Symmetry Group, Tactile IH Type & Shape Preset Properties
// ============================================================================

void WallpaperTilingResource::set_authoring_mode(TilingAuthoringModeType p_mode) {
    if (get_authoring_mode() == p_mode) return;
    if (p_mode == TILING_MODE_ISOHEDRAL_81) {
        core_tiling.reset_from_ih_type(core_tiling.get_tactile_ih_type());
    } else {
        core_tiling.reset_from_wallpaper_group(core_tiling.get_wallpaper_group(), core_tiling.get_shape_preset());
    }
    _sync_edge_splines_to_core();
    emit_signal("topology_changed");
    emit_changed();
}

TilingAuthoringModeType WallpaperTilingResource::get_authoring_mode() const {
    return static_cast<TilingAuthoringModeType>(core_tiling.get_authoring_mode());
}

void WallpaperTilingResource::set_wallpaper_group(WallpaperGroupType p_group) {
    const auto grp = static_cast<core::WallpaperGroupId>(std::clamp<int>(p_group, 0, 16));
    if (core_tiling.get_wallpaper_group() == grp && core_tiling.get_authoring_mode() == core::TilingAuthoringMode::WALLPAPER_GROUP_17) {
        return;
    }
    core_tiling.reset_from_wallpaper_group(grp, core_tiling.get_shape_preset());
    _sync_edge_splines_to_core();
    emit_signal("topology_changed");
    emit_signal("prototile_changed");
    emit_changed();
}

WallpaperGroupType WallpaperTilingResource::get_wallpaper_group() const {
    return static_cast<WallpaperGroupType>(core_tiling.get_wallpaper_group());
}

void WallpaperTilingResource::set_tactile_ih_type(int p_ih_type) {
    const uint8_t ih = static_cast<uint8_t>(std::clamp(p_ih_type, 1, 93));
    if (!CoreTiling::is_valid_ih_type(ih)) return;
    if (core_tiling.get_tactile_ih_type() == ih &&
        core_tiling.get_authoring_mode() == core::TilingAuthoringMode::ISOHEDRAL_TACTILE_81) {
        return;
    }
    core_tiling.reset_from_ih_type(ih);
    _sync_edge_splines_to_core();
    emit_signal("topology_changed");
    emit_signal("prototile_changed");
    emit_changed();
}

int WallpaperTilingResource::get_tactile_ih_type() const {
    return static_cast<int>(core_tiling.get_tactile_ih_type());
}

void WallpaperTilingResource::set_shape_preset(TilingShapePresetType p_preset) {
    const auto preset = static_cast<core::FundamentalShapePreset>(std::clamp<int>(p_preset, 0, 7));
    if (core_tiling.get_shape_preset() == preset) return;
    core_tiling.set_shape_preset(preset);
    emit_signal("prototile_changed");
    emit_changed();
}

TilingShapePresetType WallpaperTilingResource::get_shape_preset() const {
    return static_cast<TilingShapePresetType>(core_tiling.get_shape_preset());
}

TilingLatticeFamily WallpaperTilingResource::get_lattice_family() const {
    const auto& desc = core::SymmetryMath2D<real_t>::get_group_descriptor(core_tiling.get_wallpaper_group());
    return static_cast<TilingLatticeFamily>(desc.lattice_type);
}

godot::String WallpaperTilingResource::get_iucr_symbol() const {
    const auto& desc = core::SymmetryMath2D<real_t>::get_group_descriptor(core_tiling.get_wallpaper_group());
    return godot::String(std::string(desc.iucr_symbol).c_str());
}

godot::String WallpaperTilingResource::get_orbifold_symbol() const {
    const auto& desc = core::SymmetryMath2D<real_t>::get_group_descriptor(core_tiling.get_wallpaper_group());
    return godot::String(std::string(desc.orbifold_symbol).c_str());
}

int WallpaperTilingResource::get_rotational_order() const {
    const auto& desc = core::SymmetryMath2D<real_t>::get_group_descriptor(core_tiling.get_wallpaper_group());
    return static_cast<int>(desc.rotational_order);
}

bool WallpaperTilingResource::has_reflection() const {
    const auto& desc = core::SymmetryMath2D<real_t>::get_group_descriptor(core_tiling.get_wallpaper_group());
    return desc.has_reflection;
}

bool WallpaperTilingResource::has_glide_reflection() const {
    const auto& desc = core::SymmetryMath2D<real_t>::get_group_descriptor(core_tiling.get_wallpaper_group());
    return desc.has_glide_reflection;
}

// ============================================================================
// Tactile Parameters (0..6 Vertex Parameters)
// ============================================================================

int WallpaperTilingResource::get_parameter_count() const {
    return static_cast<int>(core_tiling.get_parameter_count());
}

void WallpaperTilingResource::set_tactile_parameters(const godot::PackedFloat32Array &p_params) {
    std::vector<real_t> vals(static_cast<size_t>(p_params.size()));
    for (int i = 0; i < p_params.size(); ++i) {
        vals[static_cast<size_t>(i)] = static_cast<real_t>(p_params[i]);
    }
    core_tiling.set_parameters(vals);
    emit_signal("prototile_changed");
    emit_changed();
}

godot::PackedFloat32Array WallpaperTilingResource::get_tactile_parameters() const {
    const auto span = core_tiling.get_parameters();
    godot::PackedFloat32Array out;
    out.resize(static_cast<int>(span.size()));
    for (size_t i = 0; i < span.size(); ++i) {
        out.set(static_cast<int>(i), static_cast<float>(span[i]));
    }
    return out;
}

void WallpaperTilingResource::set_tactile_parameter(int p_index, real_t p_value) {
    if (p_index < 0 || p_index >= get_parameter_count()) return;
    core_tiling.set_parameter(static_cast<size_t>(p_index), p_value);
    emit_signal("prototile_changed");
    emit_changed();
}

real_t WallpaperTilingResource::get_tactile_parameter(int p_index) const {
    if (p_index < 0 || p_index >= get_parameter_count()) return static_cast<real_t>(0.0);
    return core_tiling.get_parameter(static_cast<size_t>(p_index));
}

// ============================================================================
// Lattice Basis Transform & Explicit Symmetry Handles
// ============================================================================

void WallpaperTilingResource::set_lattice_scale(const godot::Vector2 &p_scale) {
    if (get_lattice_scale().is_equal_approx(p_scale)) return;
    core_tiling.set_lattice_transform(
        _to_glm(p_scale),
        core_tiling.get_lattice_rotation_rad(),
        core_tiling.get_lattice_origin()
    );
    emit_signal("topology_changed");
    emit_changed();
}

godot::Vector2 WallpaperTilingResource::get_lattice_scale() const {
    return _to_godot(core_tiling.get_lattice_scale());
}

void WallpaperTilingResource::set_lattice_rotation_radians(real_t p_rotation_rad) {
    if (std::abs(core_tiling.get_lattice_rotation_rad() - p_rotation_rad) <= 1e-6f) return;
    core_tiling.set_lattice_transform(
        core_tiling.get_lattice_scale(),
        p_rotation_rad,
        core_tiling.get_lattice_origin()
    );
    emit_signal("topology_changed");
    emit_changed();
}

real_t WallpaperTilingResource::get_lattice_rotation_radians() const {
    return core_tiling.get_lattice_rotation_rad();
}

void WallpaperTilingResource::set_lattice_origin(const godot::Vector2 &p_origin) {
    if (get_lattice_origin().is_equal_approx(p_origin)) return;
    core_tiling.set_lattice_transform(
        core_tiling.get_lattice_scale(),
        core_tiling.get_lattice_rotation_rad(),
        _to_glm(p_origin)
    );
    emit_signal("topology_changed");
    emit_changed();
}

godot::Vector2 WallpaperTilingResource::get_lattice_origin() const {
    return _to_godot(core_tiling.get_lattice_origin());
}

godot::Vector2 WallpaperTilingResource::get_translation_vector_t1() const {
    return _to_godot(core_tiling.get_lattice_basis().t1);
}

godot::Vector2 WallpaperTilingResource::get_translation_vector_t2() const {
    return _to_godot(core_tiling.get_lattice_basis().t2);
}

void WallpaperTilingResource::set_axes_of_symmetry(const godot::PackedVector2Array &p_axes) {
    auto handles = core_tiling.get_symmetry_handles();
    handles.axis_count = static_cast<uint8_t>(std::min<int>(p_axes.size(), 4));
    for (uint8_t i = 0; i < handles.axis_count; ++i) {
        handles.axes_of_symmetry[i] = _to_glm(p_axes[i]);
    }
    core_tiling.set_symmetry_handles(handles);
    emit_signal("topology_changed");
    emit_changed();
}

godot::PackedVector2Array WallpaperTilingResource::get_axes_of_symmetry() const {
    const auto &handles = core_tiling.get_symmetry_handles();
    godot::PackedVector2Array out;
    out.resize(handles.axis_count);
    for (uint8_t i = 0; i < handles.axis_count; ++i) {
        out.set(i, _to_godot(handles.axes_of_symmetry[i]));
    }
    return out;
}

void WallpaperTilingResource::set_points_of_rotation(const godot::PackedVector2Array &p_points) {
    auto handles = core_tiling.get_symmetry_handles();
    handles.rotation_pivot_count = static_cast<uint8_t>(std::min<int>(p_points.size(), 5));
    for (uint8_t i = 0; i < handles.rotation_pivot_count; ++i) {
        handles.points_of_rotation[i] = _to_glm(p_points[i]);
    }
    core_tiling.set_symmetry_handles(handles);
    emit_signal("topology_changed");
    emit_changed();
}

godot::PackedVector2Array WallpaperTilingResource::get_points_of_rotation() const {
    const auto &handles = core_tiling.get_symmetry_handles();
    godot::PackedVector2Array out;
    out.resize(handles.rotation_pivot_count);
    for (uint8_t i = 0; i < handles.rotation_pivot_count; ++i) {
        out.set(i, _to_godot(handles.points_of_rotation[i]));
    }
    return out;
}

void WallpaperTilingResource::set_glides(const godot::PackedVector2Array &p_glides) {
    auto handles = core_tiling.get_symmetry_handles();
    handles.glide_count = static_cast<uint8_t>(std::min<int>(p_glides.size(), 2));
    for (uint8_t i = 0; i < handles.glide_count; ++i) {
        handles.glides[i] = _to_glm(p_glides[i]);
    }
    core_tiling.set_symmetry_handles(handles);
    emit_signal("topology_changed");
    emit_changed();
}

godot::PackedVector2Array WallpaperTilingResource::get_glides() const {
    const auto &handles = core_tiling.get_symmetry_handles();
    godot::PackedVector2Array out;
    out.resize(handles.glide_count);
    for (uint8_t i = 0; i < handles.glide_count; ++i) {
        out.set(i, _to_godot(handles.glides[i]));
    }
    return out;
}

// ============================================================================
// Custom `Spline2DResource` Escher Edge Curves
// ============================================================================

void WallpaperTilingResource::set_edge_splines(const godot::TypedArray<Spline2DResource> &p_splines) {
    _disconnect_edge_spline_signals();
    edge_splines = p_splines;
    _connect_edge_spline_signals();
    _sync_edge_splines_to_core();
    emit_signal("prototile_changed");
    emit_changed();
}

void WallpaperTilingResource::set_edge_spline(int p_edge_shape_id, const godot::Ref<Spline2DResource> &p_spline) {
    if (p_edge_shape_id < 0 || p_edge_shape_id >= 6) return;
    _disconnect_edge_spline_signals();
    if (edge_splines.size() <= p_edge_shape_id) {
        edge_splines.resize(p_edge_shape_id + 1);
    }
    edge_splines[p_edge_shape_id] = p_spline;
    _connect_edge_spline_signals();
    _sync_edge_splines_to_core();
    emit_signal("prototile_changed");
    emit_changed();
}

godot::Ref<Spline2DResource> WallpaperTilingResource::get_edge_spline(int p_edge_shape_id) const {
    if (p_edge_shape_id < 0 || p_edge_shape_id >= edge_splines.size()) {
        return {};
    }
    return edge_splines[p_edge_shape_id];
}

TilingEdgeShapeType WallpaperTilingResource::get_edge_shape_constraint(int p_edge_shape_id) const {
    if (p_edge_shape_id < 0 || p_edge_shape_id >= 6) return EDGE_SHAPE_I;
    return static_cast<TilingEdgeShapeType>(
        core_tiling.get_edge_shape_constraint(static_cast<uint8_t>(p_edge_shape_id))
    );
}

void WallpaperTilingResource::clear_edge_splines() {
    _disconnect_edge_spline_signals();
    edge_splines.clear();
    core_tiling.clear_all_edge_splines();
    emit_signal("prototile_changed");
    emit_changed();
}

// ============================================================================
// Undo/Redo Committed Mutations
// ============================================================================

void WallpaperTilingResource::commit_set_wallpaper_group(WallpaperGroupType p_group) {
    const WallpaperGroupType prev = get_wallpaper_group();
    if (prev == p_group) return;
    godot::Ref<IdeamUndoRedo> ur = _resolve_undo_redo();
    ur->create_action("Change Wallpaper Group", godot::UndoRedo::MERGE_DISABLE, this);
    ur->add_do_method(godot::Callable(this, "set_wallpaper_group").bind(static_cast<int>(p_group)));
    ur->add_undo_method(godot::Callable(this, "set_wallpaper_group").bind(static_cast<int>(prev)));
    ur->commit_action(true);
}

void WallpaperTilingResource::commit_set_tactile_ih_type(int p_ih_type) {
    const int prev = get_tactile_ih_type();
    if (prev == p_ih_type) return;
    godot::Ref<IdeamUndoRedo> ur = _resolve_undo_redo();
    ur->create_action("Change Isohedral Tiling Type", godot::UndoRedo::MERGE_DISABLE, this);
    ur->add_do_method(godot::Callable(this, "set_tactile_ih_type").bind(p_ih_type));
    ur->add_undo_method(godot::Callable(this, "set_tactile_ih_type").bind(prev));
    ur->commit_action(true);
}

void WallpaperTilingResource::commit_set_shape_preset(TilingShapePresetType p_preset) {
    const TilingShapePresetType prev = get_shape_preset();
    if (prev == p_preset) return;
    godot::Ref<IdeamUndoRedo> ur = _resolve_undo_redo();
    ur->create_action("Change Tile Shape Preset", godot::UndoRedo::MERGE_DISABLE, this);
    ur->add_do_method(godot::Callable(this, "set_shape_preset").bind(static_cast<int>(p_preset)));
    ur->add_undo_method(godot::Callable(this, "set_shape_preset").bind(static_cast<int>(prev)));
    ur->commit_action(true);
}

void WallpaperTilingResource::commit_set_tactile_parameter(int p_index, real_t p_value) {
    if (p_index < 0 || p_index >= get_parameter_count()) return;
    const real_t prev = get_tactile_parameter(p_index);
    if (std::abs(prev - p_value) <= 1e-6f) return;
    godot::Ref<IdeamUndoRedo> ur = _resolve_undo_redo();
    ur->create_action("Adjust Tile Shape Parameter", godot::UndoRedo::MERGE_ENDS, this);
    ur->add_do_method(godot::Callable(this, "set_tactile_parameter").bind(p_index, p_value));
    ur->add_undo_method(godot::Callable(this, "set_tactile_parameter").bind(p_index, prev));
    ur->commit_action(true);
}

void WallpaperTilingResource::commit_set_lattice_scale(const godot::Vector2 &p_scale) {
    const godot::Vector2 prev = get_lattice_scale();
    if (prev.is_equal_approx(p_scale)) return;
    godot::Ref<IdeamUndoRedo> ur = _resolve_undo_redo();
    ur->create_action("Scale Tiling Lattice", godot::UndoRedo::MERGE_ENDS, this);
    ur->add_do_method(godot::Callable(this, "set_lattice_scale").bind(p_scale));
    ur->add_undo_method(godot::Callable(this, "set_lattice_scale").bind(prev));
    ur->commit_action(true);
}

void WallpaperTilingResource::commit_set_lattice_rotation_radians(real_t p_rotation_rad) {
    const real_t prev = get_lattice_rotation_radians();
    if (std::abs(prev - p_rotation_rad) <= 1e-6f) return;
    godot::Ref<IdeamUndoRedo> ur = _resolve_undo_redo();
    ur->create_action("Rotate Tiling Lattice", godot::UndoRedo::MERGE_ENDS, this);
    ur->add_do_method(godot::Callable(this, "set_lattice_rotation_radians").bind(p_rotation_rad));
    ur->add_undo_method(godot::Callable(this, "set_lattice_rotation_radians").bind(prev));
    ur->commit_action(true);
}

void WallpaperTilingResource::commit_set_lattice_origin(const godot::Vector2 &p_origin) {
    const godot::Vector2 prev = get_lattice_origin();
    if (prev.is_equal_approx(p_origin)) return;
    godot::Ref<IdeamUndoRedo> ur = _resolve_undo_redo();
    ur->create_action("Move Tiling Lattice Origin", godot::UndoRedo::MERGE_ENDS, this);
    ur->add_do_method(godot::Callable(this, "set_lattice_origin").bind(p_origin));
    ur->add_undo_method(godot::Callable(this, "set_lattice_origin").bind(prev));
    ur->commit_action(true);
}

void WallpaperTilingResource::commit_set_edge_spline(int p_edge_shape_id, const godot::Ref<Spline2DResource> &p_spline) {
    const godot::Ref<Spline2DResource> prev = get_edge_spline(p_edge_shape_id);
    if (prev == p_spline) return;
    godot::Ref<IdeamUndoRedo> ur = _resolve_undo_redo();
    ur->create_action("Assign Tile Edge Spline", godot::UndoRedo::MERGE_DISABLE, this);
    ur->add_do_method(godot::Callable(this, "set_edge_spline").bind(p_edge_shape_id, p_spline));
    ur->add_undo_method(godot::Callable(this, "set_edge_spline").bind(p_edge_shape_id, prev));
    ur->commit_action(true);
}

// ============================================================================
// Direct Lattice, Aspect & Quotient Queries
// ============================================================================

godot::PackedVector2Array WallpaperTilingResource::get_base_prototile_vertices() const {
    const auto span = core_tiling.get_base_prototile_vertices();
    godot::PackedVector2Array out;
    out.resize(static_cast<int>(span.size()));
    for (size_t i = 0; i < span.size(); ++i) {
        out.set(static_cast<int>(i), _to_godot(span[i]));
    }
    return out;
}

godot::Transform2D WallpaperTilingResource::get_cell_transform_2d(int p_t1, int p_t2, int p_aspect) const {
    const auto m = core_tiling.get_cell_world_matrix(
        static_cast<int32_t>(p_t1),
        static_cast<int32_t>(p_t2),
        static_cast<uint8_t>(std::max(0, p_aspect))
    );
    return godot::Transform2D(
        static_cast<float>(m[0][0]), static_cast<float>(m[0][1]),
        static_cast<float>(m[1][0]), static_cast<float>(m[1][1]),
        static_cast<float>(m[2][0]), static_cast<float>(m[2][1])
    );
}

godot::PackedVector2Array WallpaperTilingResource::get_cell_world_polygon(
    int p_t1,
    int p_t2,
    int p_aspect,
    bool p_use_deformed_edges,
    int p_subdivisions
) const {
    const auto pts = core_tiling.get_cell_world_polygon(
        static_cast<int32_t>(p_t1),
        static_cast<int32_t>(p_t2),
        static_cast<uint8_t>(std::max(0, p_aspect)),
        p_use_deformed_edges,
        p_subdivisions,
        true
    );
    godot::PackedVector2Array out;
    out.resize(static_cast<int>(pts.size()));
    for (size_t i = 0; i < pts.size(); ++i) {
        out.set(static_cast<int>(i), _to_godot(pts[i]));
    }
    return out;
}

int WallpaperTilingResource::get_cell_colour(int p_t1, int p_t2, int p_aspect) const {
    return static_cast<int>(core_tiling.get_colour(
        static_cast<int32_t>(p_t1),
        static_cast<int32_t>(p_t2),
        static_cast<uint8_t>(std::max(0, p_aspect))
    ));
}

godot::Vector3i WallpaperTilingResource::locate_cell_at_world_point(const godot::Vector2 &p_world_point) const {
    int32_t t1 = 0, t2 = 0;
    uint8_t asp = 0u;
    if (core_tiling.locate_cell_at_world_point(_to_glm(p_world_point), t1, t2, asp)) {
        return godot::Vector3i(t1, t2, static_cast<int32_t>(asp));
    }
    return godot::Vector3i(0, 0, -1);
}

godot::Vector3i WallpaperTilingResource::step_to_neighbor(int p_t1, int p_t2, int p_aspect, int p_edge_slot) const {
    int32_t next_t1 = p_t1, next_t2 = p_t2;
    uint8_t next_asp = 0u;
    if (core_tiling.step_to_neighbor(
            static_cast<int32_t>(p_t1),
            static_cast<int32_t>(p_t2),
            static_cast<uint8_t>(std::max(0, p_aspect)),
            static_cast<uint8_t>(std::max(0, p_edge_slot)),
            next_t1,
            next_t2,
            next_asp)) {
        return godot::Vector3i(next_t1, next_t2, static_cast<int32_t>(next_asp));
    }
    return godot::Vector3i(p_t1, p_t2, -1);
}

godot::PackedInt32Array WallpaperTilingResource::get_all_valid_ih_types() {
    const auto span = CoreTiling::get_all_ih_types();
    godot::PackedInt32Array out;
    out.resize(static_cast<int>(span.size()));
    for (size_t i = 0; i < span.size(); ++i) {
        out.set(static_cast<int>(i), static_cast<int32_t>(span[i]));
    }
    return out;
}

godot::PackedInt32Array WallpaperTilingResource::get_ih_types_for_group(WallpaperGroupType p_group) {
    const auto list = CoreTiling::get_ih_types_for_wallpaper_group(
        static_cast<core::WallpaperGroupId>(std::clamp<int>(p_group, 0, 16))
    );
    godot::PackedInt32Array out;
    out.resize(static_cast<int>(list.size()));
    for (size_t i = 0; i < list.size(); ++i) {
        out.set(static_cast<int>(i), static_cast<int32_t>(list[i]));
    }
    return out;
}

} // namespace ideam::godot_ext