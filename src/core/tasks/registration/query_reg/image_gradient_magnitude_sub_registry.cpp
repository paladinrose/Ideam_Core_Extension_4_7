// AUTO-GENERATED FILE
#include "../query_logic_sub_registry.h"
#include "../../query_logic/image_gradient_magnitude_query_logic.h" // Ensure the compiler sees the logic struct

namespace ideam::core {

// Explicitly instantiate the factory matrix for ImageGradientMagnitude
template struct QueryLogicSubRegistry<QueryLogicID::ImageGradientMagnitude_Moore_R1>;
template struct QueryLogicSubRegistry<QueryLogicID::ImageGradientMagnitude_VonNeumann_R1>;

} // namespace ideam::core
