#include "data_oriented_cymatics.h"

#include <godot_cpp/variant/dictionary.hpp>

#include <algorithm>
#include <cstring>

namespace ideam::godot_ext {

void DataOrientedCymatics::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_memory_manager_resource", "mem"), &DataOrientedCymatics::set_memory_manager_resource);
    ClassDB::bind_method(D_METHOD("get_memory_manager_resource"), &DataOrientedCymatics::get_memory_manager_resource);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "memory_manager_resource", PROPERTY_HINT_RESOURCE_TYPE, "MemoryManagerResource"),
        "set_memory_manager_resource",
        "get_memory_manager_resource"
    );

    ClassDB::bind_method(D_METHOD("set_task_graph_resource", "graph"), &DataOrientedCymatics::set_task_graph_resource);
    ClassDB::bind_method(D_METHOD("get_task_graph_resource"), &DataOrientedCymatics::get_task_graph_resource);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "task_graph_resource", PROPERTY_HINT_RESOURCE_TYPE, "TaskGraphResource"),
        "set_task_graph_resource",
        "get_task_graph_resource"
    );

    ClassDB::bind_method(D_METHOD("set_grid_width", "width"), &DataOrientedCymatics::set_grid_width);
    ClassDB::bind_method(D_METHOD("get_grid_width"), &DataOrientedCymatics::get_grid_width);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "grid_width"), "set_grid_width", "get_grid_width");

    ClassDB::bind_method(D_METHOD("set_grid_height", "height"), &DataOrientedCymatics::set_grid_height);
    ClassDB::bind_method(D_METHOD("get_grid_height"), &DataOrientedCymatics::get_grid_height);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "grid_height"), "set_grid_height", "get_grid_height");

    ClassDB::bind_method(D_METHOD("set_grid_depth", "depth"), &DataOrientedCymatics::set_grid_depth);
    ClassDB::bind_method(D_METHOD("get_grid_depth"), &DataOrientedCymatics::get_grid_depth);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "grid_depth"), "set_grid_depth", "get_grid_depth");

    ClassDB::bind_method(
        D_METHOD("execute_pipeline_direct", "stage_mask", "fft_peaks"),
        &DataOrientedCymatics::execute_pipeline_direct,
        DEFVAL(static_cast<int>(core::CYM_STAGE_DEFAULT_MASK)),
        DEFVAL(PackedFloat32Array())
    );
    ClassDB::bind_method(D_METHOD("inject_impulse", "x", "y", "amplitude"), &DataOrientedCymatics::inject_impulse);
    ClassDB::bind_method(D_METHOD("set_listener", "grid_pos", "yaw_rad"), &DataOrientedCymatics::set_listener);
    ClassDB::bind_method(D_METHOD("set_primary_source", "grid_pos"), &DataOrientedCymatics::set_primary_source);

    ClassDB::bind_method(D_METHOD("read_modal_field"), &DataOrientedCymatics::read_modal_field);
    ClassDB::bind_method(D_METHOD("read_shadow_classes"), &DataOrientedCymatics::read_shadow_classes);
    ClassDB::bind_method(D_METHOD("read_sectors_as_dicts"), &DataOrientedCymatics::read_sectors_as_dicts);
}

DataOrientedCymatics::DataOrientedCymatics() {
    dod_engine_ = std::make_unique<core::CymaticsDOD>();
}

void DataOrientedCymatics::set_grid_width(int p_w) {
    grid_width_ = std::clamp(p_w, 3, 256);
    if (dod_engine_) {
        auto cfg  = dod_engine_->get_config();
        cfg.width = static_cast<uint32_t>(grid_width_);
        dod_engine_->configure(cfg);
    }
}

void DataOrientedCymatics::set_grid_height(int p_h) {
    grid_height_ = std::clamp(p_h, 3, 256);
    if (dod_engine_) {
        auto cfg   = dod_engine_->get_config();
        cfg.height = static_cast<uint32_t>(grid_height_);
        dod_engine_->configure(cfg);
    }
}

void DataOrientedCymatics::set_grid_depth(int p_d) {
    grid_depth_ = std::clamp(p_d, 1, 64);
    if (dod_engine_) {
        auto cfg  = dod_engine_->get_config();
        cfg.depth = static_cast<uint32_t>(grid_depth_);
        dod_engine_->configure(cfg);
    }
}

bool DataOrientedCymatics::execute_pipeline_direct(int p_stage_mask, const godot::PackedFloat32Array &p_fft_peaks) {
    if (!dod_engine_) {
        return false;
    }
    const float *fft_ptr = p_fft_peaks.is_empty() ? nullptr : p_fft_peaks.ptr();
    return dod_engine_->execute_pipeline(static_cast<uint32_t>(p_stage_mask), fft_ptr, static_cast<uint32_t>(p_fft_peaks.size()));
}

void DataOrientedCymatics::inject_impulse(int p_x, int p_y, float p_amplitude) {
    if (dod_engine_ && p_x >= 0 && p_y >= 0) {
        dod_engine_->inject_impulse(static_cast<uint32_t>(p_x), static_cast<uint32_t>(p_y), p_amplitude);
    }
}

void DataOrientedCymatics::set_listener(const godot::Vector2 &p_grid_pos, float p_yaw_rad) {
    if (dod_engine_) {
        dod_engine_->set_listener(p_grid_pos.x, p_grid_pos.y, p_yaw_rad);
    }
}

void DataOrientedCymatics::set_primary_source(const godot::Vector2 &p_grid_pos) {
    if (dod_engine_) {
        dod_engine_->set_primary_source(p_grid_pos.x, p_grid_pos.y);
    }
}

godot::PackedFloat32Array DataOrientedCymatics::read_modal_field() const {
    godot::PackedFloat32Array out;
    if (!dod_engine_) return out;
    const auto &f = dod_engine_->get_modal_field();
    out.resize(static_cast<int64_t>(f.size()));
    if (!f.empty()) {
        std::memcpy(out.ptrw(), f.data(), f.size() * sizeof(float));
    }
    return out;
}

godot::PackedInt32Array DataOrientedCymatics::read_shadow_classes() const {
    godot::PackedInt32Array out;
    if (!dod_engine_) return out;
    const auto &cls = dod_engine_->get_shadow_classes();
    out.resize(static_cast<int64_t>(cls.size()));
    for (size_t i = 0; i < cls.size(); ++i) {
        out.set(static_cast<int64_t>(i), static_cast<int32_t>(cls[i]));
    }
    return out;
}

godot::Array DataOrientedCymatics::read_sectors_as_dicts() const {
    godot::Array out;
    if (!dod_engine_) return out;
    for (const auto &s : dod_engine_->get_sectors()) {
        godot::Dictionary d;
        d["energy"]            = s.energy;
        d["peak_hold"]         = s.peak_hold;
        d["dominant_category"] = s.dominant_category;
        d["azimuth_rad"]       = s.azimuth_rad;
        out.append(d);
    }
    return out;
}

} // namespace ideam::godot_ext
