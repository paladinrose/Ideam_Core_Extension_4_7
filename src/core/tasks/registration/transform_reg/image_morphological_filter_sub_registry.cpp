// AUTO-GENERATED FILE
#include "../transform_logic_sub_registry.h"
#include "../../transform_logic/image_morphological_filter_transform_logic.h" // Ensure the compiler sees the logic struct

namespace ideam::core {

// Explicitly instantiate the factory matrix for ImageMorphologicalFilter
template struct TransformLogicSubRegistry<TransformLogicID::ImageMorphologicalFilter_Moore_R1>;
template struct TransformLogicSubRegistry<TransformLogicID::ImageMorphologicalFilter_Moore_R2>;
template struct TransformLogicSubRegistry<TransformLogicID::ImageMorphologicalFilter_Moore_R3>;
template struct TransformLogicSubRegistry<TransformLogicID::ImageMorphologicalFilter_VonNeumann_R1>;
template struct TransformLogicSubRegistry<TransformLogicID::ImageMorphologicalFilter_VonNeumann_R2>;
template struct TransformLogicSubRegistry<TransformLogicID::ImageMorphologicalFilter_VonNeumann_R3>;


} // namespace ideam::core
