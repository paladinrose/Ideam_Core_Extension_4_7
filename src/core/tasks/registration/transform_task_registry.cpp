#include "transform_task_registry.h"
#include "transform_logic_sub_registry.h"
#include <utility>

namespace ideam::core {

namespace { // Translation Unit Firewall
    // --- UI Labels for OptionButtons ---
    constexpr const char* stencil_labels[] = {
        "Moore (R1)", "Moore (R2)", "Moore (R3)",
        "Von Neumann (R1)", "Von Neumann (R2)", "Von Neumann (R3)"
    };

    constexpr const char* r1_stencil_labels[] = {
        "Moore (R1)", "Von Neumann (R1)"
    };

    constexpr const char* blend_shape_labels[] = {
        "1 Shape", "2 Shapes", "4 Shapes", "8 Shapes"
    };
} // namespace

// --- Populate the Variant Map ---
const std::array<LogicVariantGroup, TransformTaskRegistry::L_COUNT> TransformTaskRegistry::logic_variants = {{
    // ArmorSpallPeel
    { true, 1, nullptr },
    // AudioBusVolumeAndEffectSync
    { true, 1, nullptr },
    // AudioGeneratorProceduralPCM
    { true, 1, nullptr },
    // AudioPolyphonicStreamTrigger
    { true, 1, nullptr },
    // AudioSpectrumAnalyzerCapture
    { true, 1, nullptr },

    // BlendShapeEvaluate_1Shape
    { true, 4, blend_shape_labels },
    // BlendShapeEvaluate_2Shapes, 4Shapes, 8Shapes
    { false, 0, nullptr },
    { false, 0, nullptr },
    { false, 0, nullptr },

    // BoundaryConstraint
    { true, 1, nullptr },
    // BoundsExtraction
    { true, 1, nullptr },
    // CollisionShapeMeshSync
    { true, 1, nullptr },
    // CPULinearBlendSkinning
    { true, 1, nullptr },
    // CustomDataPack
    { true, 1, nullptr },
    // DataScatter
    { true, 1, nullptr },
    // DataSort
    { true, 1, nullptr },
    // EdgeSplitTear
    { true, 1, nullptr },
    // EulerIntegration
    { true, 1, nullptr },
    // FabrikChainIK
    { true, 1, nullptr },
    // FaceCenterAndArea
    { true, 1, nullptr },
    // FastNoiseLite
    { true, 1, nullptr },
    // ForwardKinematicsChain
    { true, 1, nullptr },
    // GaugeBatchTelemetry
    { true, 1, nullptr },
    // GlobalShaderParameterSync
    { true, 1, nullptr },
    // GPUParticlesEmitBridge
    { true, 1, nullptr },
    // GraphShortestPathWavefront
    { true, 1, nullptr },
    // GraphStateTransition
    { true, 1, nullptr },
    // GridDensitySplat
    { true, 1, nullptr },
    // HalfFloatPack
    { true, 1, nullptr },
    // HeatmapGridBufferSync
    { true, 1, nullptr },

    // ImageBilateralDenoise_Moore_R1
    { true, 6, stencil_labels },
    // ImageBilateralDenoise_Moore_R2, R3, VonNeumann_R1, R2, R3
    { false, 0, nullptr },
    { false, 0, nullptr },
    { false, 0, nullptr },
    { false, 0, nullptr },
    { false, 0, nullptr },

    // ImageChannelExtract
    { true, 1, nullptr },
    // ImageChannelPack
    { true, 1, nullptr },
    // ImageHistogramEqualize
    { true, 1, nullptr },
    // ImageMipmapDownsample
    { true, 1, nullptr },

    // ImageMorphologicalFilter_Moore_R1
    { true, 6, stencil_labels },
    // ImageMorphologicalFilter_Moore_R2, R3, VonNeumann_R1, R2, R3
    { false, 0, nullptr },
    { false, 0, nullptr },
    { false, 0, nullptr },
    { false, 0, nullptr },
    { false, 0, nullptr },

    // ImageNormalFromHeightmap_Moore_R1
    { true, 2, r1_stencil_labels },
    // ImageNormalFromHeightmap_VonNeumann_R1
    { false, 0, nullptr },

    // ImageNormalizeScalar
    { true, 1, nullptr },
    // ImagePaletteLut
    { true, 1, nullptr },
    // ImageRawExport
    { true, 1, nullptr },
    // ImageRawImport
    { true, 1, nullptr },
    // ImageSeamlessTileBlend
    { true, 1, nullptr },
    // ImageSpatial2DExport
    { true, 1, nullptr },
    // ImageSpatial2DImport
    { true, 1, nullptr },
    // ImageTexture2DServerUpdate
    { true, 1, nullptr },
    // InstanceShaderParamBatch
    { true, 1, nullptr },
    // JoltConvexAndHeightmapShapeSync
    { true, 1, nullptr },
    // JoltJointMotorAndLimitSync
    { true, 1, nullptr },

    // MeshAttributeRegionCommit
    { true, 1, nullptr },
    // MeshBoundsSync
    { true, 1, nullptr },
    // MeshHolePunchBoolean
    { true, 1, nullptr },
    // MeshLaplacianCurvature
    { true, 1, nullptr },
    // MeshSkinRegionCommit
    { true, 1, nullptr },
    // MeshStreamPack
    { true, 1, nullptr },
    // MeshStreamUnpack
    { true, 1, nullptr },
    // MeshSurfaceExport
    { true, 1, nullptr },
    // MeshSurfaceImport
    { true, 1, nullptr },
    // MeshSurfaceScatter
    { true, 1, nullptr },
    // MeshUVProjection
    { true, 1, nullptr },
    // MeshVertexColorMap
    { true, 1, nullptr },
    // MeshVertexDeform
    { true, 1, nullptr },
    // MeshVertexRegionCommit
    { true, 1, nullptr },
    // MultiMeshBufferSync
    { true, 1, nullptr },

    // NoiseInjection
    { true, 1, nullptr },
    // NormalRecalc
    { true, 1, nullptr },
    // PackedArrayExport
    { true, 1, nullptr },
    // ParticleEmitterClaim
    { true, 1, nullptr },
    // ParticleFieldAdvection
    { true, 1, nullptr },
    // ParticleLifetimeAndCurve
    { true, 1, nullptr },
    // ParticleTrailRibbonGenerate
    { true, 1, nullptr },
    // PhysicsBatchRaycastAndShapeCast
    { true, 1, nullptr },
    // PhysicsBodyStateBatchSync
    { true, 1, nullptr },
    // PhysicsImpulseAndForceBatch
    { true, 1, nullptr },
    // PlanarMeshSlice
    { true, 1, nullptr },
    // QuadricMeshDecimate
    { true, 1, nullptr },
    // RDAsyncReadbackPoll
    { true, 1, nullptr },
    // RDComputeDispatchIndirectPrep
    { true, 1, nullptr },
    // RDComputePipelineDispatch
    { true, 1, nullptr },
    // RDPushConstantPack
    { true, 1, nullptr },
    // RDStd140Std430LayoutPack
    { true, 1, nullptr },
    // RDStorageBufferSync
    { true, 1, nullptr },
    // RDTexture2DRDBridge
    { true, 1, nullptr },
    // RDVertexAttributeBufferBridge
    { true, 1, nullptr },
    // ShaderDataTexturePack
    { true, 1, nullptr },
    // ShaderMaterialParamSync
    { true, 1, nullptr },
    // SignalAndRetargetFlush
    { true, 1, nullptr },
    // SkeletonPoseBridge
    { true, 1, nullptr },
    // SpatialAudioAttenuationAndDoppler
    { true, 1, nullptr },
    // SpringBoneChain
    { true, 1, nullptr },

    // Stencil_Moore_R1
    { true, 6, stencil_labels },
    // Stencil_Moore_R2, R3, VonNeumann_R1, R2, R3
    { false, 0, nullptr },
    { false, 0, nullptr },
    { false, 0, nullptr },
    { false, 0, nullptr },
    { false, 0, nullptr },

    // SurfaceHeatAblation
    { true, 1, nullptr },
    // SurfaceWavePDE
    { true, 1, nullptr },
    // TangentRecalc
    { true, 1, nullptr },
    // Transform3DBatch
    { true, 1, nullptr },
    // TwoBoneIK
    { true, 1, nullptr },
    // UndoRedoBatchCommandRecord
    { true, 1, nullptr },
    // ValueAccumulation
    { true, 1, nullptr },

    // VectorFieldCurlNoise_Moore_R1
    { true, 2, r1_stencil_labels },
    // VectorFieldCurlNoise_VonNeumann_R1
    { false, 0, nullptr },

    // VoronoiMeshFracture
    { true, 1, nullptr },
    // XPBDVertexSoftBody
    { true, 1, nullptr }
}};

std::array<const TransformTaskRegistry::SubMatrix*, TransformTaskRegistry::L_COUNT> TransformTaskRegistry::logic_matrices = {};

namespace { // Translation Unit Firewall
    // --- Execution Routing (Fast Path) ---
    template <size_t... Is>
    void init_routing_all_sub_registries(std::index_sequence<Is...>) {
        (TransformLogicSubRegistry<static_cast<TransformLogicID>(Is)>::init_execution_routing(), ...);
        ((TransformTaskRegistry::logic_matrices[Is] = &TransformLogicSubRegistry<static_cast<TransformLogicID>(Is)>::factories), ...);
    }

