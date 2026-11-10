#include "data_oriented_splines.h"

#include <godot_cpp/variant/basis.hpp>
#include <algorithm>
#include <string>

namespace ideam::godot_ext {

void DataOrientedSplines::_bind_methods() {
    using namespace godot;

    BIND_ENUM_CONSTANT(SPLINES_DOD_EXEC_DISABLED);
    BIND_ENUM_CONSTANT(SPLINES_DOD_EXEC_COMPOSABLE_STAGES);
    BIND_ENUM_CONSTANT(SPLINES_DOD_EXEC_CUSTOM_GRAPH_RESOURCE);
    BIND_ENUM_CONSTANT(SPLINES_DOD_EXEC_MANUAL_GRAPH);

    ADD_GROUP("Spline Resources & Geometry Bridge", "");

    ClassDB::bind_method(D_METHOD("set_splines", "splines"), &DataOrientedSplines::set_splines);
    ClassDB::bind_method(D_METHOD("get_splines"), &DataOrientedSplines::get_splines);
    ADD_PROPERTY(
        PropertyInfo(Variant::ARRAY, "splines", PROPERTY_HINT_ARRAY_TYPE, "SplineResource"),
        "set_splines",
        "get_splines"
    );

    ClassDB::bind_method(D_METHOD("set_linked_geometry_resource", "geometry"), &DataOrientedSplines::set_linked_geometry_resource);
    ClassDB::bind_method(D_METHOD("get_linked_geometry_resource"), &DataOrientedSplines::get_linked_geometry_resource);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "linked_geometry_resource", PROPERTY_HINT_RESOURCE_TYPE, "DataOrientedGeometry"),
        "set_linked_geometry_resource",
        "get_linked_geometry_resource"
    );

    ClassDB::bind_method(D_METHOD("bind_data_oriented_geometry", "geometry"), &DataOrientedSplines::bind_data_oriented_geometry);

    ADD_GROUP("DOD Pipeline Configuration", "");

    ClassDB::bind_method(D_METHOD("set_dod_execution_mode", "mode"), &DataOrientedSplines::set_dod_execution_mode);
    ClassDB::bind_method(D_METHOD("get_dod_execution_mode"), &DataOrientedSplines::get_dod_execution_mode);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "dod_execution_mode", PROPERTY_HINT_ENUM, "Disabled:0,Composable Stages:1,Custom TaskGraphResource:2,Manual Graph:3"),
        "set_dod_execution_mode",
        "get_dod_execution_mode"
    );

    ClassDB::bind_method(D_METHOD("set_pipeline_stage_mask", "mask"), &DataOrientedSplines::set_pipeline_stage_mask);
    ClassDB::bind_method(D_METHOD("get_pipeline_stage_mask"), &DataOrientedSplines::get_pipeline_stage_mask);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "pipeline_stage_mask"), "set_pipeline_stage_mask", "get_pipeline_stage_mask");

    ClassDB::bind_method(D_METHOD("set_auto_connect_pipeline_stages", "auto_connect"), &DataOrientedSplines::set_auto_connect_pipeline_stages);
    ClassDB::bind_method(D_METHOD("get_auto_connect_pipeline_stages"), &DataOrientedSplines::get_auto_connect_pipeline_stages);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_connect_pipeline_stages"), "set_auto_connect_pipeline_stages", "get_auto_connect_pipeline_stages");

    ClassDB::bind_method(D_METHOD("set_default_subdivisions_per_section", "subdivs"), &DataOrientedSplines::set_default_subdivisions_per_section);
    ClassDB::bind_method(D_METHOD("get_default_subdivisions_per_section"), &DataOrientedSplines::get_default_subdivisions_per_section);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "default_subdivisions_per_section"), "set_default_subdivisions_per_section", "get_default_subdivisions_per_section");

    ClassDB::bind_method(D_METHOD("set_lut_bins_per_spline", "bins"), &DataOrientedSplines::set_lut_bins_per_spline);
    ClassDB::bind_method(D_METHOD("get_lut_bins_per_spline"), &DataOrientedSplines::get_lut_bins_per_spline);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "lut_bins_per_spline"), "set_lut_bins_per_spline", "get_lut_bins_per_spline");

    ClassDB::bind_method(D_METHOD("set_custom_task_graph_resource", "resource"), &DataOrientedSplines::set_custom_task_graph_resource);
    ClassDB::bind_method(D_METHOD("get_custom_task_graph_resource"), &DataOrientedSplines::get_custom_task_graph_resource);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "custom_task_graph_resource", PROPERTY_HINT_RESOURCE_TYPE, "TaskGraphResource"),
        "set_custom_task_graph_resource",
        "get_custom_task_graph_resource"
    );

    ADD_GROUP("DOD Buffer Capacities", "");

    ClassDB::bind_method(D_METHOD("set_max_dod_splines", "max"), &DataOrientedSplines::set_max_dod_splines);
    ClassDB::bind_method(D_METHOD("get_max_dod_splines"), &DataOrientedSplines::get_max_dod_splines);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_dod_splines"), "set_max_dod_splines", "get_max_dod_splines");

    ClassDB::bind_method(D_METHOD("set_max_dod_segments", "max"), &DataOrientedSplines::set_max_dod_segments);
    ClassDB::bind_method(D_METHOD("get_max_dod_segments"), &DataOrientedSplines::get_max_dod_segments);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_dod_segments"), "set_max_dod_segments", "get_max_dod_segments");

    ClassDB::bind_method(D_METHOD("set_max_dod_sample_requests", "max"), &DataOrientedSplines::set_max_dod_sample_requests);
    ClassDB::bind_method(D_METHOD("get_max_dod_sample_requests"), &DataOrientedSplines::get_max_dod_sample_requests);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_dod_sample_requests"), "set_max_dod_sample_requests", "get_max_dod_sample_requests");

    ClassDB::bind_method(D_METHOD("set_max_dod_query_points", "max"), &DataOrientedSplines::set_max_dod_query_points);
    ClassDB::bind_method(D_METHOD("get_max_dod_query_points"), &DataOrientedSplines::get_max_dod_query_points);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_dod_query_points"), "set_max_dod_query_points", "get_max_dod_query_points");

    // --- Methods ---
    ClassDB::bind_method(D_METHOD("setup_dod_pipeline"), &DataOrientedSplines::setup_dod_pipeline);
    ClassDB::bind_method(D_METHOD("bake_to_dod"), &DataOrientedSplines::bake_to_dod);
    ClassDB::bind_method(D_METHOD("rebuild_dod_task_graph"), &DataOrientedSplines::rebuild_dod_task_graph);
    ClassDB::bind_method(D_METHOD("execute_dod_step", "delta"), &DataOrientedSplines::execute_dod_step);

    ClassDB::bind_method(
        D_METHOD("stage_batch_samples", "spline_slots", "percents", "weighted", "compute_tangents"),
        &DataOrientedSplines::stage_batch_samples,
        DEFVAL(true),
        DEFVAL(true)
    );
    ClassDB::bind_method(D_METHOD("get_evaluated_outputs"), &DataOrientedSplines::get_evaluated_outputs);
    ClassDB::bind_method(D_METHOD("get_evaluated_tangents"), &DataOrientedSplines::get_evaluated_tangents);

    ClassDB::bind_method(D_METHOD("stage_proximity_queries", "query_points"), &DataOrientedSplines::stage_proximity_queries);
    ClassDB::bind_method(D_METHOD("get_closest_percents"), &DataOrientedSplines::get_closest_percents);
    ClassDB::bind_method(D_METHOD("get_closest_distances"), &DataOrientedSplines::get_closest_distances);
    ClassDB::bind_method(D_METHOD("get_closest_anchors"), &DataOrientedSplines::get_closest_anchors);

    ClassDB::bind_method(D_METHOD("get_spline_dod_slot", "spline"), &DataOrientedSplines::get_spline_dod_slot);
    ClassDB::bind_method(D_METHOD("get_spline_geometry_polyline_slot", "spline_slot"), &DataOrientedSplines::get_spline_geometry_polyline_slot);
    ClassDB::bind_method(D_METHOD("get_flattened_polyline_2d", "spline_slot"), &DataOrientedSplines::get_flattened_polyline_2d);
    ClassDB::bind_method(D_METHOD("get_flattened_polyline_3d", "spline_slot"), &DataOrientedSplines::get_flattened_polyline_3d);
    ClassDB::bind_method(D_METHOD("get_swept_bishop_transforms_3d", "spline_slot"), &DataOrientedSplines::get_swept_bishop_transforms_3d);

    ClassDB::bind_method(D_METHOD("get_buffer_id", "semantic_name"), &DataOrientedSplines::get_buffer_id);
    ClassDB::bind_method(D_METHOD("get_buffer_manifest_dictionary"), &DataOrientedSplines::get_buffer_manifest_dictionary);

    ClassDB::bind_method(
        D_METHOD("add_splines_transform_node", "logic_id", "properties", "priority", "use_registry_factory"),
        &DataOrientedSplines::add_splines_transform_node,
        DEFVAL(Dictionary()),
        DEFVAL(0),
        DEFVAL(false)
    );
    ClassDB::bind_method(
        D_METHOD("add_splines_query_node", "logic_id", "query_op", "properties", "priority", "use_registry_factory"),
        &DataOrientedSplines::add_splines_query_node,
        DEFVAL(0),
        DEFVAL(Dictionary()),
        DEFVAL(0),
        DEFVAL(false)
    );
    ClassDB::bind_method(
        D_METHOD("add_splines_metadata_node", "logic_id", "properties", "priority", "use_registry_factory"),
        &DataOrientedSplines::add_splines_metadata_node,
        DEFVAL(Dictionary()),
        DEFVAL(0),
        DEFVAL(false)
    );

    ClassDB::bind_method(
        D_METHOD("schedule_node_with_stage", "stage_flag", "custom_node_id", "join_downstream"),
        &DataOrientedSplines::schedule_node_with_stage,
        DEFVAL(false)
    );
    ClassDB::bind_method(
        D_METHOD("schedule_node_between_stages", "upstream_stage_flag", "downstream_stage_flag", "custom_node_id"),
        &DataOrientedSplines::schedule_node_between_stages
    );
}

