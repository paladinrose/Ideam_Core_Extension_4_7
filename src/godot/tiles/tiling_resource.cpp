#include "tiling_resource.h"

namespace ideam::godot_ext {

void TilingResource::_bind_methods() {
    using namespace godot;

    // --- Wallpaper Group Enum Constants (17 Groups) ---
    BIND_ENUM_CONSTANT(WALLPAPER_P1);
    BIND_ENUM_CONSTANT(WALLPAPER_P2);
    BIND_ENUM_CONSTANT(WALLPAPER_PM);
    BIND_ENUM_CONSTANT(WALLPAPER_PG);
    BIND_ENUM_CONSTANT(WALLPAPER_CM);
    BIND_ENUM_CONSTANT(WALLPAPER_PMM);
    BIND_ENUM_CONSTANT(WALLPAPER_PMG);
    BIND_ENUM_CONSTANT(WALLPAPER_PGG);
    BIND_ENUM_CONSTANT(WALLPAPER_CMM);
    BIND_ENUM_CONSTANT(WALLPAPER_P4);
    BIND_ENUM_CONSTANT(WALLPAPER_P4M);
    BIND_ENUM_CONSTANT(WALLPAPER_P4G);
    BIND_ENUM_CONSTANT(WALLPAPER_P3);
    BIND_ENUM_CONSTANT(WALLPAPER_P3M1);
    BIND_ENUM_CONSTANT(WALLPAPER_P31M);
    BIND_ENUM_CONSTANT(WALLPAPER_P6);
    BIND_ENUM_CONSTANT(WALLPAPER_P6M);

    // --- Bravais Lattice Family Constants ---
    BIND_ENUM_CONSTANT(LATTICE_OBLIQUE);
    BIND_ENUM_CONSTANT(LATTICE_RECTANGULAR);
    BIND_ENUM_CONSTANT(LATTICE_RHOMBIC);
    BIND_ENUM_CONSTANT(LATTICE_SQUARE);
    BIND_ENUM_CONSTANT(LATTICE_HEXAGONAL);

    // --- Authoring Mode & Preset Constants ---
    BIND_ENUM_CONSTANT(TILING_MODE_WALLPAPER_17);
    BIND_ENUM_CONSTANT(TILING_MODE_ISOHEDRAL_81);

    BIND_ENUM_CONSTANT(SHAPE_PRESET_TACTILE_DEFAULT);
    BIND_ENUM_CONSTANT(SHAPE_PRESET_QUAD);
    BIND_ENUM_CONSTANT(SHAPE_PRESET_BRICK_HEX);
    BIND_ENUM_CONSTANT(SHAPE_PRESET_RIGHT_TRIANGLE);
    BIND_ENUM_CONSTANT(SHAPE_PRESET_EQUILATERAL_TRIANGLE);
    BIND_ENUM_CONSTANT(SHAPE_PRESET_FLAT_TRIANGLE);
    BIND_ENUM_CONSTANT(SHAPE_PRESET_TALL_TRIANGLE);
    BIND_ENUM_CONSTANT(SHAPE_PRESET_RHOMBUS_60);

    // --- Tactile Edge Shape Constants (J, U, S, I) ---
    BIND_ENUM_CONSTANT(EDGE_SHAPE_J);
    BIND_ENUM_CONSTANT(EDGE_SHAPE_U);
    BIND_ENUM_CONSTANT(EDGE_SHAPE_S);
    BIND_ENUM_CONSTANT(EDGE_SHAPE_I);

    // --- Coverage, Stitch & Port Direction Constants ---
    BIND_ENUM_CONSTANT(COVERAGE_STRICT_INTERIOR);
    BIND_ENUM_CONSTANT(COVERAGE_CENTROID_INSIDE);
    BIND_ENUM_CONSTANT(COVERAGE_INCLUDE_STRADDLING);
    BIND_ENUM_CONSTANT(COVERAGE_CLIP_EXACT_TO_SPLINE);

    BIND_ENUM_CONSTANT(STITCH_QUAD_BRIDGE);
    BIND_ENUM_CONSTANT(STITCH_TOPOLOGICAL_SEAM_WELD);
    BIND_ENUM_CONSTANT(STITCH_CURVILINEAR_METRIC_MORPH);

    BIND_ENUM_CONSTANT(PORT_DIR_FREE);
    BIND_ENUM_CONSTANT(PORT_DIR_IN_ONLY);
    BIND_ENUM_CONSTANT(PORT_DIR_OUT_ONLY);
    BIND_ENUM_CONSTANT(PORT_DIR_CLOSED);

    // --- Signals ---
    ADD_SIGNAL(MethodInfo("topology_changed"));
    ADD_SIGNAL(MethodInfo("prototile_changed"));

    // --- Shared Polymorphic Methods ---
    ClassDB::bind_method(D_METHOD("get_vertex_count"), &TilingResource::get_vertex_count);
    ClassDB::bind_method(D_METHOD("get_aspect_count"), &TilingResource::get_aspect_count);
    ClassDB::bind_method(D_METHOD("get_edge_shape_count"), &TilingResource::get_edge_shape_count);
    ClassDB::bind_method(
        D_METHOD("get_prototile_outline", "subdivisions"),
        &TilingResource::get_prototile_outline,
        DEFVAL(12)
    );
    ClassDB::bind_method(
        D_METHOD("get_prototile_triangles", "subdivisions"),
        &TilingResource::get_prototile_triangles,
        DEFVAL(12)
    );
}

TilingResource::TilingResource() = default;

TilingResource::~TilingResource() = default;

} // namespace ideam::godot_ext