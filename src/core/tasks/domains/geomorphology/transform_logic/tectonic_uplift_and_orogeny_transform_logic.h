#pragma once

/**
 * @file tectonic_uplift_and_orogeny_transform_logic.h
 * @brief TransformLogic task (GEO_T1: TectonicUpliftAndOrogeny) for IdeamGeomorphology.
 *
 * Evaluates plate-boundary convergent/divergent/transform stresses, updates
 * crust thickness, applies Airy isostatic compensation, and uplifts/subsides
 * the stratigraphic column bedrock elevation.
 */

#include "../../../../geomorphology/geomorphology_core.h"
#include "../geomorphology_dod_common.h"
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
#include <span>
#include <string_view>

namespace ideam::core {

template <
    typename T = uint32_t,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) TectonicUpliftAndOrogenyTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts =
        BufferLayoutType::FLAT | BufferLayoutType::AOS | BufferLayoutType::SOA;
    static constexpr DataType required_types =
        DataType::BYTE | DataType::INT32 | DataType::FLOAT32 | DataType::VECTOR3;

    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Tectonic Uplift & Orogeny";
    static constexpr std::string_view ui_category  = "Geomorphology";

    uint32_t target_buffer_id            = INVALID_ID;
    uint32_t seed_profile_buffer_id      = INVALID_ID;
    uint32_t plate_catalog_buffer_id     = INVALID_ID;
    uint32_t lithology_catalog_buffer_id = INVALID_ID;
    uint32_t crust_cell_buffer_id        = INVALID_ID;
    uint32_t strata_column_buffer_id     = INVALID_ID;
    float    dt_millions_of_years        = 0.025f;
    float    orogenic_uplift_multiplier  = 1.0f;
    uint8_t  reserved_padding[32]        = {0};

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_prop = [&](const char *p_name, godot::Variant::Type p_type, godot::Variant p_def) {
            godot::Dictionary d;
            d["name"]    = p_name;
            d["type"]    = static_cast<int>(p_type);
            d["default"] = p_def;
            props.push_back(d);
        };
        add_prop("target_buffer_id",            godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("seed_profile_buffer_id",      godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("plate_catalog_buffer_id",     godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("lithology_catalog_buffer_id", godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("crust_cell_buffer_id",        godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("strata_column_buffer_id",     godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("dt_millions_of_years",        godot::Variant::FLOAT, 0.025);
        add_prop("orogenic_uplift_multiplier",  godot::Variant::FLOAT, 1.0);
        return props;
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        return (target_buffer_id != INVALID_ID) ? target_buffer_id : strata_column_buffer_id;
    }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id"))            target_buffer_id            = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("seed_profile_buffer_id"))      seed_profile_buffer_id      = static_cast<uint32_t>(static_cast<int64_t>(p_props["seed_profile_buffer_id"]));
        if (p_props.has("plate_catalog_buffer_id"))     plate_catalog_buffer_id     = static_cast<uint32_t>(static_cast<int64_t>(p_props["plate_catalog_buffer_id"]));
        if (p_props.has("lithology_catalog_buffer_id")) lithology_catalog_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["lithology_catalog_buffer_id"]));
        if (p_props.has("crust_cell_buffer_id"))        crust_cell_buffer_id        = static_cast<uint32_t>(static_cast<int64_t>(p_props["crust_cell_buffer_id"]));
        if (p_props.has("strata_column_buffer_id"))     strata_column_buffer_id     = static_cast<uint32_t>(static_cast<int64_t>(p_props["strata_column_buffer_id"]));
        if (p_props.has("dt_millions_of_years"))        dt_millions_of_years        = static_cast<float>(static_cast<double>(p_props["dt_millions_of_years"]));
        if (p_props.has("orogenic_uplift_multiplier"))  orogenic_uplift_multiplier  = static_cast<float>(static_cast<double>(p_props["orogenic_uplift_multiplier"]));
    }

    template <typename T_ActiveView>
    inline void configure_view(T_ActiveView &p_view, const TaskContextPOD &p_ctx) const noexcept {
        (void)p_view;
        (void)p_ctx;
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    void execute(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        (void)p_view;
        const GrantPartPOD *seed_part   = p_ctx.get_grant_part(seed_profile_buffer_id);
        const GrantPartPOD *plate_part  = p_ctx.get_grant_part(plate_catalog_buffer_id);
        const GrantPartPOD *cat_part    = p_ctx.get_grant_part(lithology_catalog_buffer_id);
        const GrantPartPOD *crust_part  = p_ctx.get_grant_part(crust_cell_buffer_id);
        const GrantPartPOD *strata_part = p_ctx.get_grant_part(strata_column_buffer_id);

        if (!seed_part || !seed_part->raw_base_ptr || !crust_part || !crust_part->raw_base_ptr || !strata_part || !strata_part->raw_base_ptr) {
            return;
        }

        const auto *seed   = reinterpret_cast<const ideam::geomorphology::GeomorphologySeedProfilePOD *>(seed_part->raw_base_ptr);
        const auto *plates = (plate_part && plate_part->raw_base_ptr)
            ? reinterpret_cast<const ideam::geomorphology::TectonicPlatePOD *>(plate_part->raw_base_ptr)
            : nullptr;
        const auto *cat    = (cat_part && cat_part->raw_base_ptr)
            ? reinterpret_cast<const ideam::geomorphology::LithologyArchetypePOD *>(cat_part->raw_base_ptr)
            : nullptr;
        auto *crust        = reinterpret_cast<ideam::geomorphology::CrustalTectonicCellPOD *>(crust_part->raw_base_ptr);
        auto *strata       = reinterpret_cast<ideam::geomorphology::StratigraphicColumnPOD *>(strata_part->raw_base_ptr);

        const size_t n = static_cast<size_t>(seed->grid_width) * static_cast<size_t>(seed->grid_height);
        const size_t p_cnt = plates ? static_cast<size_t>(seed->plate_count) : 0u;
        const size_t c_cnt = cat ? (cat_part->capacity_bytes / sizeof(ideam::geomorphology::LithologyArchetypePOD)) : 0u;

        ideam::geomorphology::GeomorphologyKernelMath::step_tectonic_uplift_and_isostasy(
            *seed,
            std::span<const ideam::geomorphology::TectonicPlatePOD>(plates, p_cnt),
            std::span<const ideam::geomorphology::LithologyArchetypePOD>(cat, c_cnt),
            dt_millions_of_years * orogenic_uplift_multiplier,
            std::span<ideam::geomorphology::CrustalTectonicCellPOD>(crust, n),
            std::span<ideam::geomorphology::StratigraphicColumnPOD>(strata, n));
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    inline void execute_transform(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        execute<T_ActiveView, T_ActiveStrategy>(p_ctx, p_view);
    }
};

static_assert(sizeof(TectonicUpliftAndOrogenyTransformLogic<>) == 64, "TectonicUpliftAndOrogenyTransformLogic must be 64 bytes!");
static_assert(IsTransformLogic<TectonicUpliftAndOrogenyTransformLogic<>>, "TectonicUpliftAndOrogenyTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core