core::SplinesBufferCapacities DataOrientedSplines::_build_capacities() const noexcept {
    core::SplinesBufferCapacities caps{};
    caps.max_splines         = static_cast<uint32_t>(std::max(16, max_dod_splines_));
    caps.max_segments        = static_cast<uint32_t>(std::max(128, max_dod_segments_));
    caps.max_lut_entries     = caps.max_splines * static_cast<uint32_t>(std::max(8, lut_bins_per_spline_));
    caps.max_sample_requests = static_cast<uint32_t>(std::max(64, max_dod_sample_requests_));
    caps.max_query_points    = static_cast<uint32_t>(std::max(64, max_dod_query_points_));
    return caps;
}

void DataOrientedSplines::_ensure_spline_dod_initialized(std::shared_ptr<core::MemoryManagerDOD> p_shared_manager) {
    if (!p_shared_manager && linked_geometry_resource_.is_valid()) {
        if (!linked_geometry_resource_->get_geometry_dod()) {
            linked_geometry_resource_->setup_dod_pipeline();
        }
        p_shared_manager = linked_geometry_resource_->get_memory_manager_shared();
    }

    const core::SplinesBufferCapacities caps = _build_capacities();
    if (!spline_dod_) {
        spline_dod_ = std::make_unique<core::SplineDOD>(std::move(p_shared_manager), caps);
    } else if (p_shared_manager && spline_dod_->get_memory_manager() != p_shared_manager) {
        spline_dod_ = std::make_unique<core::SplineDOD>(std::move(p_shared_manager), caps);
    }

    if (linked_geometry_resource_.is_valid() && linked_geometry_resource_->get_geometry_dod()) {
        spline_dod_->bind_external_geometry_buffers(
            linked_geometry_resource_->get_geometry_dod()->get_buffer_manifest()
        );
    }

    if (!active_task_graph_) {
        active_task_graph_ = spline_dod_->get_default_task_graph();
    }
}

