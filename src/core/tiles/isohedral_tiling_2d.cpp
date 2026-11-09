#include "isohedral_tiling_2d.h"
#include "../../external/tactile/tiling.hpp"
#include <glm/gtc/matrix_inverse.hpp>
#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_set>

namespace ideam::core {

// ============================================================================
// Static Catalogue & Wallpaper <-> Tactile IH Lookup Helpers
// ============================================================================

template <FloatingPointScalar T>
bool IsohedralTiling2D<T>::is_valid_ih_type(uint8_t p_ih_type) noexcept {
    for (size_t i = 0; i < csk::num_types; ++i) {
        if (csk::tiling_types[i] == p_ih_type) {
            return true;
        }
    }
    return false;
}

template <FloatingPointScalar T>
std::span<const uint8_t> IsohedralTiling2D<T>::get_all_ih_types() noexcept {
    return std::span<const uint8_t>(csk::tiling_types, csk::num_types);
}

template <FloatingPointScalar T>
WallpaperGroupId IsohedralTiling2D<T>::map_ih_type_to_wallpaper_group(uint8_t p_ih_type) noexcept {
    // Canonical Grunbaum & Shephard / Kaplan Tactile classification of IH01..IH93 into the 17 Wallpaper Groups
    switch (p_ih_type) {
        case 1:  case 8:  case 10: case 12: case 14: case 22: case 33: case 41: case 57: case 64: case 68:
            return WallpaperGroupId::P1;
        case 2:  case 4:  case 23: case 24: case 34: case 46: case 47: case 58: case 66: case 83: case 84:
            return WallpaperGroupId::P2;
        case 13: case 28: case 42: case 61: case 69:
            return WallpaperGroupId::PM;
        case 3:  case 9:  case 15: case 25: case 27: case 43: case 45: case 59: case 71: case 85:
            return WallpaperGroupId::PG;
        case 11: case 16: case 29: case 44: case 62: case 72: case 74: case 86:
            return WallpaperGroupId::CM;
        case 50: case 67: case 73:
            return WallpaperGroupId::PMM;
        case 5:  case 17: case 49: case 53: case 76: case 88:
            return WallpaperGroupId::PMG;
        case 6:  case 18: case 26: case 51: case 54: case 78: case 90: case 91:
            return WallpaperGroupId::PGG;
        case 52: case 79: case 82:
            return WallpaperGroupId::CMM;
        case 20: case 55: case 63:
            return WallpaperGroupId::P4;
        case 75: case 81:
            return WallpaperGroupId::P4M;
        case 56: case 80: case 93:
            return WallpaperGroupId::P4G;
        case 7:  case 32:
            return WallpaperGroupId::P3;
        case 36: case 38: case 40:
            return WallpaperGroupId::P3M1;
        case 30: case 37: case 39:
            return WallpaperGroupId::P31M;
        case 21: case 31:
            return WallpaperGroupId::P6;
        case 77:
            return WallpaperGroupId::P6M;
        default:
            break;
    }

    // Fallback inspection via aspect count and orientation signs if an unlisted ID is passed
    if (!is_valid_ih_type(p_ih_type)) {
        return WallpaperGroupId::P1;
    }
    csk::IsohedralTiling probe(p_ih_type);
    const uint8_t na = probe.numAspects();
    if (na == 12) return WallpaperGroupId::P6M;
    if (na == 8)  return WallpaperGroupId::P4M;
    if (na == 6)  return WallpaperGroupId::P6;
    if (na == 3)  return WallpaperGroupId::P3;
    if (na == 4)  return WallpaperGroupId::PMM;
    if (na == 2)  return WallpaperGroupId::P2;
    return WallpaperGroupId::P1;
}

template <FloatingPointScalar T>
std::vector<uint8_t> IsohedralTiling2D<T>::get_ih_types_for_wallpaper_group(WallpaperGroupId p_group) {
    std::vector<uint8_t> matching;
    for (size_t i = 0; i < csk::num_types; ++i) {
        const uint8_t ih = csk::tiling_types[i];
        if (map_ih_type_to_wallpaper_group(ih) == p_group) {
            matching.push_back(ih);
        }
    }
    if (matching.empty()) {
        matching.push_back(SymmetryMath2D<T>::get_group_descriptor(p_group).primary_tactile_ih_type);
    }
    return matching;
}

template <FloatingPointScalar T>
constexpr FundamentalShapePreset IsohedralTiling2D<T>::get_default_legacy_shape_preset(WallpaperGroupId p_group) noexcept {
    switch (p_group) {
        case WallpaperGroupId::P1:
        case WallpaperGroupId::P2:
        case WallpaperGroupId::PM:
        case WallpaperGroupId::PMM:
        case WallpaperGroupId::PMG:
        case WallpaperGroupId::PGG:
        case WallpaperGroupId::P4:
            return FundamentalShapePreset::QUAD;
        case WallpaperGroupId::PG:
            return FundamentalShapePreset::BRICK_HEX;
        case WallpaperGroupId::CM:
        case WallpaperGroupId::P31M:
        case WallpaperGroupId::P6:
            return FundamentalShapePreset::FLAT_TRIANGLE;
        case WallpaperGroupId::CMM:
        case WallpaperGroupId::P4M:
        case WallpaperGroupId::P4G:
            return FundamentalShapePreset::RIGHT_TRIANGLE;
        case WallpaperGroupId::P3:
            return FundamentalShapePreset::RHOMBUS_60;
        case WallpaperGroupId::P3M1:
            return FundamentalShapePreset::EQUILATERAL_TRIANGLE;
        case WallpaperGroupId::P6M:
            return FundamentalShapePreset::TALL_TRIANGLE;
        default:
            return FundamentalShapePreset::QUAD;
    }
}

template <FloatingPointScalar T>
std::vector<typename IsohedralTiling2D<T>::Vec2> IsohedralTiling2D<T>::get_legacy_primitive_vertices(
    FundamentalShapePreset p_preset,
    T p_scale
) {
    switch (p_preset) {
        case FundamentalShapePreset::QUAD:
        case FundamentalShapePreset::TACTILE_DEFAULT:
            return {
                Vec2(static_cast<T>(-0.5), static_cast<T>(-0.5)) * p_scale,
                Vec2(static_cast<T>(-0.5), static_cast<T>( 0.5)) * p_scale,
                Vec2(static_cast<T>( 0.5), static_cast<T>( 0.5)) * p_scale,
                Vec2(static_cast<T>( 0.5), static_cast<T>(-0.5)) * p_scale
            };
        case FundamentalShapePreset::RHOMBUS_60:
            return {
                Vec2(static_cast<T>(-0.5),     static_cast<T>(-0.28867513)) * p_scale,
                Vec2(static_cast<T>(-0.166667),static_cast<T>( 0.28867513)) * p_scale,
                Vec2(static_cast<T>( 0.5),     static_cast<T>( 0.28867513)) * p_scale,
                Vec2(static_cast<T>( 0.166667),static_cast<T>(-0.28867513)) * p_scale
            };
        case FundamentalShapePreset::RIGHT_TRIANGLE:
            return {
                Vec2(static_cast<T>(-0.5), static_cast<T>(-0.5)) * p_scale,
                Vec2(static_cast<T>( 0.5), static_cast<T>( 0.5)) * p_scale,
                Vec2(static_cast<T>( 0.5), static_cast<T>(-0.5)) * p_scale
            };
        case FundamentalShapePreset::EQUILATERAL_TRIANGLE:
            return {
                Vec2(static_cast<T>(-0.5), static_cast<T>(-0.28867513)) * p_scale,
                Vec2(static_cast<T>( 0.0), static_cast<T>( 0.57735027)) * p_scale,
                Vec2(static_cast<T>( 0.5), static_cast<T>(-0.28867513)) * p_scale
            };
        case FundamentalShapePreset::TALL_TRIANGLE:
            return {
                Vec2(static_cast<T>(-0.28867513), static_cast<T>(-0.5)) * p_scale,
                Vec2(static_cast<T>( 0.28867513), static_cast<T>( 0.5)) * p_scale,
                Vec2(static_cast<T>( 0.28867513), static_cast<T>(-0.5)) * p_scale
            };
        case FundamentalShapePreset::FLAT_TRIANGLE:
            return {
                Vec2(static_cast<T>(-0.5), static_cast<T>(-0.14433757)) * p_scale,
                Vec2(static_cast<T>( 0.0), static_cast<T>( 0.14433757)) * p_scale,
                Vec2(static_cast<T>( 0.5), static_cast<T>(-0.14433757)) * p_scale
            };
        case FundamentalShapePreset::BRICK_HEX:
            return {
                Vec2(static_cast<T>(-0.5), static_cast<T>(-0.5)) * p_scale,
                Vec2(static_cast<T>(-0.5), static_cast<T>( 0.5)) * p_scale,
                Vec2(static_cast<T>( 0.0), static_cast<T>( 0.5)) * p_scale,
                Vec2(static_cast<T>( 0.5), static_cast<T>( 0.5)) * p_scale,
                Vec2(static_cast<T>( 0.5), static_cast<T>(-0.5)) * p_scale,
                Vec2(static_cast<T>( 0.0), static_cast<T>(-0.5)) * p_scale
            };
    }
    return {};
}

// ============================================================================
// Constructors & Mode Configuration
// ============================================================================

template <FloatingPointScalar T>
IsohedralTiling2D<T>::IsohedralTiling2D() noexcept {
    reset_from_wallpaper_group(WallpaperGroupId::P1, FundamentalShapePreset::TACTILE_DEFAULT);
}

template <FloatingPointScalar T>
IsohedralTiling2D<T>::IsohedralTiling2D(
    WallpaperGroupId p_group,
    FundamentalShapePreset p_preset
) noexcept {
    reset_from_wallpaper_group(p_group, p_preset);
}

template <FloatingPointScalar T>
IsohedralTiling2D<T>::IsohedralTiling2D(uint8_t p_tactile_ih_type) noexcept {
    if (!reset_from_ih_type(p_tactile_ih_type)) {
        reset_from_wallpaper_group(WallpaperGroupId::P1, FundamentalShapePreset::TACTILE_DEFAULT);
    }
}

template <FloatingPointScalar T>
void IsohedralTiling2D<T>::reset_from_wallpaper_group(
    WallpaperGroupId p_group,
    FundamentalShapePreset p_preset
) noexcept {
    authoring_mode_  = TilingAuthoringMode::WALLPAPER_GROUP_17;
    wallpaper_group_ = p_group;
    shape_preset_    = p_preset;
    tactile_ih_type_ = SymmetryMath2D<T>::get_group_descriptor(p_group).primary_tactile_ih_type;

    _rebuild_default_symmetry_handles();
    _rebuild_from_tactile(nullptr);
}

template <FloatingPointScalar T>
bool IsohedralTiling2D<T>::reset_from_ih_type(uint8_t p_ih_type) noexcept {
    if (!is_valid_ih_type(p_ih_type)) {
        return false;
    }
    authoring_mode_  = TilingAuthoringMode::ISOHEDRAL_TACTILE_81;
    tactile_ih_type_ = p_ih_type;
    wallpaper_group_ = map_ih_type_to_wallpaper_group(p_ih_type);
    shape_preset_    = FundamentalShapePreset::TACTILE_DEFAULT;

    _rebuild_default_symmetry_handles();
    _rebuild_from_tactile(nullptr);
    return true;
}

template <FloatingPointScalar T>
void IsohedralTiling2D<T>::set_shape_preset(FundamentalShapePreset p_preset) noexcept {
    if (shape_preset_ == p_preset) return;
    shape_preset_  = p_preset;
    outline_dirty_ = true;
}

// ============================================================================
// Internal Tactile & Symmetry Rebuild Pipeline
// ============================================================================

template <FloatingPointScalar T>
void IsohedralTiling2D<T>::_rebuild_default_symmetry_handles() noexcept {
    symmetry_handles_ = HandlesPOD{};
    const Vec2 major_axis(static_cast<T>(0.0), static_cast<T>(1.0));
    const Vec2 minor_axis(static_cast<T>(1.0), static_cast<T>(0.0));

    symmetry_handles_.axes_of_symmetry[0] = major_axis;
    symmetry_handles_.axes_of_symmetry[1] = minor_axis;
    symmetry_handles_.axis_count          = 2u;

    switch (wallpaper_group_) {
        case WallpaperGroupId::P2:
        case WallpaperGroupId::PMG:
            symmetry_handles_.points_of_rotation[0] =  minor_axis * static_cast<T>(0.5);
            symmetry_handles_.points_of_rotation[1] = -minor_axis * static_cast<T>(0.5);
            symmetry_handles_.rotation_pivot_count  = 2u;
            break;
        case WallpaperGroupId::PG:
        case WallpaperGroupId::CM:
            symmetry_handles_.glides[0]   = minor_axis * static_cast<T>(0.5);
            symmetry_handles_.glide_count = 1u;
            break;
        case WallpaperGroupId::PGG:
        case WallpaperGroupId::CMM:
            symmetry_handles_.points_of_rotation[0] =  major_axis * static_cast<T>(0.5);
            symmetry_handles_.points_of_rotation[1] = -major_axis * static_cast<T>(0.5);
            symmetry_handles_.rotation_pivot_count  = 2u;
            symmetry_handles_.glides[0]             =  minor_axis;
            symmetry_handles_.glides[1]             = -minor_axis;
            symmetry_handles_.glide_count           = 2u;
            break;
        case WallpaperGroupId::P4:
        case WallpaperGroupId::P4M:
        case WallpaperGroupId::P4G:
            symmetry_handles_.points_of_rotation[0] = Vec2(static_cast<T>(0.0));
            symmetry_handles_.points_of_rotation[1] = major_axis - minor_axis;
            symmetry_handles_.points_of_rotation[2] = major_axis + minor_axis;
            symmetry_handles_.points_of_rotation[3] = -major_axis + minor_axis;
            symmetry_handles_.points_of_rotation[4] = -major_axis - minor_axis;
            symmetry_handles_.rotation_pivot_count  = 5u;
            break;
        case WallpaperGroupId::P3:
        case WallpaperGroupId::P3M1:
        case WallpaperGroupId::P31M:
        case WallpaperGroupId::P6:
        case WallpaperGroupId::P6M:
            symmetry_handles_.points_of_rotation[0] = Vec2(static_cast<T>(0.0), static_cast<T>(0.57735027));
            symmetry_handles_.points_of_rotation[1] = Vec2(static_cast<T>(-0.5), static_cast<T>(-0.28867513));
            symmetry_handles_.points_of_rotation[2] = Vec2(static_cast<T>( 0.5), static_cast<T>(-0.28867513));
            symmetry_handles_.rotation_pivot_count  = 3u;
            break;
        default:
            break;
    }
}

template <FloatingPointScalar T>
void IsohedralTiling2D<T>::_rebuild_from_tactile(const double* p_override_params) noexcept {
    if (!is_valid_ih_type(tactile_ih_type_)) {
        tactile_ih_type_ = 1u;
    }

    csk::IsohedralTiling t(tactile_ih_type_);
    if (p_override_params != nullptr && t.numParameters() > 0) {
        t.setParameters(p_override_params);
    }

    baked_ = BakedPOD{};
    baked_.wallpaper_group = wallpaper_group_;
    baked_.tactile_ih_type = tactile_ih_type_;
    baked_.num_vertices    = std::min<uint8_t>(t.numVertices(), BakedPOD::MAX_VERTICES);
    baked_.num_aspects     = std::min<uint8_t>(t.numAspects(), BakedPOD::MAX_ASPECTS);
    baked_.num_params      = std::min<uint8_t>(t.numParameters(), BakedPOD::MAX_PARAMETERS);
    baked_.num_edge_shapes = std::min<uint8_t>(t.numEdgeShapes(), BakedPOD::MAX_VERTICES);
    baked_.shape_preset    = shape_preset_;

    // Copy parameters
    double raw_params[BakedPOD::MAX_PARAMETERS] = {0.0};
    t.getParameters(raw_params);
    for (uint8_t i = 0; i < baked_.num_params; ++i) {
        baked_.parameters[i] = static_cast<T>(raw_params[i]);
    }

    // Build global lattice scale + rotation matrix
    const T cos_r = std::cos(lattice_rotation_rad_);
    const T sin_r = std::sin(lattice_rotation_rad_);
    const Mat2 rot_scale(
        Vec2(cos_r * lattice_scale_.x,  sin_r * lattice_scale_.x),
        Vec2(-sin_r * lattice_scale_.y, cos_r * lattice_scale_.y)
    );
    Mat3 world_frame(static_cast<T>(1.0));
    world_frame[0][0] = rot_scale[0][0];
    world_frame[0][1] = rot_scale[0][1];
    world_frame[1][0] = rot_scale[1][0];
    world_frame[1][1] = rot_scale[1][1];
    world_frame[2][0] = lattice_origin_.x;
    world_frame[2][1] = lattice_origin_.y;

    // Copy prototile vertices (scaled by lattice_scale_)
    for (uint8_t i = 0; i < baked_.num_vertices; ++i) {
        const glm::dvec2& dv = t.getVertex(i);
        baked_.base_vertices[i] = Vec2(static_cast<T>(dv.x), static_cast<T>(dv.y));
    }

    // Copy edge shape descriptors
    size_t e_idx = 0;
    for (auto it = t.beginShape(); it != t.endShape(); ++it, ++e_idx) {
        if (e_idx >= baked_.num_vertices) break;

        PrototileEdgeDescriptorPOD<T>& ed = baked_.edges[e_idx];
        ed.edge_shape_id = it.getId();
        ed.shape         = static_cast<TactileEdgeShape>(it.getShape());
        ed.reversed      = it.isReversed();
        ed.start_vertex  = baked_.base_vertices[e_idx];
        ed.end_vertex    = baked_.base_vertices[(e_idx + 1u) % baked_.num_vertices];

        const glm::dmat3& dm = it.getTransform();
        for (int c = 0; c < 3; ++c) {
            for (int r = 0; r < 3; ++r) {
                ed.canonical_to_edge_xform[c][r] = static_cast<T>(dm[c][r]);
            }
        }
    }

    // Copy translational basis vectors t1, t2 transformed by rot_scale
    const Vec2 raw_t1(static_cast<T>(t.getT1().x), static_cast<T>(t.getT1().y));
    const Vec2 raw_t2(static_cast<T>(t.getT2().x), static_cast<T>(t.getT2().y));
    baked_.lattice_basis = SymmetryMath2D<T>::make_lattice_basis(
        rot_scale * raw_t1,
        rot_scale * raw_t2,
        lattice_origin_
    );

    // Copy and transform aspect matrices
    Mat3 linear_rot_scale = world_frame;
    linear_rot_scale[2][0] = static_cast<T>(0.0);
    linear_rot_scale[2][1] = static_cast<T>(0.0);

    for (uint8_t a = 0; a < baked_.num_aspects; ++a) {
        const glm::dmat3& dm = t.getAspectTransform(a);
        Mat3 raw_asp(static_cast<T>(1.0));
        for (int c = 0; c < 3; ++c) {
            for (int r = 0; r < 3; ++r) {
                raw_asp[c][r] = static_cast<T>(dm[c][r]);
            }
        }
        baked_.aspect_xforms[a]     = linear_rot_scale * raw_asp;
        baked_.inv_aspect_xforms[a] = glm::inverse(baked_.aspect_xforms[a]);
    }

    // Copy 19-byte colouring table from raw type data
    if (const csk::TilingTypeData* ttd = t.getRawTypeData()) {
        if (ttd->colouring != nullptr) {
            for (size_t i = 0; i < 19; ++i) {
                baked_.colouring_table[i] = ttd->colouring[i];
            }
        }
    }

    _decompose_aspect_transforms();
    _compute_quotient_adjacency();
    outline_dirty_ = true;
}

template <FloatingPointScalar T>
void IsohedralTiling2D<T>::_decompose_aspect_transforms() noexcept {
    // Compute local centroid of base prototile vertices
    const std::span<const Vec2> base_verts(baked_.base_vertices.data(), baked_.num_vertices);
    const Vec2 proto_centroid = PolygonMath2D<T>::centroid(base_verts);

    for (uint8_t a = 0; a < baked_.num_aspects; ++a) {
        const Mat3& m = baked_.aspect_xforms[a];
        baked_.aspect_local_centers[a] = SymmetryMath2D<T>::transform_point(m, proto_centroid);

        const T det = m[0][0] * m[1][1] - m[1][0] * m[0][1];
        baked_.aspect_orientations[a] = (det >= static_cast<T>(0.0)) ? int8_t{1} : int8_t{-1};

        const Vec2 col0(m[0][0], m[0][1]);
        const Vec2 col1(m[1][0], m[1][1]);
        const T sx = glm::length(col0);
        const T sy = glm::length(col1) * static_cast<T>(baked_.aspect_orientations[a]);

        baked_.aspect_scales[a]     = Vec2(sx, sy);
        baked_.aspect_angles_rad[a] = std::atan2(col0.y, col0.x);
    }
}

template <FloatingPointScalar T>
void IsohedralTiling2D<T>::_compute_quotient_adjacency() noexcept {
    const uint8_t nv = baked_.num_vertices;
    const uint8_t na = baked_.num_aspects;
    if (nv < 3 || na == 0) return;

    // Precompute edge midpoints for each aspect in unit cell (0, 0)
    std::array<std::array<Vec2, BakedPOD::MAX_VERTICES>, BakedPOD::MAX_ASPECTS> edge_mids_00{};
    for (uint8_t a = 0; a < na; ++a) {
        const Mat3& m = baked_.aspect_xforms[a];
        for (uint8_t e = 0; e < nv; ++e) {
            const Vec2 p0 = SymmetryMath2D<T>::transform_point(m, baked_.base_vertices[e]);
            const Vec2 p1 = SymmetryMath2D<T>::transform_point(m, baked_.base_vertices[(e + 1u) % nv]);
            edge_mids_00[a][e] = (p0 + p1) * static_cast<T>(0.5);
        }
    }

    const Vec2& t1 = baked_.lattice_basis.t1;
    const Vec2& t2 = baked_.lattice_basis.t2;

    // Match each edge midpoint of (0, 0, a, e) against all candidate neighbors in [-2, 2]^2
    for (uint8_t a = 0; a < na; ++a) {
        for (uint8_t e = 0; e < nv; ++e) {
            QuotientNeighborStepPOD step{};
            const Vec2& target_mid = edge_mids_00[a][e];
            T best_dist_sq = GeometryConstants<T>::WELD_EPSILON * static_cast<T>(25.0);

            for (int di = -2; di <= 2; ++di) {
                for (int dj = -2; dj <= 2; ++dj) {
                    const Vec2 cell_shift = t1 * static_cast<T>(di) + t2 * static_cast<T>(dj);
                    for (uint8_t b = 0; b < na; ++b) {
                        if (di == 0 && dj == 0 && b == a) continue;
                        for (uint8_t e2 = 0; e2 < nv; ++e2) {
                            const Vec2 cand_mid = edge_mids_00[b][e2] + cell_shift;
                            const Vec2 diff = target_mid - cand_mid;
                            const T d2 = glm::dot(diff, diff);
                            if (d2 < best_dist_sq) {
                                best_dist_sq          = d2;
                                step.delta_i          = static_cast<int8_t>(di);
                                step.delta_j          = static_cast<int8_t>(dj);
                                step.next_aspect      = b;
                                step.shared_edge_slot = e2;
                                step.is_valid         = 1u;
                                step.reflection_change= (baked_.aspect_orientations[a] != baked_.aspect_orientations[b]) ? 1u : 0u;

                                const Vec2 next_center = baked_.aspect_local_centers[b] + cell_shift;
                                const T center_dist    = glm::distance(baked_.aspect_local_centers[a], next_center);
                                step.base_cost_fp8     = static_cast<uint16_t>(std::clamp(
                                    std::round(center_dist * static_cast<T>(256.0)),
                                    static_cast<T>(1.0),
                                    static_cast<T>(65535.0)
                                ));
                            }
                        }
                    }
                }
            }

            baked_.quotient_adjacency[a][e] = step;
        }
    }
}

// ============================================================================
// Parameter & Handle Mutators
// ============================================================================

template <FloatingPointScalar T>
void IsohedralTiling2D<T>::set_parameters(std::span<const T> p_params) noexcept {
    if (baked_.num_params == 0) return;
    double raw[BakedPOD::MAX_PARAMETERS] = {0.0};
    for (uint8_t i = 0; i < baked_.num_params; ++i) {
        raw[i] = (i < p_params.size())
            ? static_cast<double>(p_params[i])
            : static_cast<double>(baked_.parameters[i]);
    }
    _rebuild_from_tactile(raw);
}

template <FloatingPointScalar T>
void IsohedralTiling2D<T>::set_parameter(size_t p_index, T p_value) noexcept {
    if (p_index >= baked_.num_params) return;
    double raw[BakedPOD::MAX_PARAMETERS] = {0.0};
    for (uint8_t i = 0; i < baked_.num_params; ++i) {
        raw[i] = static_cast<double>(baked_.parameters[i]);
    }
    raw[p_index] = static_cast<double>(p_value);
    _rebuild_from_tactile(raw);
}

template <FloatingPointScalar T>
T IsohedralTiling2D<T>::get_parameter(size_t p_index) const noexcept {
    return (p_index < baked_.num_params) ? baked_.parameters[p_index] : static_cast<T>(0.0);
}

template <FloatingPointScalar T>
void IsohedralTiling2D<T>::set_lattice_transform(
    const Vec2& p_scale,
    T p_rotation_rad,
    const Vec2& p_origin
) noexcept {
    lattice_scale_.x = (std::abs(p_scale.x) > GeometryConstants<T>::EPSILON) ? p_scale.x : static_cast<T>(1.0);
    lattice_scale_.y = (std::abs(p_scale.y) > GeometryConstants<T>::EPSILON) ? p_scale.y : static_cast<T>(1.0);
    lattice_rotation_rad_ = p_rotation_rad;
    lattice_origin_       = p_origin;

    double raw[BakedPOD::MAX_PARAMETERS] = {0.0};
    for (uint8_t i = 0; i < baked_.num_params; ++i) {
        raw[i] = static_cast<double>(baked_.parameters[i]);
    }
    _rebuild_from_tactile(baked_.num_params > 0 ? raw : nullptr);
}

template <FloatingPointScalar T>
void IsohedralTiling2D<T>::set_symmetry_handles(const HandlesPOD& p_handles) noexcept {
    symmetry_handles_ = p_handles;
    // If major/minor axes are provided, align lattice basis scale/rotation with them
    if (symmetry_handles_.axis_count >= 2) {
        const Vec2& ax0 = symmetry_handles_.axes_of_symmetry[0];
        const Vec2& ax1 = symmetry_handles_.axes_of_symmetry[1];
        const T len0 = glm::length(ax0);
        const T len1 = glm::length(ax1);
        if (len0 > GeometryConstants<T>::EPSILON && len1 > GeometryConstants<T>::EPSILON) {
            lattice_scale_ = Vec2(len1, len0);
            lattice_rotation_rad_ = std::atan2(ax1.y, ax1.x);
            double raw[BakedPOD::MAX_PARAMETERS] = {0.0};
            for (uint8_t i = 0; i < baked_.num_params; ++i) {
                raw[i] = static_cast<double>(baked_.parameters[i]);
            }
            _rebuild_from_tactile(baked_.num_params > 0 ? raw : nullptr);
        }
    }
}

// ============================================================================
// Custom `Spline2D` Escher Edge Curves (`J`, `U`, `S`, `I`)
// ============================================================================

template <FloatingPointScalar T>
TactileEdgeShape IsohedralTiling2D<T>::get_edge_shape_constraint(uint8_t p_edge_shape_id) const noexcept {
    for (uint8_t i = 0; i < baked_.num_vertices; ++i) {
        if (baked_.edges[i].edge_shape_id == p_edge_shape_id) {
            return baked_.edges[i].shape;
        }
    }
    return TactileEdgeShape::EDGE_I;
}

template <FloatingPointScalar T>
void IsohedralTiling2D<T>::set_edge_spline(uint8_t p_edge_shape_id, const Spline2& p_canonical_spline) {
    if (p_edge_shape_id >= BakedPOD::MAX_VERTICES) return;
    edge_splines_[p_edge_shape_id]    = p_canonical_spline;
    has_edge_spline_[p_edge_shape_id] = (p_canonical_spline.get_point_count() >= 2);
    outline_dirty_                    = true;
}

template <FloatingPointScalar T>
bool IsohedralTiling2D<T>::has_edge_spline(uint8_t p_edge_shape_id) const noexcept {
    return (p_edge_shape_id < BakedPOD::MAX_VERTICES) && has_edge_spline_[p_edge_shape_id];
}

template <FloatingPointScalar T>
const typename IsohedralTiling2D<T>::Spline2* IsohedralTiling2D<T>::get_edge_spline(uint8_t p_edge_shape_id) const noexcept {
    if (p_edge_shape_id >= BakedPOD::MAX_VERTICES || !has_edge_spline_[p_edge_shape_id]) {
        return nullptr;
    }
    return &edge_splines_[p_edge_shape_id];
}

template <FloatingPointScalar T>
void IsohedralTiling2D<T>::clear_edge_spline(uint8_t p_edge_shape_id) noexcept {
    if (p_edge_shape_id >= BakedPOD::MAX_VERTICES) return;
    edge_splines_[p_edge_shape_id].clear();
    has_edge_spline_[p_edge_shape_id] = false;
    outline_dirty_                    = true;
}

template <FloatingPointScalar T>
void IsohedralTiling2D<T>::clear_all_edge_splines() noexcept {
    for (size_t i = 0; i < BakedPOD::MAX_VERTICES; ++i) {
        edge_splines_[i].clear();
        has_edge_spline_[i] = false;
    }
    outline_dirty_ = true;
}

template <FloatingPointScalar T>
bool IsohedralTiling2D<T>::has_any_custom_edge_spline() const noexcept {
    for (bool b : has_edge_spline_) {
        if (b) return true;
    }
    return false;
}

template <FloatingPointScalar T>
void IsohedralTiling2D<T>::_ensure_deformed_outline(int p_subdivisions_per_edge) const {
    const int subs = std::max(2, p_subdivisions_per_edge);
    if (!outline_dirty_ && cached_outline_subdivisions_ == subs) {
        return;
    }

    cached_deformed_outline_.clear();
    cached_deformed_triangles_.clear();
    cached_outline_subdivisions_ = subs;

    if (shape_preset_ != FundamentalShapePreset::TACTILE_DEFAULT && !has_any_custom_edge_spline()) {
        cached_deformed_outline_ = get_legacy_primitive_vertices(shape_preset_, static_cast<T>(1.0));
        PolygonMath2D<T>::triangulate_into(cached_deformed_outline_, cached_deformed_triangles_);
        outline_dirty_ = false;
        return;
    }

    if (!has_any_custom_edge_spline()) {
        cached_deformed_outline_.assign(
            baked_.base_vertices.begin(),
            baked_.base_vertices.begin() + baked_.num_vertices
        );
        PolygonMath2D<T>::triangulate_into(cached_deformed_outline_, cached_deformed_triangles_);
        outline_dirty_ = false;
        return;
    }

    for (uint8_t e = 0; e < baked_.num_vertices; ++e) {
        const PrototileEdgeDescriptorPOD<T>& ed = baked_.edges[e];
        const uint8_t sid = ed.edge_shape_id;

        if (ed.shape != TactileEdgeShape::EDGE_I &&
            sid < BakedPOD::MAX_VERTICES &&
            has_edge_spline_[sid]) {
            const std::vector<Vec2> canonical_pts = edge_splines_[sid].get_smoothed_points(subs);
            const std::vector<Vec2> edge_pts = SymmetryMath2D<T>::evaluate_symmetric_tile_edge(
                canonical_pts,
                ed.start_vertex,
                ed.end_vertex,
                ed.shape,
                ed.reversed
            );
            // Append all except the final endpoint (which is the start of edge e+1)
            for (size_t k = 0; k + 1 < edge_pts.size(); ++k) {
                cached_deformed_outline_.push_back(edge_pts[k]);
            }
        } else {
            cached_deformed_outline_.push_back(ed.start_vertex);
        }
    }

    PolygonMath2D<T>::triangulate_into(cached_deformed_outline_, cached_deformed_triangles_);
    outline_dirty_ = false;
}

template <FloatingPointScalar T>
std::span<const typename IsohedralTiling2D<T>::Vec2> IsohedralTiling2D<T>::get_deformed_prototile_outline(
    int p_subdivisions_per_edge
) const {
    _ensure_deformed_outline(p_subdivisions_per_edge);
    return cached_deformed_outline_;
}

template <FloatingPointScalar T>
std::span<const uint32_t> IsohedralTiling2D<T>::get_deformed_prototile_triangles(
    int p_subdivisions_per_edge
) const {
    _ensure_deformed_outline(p_subdivisions_per_edge);
    return cached_deformed_triangles_;
}

// ============================================================================
// Cell Matrices, Polygons, 3-Coloring & Quotient Step Queries
// ============================================================================

template <FloatingPointScalar T>
typename IsohedralTiling2D<T>::Mat3 IsohedralTiling2D<T>::get_cell_world_matrix(
    int32_t p_t1,
    int32_t p_t2,
    uint8_t p_aspect
) const noexcept {
    const uint8_t asp = (p_aspect < baked_.num_aspects) ? p_aspect : 0u;
    Mat3 m = baked_.aspect_xforms[asp];
    const Vec2 world_shift = baked_.lattice_basis.origin +
                             baked_.lattice_basis.t1 * static_cast<T>(p_t1) +
                             baked_.lattice_basis.t2 * static_cast<T>(p_t2);
    m[2][0] += world_shift.x;
    m[2][1] += world_shift.y;
    return m;
}

template <FloatingPointScalar T>
TileTransformPOD IsohedralTiling2D<T>::get_cell_transform_pod(
    int32_t p_t1,
    int32_t p_t2,
    uint8_t p_aspect,
    uint32_t p_cell_id,
    uint16_t p_generation
) const noexcept {
    const uint8_t asp = (p_aspect < baked_.num_aspects) ? p_aspect : 0u;
    const Vec2 world_shift = baked_.lattice_basis.origin +
                             baked_.lattice_basis.t1 * static_cast<T>(p_t1) +
                             baked_.lattice_basis.t2 * static_cast<T>(p_t2);
    const Vec2 pos(
        baked_.aspect_xforms[asp][2][0] + world_shift.x,
        baked_.aspect_xforms[asp][2][1] + world_shift.y
    );
    const uint8_t col = get_colour(p_t1, p_t2, asp);

    return TileTransformPOD{
        .position    = glm::vec2(static_cast<float>(pos.x), static_cast<float>(pos.y)),
        .scale       = glm::vec2(
            static_cast<float>(baked_.aspect_scales[asp].x),
            static_cast<float>(baked_.aspect_scales[asp].y)
        ),
        .angle_rad   = static_cast<float>(baked_.aspect_angles_rad[asp]),
        .cell_id     = p_cell_id,
        .custom_data = glm::vec2(static_cast<float>(col) * 0.5f, static_cast<float>(p_generation))
    };
}

template <FloatingPointScalar T>
std::vector<typename IsohedralTiling2D<T>::Vec2> IsohedralTiling2D<T>::get_cell_world_polygon(
    int32_t p_t1,
    int32_t p_t2,
    uint8_t p_aspect,
    bool p_use_deformed_edges,
    int p_subdivisions_per_edge,
    bool p_enforce_ccw
) const {
    const Mat3 m = get_cell_world_matrix(p_t1, p_t2, p_aspect);
    const std::span<const Vec2> local_pts = p_use_deformed_edges
        ? get_deformed_prototile_outline(p_subdivisions_per_edge)
        : get_base_prototile_vertices();

    std::vector<Vec2> world_pts;
    world_pts.reserve(local_pts.size());
    for (const auto& pt : local_pts) {
        world_pts.push_back(SymmetryMath2D<T>::transform_point(m, pt));
    }
    if (p_enforce_ccw && world_pts.size() >= 3) {
        PolygonMath2D<T>::ensure_winding(world_pts, WindingOrder::COUNTER_CLOCKWISE);
    }
    return world_pts;
}

template <FloatingPointScalar T>
uint8_t IsohedralTiling2D<T>::get_colour(int32_t p_t1, int32_t p_t2, uint8_t p_aspect) const noexcept {
    const uint8_t asp = (p_aspect < baked_.num_aspects) ? p_aspect : 0u;
    const int32_t nc  = static_cast<int32_t>(baked_.colouring_table[18]);
    if (nc <= 0) return 0u;

    int32_t mt1 = p_t1 % nc;
    if (mt1 < 0) mt1 += nc;
    int32_t mt2 = p_t2 % nc;
    if (mt2 < 0) mt2 += nc;

    uint8_t col = baked_.colouring_table[asp];
    for (int32_t idx = 0; idx < mt1; ++idx) {
        col = baked_.colouring_table[12 + (col % 3u)];
    }
    for (int32_t idx = 0; idx < mt2; ++idx) {
        col = baked_.colouring_table[15 + (col % 3u)];
    }
    return col;
}

template <FloatingPointScalar T>
bool IsohedralTiling2D<T>::step_to_neighbor(
    int32_t p_t1,
    int32_t p_t2,
    uint8_t p_aspect,
    uint8_t p_edge_slot,
    int32_t& r_next_t1,
    int32_t& r_next_t2,
    uint8_t& r_next_aspect,
    uint8_t* r_shared_edge_slot
) const noexcept {
    if (p_aspect >= baked_.num_aspects || p_edge_slot >= baked_.num_vertices) {
        return false;
    }
    const QuotientNeighborStepPOD& step = baked_.quotient_adjacency[p_aspect][p_edge_slot];
    if (!step.is_valid) {
        return false;
    }
    r_next_t1     = p_t1 + static_cast<int32_t>(step.delta_i);
    r_next_t2     = p_t2 + static_cast<int32_t>(step.delta_j);
    r_next_aspect = step.next_aspect;
    if (r_shared_edge_slot) {
        *r_shared_edge_slot = step.shared_edge_slot;
    }
    return true;
}

template <FloatingPointScalar T>
bool IsohedralTiling2D<T>::locate_cell_at_world_point(
    const Vec2& p_world_point,
    int32_t& r_t1,
    int32_t& r_t2,
    uint8_t& r_aspect
) const noexcept {
    Vec2i base_ij;
    Vec2 frac_uv;
    SymmetryMath2D<T>::world_to_cell_coord(baked_.lattice_basis, p_world_point, base_ij, frac_uv);

    const std::span<const Vec2> proto_verts = get_base_prototile_vertices();
    const Vec2 rel_world = p_world_point - baked_.lattice_basis.origin;

    // Test candidate aspects in the 3x3 neighborhood around (base_ij.x, base_ij.y)
    for (int di = -1; di <= 1; ++di) {
        for (int dj = -1; dj <= 1; ++dj) {
            const int32_t ci = base_ij.x + di;
            const int32_t cj = base_ij.y + dj;
            const Vec2 cell_shift = baked_.lattice_basis.t1 * static_cast<T>(ci) +
                                    baked_.lattice_basis.t2 * static_cast<T>(cj);
            const Vec2 unit_pt = rel_world - cell_shift;

            for (uint8_t a = 0; a < baked_.num_aspects; ++a) {
                const Vec2 local_pt = SymmetryMath2D<T>::transform_point(baked_.inv_aspect_xforms[a], unit_pt);
                if (PolygonMath2D<T>::contains_point(proto_verts, local_pt)) {
                    r_t1     = ci;
                    r_t2     = cj;
                    r_aspect = a;
                    return true;
                }
            }
        }
    }

    // Closest aspect centroid fallback for points lying exactly on an edge
    T best_d2 = std::numeric_limits<T>::max();
    for (int di = -1; di <= 1; ++di) {
        for (int dj = -1; dj <= 1; ++dj) {
            const int32_t ci = base_ij.x + di;
            const int32_t cj = base_ij.y + dj;
            const Vec2 cell_shift = baked_.lattice_basis.origin +
                                    baked_.lattice_basis.t1 * static_cast<T>(ci) +
                                    baked_.lattice_basis.t2 * static_cast<T>(cj);
            for (uint8_t a = 0; a < baked_.num_aspects; ++a) {
                const Vec2 c = baked_.aspect_local_centers[a] + cell_shift;
                const T d2   = glm::dot(p_world_point - c, p_world_point - c);
                if (d2 < best_d2) {
                    best_d2  = d2;
                    r_t1     = ci;
                    r_t2     = cj;
                    r_aspect = a;
                }
            }
        }
    }
    return best_d2 < std::numeric_limits<T>::max();
}

// ============================================================================
// Analytical Region Enumeration & Multi-Modal `Spline2D` Fill
// ============================================================================

template <FloatingPointScalar T>
TilingRegionFillResult<T> IsohedralTiling2D<T>::fill_region_aabb(
    const AABB2D<T>& p_bounds,
    const Vec2& p_seed_point
) const {
    TilingRegionFillResult<T> out;
    if (!p_bounds.is_valid()) return out;

    // Project the 4 corners of p_bounds into lattice (u, v) coordinates
    const Vec2 corners[4] = {
        Vec2(p_bounds.min_bounds.x, p_bounds.min_bounds.y),
        Vec2(p_bounds.max_bounds.x, p_bounds.min_bounds.y),
        Vec2(p_bounds.max_bounds.x, p_bounds.max_bounds.y),
        Vec2(p_bounds.min_bounds.x, p_bounds.max_bounds.y)
    };

    AABB2D<T> uv_bounds;
    for (const auto& c : corners) {
        uv_bounds.expand_to(SymmetryMath2D<T>::world_to_lattice(baked_.lattice_basis, c));
    }

    // Pad by 1 unit cell in each direction so rotated/offset aspects on the perimeter are never missed
    const int32_t min_i = static_cast<int32_t>(std::floor(uv_bounds.min_bounds.x)) - 1;
    const int32_t max_i = static_cast<int32_t>(std::ceil(uv_bounds.max_bounds.x))  + 1;
    const int32_t min_j = static_cast<int32_t>(std::floor(uv_bounds.min_bounds.y)) - 1;
    const int32_t max_j = static_cast<int32_t>(std::ceil(uv_bounds.max_bounds.y))  + 1;

    Vec2i seed_ij{0, 0};
    Vec2 seed_frac{};
    SymmetryMath2D<T>::world_to_cell_coord(baked_.lattice_basis, p_seed_point, seed_ij, seed_frac);

    const size_t est_cells = static_cast<size_t>(std::max(1, (max_i - min_i + 1) * (max_j - min_j + 1))) * baked_.num_aspects;
    out.cells.reserve(est_cells);
    out.transforms.reserve(est_cells);
    out.classifications.reserve(est_cells);
    out.interior_cell_indices.reserve(est_cells);

    for (int32_t j = min_j; j <= max_j; ++j) {
        for (int32_t i = min_i; i <= max_i; ++i) {
            const Vec2 cell_shift = baked_.lattice_basis.origin +
                                    baked_.lattice_basis.t1 * static_cast<T>(i) +
                                    baked_.lattice_basis.t2 * static_cast<T>(j);
            const uint16_t gen = static_cast<uint16_t>(
                std::min<int32_t>(65535, std::abs(i - seed_ij.x) + std::abs(j - seed_ij.y))
            );

            for (uint8_t a = 0; a < baked_.num_aspects; ++a) {
                const Vec2 world_center = baked_.aspect_local_centers[a] + cell_shift;
                if (!p_bounds.contains_point(world_center)) continue;

                const uint32_t cell_id = static_cast<uint32_t>(out.cells.size());
                const uint8_t col      = get_colour(i, j, a);
                uint8_t flags          = CELL_FLAG_NONE;
                if (baked_.aspect_orientations[a] < 0) {
                    flags |= CELL_FLAG_REFLECTED;
                }

                out.cells.push_back(IsoCellPOD{
                    .lattice_i        = static_cast<int16_t>(i),
                    .lattice_j        = static_cast<int16_t>(j),
                    .group_slot       = 0u,
                    .aspect_index     = a,
                    .vertex_count     = baked_.num_vertices,
                    .flags            = flags,
                    .generation       = gen,
                    .colour_id        = col,
                    .first_vertex_idx = 0xFFFFFFFFu,
                    .first_edge_idx   = 0xFFFFFFFFu,
                    .super_hex_id     = 0xFFFFFFFFu,
                    .center           = glm::vec2(static_cast<float>(world_center.x), static_cast<float>(world_center.y))
                });
                out.transforms.push_back(get_cell_transform_pod(i, j, a, cell_id, gen));
                out.classifications.push_back(ClipClassification2D::FULLY_INSIDE);
                out.interior_cell_indices.push_back(cell_id);
            }
        }
    }

    return out;
}

template <FloatingPointScalar T>
TilingRegionFillResult<T> IsohedralTiling2D<T>::fill_spline_region(
    std::span<const Spline2> p_region_splines,
    const Vec2& p_seed_point,
    TilingCoverageMode p_coverage_mode,
    int p_spline_subdivisions
) const {
    TilingRegionFillResult<T> out;
    if (p_region_splines.empty()) return out;

    // Flatten all region splines and compute the combined world AABB
    std::vector<std::vector<Vec2>> boundary_polys;
    std::vector<AABB2D<T>> boundary_aabbs;
    AABB2D<T> combined_aabb;

    for (const auto& sp : p_region_splines) {
        std::vector<Vec2> pts = sp.get_smoothed_points(p_spline_subdivisions);
        if (pts.size() > 3 && glm::distance(pts.front(), pts.back()) < GeometryConstants<T>::WELD_EPSILON) {
            pts.pop_back();
        }
        if (pts.size() >= 3) {
            const AABB2D<T> box = PolygonMath2D<T>::compute_aabb(pts);
            combined_aabb.merge_with(box);
            boundary_polys.push_back(std::move(pts));
            boundary_aabbs.push_back(box);
        }
    }

    if (boundary_polys.empty() || !combined_aabb.is_valid()) {
        return out;
    }

    // Project combined AABB into lattice space
    const Vec2 corners[4] = {
        Vec2(combined_aabb.min_bounds.x, combined_aabb.min_bounds.y),
        Vec2(combined_aabb.max_bounds.x, combined_aabb.min_bounds.y),
        Vec2(combined_aabb.max_bounds.x, combined_aabb.max_bounds.y),
        Vec2(combined_aabb.min_bounds.x, combined_aabb.max_bounds.y)
    };
    AABB2D<T> uv_bounds;
    for (const auto& c : corners) {
        uv_bounds.expand_to(SymmetryMath2D<T>::world_to_lattice(baked_.lattice_basis, c));
    }

    const int32_t min_i = static_cast<int32_t>(std::floor(uv_bounds.min_bounds.x)) - 2;
    const int32_t max_i = static_cast<int32_t>(std::ceil(uv_bounds.max_bounds.x))  + 2;
    const int32_t min_j = static_cast<int32_t>(std::floor(uv_bounds.min_bounds.y)) - 2;
    const int32_t max_j = static_cast<int32_t>(std::ceil(uv_bounds.max_bounds.y))  + 2;

    Vec2i seed_ij{0, 0};
    Vec2 seed_frac{};
    SymmetryMath2D<T>::world_to_cell_coord(baked_.lattice_basis, p_seed_point, seed_ij, seed_frac);

    const bool use_deformed = has_any_custom_edge_spline();

    for (int32_t j = min_j; j <= max_j; ++j) {
        for (int32_t i = min_i; i <= max_i; ++i) {
            const uint16_t gen = static_cast<uint16_t>(
                std::min<int32_t>(65535, std::abs(i - seed_ij.x) + std::abs(j - seed_ij.y))
            );

            for (uint8_t a = 0; a < baked_.num_aspects; ++a) {
                const std::vector<Vec2> cell_poly = get_cell_world_polygon(
                    i, j, a, use_deformed, p_spline_subdivisions, true
                );
                if (cell_poly.size() < 3) continue;

                const Vec2 world_center = PolygonMath2D<T>::centroid(cell_poly);

                // Classify against the union of boundary polygons
                ClipClassification2D best_class = ClipClassification2D::FULLY_OUTSIDE;
                size_t hit_boundary_idx = 0;

                for (size_t b = 0; b < boundary_polys.size(); ++b) {
                    const ClipClassification2D c = CSGMath2D<T>::classify_polygon_against_boundary(
                        cell_poly, boundary_polys[b], boundary_aabbs[b]
                    );
                    if (c == ClipClassification2D::FULLY_INSIDE) {
                        best_class = ClipClassification2D::FULLY_INSIDE;
                        hit_boundary_idx = b;
                        break;
                    }
                    if (c == ClipClassification2D::STRADDLING_BOUNDARY) {
                        best_class = ClipClassification2D::STRADDLING_BOUNDARY;
                        hit_boundary_idx = b;
                    }
                }

                if (best_class == ClipClassification2D::FULLY_OUTSIDE) {
                    continue;
                }

                // Filter according to TilingCoverageMode
                if (p_coverage_mode == TilingCoverageMode::STRICT_INTERIOR &&
                    best_class != ClipClassification2D::FULLY_INSIDE) {
                    continue;
                }
                if (p_coverage_mode == TilingCoverageMode::CENTROID_INSIDE &&
                    best_class != ClipClassification2D::FULLY_INSIDE) {
                    bool center_in = false;
                    for (const auto& bp : boundary_polys) {
                        if (PolygonMath2D<T>::contains_point(bp, world_center)) {
                            center_in = true;
                            break;
                        }
                    }
                    if (!center_in) continue;
                }

                const uint32_t cell_id = static_cast<uint32_t>(out.cells.size());
                const uint8_t col      = get_colour(i, j, a);
                uint8_t flags          = CELL_FLAG_NONE;
                if (baked_.aspect_orientations[a] < 0) {
                    flags |= CELL_FLAG_REFLECTED;
                }
                if (best_class == ClipClassification2D::STRADDLING_BOUNDARY) {
                    flags |= CELL_FLAG_ON_BORDER;
                }

                if (p_coverage_mode == TilingCoverageMode::CLIP_EXACT_TO_SPLINE &&
                    best_class == ClipClassification2D::STRADDLING_BOUNDARY) {
                    std::vector<Vec2> clipped = CSGMath2D<T>::clip_polygon_convex(
                        cell_poly, boundary_polys[hit_boundary_idx]
                    );
                    if (clipped.size() < 3) {
                        // Non-convex fallback: keep cell_poly if centroid is inside
                        if (PolygonMath2D<T>::contains_point(boundary_polys[hit_boundary_idx], world_center)) {
                            clipped = cell_poly;
                        } else {
                            continue;
                        }
                    }
                    flags |= CELL_FLAG_CLIPPED_CAP;
                    out.clipped_boundary_polygons.push_back(std::move(clipped));
                }

                out.cells.push_back(IsoCellPOD{
                    .lattice_i        = static_cast<int16_t>(i),
                    .lattice_j        = static_cast<int16_t>(j),
                    .group_slot       = 0u,
                    .aspect_index     = a,
                    .vertex_count     = baked_.num_vertices,
                    .flags            = flags,
                    .generation       = gen,
                    .colour_id        = col,
                    .first_vertex_idx = 0xFFFFFFFFu,
                    .first_edge_idx   = 0xFFFFFFFFu,
                    .super_hex_id     = 0xFFFFFFFFu,
                    .center           = glm::vec2(static_cast<float>(world_center.x), static_cast<float>(world_center.y))
                });
                out.transforms.push_back(get_cell_transform_pod(i, j, a, cell_id, gen));
                out.classifications.push_back(best_class);

                if (best_class == ClipClassification2D::FULLY_INSIDE) {
                    out.interior_cell_indices.push_back(cell_id);
                } else {
                    out.boundary_seam_cell_indices.push_back(cell_id);
                }
            }
        }
    }

    return out;
}

// Explicit Instantiations
template class IsohedralTiling2D<float>;
template class IsohedralTiling2D<double>;

} // namespace ideam::core