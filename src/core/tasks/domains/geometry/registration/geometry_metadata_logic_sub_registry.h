#pragma once

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/packed_int64_array.hpp>

#include "geometry_metadata_task_registry.h"
#include "../../../registration/task_view_bridge.h"
#include "../../../metadata_task.h"
#include "../../../metadata_logic/metadata_logic_traits.h"
#include "../../../../memory/views/view_traits.h"

// --- Geometry Metadata Logics ---
#include "../metadata_logic/spatial_hash_partition_metadata_logic.h"

// --- Views & Strategies ---
#include "../../../../memory/views/aosoa_view.h"
#include "../../../../memory/views/atomic_view.h"
#include "../../../../memory/views/bridge_view.h"
#include "../../../../memory/views/multi_element_view.h"
#include "../../../../memory/views/paged_view.h"
#include "../../../../memory/views/ring_view.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/sparse_set_view.h"
#include "../../../../memory/views/static_stencil_view.h"
#include "../../../../memory/views/stencil_view.h"
#include "../../../../memory/views/stencil_math.h"
#include "../../../../memory/views/swap_view.h"
#include "../../../../memory/views/strategies.h"

namespace ideam::core {

namespace {
    consteval size_t geometry_metadata_floor_pow2(size_t n) {
        if (n == 0) return 1;
        size_t res = 1;
        while ((res << 1) <= n) res <<= 1;
        return res;
    }

    template <MemoryTypes MemType, BufferAlignmentMode AlignMode, size_t TargetVectorWidthBytes>
    struct GeometryMetadataAOSOALaneCalculator {
        static constexpr DataType DataFlag = NativeMemoryTraits<MemType>::DataFlag;
        static constexpr size_t get_lane_width() {
            if constexpr (DataFlag == DataType::CUSTOM) return 4;
            else {
                constexpr size_t byte_size = MemoryUtilities::get_type_byte_size(DataFlag, AlignMode);
                if constexpr (byte_size == 0) return 1;
                constexpr size_t raw_lanes = TargetVectorWidthBytes / byte_size;
                return geometry_metadata_floor_pow2(raw_lanes > 0 ? raw_lanes : 1);
            }
        }
    };

    template <typename S, typename = void> struct GeometryMetadataStrategyDimExtractor { static constexpr size_t value = 1; };
    template <typename S> struct GeometryMetadataStrategyDimExtractor<S, std::void_t<decltype(S::dimensions)>> { static constexpr size_t value = S::dimensions; };

    template <typename T_Resolver, typename Enable = void> struct GeometryMetadataKernelExtractorImpl { static constexpr size_t value = 0; static constexpr bool has_kernel = false; };
    template <typename T_Resolver> struct GeometryMetadataKernelExtractorImpl<T_Resolver, std::void_t<decltype(T_Resolver::Type::KERNEL_POINTS)>> { static constexpr size_t value = T_Resolver::Type::KERNEL_POINTS; static constexpr bool has_kernel = true; };

    // --- GeometryMetadataLogicResolver Specializations ---
    template <GeometryMetadataLogicID ID, typename T_Concrete, typename T_Strategy>
    struct GeometryMetadataLogicResolver { static constexpr bool is_valid = false; };
    template <typename C, typename S> struct GeometryMetadataLogicResolver<GeometryMetadataLogicID::SpatialHashPartition, C, S> { using Type = SpatialHashPartitionMetadataLogic<C>; static constexpr bool is_valid = true; };

    template <GeometryMetadataLogicID LogicID, typename C, typename S>
    struct GeometryMetadataLogicKernelExtractor : GeometryMetadataKernelExtractorImpl<GeometryMetadataLogicResolver<LogicID, C, S>> {};

