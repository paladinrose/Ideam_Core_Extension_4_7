#pragma once

#include "transform_task_registry.h"
#include "task_view_bridge.h" // Ensures strict one-way dependency graph
#include "../transform_task.h"
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/packed_int64_array.hpp>

// --- Logics ---
#include "../transform_logic/armor_spall_peel_transform_logic.h"
#include "../transform_logic/audio_bus_volume_and_effect_sync_transform_logic.h"
#include "../transform_logic/audio_generator_procedural_pcm_transform_logic.h"
#include "../transform_logic/audio_polyphonic_stream_trigger_transform_logic.h"
#include "../transform_logic/audio_spectrum_analyzer_capture_transform_logic.h"
#include "../transform_logic/blend_shape_evaluate_transform_logic.h"
#include "../transform_logic/boundary_constraint_transform_logic.h"
#include "../transform_logic/bounds_extraction_transform_logic.h"
#include "../transform_logic/collision_shape_mesh_sync_transform_logic.h"
#include "../transform_logic/cpu_linear_blend_skinning_transform_logic.h"
#include "../transform_logic/custom_data_pack_transform_logic.h"
#include "../transform_logic/data_scatter_transform_logic.h"
#include "../transform_logic/data_sort_transform_logic.h"
#include "../transform_logic/edge_split_tear_transform_logic.h"
#include "../transform_logic/euler_integration_transform_logic.h"
#include "../transform_logic/fabrik_chain_ik_transform_logic.h"
#include "../transform_logic/face_center_and_area_transform_logic.h"
#include "../transform_logic/fast_noise_lite_transform_logic.h"
#include "../transform_logic/forward_kinematics_chain_transform_logic.h"
#include "../transform_logic/gauge_batch_telemetry_transform_logic.h"
#include "../transform_logic/global_shader_parameter_sync_transform_logic.h"
#include "../transform_logic/gpu_particles_emit_bridge_transform_logic.h"
#include "../transform_logic/graph_shortest_path_wavefront_transform_logic.h"
#include "../transform_logic/graph_state_transition_transform_logic.h"
#include "../transform_logic/grid_density_splat_transform_logic.h"
#include "../transform_logic/half_float_pack_transform_logic.h"
#include "../transform_logic/heatmap_grid_buffer_sync_transform_logic.h"
#include "../transform_logic/image_bilateral_denoise_transform_logic.h"
#include "../transform_logic/image_channel_extract_transform_logic.h"
#include "../transform_logic/image_channel_pack_transform_logic.h"
#include "../transform_logic/image_histogram_equalize_transform_logic.h"
#include "../transform_logic/image_mipmap_downsample_transform_logic.h"
#include "../transform_logic/image_morphological_filter_transform_logic.h"
#include "../transform_logic/image_normal_from_heightmap_transform_logic.h"
#include "../transform_logic/image_normalize_scalar_transform_logic.h"
#include "../transform_logic/image_palette_lut_transform_logic.h"
#include "../transform_logic/image_raw_export_transform_logic.h"
#include "../transform_logic/image_raw_import_transform_logic.h"
#include "../transform_logic/image_seamless_tile_blend_transform_logic.h"
#include "../transform_logic/image_spatial2d_export_transform_logic.h"
#include "../transform_logic/image_spatial2d_import_transform_logic.h"
#include "../transform_logic/image_texture_2d_server_update_transform_logic.h"
#include "../transform_logic/instance_shader_param_batch_transform_logic.h"
#include "../transform_logic/jolt_convex_and_heightmap_shape_sync_transform_logic.h"
#include "../transform_logic/jolt_joint_motor_and_limit_sync_transform_logic.h"
#include "../transform_logic/mesh_attribute_region_commit_transform_logic.h"
#include "../transform_logic/mesh_bounds_sync_transform_logic.h"
#include "../transform_logic/mesh_hole_punch_boolean_transform_logic.h"
#include "../transform_logic/mesh_laplacian_curvature_transform_logic.h"
#include "../transform_logic/mesh_skin_region_commit_transform_logic.h"
#include "../transform_logic/mesh_stream_pack_transform_logic.h"
#include "../transform_logic/mesh_stream_unpack_transform_logic.h"
#include "../transform_logic/mesh_surface_export_transform_logic.h"
#include "../transform_logic/mesh_surface_import_transform_logic.h"
#include "../transform_logic/mesh_surface_scatter_transform_logic.h"
#include "../transform_logic/mesh_uv_projection_transform_logic.h"
#include "../transform_logic/mesh_vertex_color_map_transform_logic.h"
#include "../transform_logic/mesh_vertex_deform_transform_logic.h"
#include "../transform_logic/mesh_vertex_region_commit_transform_logic.h"
#include "../transform_logic/multi_mesh_buffer_sync_transform_logic.h"
#include "../transform_logic/noise_injection_transform_logic.h"
#include "../transform_logic/normal_recalc_transform_logic.h"
#include "../transform_logic/packed_array_export_transform_logic.h"
#include "../transform_logic/particle_emitter_claim_transform_logic.h"
#include "../transform_logic/particle_field_advection_transform_logic.h"
#include "../transform_logic/particle_lifetime_and_curve_transform_logic.h"
#include "../transform_logic/particle_trail_ribbon_generate_transform_logic.h"
#include "../transform_logic/physics_batch_raycast_and_shape_cast_transform_logic.h"
#include "../transform_logic/physics_body_state_batch_sync_transform_logic.h"
#include "../transform_logic/physics_impulse_and_force_batch_transform_logic.h"
#include "../transform_logic/planar_mesh_slice_transform_logic.h"
#include "../transform_logic/quadric_mesh_decimate_transform_logic.h"
#include "../transform_logic/rd_async_readback_poll_transform_logic.h"
#include "../transform_logic/rd_compute_dispatch_indirect_prep_transform_logic.h"
#include "../transform_logic/rd_compute_pipeline_dispatch_transform_logic.h"
#include "../transform_logic/rd_push_constant_pack_transform_logic.h"
#include "../transform_logic/rd_std140_std430_layout_pack_transform_logic.h"
#include "../transform_logic/rd_storage_buffer_sync_transform_logic.h"
#include "../transform_logic/rd_texture_2drd_bridge_transform_logic.h"
#include "../transform_logic/rd_vertex_attribute_buffer_bridge_transform_logic.h"
#include "../transform_logic/shader_data_texture_pack_transform_logic.h"
#include "../transform_logic/shader_material_param_sync_transform_logic.h"
#include "../transform_logic/signal_and_retarget_flush_transform_logic.h"
#include "../transform_logic/skeleton_pose_bridge_transform_logic.h"
#include "../transform_logic/spatial_audio_attenuation_and_doppler_transform_logic.h"
#include "../transform_logic/spring_bone_chain_transform_logic.h"
#include "../transform_logic/stencil_convolution_transform_logic.h"
#include "../transform_logic/surface_heat_ablation_transform_logic.h"
#include "../transform_logic/surface_wave_pde_transform_logic.h"
#include "../transform_logic/tangent_recalc_transform_logic.h"
#include "../transform_logic/transform_3d_batch_transform_logic.h"
#include "../transform_logic/two_bone_ik_transform_logic.h"
#include "../transform_logic/undo_redo_batch_command_record_transform_logic.h"
#include "../transform_logic/value_accumulation_transform_logic.h"
#include "../transform_logic/vector_field_curl_noise_transform_logic.h"
#include "../transform_logic/voronoi_mesh_fracture_transform_logic.h"
#include "../transform_logic/xpbd_vertex_soft_body_transform_logic.h"

