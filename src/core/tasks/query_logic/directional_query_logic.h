#pragma once

#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector4.hpp>
#include <algorithm>
#include <cmath>
#include <bit>

namespace ideam::core {

template <typename T>
struct DirectionalQueryLogic {
    using ValueType       = T; 
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS | ViewCapability::RANDOM_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::VECTOR2 | DataType::VECTOR3 | DataType::VECTOR4 | DataType::VECTOR2D | DataType::VECTOR3D | DataType::VECTOR4D;
    
    // --- Explicit Spatial Contracts ---
    static constexpr size_t dimensions = 0; // Point-based lookup
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size = 0;
    
    static constexpr size_t transient_workspace_bytes     = 0;

    static constexpr bool supports_cull = true;
    static constexpr bool supports_addition = true;

    static constexpr std::string_view display_name = "Directional";
    static constexpr std::string_view ui_category  = "Spatial";
    
    /**
     * Comparison modes:
     * - WITHIN_ANGLE / OUTSIDE_ANGLE / FACING_EACH_OTHER / PERPENDICULAR: Directional dot-product cone tests.
     * - COLLINEAR_ALIGNMENT: Closed-form 3D/ND multi-target collinearity test ported from legacy
     *   `old_gdscript_plugins/ideam_sensors/scripts/alignment3d_sensor.gd`. Evaluates whether each point P_i
     *   lies within perpendicular distance `alignment_tolerance` (epsilon) of the reference line passing
     *   through `line_start` (A) and `line_end` (B):
     *       ||(P_i - A) x (B - A)||^2 / ||B - A||^2 <= epsilon^2
     */
    enum class Comparison : uint8_t {
        WITHIN_ANGLE        = 0,
        OUTSIDE_ANGLE       = 1,
        FACING_EACH_OTHER   = 2,
        PERPENDICULAR       = 3,
        COLLINEAR_ALIGNMENT = 4
    };

    uint32_t target_buffer_id = 0;
    uint32_t column_id = 0;
    T target_direction{};       // Normalized direction vector (or fallback line direction B - A)
    T line_start{};             // Reference line endpoint A for COLLINEAR_ALIGNMENT
    T line_end{};               // Reference line endpoint B for COLLINEAR_ALIGNMENT
    float angle_threshold = 0.0f;     // Cosine of angle threshold
    float alignment_tolerance = 0.1f; // Collinear distance tolerance epsilon (meters)
    Comparison op = Comparison::WITHIN_ANGLE;

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary col_prop;
        col_prop["name"] = "column_id";
        col_prop["type"] = godot::Variant::INT;
        col_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(col_prop);

        godot::Dictionary op_prop;
        op_prop["name"] = "op";
        op_prop["type"] = godot::Variant::INT;
        op_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        op_prop["hint_string"] = "Within Angle:0,Outside Angle:1,Facing Each Other:2,Perpendicular:3,Collinear Alignment (3D Line Sensor):4";
        props.push_back(op_prop);

        godot::Dictionary dir_prop;
        dir_prop["name"] = "target_direction";
        dir_prop["type"] = "T"; 
        dir_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(dir_prop);

        godot::Dictionary start_prop;
        start_prop["name"] = "line_start";
        start_prop["type"] = "T";
        start_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(start_prop);

        godot::Dictionary end_prop;
        end_prop["name"] = "line_end";
        end_prop["type"] = "T";
        end_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(end_prop);

        godot::Dictionary angle_prop;
        angle_prop["name"] = "angle_threshold";
        angle_prop["type"] = godot::Variant::FLOAT;
        angle_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(angle_prop);

        godot::Dictionary tol_prop;
        tol_prop["name"] = "alignment_tolerance";
        tol_prop["type"] = godot::Variant::FLOAT;
        tol_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        tol_prop["hint_string"] = "0.0001,100.0,0.001,or_greater";
        props.push_back(tol_prop);

        return props;
    }
    
    [[nodiscard]] uint32_t get_target_buffer_id() const { return target_buffer_id; }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("column_id")) {
            column_id = static_cast<uint32_t>(p_props["column_id"]);
        }
        if (p_props.has("op")) {
            op = static_cast<Comparison>(static_cast<uint8_t>(p_props["op"]));
        }
        if (p_props.has("target_direction")) {
            target_direction = static_cast<T>(p_props["target_direction"]);
        }
        if (p_props.has("line_start")) {
            line_start = static_cast<T>(p_props["line_start"]);
        }
        if (p_props.has("line_end")) {
            line_end = static_cast<T>(p_props["line_end"]);
        }
        if (p_props.has("angle_threshold")) {
            angle_threshold = std::cos(static_cast<float>(p_props["angle_threshold"]));
        }
        if (p_props.has("alignment_tolerance")) {
            alignment_tolerance = std::max(0.0f, static_cast<float>(p_props["alignment_tolerance"]));
        }
    }
    
    template <QueryOp Op, typename T_View, typename T_Strategy>
    void execute(MemoryBufferSelectionPOD& r_selection, 
                 const TaskContextPOD& p_context, 
                 const T_View& p_view) const {
        
        if constexpr (Op == QueryOp::CULL) {
            if (r_selection.mode == SelectionMode::DENSE) _cull_dense(r_selection, p_view);
            else _cull_sparse(r_selection, p_view);
        } else if constexpr (Op == QueryOp::ADD) {
            _add_available(r_selection, p_view, p_context);
        }
    }

