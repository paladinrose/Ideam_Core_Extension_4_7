#include "geometry_transform_task_registry.h"
#include "../transform_logic/polygon_triangulate_and_offset_transform_logic.h"
#include "../transform_logic/csg_clip_and_seam_weld_transform_logic.h"
#include "../transform_logic/voronoi_delaunay_dual_transform_logic.h"
#include "../transform_logic/sdf_field_and_contour_transform_logic.h"
#include "../../../transform_task.h"

#include <godot_cpp/variant/packed_int64_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace ideam::core {

const std::array<LogicVariantGroup, GeometryTransformTaskRegistry::STORAGE_COUNT>
GeometryTransformTaskRegistry::logic_variants = {{
    { true, 1, nullptr },
    { true, 1, nullptr },
    { true, 1, nullptr },
    { true, 1, nullptr }
}};

void GeometryTransformTaskRegistry::init_execution_routing() {}

void GeometryTransformTaskRegistry::cleanup_execution_routing() {}

namespace {

template <typename T_Logic>
void append_transform_ui_entry(
    godot::Dictionary &r_matrix,
    GeometryTransformLogicID p_id,
    MemoryTypes p_default_type
) {
    godot::StringName key(godot::String::num_int64(static_cast<int64_t>(p_id)));
    godot::Dictionary dict;
    dict["properties"] = T_Logic::get_ui_properties();
    dict["name"]       = godot::String(T_Logic::display_name.data());
    dict["category"]   = godot::String(T_Logic::ui_category.data());

    godot::PackedInt64Array combos;
    const size_t flat_idx =
        static_cast<size_t>(MemoryView::SingleElementView) *
            (GeometryTransformTaskRegistry::S_COUNT * GeometryTransformTaskRegistry::T_COUNT) +
        static_cast<size_t>(MemoryStrategy::FlatStrategy) * GeometryTransformTaskRegistry::T_COUNT +
        static_cast<size_t>(p_default_type);
    combos.push_back(static_cast<int64_t>(flat_idx));
    dict["valid_combinations"] = combos;
    r_matrix[key] = dict;
}

} // namespace

void GeometryTransformTaskRegistry::generate_ui_matrices(godot::Dictionary &p_matrix) {
    append_transform_ui_entry<PolygonTriangulateAndOffsetTransformLogic<>>(
        p_matrix, GeometryTransformLogicID::PolygonTriangulateAndOffset, MemoryTypes::INT32
    );
    append_transform_ui_entry<CSGClipAndSeamWeldTransformLogic<>>(
        p_matrix, GeometryTransformLogicID::CSGClipAndSeamWeld, MemoryTypes::INT32
    );
    append_transform_ui_entry<VoronoiDelaunayDualTransformLogic<>>(
        p_matrix, GeometryTransformLogicID::VoronoiDelaunayDual, MemoryTypes::INT32
    );
    append_transform_ui_entry<SDFFieldAndContourTransformLogic<>>(
        p_matrix, GeometryTransformLogicID::SDFFieldAndContour, MemoryTypes::FLOAT32
    );
}

std::unique_ptr<INativeTask> GeometryTransformTaskRegistry::create(
    uint32_t p_logic_id,
    uint32_t p_view_id,
    uint32_t p_strategy_id,
    uint32_t p_type_id
) {
    (void)p_view_id;
    (void)p_strategy_id;
    (void)p_type_id;
    switch (static_cast<GeometryTransformLogicID>(p_logic_id)) {
        case GeometryTransformLogicID::PolygonTriangulateAndOffset:
            return std::make_unique<TransformTask<PolygonTriangulateAndOffsetTransformLogic<>>>();
        case GeometryTransformLogicID::CSGClipAndSeamWeld:
            return std::make_unique<TransformTask<CSGClipAndSeamWeldTransformLogic<>>>();
        case GeometryTransformLogicID::VoronoiDelaunayDual:
            return std::make_unique<TransformTask<VoronoiDelaunayDualTransformLogic<>>>();
        case GeometryTransformLogicID::SDFFieldAndContour:
            return std::make_unique<TransformTask<SDFFieldAndContourTransformLogic<>>>();
        default:
            return nullptr;
    }
}

} // namespace ideam::core
