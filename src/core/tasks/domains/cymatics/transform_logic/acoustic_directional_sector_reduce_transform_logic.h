#pragma once

/**
 * acoustic_directional_sector_reduce_transform_logic.h
 * 64-byte cache-aligned TransformLogic task struct (`CYM5`) for `IdeamCymatics`.
 * Integrates acoustic wave energy around a listener into K polar sectors (`CymaticSectorPOD`)
 * for `CymaticAudioCompass`, `MindDOD`, and `CommunicationsDOD`.
 */

#include "../../../../cymatics/cymatics_solver_core.h"
#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_grant_pod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include <algorithm>
#include <cstdint>
#include <string_view>

namespace ideam::core {

template <
    typename T = float,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) AcousticDirectionalSectorReduceTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts =
        BufferLayoutType::FLAT | BufferLayoutType::AOS | BufferLayoutType::SOA;
    static constexpr DataType required_types = DataType::FLOAT32 | DataType::CUSTOM;

    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "CYM5 Acoustic Directional Sector Reduce";
    static constexpr std::string_view ui_category  = "Cymatics";

    uint32_t target_buffer_id    = INVALID_ID; // Primary acoustic field grid buffer
    uint32_t sectors_buffer_id   = INVALID_ID;
    uint32_t grid_width          = 32;
    uint32_t grid_height         = 32;
    uint32_t sector_count        = 16;
    float listener_grid_x        = 16.0f;
    float listener_grid_y        = 16.0f;
    float reference_yaw_rad      = 0.0f;
    uint8_t reserved_padding[32] = {0};

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_buf = [&](const char *n) {
            godot::Dictionary dict;
            dict["name"]        = n;
            dict["type"]        = static_cast<int>(godot::Variant::INT);
            dict["hint"]        = static_cast<int>(godot::PROPERTY_HINT_NONE);
            dict["hint_string"] = "buffer_option";
            dict["default"]     = static_cast<int64_t>(INVALID_ID);
            props.push_back(dict);
        };
        auto add = [&](const char *n, godot::Variant::Type t, godot::Variant d) {
            godot::Dictionary dict;
            dict["name"]    = n;
            dict["type"]    = static_cast<int>(t);
            dict["default"] = d;
            props.push_back(dict);
        };
        add_buf("target_buffer_id");
        add_buf("sectors_buffer_id");
        add("grid_width",        godot::Variant::INT,   static_cast<int64_t>(32));
        add("grid_height",       godot::Variant::INT,   static_cast<int64_t>(32));
        add("sector_count",      godot::Variant::INT,   static_cast<int64_t>(16));
        add("listener_grid_x",   godot::Variant::FLOAT, 16.0);
        add("listener_grid_y",   godot::Variant::FLOAT, 16.0);
        add("reference_yaw_rad", godot::Variant::FLOAT, 0.0);
        return props;
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        return (target_buffer_id != INVALID_ID) ? target_buffer_id : sectors_buffer_id;
    }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id")) {
            target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        } else if (p_props.has("field_buffer_id") && target_buffer_id == INVALID_ID) {
            target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["field_buffer_id"]));
        }
        if (p_props.has("sectors_buffer_id")) sectors_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["sectors_buffer_id"]));
        if (p_props.has("grid_width"))        grid_width        = static_cast<uint32_t>(static_cast<int64_t>(p_props["grid_width"]));
        if (p_props.has("grid_height"))       grid_height       = static_cast<uint32_t>(static_cast<int64_t>(p_props["grid_height"]));
        if (p_props.has("sector_count"))      sector_count      = static_cast<uint32_t>(static_cast<int64_t>(p_props["sector_count"]));
        if (p_props.has("listener_grid_x"))   listener_grid_x   = static_cast<float>(static_cast<double>(p_props["listener_grid_x"]));
        if (p_props.has("listener_grid_y"))   listener_grid_y   = static_cast<float>(static_cast<double>(p_props["listener_grid_y"]));
        if (p_props.has("reference_yaw_rad")) reference_yaw_rad = static_cast<float>(static_cast<double>(p_props["reference_yaw_rad"]));
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    void execute(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        (void)p_view;
        const GrantPartPOD *fp = p_ctx.get_grant_part(target_buffer_id);
        const GrantPartPOD *sp = p_ctx.get_grant_part(sectors_buffer_id);
        if (!fp || !sp || !fp->raw_base_ptr || !sp->raw_base_ptr) {
            return;
        }
        if (fp->capacity_bytes / sizeof(float) < static_cast<size_t>(grid_width) * grid_height) {
            return;
        }
        const uint32_t cap_sectors = static_cast<uint32_t>(sp->capacity_bytes / sizeof(CymaticSectorPOD));
        const uint32_t k           = std::min(sector_count, cap_sectors);
        if (k == 0u) {
            return;
        }

        CymaticGridConfigPOD cfg;
        cfg.width  = grid_width;
        cfg.height = grid_height;

        CymaticsSolverCore::reduce_directional_sectors(
            reinterpret_cast<const float *>(fp->raw_base_ptr),
            cfg,
            listener_grid_x,
            listener_grid_y,
            reference_yaw_rad,
            reinterpret_cast<CymaticSectorPOD *>(sp->raw_base_ptr),
            k
        );
    }
};

static_assert(sizeof(AcousticDirectionalSectorReduceTransformLogic<>) == 64, "CYM5 must be 64 bytes!");
static_assert(IsTransformLogic<AcousticDirectionalSectorReduceTransformLogic<>>, "CYM5 must satisfy IsTransformLogic!");

} // namespace ideam::core