    template <MemoryStrategy ID> struct GeometryMetadataStrategyResolver { static constexpr bool is_valid = false; };
    template <> struct GeometryMetadataStrategyResolver<MemoryStrategy::FlatStrategy> { using Type = FlatStrategy; static constexpr bool is_valid = true; };
    template <> struct GeometryMetadataStrategyResolver<MemoryStrategy::SoAStrategy> { using Type = SoAStrategy; static constexpr bool is_valid = true; };
    template <> struct GeometryMetadataStrategyResolver<MemoryStrategy::AoSStrategy> { using Type = AoSStrategy; static constexpr bool is_valid = true; };
    template <> struct GeometryMetadataStrategyResolver<MemoryStrategy::Spatial2DStrategy> { using Type = Spatial2DStrategy; static constexpr bool is_valid = true; };
    template <> struct GeometryMetadataStrategyResolver<MemoryStrategy::Spatial3DStrategy> { using Type = Spatial3DStrategy; static constexpr bool is_valid = true; };
    template <> struct GeometryMetadataStrategyResolver<MemoryStrategy::Spatial4DStrategy> { using Type = Spatial4DStrategy; static constexpr bool is_valid = true; };
    template <> struct GeometryMetadataStrategyResolver<MemoryStrategy::TiledSoAStrategy> { using Type = TiledSoAStrategy; static constexpr bool is_valid = true; };
    template <> struct GeometryMetadataStrategyResolver<MemoryStrategy::RingStrategy> { using Type = RingStrategy; static constexpr bool is_valid = true; };
    template <> struct GeometryMetadataStrategyResolver<MemoryStrategy::PagedStrategy> { using Type = PagedStrategy; static constexpr bool is_valid = true; };

    template <MemoryView ViewID, GeometryMetadataLogicID LogicID, MemoryTypes MemType, typename T_Concrete, typename T_Strategy>
    struct GeometryMetadataViewResolver { static constexpr bool is_valid = false; };
    template <GeometryMetadataLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct GeometryMetadataViewResolver<MemoryView::SingleElementView, LogicID, MemType, C, S> { using Type = SingleElementView<C, S>; static constexpr bool is_valid = true; };
    template <GeometryMetadataLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct GeometryMetadataViewResolver<MemoryView::MultiElementView, LogicID, MemType, C, S> { using Type = MultiElementView<S>; static constexpr bool is_valid = true; };
    template <GeometryMetadataLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct GeometryMetadataViewResolver<MemoryView::SparseSetView, LogicID, MemType, C, S> { using Type = SparseSetView<C, S>; static constexpr bool is_valid = true; };
    template <GeometryMetadataLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct GeometryMetadataViewResolver<MemoryView::PagedView, LogicID, MemType, C, S> { using Type = PagedView<C, S>; static constexpr bool is_valid = true; };
    template <GeometryMetadataLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct GeometryMetadataViewResolver<MemoryView::RingView, LogicID, MemType, C, S> { using Type = RingView<C, S>; static constexpr bool is_valid = true; };
    template <GeometryMetadataLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct GeometryMetadataViewResolver<MemoryView::StencilView, LogicID, MemType, C, S> { using Type = StencilView<C, S>; static constexpr bool is_valid = true; };
    template <GeometryMetadataLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct GeometryMetadataViewResolver<MemoryView::AtomicView, LogicID, MemType, C, S> { using Type = AtomicView<C, S>; static constexpr bool is_valid = true; };
    template <GeometryMetadataLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct GeometryMetadataViewResolver<MemoryView::SwapView, LogicID, MemType, C, S> { using Type = SwapView<C, S>; static constexpr bool is_valid = true; };
    template <GeometryMetadataLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct GeometryMetadataViewResolver<MemoryView::BridgeView, LogicID, MemType, C, S> { static constexpr size_t DimCount = GeometryMetadataStrategyDimExtractor<S>::value; using Type = BridgeView<C, C, DimCount, S>; static constexpr bool is_valid = true; };
    template <GeometryMetadataLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct GeometryMetadataViewResolver<MemoryView::StaticStencilView, LogicID, MemType, C, S> { using Extractor = GeometryMetadataLogicKernelExtractor<LogicID, C, S>; static constexpr size_t PointCount = Extractor::has_kernel ? Extractor::value : 1; using Type = StaticStencilView<C, S, PointCount>; static constexpr bool is_valid = Extractor::has_kernel; };
    template <GeometryMetadataLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct GeometryMetadataViewResolver<MemoryView::AOSOA_Tight_AVX2, LogicID, MemType, C, S> { static constexpr size_t LaneWidth = GeometryMetadataAOSOALaneCalculator<MemType, BufferAlignmentMode::TIGHT, 32>::get_lane_width(); using Type = AOSOAView<C, LaneWidth, S>; static constexpr bool is_valid = true; };
    template <GeometryMetadataLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct GeometryMetadataViewResolver<MemoryView::AOSOA_Tight_AVX512, LogicID, MemType, C, S> { static constexpr size_t LaneWidth = GeometryMetadataAOSOALaneCalculator<MemType, BufferAlignmentMode::TIGHT, 64>::get_lane_width(); using Type = AOSOAView<C, LaneWidth, S>; static constexpr bool is_valid = true; };
    template <GeometryMetadataLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct GeometryMetadataViewResolver<MemoryView::AOSOA_STD430_AVX2, LogicID, MemType, C, S> { static constexpr size_t LaneWidth = GeometryMetadataAOSOALaneCalculator<MemType, BufferAlignmentMode::STD430, 32>::get_lane_width(); using Type = AOSOAView<C, LaneWidth, S>; static constexpr bool is_valid = true; };
    template <GeometryMetadataLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct GeometryMetadataViewResolver<MemoryView::AOSOA_STD430_AVX512, LogicID, MemType, C, S> { static constexpr size_t LaneWidth = GeometryMetadataAOSOALaneCalculator<MemType, BufferAlignmentMode::STD430, 64>::get_lane_width(); using Type = AOSOAView<C, LaneWidth, S>; static constexpr bool is_valid = true; };
    template <GeometryMetadataLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct GeometryMetadataViewResolver<MemoryView::AOSOA_STD140_AVX2, LogicID, MemType, C, S> { static constexpr size_t LaneWidth = GeometryMetadataAOSOALaneCalculator<MemType, BufferAlignmentMode::STD140, 32>::get_lane_width(); using Type = AOSOAView<C, LaneWidth, S>; static constexpr bool is_valid = true; };
    template <GeometryMetadataLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct GeometryMetadataViewResolver<MemoryView::AOSOA_STD140_AVX512, LogicID, MemType, C, S> { static constexpr size_t LaneWidth = GeometryMetadataAOSOALaneCalculator<MemType, BufferAlignmentMode::STD140, 64>::get_lane_width(); using Type = AOSOAView<C, LaneWidth, S>; static constexpr bool is_valid = true; };
} // namespace

template <GeometryMetadataLogicID L>
struct GeometryMetadataLogicSubRegistry {
    static std::array<GeometryMetadataTaskFactoryFn, GeometryMetadataTaskRegistry::SUB_MATRIX_SIZE> factories;