void DataOrientedSplines::_clear_resource_maps() noexcept {
    object_id_to_spline_slot_.clear();
    spline_slot_to_resource_.clear();
}

void DataOrientedSplines::set_splines(const godot::TypedArray<SplineResource> &p_splines) {
    splines_ = p_splines;
    emit_changed();
}

void DataOrientedSplines::set_linked_geometry_resource(const godot::Ref<DataOrientedGeometry> &p_geometry) {
    linked_geometry_resource_ = p_geometry;
    emit_changed();
}

bool DataOrientedSplines::bind_data_oriented_geometry(const godot::Ref<DataOrientedGeometry> &p_geometry) {
    if (p_geometry.is_null()) return false;
    linked_geometry_resource_ = p_geometry;
    if (!p_geometry->get_geometry_dod()) {
        p_geometry->setup_dod_pipeline();
    }
    setup_dod_pipeline_with_manager(
        p_geometry->get_memory_manager_shared(),
        p_geometry->get_active_task_graph_shared()
    );
    return true;
}

void DataOrientedSplines::set_dod_execution_mode(SplinesDODExecutionMode p_mode) {
    dod_execution_mode_ = p_mode;
}

void DataOrientedSplines::set_pipeline_stage_mask(int p_mask) {
    pipeline_stage_mask_ = static_cast<uint32_t>(p_mask) & core::SPLINES_STAGE_ALL_MASK;
}

