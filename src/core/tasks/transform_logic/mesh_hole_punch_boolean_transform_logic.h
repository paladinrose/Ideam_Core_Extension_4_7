#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

enum class HolePunchShapeMode : uint8_t {
    CYLINDRICAL_BORE = 0,
    SPHERICAL_CAVITY = 1
};

/**
 * MeshHolePunchBooleanTransformLogic<T> (M29)
 * Cylindrical or spherical CSG hole punch through thin-walled 3D meshes along
 * (punch_origin, punch_axis, punch_radius), radially projecting interior vertices onto
 * the bore wall with entry/exit crater lip flaring.
 */
template <typename T = godot::Vector3>
struct alignas(64) MeshHolePunchBooleanTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::VECTOR3 | DataType::VECTOR3D | DataType::INT32;

    // --- Explicit Spatial Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "CSG Cylindrical/Spherical Hole Punch";
    static constexpr std::string_view ui_category  = "Model/Destruction";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t vertex_buffer_id = INVALID_ID; // 4B: Primary target vertex buffer (Vector3)
    uint32_t index_buffer_id  = INVALID_ID; // 4B: Optional triangle index buffer (int32_t)

    godot::Vector3 punch_origin{0.0f, 0.0f, 0.0f}; // 12B: Bore entry or sphere center
    godot::Vector3 punch_axis{0.0f, 0.0f, -1.0f};  // 12B: Normalized bore axis

    float punch_radius                 = 0.5f;  // 4B: Bore/cavity radius (meters)
    float punch_depth                  = 4.0f;  // 4B: Axial penetration depth (meters)
    float rim_flare_height             = 0.12f; // 4B: Outward crater lip height (meters)
    float rim_flare_width              = 0.25f; // 4B: Radial width of raised crater rim (meters)
    HolePunchShapeMode shape_mode      = HolePunchShapeMode::CYLINDRICAL_BORE; // 1B
    bool cull_fully_interior_triangles = true;  // 1B: Degenerate triangles fully inside bore
    uint8_t reserved_padding[14]       = {0};   // 14B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary vtx_prop;
        vtx_prop["name"] = "vertex_buffer_id";
        vtx_prop["type"] = godot::Variant::INT;
        vtx_prop["hint_string"] = "buffer_option";
        props.push_back(vtx_prop);

        godot::Dictionary idx_prop;
        idx_prop["name"] = "index_buffer_id";
        idx_prop["type"] = godot::Variant::INT;
        idx_prop["hint_string"] = "buffer_option";
        props.push_back(idx_prop);

        godot::Dictionary mode_prop;
        mode_prop["name"] = "shape_mode";
        mode_prop["type"] = godot::Variant::INT;
        mode_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        mode_prop["hint_string"] = "Cylindrical Bore:0,Spherical Cavity:1";
        props.push_back(mode_prop);

        godot::Dictionary orig_prop;
        orig_prop["name"] = "punch_origin";
        orig_prop["type"] = godot::Variant::VECTOR3;
        orig_prop["hint_string"] = "suffix:m";
        props.push_back(orig_prop);

        godot::Dictionary axis_prop;
        axis_prop["name"] = "punch_axis";
        axis_prop["type"] = godot::Variant::VECTOR3;
        props.push_back(axis_prop);

        godot::Dictionary rad_prop;
        rad_prop["name"] = "punch_radius";
        rad_prop["type"] = godot::Variant::FLOAT;
        rad_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        rad_prop["hint_string"] = "0.01,25.0,0.01,suffix:m";
        props.push_back(rad_prop);

        godot::Dictionary dep_prop;
        dep_prop["name"] = "punch_depth";
        dep_prop["type"] = godot::Variant::FLOAT;
        dep_prop["hint_string"] = "suffix:m";
        props.push_back(dep_prop);

        godot::Dictionary fh_prop;
        fh_prop["name"] = "rim_flare_height";
        fh_prop["type"] = godot::Variant::FLOAT;
        fh_prop["hint_string"] = "suffix:m";
        props.push_back(fh_prop);

        godot::Dictionary fw_prop;
        fw_prop["name"] = "rim_flare_width";
        fw_prop["type"] = godot::Variant::FLOAT;
        fw_prop["hint_string"] = "suffix:m";
        props.push_back(fw_prop);

        godot::Dictionary cull_prop;
        cull_prop["name"] = "cull_fully_interior_triangles";
        cull_prop["type"] = godot::Variant::BOOL;
        props.push_back(cull_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return (vertex_buffer_id != INVALID_ID) ? vertex_buffer_id : index_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("vertex_buffer_id")) {
            vertex_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["vertex_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && vertex_buffer_id == INVALID_ID) {
            vertex_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        } else if (p_props.has("buffer_id") && vertex_buffer_id == INVALID_ID) {
            vertex_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["buffer_id"]));
        }

        if (p_props.has("index_buffer_id")) {
            index_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["index_buffer_id"]));
        }
        if (p_props.has("shape_mode")) {
            shape_mode = static_cast<HolePunchShapeMode>(std::clamp<int64_t>(static_cast<int64_t>(p_props["shape_mode"]), 0, 1));
        }
        if (p_props.has("punch_origin")) {
            punch_origin = static_cast<godot::Vector3>(p_props["punch_origin"]);
        }
        if (p_props.has("punch_axis")) {
            const godot::Vector3 a = static_cast<godot::Vector3>(p_props["punch_axis"]);
            punch_axis = (a.length_squared() > 1e-8f) ? a.normalized() : godot::Vector3(0.0f, 0.0f, -1.0f);
        }
        if (p_props.has("punch_radius")) {
            punch_radius = std::max(1e-4f, static_cast<float>(p_props["punch_radius"]));
        }
        if (p_props.has("punch_depth")) {
            punch_depth = std::max(1e-4f, static_cast<float>(p_props["punch_depth"]));
        }
        if (p_props.has("rim_flare_height")) {
            rim_flare_height = static_cast<float>(p_props["rim_flare_height"]);
        }
        if (p_props.has("rim_flare_width")) {
            rim_flare_width = std::max(1e-4f, static_cast<float>(p_props["rim_flare_width"]));
        }
        if (p_props.has("cull_fully_interior_triangles")) {
            cull_fully_interior_triangles = static_cast<bool>(p_props["cull_fully_interior_triangles"]);
        }
    }

    template <typename T_View, typename T_Strategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_context, T_View& p_view) const {
        if constexpr (std::is_same_v<T, godot::Vector3>) {
            const GrantPartPOD* vtx_part = p_context.get_grant_part(vertex_buffer_id);
            if (!vtx_part || !vtx_part->raw_base_ptr) return;

            GrantPartPOD* idx_part = (index_buffer_id != INVALID_ID)
                ? const_cast<GrantPartPOD*>(p_context.get_grant_part(index_buffer_id))
                : nullptr;

            const godot::Vector3 axis = (punch_axis.length_squared() > 1e-8f)
                ? punch_axis.normalized()
                : godot::Vector3(0.0f, 0.0f, -1.0f);
            const float outer_band = punch_radius + rim_flare_width;

            auto process_vertex = [&](int64_t i) noexcept {
                T v = _read_view(p_view, i);
                const T rel = v - punch_origin;

                if (shape_mode == HolePunchShapeMode::SPHERICAL_CAVITY) {
                    const float d = rel.length();
                    if (d < outer_band) {
                        const T dir = (d > 1e-6f) ? (rel / d) : axis;
                        if (d < punch_radius) {
                            v = punch_origin + dir * punch_radius;
                            if (vtx_part->selection.partition_ids && i < vtx_part->selection.capacity) {
                                vtx_part->selection.partition_ids[i] = 1;
                            }
                        } else {
                            const float t = 1.0f - (d - punch_radius) / rim_flare_width;
                            v += dir * (rim_flare_height * t * t);
                        }
                        _write_view(p_view, i, v);
                    }
                } else {
                    const float axial = rel.dot(axis);
                    if (axial < -punch_depth * 0.5f || axial > punch_depth) return;

                    const T radial = rel - axis * axial;
                    const float r = radial.length();
                    if (r >= outer_band) return;

                    const T r_dir = (r > 1e-6f)
                        ? (radial / r)
                        : ((std::abs(axis.y) < 0.99f) ? axis.cross(T(0.0f, 1.0f, 0.0f)).normalized()
                                                      : axis.cross(T(1.0f, 0.0f, 0.0f)).normalized());

                    if (r < punch_radius) {
                        const float sign_axial = (axial >= 0.0f) ? 1.0f : -1.0f;
                        v = punch_origin + axis * (axial + sign_axial * rim_flare_height) + r_dir * punch_radius;
                        if (vtx_part->selection.partition_ids && i < vtx_part->selection.capacity) {
                            vtx_part->selection.partition_ids[i] = 1;
                        }
                    } else {
                        const float t = 1.0f - (r - punch_radius) / rim_flare_width;
                        const float sign_axial = (axial >= 0.0f) ? -1.0f : 1.0f;
                        v += axis * (sign_axial * rim_flare_height * t * t);
                    }
                    _write_view(p_view, i, v);
                }
            };

            const MemoryBufferSelectionPOD& sel = vtx_part->selection;
            if (sel.mode == SelectionMode::DENSE && sel.data.bitset) {
                const uint64_t* bitset = sel.data.bitset;
                for (int64_t i = 0; i < sel.capacity; ++i) {
                    if (bitset[i >> 6] & (1ULL << (i & 63))) process_vertex(i);
                }
            } else if (sel.mode == SelectionMode::SPARSE && sel.data.indices) {
                for (int64_t i = 0; i < sel.element_count; ++i) {
                    process_vertex(sel.data.indices[i]);
                }
            } else {
                const int64_t end = sel.start_index + sel.element_count;
                for (int64_t i = sel.start_index; i < end; ++i) {
                    process_vertex(i);
                }
            }

            if (cull_fully_interior_triangles && idx_part && idx_part->raw_base_ptr && vtx_part->selection.partition_ids) {
                int32_t* indices = reinterpret_cast<int32_t*>(idx_part->raw_base_ptr);
                const int32_t n_indices = (idx_part->selection.element_count > 0)
                    ? static_cast<int32_t>(idx_part->selection.element_count)
                    : static_cast<int32_t>(idx_part->capacity_bytes / sizeof(int32_t));
                const int32_t n_tris = n_indices / 3;
                const int64_t v_cap = vtx_part->selection.capacity;

                for (int32_t t = 0; t < n_tris; ++t) {
                    const int32_t i0 = indices[t * 3 + 0];
                    const int32_t i1 = indices[t * 3 + 1];
                    const int32_t i2 = indices[t * 3 + 2];
                    if (i0 < 0 || i0 >= v_cap || i1 < 0 || i1 >= v_cap || i2 < 0 || i2 >= v_cap) continue;
                    if (vtx_part->selection.partition_ids[i0] == 1 &&
                        vtx_part->selection.partition_ids[i1] == 1 &&
                        vtx_part->selection.partition_ids[i2] == 1) {
                        indices[t * 3 + 1] = i0;
                        indices[t * 3 + 2] = i0;
                    }
                }
            }
        }
    }