    static void init_execution_routing() {
        _fill_matrix<false>(std::make_index_sequence<GeometryMetadataTaskRegistry::SUB_MATRIX_SIZE>{}, nullptr);
    }

    static void cleanup_execution_routing() {
        factories.fill(nullptr);
    }

    static void generate_ui_matrices(godot::Dictionary& p_matrix) {
        IdeamTaskRegistry::log_with_registry(godot::vformat("    -> [GeometryMetadata] Evaluating LogicID: %d", static_cast<int64_t>(L)));
        _fill_matrix<true>(std::make_index_sequence<GeometryMetadataTaskRegistry::SUB_MATRIX_SIZE>{}, &p_matrix);

        godot::StringName logic_key(godot::String::num_int64(static_cast<int64_t>(L)));
        if (p_matrix.has(logic_key)) {
            godot::Dictionary dict = p_matrix[logic_key];
            godot::PackedInt64Array combos = dict["valid_combinations"];
            IdeamTaskRegistry::log_with_registry(godot::vformat("      -> SUCCESS: LogicID %d appended with %d valid combinations.", static_cast<int64_t>(L), combos.size()));
        } else {
            IdeamTaskRegistry::log_with_registry(godot::vformat("      -> FAILURE: LogicID %d resulted in 0 valid combinations. Key not created.", static_cast<int64_t>(L)));
        }
    }

private:
    template <bool BuildUI, size_t... Indices>
    static void _fill_matrix(std::index_sequence<Indices...>, godot::Dictionary* p_matrix) {
        (_fill_single<BuildUI, Indices>(p_matrix), ...);
    }

