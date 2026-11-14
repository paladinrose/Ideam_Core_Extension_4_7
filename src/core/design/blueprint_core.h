#pragma once

/**
 * blueprint_core.h
 * Pure C++26 Multi-Layer CAD Architectural Floorplan Kernel (`src/core/design/blueprint_core.h`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES (SECTION 2.1 — `Blueprint.cs` & `ArchitecturalPlot.cs` Port)
 * ============================================================================
 * Represents architectural floorplans across 4 linked reactive CAD layers with automatic
 * line-weight rules and a color-blind-safe (Okabe-Ito) palette table:
 *   1. `LAYER_SPACES` (`0`): Room/courtyard polygons (`Spline2D` / `Polygon2D`), thin line
 *      weight (`1.25px`), per-room semantic `Sememe` / `SemanticRoleID` tag.
 *   2. `LAYER_WALLS` (`1`): Automatically classifies wall segments at a 4-foot (`1.2192m`)
 *      horizontal section cut into **Exterior Heavy Weight** (`3.5px`, `0.30m` thick) vs.
 *      **Interior Partition Medium-Heavy Weight** (`2.2px`, `0.15m` thick) based on whether
 *      a wall segment is shared by two room polygons or borders the exterior.
 *   3. `LAYER_FEATURES_AND_APERTURES` (`2`): `Door`, `Window`, `Archway`, `Stair`, and
 *      `Fixture` symbols synced to a central Map Key (`BlueprintMapKeyEntryPOD`) and
 *      automatically generating `ApertureCutoutPod` boolean cutouts on host walls.
 *   4. `LAYER_LABELS_AND_MEASUREMENTS` (`3`): Live dimensional annotations (`length`,
 *      `area_sq_meters`) and `Location` / `GameBoard` tags.
 */

#include "structure_layout_solver.h"
#include <cstdint>
#include <glm/glm.hpp>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace ideam::core {

enum class BlueprintLayerType : uint8_t {
    LAYER_SPACES                    = 0,
    LAYER_WALLS                     = 1,
    LAYER_FEATURES_AND_APERTURES    = 2,
    LAYER_LABELS_AND_MEASUREMENTS   = 3,
    COUNT                           = 4
};

enum class BlueprintFeatureSymbolType : uint8_t {
    SYMBOL_DOOR_SINGLE  = 0,
    SYMBOL_DOOR_DOUBLE  = 1,
    SYMBOL_WINDOW_SASH  = 2,
    SYMBOL_ARCHWAY      = 3,
    SYMBOL_STAIR_RUN    = 4,
    SYMBOL_FIXTURE_PROP = 5
};

/**
 * Color-blind-safe (Okabe-Ito) RGBA color and automatic CAD stroke weight per layer.
 */
struct BlueprintLayerVisualStylePOD {
    glm::vec4 color_rgba         = glm::vec4(0.0f, 0.45f, 0.70f, 1.0f);
    float     stroke_weight_px   = 1.5f;
    float     section_cut_height = 1.2192f; ///< Standard 4-foot (48-inch) architectural section cut
};

struct BlueprintSpacePolygonPOD {
    uint32_t               space_id          = 0;
    uint32_t               semantic_role_id  = SEMANTIC_ROLE_ASSEMBLY_ROOM;
    uint32_t               nuclear_seme_mask = 0u;
    std::string            room_name;
    std::vector<glm::vec2> vertices_2d;      ///< Closed CCW 2D polygon in meters `(X, Z)`
    float                  floor_elevation   = 0.0f;
    float                  ceiling_height    = 3.0f;
    float                  computed_area_sqm = 0.0f;
};

struct BlueprintWallSegmentPOD {
    uint32_t  wall_id          = 0;
    uint32_t  space_a_id       = 0xFFFFFFFFu;
    uint32_t  space_b_id       = 0xFFFFFFFFu; ///< `0xFFFFFFFFu` if exterior wall, else shared interior wall
    glm::vec2 start_2d         = glm::vec2(0.0f);
    glm::vec2 end_2d           = glm::vec2(0.0f);
    float     thickness_meters = 0.30f;
    float     height_meters    = 3.0f;
    float     stroke_weight_px = 3.5f;        ///< `3.5f` for exterior heavy weight, `2.2f` for interior medium-heavy
    uint8_t   is_exterior      = 1u;
};

