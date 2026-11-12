using Ideam.Science.Formal.Mathematics.VectorFields;

namespace Ideam.Science.Formal.Mathematics.Tensors.Tracing
{
    public struct TracingSeed
    {
        public readonly Float2 Point;
        public readonly IFloat2Field Field;
        public readonly IFloat2Field AlternativeField;

        public TracingSeed(Float2 point, IFloat2Field field, IFloat2Field alternativeField)
            : this()
        {
            Point = point;
            Field = field;
            AlternativeField = alternativeField;
        }
    }

}