    template <bool BuildUI, size_t FlatIdx>
    static void _fill_single(godot::Dictionary* p_matrix) {
        constexpr size_t T_COUNT = GeometryMetadataTaskRegistry::T_COUNT;
        constexpr size_t S_COUNT = GeometryMetadataTaskRegistry::S_COUNT;

        constexpr size_t T = FlatIdx % T_COUNT;
        constexpr size_t S = (FlatIdx / T_COUNT) % S_COUNT;
        constexpr size_t V = FlatIdx / (T_COUNT * S_COUNT);

        constexpr MemoryView ViewEnum = static_cast<MemoryView>(V);
        constexpr MemoryStrategy StrategyEnum = static_cast<MemoryStrategy>(S);
        constexpr MemoryTypes MemTypeEnum = static_cast<MemoryTypes>(T);

        using Traits = NativeMemoryTraits<MemTypeEnum>;
        using ConcreteType = typename Traits::ConcreteType;

        using ResolvedStrategy = GeometryMetadataStrategyResolver<StrategyEnum>;
        if constexpr (!ResolvedStrategy::is_valid) return;
        using T_Strategy = typename ResolvedStrategy::Type;

        using ResolvedLogic = GeometryMetadataLogicResolver<L, ConcreteType, T_Strategy>;
        if constexpr (!ResolvedLogic::is_valid) return;
        using L_Type = typename ResolvedLogic::Type;

        using ResolvedView = GeometryMetadataViewResolver<ViewEnum, L, MemTypeEnum, ConcreteType, T_Strategy>;
        if constexpr (!ResolvedView::is_valid) return;
        using V_Type = typename ResolvedView::Type;
        using V_Traits = ViewTraits<V_Type>;

        constexpr bool is_fully_valid = []() consteval {
            constexpr ViewStrategies iter_strategy_mask = to_view_strategy_mask(StrategyEnum);
            if ((V_Traits::supported_strategies & iter_strategy_mask) == ViewStrategies::NONE) return false;

            constexpr DataType iter_type_mask = Traits::DataFlag;
            if ((L_Type::required_types & iter_type_mask) == DataType::NONE) return false;
            if ((V_Traits::supported_types & iter_type_mask) == DataType::NONE) return false;

            return MetadataLogicValidator::validate<L_Type, V_Type>();
        }();

        if constexpr (is_fully_valid) {
            if constexpr (!BuildUI) {
                factories[FlatIdx] = []() -> INativeTask* {
                    return new MetadataTask<L_Type, V_Type, T_Strategy>();
                };
            } else {
                if (p_matrix) {
                    IdeamTaskRegistry::log_with_registry(
                        godot::vformat("        -> Valid Combo: T=%d, S=%d, V=%d(FlatIdx: %d)", T, S, V, FlatIdx)
                    );

                    godot::StringName logic_key(godot::String::num_int64(static_cast<int64_t>(L)));

                    if (!p_matrix->has(logic_key)) {
                        godot::Dictionary dict;
                        dict["properties"] = L_Type::get_ui_properties();
                        dict["valid_combinations"] = godot::PackedInt64Array();
                        dict["name"] = godot::String(L_Type::display_name.data());

                        constexpr std::string_view cat = LogicCategoryExtractor<L_Type>::value;
                        dict["category"] = godot::String(cat.data());

                        const auto& variant_info = GeometryMetadataTaskRegistry::logic_variants[static_cast<size_t>(L)];
                        if (variant_info.is_primary && variant_info.variant_count > 1) {
                            dict["variant_count"] = variant_info.variant_count;

                            godot::Array ui_labels;
                            for (uint32_t i = 0; i < variant_info.variant_count; ++i) {
                                ui_labels.push_back(godot::String(variant_info.labels[i]));
                            }
                            dict["variant_labels"] = ui_labels;
                        }

                        (*p_matrix)[logic_key] = dict;
                    }

                    godot::Dictionary dict = (*p_matrix)[logic_key];
                    godot::PackedInt64Array combos = dict["valid_combinations"];
                    combos.push_back(static_cast<int64_t>(FlatIdx));
                    dict["valid_combinations"] = combos;
                    (*p_matrix)[logic_key] = dict;
                }
            }
        }
    }
};

template <GeometryMetadataLogicID L>
std::array<GeometryMetadataTaskFactoryFn, GeometryMetadataTaskRegistry::SUB_MATRIX_SIZE>
GeometryMetadataLogicSubRegistry<L>::factories = [] {
    std::array<GeometryMetadataTaskFactoryFn, GeometryMetadataTaskRegistry::SUB_MATRIX_SIZE> arr;
    arr.fill(nullptr);
    return arr;
}();

} // namespace ideam::core