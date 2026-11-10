#pragma once

/**
 * data_oriented_splines.h
 * Godot-facing Resource bridge class in `src/godot/tasks/domains/splines/`.
 *
 * Extends `ideam::godot_ext::GeometryResource` to unite 1D-4D `SplineResource` objects
 * (`Spline1DResource`, `Spline2DResource`, `Spline3DResource`, `Spline4DResource`) with
 * `ideam::core::SplineDOD` and `ideam::core::GeometryDOD` (`DataOrientedGeometry`).
 *
 * Key Responsibilities:
 *   1. Bakes any heterogeneous collection of 1D, 2D, 3D, and 4D `SplineResource`s into
 *      uniform 64-byte `SplineSegmentCoeffsPOD<4, float>` Horner segments and 64-bin
 *      weighted & arc-length `SplineParametricLUTEntryPOD<float>` tables.
 *   2. Automatically bridges to `DataOrientedGeometry` (`bind_data_oriented_geometry`) via
 *      `SplineDOD::bind_external_geometry_buffers()`, sharing the same `MemoryManagerDOD`
 *      and `TaskGraphDOD` so Wave 1 `SplineFlattenToPolyline` and `SplineBishopFrameSweep`
 *      populate `GeometryDOD`'s polyline and 3D Bishop frame buffers with zero copying.
 */

#include "../../../../core/tasks/domains/splines/spline_dod.h"
#include "../../../geometry/geometry_resource.h"
#include "../../../splines/spline_1d/spline_1d_resource.h"
#include "../../../splines/spline_2d/spline_2d_resource.h"
#include "../../../splines/spline_3d/spline_3d_resource.h"
#include "../../../splines/spline_4d/spline_4d_resource.h"
#include "../../../splines/spline_resource.h"
#include "../geometry/data_oriented_geometry.h"
#include "../../task_graph_resource.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/packed_vector4_array.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include <cstdint>
#include <memory>
#include <unordered_map>

namespace ideam::godot_ext {

enum SplinesDODExecutionMode : int32_t {
    SPLINES_DOD_EXEC_DISABLED              = 0,
    SPLINES_DOD_EXEC_COMPOSABLE_STAGES     = 1,
    SPLINES_DOD_EXEC_CUSTOM_GRAPH_RESOURCE = 2,
    SPLINES_DOD_EXEC_MANUAL_GRAPH          = 3
};

class DataOrientedSplines : public GeometryResource {
    GDCLASS(DataOrientedSplines, GeometryResource)

protected:
    static void _bind_methods();

private:
    std::unique_ptr<core::SplineDOD> spline_dod_;
    std::shared_ptr<core::TaskGraphDOD> active_task_graph_;
    godot::HashMap<godot::StringName, core::NodeID> ui_to_dod_map_;

    // --- Managed Spline Resources & Optional Linked GeometryDOD Resource ---
    godot::TypedArray<SplineResource> splines_;
    godot::Ref<DataOrientedGeometry> linked_geometry_resource_;

    // --- Inspector Configuration ---
    SplinesDODExecutionMode dod_execution_mode_ = SPLINES_DOD_EXEC_COMPOSABLE_STAGES;
    uint32_t pipeline_stage_mask_               = core::SPLINES_STAGE_DEFAULT_PIPELINE_MASK;
    bool auto_connect_pipeline_stages_          = true;
    int default_subdivisions_per_section_       = 12;
    int lut_bins_per_spline_                    = 64;

    godot::Ref<TaskGraphResource> custom_task_graph_resource_;

    // Configurable Buffer Capacities
    int max_dod_splines_         = 512;
    int max_dod_segments_        = 8192;
    int max_dod_sample_requests_ = 16384;
    int max_dod_query_points_    = 8192;

    // --- Bidirectional ObjectID <-> DOD Spline Slot Registries ---
    std::unordered_map<uint64_t, uint32_t> object_id_to_spline_slot_;
    std::unordered_map<uint32_t, godot::Ref<SplineResource>> spline_slot_to_resource_;

    core::SplinesBufferCapacities _build_capacities() const noexcept;
    void _ensure_spline_dod_initialized(std::shared_ptr<core::MemoryManagerDOD> p_shared_manager = nullptr);
    void _clear_resource_maps() noexcept;

public:
    DataOrientedSplines() = default;
    ~DataOrientedSplines() override = default;

    // --- Direct C++ Accessors ---
    [[nodiscard]] core::SplineDOD *get_spline_dod() const noexcept { return spline_dod_.get(); }
    [[nodiscard]] std::shared_ptr<core::TaskGraphDOD> get_active_task_graph_shared() const noexcept { return active_task_graph_; }
    [[nodiscard]] core::TaskGraphDOD *get_active_task_graph_dod() const noexcept { return active_task_graph_.get(); }
    [[nodiscard]] std::shared_ptr<core::MemoryManagerDOD> get_memory_manager_shared() const noexcept {
        return spline_dod_ ? spline_dod_->get_memory_manager() : nullptr;
    }
    [[nodiscard]] core::MemoryManagerDOD *get_memory_manager_dod() const noexcept {
        return spline_dod_ ? spline_dod_->get_memory_manager_raw() : nullptr;
    }

    // --- Resource Collection & GeometryDOD Bridging ---
    void set_splines(const godot::TypedArray<SplineResource> &p_splines);
    [[nodiscard]] godot::TypedArray<SplineResource> get_splines() const { return splines_; }