void DataOrientedSplines::set_auto_connect_pipeline_stages(bool p_auto_connect) {
    auto_connect_pipeline_stages_ = p_auto_connect;
}

void DataOrientedSplines::set_default_subdivisions_per_section(int p_subdivs) {
    default_subdivisions_per_section_ = std::clamp(p_subdivs, 1, 128);
}

void DataOrientedSplines::set_lut_bins_per_spline(int p_bins) {
    lut_bins_per_spline_ = std::clamp(p_bins, 8, 512);
}

void DataOrientedSplines::set_custom_task_graph_resource(const godot::Ref<TaskGraphResource> &p_resource) {
    custom_task_graph_resource_ = p_resource;
}

void DataOrientedSplines::set_max_dod_splines(int p_max) {
    max_dod_splines_ = std::max(16, p_max);
}

void DataOrientedSplines::set_max_dod_segments(int p_max) {
    max_dod_segments_ = std::max(128, p_max);
}

void DataOrientedSplines::set_max_dod_sample_requests(int p_max) {
    max_dod_sample_requests_ = std::max(64, p_max);
}

void DataOrientedSplines::set_max_dod_query_points(int p_max) {
    max_dod_query_points_ = std::max(64, p_max);
}

void DataOrientedSplines::setup_dod_pipeline_with_manager(
    std::shared_ptr<core::MemoryManagerDOD> p_shared_manager,
    std::shared_ptr<core::TaskGraphDOD> p_shared_graph
) {
    _ensure_spline_dod_initialized(std::move(p_shared_manager));
    if (p_shared_graph) {
        active_task_graph_ = std::move(p_shared_graph);
        spline_dod_->set_default_task_graph(active_task_graph_);
    }
    bake_to_dod();
    rebuild_dod_task_graph();
}

void DataOrientedSplines::setup_dod_pipeline() {
    setup_dod_pipeline_with_manager(nullptr, nullptr);
}

void DataOrientedSplines::bake_to_dod() {
    _ensure_spline_dod_initialized();
    spline_dod_->reset_all_slots();
    _clear_resource_maps();

    core::GeometryDOD *linked_geom = (linked_geometry_resource_.is_valid())
        ? linked_geometry_resource_->get_geometry_dod()
        : nullptr;
    if (linked_geom) {
        spline_dod_->bind_external_geometry_buffers(linked_geom->get_buffer_manifest());
    }

    const uint8_t subdivs = static_cast<uint8_t>(std::clamp(default_subdivisions_per_section_, 1, 128));
    const uint32_t bins   = static_cast<uint32_t>(std::clamp(lut_bins_per_spline_, 8, 512));

    for (int64_t i = 0; i < splines_.size(); ++i) {
        godot::Ref<SplineResource> sp = splines_[i];
        if (sp.is_null()) continue;

        uint32_t slot = core::INVALID_ID;
        if (Spline1DResource *s1 = godot::Object::cast_to<Spline1DResource>(sp.ptr())) {
            slot = spline_dod_->bake_spline(s1->get_core_spline(), subdivs, 0u, bins, linked_geom);
        } else if (Spline2DResource *s2 = godot::Object::cast_to<Spline2DResource>(sp.ptr())) {
            slot = spline_dod_->bake_spline(s2->get_core_spline(), subdivs, 0u, bins, linked_geom);
        } else if (Spline3DResource *s3 = godot::Object::cast_to<Spline3DResource>(sp.ptr())) {
            slot = spline_dod_->bake_spline(s3->get_core_spline(), subdivs, 0u, bins, linked_geom);
        } else if (Spline4DResource *s4 = godot::Object::cast_to<Spline4DResource>(sp.ptr())) {
            slot = spline_dod_->bake_spline(s4->get_core_spline(), subdivs, 0u, bins, linked_geom);
        }

        if (slot != core::INVALID_ID) {
            object_id_to_spline_slot_[sp->get_instance_id()] = slot;
            spline_slot_to_resource_[slot]                   = sp;
        }
    }

    spline_dod_->sync_selection_ranges();
}