    template <size_t... Is>
    void cleanup_routing_all_sub_registries(std::index_sequence<Is...>) {
        (TransformLogicSubRegistry<static_cast<TransformLogicID>(Is)>::cleanup_execution_routing(), ...);
    }

    // --- UI Matrices (Heavy Path) ---
    template <size_t... Is>
    void generate_ui_all_sub_registries(godot::Dictionary& p_matrix, std::index_sequence<Is...>) {
        (TransformLogicSubRegistry<static_cast<TransformLogicID>(Is)>::generate_ui_matrices(p_matrix), ...);
    }
} // namespace

void TransformTaskRegistry::init_execution_routing() {
    // Compiles into consecutive fast-path init calls and pointer assignments
    init_routing_all_sub_registries(std::make_index_sequence<L_COUNT>{});
}

void TransformTaskRegistry::cleanup_execution_routing() {
    cleanup_routing_all_sub_registries(std::make_index_sequence<L_COUNT>{});
    logic_matrices.fill(nullptr);
}

void TransformTaskRegistry::generate_ui_matrices(godot::Dictionary& p_matrix) {
    IdeamTaskRegistry::log_with_registry("  -> TransformTaskRegistry: Unrolling templates for L_COUNT = " + godot::itos(L_COUNT));
    // Rips through valid template permutations to build the Godot Inspector UI
    generate_ui_all_sub_registries(p_matrix, std::make_index_sequence<L_COUNT>{});
    IdeamTaskRegistry::log_with_registry("  -> TransformTaskRegistry: Template unrolling complete.");
}

// --- The O(1) Dispatcher ---
std::unique_ptr<INativeTask> TransformTaskRegistry::create(uint32_t p_logic_id, uint32_t p_view_id, uint32_t p_strategy_id, uint32_t p_type_id) {
    if (p_logic_id >= L_COUNT || p_view_id >= V_COUNT || p_strategy_id >= S_COUNT || p_type_id >= T_COUNT) {
        return nullptr;
    }

    // 1. O(1) Cache-line hit for the sub-registry pointer
    const SubMatrix* sub_matrix = logic_matrices[p_logic_id];
    if (!sub_matrix) return nullptr;

    // 2. O(1) Flat Index Math (Dropped the L dimension)
    size_t flat_idx =
        p_view_id * (S_COUNT * T_COUNT) +
        p_strategy_id * (T_COUNT) +
        p_type_id;

    if ((*sub_matrix)[flat_idx]) {
        return std::unique_ptr<INativeTask>((*sub_matrix)[flat_idx]());
    }

    return nullptr;
}

} // namespace ideam::core