    void set_linked_geometry_resource(const godot::Ref<DataOrientedGeometry> &p_geometry);
    [[nodiscard]] godot::Ref<DataOrientedGeometry> get_linked_geometry_resource() const { return linked_geometry_resource_; }
    bool bind_data_oriented_geometry(const godot::Ref<DataOrientedGeometry> &p_geometry);

    // --- Inspector Configuration Setters / Getters ---
    void set_dod_execution_mode(SplinesDODExecutionMode p_mode);
    [[nodiscard]] SplinesDODExecutionMode get_dod_execution_mode() const noexcept { return dod_execution_mode_; }

    void set_pipeline_stage_mask(int p_mask);
    [[nodiscard]] int get_pipeline_stage_mask() const noexcept { return static_cast<int>(pipeline_stage_mask_); }

    void set_auto_connect_pipeline_stages(bool p_auto_connect);
    [[nodiscard]] bool get_auto_connect_pipeline_stages() const noexcept { return auto_connect_pipeline_stages_; }

    void set_default_subdivisions_per_section(int p_subdivs);
    [[nodiscard]] int get_default_subdivisions_per_section() const noexcept { return default_subdivisions_per_section_; }

    void set_lut_bins_per_spline(int p_bins);
    [[nodiscard]] int get_lut_bins_per_spline() const noexcept { return lut_bins_per_spline_; }

    void set_custom_task_graph_resource(const godot::Ref<TaskGraphResource> &p_resource);
    [[nodiscard]] godot::Ref<TaskGraphResource> get_custom_task_graph_resource() const { return custom_task_graph_resource_; }

    void set_max_dod_splines(int p_max);
    [[nodiscard]] int get_max_dod_splines() const noexcept { return max_dod_splines_; }

    void set_max_dod_segments(int p_max);
    [[nodiscard]] int get_max_dod_segments() const noexcept { return max_dod_segments_; }

    void set_max_dod_sample_requests(int p_max);
    [[nodiscard]] int get_max_dod_sample_requests() const noexcept { return max_dod_sample_requests_; }

    void set_max_dod_query_points(int p_max);
    [[nodiscard]] int get_max_dod_query_points() const noexcept { return max_dod_query_points_; }

    // --- Pipeline Setup, Baking & Execution API ---
    void setup_dod_pipeline_with_manager(
        std::shared_ptr<core::MemoryManagerDOD> p_shared_manager,
        std::shared_ptr<core::TaskGraphDOD> p_shared_graph = nullptr
    );
    void setup_dod_pipeline();
    void bake_to_dod();
    void rebuild_dod_task_graph();
    void execute_dod_step(double p_delta);

    // --- Batch Sample & Proximity Query Staging / Readback ---
    void stage_batch_samples(
        const godot::PackedInt32Array &p_spline_slots,
        const godot::PackedFloat32Array &p_percents,
        bool p_weighted = true,
        bool p_compute_tangents = true
    );
    [[nodiscard]] godot::PackedVector4Array get_evaluated_outputs() const;
    [[nodiscard]] godot::PackedVector4Array get_evaluated_tangents() const;

    void stage_proximity_queries(const godot::PackedVector4Array &p_query_points);
    [[nodiscard]] godot::PackedFloat32Array get_closest_percents() const;
    [[nodiscard]] godot::PackedFloat32Array get_closest_distances() const;
    [[nodiscard]] godot::PackedInt32Array get_closest_anchors() const;

    // --- Flattened GeometryDOD Polyline & 3D Bishop Frame Readback ---
    [[nodiscard]] int get_spline_dod_slot(const godot::Ref<SplineResource> &p_spline) const;
    [[nodiscard]] int get_spline_geometry_polyline_slot(int p_spline_slot) const;
    [[nodiscard]] godot::PackedVector2Array get_flattened_polyline_2d(int p_spline_slot) const;
    [[nodiscard]] godot::PackedVector3Array get_flattened_polyline_3d(int p_spline_slot) const;
    [[nodiscard]] godot::TypedArray<godot::Transform3D> get_swept_bishop_transforms_3d(int p_spline_slot) const;

    [[nodiscard]] int get_buffer_id(const godot::StringName &p_semantic_name) const;
    [[nodiscard]] godot::Dictionary get_buffer_manifest_dictionary() const;

    // --- Dynamic Task Node & Wave Co-Scheduling API ---
    int add_splines_transform_node(
        int p_logic_id,
        const godot::Dictionary &p_properties = godot::Dictionary(),
        int p_priority = 0,
        bool p_use_registry_factory = false
    );
    int add_splines_query_node(
        int p_logic_id,
        int p_query_op = 0,
        const godot::Dictionary &p_properties = godot::Dictionary(),
        int p_priority = 0,
        bool p_use_registry_factory = false
    );
    int add_splines_metadata_node(
        int p_logic_id,
        const godot::Dictionary &p_properties = godot::Dictionary(),
        int p_priority = 0,
        bool p_use_registry_factory = false
    );

    bool schedule_node_with_stage(int p_stage_flag, int p_custom_node_id, bool p_join_downstream = false);
    bool schedule_node_between_stages(int p_upstream_stage_flag, int p_downstream_stage_flag, int p_custom_node_id);
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::SplinesDODExecutionMode);