void DataOrientedSplines::rebuild_dod_task_graph() {
    _ensure_spline_dod_initialized();
    if (!active_task_graph_) return;

    if (dod_execution_mode_ == SPLINES_DOD_EXEC_DISABLED ||
        dod_execution_mode_ == SPLINES_DOD_EXEC_MANUAL_GRAPH) {
        return;
    }

    if (dod_execution_mode_ == SPLINES_DOD_EXEC_CUSTOM_GRAPH_RESOURCE &&
        custom_task_graph_resource_.is_valid()) {
        ui_to_dod_map_.clear();
        custom_task_graph_resource_->compile_to_task_graph(*active_task_graph_, ui_to_dod_map_);
        return;
    }

    spline_dod_->inject_pipeline_stages(
        *active_task_graph_,
        pipeline_stage_mask_,
        auto_connect_pipeline_stages_
    );
}

void DataOrientedSplines::execute_dod_step(double p_delta) {
    if (dod_execution_mode_ == SPLINES_DOD_EXEC_DISABLED || !spline_dod_) return;
    spline_dod_->execute_step(p_delta, active_task_graph_.get());
}

void DataOrientedSplines::stage_batch_samples(
    const godot::PackedInt32Array &p_spline_slots,
    const godot::PackedFloat32Array &p_percents,
    bool p_weighted,
    bool p_compute_tangents
) {
    _ensure_spline_dod_initialized();
    const int64_t n = std::min(p_spline_slots.size(), p_percents.size());
    std::vector<core::SplineSampleRequestPOD> reqs(static_cast<size_t>(n));
    for (int64_t i = 0; i < n; ++i) {
        reqs[static_cast<size_t>(i)].spline_slot     = static_cast<uint32_t>(std::max<int32_t>(0, p_spline_slots[i]));
        reqs[static_cast<size_t>(i)].percent         = p_percents[i];
        reqs[static_cast<size_t>(i)].is_weighted     = p_weighted ? 1u : 0u;
        reqs[static_cast<size_t>(i)].compute_tangent = p_compute_tangents ? 1u : 0u;
    }
    spline_dod_->stage_sample_requests(reqs);
}

godot::PackedVector4Array DataOrientedSplines::get_evaluated_outputs() const {
    godot::PackedVector4Array out;
    if (!spline_dod_) return out;
    const auto span = spline_dod_->get_evaluated_outputs();
    out.resize(static_cast<int64_t>(span.size()));
    for (size_t i = 0; i < span.size(); ++i) {
        out.set(static_cast<int64_t>(i), godot::Vector4(span[i].x, span[i].y, span[i].z, span[i].w));
    }
    return out;
}

godot::PackedVector4Array DataOrientedSplines::get_evaluated_tangents() const {
    godot::PackedVector4Array out;
    if (!spline_dod_) return out;
    const auto span = spline_dod_->get_evaluated_tangents();
    out.resize(static_cast<int64_t>(span.size()));
    for (size_t i = 0; i < span.size(); ++i) {
        out.set(static_cast<int64_t>(i), godot::Vector4(span[i].x, span[i].y, span[i].z, span[i].w));
    }
    return out;
}

void DataOrientedSplines::stage_proximity_queries(const godot::PackedVector4Array &p_query_points) {
    _ensure_spline_dod_initialized();
    std::vector<glm::vec4> qpts(static_cast<size_t>(p_query_points.size()));
    for (int64_t i = 0; i < p_query_points.size(); ++i) {
        const godot::Vector4 &v = p_query_points[i];
        qpts[static_cast<size_t>(i)] = glm::vec4(v.x, v.y, v.z, v.w);
    }
    spline_dod_->stage_proximity_query_points(qpts);
}

