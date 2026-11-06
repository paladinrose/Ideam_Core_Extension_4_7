#pragma once

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/memory_buffer_selection_pod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"

#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

#ifndef IDEAM_GAMES_CONTROL_MAP_INPUT_FILTER_ENUMS_DEFINED
#define IDEAM_GAMES_CONTROL_MAP_INPUT_FILTER_ENUMS_DEFINED

/**
 * DOD DeadzoneCurveShape (mirrors ideam::godot_ext::DeadzoneCurveShape in gameplay_control_map.h).
 */
enum ControlMapDeadzoneCurve : uint8_t {
    INPUT_DEADZONE_RADIAL_SCALED     = 0, // Radial inner/outer deadzone rescaled smoothly to [0..1]
    INPUT_DEADZONE_AXIAL_INDEPENDENT = 1, // Independent X/Y axial deadzones (tank controls, throttles, 2D)
    INPUT_DEADZONE_CUBIC_PRECISION   = 2  // Radial deadzone blended with 65% cubic (x^3) + 35% linear curve
};

/**
 * DOD MovementAlignmentMode (mirrors ideam::godot_ext::MovementAlignmentMode in gameplay_perspective.h).
 */
enum ControlMapMovementAlignment : uint8_t {
    INPUT_ALIGN_CAMERA_RELATIVE    = 0, // Directional input projected onto camera/head horizontal basis
    INPUT_ALIGN_CHARACTER_RELATIVE = 1, // Directional input projected onto agent/character horizontal basis
    INPUT_ALIGN_WORLD_AXIS_ALIGNED = 2, // Fixed world/screen-axis projection: (x, 0, -y)
    INPUT_ALIGN_CURSOR_DIRECTED    = 3  // Twin-stick / cursor-directed horizontal projection
};

/**
 * Compact 48-byte POD layout matching godot::Transform3D (3x Vector3 basis + 1x Vector3 origin)
 * for zero-overhead basis extraction in DOD buffers without pulling in heavy scene headers.
 */
struct InputTransform3DPOD {
    godot::Vector3 basis_row0{1.0f, 0.0f, 0.0f};
    godot::Vector3 basis_row1{0.0f, 1.0f, 0.0f};
    godot::Vector3 basis_row2{0.0f, 0.0f, 1.0f};
    godot::Vector3 origin{0.0f, 0.0f, 0.0f};
};

static_assert(sizeof(InputTransform3DPOD) == 48, "InputTransform3DPOD must be 48 bytes!");

#endif // IDEAM_GAMES_CONTROL_MAP_INPUT_FILTER_ENUMS_DEFINED

/**
 * ControlMapInputFilterTransformLogic<T> (G10)
 * Applies DeadzoneCurveShape (RADIAL_SCALED, AXIAL_INDEPENDENT, CUBIC_PRECISION), custom USB/Arduino
 * ADC normalization, response exponent curves, sensitivity/inversion, and perspective-relative 3D
 * movement projection across Vector2 stick/look streams or float trigger/ADC channel streams.
 * Replaces GameplayControlMap::apply_stick_deadzone(), apply_axis_deadzone(),
 * _normalize_custom_adc_value(), and get_world_move_direction().
 */
