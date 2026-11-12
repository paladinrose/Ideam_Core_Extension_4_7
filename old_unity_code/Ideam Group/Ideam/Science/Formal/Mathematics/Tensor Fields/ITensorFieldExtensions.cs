using Ideam.Science.Formal.Mathematics.EiginVectors;

namespace Ideam.Science.Formal.Mathematics.Tensors
{
    public static class ITensorFieldExtensions
    {
        public static IEigenField Presample(this ITensorField field, Float2 min, Float2 max, uint resolution)
        {

            return ResampleAndRescaleEigenField.Create(field, min, max, resolution);
        }

        public static Tensor Sample(this ITensorField field, Float2 position)
        {

            Tensor result;
            field.Sample(ref position, out result);
            return result;
        }

        public static ITensorField DecayDistanceFromPoint(this ITensorField field, Float2 center, float decay)
        {

            return new PointDistanceDecayTensorField(field, center, decay);
        }
    }

}