private:
    // --- The DOD View Adapter ---
    template <typename T_View>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline T _read_view(const T_View& p_view, int64_t idx) const {
        if constexpr (std::is_pointer_v<decltype(p_view[idx])>) {
            return *reinterpret_cast<const T*>(p_view[idx]);
        } else if constexpr (requires { static_cast<T>(p_view[idx]); }) {
            return static_cast<T>(p_view[idx]);
        } else {
            return T{}; 
        }
    }

    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline bool _evaluate_collinear(const T& p_point) const noexcept {
        // Compute reference line vector AB = line_end - line_start (fallback to target_direction if AB is degenerate)
        T ab = line_end - line_start;
        auto ab_len_sq = ab.dot(ab);
        if (ab_len_sq <= static_cast<decltype(ab_len_sq)>(1e-12)) {
            ab = target_direction;
            ab_len_sq = ab.dot(ab);
            if (ab_len_sq <= static_cast<decltype(ab_len_sq)>(1e-12)) {
                return false;
            }
        }

        const T ap = p_point - line_start;
        const auto eps = static_cast<decltype(ab_len_sq)>(alignment_tolerance);
        const auto max_cross_sq = (eps * eps) * ab_len_sq;

        if constexpr (requires { ap.cross(ab).length_squared(); }) {
            // 3D closed-form cross-product magnitude squared: ||(P_i - A) x (B - A)||^2 <= eps^2 * ||B - A||^2
            const auto cross_vec = ap.cross(ab);
            return cross_vec.dot(cross_vec) <= max_cross_sq;
        } else {
            // General N-dimensional Lagrange identity: ||AP||^2 ||AB||^2 - (AP . AB)^2 <= eps^2 * ||AB||^2
            const auto ap_len_sq = ap.dot(ap);
            const auto dot_ap_ab = ap.dot(ab);
            const auto ortho_num = std::max<decltype(ab_len_sq)>(0, ap_len_sq * ab_len_sq - dot_ap_ab * dot_ap_ab);
            return ortho_num <= max_cross_sq;
        }
    }

    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline bool _evaluate(const T& p_val) const {
        if (op == Comparison::COLLINEAR_ALIGNMENT) {
            return _evaluate_collinear(p_val);
        }

        float dot_product = static_cast<float>(p_val.dot(target_direction));
        
        switch(op) {
            case Comparison::WITHIN_ANGLE: return dot_product >= angle_threshold;
            case Comparison::OUTSIDE_ANGLE: return dot_product < angle_threshold;
            case Comparison::FACING_EACH_OTHER: return dot_product <= -angle_threshold;
            case Comparison::PERPENDICULAR: return std::abs(dot_product) <= angle_threshold;
            case Comparison::COLLINEAR_ALIGNMENT: return _evaluate_collinear(p_val);
        }
        return false;
    }

    template <typename T_View>
    void _cull_dense(MemoryBufferSelectionPOD& r_selection, const T_View& p_view) const {
        uint64_t* bitset = r_selection.data.bitset;
        for (int64_t i = 0; i < r_selection.capacity; ++i) {
            if (bitset[i >> 6] & (1ULL << (i & 63))) {
                if (!_evaluate(_read_view(p_view, i))) {
                    bitset[i >> 6] &= ~(1ULL << (i & 63));
                    r_selection.element_count--;
                }
            }
        }
    }

    template <typename T_View>
    void _cull_sparse(MemoryBufferSelectionPOD& r_selection, const T_View& p_view) const {
        int64_t write_ptr = 0;
        for (int64_t i = 0; i < r_selection.element_count; ++i) {
            if (_evaluate(_read_view(p_view, r_selection.data.indices[i]))) r_selection.data.indices[write_ptr++] = r_selection.data.indices[i];
        }
        r_selection.element_count = write_ptr;
    }

    template <typename T_View>
    void _add_available(const MemoryBufferSelectionPOD& r_selection, const T_View& p_view, const TaskContextPOD& p_ctx) const {
        const uint64_t* unclaimed = r_selection.unclaimed_mask;
        if (!unclaimed) return;

        const int64_t words = (r_selection.capacity + 63) >> 6;
        for (int64_t w = 0; w < words; ++w) {
            uint64_t mask = unclaimed[w];
            while (mask != 0) {
                int bit_index = std::countr_zero(mask);
                int64_t global_index = (w << 6) + bit_index;
                
                if (global_index >= r_selection.capacity) break;

                if (_evaluate(_read_view(p_view, global_index))) {
                    p_ctx.queue_selection_command(target_buffer_id, global_index);
                }
                mask &= (mask - 1); 
            }
        }
    }
};

} // namespace ideam::core

 // IDEAM_CORE_DIRECTIONAL_QUERY_LOGIC_H