template <typename T = godot::Vector2>
struct alignas(64) ControlMapInputFilterTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::VECTOR2 | DataType::FLOAT32;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Control Map Input Filter";
    static constexpr std::string_view ui_category  = "Input";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_input_buffer_id     = INVALID_ID;                   // 4B: Primary Vector2 or float filtered input buffer
    uint32_t raw_input_buffer_id        = INVALID_ID;                   // 4B: Optional source raw input buffer (in-place if INVALID_ID)
    uint32_t world_move_dir_buffer_id   = INVALID_ID;                   // 4B: Optional output Vector3 world movement direction buffer
    uint32_t camera_transform_buffer_id = INVALID_ID;                   // 4B: Optional Transform3D (48B) camera/HMD transform buffer
    uint32_t agent_transform_buffer_id  = INVALID_ID;                   // 4B: Optional Transform3D (48B) agent transform buffer

    float inner_deadzone                = 0.15f;                        // 4B: Inner deadzone threshold [0.0 .. 0.95]
    float outer_deadzone                = 0.95f;                        // 4B: Outer deadzone saturation [inner + 0.01 .. 1.0]
    float response_exponent             = 1.0f;                         // 4B: Power response curve exponent [0.25 .. 5.0]
    float sensitivity_x                 = 1.0f;                         // 4B: Horizontal / primary axis sensitivity multiplier
    float sensitivity_y                 = 1.0f;                         // 4B: Vertical / secondary axis sensitivity multiplier
    float reference_yaw_radians         = 0.0f;                         // 4B: Fallback yaw angle when Transform3D buffers are unbound

    uint16_t adc_resolution_max         = 1023;                         // 2B: Max ADC integer count (1023 for 10-bit Arduino, 4095 for 12-bit)
    uint16_t adc_center_value           = 512;                          // 2B: Neutral center ADC count for bipolar channels

    uint8_t deadzone_curve_shape        = INPUT_DEADZONE_RADIAL_SCALED; // 1B: ControlMapDeadzoneCurve
    uint8_t movement_alignment_mode     = INPUT_ALIGN_CAMERA_RELATIVE;  // 1B: ControlMapMovementAlignment
    bool normalize_raw_adc              = false;                        // 1B: Normalize raw ADC counts [0..adc_resolution_max] first
    bool adc_bipolar                    = true;                         // 1B: True = [-1..1] around adc_center_value; False = [0..1]
    bool invert_x                       = false;                        // 1B: Invert X / primary axis
    bool invert_y                       = false;                        // 1B: Invert Y / secondary axis
    bool clamp_unit_length              = true;                         // 1B: Limit output Vector2/Vector3 magnitude to 1.0
    uint8_t reserved_padding[9]         = {0};                          // 9B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary tgt_prop;
        tgt_prop["name"] = "target_input_buffer_id";
        tgt_prop["type"] = godot::Variant::INT;
        tgt_prop["hint_string"] = "buffer_option";
        props.push_back(tgt_prop);

        godot::Dictionary raw_prop;
        raw_prop["name"] = "raw_input_buffer_id";
        raw_prop["type"] = godot::Variant::INT;
        raw_prop["hint_string"] = "buffer_option";
        props.push_back(raw_prop);

        godot::Dictionary wdir_prop;
        wdir_prop["name"] = "world_move_dir_buffer_id";
        wdir_prop["type"] = godot::Variant::INT;
        wdir_prop["hint_string"] = "buffer_option";
        props.push_back(wdir_prop);

        godot::Dictionary cam_prop;
        cam_prop["name"] = "camera_transform_buffer_id";
        cam_prop["type"] = godot::Variant::INT;
        cam_prop["hint_string"] = "buffer_option";
        props.push_back(cam_prop);

        godot::Dictionary agt_prop;
        agt_prop["name"] = "agent_transform_buffer_id";
        agt_prop["type"] = godot::Variant::INT;
        agt_prop["hint_string"] = "buffer_option";
        props.push_back(agt_prop);

        godot::Dictionary crv_prop;
        crv_prop["name"] = "deadzone_curve_shape";
        crv_prop["type"] = godot::Variant::INT;
        crv_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        crv_prop["hint_string"] = "Radial Scaled:0,Axial Independent:1,Cubic Precision:2";
        props.push_back(crv_prop);

        godot::Dictionary alg_prop;
        alg_prop["name"] = "movement_alignment_mode";
        alg_prop["type"] = godot::Variant::INT;
        alg_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        alg_prop["hint_string"] = "Camera Relative:0,Character Relative:1,World Axis Aligned:2,Cursor Directed:3";
        props.push_back(alg_prop);

        godot::Dictionary idz_prop;
        idz_prop["name"] = "inner_deadzone";
        idz_prop["type"] = godot::Variant::FLOAT;
        idz_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        idz_prop["hint_string"] = "0.0,0.8,0.01";
        props.push_back(idz_prop);

        godot::Dictionary odz_prop;
        odz_prop["name"] = "outer_deadzone";
        odz_prop["type"] = godot::Variant::FLOAT;
        odz_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        odz_prop["hint_string"] = "0.2,1.0,0.01";
        props.push_back(odz_prop);

        godot::Dictionary exp_prop;
        exp_prop["name"] = "response_exponent";
        exp_prop["type"] = godot::Variant::FLOAT;
        exp_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        exp_prop["hint_string"] = "0.25,5.0,0.05";
        props.push_back(exp_prop);

        godot::Dictionary sx_prop;
        sx_prop["name"] = "sensitivity_x";
        sx_prop["type"] = godot::Variant::FLOAT;
        sx_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        sx_prop["hint_string"] = "0.01,10.0,0.05";
        props.push_back(sx_prop);

        godot::Dictionary sy_prop;
        sy_prop["name"] = "sensitivity_y";
        sy_prop["type"] = godot::Variant::FLOAT;
        sy_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        sy_prop["hint_string"] = "0.01,10.0,0.05";
        props.push_back(sy_prop);

        godot::Dictionary yaw_prop;
        yaw_prop["name"] = "reference_yaw_radians";
        yaw_prop["type"] = godot::Variant::FLOAT;
        yaw_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        yaw_prop["hint_string"] = "-6.283185,6.283185,0.01";
        props.push_back(yaw_prop);

        godot::Dictionary amax_prop;
        amax_prop["name"] = "adc_resolution_max";
        amax_prop["type"] = godot::Variant::INT;
        amax_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        amax_prop["hint_string"] = "1,65535,1";
        props.push_back(amax_prop);

        godot::Dictionary acen_prop;
        acen_prop["name"] = "adc_center_value";
        acen_prop["type"] = godot::Variant::INT;
        acen_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        acen_prop["hint_string"] = "0,65535,1";
        props.push_back(acen_prop);

        godot::Dictionary nadc_prop;
        nadc_prop["name"] = "normalize_raw_adc";
        nadc_prop["type"] = godot::Variant::BOOL;
        nadc_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(nadc_prop);

        godot::Dictionary bip_prop;
        bip_prop["name"] = "adc_bipolar";
        bip_prop["type"] = godot::Variant::BOOL;
        bip_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(bip_prop);

        godot::Dictionary ivx_prop;
        ivx_prop["name"] = "invert_x";
        ivx_prop["type"] = godot::Variant::BOOL;
        ivx_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(ivx_prop);

        godot::Dictionary ivy_prop;
        ivy_prop["name"] = "invert_y";
        ivy_prop["type"] = godot::Variant::BOOL;
        ivy_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(ivy_prop);

        godot::Dictionary clp_prop;
        clp_prop["name"] = "clamp_unit_length";
        clp_prop["type"] = godot::Variant::BOOL;
        clp_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(clp_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        if (target_input_buffer_id != INVALID_ID) return target_input_buffer_id;
        return raw_input_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_input_buffer_id")) {
            target_input_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_input_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && target_input_buffer_id == INVALID_ID) {
            target_input_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("raw_input_buffer_id")) {
            raw_input_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["raw_input_buffer_id"]));
        }
        if (p_props.has("world_move_dir_buffer_id")) {
            world_move_dir_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["world_move_dir_buffer_id"]));
        }
        if (p_props.has("camera_transform_buffer_id")) {
            camera_transform_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["camera_transform_buffer_id"]));
        }
        if (p_props.has("agent_transform_buffer_id")) {
            agent_transform_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["agent_transform_buffer_id"]));
        }
        if (p_props.has("inner_deadzone")) {
            inner_deadzone = static_cast<float>(p_props["inner_deadzone"]);
        }
        if (p_props.has("outer_deadzone")) {
            outer_deadzone = static_cast<float>(p_props["outer_deadzone"]);
        }
        if (p_props.has("response_exponent")) {
            response_exponent = static_cast<float>(p_props["response_exponent"]);
        }
        if (p_props.has("sensitivity_x")) {
            sensitivity_x = static_cast<float>(p_props["sensitivity_x"]);
        }
        if (p_props.has("sensitivity_y")) {
            sensitivity_y = static_cast<float>(p_props["sensitivity_y"]);
        }
        if (p_props.has("reference_yaw_radians")) {
            reference_yaw_radians = static_cast<float>(p_props["reference_yaw_radians"]);
        }
        if (p_props.has("adc_resolution_max")) {
            adc_resolution_max = static_cast<uint16_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["adc_resolution_max"]), 1, 65535));
        }
        if (p_props.has("adc_center_value")) {
            adc_center_value = static_cast<uint16_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["adc_center_value"]), 0, 65535));
        }
        if (p_props.has("deadzone_curve_shape")) {
            deadzone_curve_shape = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["deadzone_curve_shape"]), 0, 2));
        }
        if (p_props.has("movement_alignment_mode")) {
            movement_alignment_mode = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["movement_alignment_mode"]), 0, 3));
        }
        if (p_props.has("normalize_raw_adc")) {
            normalize_raw_adc = static_cast<bool>(p_props["normalize_raw_adc"]);
        }
        if (p_props.has("adc_bipolar")) {
            adc_bipolar = static_cast<bool>(p_props["adc_bipolar"]);
        }
        if (p_props.has("invert_x")) {
            invert_x = static_cast<bool>(p_props["invert_x"]);
        }
        if (p_props.has("invert_y")) {
            invert_y = static_cast<bool>(p_props["invert_y"]);
        }
        if (p_props.has("clamp_unit_length")) {
            clamp_unit_length = static_cast<bool>(p_props["clamp_unit_length"]);
        }
    }

    template <typename T_View, typename T_Strategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_context, T_View& p_view) const {
        (void)p_view;
        if constexpr (std::is_same_v<T, godot::Vector2> || std::is_same_v<T, float>) {
            const uint32_t dst_id = get_target_buffer_id();
            const GrantPartPOD* dst_part = (dst_id != INVALID_ID) ? p_context.get_grant_part(dst_id) : nullptr;
            if (!dst_part || !dst_part->raw_base_ptr) return;

            const GrantPartPOD* raw_part  = (raw_input_buffer_id != INVALID_ID && raw_input_buffer_id != dst_id)
                ? p_context.get_grant_part(raw_input_buffer_id)
                : nullptr;
            const GrantPartPOD* wdir_part = (world_move_dir_buffer_id != INVALID_ID)   ? p_context.get_grant_part(world_move_dir_buffer_id)   : nullptr;
            const GrantPartPOD* cam_part  = (camera_transform_buffer_id != INVALID_ID) ? p_context.get_grant_part(camera_transform_buffer_id) : nullptr;
            const GrantPartPOD* agt_part  = (agent_transform_buffer_id != INVALID_ID)  ? p_context.get_grant_part(agent_transform_buffer_id)  : nullptr;

            T* __restrict dst_inputs       = reinterpret_cast<T*>(dst_part->raw_base_ptr);
            const T* __restrict raw_inputs = (raw_part && raw_part->raw_base_ptr)
                ? reinterpret_cast<const T*>(raw_part->raw_base_ptr)
                : dst_inputs;

            godot::Vector3* __restrict world_dirs = (wdir_part && wdir_part->raw_base_ptr)
                ? reinterpret_cast<godot::Vector3*>(wdir_part->raw_base_ptr)
                : nullptr;
            const InputTransform3DPOD* __restrict cam_xforms = (cam_part && cam_part->raw_base_ptr)
                ? reinterpret_cast<const InputTransform3DPOD*>(cam_part->raw_base_ptr)
                : nullptr;
            const InputTransform3DPOD* __restrict agt_xforms = (agt_part && agt_part->raw_base_ptr)
                ? reinterpret_cast<const InputTransform3DPOD*>(agt_part->raw_base_ptr)
                : nullptr;

            const int64_t wdir_cap = world_dirs ? static_cast<int64_t>(wdir_part->capacity_bytes / sizeof(godot::Vector3))      : 0;
            const int64_t cam_cap  = cam_xforms ? static_cast<int64_t>(cam_part->capacity_bytes / sizeof(InputTransform3DPOD)) : 0;
            const int64_t agt_cap  = agt_xforms ? static_cast<int64_t>(agt_part->capacity_bytes / sizeof(InputTransform3DPOD)) : 0;

            int64_t elem_count = static_cast<int64_t>(dst_part->capacity_bytes / sizeof(T));
            if (raw_part) elem_count = std::min<int64_t>(elem_count, static_cast<int64_t>(raw_part->capacity_bytes / sizeof(T)));
            if (elem_count <= 0) return;

            const float inner   = std::clamp(inner_deadzone, 0.0f, 0.95f);
            const float outer   = std::clamp(outer_deadzone, inner + 0.01f, 1.0f);
            const float exp_val = std::clamp(response_exponent, 0.25f, 5.0f);
            const float sx      = sensitivity_x * (invert_x ? -1.0f : 1.0f);
            const float sy      = sensitivity_y * (invert_y ? -1.0f : 1.0f);

            auto eval_input = [&](int64_t idx) noexcept {
                if constexpr (std::is_same_v<T, float>) {
                    float val = raw_inputs[idx];
                    if (normalize_raw_adc) {
                        val = _normalize_adc(val);
                    }
                    float filtered = _apply_axis_deadzone(val, inner, outer);
                    if (std::abs(exp_val - 1.0f) > 0.0001f && filtered != 0.0f) {
                        filtered = (filtered < 0.0f ? -1.0f : 1.0f) * std::pow(std::abs(filtered), exp_val);
                    }
                    filtered *= sx;
                    if (clamp_unit_length) {
                        filtered = std::clamp(filtered, -1.0f, 1.0f);
                    }
                    dst_inputs[idx] = filtered;
                } else {
                    godot::Vector2 raw = raw_inputs[idx];
                    if (normalize_raw_adc) {
                        raw.x = _normalize_adc(raw.x);
                        raw.y = _normalize_adc(raw.y);
                    }

                    godot::Vector2 filtered = _apply_stick_deadzone(raw, inner, outer, deadzone_curve_shape, exp_val);
                    filtered.x *= sx;
                    filtered.y *= sy;

                    if (clamp_unit_length) {
                        const float len_sq = filtered.x * filtered.x + filtered.y * filtered.y;
                        if (len_sq > 1.0f) {
                            const float inv_len = 1.0f / std::sqrt(len_sq);
                            filtered.x *= inv_len;
                            filtered.y *= inv_len;
                        }
                    }
                    dst_inputs[idx] = filtered;

                    if (world_dirs && idx < wdir_cap) {
                        const InputTransform3DPOD* cam_xf = (cam_xforms && cam_cap > 0)
                            ? &cam_xforms[std::min<int64_t>(idx, cam_cap - 1)]
                            : nullptr;
                        const InputTransform3DPOD* agt_xf = (agt_xforms && agt_cap > 0)
                            ? &agt_xforms[std::min<int64_t>(idx, agt_cap - 1)]
                            : nullptr;
                        world_dirs[idx] = _project_world_move_direction(filtered, cam_xf, agt_xf);
                    }
                }
            };

            _iterate_selection(dst_part->selection, elem_count, eval_input);
        }
    }

