#include "../transform_logic_sub_registry.h"
#include "../../transform_logic/blend_shape_evaluate_transform_logic.h"

namespace ideam::core {

template struct TransformLogicSubRegistry<TransformLogicID::BlendShapeEvaluate_1Shape>;
template struct TransformLogicSubRegistry<TransformLogicID::BlendShapeEvaluate_2Shapes>;
template struct TransformLogicSubRegistry<TransformLogicID::BlendShapeEvaluate_4Shapes>;
template struct TransformLogicSubRegistry<TransformLogicID::BlendShapeEvaluate_8Shapes>;

} // namespace ideam::core