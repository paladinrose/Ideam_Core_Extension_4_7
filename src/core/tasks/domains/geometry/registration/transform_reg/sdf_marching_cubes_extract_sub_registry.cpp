// AUTO-GENERATED FILE
#include "../geometry_transform_logic_sub_registry.h"
#include "../../transform_logic/sdf_marching_cubes_extract_transform_logic.h" // Ensure the compiler sees the logic struct

namespace ideam::core {

// Explicitly instantiate the factory matrix for SDFMarchingCubesExtract
template struct GeometryTransformLogicSubRegistry<GeometryTransformLogicID::SDFMarchingCubesExtract_Moore_R1>;
template struct GeometryTransformLogicSubRegistry<GeometryTransformLogicID::SDFMarchingCubesExtract_VonNeumann_R1>;

} // namespace ideam::core