// --- Views & Strategies ---
#include "../../memory/views/aosoa_view.h"
#include "../../memory/views/atomic_view.h"
#include "../../memory/views/bridge_view.h"
#include "../../memory/views/multi_element_view.h"
#include "../../memory/views/paged_view.h"
#include "../../memory/views/ring_view.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/sparse_set_view.h"
#include "../../memory/views/static_stencil_view.h"
#include "../../memory/views/stencil_view.h"
#include "../../memory/views/stencil_math.h"
#include "../../memory/views/swap_view.h"
#include "../../memory/views/strategies.h"

namespace ideam::core {

namespace {
    // --- Resolvers & Extractor Logic ---
    consteval size_t floor_power_of_2(size_t n) {
        if (n == 0) return 1;
        size_t res = 1;
        while ((res << 1) <= n) res <<= 1;
        return res;
    }

    template <MemoryTypes MemType, BufferAlignmentMode AlignMode, size_t TargetVectorWidthBytes>
    struct AOSOALaneCalculator {
        static constexpr DataType DataFlag = NativeMemoryTraits<MemType>::DataFlag;
        static constexpr size_t get_lane_width() {
            if constexpr (DataFlag == DataType::CUSTOM) return 4;
            else {
                constexpr size_t byte_size = MemoryUtilities::get_type_byte_size(DataFlag, AlignMode);
                if constexpr (byte_size == 0) return 1;
                constexpr size_t raw_lanes = TargetVectorWidthBytes / byte_size;
                return floor_power_of_2(raw_lanes > 0 ? raw_lanes : 1);
            }
        }
    };

    template <typename S, typename = void> struct StrategyDimExtractor { static constexpr size_t value = 1; };
    template <typename S> struct StrategyDimExtractor<S, std::void_t<decltype(S::dimensions)>> { static constexpr size_t value = S::dimensions; };