struct BlueprintFeatureSymbolPOD {
    uint32_t                   feature_id      = 0;
    uint32_t                   host_wall_id    = 0xFFFFFFFFu;
    uint32_t                   map_key_code    = 0;
    BlueprintFeatureSymbolType symbol_type     = BlueprintFeatureSymbolType::SYMBOL_DOOR_SINGLE;
    glm::vec2                  position_2d     = glm::vec2(0.0f);
    float                      width_meters    = 0.90f;
    float                      height_meters   = 2.10f;
    float                      sill_elevation  = 0.0f;
};

struct BlueprintMapKeyEntryPOD {
    uint32_t                   map_key_code    = 0;
    BlueprintFeatureSymbolType symbol_type     = BlueprintFeatureSymbolType::SYMBOL_DOOR_SINGLE;
    std::string                symbol_label;
    uint32_t                   instance_count  = 0;
};

struct BlueprintMeasurementAnnotationPOD {
    uint32_t    annotation_id = 0;
    uint32_t    target_id     = 0;
    glm::vec2   anchor_2d     = glm::vec2(0.0f);
    float       measured_val  = 0.0f;
    std::string label_text;
};

class ArchitecturalBlueprintCore {
private:
    static constexpr float FOUR_FOOT_SECTION_CUT_METERS = 1.2192f;

    std::vector<BlueprintSpacePolygonPOD> spaces_;
    std::vector<BlueprintWallSegmentPOD> walls_;
    std::vector<BlueprintFeatureSymbolPOD> features_;
    std::vector<BlueprintMapKeyEntryPOD> map_key_;
    std::vector<BlueprintMeasurementAnnotationPOD> annotations_;

    [[nodiscard]] static float compute_polygon_area_2d(std::span<const glm::vec2> p_verts) noexcept;
    [[nodiscard]] static bool are_segments_collinear_shared(
        const glm::vec2 &p_a0,
        const glm::vec2 &p_a1,
        const glm::vec2 &p_b0,
        const glm::vec2 &p_b1,
        float p_tol = 0.05f
    ) noexcept;

public:
    ArchitecturalBlueprintCore() = default;
    ~ArchitecturalBlueprintCore() = default;

    void clear() noexcept;

    [[nodiscard]] static BlueprintLayerVisualStylePOD get_layer_visual_style(
        BlueprintLayerType p_layer,
        bool p_is_exterior_wall = true
    ) noexcept;

    uint32_t add_space_polygon(
        std::string_view p_room_name,
        std::span<const glm::vec2> p_vertices_2d,
        uint32_t p_semantic_role_id = SEMANTIC_ROLE_ASSEMBLY_ROOM,
        uint32_t p_nuclear_seme_mask = 0u,
        float p_floor_elevation = 0.0f,
        float p_ceiling_height = 3.0f
    );

    uint32_t add_feature_symbol(
        BlueprintFeatureSymbolType p_type,
        const glm::vec2 &p_position_2d,
        float p_width_meters = 0.90f,
        float p_height_meters = 2.10f,
        float p_sill_elevation = 0.0f,
        std::string_view p_map_key_label = {}
    );

    /**
     * Rebuilds `LAYER_WALLS`, `LAYER_FEATURES_AND_APERTURES` Map Key counts, and
     * `LAYER_LABELS_AND_MEASUREMENTS` reactively from `spaces_` and `features_`.
     */
    void rebuild_reactive_layers();

    /**
     * Extrudes the 4-layer CAD floorplan into 3D `StructureNodePod` rooms, floors,
     * ceilings, mitered exterior/interior walls, and `ApertureCutoutPod` boolean cutouts
     * inside `r_solver`.
     */
    void extrude_to_structure_layout(StructureLayoutSolver &r_solver) const;

    [[nodiscard]] const std::vector<BlueprintSpacePolygonPOD> &spaces() const noexcept { return spaces_; }
    [[nodiscard]] const std::vector<BlueprintWallSegmentPOD> &walls() const noexcept { return walls_; }
    [[nodiscard]] const std::vector<BlueprintFeatureSymbolPOD> &features() const noexcept { return features_; }
    [[nodiscard]] const std::vector<BlueprintMapKeyEntryPOD> &map_key() const noexcept { return map_key_; }
    [[nodiscard]] const std::vector<BlueprintMeasurementAnnotationPOD> &annotations() const noexcept {
        return annotations_;
    }
};

} // namespace ideam::core