private:
    template <typename T_View>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline T _read_view(const T_View& p_view, int64_t idx) const noexcept {
        if constexpr (requires { p_view[idx].read(); }) {
            return static_cast<T>(p_view[idx].read());
        } else if constexpr (requires { p_view[idx].load(); }) {
            return static_cast<T>(p_view[idx].load());
        } else if constexpr (std::is_pointer_v<decltype(p_view[idx])>) {
            return *reinterpret_cast<const T*>(p_view[idx]);
        } else {
            return static_cast<T>(p_view[idx]);
        }
    }

    template <typename T_View>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline void _write_view(const T_View& p_view, int64_t idx, const T& value) const noexcept {
        if constexpr (std::is_pointer_v<decltype(p_view[idx])>) {
            *reinterpret_cast<T*>(p_view[idx]) = value;
        } else if constexpr (requires { p_view[idx].write(); }) {
            p_view[idx] = value;
        } else if constexpr (requires { p_view[idx].store(value); }) {
            p_view[idx].store(value);
        } else {
            (void)(p_view[idx] = value);
        }
    }
};

static_assert(sizeof(MeshHolePunchBooleanTransformLogic<godot::Vector3>) == 64, "MeshHolePunchBooleanTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<MeshHolePunchBooleanTransformLogic<godot::Vector3>>, "MeshHolePunchBooleanTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core