godot::PackedFloat32Array DataOrientedSplines::get_closest_percents() const {
    godot::PackedFloat32Array out;
    if (!spline_dod_) return out;
    const auto span = spline_dod_->get_closest_percents();
    out.resize(static_cast<int64_t>(span.size()));
    for (size_t i = 0; i < span.size(); ++i) {
        out.set(static_cast<int64_t>(i), span[i]);
    }
    return out;
}

godot::PackedFloat32Array DataOrientedSplines::get_closest_distances() const {
    godot::PackedFloat32Array out;
    if (!spline_dod_) return out;
    const auto span = spline_dod_->get_closest_distances();
    out.resize(static_cast<int64_t>(span.size()));
    for (size_t i = 0; i < span.size(); ++i) {
        out.set(static_cast<int64_t>(i), span[i]);
    }
    return out;
}

godot::PackedInt32Array DataOrientedSplines::get_closest_anchors() const {
    godot::PackedInt32Array out;
    if (!spline_dod_) return out;
    const auto span = spline_dod_->get_closest_anchors();
    out.resize(static_cast<int64_t>(span.size()));
    for (size_t i = 0; i < span.size(); ++i) {
        out.set(static_cast<int64_t>(i), span[i]);
    }
    return out;
}

int DataOrientedSplines::get_spline_dod_slot(const godot::Ref<SplineResource> &p_spline) const {
    if (p_spline.is_null()) return -1;
    auto it = object_id_to_spline_slot_.find(p_spline->get_instance_id());
    return (it != object_id_to_spline_slot_.end()) ? static_cast<int>(it->second) : -1;
}

int DataOrientedSplines::get_spline_geometry_polyline_slot(int p_spline_slot) const {
    if (!spline_dod_ || p_spline_slot < 0) return -1;
    const core::SplineHeaderPOD hdr = spline_dod_->get_spline_header(static_cast<uint32_t>(p_spline_slot));
    return (hdr.geom_polyline_slot != 0xFFFFFFFFu) ? static_cast<int>(hdr.geom_polyline_slot) : p_spline_slot;
}

godot::PackedVector2Array DataOrientedSplines::get_flattened_polyline_2d(int p_spline_slot) const {
    godot::PackedVector2Array out;
    if (!spline_dod_ || p_spline_slot < 0) return out;
    const auto span = spline_dod_->get_flattened_polyline_vertices_2d(static_cast<uint32_t>(p_spline_slot));
    out.resize(static_cast<int64_t>(span.size()));
    for (size_t i = 0; i < span.size(); ++i) {
        out.set(static_cast<int64_t>(i), godot::Vector2(span[i].x, span[i].y));
    }
    return out;
}

godot::PackedVector3Array DataOrientedSplines::get_flattened_polyline_3d(int p_spline_slot) const {
    godot::PackedVector3Array out;
    if (!spline_dod_ || p_spline_slot < 0) return out;
    const auto span = spline_dod_->get_flattened_polyline_vertices_3d(static_cast<uint32_t>(p_spline_slot));
    out.resize(static_cast<int64_t>(span.size()));
    for (size_t i = 0; i < span.size(); ++i) {
        out.set(static_cast<int64_t>(i), godot::Vector3(span[i].x, span[i].y, span[i].z));
    }
    return out;
}

godot::TypedArray<godot::Transform3D> DataOrientedSplines::get_swept_bishop_transforms_3d(int p_spline_slot) const {
    godot::TypedArray<godot::Transform3D> out;
    if (!spline_dod_ || p_spline_slot < 0) return out;
    const auto frames = spline_dod_->get_swept_bishop_frames_3d(static_cast<uint32_t>(p_spline_slot));
    for (const auto &f : frames) {
        const godot::Vector3 x_col(f.binormal.x * f.scale, f.binormal.y * f.scale, f.binormal.z * f.scale);
        const godot::Vector3 y_col(f.normal.x * f.scale, f.normal.y * f.scale, f.normal.z * f.scale);
        const godot::Vector3 z_col(f.tangent.x, f.tangent.y, f.tangent.z);
        const godot::Vector3 orig(f.position.x, f.position.y, f.position.z);
        out.push_back(godot::Transform3D(godot::Basis(x_col, y_col, z_col), orig));
    }
    return out;
}

