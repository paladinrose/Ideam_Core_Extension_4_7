#pragma once

/**
 * @file biogeochemical_nutrient_and_succession_transform_logic.h
 * @brief TransformLogic task (ECO_T2: BiogeochemicalNutrientAndSuccession) for IdeamEcology.
 *
 * Solves Century-lite soil organic carbon humification, Q10 temperature/moisture
 * microbial decomposition, nitrogen fixation, and seral succession advancement.
 */

#include "../../../../ecology/ecology_core.h"
#include "../ecology_dod_common.h"
#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_grant_pod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <cstdint>
#include <span>
#include <string_view>

namespace ideam::core {

template <
    typename T = uint32_t,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) BiogeochemicalNutrientAndSuccessionTransformLogic {
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

    static constexpr std::string_view display_name = "Biogeochemical Nutrient & Seral Succession";
    static constexpr std::string_view ui_category  = "Ecology";

    uint32_t target_buffer_id          = INVALID_ID;
    uint32_t seed_profile_buffer_id    = INVALID_ID;
    uint32_t guild_catalog_buffer_id   = INVALID_ID;
    uint32_t ecological_cell_buffer_id = INVALID_ID;
    uint32_t biogeo_trophic_buffer_id  = INVALID_ID;
    float    dt_years                  = 0.25f;
    uint8_t  reserved_padding[40]      = {0};

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_prop = [&](const char *p_name, godot::Variant::Type p_type, godot::Variant p_def) {
            godot::Dictionary d;
            d["name"]    = p_name;
            d["type"]    = static_cast<int>(p_type);
            d["default"] = p_def;
            props.push_back(d);
        };
        add_prop("target_buffer_id",          godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("seed_profile_buffer_id",    godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("guild_catalog_buffer_id",   godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("ecological_cell_buffer_id", godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("biogeo_trophic_buffer_id",  godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("dt_years",                  godot::Variant::FLOAT, 0.25);
        return props;
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        return (target_buffer_id != INVALID_ID) ? target_buffer_id : biogeo_trophic_buffer_id;
    }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id"))          target_buffer_id          = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("seed_profile_buffer_id"))    seed_profile_buffer_id    = static_cast<uint32_t>(static_cast<int64_t>(p_props["seed_profile_buffer_id"]));
        if (p_props.has("guild_catalog_buffer_id"))   guild_catalog_buffer_id   = static_cast<uint32_t>(static_cast<int64_t>(p_props["guild_catalog_buffer_id"]));
        if (p_props.has("ecological_cell_buffer_id")) ecological_cell_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["ecological_cell_buffer_id"]));
        if (p_props.has("biogeo_trophic_buffer_id"))  biogeo_trophic_buffer_id  = static_cast<uint32_t>(static_cast<int64_t>(p_props["biogeo_trophic_buffer_id"]));
        if (p_props.has("dt_years"))                  dt_years                  = static_cast<float>(static_cast<double>(p_props["dt_years"]));
    }

    template <typename T_ActiveView>
    inline void configure_view(T_ActiveView &p_view, const TaskContextPOD &p_ctx) const noexcept {
        (void)p_view;
        (void)p_ctx;
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    void execute(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        (void)p_view;
        const GrantPartPOD *seed_part = p_ctx.get_grant_part(seed_profile_buffer_id);
        const GrantPartPOD *cat_part  = p_ctx.get_grant_part(guild_catalog_buffer_id);
        const GrantPartPOD *eco_part  = p_ctx.get_grant_part(ecological_cell_buffer_id);
        const GrantPartPOD *bio_part  = p_ctx.get_grant_part(biogeo_trophic_buffer_id);
        if (!seed_part || !seed_part->raw_base_ptr || !eco_part || !eco_part->raw_base_ptr || !bio_part || !bio_part->raw_base_ptr) {
            return;
        }
        const auto *seed = reinterpret_cast<const ideam::ecology::EcologySeedProfilePOD *>(seed_part->raw_base_ptr);
        const auto *cat  = (cat_part && cat_part->raw_base_ptr)
            ? reinterpret_cast<const ideam::ecology::FloraGuildArchetypePOD *>(cat_part->raw_base_ptr)
            : nullptr;
        auto *eco        = reinterpret_cast<ideam::ecology::EcologicalCellStatePOD *>(eco_part->raw_base_ptr);
        auto *bio        = reinterpret_cast<ideam::ecology::BiogeochemicalAndTrophicCellPOD *>(bio_part->raw_base_ptr);
        const size_t n   = static_cast<size_t>(seed->grid_width) * static_cast<size_t>(seed->grid_height);
        const size_t c_cnt = cat ? (cat_part->capacity_bytes / sizeof(ideam::ecology::FloraGuildArchetypePOD)) : 0u;
        ideam::ecology::EcologyKernelMath::step_biogeochemical_npk_and_succession(
            *seed,
            std::span<const ideam::ecology::FloraGuildArchetypePOD>(cat, c_cnt),
            dt_years,
            std::span<ideam::ecology::EcologicalCellStatePOD>(eco, n),
            std::span<ideam::ecology::BiogeochemicalAndTrophicCellPOD>(bio, n));
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    inline void execute_transform(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        execute<T_ActiveView, T_ActiveStrategy>(p_ctx, p_view);
    }
};

static_assert(sizeof(BiogeochemicalNutrientAndSuccessionTransformLogic<>) == 64, "BiogeochemicalNutrientAndSuccessionTransformLogic must be 64 bytes!");
static_assert(IsTransformLogic<BiogeochemicalNutrientAndSuccessionTransformLogic<>>, "BiogeochemicalNutrientAndSuccessionTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core
