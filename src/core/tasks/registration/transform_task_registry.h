#pragma once

#include "../i_native_task.h"
#include "../../memory/memory_common.h"
#include "ideam_task_registry.h"
#include <godot_cpp/variant/dictionary.hpp>
#include <memory>
#include <array>
#include <cstdint>

namespace ideam::core {

// --- Compile-Time Coordinates (The O(1) Indices) ---
enum class TransformLogicID : uint32_t {
    ArmorSpallPeel = 0,
    AudioBusVolumeAndEffectSync,
    AudioGeneratorProceduralPCM,
    AudioPolyphonicStreamTrigger,
    AudioSpectrumAnalyzerCapture,
    BlendShapeEvaluate_1Shape,
    BlendShapeEvaluate_2Shapes,
    BlendShapeEvaluate_4Shapes,
    BlendShapeEvaluate_8Shapes,
    BoundaryConstraint,
    BoundsExtraction,
    CollisionShapeMeshSync,
    CPULinearBlendSkinning,
    CustomDataPack,
    DataScatter,
    DataSort,
    EdgeSplitTear,
    EulerIntegration,
    FabrikChainIK,
    FaceCenterAndArea,
    FastNoiseLite,
    ForwardKinematicsChain,
    GlobalShaderParameterSync,
    GPUParticlesEmitBridge,
    GridDensitySplat,
    HalfFloatPack,
    ImageBilateralDenoise_Moore_R1,
    ImageBilateralDenoise_Moore_R2,
    ImageBilateralDenoise_Moore_R3,
    ImageBilateralDenoise_VonNeumann_R1,
    ImageBilateralDenoise_VonNeumann_R2,
    ImageBilateralDenoise_VonNeumann_R3,
    ImageChannelExtract,
    ImageChannelPack,
    ImageHistogramEqualize,
    ImageMipmapDownsample,
    ImageMorphologicalFilter_Moore_R1,
    ImageMorphologicalFilter_Moore_R2,
    ImageMorphologicalFilter_Moore_R3,
    ImageMorphologicalFilter_VonNeumann_R1,
    ImageMorphologicalFilter_VonNeumann_R2,
    ImageMorphologicalFilter_VonNeumann_R3,
    ImageNormalFromHeightmap_Moore_R1,
    ImageNormalFromHeightmap_VonNeumann_R1,
    ImageNormalizeScalar,
    ImagePaletteLut,
    ImageRawExport,
    ImageRawImport,
    ImageSeamlessTileBlend,
    ImageSpatial2DExport,
    ImageSpatial2DImport,
    ImageTexture2DServerUpdate,
    InstanceShaderParamBatch,
    JoltConvexAndHeightmapShapeSync,
    JoltJointMotorAndLimitSync,
    MeshAttributeRegionCommit,
    MeshBoundsSync,
    MeshHolePunchBoolean,
    MeshLaplacianCurvature,
    MeshSkinRegionCommit,
    MeshStreamPack,
    MeshStreamUnpack,
    MeshSurfaceExport,
    MeshSurfaceImport,
    MeshSurfaceScatter,
    MeshUVProjection,
    MeshVertexColorMap,
    MeshVertexDeform,
    MeshVertexRegionCommit,
    MultiMeshBufferSync,
    NoiseInjection,
    NormalRecalc,
    PackedArrayExport,
    ParticleEmitterClaim,
    ParticleFieldAdvection,
    ParticleLifetimeAndCurve,
    ParticleTrailRibbonGenerate,
    PhysicsBatchRaycastAndShapeCast,
    PhysicsBodyStateBatchSync,
    PhysicsImpulseAndForceBatch,
    PlanarMeshSlice,
    QuadricMeshDecimate,
    RDAsyncReadbackPoll,
    RDComputeDispatchIndirectPrep,
    RDComputePipelineDispatch,
    RDPushConstantPack,
    RDStd140Std430LayoutPack,
    RDStorageBufferSync,
    RDTexture2DRDBridge,
    RDVertexAttributeBufferBridge,
    ShaderDataTexturePack,
    ShaderMaterialParamSync,
    SkeletonPoseBridge,
    SpatialAudioAttenuationAndDoppler,
    SpringBoneChain,
    Stencil_Moore_R1,
    Stencil_Moore_R2,
    Stencil_Moore_R3,
    Stencil_VonNeumann_R1,
    Stencil_VonNeumann_R2,
    Stencil_VonNeumann_R3,
    SurfaceHeatAblation,
    SurfaceWavePDE,
    TangentRecalc,
    Transform3DBatch,
    TwoBoneIK,
    ValueAccumulation,
    VectorFieldCurlNoise_Moore_R1,
    VectorFieldCurlNoise_VonNeumann_R1,
    VoronoiMeshFracture,
    XPBDVertexSoftBody,
    Count
};

using TransformTaskFactoryFn = INativeTask* (*)();

class TransformTaskRegistry {
public:
    static constexpr size_t L_COUNT = static_cast<size_t>(TransformLogicID::Count);
    static constexpr size_t V_COUNT = static_cast<size_t>(MemoryView::Count);
    static constexpr size_t S_COUNT = static_cast<size_t>(MemoryStrategy::Count);
    static constexpr size_t T_COUNT = static_cast<size_t>(MemoryTypes::Count);

    static const std::array<LogicVariantGroup, L_COUNT> logic_variants;

    // --- DOD Matrix Bounds (3D Sub-Matrix) ---
    static constexpr size_t SUB_MATRIX_SIZE = V_COUNT * S_COUNT * T_COUNT;

    using SubMatrix = std::array<TransformTaskFactoryFn, SUB_MATRIX_SIZE>;

    // --- The O(1) Multi-Dimensional Factory Routers ---
    static std::array<const SubMatrix*, L_COUNT> logic_matrices;

    /**
     * ui_transform_matrix Data Schema (Exported to Godot):
     * {
     * "LogicID (Stringified Int)": {
     * "properties": [ Array of godot::Dictionary (PropertyInfo structures from Logic::get_ui_properties) ],
     * "valid_combinations": PackedInt64Array [ FlatIdx hashes of valid View/Strategy/Type combinations ]
     * }
     * }
     */
    //static godot::Dictionary ui_transform_matrix;

    // --- Lifecycle Management ---

    // Fast-path: Instantiates C++ function pointers for runtime execution
    static void init_execution_routing();
    static void cleanup_execution_routing();

    // Heavy-path: Allocates Godot Dictionaries for the Editor UI (Call ONLY when baking manifest)
    static void generate_ui_matrices(godot::Dictionary& p_matrix);

    static std::unique_ptr<INativeTask> create(uint32_t p_logic_id, uint32_t p_view_id, uint32_t p_strategy_id, uint32_t p_type_id);
};

} // namespace ideam::core