private:
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    [[nodiscard]] inline float _normalize_adc(float p_raw) const noexcept {
        const float max_v    = static_cast<float>(std::max<uint16_t>(1u, adc_resolution_max));
        const float center_v = static_cast<float>(adc_center_value);
        if (!adc_bipolar) {
            return std::clamp(p_raw / max_v, 0.0f, 1.0f);
        }
        if (p_raw >= center_v) {
            const float pos_span = std::max(0.0001f, max_v - center_v);
            return std::clamp((p_raw - center_v) / pos_span, 0.0f, 1.0f);
        }
        const float neg_span = std::max(0.0001f, center_v);
        return -std::clamp((center_v - p_raw) / neg_span, 0.0f, 1.0f);
    }

    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    [[nodiscard]] static inline float _apply_axis_deadzone(float p_raw, float p_inner, float p_outer) noexcept {
        const float abs_val = std::abs(p_raw);
        if (abs_val <= p_inner) return 0.0f;
        const float norm = std::clamp((abs_val - p_inner) / (p_outer - p_inner), 0.0f, 1.0f);
        return (p_raw < 0.0f) ? -norm : norm;
    }

    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    [[nodiscard]] static inline godot::Vector2 _apply_stick_deadzone(
        const godot::Vector2& p_raw,
        float p_inner,
        float p_outer,
        uint8_t p_curve,
        float p_exp
    ) noexcept {
        if (p_curve == INPUT_DEADZONE_AXIAL_INDEPENDENT) {
            float x = _apply_axis_deadzone(p_raw.x, p_inner, p_outer);
            float y = _apply_axis_deadzone(p_raw.y, p_inner, p_outer);
            if (std::abs(p_exp - 1.0f) > 0.0001f) {
                if (x != 0.0f) x = (x < 0.0f ? -1.0f : 1.0f) * std::pow(std::abs(x), p_exp);
                if (y != 0.0f) y = (y < 0.0f ? -1.0f : 1.0f) * std::pow(std::abs(y), p_exp);
            }
            return godot::Vector2{x, y};
        }

        const float mag_sq = p_raw.x * p_raw.x + p_raw.y * p_raw.y;
        const float mag    = std::sqrt(mag_sq);
        if (mag <= p_inner || mag <= 0.00001f) {
            return godot::Vector2{0.0f, 0.0f};
        }

        float scaled_mag = std::clamp((mag - p_inner) / (p_outer - p_inner), 0.0f, 1.0f);
        if (p_curve == INPUT_DEADZONE_CUBIC_PRECISION) {
            // Mirrors GameplayControlMap::apply_stick_deadzone: 65% cubic + 35% linear
            scaled_mag = 0.65f * (scaled_mag * scaled_mag * scaled_mag) + 0.35f * scaled_mag;
        }
        if (std::abs(p_exp - 1.0f) > 0.0001f) {
            scaled_mag = std::pow(scaled_mag, p_exp);
        }

        const float scale = scaled_mag / mag;
        return godot::Vector2{p_raw.x * scale, p_raw.y * scale};
    }

    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    [[nodiscard]] inline godot::Vector3 _project_world_move_direction(
        const godot::Vector2& p_move,
        const InputTransform3DPOD* p_cam_xf,
        const InputTransform3DPOD* p_agt_xf
    ) const noexcept {
        const float move_len_sq = p_move.x * p_move.x + p_move.y * p_move.y;
        if (move_len_sq <= 0.00001f) {
            return godot::Vector3{0.0f, 0.0f, 0.0f};
        }

        if (movement_alignment_mode == INPUT_ALIGN_WORLD_AXIS_ALIGNED) {
            godot::Vector3 out{p_move.x, 0.0f, -p_move.y};
            if (clamp_unit_length && move_len_sq > 1.0f) {
                const float inv = 1.0f / std::sqrt(move_len_sq);
                out.x *= inv;
                out.z *= inv;
            }
            return out;
        }

        const InputTransform3DPOD* basis_xf =
            (movement_alignment_mode == INPUT_ALIGN_CHARACTER_RELATIVE)
                ? (p_agt_xf ? p_agt_xf : p_cam_xf)
                : (p_cam_xf ? p_cam_xf : p_agt_xf);

        float right_x = 1.0f, right_z = 0.0f;
        float fwd_x   = 0.0f, fwd_z   = -1.0f;

        if (basis_xf) {
            // In Godot Transform3D, column 0 is (row0.x, row1.x, row2.x) and column 2 is (row0.z, row1.z, row2.z)
            right_x = basis_xf->basis_row0.x;
            right_z = basis_xf->basis_row2.x;
            fwd_x   = -basis_xf->basis_row0.z;
            fwd_z   = -basis_xf->basis_row2.z;

            const float fwd_len_sq = fwd_x * fwd_x + fwd_z * fwd_z;
            if (fwd_len_sq < 0.0001f) {
                // Straight top-down (-90 deg pitch) camera fallback: use column 1 (up vector)
                fwd_x = basis_xf->basis_row0.y;
                fwd_z = basis_xf->basis_row2.y;
            }
        } else if (std::abs(reference_yaw_radians) > 0.00001f) {
            const float c = std::cos(reference_yaw_radians);
            const float s = std::sin(reference_yaw_radians);
            right_x = c;
            right_z = -s;
            fwd_x   = -s;
            fwd_z   = -c;
        }

        const float r_len = std::sqrt(right_x * right_x + right_z * right_z);
        if (r_len > 0.0001f) {
            right_x /= r_len;
            right_z /= r_len;
        }
        const float f_len = std::sqrt(fwd_x * fwd_x + fwd_z * fwd_z);
        if (f_len > 0.0001f) {
            fwd_x /= f_len;
            fwd_z /= f_len;
        }

        godot::Vector3 world{
            right_x * p_move.x + fwd_x * p_move.y,
            0.0f,
            right_z * p_move.x + fwd_z * p_move.y
        };
        if (clamp_unit_length) {
            const float w_sq = world.x * world.x + world.z * world.z;
            if (w_sq > 1.0f) {
                const float inv = 1.0f / std::sqrt(w_sq);
                world.x *= inv;
                world.z *= inv;
            }
        }
        return world;
    }

    template <typename Fn>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    static inline void _iterate_selection(const MemoryBufferSelectionPOD& p_sel, int64_t p_cap, Fn&& p_fn) noexcept {
        if (p_sel.mode == SelectionMode::DENSE && p_sel.data.bitset) {
            const uint64_t* bitset = p_sel.data.bitset;
            const int64_t cap = std::min<int64_t>(p_sel.capacity, p_cap);
            for (int64_t i = 0; i < cap; ++i) {
                if (bitset[i >> 6] & (1ULL << (i & 63))) {
                    p_fn(i);
                }
            }
        } else if (p_sel.mode == SelectionMode::SPARSE && p_sel.data.indices) {
            for (int64_t i = 0; i < p_sel.element_count; ++i) {
                const int64_t idx = p_sel.data.indices[i];
                if (idx >= 0 && idx < p_cap) {
                    p_fn(idx);
                }
            }
        } else {
            const int64_t start = std::max<int64_t>(0, p_sel.start_index);
            const int64_t count = (p_sel.element_count > 0) ? p_sel.element_count : p_cap;
            const int64_t end   = std::min<int64_t>(start + count, p_cap);
            for (int64_t i = start; i < end; ++i) {
                p_fn(i);
            }
        }
    }
};

static_assert(sizeof(ControlMapInputFilterTransformLogic<>) == 64, "ControlMapInputFilterTransformLogic must be exactly 64 bytes!");

} // namespace ideam::core