    template <typename T_Resolver, typename Enable = void> struct KernelExtractorImpl { static constexpr size_t value = 0; static constexpr bool has_kernel = false; };
    template <typename T_Resolver> struct KernelExtractorImpl<T_Resolver, std::void_t<decltype(T_Resolver::K_SIZE)>> { static constexpr size_t value = T_Resolver::K_SIZE; static constexpr bool has_kernel = true; };

    template <TransformLogicID ID, typename T_Concrete, typename T_Strategy> struct TransformLogicResolver { static constexpr bool is_valid = false; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::ArmorSpallPeel, C, S> { using Type = ArmorSpallPeelTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::AudioBusVolumeAndEffectSync, C, S> { using Type = AudioBusVolumeAndEffectSyncTransformLogic<C, S>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::AudioGeneratorProceduralPCM, C, S> { using Type = AudioGeneratorProceduralPCMTransformLogic<C, S>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::AudioPolyphonicStreamTrigger, C, S> { using Type = AudioPolyphonicStreamTriggerTransformLogic<C, S>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::AudioSpectrumAnalyzerCapture, C, S> { using Type = AudioSpectrumAnalyzerCaptureTransformLogic<C, S>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::BlendShapeEvaluate_1Shape, C, S>  { using Type = BlendShapeEvaluateTransformLogic<C, 1>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::BlendShapeEvaluate_2Shapes, C, S> { using Type = BlendShapeEvaluateTransformLogic<C, 2>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::BlendShapeEvaluate_4Shapes, C, S> { using Type = BlendShapeEvaluateTransformLogic<C, 4>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::BlendShapeEvaluate_8Shapes, C, S> { using Type = BlendShapeEvaluateTransformLogic<C, 8>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::BoundaryConstraint, C, S> { using Type = BoundaryConstraintTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::BoundsExtraction, C, S> { using Type = BoundsExtractionTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::CollisionShapeMeshSync, C, S> { using Type = CollisionShapeMeshSyncTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::CPULinearBlendSkinning, C, S> { using Type = CPULinearBlendSkinningTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::CustomDataPack, C, S> { using Type = CustomDataPackTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::DataScatter, C, S> { using Type = DataScatterTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::DataSort, C, S> { using Type = DataSortTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::EdgeSplitTear, C, S> { using Type = EdgeSplitTearTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::EulerIntegration, C, S> { using Type = EulerIntegrationTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::FabrikChainIK, C, S> { using Type = FabrikChainIKTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::FaceCenterAndArea, C, S> { using Type = FaceCenterAndAreaTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::FastNoiseLite, C, S> { using Type = FastNoiseLiteTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::ForwardKinematicsChain, C, S> { using Type = ForwardKinematicsChainTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::GaugeBatchTelemetry, C, S> { using Type = GaugeBatchTelemetryTransformLogic<C, S>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::GlobalShaderParameterSync, C, S> { using Type = GlobalShaderParameterSyncTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::GPUParticlesEmitBridge, C, S> { using Type = GPUParticlesEmitBridgeTransformLogic<C, S>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::GraphShortestPathWavefront, C, S> { using Type = GraphShortestPathWavefrontTransformLogic<C, S>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::GraphStateTransition, C, S> { using Type = GraphStateTransitionTransformLogic<C, S>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::GridDensitySplat, C, S> { using Type = GridDensitySplatTransformLogic<C, S>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::HalfFloatPack, C, S> { using Type = HalfFloatPackTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::HeatmapGridBufferSync, C, S> { using Type = HeatmapGridBufferSyncTransformLogic<C, S>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::ImageBilateralDenoise_Moore_R1, C, S> {
        static constexpr size_t K_SIZE = stencil_math::moore_size<S::dimensions, 1>();
        using Type = ImageBilateralDenoiseTransformLogic<C, S, K_SIZE>;
        static constexpr bool is_valid = true;
    };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::ImageBilateralDenoise_Moore_R2, C, S> {
        static constexpr size_t K_SIZE = stencil_math::moore_size<S::dimensions, 2>();
        using Type = ImageBilateralDenoiseTransformLogic<C, S, K_SIZE>;
        static constexpr bool is_valid = true;
    };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::ImageBilateralDenoise_Moore_R3, C, S> {
        static constexpr size_t K_SIZE = stencil_math::moore_size<S::dimensions, 3>();
        using Type = ImageBilateralDenoiseTransformLogic<C, S, K_SIZE>;
        static constexpr bool is_valid = true;
    };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::ImageBilateralDenoise_VonNeumann_R1, C, S> {
        static constexpr size_t K_SIZE = stencil_math::von_neumann_size<S::dimensions, 1>();
        using Type = ImageBilateralDenoiseTransformLogic<C, S, K_SIZE>;
        static constexpr bool is_valid = true;
    };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::ImageBilateralDenoise_VonNeumann_R2, C, S> {
        static constexpr size_t K_SIZE = stencil_math::von_neumann_size<S::dimensions, 2>();
        using Type = ImageBilateralDenoiseTransformLogic<C, S, K_SIZE>;
        static constexpr bool is_valid = true;
    };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::ImageBilateralDenoise_VonNeumann_R3, C, S> {
        static constexpr size_t K_SIZE = stencil_math::von_neumann_size<S::dimensions, 3>();
        using Type = ImageBilateralDenoiseTransformLogic<C, S, K_SIZE>;
        static constexpr bool is_valid = true;
    };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::ImageChannelExtract, C, S> { using Type = ImageChannelExtractTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::ImageChannelPack, C, S> { using Type = ImageChannelPackTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::ImageHistogramEqualize, C, S> { using Type = ImageHistogramEqualizeTransformLogic<C, S>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::ImageMipmapDownsample, C, S> { using Type = ImageMipmapDownsampleTransformLogic<C, S>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::ImageMorphologicalFilter_Moore_R1, C, S> {
        static constexpr size_t K_SIZE = stencil_math::moore_size<S::dimensions, 1>();
        using Type = ImageMorphologicalFilterTransformLogic<C, S, K_SIZE>;
        static constexpr bool is_valid = true;
    };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::ImageMorphologicalFilter_Moore_R2, C, S> {
        static constexpr size_t K_SIZE = stencil_math::moore_size<S::dimensions, 2>();
        using Type = ImageMorphologicalFilterTransformLogic<C, S, K_SIZE>;
        static constexpr bool is_valid = true;
    };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::ImageMorphologicalFilter_Moore_R3, C, S> {
        static constexpr size_t K_SIZE = stencil_math::moore_size<S::dimensions, 3>();
        using Type = ImageMorphologicalFilterTransformLogic<C, S, K_SIZE>;
        static constexpr bool is_valid = true;
    };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::ImageMorphologicalFilter_VonNeumann_R1, C, S> {
        static constexpr size_t K_SIZE = stencil_math::von_neumann_size<S::dimensions, 1>();
        using Type = ImageMorphologicalFilterTransformLogic<C, S, K_SIZE>;
        static constexpr bool is_valid = true;
    };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::ImageMorphologicalFilter_VonNeumann_R2, C, S> {
        static constexpr size_t K_SIZE = stencil_math::von_neumann_size<S::dimensions, 2>();
        using Type = ImageMorphologicalFilterTransformLogic<C, S, K_SIZE>;
        static constexpr bool is_valid = true;
    };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::ImageMorphologicalFilter_VonNeumann_R3, C, S> {
        static constexpr size_t K_SIZE = stencil_math::von_neumann_size<S::dimensions, 3>();
        using Type = ImageMorphologicalFilterTransformLogic<C, S, K_SIZE>;
        static constexpr bool is_valid = true;
    };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::ImageNormalFromHeightmap_Moore_R1, C, S> {
        static constexpr size_t K_SIZE = stencil_math::moore_size<S::dimensions, 1>();
        using Type = ImageNormalFromHeightmapTransformLogic<C, S, K_SIZE>;
        static constexpr bool is_valid = true;
    };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::ImageNormalFromHeightmap_VonNeumann_R1, C, S> {
        static constexpr size_t K_SIZE = stencil_math::von_neumann_size<S::dimensions, 1>();
        using Type = ImageNormalFromHeightmapTransformLogic<C, S, K_SIZE>;
        static constexpr bool is_valid = true;
    };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::ImageNormalizeScalar, C, S> { using Type = ImageNormalizeScalarTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::ImagePaletteLut, C, S> { using Type = ImagePaletteLutTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::ImageRawExport, C, S> { using Type = ImageRawExportTransformLogic; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::ImageRawImport, C, S> { using Type = ImageRawImportTransformLogic; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::ImageSeamlessTileBlend, C, S> { using Type = ImageSeamlessTileBlendTransformLogic<C, S>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::ImageSpatial2DExport, C, S> { using Type = ImageSpatial2DExportTransformLogic<C, S>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::ImageSpatial2DImport, C, S> { using Type = ImageSpatial2DImportTransformLogic<C, S>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::ImageTexture2DServerUpdate, C, S> { using Type = ImageTexture2DServerUpdateTransformLogic<C, S>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::InstanceShaderParamBatch, C, S> { using Type = InstanceShaderParamBatchTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::JoltConvexAndHeightmapShapeSync, C, S> { using Type = JoltConvexAndHeightmapShapeSyncTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::JoltJointMotorAndLimitSync, C, S> { using Type = JoltJointMotorAndLimitSyncTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::MeshAttributeRegionCommit, C, S> { using Type = MeshAttributeRegionCommitTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::MeshBoundsSync, C, S> { using Type = MeshBoundsSyncTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::MeshHolePunchBoolean, C, S> { using Type = MeshHolePunchBooleanTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::MeshLaplacianCurvature, C, S> { using Type = MeshLaplacianCurvatureTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::MeshSkinRegionCommit, C, S> { using Type = MeshSkinRegionCommitTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::MeshStreamPack, C, S> { using Type = MeshStreamPackTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::MeshStreamUnpack, C, S> { using Type = MeshStreamUnpackTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::MeshSurfaceExport, C, S> { using Type = MeshSurfaceExportTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::MeshSurfaceImport, C, S> { using Type = MeshSurfaceImportTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::MeshSurfaceScatter, C, S> { using Type = MeshSurfaceScatterTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::MeshUVProjection, C, S> { using Type = MeshUVProjectionTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::MeshVertexColorMap, C, S> { using Type = MeshVertexColorMapTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::MeshVertexDeform, C, S> { using Type = MeshVertexDeformTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::MeshVertexRegionCommit, C, S> { using Type = MeshVertexRegionCommitTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::MultiMeshBufferSync, C, S> { using Type = MultiMeshBufferSyncTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::NoiseInjection, C, S> { using Type = NoiseInjectionTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::NormalRecalc, C, S> { using Type = NormalRecalcTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::PackedArrayExport, C, S> { using Type = PackedArrayExportTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::ParticleEmitterClaim, C, S> { using Type = ParticleEmitterClaimTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::ParticleFieldAdvection, C, S> { using Type = ParticleFieldAdvectionTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::ParticleLifetimeAndCurve, C, S> { using Type = ParticleLifetimeAndCurveTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::ParticleTrailRibbonGenerate, C, S> { using Type = ParticleTrailRibbonGenerateTransformLogic<C, S>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::PhysicsBatchRaycastAndShapeCast, C, S> { using Type = PhysicsBatchRaycastAndShapeCastTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::PhysicsBodyStateBatchSync, C, S> { using Type = PhysicsBodyStateBatchSyncTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::PhysicsImpulseAndForceBatch, C, S> { using Type = PhysicsImpulseAndForceBatchTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::PlanarMeshSlice, C, S> { using Type = PlanarMeshSliceTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::QuadricMeshDecimate, C, S> { using Type = QuadricMeshDecimateTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::RDAsyncReadbackPoll, C, S> { using Type = RDAsyncReadbackPollTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::RDComputeDispatchIndirectPrep, C, S> { using Type = RDComputeDispatchIndirectPrepTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::RDComputePipelineDispatch, C, S> { using Type = RDComputePipelineDispatchTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::RDPushConstantPack, C, S> { using Type = RDPushConstantPackTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::RDStd140Std430LayoutPack, C, S> { using Type = RDStd140Std430LayoutPackTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::RDStorageBufferSync, C, S> { using Type = RDStorageBufferSyncTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::RDTexture2DRDBridge, C, S> { using Type = RDTexture2DRDBridgeTransformLogic<C, S>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::RDVertexAttributeBufferBridge, C, S> { using Type = RDVertexAttributeBufferBridgeTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::ShaderDataTexturePack, C, S> { using Type = ShaderDataTexturePackTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::ShaderMaterialParamSync, C, S> { using Type = ShaderMaterialParamSyncTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::SignalAndRetargetFlush, C, S> { using Type = SignalAndRetargetFlushTransformLogic<C, S>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::SkeletonPoseBridge, C, S> { using Type = SkeletonPoseBridgeTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::SpatialAudioAttenuationAndDoppler, C, S> { using Type = SpatialAudioAttenuationAndDopplerTransformLogic<C, S>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::SpringBoneChain, C, S> { using Type = SpringBoneChainTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::Stencil_Moore_R1, C, S> {
        static constexpr size_t K_SIZE = stencil_math::moore_size<S::dimensions, 1>();
        using Type = StencilConvolutionTransformLogic<C, S, K_SIZE>;
        static constexpr bool is_valid = true;
    };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::Stencil_Moore_R2, C, S> {
        static constexpr size_t K_SIZE = stencil_math::moore_size<S::dimensions, 2>();
        using Type = StencilConvolutionTransformLogic<C, S, K_SIZE>;
        static constexpr bool is_valid = true;
    };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::Stencil_Moore_R3, C, S> {
        static constexpr size_t K_SIZE = stencil_math::moore_size<S::dimensions, 3>();
        using Type = StencilConvolutionTransformLogic<C, S, K_SIZE>;
        static constexpr bool is_valid = true;
    };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::Stencil_VonNeumann_R1, C, S> {
        static constexpr size_t K_SIZE = stencil_math::von_neumann_size<S::dimensions, 1>();
        using Type = StencilConvolutionTransformLogic<C, S, K_SIZE>;
        static constexpr bool is_valid = true;
    };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::Stencil_VonNeumann_R2, C, S> {
        static constexpr size_t K_SIZE = stencil_math::von_neumann_size<S::dimensions, 2>();
        using Type = StencilConvolutionTransformLogic<C, S, K_SIZE>;
        static constexpr bool is_valid = true;
    };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::Stencil_VonNeumann_R3, C, S> {
        static constexpr size_t K_SIZE = stencil_math::von_neumann_size<S::dimensions, 3>();
        using Type = StencilConvolutionTransformLogic<C, S, K_SIZE>;
        static constexpr bool is_valid = true;
    };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::SurfaceHeatAblation, C, S> { using Type = SurfaceHeatAblationTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::SurfaceWavePDE, C, S> { using Type = SurfaceWavePDETransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::TangentRecalc, C, S> { using Type = TangentRecalcTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::Transform3DBatch, C, S> { using Type = Transform3DBatchTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::TwoBoneIK, C, S> { using Type = TwoBoneIKTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::UndoRedoBatchCommandRecord, C, S> { using Type = UndoRedoBatchCommandRecordTransformLogic<C, S>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::ValueAccumulation, C, S> { using Type = ValueAccumulationTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::VectorFieldCurlNoise_Moore_R1, C, S> {
        static constexpr size_t K_SIZE = stencil_math::moore_size<S::dimensions, 1>();
        using Type = VectorFieldCurlNoiseTransformLogic<C, S, K_SIZE>;
        static constexpr bool is_valid = true;
    };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::VectorFieldCurlNoise_VonNeumann_R1, C, S> {
        static constexpr size_t K_SIZE = stencil_math::von_neumann_size<S::dimensions, 1>();
        using Type = VectorFieldCurlNoiseTransformLogic<C, S, K_SIZE>;
        static constexpr bool is_valid = true;
    };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::VoronoiMeshFracture, C, S> { using Type = VoronoiMeshFractureTransformLogic<C>; static constexpr bool is_valid = true; };
    template <typename C, typename S> struct TransformLogicResolver<TransformLogicID::XPBDVertexSoftBody, C, S> { using Type = XPBDVertexSoftBodyTransformLogic<C>; static constexpr bool is_valid = true; };

    template <TransformLogicID LogicID, typename C, typename S> struct LogicKernelExtractor : KernelExtractorImpl<TransformLogicResolver<LogicID, C, S>> {};

    template <MemoryStrategy ID> struct StrategyResolver { static constexpr bool is_valid = false; };
    template <> struct StrategyResolver<MemoryStrategy::FlatStrategy> { using Type = FlatStrategy; static constexpr bool is_valid = true; };
    template <> struct StrategyResolver<MemoryStrategy::SoAStrategy> { using Type = SoAStrategy; static constexpr bool is_valid = true; };
    template <> struct StrategyResolver<MemoryStrategy::AoSStrategy> { using Type = AoSStrategy; static constexpr bool is_valid = true; };
    template <> struct StrategyResolver<MemoryStrategy::Spatial2DStrategy> { using Type = Spatial2DStrategy; static constexpr bool is_valid = true; };
    template <> struct StrategyResolver<MemoryStrategy::Spatial3DStrategy> { using Type = Spatial3DStrategy; static constexpr bool is_valid = true; };
    template <> struct StrategyResolver<MemoryStrategy::Spatial4DStrategy> { using Type = Spatial4DStrategy; static constexpr bool is_valid = true; };
    template <> struct StrategyResolver<MemoryStrategy::TiledSoAStrategy> { using Type = TiledSoAStrategy; static constexpr bool is_valid = true; };
    template <> struct StrategyResolver<MemoryStrategy::RingStrategy> { using Type = RingStrategy; static constexpr bool is_valid = true; };
    template <> struct StrategyResolver<MemoryStrategy::PagedStrategy> { using Type = PagedStrategy; static constexpr bool is_valid = true; };

    template <MemoryView ViewID, TransformLogicID LogicID, MemoryTypes MemType, typename T_Concrete, typename T_Strategy> struct TransformViewResolver { static constexpr bool is_valid = false; };
    template <TransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct TransformViewResolver<MemoryView::SingleElementView, LogicID, MemType, C, S> { using Type = SingleElementView<C, S>; static constexpr bool is_valid = true; };
    template <TransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct TransformViewResolver<MemoryView::MultiElementView, LogicID, MemType, C, S> { using Type = MultiElementView<S>; static constexpr bool is_valid = true; };
    template <TransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct TransformViewResolver<MemoryView::SparseSetView, LogicID, MemType, C, S> { using Type = SparseSetView<C, S>; static constexpr bool is_valid = true; };
    template <TransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct TransformViewResolver<MemoryView::PagedView, LogicID, MemType, C, S> { using Type = PagedView<C, S>; static constexpr bool is_valid = true; };
    template <TransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct TransformViewResolver<MemoryView::RingView, LogicID, MemType, C, S> { using Type = RingView<C, S>; static constexpr bool is_valid = true; };
    template <TransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct TransformViewResolver<MemoryView::StencilView, LogicID, MemType, C, S> { using Type = StencilView<C, S>; static constexpr bool is_valid = true; };
    template <TransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct TransformViewResolver<MemoryView::AtomicView, LogicID, MemType, C, S> { using Type = AtomicView<C, S>; static constexpr bool is_valid = true; };
    template <TransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct TransformViewResolver<MemoryView::SwapView, LogicID, MemType, C, S> { using Type = SwapView<C, S>; static constexpr bool is_valid = true; };
    template <TransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct TransformViewResolver<MemoryView::BridgeView, LogicID, MemType, C, S> { static constexpr size_t DimCount = StrategyDimExtractor<S>::value; using Type = BridgeView<C, C, DimCount, S>; static constexpr bool is_valid = true; };
    template <TransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct TransformViewResolver<MemoryView::StaticStencilView, LogicID, MemType, C, S> { using Extractor = LogicKernelExtractor<LogicID, C, S>; static constexpr size_t PointCount = Extractor::has_kernel ? Extractor::value : 1; using Type = StaticStencilView<C, S, PointCount>; static constexpr bool is_valid = Extractor::has_kernel; };
    template <TransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct TransformViewResolver<MemoryView::AOSOA_Tight_AVX2, LogicID, MemType, C, S> { static constexpr size_t LaneWidth = AOSOALaneCalculator<MemType, BufferAlignmentMode::TIGHT, 32>::get_lane_width(); using Type = AOSOAView<C, LaneWidth, S>; static constexpr bool is_valid = true; };
    template <TransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct TransformViewResolver<MemoryView::AOSOA_Tight_AVX512, LogicID, MemType, C, S> { static constexpr size_t LaneWidth = AOSOALaneCalculator<MemType, BufferAlignmentMode::TIGHT, 64>::get_lane_width(); using Type = AOSOAView<C, LaneWidth, S>; static constexpr bool is_valid = true; };
    template <TransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct TransformViewResolver<MemoryView::AOSOA_STD430_AVX2, LogicID, MemType, C, S> { static constexpr size_t LaneWidth = AOSOALaneCalculator<MemType, BufferAlignmentMode::STD430, 32>::get_lane_width(); using Type = AOSOAView<C, LaneWidth, S>; static constexpr bool is_valid = true; };
    template <TransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct TransformViewResolver<MemoryView::AOSOA_STD430_AVX512, LogicID, MemType, C, S> { static constexpr size_t LaneWidth = AOSOALaneCalculator<MemType, BufferAlignmentMode::STD430, 64>::get_lane_width(); using Type = AOSOAView<C, LaneWidth, S>; static constexpr bool is_valid = true; };
    template <TransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct TransformViewResolver<MemoryView::AOSOA_STD140_AVX2, LogicID, MemType, C, S> { static constexpr size_t LaneWidth = AOSOALaneCalculator<MemType, BufferAlignmentMode::STD140, 32>::get_lane_width(); using Type = AOSOAView<C, LaneWidth, S>; static constexpr bool is_valid = true; };
    template <TransformLogicID LogicID, MemoryTypes MemType, typename C, typename S> struct TransformViewResolver<MemoryView::AOSOA_STD140_AVX512, LogicID, MemType, C, S> { static constexpr size_t LaneWidth = AOSOALaneCalculator<MemType, BufferAlignmentMode::STD140, 64>::get_lane_width(); using Type = AOSOAView<C, LaneWidth, S>; static constexpr bool is_valid = true; };
}

template <TransformLogicID L>
struct TransformLogicSubRegistry {
    static std::array<TransformTaskFactoryFn, TransformTaskRegistry::SUB_MATRIX_SIZE> factories;

    // Fast-path: Instantiates C++ function pointers for runtime execution
    static void init_execution_routing() {
        _fill_matrix<false>(std::make_index_sequence<TransformTaskRegistry::SUB_MATRIX_SIZE>{}, nullptr);
    }

    static void cleanup_execution_routing() {
        factories.fill(nullptr);
    }

    // Heavy-path: Allocates Godot Dictionaries for the Editor UI
    static void generate_ui_matrices(godot::Dictionary& p_matrix) {
        IdeamTaskRegistry::log_with_registry(godot::vformat("    -> Evaluating LogicID: %d", static_cast<int64_t>(L)));

        _fill_matrix<true>(std::make_index_sequence<TransformTaskRegistry::SUB_MATRIX_SIZE>{}, &p_matrix);

        // POST-EVALUATION METRIC GATHERING
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
        constexpr size_t T_COUNT = TransformTaskRegistry::T_COUNT;
        constexpr size_t S_COUNT = TransformTaskRegistry::S_COUNT;

        constexpr size_t T = FlatIdx % T_COUNT;
        constexpr size_t S = (FlatIdx / T_COUNT) % S_COUNT;
        constexpr size_t V = FlatIdx / (T_COUNT * S_COUNT);

        constexpr MemoryView ViewEnum = static_cast<MemoryView>(V);
        constexpr MemoryStrategy StrategyEnum = static_cast<MemoryStrategy>(S);
        constexpr MemoryTypes MemTypeEnum = static_cast<MemoryTypes>(T);

        using Traits = NativeMemoryTraits<MemTypeEnum>;
        using ConcreteType = typename Traits::ConcreteType;

        // --- FAST-FAIL TIER 1: Base Resolution ---
        using ResolvedStrategy = StrategyResolver<StrategyEnum>;
        if constexpr (!ResolvedStrategy::is_valid) return;
        using T_Strategy = typename ResolvedStrategy::Type;

        using ResolvedLogic = TransformLogicResolver<L, ConcreteType, T_Strategy>;
        if constexpr (!ResolvedLogic::is_valid) return;
        using L_Type = typename ResolvedLogic::Type;

        using ResolvedView = TransformViewResolver<ViewEnum, L, MemTypeEnum, ConcreteType, T_Strategy>;
        if constexpr (!ResolvedView::is_valid) return;
        using V_Type = typename ResolvedView::Type;
        using V_Traits = ViewTraits<V_Type>;

        // --- FAST-FAIL TIER 2: Deep DOD Intersection ---
        constexpr bool is_fully_valid = []() consteval {
            // 1. Does the View support this Strategy at all?
            constexpr ViewStrategies iter_strategy_mask = to_view_strategy_mask(StrategyEnum);
            if ((V_Traits::supported_strategies & iter_strategy_mask) == ViewStrategies::NONE) return false;

            // 2. Does the primitive type satisfy both the View and the Logic?
            constexpr DataType iter_type_mask = Traits::DataFlag;
            if ((L_Type::required_types & iter_type_mask) == DataType::NONE) return false;
            if ((V_Traits::supported_types & iter_type_mask) == DataType::NONE) return false;

            // 3. Deep DOD Validation via Types (Capabilities, Layouts, Types, Dimensions, Kernels)
            return TransformLogicValidator::validate<L_Type, V_Type>();
        }();

        // --- FACTORY GENERATION ---
        if constexpr (is_fully_valid) {
            if constexpr (!BuildUI) {
                // Hot-Path Execution Routing
                factories[FlatIdx] = []() -> INativeTask* {
                    return new TransformTask<L_Type, V_Type, T_Strategy>();
                };
            } else {
                // Cold-Path UI Collection
                if (p_matrix) {
                    IdeamTaskRegistry::log_with_registry(
                        godot::vformat("        -> Valid Combo: T=%d, S=%d, V=%d(FlatIdx: %d)", T, S, V, FlatIdx)
                    );

                    godot::StringName logic_key(godot::String::num_int64(static_cast<int64_t>(L)));

                    if (!p_matrix->has(logic_key)) {
                        godot::Dictionary dict;
                        // Instantiate the property array exactly once per logic struct type
                        dict["properties"] = L_Type::get_ui_properties();
                        dict["valid_combinations"] = godot::PackedInt64Array();
                        dict["name"] = godot::String(L_Type::display_name.data());
                        constexpr std::string_view cat = LogicCategoryExtractor<L_Type>::value;
                        dict["category"] = godot::String(cat.data());

                        const auto& variant_info = TransformTaskRegistry::logic_variants[static_cast<size_t>(L)];
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

                    // Map this valid 3D configuration hash (FlatIdx) so the UI knows it's an allowed permutation
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

template <TransformLogicID L>
std::array<TransformTaskFactoryFn, TransformTaskRegistry::SUB_MATRIX_SIZE>
TransformLogicSubRegistry<L>::factories = []{
    std::array<TransformTaskFactoryFn, TransformTaskRegistry::SUB_MATRIX_SIZE> arr;
    arr.fill(nullptr);
    return arr;
}();

} // namespace ideam::core