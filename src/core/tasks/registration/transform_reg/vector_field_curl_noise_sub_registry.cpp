// AUTO-GENERATED FILE
#include "../transform_logic_sub_registry.h"
#include "../../transform_logic/vector_field_curl_noise_transform_logic.h" // Ensure the compiler sees the logic struct

namespace ideam::core {

// Explicitly instantiate the factory matrix for VectorFieldCurlNoise
template struct TransformLogicSubRegistry<TransformLogicID::VectorFieldCurlNoise_Moore_R1>;
template struct TransformLogicSubRegistry<TransformLogicID::VectorFieldCurlNoise_VonNeumann_R1>;

} // namespace ideam::core