int DataOrientedSplines::get_buffer_id(const godot::StringName &p_semantic_name) const {
    if (!spline_dod_) return -1;
    const godot::String s(p_semantic_name);
    const uint32_t id = spline_dod_->get_buffer_id_by_name(s.utf8().get_data());
    return (id != core::INVALID_ID) ? static_cast<int>(id) : -1;
}

godot::Dictionary DataOrientedSplines::get_buffer_manifest_dictionary() const {
    godot::Dictionary d;
    if (spline_dod_) {
        spline_dod_->populate_task_buffer_dictionary(d, true);
    }
    return d;
}

int DataOrientedSplines::add_splines_transform_node(
    int p_logic_id,
    const godot::Dictionary &p_properties,
    int p_priority,
    bool p_use_registry_factory
) {
    _ensure_spline_dod_initialized();
    if (!active_task_graph_) return -1;
    core::SplinesNodeExecutionConfig cfg{};
    cfg.execution_priority   = p_priority;
    cfg.use_registry_factory = p_use_registry_factory;
    const core::NodeID nid = spline_dod_->add_transform_node_by_id(
        *active_task_graph_,
        static_cast<core::SplinesTransformLogicID>(p_logic_id),
        p_properties,
        cfg
    );
    return (nid != core::INVALID_ID) ? static_cast<int>(nid) : -1;
}

int DataOrientedSplines::add_splines_query_node(
    int p_logic_id,
    int p_query_op,
    const godot::Dictionary &p_properties,
    int p_priority,
    bool p_use_registry_factory
) {
    _ensure_spline_dod_initialized();
    if (!active_task_graph_) return -1;
    core::SplinesNodeExecutionConfig cfg{};
    cfg.query_op             = (p_query_op == 1) ? core::QueryOp::ADD : core::QueryOp::CULL;
    cfg.execution_priority   = p_priority;
    cfg.use_registry_factory = p_use_registry_factory;
    const core::NodeID nid = spline_dod_->add_query_node_by_id(
        *active_task_graph_,
        static_cast<core::SplinesQueryLogicID>(p_logic_id),
        p_properties,
        cfg
    );
    return (nid != core::INVALID_ID) ? static_cast<int>(nid) : -1;
}

int DataOrientedSplines::add_splines_metadata_node(
    int p_logic_id,
    const godot::Dictionary &p_properties,
    int p_priority,
    bool p_use_registry_factory
) {
    _ensure_spline_dod_initialized();
    if (!active_task_graph_) return -1;
    core::SplinesNodeExecutionConfig cfg{};
    cfg.execution_priority   = p_priority;
    cfg.use_registry_factory = p_use_registry_factory;
    const core::NodeID nid = spline_dod_->add_metadata_node_by_id(
        *active_task_graph_,
        static_cast<core::SplinesMetadataLogicID>(p_logic_id),
        p_properties,
        cfg
    );
    return (nid != core::INVALID_ID) ? static_cast<int>(nid) : -1;
}

bool DataOrientedSplines::schedule_node_with_stage(int p_stage_flag, int p_custom_node_id, bool p_join_downstream) {
    if (!spline_dod_ || !active_task_graph_ || p_custom_node_id < 0) return false;
    return core::SplineDOD::schedule_node_with_stage(
        *active_task_graph_,
        spline_dod_->get_active_topology(),
        static_cast<core::SplinesPipelineStageFlags>(p_stage_flag),
        static_cast<core::NodeID>(p_custom_node_id),
        p_join_downstream
    );
}

bool DataOrientedSplines::schedule_node_between_stages(int p_upstream_stage_flag, int p_downstream_stage_flag, int p_custom_node_id) {
    if (!spline_dod_ || !active_task_graph_ || p_custom_node_id < 0) return false;
    return core::SplineDOD::schedule_node_between_stages(
        *active_task_graph_,
        spline_dod_->get_active_topology(),
        static_cast<core::SplinesPipelineStageFlags>(p_upstream_stage_flag),
        static_cast<core::SplinesPipelineStageFlags>(p_downstream_stage_flag),
        static_cast<core::NodeID>(p_custom_node_id)
    );
}

} // namespace ideam::godot_ext
