using System;

namespace Ideam.Science.Formal.Mathematics.Tensors
{
    public class PointDistanceDecayTensorField : ITensorField
    {
        private readonly ITensorField _field;
        private readonly Float2 _center;
        private readonly float _decay;

        public PointDistanceDecayTensorField(ITensorField field, Float2 center, float decay)
        {
            
            _field = field;
            _center = center;
            _decay = decay;
        }

        public void Sample(ref Float2 position, out Tensor result)
        {
            var exp = DistanceDecay(_decay, (position - _center).SquareMagnitude());

            Tensor sample;
            _field.Sample(ref position, out sample);

            result = exp * sample;
        }

        public static float DistanceDecay(float distanceSqr, float decay)
        {
            return (float)Math.Exp(-decay * distanceSqr);
        }
    }
}
